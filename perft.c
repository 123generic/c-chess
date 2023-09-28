#include "common.h"
#include "board.h"
#include "lookup.h"
#include "movegen.h"
#include "makemove.h"
#include "rng.h"

#include <stdio.h>
#include <assert.h>

U64 perft(ChessBoard board, int depth) {
	if (depth == 0) return 1;

	U64 total_moves = 0;
	U64 moves[256];
	int num_moves;

	MoveGenStage stage[] = {promotions, captures, castling, quiets};
	int len = sizeof(stage) / sizeof(stage[0]);
	for (int i = 0; i < len; i++) {
		num_moves = generate_moves(&board, moves, attackers(&board, !board.side), stage[i]);

		for (int move_p = 0; move_p < num_moves; move_p++) {
			ChessBoard new_board = make_move(board, moves[move_p]);
			#ifdef DEBUG
			assert(new_board.hash == manual_compute_hash(&new_board));
			#endif
			if (is_legal(&new_board, attackers(&new_board, new_board.side), !new_board.side)) {
				total_moves += perft(new_board, depth - 1);
			}
		}
	}

	return total_moves;
}

U64 _test_zobrist_helper(ChessBoard board, int depth) {
	if (depth == 0) return 1;

	U64 total_moves = 0;
	U64 moves[256];
	int num_moves;

	MoveGenStage stage[] = {promotions, captures, castling, quiets};
	int len = sizeof(stage) / sizeof(stage[0]);
	for (int i = 0; i < len; i++) {
		num_moves = generate_moves(&board, moves, attackers(&board, !board.side), stage[i]);

		for (int move_p = 0; move_p < num_moves; move_p++) {
			ChessBoard new_board = make_move(board, moves[move_p]);
			assert(new_board.hash == manual_compute_hash(&new_board));
			if (is_legal(&new_board, attackers(&new_board, new_board.side), !new_board.side)) {
				total_moves += perft(new_board, depth - 1);
			}
		}
	}

	return total_moves;
}

U64 divide(ChessBoard board, int depth) {
	if (depth == 0) return 1;

	U64 total_moves = 0;
	U64 moves[256];
	int num_moves;

	MoveGenStage stage[] = {promotions, captures, castling, quiets};
	int len = sizeof(stage) / sizeof(stage[0]);
	for (int i = 0; i < len; i++) {
		num_moves = generate_moves(&board, moves, attackers(&board, !board.side), stage[i]);

		for (int move_p = 0; move_p < num_moves; move_p++) {
			ChessBoard new_board = make_move(board, moves[move_p]);
			if (is_legal(&new_board, attackers(&new_board, new_board.side), !new_board.side)) {
				U64 nodes = perft(new_board, depth - 1);
				char ascii_move[6];
				move_to_uci(moves[move_p], ascii_move);
				printf("%s: %llu\n", ascii_move, nodes);
				total_moves += nodes;
			}
		}
	}

	return total_moves;
}

int main(void) {
	global_init();

	ChessBoard board;
	ChessBoard_from_FEN(&board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
	// ChessBoard_from_FEN(&board, "k7/8/8/8/8/8/8/K7 w - - 0 1");

	for (int depth = 1; depth <= 5; depth++) {
		U64 nodes = perft(board, depth);
		printf("Depth %d: %llu\n", depth, nodes);
	}

	return 0;
}
