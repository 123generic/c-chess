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
int rightmost_set(U64 bb);
void print_bb(U64 bb);
U64 make_bitboard(char *str);

// Bitboard
typedef struct {
    // BitBoards
    U64 white_pawns;
    U64 white_rooks;
    U64 white_knights;
    U64 white_bishops;
    U64 white_queens;
    U64 white_king;

    U64 black_pawns;
    U64 black_rooks;
    U64 black_knights;
    U64 black_bishops;
    U64 black_queens;
    U64 black_king;

	// Extra boards
    U64 white_pieces;
    U64 black_pieces;
    U64 all_pieces;

    // Board State
    int white_to_move;

    // Castling
    int KC, QC, kc, qc;

    // EP
    // -1 default; ep +- 1 are valid ep from squares
    int ep;

    // Halfmove Clock
    int halfmove_clock;

    // Fullmove Number
    int fullmove_number;
} ChessBoard;

void init_ChessBoard(ChessBoard *board);
void ChessBoard_from_FEN(ChessBoard *board, char *fen);
void ChessBoard_str(ChessBoard *board, char *str);
void ChessBoard_to_FEN(ChessBoard *board, char *str);
int ChessBoard_piece_at(ChessBoard *board, int ind);

#endif  // CHESS_H
