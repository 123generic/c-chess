#include "board.h"
#include "common.h"
#include "movegen.h"
#include "lookup.h"

#include <stdio.h>
#include <stdlib.h>

void update_castling_rights(ChessBoard *board, U64 move) {
    int from = move & 0x3f;
    int to = (move >> 6) & 0x3f;
    Piece piece = (move >> 12) & 0xf;
    Piece captured = (move >> 16) & 0xf;

    // If our king moves
    if (piece == king) {
		board->hash ^= board->KC[board->side] != 0 ? zobrist.castling[board->side][0] : 0;
		board->hash ^= board->QC[board->side] != 0 ? zobrist.castling[board->side][1] : 0;
        board->KC[board->side] = 0;
        board->QC[board->side] = 0;
    }

    // If our rook moves
	int rook_kc = board->side == white ? 0 : 56;
	int rook_qc = board->side == white ? 7 : 63;
    if (piece == rook && from == rook_kc) {
		board->hash ^= board->KC[board->side] != 0 ? zobrist.castling[board->side][0] : 0;
        board->KC[board->side] = 0;
    } else if (piece == rook && from == rook_qc) {
		board->hash ^= board->QC[board->side] != 0 ? zobrist.castling[board->side][1] : 0;
        board->QC[board->side] = 0;
    }

    // If opponent's rook is captured
	rook_kc = board->side == white ? 56 : 0;
	rook_qc = board->side == white ? 63 : 7;
    if (captured == rook && to == rook_kc) {
		board->hash ^= board->KC[!board->side] != 0 ? zobrist.castling[!board->side][0] : 0;
        board->KC[!board->side] = 0;
    } else if (captured == rook && to == rook_qc) {
		board->hash ^= board->QC[!board->side] != 0 ? zobrist.castling[!board->side][1] : 0;
        board->QC[!board->side] = 0;
    }
}

#define ON(a, b, sq) (board.bitboards[(a) + (b)] |= (1ULL << (sq))); (board.hash ^= zobrist.piece[b][a / 2][sq])
#define OFF(a, b, sq) (board.bitboards[(a) + (b)] &= ~(1ULL << (sq))); (board.hash ^= zobrist.piece[b][a / 2][sq])

#define ON_NO_HASH(piece, side, sq) (board.bitboards[(piece) + (side)] |= (1ULL << (sq)))
#define OFF_NO_HASH(piece, side, sq) (board.bitboards[(piece) + (side)] &= ~(1ULL << (sq)))

ChessBoard make_move(ChessBoard board, U64 move) {
    int from, to;
    Piece piece, captured;
    MoveType move_type;
    Piece promotion_piece;

    from = move & 0x3f;
    to = (move >> 6) & 0x3f;
    piece = (move >> 12) & 0xf;
    captured = (move >> 16) & 0xf;
    move_type = (move >> 20) & 0xf;
    promotion_piece = (move >> 24) & 0xf;

    // Board Updates
    ON(piece, board.side, to);
    OFF(piece, board.side, from);

	ON_NO_HASH(all_pieces, board.side, to);
	OFF_NO_HASH(all_pieces, board.side, from);
	
	ON_NO_HASH(all_pieces, all, to);
	OFF_NO_HASH(all_pieces, all, from);

    // Conditional stuff
    switch (move_type) {
        int dir;

        case NORMAL:
            // Update board
            if (captured != empty) {
                OFF(captured, !board.side, to);
				OFF_NO_HASH(all_pieces, !board.side, to);
            }
            break;

        case EN_PASSANT:
            dir = 8 * (board.side - !board.side);
            OFF(pawn, !board.side, to + dir);
			OFF_NO_HASH(all_pieces, !board.side, to + dir);
			OFF_NO_HASH(all_pieces, all, to + dir);
            break;

        case PROMOTION:
            OFF(piece, board.side, to);
            ON(promotion_piece, board.side, to);

            if (captured != empty) {
                OFF(captured, !board.side, to);
				OFF_NO_HASH(all_pieces, !board.side, to);
            }
            break;

        case CASTLE_KING:
            OFF(rook, board.side, from - 3);
            ON(rook, board.side, to + 1);

			OFF_NO_HASH(all_pieces, board.side, from - 3);
			ON_NO_HASH(all_pieces, board.side, to + 1);

			OFF_NO_HASH(all_pieces, all, from - 3);
			ON_NO_HASH(all_pieces, all, to + 1);
            break;

        case CASTLE_QUEEN:
            OFF(rook, board.side, from + 4);
            ON(rook, board.side, to - 1);

			OFF_NO_HASH(all_pieces, board.side, from + 4);
			ON_NO_HASH(all_pieces, board.side, to - 1);

			OFF_NO_HASH(all_pieces, all, from + 4);
			ON_NO_HASH(all_pieces, all, to - 1);
            break;

        case UNKNOWN:
            fprintf(stderr, "[%s (%s:%d)] Error: unknown move type\n", __func__,
                    __FILE__, __LINE__);
            exit(1);
            break;
    }

    // Castling
    update_castling_rights(&board, move);

    // En Passant
	board.hash ^= board.ep != -1 ? zobrist.ep[board.ep] : 0;
    board.ep = -1;
    if (piece == pawn && abs(from - to) == 16) {
        board.ep = (from + to) / 2;
		board.hash ^= zobrist.ep[(from + to) / 2];
    }

    // Halfmove Clock
    board.halfmove_clock++;
    if (piece == pawn || captured != empty) {
        board.halfmove_clock = 0;
    }

    // Fullmove Number
    if (board.side == black) {
        board.fullmove_number++;
    }

    // Side
    board.side = !board.side;
	board.hash ^= zobrist.side;

	return board;
}

#undef ON
#undef OFF

#undef ON_NO_HASH
#undef OFF_NO_HASH
