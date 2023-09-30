#include "board.h"
#include "eval.h"
#include "movegen.h"
#include "makemove.h"

#include <stdio.h>

const i16 INF = 20000;
const i16 out_of_time = INF - 1;
const u64 max_nodes = 100000000;

u64 nodes = 0;
i16 alphabeta(ChessBoard board, i16 alpha, i16 beta, u16 depth, u64 *best_move) {
	nodes++;

	if (nodes > max_nodes) {
		return -out_of_time;
	}

	if (depth == 0) {
		return eval(&board);
	}

	int legal_moves = 0;
	i16 best_score = -INF;
	*best_move = 0;

	MoveGenStage stage[] = {promotions, captures, castling, quiets};
	int len = sizeof(stage) / sizeof(stage[0]);

	u64 moves[256];
	u64 attack_mask = attackers(&board, !board.side);
	for (int i = 0; i < len; i++) {
		int num_moves = generate_moves(&board, moves, attack_mask, stage[i]);

		for (int move_p = 0; move_p < num_moves; move_p++) {
			ChessBoard new_board = make_move(board, moves[move_p]);
			if (is_legal(&new_board, attackers(&new_board, new_board.side), !new_board.side)) {
				legal_moves++;

				// recurse
				u64 _move;
				i16 score = -alphabeta(new_board, -beta, -alpha, depth - 1, &_move);
				
				// time management
				if (score == out_of_time) {
					return -out_of_time;
				}

				// beta cutoff
				if (score >= beta) {
					return beta;
				}

				// raise alpha
				alpha = score > alpha ? score : alpha;

				// minimax stuff
				if (score > best_score) {
					best_score = score;
					*best_move = moves[move_p];
				}
			}
		}
	}

	if (legal_moves == 0) {
		// checkmate
		if (attack_mask & board.bitboards[board.side + king])
			return -INF;
		
		// stalemate
		return 0;
	}

	return best_score;
}

i16 iterative_deepening(ChessBoard board) {
	nodes = 0;  // vital, used to cutoff for time
	u64 best_move;
	i16 best_score = -INF;
	for (int depth = 1; /* true */; depth++) {
		i16 score = alphabeta(board, -INF, INF, depth, &best_move);

		if (score == -out_of_time) {
			return best_score;
		}

		best_score = score;

		// logging
		char ascii_move[5];
		move_to_uci(best_move, ascii_move);

		printf("depth: %d, score: %d, best move: %s\n", depth, score, ascii_move);
	}
}

int main(void) {
	global_init();

	ChessBoard board;
	// char starting_fen[] = "r1bqkb1r/pppp1ppp/2n5/1B2p3/4n3/5N2/PPPP1PPP/RNBQ1RK1 w kq - 0 5";
	char starting_fen[] = "r1bqkb1r/pppp1ppp/2n5/1B2p3/4n3/5N2/PPPP1PPP/RNBQR1K1 b kq - 1 5";
	ChessBoard_from_FEN(&board, starting_fen);
	ChessBoard_print(&board);

	iterative_deepening(board);
	printf("Finished search\n");
}
