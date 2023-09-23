#include "board.h"
#include "movegen.h"
#include "lookup.h"

#include <stdio.h>
#include <stdlib.h>

void update_castling_rights(ChessBoard *board, U64 move) {
    int from = move & 0x3f;
    int to = (move >> 6) & 0x3f;
    Piece piece = (move >> 12) & 0xf;
    Piece captured = (move >> 16) & 0xf;

    // Note: zobrist.castling[side][0] = kc, [side][1] = qc

    // If our king moves
    if (piece == king) {
        board->KC[board->side] = 0;
        board->QC[board->side] = 0;
    }

    // If our rook moves
    if (piece == rook && from % 8 == 0) {
        board->KC[board->side] = 0;
    } else if (piece == rook && from % 8 == 7) {
        board->QC[board->side] = 0;
    }

    // If opponent's rook is captured
    if (captured == rook && to % 8 == 0) {
        board->KC[!board->side] = 0;
    } else if (captured == rook && to % 8 == 7) {
        board->QC[!board->side] = 0;
    }
}

#define ON(a, b, sq) (board->bitboards[(a) + (b)] |= (1ULL << (sq)))
#define OFF(a, b, sq) (board->bitboards[(a) + (b)] &= ~(1ULL << (sq)))

void make_move(ChessBoard *board, U64 move) {
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
    ON(piece, board->side, to);
    OFF(piece, board->side, from);

    // Conditional stuff
    switch (move_type) {
        int dir;

        case NORMAL:
            // Update board
            if (captured != empty) {
                OFF(captured, !board->side, to);
            }
            break;

        case EN_PASSANT:
            dir = 8 * (board->side - !board->side);
            OFF(pawn, !board->side, to + dir);
            break;

        case PROMOTION:
            OFF(piece, board->side, from);
            ON(promotion_piece, board->side, to);
            // Note: all pieces already set

            if (captured != empty) {
                OFF(captured, !board->side, to);
            }
            break;

        case CASTLE_KING:
            OFF(rook, board->side, from - 3);
            ON(rook, board->side, to + 1);
            break;

        case CASTLE_QUEEN:
            OFF(rook, board->side, from + 4);
            ON(rook, board->side, to - 1);
            break;

        case UNKNOWN:
            fprintf(stderr, "[%s (%s:%d)] Error: unknown move type\n", __func__,
                    __FILE__, __LINE__);
            exit(1);
            break;
    }

    board->bitboards[all_pieces + white] =
        board->bitboards[pawn + white] | board->bitboards[knight + white] |
        board->bitboards[bishop + white] | board->bitboards[rook + white] |
        board->bitboards[queen + white] | board->bitboards[king + white];
    board->bitboards[all_pieces + black] =
        board->bitboards[pawn + black] | board->bitboards[knight + black] |
        board->bitboards[bishop + black] | board->bitboards[rook + black] |
        board->bitboards[queen + black] | board->bitboards[king + black];
    board->bitboards[all_pieces + all] = board->bitboards[all_pieces + white] |
                                         board->bitboards[all_pieces + black];

    // Castling
    update_castling_rights(board, move);

    // En Passant
    board->ep = -1;
    if (piece == pawn && abs(from - to) == 16) {
        board->ep = (from + to) / 2;
    }

    // Halfmove Clock
    board->halfmove_clock++;
    if (piece == pawn || captured != empty) {
        board->halfmove_clock = 0;
    }

    // Fullmove Number
    if (board->side == black) {
        board->fullmove_number++;
    }

    // Side
    board->side = !board->side;
}

#undef ON
#undef OFF