#ifndef SEARCH_H
#define SEARCH_H

#include "common.h"
#include "board.h"

i16 alphabeta(ChessBoard board, u64 attack_mask, i16 alpha, i16 beta, u16 depth, u16 ply, u64 *best_move);
i16 quiescence(ChessBoard board, i16 alpha, i16 beta, u16 ply);
i16 iterative_deepening(ChessBoard board);

#endif  // SEARCH_H
