#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "common.h"
#include "lookup.h"

// Move representation
// moves are packed into a U64 (LSB -> MSB)
// 0-5: from square
// 6-11: to square
// 12-15: piece
// 16-19: capture piece
// 20-23: move type
// 24-26: promote type

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
U64 get_pawn_moves(ChessBoard *board, PawnMoveType move_type);
int extract_pawn_moves(ChessBoard *board, U64 *moves, int move_p,
                       U64 pawn_moves, PawnMoveType move_type);

// Other pieces
U64 get_moves(ChessBoard *board, LookupTable *lookup, int sq, Piece p);
int extract_moves(ChessBoard *board, U64 *moves, int move_p, U64 move_bb,
                  int sq, Piece p, int quiet);
int extract_all_moves(ChessBoard *board, LookupTable *table, U64 *moves,
                      int move_p, Piece p);

// Castling
int generate_castling(ChessBoard *board, U64 *moves, U64 attacked, int move_p);

// Attackers
U64 get_attacks(ChessBoard *board, LookupTable *lookup, int sq, Piece p);
U64 attackers(ChessBoard *board, LookupTable *lookup, int side);

// Move Generation
int generate_promotions(ChessBoard *board, U64 *moves);
int generate_normal_moves_pawn(ChessBoard *board, U64 *moves, int quiet);
int generate_normal_moves(ChessBoard *board, LookupTable *table, U64 *moves,
                          int quiet);
int generate_moves(ChessBoard *board, LookupTable *table, U64 *moves,
                   U64 attackers, MoveGenStage stage);

// Utilities
U64 move_from_uci(ChessBoard *board, char *uci);
void move_to_uci(U64 move, char *uci);
int get_piece(Piece p, int wtm);
U64 get_bb(ChessBoard *board, Piece p, int wtm);

#endif  // MOVEGEN_H
