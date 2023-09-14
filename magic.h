#ifndef MAGIC_H
#define MAGIC_H

#include "board.h"
#include "common.h"

// Magics
/*
 * Note: table is fixed size, but bishop table will only use 64 * 512 entries
 */
typedef struct {
    U64 magic[64];
    U64 occupancy_mask[64];
    U64 move[64 * 4096];  // index as 4096 * square + magic_ind
} MagicTable;

// Magic gen
U64 find_magic(U64 *occupancy_mask_table, int sq, Piece p);
void init_magics(MagicTable *magic_table, Piece p);

// Rooks
void gen_occupancy_rook(MagicTable *magic_table);
U64 manual_gen_rook_moves(U64 bb, int square);
void fill_rook_moves(U64 *move, U64 occupancy_mask, U64 magic, int sq);

// Bishops
void gen_occupancy_bishop(MagicTable *magic_table);
U64 manual_gen_bishop_moves(U64 bb, int square);
void fill_bishop_moves(U64 *move, U64 occupancy_mask, U64 magic, int sq);

#endif  // MAGIC_H
