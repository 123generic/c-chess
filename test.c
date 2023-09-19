#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "common.h"
#include "lookup.h"
#include "movegen.h"
#include "rng.h"

// Tests
void test_rightmost_set(void) {
    // Test with 0
    if (rightmost_set(0) != -1) {
        printf("Test failed: input 0\n");
        return;
    }

    // Test with 1
    if (rightmost_set(1) != 0) {
        printf("Test failed: input 1\n");
        return;
    }

    // Test with 2 (10 in binary)
    if (rightmost_set(2) != 1) {
        printf("Test failed: input 2\n");
        return;
    }

    // Test with 8 (1000 in binary)
    if (rightmost_set(8) != 3) {
        printf("Test failed: input 8\n");
        return;
    }

    // Test with 9 (1001 in binary)
    if (rightmost_set(9) != 0) {
        printf("Test failed: input 9\n");
        return;
    }

    // Test with large number
    U64 large_num = ((U64)1 << 63);  // The most significant bit is set
    if (rightmost_set(large_num) != 63) {
        printf("Test failed: input large_num\n");
        return;
    }

    printf("test_rightmost_set: All tests passed.\n");
}

void test_ChessBoard_str(void) {
    ChessBoard board;
    char str[65];  // 64 + 1 for null terminator

    // Test Case 1: Starting position
    ChessBoard_from_FEN(
        &board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    ChessBoard_str(&board, str);
    if (strcmp(str,
               "rnbqkbnrpppppppp................................"
               "PPPPPPPPRNBQKBNR") != 0) {
        printf("Test failed: Starting position\n");
        return;
    }

    // Test Case 2: Some arbitrary position
    ChessBoard_from_FEN(
        &board,
        "rnbqk2r/pppp1ppp/4pn2/8/1b1P4/2N1PN2/PPP2PPP/R1BQK2R w KQkq - 0 1");
    ChessBoard_str(&board, str);
    if (strcmp(str,
               "rnbqk..rpppp.ppp....pn...........b.P......N.PN..PPP..PPPR.BQK.."
               "R") != 0) {
        printf("Test failed: Some arbitrary position\n");
        return;
    }

    // Test Case 3: An empty board
    ChessBoard_from_FEN(&board, "8/8/8/8/8/8/8/8 w - - 0 1");
    ChessBoard_str(&board, str);
    if (strcmp(str,
               "..............................................................."
               ".") != 0) {
        printf("Test failed: An empty board\n");
        return;
    }

    printf("test_ChessBoard_str: All tests passed.\n");
}

void test_ChessBoard_to_FEN(void) {
    ChessBoard board;
    char fen[128];

    // Test Case 1: Starting Position
    ChessBoard_from_FEN(
        &board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    ChessBoard_to_FEN(&board, fen);
    if (strcmp(fen,
               "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") !=
        0) {
        printf("Test Case 1 failed.\n");
        return;
    }

    // Test Case 2: Some arbitrary position
    ChessBoard_from_FEN(
        &board,
        "rnbqk2r/pppp1ppp/4pn2/8/1b1P4/2N1PN2/PPP2PPP/R1BQK2R w KQkq - 0 1");
    ChessBoard_to_FEN(&board, fen);
    if (strcmp(fen,
               "rnbqk2r/pppp1ppp/4pn2/8/1b1P4/2N1PN2/PPP2PPP/R1BQK2R w KQkq - "
               "0 1") != 0) {
        printf("Test Case 2 failed.\n");
        return;
    }

    // Test Case 3: No castling rights and no en passant target square
    ChessBoard_from_FEN(
        &board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b - - 0 1");
    ChessBoard_to_FEN(&board, fen);
    if (strcmp(fen, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b - - 0 1") !=
        0) {
        printf("Test Case 3 failed.\n");
        return;
    }

    printf("test_ChessBoard_to_FEN: All tests passed.\n");
}

// Utility function to compare move lists
// O(n^2) but n is small, also I'm lazy
int compare_move_lists(U64 *expected, U64 *actual, int n) {
    int i, j, found;
    for (i = 0; i < n; i++) {
        found = 0;

        for (j = 0; j < n; j++) {
            U64 exp_trunc = expected[i] & 0xfff;  // Truncate to 12 bits
            U64 act_trunc = actual[j] & 0xfff;    // Truncate to 12 bits
            if (exp_trunc == act_trunc) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }
    return 1;
}

void test_extract_pawn_moves(void) {
    ChessBoard board;
    U64 pawn_moves;
    U64 moves[256] = {0};
    int move_p = 0;

    // Test 1: White to move, pawns at the initial position
    ChessBoard_from_FEN(
        &board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // move_p += single_pawn_pushes(&board, moves, move_p);
    pawn_moves = get_pawn_moves(&board, SINGLE_PUSH);
    move_p +=
        extract_pawn_moves(&board, moves, move_p, pawn_moves, SINGLE_PUSH);
    char *expected_uci_1[] = {"a2a3", "b2b3", "c2c3", "d2d3",
                              "e2e3", "f2f3", "g2g3", "h2h3"};
    U64 expected_moves_1[8];
    for (int i = 0; i < 8; i++) {
        expected_moves_1[i] = move_from_uci(&board, expected_uci_1[i]);
    }
    if (!compare_move_lists(expected_moves_1, moves, 8)) {
        printf("Test 1 failed.\n");
        return;
    }

    // Test 2: Black to move, pawns at the initial position
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    pawn_moves = get_pawn_moves(&board, SINGLE_PUSH);
    move_p +=
        extract_pawn_moves(&board, moves, move_p, pawn_moves, SINGLE_PUSH);
    char *expected_uci_2[] = {"a7a6", "b7b6", "c7c6", "d7d6",
                              "e7e6", "f7f6", "g7g6", "h7h6"};
    U64 expected_moves_2[8];
    for (int i = 0; i < 8; i++) {
        expected_moves_2[i] = move_from_uci(&board, expected_uci_2[i]);
    }
    if (!compare_move_lists(expected_moves_2, moves, 8)) {
        printf("Test 2 failed.\n");
        return;
    }

    // Test 3: Custom board setup with white and black pawns in odd locations
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(&board, "8/8/4pP2/2P1P3/1p1p4/8/8/8 w - - 0 1");
    pawn_moves = get_pawn_moves(&board, SINGLE_PUSH);
    move_p +=
        extract_pawn_moves(&board, moves, move_p, pawn_moves, SINGLE_PUSH);
    char *expected_uci_3[] = {"c5c6", "f6f7"};
    U64 expected_moves_3[2];
    for (int i = 0; i < 2; i++) {
        expected_moves_3[i] = move_from_uci(&board, expected_uci_3[i]);
    }
    if (!compare_move_lists(expected_moves_3, moves, 2)) {
        printf("Test 3 failed.\n");
        return;
    }

    // Test 4: Custom board setup with white and black pawns in odd locations
    // (black's turn)
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(&board, "8/8/4pP2/2P1P3/1p1p4/8/8/8 b - - 0 1");
    pawn_moves = get_pawn_moves(&board, SINGLE_PUSH);
    move_p +=
        extract_pawn_moves(&board, moves, move_p, pawn_moves, SINGLE_PUSH);
    char *expected_uci_4[] = {"d4d3", "b4b3"};
    U64 expected_moves_4[2];
    for (int i = 0; i < 2; i++) {
        expected_moves_4[i] = move_from_uci(&board, expected_uci_4[i]);
    }
    if (!compare_move_lists(expected_moves_4, moves, 2)) {
        printf("Test 4 failed.\n");
        return;
    }

    printf("test_extract_pawn_moves: All tests passed.\n");
}

void test_gen_occupancy_rook(void) {
    U64 expected;
    LookupTable lookup = {0};

    init_LookupTable(&lookup);

    // Test 1: 12 bits on
    expected = make_bitboard(
        "01111110"
        "10000000"
        "10000000"
        "10000000"
        "10000000"
        "10000000"
        "10000000"
        "00000000");
    if (lookup.rook_mask[63] != expected) {
        printf("Test 1 failed.\n");
        return;
    }

    // Test 2: generic
    expected = make_bitboard(
        "00000000"
        "00010000"
        "00010000"
        "00010000"
        "01101110"
        "00010000"
        "00010000"
        "00000000");
    if (lookup.rook_mask[28] != expected) {
        printf("Test 2 failed.\n");
        return;
    }

    printf("test_gen_occupancy_rook: All tests passed.\n");
}

void test_manual_gen_rook_moves(void) {
    // Test 1: Rook on an empty board
    U64 bb = make_bitboard(
        "00000000"
        "00000000"
        "00000000"
        "00000000"
        "00001000"
        "00000000"
        "00000000"
        "00000000");
    U64 expected = make_bitboard(
        "00001000"
        "00001000"
        "00001000"
        "00001000"
        "11110111"
        "00001000"
        "00001000"
        "00001000");
    U64 result = manual_gen_rook_moves(bb, 27);
    if (result != expected) {
        printf("Test 1 failed.\n");
        return;
    }

    // Test 2: Rook with some blocking pieces
    bb = make_bitboard(
        "00000000"
        "00000000"
        "00000000"
        "00010000"
        "00001000"
        "00000000"
        "00000000"
        "00000000");
    expected = make_bitboard(
        "00000000"
        "00000000"
        "00000000"
        "00010000"
        "11101000"
        "00010000"
        "00010000"
        "00010000");
    result = manual_gen_rook_moves(bb, 28);
    if (result != expected) {
        printf("Test 2 failed.\n");
        return;
    }

    printf("test_manual_gen_rook_moves: All tests passed.\n");
}

void test_gen_occupancy_bishop(void) {
    U64 expected;
    LookupTable lookup = {0};

    init_LookupTable(&lookup);

    // Test 1
    expected = make_bitboard(
        "00000000"
        "00001010"
        "00000000"
        "00001010"
        "00010000"
        "00100000"
        "01000000"
        "00000000");
    if (lookup.bishop_mask[42] != expected) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    // Test 2
    expected = make_bitboard(
        "00000000"
        "00000010"
        "00000100"
        "00001000"
        "00010000"
        "00100000"
        "01000000"
        "00000000");
    if (lookup.bishop_mask[7] != expected &&
        lookup.bishop_mask[56] != expected) {
        printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    // Test 3
    expected = make_bitboard(
        "00000000"
        "00100000"
        "01000000"
        "00000000"
        "01000000"
        "00100000"
        "00010000"
        "00000000");
    if (lookup.bishop_mask[39] != expected) {
        printf("[%s %s:%d] Test 3 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    printf("test_gen_occupancy_bishop: All tests passed.\n");
}

void test_manual_gen_bishop_moves(void) {
    U64 bb, expected, result;

    // Test 1
    bb = make_bitboard(
        "00000000"
        "00000000"
        "00000000"
        "00000000"
        "00001000"
        "00000000"
        "00000000"
        "00000000");
    expected = make_bitboard(
        "10000000"
        "01000001"
        "00100010"
        "00010100"
        "00000000"
        "00010100"
        "00100010"
        "01000001");
    result = manual_gen_bishop_moves(bb, 27);
    if (result != expected) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    // Test 2
    bb = make_bitboard(
        "00000000"
        "00000000"
        "00011000"
        "00010000"
        "00001000"
        "00000000"
        "00000000"
        "00000000");
    expected = make_bitboard(
        "10001000"
        "01010000"
        "00000000"
        "01010000"
        "10000000"
        "00000000"
        "00000000"
        "00000000");
    result = manual_gen_bishop_moves(bb, 45);
    if (result != expected) {
        printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    printf("test_manual_gen_bishop_moves: All tests passed.\n");
}

void test_init_magic_rook(void) {
    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    // Test 1: Validate that the magic table produces the correct moves
    // for a rook on square 27 and square 28 for specific occupancy boards.
    U64 occupancy1 = make_bitboard(
        "00000000"
        "00000000"
        "00000000"
        "00000000"
        "00000000"
        "00000000"
        "00000000"
        "00000000");
    U64 expected1 = make_bitboard(
        "00001000"
        "00001000"
        "00001000"
        "00001000"
        "11110111"
        "00001000"
        "00001000"
        "00001000");
    int sq = 27;
    int ind1 = (occupancy1 * lookup.rook_magic[sq]) >> (64 - 12);
    U64 magic_moves1 = lookup.rook_move[4096 * sq + ind1];
    if (magic_moves1 != expected1) {
        printf("Test 1 failed for square 27.\n");
        return;
    }

    U64 occupancy2 = make_bitboard(
        "00000000"
        "00000000"
        "00000000"
        "00010000"
        "00001000"
        "00000000"
        "00000000"
        "00000000");
    U64 expected2 = make_bitboard(
        "00000000"
        "00000000"
        "00000000"
        "00010000"
        "11101000"
        "00010000"
        "00010000"
        "00010000");
    int ind2 = (occupancy2 * lookup.rook_magic[28]) >> (64 - 12);
    U64 magic_moves2 = lookup.rook_move[4096 * 28 + ind2];
    if (magic_moves2 != expected2) {
        printf("Test 1 failed for square 28.\n");
        return;
    }

    printf("test_init_magic_rook: All tests passed.\n");
}

void test_init_magic_bishop(void) {
    U64 occupancy, expected, magic_moves;
    int sq, ind;

    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    // Test 1
    occupancy = make_bitboard(
        "00000000"
        "00000000"
        "00000010"
        "00000000"
        "00001000"
        "00000000"
        "00000000"
        "00000000");
    expected = make_bitboard(
        "00000000"
        "00000000"
        "10000000"
        "01000000"
        "00101000"
        "00000000"
        "00101000"
        "01000100");
    sq = 20;
    ind = (occupancy * lookup.bishop_magic[sq]) >> (64 - 9);
    magic_moves = lookup.bishop_move[512 * sq + ind];
    if (magic_moves != expected) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    // Test 2
    occupancy = make_bitboard(
        "00000000"
        "00000000"
        "00100000"
        "01000100"
        "00000010"
        "00000000"
        "00000000"
        "00000000");
    expected = make_bitboard(
        "00101000"
        "00000000"
        "00101000"
        "00000100"
        "00000000"
        "00000000"
        "00000000"
        "00000000");
    sq = 52;
    ind = (occupancy * lookup.bishop_magic[sq]) >> (64 - 9);
    magic_moves = lookup.bishop_move[512 * sq + ind];
    if (magic_moves != expected) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    printf("test_init_magic_bishop: All tests passed.\n");
}

void test_extract_magic_moves_rook(void) {
    ChessBoard board;
    U64 moves[256] = {0};
    int move_p = 0;

    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    // Test 1
    ChessBoard_from_FEN(
        &board,
        "3B4/2R1p1Q1/1PRp1P1P/2pkb3/pp1r2P1/3p1N1P/r1nnP3/6K1 w - - 0 1");

    move_p += extract_magic_moves(&board, &lookup, moves, move_p, rook);

    char *expected_uci_1[] = {"c6c5", "c6d6", "c7a7", "c7b7",
                              "c7d7", "c7e7", "c7c8"};
    const size_t len = sizeof(expected_uci_1) / sizeof(expected_uci_1[0]);
    U64 expected_moves_1[len];
    for (size_t i = 0; i < len; i++) {
        expected_moves_1[i] = move_from_uci(&board, expected_uci_1[i]);
    }

    if (!compare_move_lists(expected_moves_1, moves, len)) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    // Test 2
    memset(moves, 0, sizeof(moves));
    move_p = 0;

    ChessBoard_from_FEN(
        &board, "3k4/pq4bp/1pppK3/2P2P2/p6R/PRpB1b2/PP2Q1P1/3nn3 w - - 0 1");

    move_p += extract_magic_moves(&board, &lookup, moves, move_p, rook);

    char expected_uci_2[][5] = {"b3b4", "b3b5", "b3b6", "b3c3", "h4h1", "h4h2",
                                "h4h3", "h4h5", "h4h6", "h4h7", "h4a4", "h4b4",
                                "h4c4", "h4d4", "h4e4", "h4f4", "h4g4"};
    char actual_uci_2[sizeof(expected_uci_2) / sizeof(expected_uci_2[0])][5];

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_2[i]);
    }

    const size_t len2 = sizeof(expected_uci_2) / sizeof(expected_uci_2[0]);
    if (move_p != len2) {
        printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    qsort(expected_uci_2, len2, sizeof(expected_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_2, len2, sizeof(actual_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len2; i++) {
        if (strcmp(expected_uci_2[i], actual_uci_2[i]) != 0) {
            printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    // Test 3
    memset(moves, 0, sizeof(moves));
    move_p = 0;

    ChessBoard_from_FEN(
        &board, "N2K4/2p2pNP/2Q2pq1/6RP/3P2p1/1PPkb1p1/1pbB1p1P/r7 w - - 0 1");

    move_p += extract_magic_moves(&board, &lookup, moves, move_p, rook);

    char expected_uci_3[][5] = {"g5a5", "g5b5", "g5c5", "g5d5",
                                "g5e5", "g5f5", "g5g4", "g5g6"};
    char actual_uci_3[sizeof(expected_uci_3) / sizeof(expected_uci_3[0])][5];

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_3[i]);
    }

    const size_t len3 = sizeof(expected_uci_3) / sizeof(expected_uci_3[0]);
    if (move_p != len3) {
        printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    qsort(expected_uci_3, len3, sizeof(expected_uci_3[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_3, len3, sizeof(actual_uci_3[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len3; i++) {
        if (strcmp(expected_uci_3[i], actual_uci_3[i]) != 0) {
            printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    printf("test_extract_magic_moves_rook: All tests passed.\n");
}

void test_extract_magic_moves_bishop(void) {
    ChessBoard board;
    U64 moves[256] = {0};
    int move_p = 0;

    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    // Test 1
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board, "6N1/5RKP/BbPP4/6P1/pR1n2qn/p1B1p3/Q1Pp3r/N1r4k w - - 0 1");

    move_p += extract_magic_moves(&board, &lookup, moves, move_p, bishop);

    char expected_uci_1[][5] = {"a6b7", "a6c8", "a6b5", "a6c4", "a6d3",
                                "a6e2", "a6f1", "c3d4", "c3d2", "c3b2"};
    char actual_uci_1[sizeof(expected_uci_1) / sizeof(expected_uci_1[0])][5];

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_1[i]);
    }

    const size_t len1 = sizeof(expected_uci_1) / sizeof(expected_uci_1[0]);
    if (move_p != len1) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    qsort(expected_uci_1, len1, sizeof(expected_uci_1[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_1, len1, sizeof(actual_uci_1[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len1; i++) {
        if (strcmp(expected_uci_1[i], actual_uci_1[i]) != 0) {
            printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    // Test 2
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board, "7b/P3QKpk/1rPq2n1/1b6/pB3p1p/p4p1N/PP1p1rP1/3B4 b - - 0 1");

    move_p += extract_magic_moves(&board, &lookup, moves, move_p, bishop);

    char expected_uci_2[][5] = {"b5a6", "b5c6", "b5c4", "b5d3", "b5e2", "b5f1"};
    char actual_uci_2[sizeof(expected_uci_2) / sizeof(expected_uci_2[0])][5];

    const size_t len2 = sizeof(expected_uci_2) / sizeof(expected_uci_2[0]);
    if (move_p != len2) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_2[i]);
    }

    qsort(expected_uci_2, len2, sizeof(expected_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_2, len2, sizeof(actual_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len2; i++) {
        if (strcmp(expected_uci_2[i], actual_uci_2[i]) != 0) {
            printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    printf("test_extract_magic_moves_bishop: All tests passed.\n");
}

void test_extract_queen_moves(void) {
    ChessBoard board;
    U64 moves[256] = {0};
    int move_p = 0;

    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    // Test 1
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board, "k5r1/Pp6/P1p1RRpb/4rpQq/2PB1pp1/1n2P1n1/1K1N3p/8 w - - 0 1");

    move_p += extract_queen_moves(&board, &lookup, moves, move_p);

    char expected_uci_1[][5] = {"g5g6", "g5g4", "g5f4", "g5f5",
                                "g5h4", "g5h5", "g5h6"};
    char actual_uci_1[sizeof(expected_uci_1) / sizeof(expected_uci_1[0])][5];

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_1[i]);
    }

    const size_t len1 = sizeof(expected_uci_1) / sizeof(expected_uci_1[0]);
    if (move_p != len1) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    qsort(expected_uci_1, len1, sizeof(expected_uci_1[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_1, len1, sizeof(actual_uci_1[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len1; i++) {
        if (strcmp(expected_uci_1[i], actual_uci_1[i]) != 0) {
            printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    // Test 2
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board, "1N4RB/1p2KNp1/Rp3p2/1kp1bq1P/2nP1rP1/2P2Q1r/p7/4n3 b - - 0 1");

    move_p += extract_queen_moves(&board, &lookup, moves, move_p);

    char expected_uci_2[][5] = {"f5b1", "f5c2", "f5d3", "f5e4", "f5g6", "f5h7",
                                "f5c8", "f5d7", "f5e6", "f5g4", "f5g5", "f5h5"};
    char actual_uci_2[sizeof(expected_uci_2) / sizeof(expected_uci_2[0])][5];

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_2[i]);
    }

    const size_t len2 = sizeof(expected_uci_2) / sizeof(expected_uci_2[0]);
    if (move_p != len2) {
        printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    qsort(expected_uci_2, len2, sizeof(expected_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_2, len2, sizeof(actual_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len2; i++) {
        if (strcmp(expected_uci_2[i], actual_uci_2[i]) != 0) {
            printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    printf("test_extract_queen_moves: All tests passed.\n");
}

void test_extract_king_moves(void) {
    ChessBoard board;
    U64 moves[256] = {0};
    int move_p = 0;

    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    // Test 1
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board, "8/bR4K1/P1p1bp1p/1nk1N3/5BP1/1PP1p1nQ/1P2BPP1/R2q4 w - - 0 1");

    move_p += extract_king_moves(&board, &lookup, moves, move_p);

    char expected_uci_1[][5] = {"g7f6", "g7g6", "g7h6", "g7h7",
                                "g7h8", "g7g8", "g7f8", "g7f7"};
    char actual_uci_1[sizeof(expected_uci_1) / sizeof(expected_uci_1[0])][5];

    const size_t len1 = sizeof(expected_uci_1) / sizeof(expected_uci_1[0]);
    if (move_p != len1) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_1[i]);
    }

    qsort(expected_uci_1, len1, sizeof(expected_uci_1[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_1, len1, sizeof(actual_uci_1[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len1; i++) {
        if (strcmp(expected_uci_1[i], actual_uci_1[i]) != 0) {
            printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    // Test 2
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board,
        "2B1nk1b/1pPR4/1P6/1r1p4/2p1pP1r/2QnNp2/1PR1b1P1/K3N3 b - - 0 1");

    move_p += extract_king_moves(&board, &lookup, moves, move_p);

    char expected_uci_2[][5] = {"f8e7", "f8f7", "f8g7", "f8g8"};
    char actual_uci_2[sizeof(expected_uci_2) / sizeof(expected_uci_2[0])][5];

    const size_t len2 = sizeof(expected_uci_2) / sizeof(expected_uci_2[0]);
    if (move_p != len2) {
        printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_2[i]);
    }

    qsort(expected_uci_2, len2, sizeof(expected_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_2, len2, sizeof(actual_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len2; i++) {
        if (strcmp(expected_uci_2[i], actual_uci_2[i]) != 0) {
            printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    printf("test_extract_king_moves: All tests passed.\n");
}

void test_extract_knight_moves(void) {
    ChessBoard board;
    U64 moves[256] = {0};
    int move_p = 0;

    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    // Test 1
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board, "n7/1Pnp1p1b/2QNrbP1/1r3p1P/2PP2P1/kp5p/N2p3q/3K4 w - - 0 1");

    move_p += extract_knight_moves(&board, &lookup, moves, move_p);

    char expected_uci_1[][5] = {"a2b4", "a2c3", "a2c1", "d6c8", "d6e8",
                                "d6f7", "d6f5", "d6e4", "d6b5"};
    char actual_uci_1[sizeof(expected_uci_1) / sizeof(expected_uci_1[0])][5];

    const size_t len1 = sizeof(expected_uci_1) / sizeof(expected_uci_1[0]);
    if (move_p != len1) {
        printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_1[i]);
    }

    qsort(expected_uci_1, len1, sizeof(expected_uci_1[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_1, len1, sizeof(actual_uci_1[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len1; i++) {
        if (strcmp(expected_uci_1[i], actual_uci_1[i]) != 0) {
            printf("[%s %s:%d] Test 1 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    // Test 2
    memset(moves, 0, sizeof(moves));
    move_p = 0;
    ChessBoard_from_FEN(
        &board, "6N1/3p2Pp/3RBrk1/P1PPbq2/5pp1/2BPp3/1n2Pp2/QR3K2 b - - 0 1");

    move_p += extract_knight_moves(&board, &lookup, moves, move_p);

    char expected_uci_2[][5] = {"b2a4", "b2c4", "b2d3", "b2d1"};
    char actual_uci_2[sizeof(expected_uci_2) / sizeof(expected_uci_2[0])][5];

    const size_t len2 = sizeof(expected_uci_2) / sizeof(expected_uci_2[0]);
    if (move_p != len2) {
        printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
        return;
    }

    for (int i = 0; i < move_p; i++) {
        move_to_uci(moves[i], actual_uci_2[i]);
    }

    qsort(expected_uci_2, len2, sizeof(expected_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);
    qsort(actual_uci_2, len2, sizeof(actual_uci_2[0]),
          (int (*)(const void *, const void *))strcmp);

    for (size_t i = 0; i < len2; i++) {
        if (strcmp(expected_uci_2[i], actual_uci_2[i]) != 0) {
            printf("[%s %s:%d] Test 2 failed.\n", __func__, __FILE__, __LINE__);
            return;
        }
    }

    printf("test_extract_knight_moves: All tests passed.\n");
}

// Debugging with printf
void debug_gen_occupancy_rook(void) {
    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    U64 *occupancy_mask = lookup.rook_mask;

    gen_occupancy_rook(&lookup);

    for (int i = 0; i < 64; i++) {
        printf("Bitboard for Square %d:\n", i);
        print_bb(occupancy_mask[i]);
        printf("\n");
    }
}

void debug_find_magic_rook(void) {
    LookupTable lookup = {0};
    init_LookupTable(&lookup);

    U64 magic;
    int sq;

    gen_occupancy_rook(&lookup);

    for (sq = 0; sq < 64; sq++) {
        magic = find_magic(lookup.rook_mask, 0, rook);
        printf("sq: %d, magic: %llu\n", sq, magic);
    }
}

void unit_test(void) {
    test_rightmost_set();

    test_ChessBoard_str();
    test_ChessBoard_to_FEN();

    test_extract_pawn_moves();

    test_gen_occupancy_rook();
    test_manual_gen_rook_moves();

    test_gen_occupancy_bishop();
    test_manual_gen_bishop_moves();

    test_init_magic_rook();
    test_init_magic_bishop();

    test_extract_magic_moves_rook();
    test_extract_magic_moves_bishop();
    test_extract_queen_moves();
    test_extract_king_moves();
    test_extract_knight_moves();

    printf("Finished unit tests.\n");
}

void debug_print(void) {
    printf("========== debug_gen_occupancy_rook ==========\n");
    debug_gen_occupancy_rook();

    printf("========== debug_find_magic_rook ==========\n");
    debug_find_magic_rook();
}

int main(void) {
    init_genrand64(0x8c364d19345930e2);  // drawn on random day

    // debug_print();
    unit_test();
    return 0;
}
