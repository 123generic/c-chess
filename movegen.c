#include "movegen.h"

#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "magic.h"

// Move generation
// moves are packed into a U64 (LSB -> MSB)
// 0-5: from square
// 6-11: to square
// 12-15: piece
// 16-19: capture piece
// 20-23: move type

U64 move_from_uci(ChessBoard *board, char *uci) {
    // uci: rank-file rank-file [promotion]
    // TODO: promotion
    int from, to, piece, captured;
    from = 7 - (uci[0] - 'a') + 8 * (uci[1] - '1');
    to = 7 - (uci[2] - 'a') + 8 * (uci[3] - '1');

    piece = ChessBoard_piece_at(board, from);
    captured = ChessBoard_piece_at(board, to);
    if (piece == 0) {
        fprintf(stderr, "Error: No piece at %s [%s(%s):%d]\n", uci, __FILE__,
                __func__, __LINE__);
        exit(1);
    }
    return from | to << 6 | piece << 12 | captured << 16 | UNKNOWN << 20;
}

// uci must have length 4+1 (or more)
void move_to_uci(U64 move, char *uci) {
	int from, to;
	from = move & 0x3f;
	to = (move >> 6) & 0x3f;

	uci[0] = 'a' + 7 - (from % 8);
	uci[1] = '1' + (from / 8);
	uci[2] = 'a' + 7 - (to % 8);
	uci[3] = '1' + (to / 8);
	uci[4] = '\0';
}

// move_p points to the end of the moves array
// moves is assumed to be large enough to hold all moves (256)
U64 get_pawn_moves(ChessBoard *board, MoveType move_type) {
    U64 pawns, moved_pawns, rank_mask;
    int wtm = board->white_to_move;

    switch (move_type) {
        case SINGLE_PUSH:
            pawns = wtm ? board->white_pawns : board->black_pawns;

            // no promotions
            rank_mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & ~rank_mask;

            // shift one rank up/down
            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;

            // don't push into pieces
            moved_pawns = moved_pawns & ~board->all_pieces;
            break;

        default:
            printf("Error: Unimplemented pawn move type [%s(%s):%d]\n",
                   __FILE__, __func__, __LINE__);
            exit(1);
    }

    return moved_pawns;
}

int extract_pawn_moves(ChessBoard *board, U64 *moves, int move_p,
                       U64 pawn_moves, MoveType move_type) {
    int ind, to, from, piece, captured;
    int num_moves = 0;
    int wtm = board->white_to_move;

    switch (move_type) {
        case SINGLE_PUSH:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                // this means the move is ind + 8 -> ind
                to = ind;
                from = wtm ? ind - 8 : ind + 8;
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = EMPTY_SQ;
                moves[move_p + num_moves] = from | to << 6 | piece << 12 |
                                            captured << 16 | move_type << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        default:
            printf("Error: Unimplemented pawn move type [%s(%s):%d]\n",
                   __FILE__, __func__, __LINE__);
            exit(1);
    }

    return num_moves;
}

// board[sq] should, of course, have a rook on it
U64 get_rook_moves_sq(ChessBoard *board, MagicTable *magic_table, int sq) {
    U64 pieces, mask, magic, moves, friendlies;
    int ind;

    pieces = board->all_pieces;
    mask = magic_table->occupancy_mask[sq];
    magic = magic_table->magic[sq];
    ind = ((pieces & mask) * magic) >> 52;

    moves = magic_table->move[4096 * sq + ind];

	friendlies = board->white_to_move ? board->white_pieces : board->black_pieces;
	moves &= ~friendlies;
	return moves;
}

int extract_rook_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                          U64 rook_moves, int sq) {
    int ind, to, piece, captured;
    int wtm = board->white_to_move;
    int num_moves = 0;

    while ((ind = rightmost_set(rook_moves)) != -1) {
        // this means the move is ind + 8 -> ind
        to = ind;
        piece = wtm ? WHITE_ROOK : BLACK_ROOK;
        captured = ChessBoard_piece_at(board, ind);
        moves[move_p + num_moves] =
            sq | to << 6 | piece << 12 | captured << 16 | SLIDE << 20;
        num_moves++;
        BB_CLEAR(rook_moves, ind);
    }

    return num_moves;
}

int extract_rook_moves(ChessBoard *board, MagicTable *magic_table, U64 *moves,
                       int move_p) {
    int sq;
    int num_moves = 0;
    U64 rook_moves;
    U64 bb = board->white_to_move ? board->white_rooks : board->black_rooks;

	while ((sq = rightmost_set(bb)) != -1) {
        rook_moves = get_rook_moves_sq(board, magic_table, sq);
        num_moves +=
            extract_rook_moves_sq(board, moves, move_p + num_moves, rook_moves, sq);

		BB_CLEAR(bb, sq);
    }

    return num_moves;
}
