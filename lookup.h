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
    U64 bishop_magic[64];
    U64 bishop_mask[64];
    U64 bishop_move[512 * 64];

    U64 rook_magic[64];
    U64 rook_mask[64];
    U64 rook_move[4096 * 64];

    U64 king_move[64];
    U64 knight_move[64];
} LookupTable;

extern LookupTable lookup;

// Magic gen
U64 find_magic(U64 *mask_table, int sq, Piece p);

// Rooks
void gen_occupancy_rook(void);
U64 manual_gen_rook_moves(U64 bb, int square);
void fill_rook_moves(U64 *move, U64 mask, U64 magic, int sq);

// Bishops
void gen_occupancy_bishop(void);
U64 manual_gen_bishop_moves(U64 bb, int square);
void fill_bishop_moves(U64 *move, U64 mask, U64 magic, int sq);

// Other
void gen_king_moves(void);
void gen_knight_moves(void);

// Init
void init_LookupTable(void);

#endif  // MAGIC_H
