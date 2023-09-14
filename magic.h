#ifndef MAGIC_H
#define MAGIC_H

#include "board.h"
#include "common.h"

// Magics
typedef struct {
    U64 magic[64];
    U64 occupancy_mask[64];
    U64 move[64 * 4096];  // index as 4096 * square + magic_ind
} MagicTable;             // TODO
extern MagicTable rook_magic;
extern MagicTable bishop_magic;

// Rooks
void gen_occupancy_rook(MagicTable *magic_table);
U64 manual_gen_rook_moves(U64 bb, int square);
U64 find_magic_rook(U64 *occupancy_mask_table, int sq);
void fill_rook_moves(U64 *move, U64 occupancy_mask, U64 magic, int sq);
void init_magics_rook(MagicTable *magic_table);

// TODO
// void gen_occupancy_bishop(MagicTable *magic_table);

#endif  // MAGIC_H
