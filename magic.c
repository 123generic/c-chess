/*
 * Reference for magic bitboards:
 * - https://www.josherv.in/2021/03/19/chess-1/
 * - https://analog-hors.github.io/site/magic-bitboards/
 */

#include "magic.h"

#include <string.h>

#include "board.h"
#include "rng.h"

// Find magic
U64 find_magic(U64 *occupancy_mask_table, int sq, Piece p) {
    U64 magic, subset, occupancy;
    U64 moves[4096] = {0};  // Initialize to zero
    int ind, shift_amt;

    occupancy = occupancy_mask_table[sq];
    shift_amt = p == rook ? (64 - 12) : (64 - 9);

    for (int loop_cnt = 0; loop_cnt < (1 << 24); loop_cnt++) {
        // Make random number fairly sparse (3 &s work well in practice)
        magic = genrand64_int64() & genrand64_int64() & genrand64_int64();

        // Clear the moves array
        memset(moves, 0, sizeof(moves));

        // Trick to iterate through subsets quickly (Carry-Rippler)
        subset = 0;
        while (1) {
            ind = (subset * magic) >> shift_amt;

            // Check magic's validity
            if (moves[ind] == 0) {
                moves[ind] = subset;
            } else if (moves[ind] != subset) {
                break;
            }

            subset = (subset - occupancy) & occupancy;
            if (subset == 0) break;  // wrapped around
        }

        // If we've reached here, magic is valid
        if (subset == 0) {
            return magic;
        }
    }

    return (U64)-1;  // failure
}

// Rook magics
void gen_occupancy_rook(MagicTable *magic_table) {
    U64 *mask = magic_table->occupancy_mask;
    U64 bb;
    int sq, cell;

    for (sq = 0; sq < 64; sq++) {
        bb = 0;

        // North
        for (cell = sq + 8; cell < 64 &&           // bounds
                            cell % 8 == sq % 8 &&  // column restriction
                            cell / 8 != 7;         // row restriction
             cell += 8)
            BB_SET(bb, cell);

        // East
        for (cell = sq - 1; cell >= 0 && cell / 8 == sq / 8 && cell % 8 != 0;
             cell--)
            BB_SET(bb, cell);

        // South
        for (cell = sq - 8; cell >= 0 && cell % 8 == sq % 8 && cell / 8 != 0;
             cell -= 8)
            BB_SET(bb, cell);

        // West
        for (cell = sq + 1; cell < 64 && cell / 8 == sq / 8 && cell % 8 != 7;
             cell++)
            BB_SET(bb, cell);

        mask[sq] = bb;
    }
}

U64 manual_gen_rook_moves(U64 bb, int sq) {
    U64 moves;
    int cell, should_break;

    moves = 0;
    // North
    for (cell = sq + 8; cell < 64 && cell % 8 == sq % 8; cell += 8) {
        should_break = BB_GET(bb, cell) != 0;
        BB_SET(moves, cell);
        if (should_break) break;
    }

    // East
    for (cell = sq - 1; 0 <= cell && cell / 8 == sq / 8; cell--) {
        should_break = BB_GET(bb, cell) != 0;
        BB_SET(moves, cell);
        if (should_break) break;
    }

    // South
    for (cell = sq - 8; 0 <= cell && cell % 8 == sq % 8; cell -= 8) {
        should_break = BB_GET(bb, cell) != 0;
        BB_SET(moves, cell);
        if (should_break) break;
    }

    // West
    for (cell = sq + 1; cell < 64 && cell / 8 == sq / 8; cell++) {
        should_break = BB_GET(bb, cell) != 0;
        BB_SET(moves, cell);
        if (should_break) break;
    }

    return moves;
}

void fill_rook_moves(U64 *move, U64 occupancy_mask, U64 magic, int sq) {
    // Subset iteration trick (Carry-Rippler)
    U64 subset = 0;
    while (1) {
        int ind = (subset * magic) >> (64 - 12);
        U64 mv = manual_gen_rook_moves(subset, sq);
        move[4096 * sq + ind] = mv;

        subset = (subset - occupancy_mask) & occupancy_mask;
        if (subset == 0) break;
    }
}

void init_magics(MagicTable *magic_table, Piece p) {
    U64 *magic = magic_table->magic;
    U64 *mask = magic_table->occupancy_mask;
    U64 *move = magic_table->move;

    memset(magic_table, 0, sizeof(MagicTable));
    if (p == rook) {
        gen_occupancy_rook(magic_table);
    } else {
        gen_occupancy_bishop(magic_table);
    }

    for (int i = 0; i < 64; i++) {
        if (p == rook) {
        	magic[i] = find_magic(mask, i, rook);
            fill_rook_moves(move, mask[i], magic[i], i);
        } else {
			magic[i] = find_magic(mask, i, bishop);
            fill_bishop_moves(move, mask[i], magic[i], i);
        }
    }
}

// Bishop magics
void gen_occupancy_bishop(MagicTable *magic_table) {
    U64 *mask = magic_table->occupancy_mask;
    U64 bb;
    int sq, cell;

    for (sq = 0; sq < 64; sq++) {
        bb = 0;

        // North-West
        for (cell = sq + 9; cell < 64 &&          // bounds
                            cell % 8 > sq % 8 &&  // column restriction
                            cell / 8 > sq / 8 &&  // row restriction
                            cell % 8 != 7 &&      // column boundary
                            cell / 8 != 7;        // row boundary
             cell += 9)
            BB_SET(bb, cell);

        // North-East
        for (cell = sq + 7; cell < 64 && cell % 8 < sq % 8 &&
                            cell / 8 > sq / 8 && cell % 8 != 0 && cell / 8 != 7;
             cell += 7)
            BB_SET(bb, cell);

        // South-East
        for (cell = sq - 9; cell >= 0 && cell % 8 < sq % 8 &&
                            cell / 8 < sq / 8 && cell % 8 != 0 && cell / 8 != 0;
             cell -= 9)
            BB_SET(bb, cell);

        // West
        for (cell = sq - 7; cell >= 0 && cell % 8 > sq % 8 &&
                            cell / 8 < sq / 8 && cell % 8 != 7 && cell / 8 != 0;
             cell -= 7)
            BB_SET(bb, cell);

        mask[sq] = bb;
    }
}

U64 manual_gen_bishop_moves(U64 bb, int sq) {
    U64 moves;
    int cell, should_break;

    moves = 0;
    // North-West
    for (cell = sq + 9; cell < 64 && cell % 8 > sq % 8 && cell / 8 > sq / 8;
         cell += 9) {
        should_break = BB_GET(bb, cell) != 0;
        BB_SET(moves, cell);
        if (should_break) break;
    }

    // North-East
    for (cell = sq + 7; cell < 64 && cell % 8 < sq % 8 && cell / 8 > sq / 8;
         cell += 7) {
        should_break = BB_GET(bb, cell) != 0;
        BB_SET(moves, cell);
        if (should_break) break;
    }

    // South-East
    for (cell = sq - 9; cell >= 0 && cell % 8 < sq % 8 && cell / 8 < sq / 8;
         cell -= 9) {
        should_break = BB_GET(bb, cell) != 0;
        BB_SET(moves, cell);
        if (should_break) break;
    }

    // South-West
    for (cell = sq - 7; cell >= 0 && cell % 8 > sq % 8 && cell / 8 < sq / 8;
         cell -= 7) {
        should_break = BB_GET(bb, cell) != 0;
        BB_SET(moves, cell);
        if (should_break) break;
    }

    return moves;
}

void fill_bishop_moves(U64 *move, U64 occupancy_mask, U64 magic, int sq) {
    // Subset iteration trick (Carry-Rippler)
    U64 subset = 0;
    while (1) {
        int ind = (subset * magic) >> (64 - 9);
        U64 mv = manual_gen_bishop_moves(subset, sq);
        move[512 * sq + ind] = mv;

        subset = (subset - occupancy_mask) & occupancy_mask;
        if (subset == 0) break;
    }
}
