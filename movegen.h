#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "common.h"
#include "lookup.h"
#include "search.h"

// Move representation
// moves are packed into a u64 (LSB -> MSB)
// 0-5: from square
// 6-11: to square
// 12-15: piece
// 16-19: capture piece
// 20-23: move type
// 24-27: promote type

typedef enum {
    SINGLE_PUSH,
    DOUBLE_PUSH,
    CAPTURE_LEFT,
    CAPTURE_RIGHT,
    PAWN_PROMOTION,
    PROMOTION_CAPTURE_LEFT,
    PROMOTION_CAPTURE_RIGHT,
    EN_PASSANT_LEFT,
    EN_PASSANT_RIGHT,
} PawnMoveType;

// Move Type (enum fits in 4 bits)
typedef enum {
    NORMAL,
    EN_PASSANT,
    PROMOTION,
    CASTLE_KING,
    CASTLE_QUEEN,

    UNKNOWN,
} MoveType;

// Move Generation Stage
typedef enum {
    promotions,
    captures,
    castling,
    quiets,
    losing,
} MoveGenStage;

// Pawns
u64 get_pawn_moves(ChessBoard *board, PawnMoveType move_type);
int extract_pawn_moves(ChessBoard *board, u64 *moves, int move_p,
                       u64 pawn_moves, PawnMoveType move_type);

// Other pieces
u64 get_moves(ChessBoard *board, int sq, Piece p);
int extract_moves(ChessBoard *board, u64 *moves, int move_p, u64 move_bb,
                  int sq, Piece p, int quiet);
int extract_all_moves(ChessBoard *board, u64 *moves, int move_p, Piece p);

// Castling
int generate_castling(ChessBoard *board, u64 *moves, u64 attacked, int move_p);

// Attackers
u64 get_attacks(ChessBoard *board, int sq, Piece p);
u64 attackers(ChessBoard *board, Side side);
int is_legal(ChessBoard *board, u64 attacked, Side side);

// Move Generation
int generate_promotions(ChessBoard *board, u64 *moves);
int generate_normal_moves_pawn(ChessBoard *board, u64 *moves, int quiet);
int generate_normal_moves(ChessBoard *board, u64 *moves, int quiet);
int generate_moves(ChessBoard *board, u64 *moves, u64 attackers, MoveGenStage stage);

// Move ordering
void sort_moves(u64 attack_mask, u64 *moves, int num_moves, KillerTable *killer_table, MoveGenStage stage);

// Utilities
u64 move_from_uci(ChessBoard *board, char *uci);
void move_to_uci(u64 move, char *uci);

#endif  // MOVEGEN_H
