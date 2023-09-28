#include "lookup.h"

#include <string.h>

#include "rng.h"

LookupTable lookup;

// Find magic
U64 find_magic(U64 *mask, int sq, Piece p) {
    U64 magic, subset, occupancy;
    U64 moves[4096] = {0};  // Initialize to zero
    int ind, shift_amt;

    occupancy = mask[sq];
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
void gen_occupancy_rook(void) {
    U64 *mask = lookup.rook_mask;
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

void fill_rook_moves(U64 *move, U64 mask, U64 magic, int sq) {
    // Subset iteration trick (Carry-Rippler)
    U64 subset = 0;
    while (1) {
        int ind = (subset * magic) >> (64 - 12);
        U64 mv = manual_gen_rook_moves(subset, sq);
        move[4096 * sq + ind] = mv;

        subset = (subset - mask) & mask;
        if (subset == 0) break;
    }
}

// Bishop magics
void gen_occupancy_bishop(void) {
    U64 *mask = lookup.bishop_mask;
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

void fill_bishop_moves(U64 *move, U64 mask, U64 magic, int sq) {
    // Subset iteration trick (Carry-Rippler)
    U64 subset = 0;
    while (1) {
        int ind = (subset * magic) >> (64 - 9);
        U64 mv = manual_gen_bishop_moves(subset, sq);
        move[512 * sq + ind] = mv;

        subset = (subset - mask) & mask;
        if (subset == 0) break;
    }
}

void gen_king_moves(void) {
    U64 *bb = lookup.king_move;
    U64 moves;
    int offsets[] = {9, 8, 7, 1, -1, -7, -8, -9};
    int offsets_len = sizeof(offsets) / sizeof(offsets[0]);
    int i, j, offset;

    for (i = 0; i < 64; i++) {
        moves = 0;

        for (j = 0; j < offsets_len; j++) {
            offset = offsets[j];
            if (0 <= i + offset && i + offset < 64) BB_SET(moves, i + offset);
        }

        if (i % 8 == 7) moves &= ~FILE_8;

        if (i % 8 == 0) moves &= ~FILE_1;

        bb[i] = moves;
    }
}

void gen_knight_moves(void) {
    U64 *bb = lookup.knight_move;
    U64 moves;
    int offsets[] = {17, 15, 6, -10, -17, -15, -6, 10};
    int offsets_len = sizeof(offsets) / sizeof(offsets[0]);
    int i, j, offset;

    for (i = 0; i < 64; i++) {
        moves = 0;

        for (j = 0; j < offsets_len; j++) {
            offset = offsets[j];
            if (0 <= i + offset && i + offset < 64) BB_SET(moves, i + offset);
        }

        if (i % 8 >= 6) moves &= ~FILE_8;

        if (i % 8 == 7) moves &= ~FILE_7;

        if (i % 8 <= 1) moves &= ~FILE_1;

        if (i % 8 == 0) moves &= ~FILE_2;

        bb[i] = moves;
    }
}

void init_LookupTable(void) {
    // Magics
    gen_occupancy_bishop();
    gen_occupancy_rook();

    for (int i = 0; i < 64; i++) {
        lookup.bishop_magic[i] =
            find_magic(lookup.bishop_mask, i, bishop);
        fill_bishop_moves(lookup.bishop_move,
                          lookup.bishop_mask[i],
                          lookup.bishop_magic[i], i);

        lookup.rook_magic[i] =
            find_magic(lookup.rook_mask, i, rook);
        fill_rook_moves(lookup.rook_move, lookup.rook_mask[i],
                        lookup.rook_magic[i], i);
    }

    // Other pieces
    gen_king_moves();
    gen_knight_moves();
}
