#include "board.h"
#include "lookup.h"
#include "movegen.h"
#include "makemove.h"
#include "rng.h"

#include <stdio.h>

U64 perft(ChessBoard board, LookupTable *lookup, int depth) {
	if (depth == 0) return 1;

	U64 total_moves = 0;
	U64 moves[256];
	int num_moves;

	MoveGenStage stage[] = {promotions, captures, castling, quiets};
	int len = sizeof(stage) / sizeof(stage[0]);
	for (int i = 0; i < len; i++) {
		num_moves = generate_moves(&board, lookup, moves, attackers(&board, lookup, !board.side), stage[i]);

		for (int move_p = 0; move_p < num_moves; move_p++) {
			ChessBoard new_board = make_move(board, moves[move_p]);
			if (is_legal(&new_board, attackers(&new_board, lookup, new_board.side), !new_board.side)) {
				total_moves += perft(new_board, lookup, depth - 1);
			}
		}
	}

	return total_moves;
}

U64 divide(ChessBoard board, LookupTable *lookup, int depth) {
	if (depth == 0) return 1;

	U64 total_moves = 0;
	U64 moves[256];
	int num_moves;

	MoveGenStage stage[] = {promotions, captures, castling, quiets};
	int len = sizeof(stage) / sizeof(stage[0]);
	for (int i = 0; i < len; i++) {
		num_moves = generate_moves(&board, lookup, moves, attackers(&board, lookup, !board.side), stage[i]);

		for (int move_p = 0; move_p < num_moves; move_p++) {
			ChessBoard new_board = make_move(board, moves[move_p]);
			if (is_legal(&new_board, attackers(&new_board, lookup, new_board.side), !new_board.side)) {
				U64 nodes = perft(new_board, lookup, depth - 1);
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
	init_genrand64(0x8c364d19345930e2);  // drawn on random day

	LookupTable lookup;
	init_LookupTable(&lookup);

	ChessBoard board;
	ChessBoard_from_FEN(&board, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
	// ChessBoard_from_FEN(&board, "r3k2r/Pppp1ppp/1b3nbN/nPP5/BB2P3/q4N2/Pp1P2PP/R2Q1RK1 b kq - 0 1");
	// ChessBoard_from_FEN(&board, "r3k2r/Pppp1ppp/1b3nbN/nPP5/BB2P3/q4N2/P2P2PP/r2Q1RK1 w kq - 0 1");
	// ChessBoard_from_FEN(&board, "r3k2r/Pppp1ppp/1b3nbN/nPP5/BB2P3/q4N2/P2P2PP/Q4RK1 b kq - 0 1");

	// int from = 4, to = 7;
	// Piece piece = queen, captured = rook;
	// MoveType move_type = NORMAL;

	// U64 move = from | (to << 6) | (piece << 12) | (captured << 16) | (move_type << 20);

	// board = make_move(board, move);

	for (int depth = 1; depth <= 6; depth++) {
		U64 nodes = perft(board, &lookup, depth);
		printf("Depth %d: %llu\n", depth, nodes);
	}

	// U64 total = divide(board, &lookup, 5);
	// printf("Total: %llu\n", total);

	return 0;
}
