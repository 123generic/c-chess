#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "common.h"
#include "magic.h"

// Move generation
// enum fits in 4 bits
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

U64 get_pawn_moves(ChessBoard *board, MoveType move_type);
int extract_pawn_moves(ChessBoard *board, U64 *moves, int move_p,
                       U64 pawn_moves, MoveType move_type);
U64 get_rook_moves_sq(ChessBoard *board, MagicTable *magic_table, int sq);
int extract_rook_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                          U64 rook_moves, int sq);
int extract_rook_moves(ChessBoard *board, MagicTable *magic_table, U64 *moves,
                       int move_p);
U64 move_from_uci(ChessBoard *board, char *uci);
void move_to_uci(U64 move, char *uci);

#endif  // MOVEGEN_H
