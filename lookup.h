/*
 * Reference for magic bitboards:
 * - https://www.josherv.in/2021/03/19/chess-1/
 * - https://analog-hors.github.io/site/magic-bitboards/
 */

#ifndef MAGIC_H
#define MAGIC_H

#include "board.h"
#include "common.h"

typedef struct {
    u64 bishop_magic[64];
    u64 bishop_mask[64];
    u64 bishop_move[512 * 64];

    u64 rook_magic[64];
    u64 rook_mask[64];
    u64 rook_move[4096 * 64];

    u64 king_move[64];
    u64 knight_move[64];
} LookupTable;

extern LookupTable lookup;

// Magic gen
u64 find_magic(u64 *mask_table, int sq, Piece p);

// Rooks
void gen_occupancy_rook(void);
u64 manual_gen_rook_moves(u64 bb, int square);
void fill_rook_moves(u64 *move, u64 mask, u64 magic, int sq);

// Bishops
void gen_occupancy_bishop(void);
u64 manual_gen_bishop_moves(u64 bb, int square);
void fill_bishop_moves(u64 *move, u64 mask, u64 magic, int sq);

// Other
void gen_king_moves(void);
void gen_knight_moves(void);

// Init
void init_LookupTable(void);

#endif  // MAGIC_H
