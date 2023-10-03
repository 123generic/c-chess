#include "search.h"

#include <stdio.h>
#include <assert.h>

#include "board.h"
#include "common.h"
#include "eval.h"
#include "hash_table.h"
#include "makemove.h"
#include "movegen.h"

const i16 MATE = 30000;
const u64 max_nodes = 400000000;
const int max_depth = 256;
const u64 NULL_MOVE = 0;
const int history_max = 2000;

// Killer table
void store_killer(KillerTable *killer_table, u16 ply, u64 move) {
	if ((killer_table[ply].move1 & 0xFFFFFFF) == (move & 0xFFFFFFF)) return;
	killer_table[ply].move2 = killer_table[ply].move1;
	killer_table[ply].move1 = move;
}

// History
void store_history(ChessBoard *board, int *history_table, u64 move, int depth) {
	int ind = board->side * 64 * 64 + from(move) * 64 + to(move);
	history_table[ind] += depth * depth;
	if (history_table[ind] > history_max) {
		for (int i = 0; i < 64 * 64 * 2; i++)
			history_table[i] /= 2;
	}
}

void decrease_history(ChessBoard *board, int *history_table, int depth, u64 *moves, int num_moves, u64 move) {
	for (int j = 0; j < num_moves; j++)
		if ((moves[j] & 0xFFFFFFF) != (move & 0xFFFFFFF)) {
			int ind = board->side * 64 * 64 + from(moves[j]) * 64 + to(moves[j]);
			history_table[ind] -= depth * depth / 3;
			history_table[ind] = history_table[ind] < 0 ? 0 : history_table[ind];
		}
}

// Extract PV
int extract_pv(ChessBoard board, u64 *pv_list) {
	int num_pv = 0;
	u64 entry;
	while ((entry = probe(board.hash))) {
		pv_list[num_pv++] = hf_move(entry);
		board = make_move(board, hf_move(entry));
	}
	
	return num_pv;
}

void print_pv(u64 *pv_list, int num_pv) {
	printf("PV: ");
	for (int i = 0; i < num_pv; i++) {
		char uci[6];
		move_to_uci(pv_list[i], uci);
		printf("%s ", uci);
	}
	printf("\n");
}


// Search
u64 nodes = 0, null_prunes = 0;
u64 stage_hash = 0, stage_capture = 0, stage_quiet = 0, stage_losing = 0;
u64 cut_hash = 0, cut_capture = 0, cut_quiet = 0, cut_losing = 0;

u64 cut_nodes = 0, first_cut = 0;
i16 alphabeta(int PV,
			  ChessBoard board, 
			  KillerTable *killer_table, 
			  u64 *counter_move, 
			  u64 prev_move,
			  int *history_table,
			  u64 attack_mask,
			  i16 alpha, 
			  i16 beta,
			  u16 depth,
              u16 ply, 
			  u64 *best_move) {
    // Recursive base case
    if (depth == 0) {
        return quiescence(board, alpha, beta, ply);
    }

    // Time management
    nodes++;
    if (nodes > max_nodes) {
        return -out_of_time;
    }

    i16 old_alpha = alpha;

    // Null Move Pruning
    if (!PV && !zugzwang(&board, attack_mask)) {
        ChessBoard new_board = null_move(board);
        u64 new_attack_mask = attackers(&new_board, !new_board.side);
        u64 _move;
        u16 new_depth = depth < 3 ? 0 : depth - 3;
        i16 score = -alphabeta(0, new_board, killer_table, counter_move, NULL_MOVE, history_table, new_attack_mask, -beta, -beta + 1,
                               new_depth, ply + 1, &_move);
        if (score == out_of_time) {
            return -out_of_time;
        }
        if (score >= beta) {
            store(board.hash, lower, beta, depth, 0);
			null_prunes++;
            return beta;
        }
    }

    // Transposition table lookup
    u64 entry = 0, hash_move = 0;
    if ((entry = probe(board.hash)) && hf_depth(entry) >= depth) {
        hash_flag_t flag = hf_flag(entry);
        i16 score = hf_score(entry);
        hash_move = flag != higher ? hf_move(entry) : 0;

        if (flag == exact) {
            *best_move = hash_move;
            return score;
        } else if (flag == lower) {
            alpha = score > alpha ? score : alpha;
        } else if (flag == higher) {
            beta = score < beta ? score : beta;
        }

        // beta cutoff
        if (alpha > beta && flag == lower) {
            store(board.hash, lower, alpha, depth, hash_move);
            return alpha;
		// raise alpha
        } else if (alpha > beta) {
            store(board.hash, higher, beta, depth, hash_move);
            return beta;
        }
    }

    // Start Search
    int legal_moves = 0;
    i16 best_score = -INF;
    *best_move = 0;

    // Hash move
    if (hash_move) {
        stage_hash++;
        ChessBoard new_board = make_move(board, hash_move);
        if (is_legal(&new_board, attackers(&new_board, new_board.side),
                     !new_board.side)) {
            legal_moves++;
            u64 _move;
            i16 score =
                -alphabeta(PV, new_board, killer_table, counter_move, hash_move, history_table, attackers(&new_board, !new_board.side),
                           -beta, -alpha, depth - 1, ply + 1, &_move);
            if (score == out_of_time) {
                return -out_of_time;
            }
            if (score >= beta) {
                cut_hash++;
				cut_nodes++;
				first_cut += legal_moves == 1;
                store(board.hash, lower, beta, depth, hash_move);
				if (captured(hash_move) == empty) {
					store_killer(killer_table, ply, hash_move);
					store_history(&board, history_table, hash_move, depth);
					if (prev_move)
						counter_move[from(prev_move) * 64 + to(prev_move)] = (hash_move & 0xFFFFFFF);
				}
                return beta;
            }
			if (score > alpha) {
				alpha = score;
				store_history(&board, history_table, hash_move, depth);
			}
            if (score > best_score) {
                best_score = score;
                *best_move = hash_move;
            }
        }
    }

    MoveGenStage stage[] = {promotions, captures, castling, quiets, losing};
    int len = sizeof(stage) / sizeof(stage[0]);

    u64 moves[256];
	int alpha_raised = 0;
    for (int i = 0; i < len; i++) {
		int stage_moves = 0;
        int num_moves = generate_moves(&board, moves, attack_mask, stage[i]);
		int moves_left = num_moves;
		sort_moves(&board, attack_mask, moves, num_moves, &killer_table[ply], counter_move, prev_move, history_table, stage[i]);
		while (moves_left) {
			u64 move = select_move(moves, moves_left--);
			if (!move) break;

            ChessBoard new_board = make_move(board, move);
            if (is_legal(&new_board, attackers(&new_board, new_board.side),
                         !new_board.side)) {
                legal_moves++;
				stage_moves++;

                u64 new_attack_mask = attackers(&new_board, !new_board.side);
                i16 E = 0, R = 0;

				int delivering_check = new_attack_mask & new_board.bitboards[new_board.side + king];
				int attacker_attacked = attack_mask & BB_SQUARE(to(move));

				// check extension
                if (delivering_check && ~attacker_attacked) E++;
				
				// standard alpha beta
				u64 _move;
				int new_depth = depth + E - R - 1 < 0 ? 0 : depth + E - R - 1;
				int is_pv = PV && legal_moves == 1 || alpha_raised;
				i16 score = -alphabeta(is_pv, new_board, killer_table, counter_move, move, history_table, new_attack_mask, -beta,
									-alpha, new_depth, ply + 1, &_move);
				
				// time management
				if (score == out_of_time) {
					return -out_of_time;
				}

                // beta cutoff
                if (score >= beta) {
                    store(board.hash, lower, beta, depth, move);

					cut_nodes++;
					first_cut += legal_moves == 1;

					if (stage[i] == quiets) {
						store_killer(killer_table, ply, move);
						if (prev_move)
							counter_move[from(prev_move) * 64 + to(prev_move)] = (move & 0xFFFFFFF);

						// decrease history
						decrease_history(&board, history_table, depth, moves, num_moves, move);
						store_history(&board, history_table, hash_move, depth);
					}

					switch (stage[i]) {
						case promotions:
							break;
						case captures:
							stage_capture++;
							cut_capture += stage_moves;
							break;
						case castling:
							break;
						case quiets:
							stage_quiet++;
							cut_quiet += stage_moves;
							break;
						case losing:
							stage_losing++;
							cut_losing += stage_moves;
							break;
					}
                    return beta;
                }

                // raise alpha
				if (score > alpha) {
					alpha = score;
					alpha_raised = 1;
					store_history(&board, history_table, move, depth);
				}

                // minimax stuff
                if (score > best_score) {
                    best_score = score;
                    *best_move = move;
                }
            }
        }
    }

    // Stalemate and checkmate detection
    if (legal_moves == 0) {
        if (attack_mask & board.bitboards[board.side + king]) {
            store(board.hash, exact, -MATE + ply, depth, 0);
            return -MATE + ply;
        }

        store(board.hash, exact, 0, depth, 0);
        return 0;
    }

    // Store Transposition Table
    if (best_score > old_alpha)
        store(board.hash, exact, best_score, depth, *best_move);
    else
        store(board.hash, higher, best_score, depth, 0);

    return best_score;
}

u64 qnodes = 0, q_stage = 0, q_cut = 0;
i16 quiescence(ChessBoard board, i16 alpha, i16 beta, u16 ply) {
    qnodes++;
    nodes++;

    if (nodes > max_nodes) return -out_of_time;

    i16 stand_pat = eval(&board);
    if (stand_pat >= beta) return beta;

    alpha = stand_pat > alpha ? stand_pat : alpha;
    i16 best_score = -INF;
    u64 moves[256];
    u64 attack_mask = attackers(&board, !board.side);
    int num_moves = generate_moves(&board, moves, attack_mask, captures);
	sort_moves(&board, attack_mask, moves, num_moves, NULL, NULL, NULL_MOVE, NULL, captures);
	int legal_moves = 0;
    while(num_moves) {
		u64 move = select_move(moves, num_moves--);
		if (!move) break;
        ChessBoard new_board = make_move(board, move);
        if (is_legal(&new_board, attackers(&new_board, new_board.side),
                     !new_board.side)) {
			legal_moves++;
			i16 score = -quiescence(new_board, -beta, -alpha, ply + 1);
			if (score == out_of_time) 
				return -out_of_time;

            if (score >= beta) {
				q_stage++;
				q_cut += legal_moves;
				return beta;
			}
            if (score > best_score) 
				best_score = score;
			alpha = score > alpha ? score : alpha;
        }
    }

    return best_score;
}

i16 iterative_deepening(ChessBoard board) {
    nodes = 0;  // vital, used to cutoff for time
    u64 best_move;
    i16 best_score = -INF;

	KillerTable killer_table[256] = {0};
	assert(max_depth == 256);
	u64 counter_move[64 * 64] = {0};
	int history_table[64 * 64 * 2] = {0};

    for (int depth = 1; depth < max_depth; depth++) {
		// logging init
        qnodes = 0;
		null_prunes = 0, 
		stage_hash = 0, stage_capture = 0, stage_quiet = 0, stage_losing = 0, q_stage = 0;
		cut_hash = 0, cut_capture = 0, cut_quiet = 0, cut_losing = 0, q_cut = 0;

		cut_nodes = 0, first_cut = 0;
        
		u64 attack_mask = attackers(&board, !board.side);
        i16 score =
            alphabeta(1, board, killer_table, counter_move, NULL_MOVE, history_table, attack_mask, -INF, INF, depth, 0, &best_move);

        if (score == -out_of_time) {
            return best_score;
        }

        best_score = score;

		// PV
		u64 pv_list[256];
		assert(max_depth <= 256);
		int num_pv = extract_pv(board, pv_list);

        // logging
        char ascii_move[5];
        move_to_uci(best_move, ascii_move);

        printf("depth: %d, nodes: %llu, score: %d, best move: %s\n", depth,
               nodes, score, ascii_move);
        printf("qnodes: %llu\n", qnodes);
		printf("null prunes: %llu\n", null_prunes);
		printf("stages: hash — %llu, capture — %llu, quiet — %llu, losing — %llu, q — %llu\n", stage_hash, stage_capture, stage_quiet, stage_losing, q_stage);
		printf("cuts: hash — %llu, capture — %llu, quiet — %llu, losing — %llu, q — %llu\n", cut_hash, cut_capture, cut_quiet, cut_losing, q_cut);
		printf("cut nodes: %llu, first cut: %llu\n", cut_nodes, first_cut);
		print_pv(pv_list, num_pv);
		printf("\n");
    }

	return best_score;
}

int main(void) {
    global_init();

    ChessBoard board;
    char starting_fen[] =
        "r1bqkb1r/pppp1ppp/2n5/1B2p3/4n3/5N2/PPPP1PPP/RNBQ1RK1 w kq - 0 5";

    ChessBoard_from_FEN(&board, starting_fen);
    ChessBoard_print(&board);

    iterative_deepening(board);
    printf("Finished search\n");
}
