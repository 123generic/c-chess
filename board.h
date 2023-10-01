#ifndef CHESS_H
#define CHESS_H

#include "common.h"

// Helper Macros
#define BB_SQUARE(square) (1ULL << (square))
#define BB_SET(bb, square) (bb |= BB_SQUARE(square))
#define BB_GET(bb, square) (bb & BB_SQUARE(square))
#define BB_CLEAR(bb, square) (bb &= ~BB_SQUARE(square))

// Board get macros
#define BOARD_GET(side, piece) (board->side##_##piece)

// Bitboard Helpers
void print_bb(u64 bb);
u64 make_bitboard(char *str);

// Bitboard
typedef struct {
    // BitBoards
    // indexed as bitboards[piece + side]
    // extra bitboards indexed as 12 + white -> white_pieces, 12 + black ->
    // black_pieces, 12 + all -> all_pieces
    u64 bitboards[15];

    // Board State
    Side side;

    // Castling
    int KC[2], QC[2];

    // EP
    // -1 default; ep +- 1 are valid ep from squares
    int ep;

    // Halfmove Clock
    int halfmove_clock;

    // Fullmove Number
    int fullmove_number;

    // Zobrist hash
    u64 hash;

    // Evaluation
    int mg[2];
    int eg[2];
    int game_phase;
} ChessBoard;

void init_ChessBoard(ChessBoard *board);
void ChessBoard_from_FEN(ChessBoard *board, char *fen);
void ChessBoard_str(ChessBoard *board, char *str);
void ChessBoard_to_FEN(ChessBoard *board, char *str);
Piece ChessBoard_piece_at(ChessBoard *board, int ind);
void ChessBoard_print(ChessBoard *board);

// Zobrist
typedef struct {
    // piece[side][piece][square] -> u64
    u64 piece[2][6][64];
    // side -> u64
    u64 side;
    // castling[side][king/queen] -> u64
    u64 castling[2][2];
    // ep[square] -> u64
    u64 ep[64];
} zobrist_t;

extern zobrist_t zobrist;

void init_zobrist(void);
u64 manual_compute_hash(ChessBoard *board);

// init
void global_init(void);

#endif  // CHESS_H
