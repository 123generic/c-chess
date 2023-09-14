#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "common.h"
#include "magic.h"

// Move representation
// moves are packed into a U64 (LSB -> MSB)
// 0-5: from square
// 6-11: to square
// 12-15: piece
// 16-19: capture piece
// 20-23: move type

// Move Type (enum fits in 4 bits)
typedef enum {
    SINGLE_PUSH = 0,
    DOUBLE_PUSH = 1,
    CAPTURE = 2,
    EN_PASSANT = 3,
    PROMOTION = 4,
    PROMOTION_CAPTURE = 5,

    CRAWL = 6,
    SLIDE = 7,

    CASTLE_KING = 8,
    CASTLE_QUEEN = 9,

    UNKNOWN = 10,
} MoveType;

// Pawns
U64 get_pawn_moves(ChessBoard *board, MoveType move_type);
int extract_pawn_moves(ChessBoard *board, U64 *moves, int move_p,
                       U64 pawn_moves, MoveType move_type);

// Magic move gen
U64 get_magic_moves_sq(ChessBoard *board, MagicTable *magic_table, int sq,
                       Piece piece);
int extract_magic_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                           U64 magic_moves, int sq, Piece p);
int extract_magic_moves(ChessBoard *board, MagicTable *magic_table, U64 *moves,
                        int move_p, Piece p);

// Utilities
U64 move_from_uci(ChessBoard *board, char *uci);
void move_to_uci(U64 move, char *uci);

#endif  // MOVEGEN_H
