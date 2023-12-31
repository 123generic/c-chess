#include "search.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "board.h"
#include "common.h"
#include "eval.h"
#include "hash_table.h"
#include "makemove.h"
#include "movegen.h"

const i16 MATE = 30000;
const u64 max_nodes = 35000000;  // ~4 seconds
const int max_depth = 256;
const u64 NULL_MOVE = 0;

// Timing Utilities
void print_time(void) {
    struct timeval tv;
    struct tm *timeinfo;
    char buffer[80];

    gettimeofday(&tv, NULL);
    timeinfo = localtime(&tv.tv_sec);

    strftime(buffer, sizeof(buffer), "%M:%S", timeinfo);
    printf("Current time: %s:%03d\n", buffer, tv.tv_usec / 1000);
}

struct timeval get_current_time(void) {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp;
}

int time_exceeded(struct timeval start_time, double duration) {
    struct timeval current_time = get_current_time();
    double elapsed_time =
        (current_time.tv_sec - start_time.tv_sec) +
        (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
    return elapsed_time > duration;
}

// Killer table
void store_killer(KillerTable *killer_table, u16 ply, u64 move) {
    if ((killer_table[ply].move1 & 0xFFFFFFF) == (move & 0xFFFFFFF)) return;
    killer_table[ply].move2 = killer_table[ply].move1;
    killer_table[ply].move1 = move;
}

// Extract PV
int extract_pv(ChessBoard board, u64 *pv_list, int max_pv) {
    int num_pv = 0;
    u64 entry;
    while (num_pv < max_pv && (entry = probe(board.hash))) {
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
u64 lmr_attempts = 0, lmr_fails = 0;
i16 alphabeta(int PV, ChessBoard board, KillerTable *killer_table,
              u64 *counter_move, u64 prev_move, u64 attack_mask, i16 alpha,
              i16 beta, u16 depth, u16 ply, u64 *best_move,
              struct timeval start_time, double duration) {
    // Recursive base case
    if (depth == 0) {
        return quiescence(board, alpha, beta, ply, start_time, duration);
    }

    nodes++;

    // Time management
    if (time_exceeded(start_time, duration)) {
        return -out_of_time;
    }

    i16 old_alpha = alpha;

    // Null Move Pruning
    if (!PV && !zugzwang(&board, attack_mask)) {
        ChessBoard new_board = null_move(board);
        u64 new_attack_mask = attackers(&new_board, !new_board.side);
        u64 _move;
        u16 new_depth = depth < 3 ? 0 : depth - 3;
        i16 score =
            -alphabeta(0, new_board, killer_table, counter_move, NULL_MOVE,
                       new_attack_mask, -beta, -beta + 1, new_depth, ply + 1,
                       &_move, start_time, duration);
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
            i16 score = -alphabeta(
                PV, new_board, killer_table, counter_move, hash_move,
                attackers(&new_board, !new_board.side), -beta, -alpha,
                depth - 1, ply + 1, &_move, start_time, duration);
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
                    if (prev_move)
                        counter_move[from(prev_move) * 64 + to(prev_move)] =
                            (hash_move & 0xFFFFFFF);
                }
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
            if (score > best_score) {
                best_score = score;
                *best_move = hash_move;
            }
        }
    }

    MoveGenStage stage[] = {promotions, captures, quiets, castling, losing};
    int len = sizeof(stage) / sizeof(stage[0]);

    u64 moves[256];
    int alpha_raised = 0;
    for (int i = 0; i < len; i++) {
        int prior_good_moves = 0;
        if (stage[i] == quiets) prior_good_moves = legal_moves;

        int stage_moves = 0;
        int num_moves = generate_moves(&board, moves, attack_mask, stage[i]);
        int moves_left = num_moves;
        sort_moves(&board, attack_mask, moves, num_moves, killer_table,
                   counter_move, prev_move, stage[i], ply);
        while (moves_left) {
            u64 move = select_move(moves, moves_left--);
            if (!move) break;

            ChessBoard new_board = make_move(board, move);
            if (is_legal(&new_board, attackers(&new_board, new_board.side),
                         !new_board.side)) {
                legal_moves++;
                stage_moves++;

                u64 new_attack_mask = attackers(&new_board, !new_board.side);

                int delivering_check =
                    new_attack_mask &
                    new_board.bitboards[new_board.side + king];
                int in_check = attack_mask & board.bitboards[board.side + king];
                int attacker_attacked = attack_mask & BB_SQUARE(to(move));

                // check extension
                i16 E = 0;
                if (delivering_check && ~attacker_attacked) E++;

				// reductions
				int R = 0;

				// Futility "pruning"
				if (depth == 2 && E == 0 && !in_check && eval(&board) + 50 < alpha)
					R++;

                // LMR
                u64 _move;
                i16 score;
                int new_depth = depth + E - R - 1 < 0 ? 0 : depth + E - R - 1;
                int is_pv = (legal_moves == 1) || alpha_raised;
                int lmr_legal_moves = prior_good_moves > 0 ? 0 : 1;
                int can_lmr = (stage[i] == quiets || stage[i] == losing) &&
                              depth >= 2 && E == 0 && !in_check &&
                              legal_moves > lmr_legal_moves;
                if (can_lmr) {
                    lmr_attempts++;
                    int lmr_reduce = ((int)sqrt((double)(depth - 1)) +
                                	  (int)sqrt((double)(legal_moves - 1)));
					lmr_reduce = is_pv ? lmr_reduce * 0.5 : lmr_reduce;
                    u16 reduced_depth = new_depth - lmr_reduce < 0 ? 0 : new_depth - lmr_reduce;
                    score = -alphabeta(0, new_board, killer_table, counter_move,
                                       move, new_attack_mask, -(alpha + 1),
                                       -alpha, reduced_depth, ply + 1, &_move,
                                       start_time, duration);
                    if (score == out_of_time) return -out_of_time;

                    if (score > alpha && score < beta) {
                        lmr_fails++;
                        score = -alphabeta(is_pv, new_board, killer_table,
                                           counter_move, move, new_attack_mask,
                                           -beta, -alpha, new_depth, ply + 1,
                                           &_move, start_time, duration);
                    }
                } else {
                    score = -alphabeta(is_pv, new_board, killer_table,
                                       counter_move, move, new_attack_mask,
                                       -beta, -alpha, new_depth, ply + 1,
                                       &_move, start_time, duration);
                }

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
                            counter_move[from(prev_move) * 64 + to(prev_move)] =
                                (move & 0xFFFFFFF);
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
            store(board.hash, exact, -INF + ply, depth, 0);
            return -INF + ply;
        }

        store(board.hash, exact, 0, depth, 0);
        return 0;
    }

    // Store Transposition Table
    if (best_score > old_alpha) {
        store(board.hash, exact, best_score, depth, *best_move);
    } else
        store(board.hash, higher, best_score, depth, 0);

    return best_score;
}

u64 qnodes = 0, q_stage = 0, q_cut = 0;
i16 quiescence(ChessBoard board, i16 alpha, i16 beta, u16 ply,
               struct timeval start_time, double duration) {
    qnodes++;

    if (time_exceeded(start_time, duration)) return -out_of_time;

    i16 stand_pat = eval(&board);
    if (stand_pat >= beta) return beta;

	// Delta pruning
	if (stand_pat < alpha - 900) return alpha;

    alpha = stand_pat > alpha ? stand_pat : alpha;
    i16 best_score = stand_pat;
    u64 moves[256];
    u64 attack_mask = attackers(&board, !board.side);
    int num_moves = generate_moves(&board, moves, attack_mask, captures);
    sort_moves(&board, attack_mask, moves, num_moves, NULL, NULL, NULL_MOVE,
               captures, ply);
    int legal_moves = 0;
    while (num_moves) {
        u64 move = select_move(moves, num_moves--);
        if (!move) break;
        ChessBoard new_board = make_move(board, move);
        if (is_legal(&new_board, attackers(&new_board, new_board.side),
                     !new_board.side)) {
            legal_moves++;
            i16 score = -quiescence(new_board, -beta, -alpha, ply + 1,
                                    start_time, duration);
            if (score == out_of_time) return -out_of_time;

            if (score >= beta) {
                q_stage++;
                q_cut += legal_moves;
                return beta;
            }
            if (score > best_score) best_score = score;
            alpha = score > alpha ? score : alpha;
        }
    }

    return best_score;
}

void write_pv(u64 *pv_list, int num_pv, char *pv_buf) {
    int buf_p = 0;
    for (int i = 0; i < num_pv; i++) {
        u64 move = pv_list[i];
        char move_buf[6] = {0};
        move_to_uci(move, move_buf);

        // TODO: bug where h1h1 gets into pv ((0, 0) move)
        if (strcmp(move_buf, "h1h1") == 0) {
            break;
        }

        for (int j = 0; move_buf[j]; j++) {
            pv_buf[buf_p++] = move_buf[j];
        }
        pv_buf[buf_p++] = ' ';
    }
    pv_buf[buf_p - 1] = 0;  // overwrite last space with null
}

void uci_search(ChessBoard board, double duration) {
    nodes = 0;
    u64 best_move;
    u64 pv_list[256];
    i16 best_score = -INF;

    KillerTable killer_table[256] = {0};
    u64 counter_move[64 * 64] = {0};

    struct timeval start_time = get_current_time();

    for (int depth = 1; depth < max_depth; depth++) {
        u64 attack_mask = attackers(&board, !board.side);
        i16 score = alphabeta(1, board, killer_table, counter_move, NULL_MOVE,
                              attack_mask, -INF, INF, depth, 0, &best_move,
                              start_time, duration);

        if (score == -out_of_time) break;
        best_score = score;

        // PV
        int num_pv = extract_pv(board, pv_list, depth);

        // Write PV
        int pv_buf_len = num_pv * 7 + 1;
        char *pv_buf = malloc(sizeof(char) * pv_buf_len);
        write_pv(pv_list, num_pv, pv_buf);

        // logging
        printf("info depth %d score cp %d pv %s\n", depth, best_score, pv_buf);
        fflush(stdout);

        free(pv_buf);
    }

    char best_move_str[6] = {0};
    move_to_uci(pv_list[0], best_move_str);
    printf("bestmove %s\n", best_move_str);
    fflush(stdout);
}

i16 iterative_deepening(ChessBoard board) {
    u64 best_move;
    i16 best_score = -INF;

    KillerTable killer_table[256] = {0};
    assert(max_depth == 256);
    u64 counter_move[64 * 64] = {0};
    struct timeval start_time = get_current_time();
    const double duration = 400;  // constant for debugging

    for (int depth = 1; depth < max_depth; depth++) {
        // logging init
        nodes = 0, qnodes = 0;
        null_prunes = 0, stage_hash = 0, stage_capture = 0, stage_quiet = 0,
        stage_losing = 0, q_stage = 0;
        cut_hash = 0, cut_capture = 0, cut_quiet = 0, cut_losing = 0, q_cut = 0;

        cut_nodes = 0, first_cut = 0;
        lmr_attempts = 0, lmr_fails = 0;

        u64 attack_mask = attackers(&board, !board.side);
        i16 score = alphabeta(1, board, killer_table, counter_move, NULL_MOVE,
                              attack_mask, -INF, INF, depth, 0, &best_move,
                              start_time, duration);

        if (score == -out_of_time) {
            return best_score;
        }

        best_score = score;

        // PV
        u64 pv_list[256];
        assert(max_depth <= 256);
        int num_pv = extract_pv(board, pv_list, depth);

        // logging
        char ascii_move[5];
        move_to_uci(best_move, ascii_move);

        printf("depth: %d, nodes: %llu, score: %d, best move: %s\n", depth,
               nodes, score, ascii_move);
        printf("qnodes: %llu\n", qnodes);
        printf("null prunes: %llu\n", null_prunes);
        printf(
            "stages: hash — %llu, capture — %llu, quiet — %llu, losing — %llu, "
            "q — %llu\n",
            stage_hash, stage_capture, stage_quiet, stage_losing, q_stage);
        printf(
            "cuts: hash — %llu, capture — %llu, quiet — %llu, losing — %llu, q "
            "— %llu\n",
            cut_hash, cut_capture, cut_quiet, cut_losing, q_cut);
        printf("cut nodes: %llu, first cut: %llu\n", cut_nodes, first_cut);
        printf("lmr attempts: %llu, lmr fails: %llu\n", lmr_attempts,
               lmr_fails);
        print_pv(pv_list, num_pv);
        printf("\n");
    }

    return best_score;
}

void debug(void) {
    global_init();

    ChessBoard board;
    char starting_fen[] =
		"rnb1kb1r/ppp1pppp/3q1n2/8/2BP4/2N5/PPP2PPP/R1BQK1NR b KQkq - 2 5";
        // "r1bqkb1r/pppp1ppp/2n5/1B2p3/4n3/5N2/PPPP1PPP/RNBQ1RK1 w kq - 0 5";
		// "b2b1r1k/3R1ppp/4qP2/4p1PQ/4P3/5B2/4N1K1/8 w - - 0 1";
    // "r1bqkb1r/pppp1ppp/2n5/1B2p3/4n3/5N2/PPPP1PPP/RNBQR1K1 b kq - 1 5";
    // "r1bqkb1r/pppp1ppp/2nn4/1B2p3/8/5N2/PPPP1PPP/RNBQR1K1 w kq - 2 6";
    // "r1bqkb1r/pppp1ppp/2nn4/1B2N3/8/8/PPPP1PPP/RNBQR1K1 b kq - 0 6";
    // "r1bqkb1r/pppp1ppp/3n4/1B2n3/8/8/PPPP1PPP/RNBQR1K1 w kq - 0 7";
    // "r1bqkb1r/pppp1ppp/3n4/1B2R3/8/8/PPPP1PPP/RNBQ2K1 b kq - 0 7";
    // "r1bqk2r/ppppbppp/3n4/1B2R3/8/8/PPPP1PPP/RNBQ2K1 w kq - 1 8";

    ChessBoard_from_FEN(&board, starting_fen);
    ChessBoard_print(&board);

    iterative_deepening(board);
    printf("Finished search\n");
}

#define MAX_INPUT_SIZE 1024

char version[] = "October Version 0.0.1";

void uci_listen(void) {
    global_init();

    ChessBoard start_board;
    ChessBoard_from_FEN(
        &start_board,
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    ChessBoard board = start_board;

    setbuf(stdout, NULL);  // no buffering

    char input[MAX_INPUT_SIZE];
    while (1) {
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "uci") == 0) {
            printf("id name %s\n", version);
            printf("uciok\n");
            continue;
        }

        if (strcmp(input, "isready") == 0) {
            printf("readyok\n");
            continue;
        }

        if (strcmp(input, "quit") == 0) {
            break;
        }

        if (strcmp(input, "display") == 0) {
            ChessBoard_print(&board);
            continue;
        }

        char first_word[10];
        sscanf(input, "%s ", first_word);

        if (strcmp(first_word, "go") == 0) {
            double duration = 4.0;
            char *token = strtok(input, " ");
            token = strtok(NULL, " ");  // wtime
            if (token) {
                int wtime = atoi(strtok(NULL, " "));
                strtok(NULL, " ");  // discard winc
                int winc = atoi(strtok(NULL, " "));
                strtok(NULL, " ");  // discard btime
                int btime = atoi(strtok(NULL, " "));
                strtok(NULL, " ");  // discard binc
                int binc = atoi(strtok(NULL, " "));
                strtok(NULL, " ");  // discard movestogo
                int movestogo = atoi(strtok(NULL, " "));

                double wduration = ((double)wtime / (movestogo + 1) + winc) / 1000;
                double bduration = ((double)btime / (movestogo + 1) + binc) / 1000;
                duration = board.side == white ? wduration : bduration;
				duration -= 0.05;  // error delta
            }
            uci_search(board, duration);
            continue;
        }

        if (strcmp(first_word, "position") != 0) {
            continue;
        }

        if (strstr(input, "position startpos")) {
            board = start_board;  // reset board

            char *token = strtok(input, " ");
            token = strtok(NULL, " ");
            token = strtok(NULL, " ");
            while (token != NULL) {
                u64 move = move_from_uci(&board, token);
                board = make_move(board, move);

                token = strtok(NULL, " ");
            }
        }

        else if (strstr(input, "position fen")) {
            int p = 0;
            for (int i = 0; i < 2; i++) {
                while (input[p] && input[p] != ' ') p++;

                p++;
            }

            char fen[100] = {0};
            int fen_p = 0;
            for (int i = 0; i < 6; i++) {
                while (input[p] && input[p] != ' ') {
                    fen[fen_p++] = input[p++];
                }

                fen[fen_p++] = input[p++];
            }
            fen[fen_p - 1] = 0;

            ChessBoard_from_FEN(&board, fen);

            char *token = strtok(&input[p], " ");
            token = strtok(NULL, " ");
            while (token != NULL) {
                u64 move = move_from_uci(&board, token);
                board = make_move(board, move);

                token = strtok(NULL, " ");
            }
        }
    }
}

int main(void) {
    uci_listen();
    // debug();
}
