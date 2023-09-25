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
	// ChessBoard_from_FEN(&board, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
	ChessBoard_from_FEN(&board, "rnQq1k1r/pp2bppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R b KQ - 1 8");

	// for (int depth = 1; depth <= 7; depth++) {
	// 	U64 nodes = perft(board, &lookup, depth);
	// 	printf("Depth %d: %llu\n", depth, nodes);
	// }

	U64 total = divide(board, &lookup, 1);
	printf("Total: %llu\n", total);

	return 0;
}
