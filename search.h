#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "common.h"

// Killer table
typedef struct {
    u64 move1;
    u64 move2;
} KillerTable;

void store_killer(KillerTable *killer_table, u16 ply, u64 move);

// Search routines
i16 alphabeta(int PV, ChessBoard board, KillerTable *killer_table, u64 *counter_move,
              u64 prev_move, int *history_table, u64 attack_mask, i16 alpha,
              i16 beta, u16 depth, u16 ply, u64 *best_move);
i16 quiescence(int PV, ChessBoard board, i16 alpha, i16 beta, u16 ply);
i16 iterative_deepening(ChessBoard board);

#endif  // SEARCH_H
