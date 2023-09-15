#include "movegen.h"

#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "common.h"
#include "magic.h"

// Utilities
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

int get_piece(Piece p, int wtm) {
    switch (p) {
        case pawn:
            return wtm ? WHITE_PAWN : BLACK_PAWN;
        case rook:
            return wtm ? WHITE_ROOK : BLACK_ROOK;
        case knight:
            return wtm ? WHITE_KNIGHT : BLACK_KNIGHT;
        case bishop:
            return wtm ? WHITE_BISHOP : BLACK_BISHOP;
        case queen:
            return wtm ? WHITE_QUEEN : BLACK_QUEEN;
        case king:
            return wtm ? WHITE_KING : BLACK_KING;
    }
}

U64 get_bb(ChessBoard *board, Piece p, int wtm) {
    switch (p) {
        case pawn:
            return wtm ? board->white_pawns : board->black_pawns;
        case rook:
            return wtm ? board->white_rooks : board->black_rooks;
        case knight:
            return wtm ? board->white_knights : board->black_knights;
        case bishop:
            return wtm ? board->white_bishops : board->black_bishops;
        case queen:
            return wtm ? board->white_queens : board->black_queens;
        case king:
            return wtm ? board->white_king : board->black_king;
    }
}

// Pawn generation
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

// Magic Generation
// board[sq] should, of course, have a rook on it
U64 get_magic_moves_sq(ChessBoard *board, MagicTable *magic_table, int sq,
                       Piece piece) {
    U64 pieces, mask, magic, moves, friendlies;
    int ind, shift_amt;

    pieces = board->all_pieces;
    mask = magic_table->occupancy_mask[sq];
    magic = magic_table->magic[sq];
    shift_amt = piece == rook ? (64 - 12) : (64 - 9);
    ind = ((pieces & mask) * magic) >> shift_amt;

    if (piece == rook) {
        moves = magic_table->move[4096 * sq + ind];
    } else {
        moves = magic_table->move[512 * sq + ind];
    }

    friendlies =
        board->white_to_move ? board->white_pieces : board->black_pieces;
    moves &= (~friendlies);
    return moves;
}

int extract_magic_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                           U64 magic_moves, int sq, Piece p) {
    int ind, to, piece, captured;
    int num_moves = 0;

    piece = get_piece(p, board->white_to_move);

    while ((ind = rightmost_set(magic_moves)) != -1) {
        // this means the move is ind + 8 -> ind
        to = ind;
        captured = ChessBoard_piece_at(board, ind);
        moves[move_p + num_moves] =
            sq | to << 6 | piece << 12 | captured << 16 | SLIDE << 20;
        num_moves++;
        BB_CLEAR(magic_moves, ind);
    }

    return num_moves;
}

int extract_magic_moves(ChessBoard *board, MagicTable *magic_table, U64 *moves,
                        int move_p, Piece p) {
    int sq, num_moves;
    U64 magic_moves, bb;

	num_moves = 0;
    bb = get_bb(board, p, board->white_to_move);

    while ((sq = rightmost_set(bb)) != -1) {
        magic_moves = get_magic_moves_sq(board, magic_table, sq, p);
        num_moves += extract_magic_moves_sq(board, moves, move_p + num_moves,
                                            magic_moves, sq, p);

        BB_CLEAR(bb, sq);
    }

    return num_moves;
}

// Queen generation
U64 get_queen_moves_sq(ChessBoard *board, MagicTable *rook_table, MagicTable *bishop_table, int sq) {
    return get_magic_moves_sq(board, rook_table, sq, rook) |
           get_magic_moves_sq(board, bishop_table, sq, bishop);
}

int extract_queen_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                           U64 move_bb, int sq, Piece p) {
    int ind, to, piece, captured;
    int num_moves = 0;

    piece = get_piece(p, board->white_to_move);

    while ((ind = rightmost_set(move_bb)) != -1) {
        // this means the move is ind + 8 -> ind
        to = ind;
        captured = ChessBoard_piece_at(board, ind);
        moves[move_p + num_moves] =
            sq | to << 6 | piece << 12 | captured << 16 | SLIDE << 20;
        num_moves++;
        BB_CLEAR(move_bb, ind);
    }

    return num_moves;
}

int extract_queen_moves(ChessBoard *board, MagicTable *rook_table, MagicTable *bishop_table, U64 *moves,
						int move_p) {
	int sq, num_moves;
	U64 magic_moves, bb;
	
	num_moves = 0;
	bb = get_bb(board, queen, board->white_to_move);

	while ((sq = rightmost_set(bb)) != -1) {
		magic_moves = get_queen_moves_sq(board, rook_table, bishop_table, sq);
		num_moves += extract_magic_moves_sq(board, moves, move_p + num_moves,
											magic_moves, sq, queen);

		BB_CLEAR(bb, sq);
	}

	return num_moves;
}

// King generation
U64 get_king_moves_sq(ChessBoard *board, int sq) {
	
}

int extract_king_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                          U64 move_bb, int sq);
int extract_king_moves(ChessBoard *board, U64 *moves, int move_p);
