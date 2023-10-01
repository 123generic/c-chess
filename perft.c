#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "common.h"
#include "eval.h"
#include "hash_table.h"
#include "lookup.h"
#include "makemove.h"
#include "movegen.h"
#include "rng.h"

u64 perft(ChessBoard board, int depth) {
    if (depth == 0) return 1;

    u64 total_moves = 0;
    u64 moves[256];
    int num_moves;

    MoveGenStage stage[] = {promotions, captures, castling, quiets};
    int len = sizeof(stage) / sizeof(stage[0]);
    for (int i = 0; i < len; i++) {
        num_moves = generate_moves(&board, moves,
                                   attackers(&board, !board.side), stage[i]);

        for (int move_p = 0; move_p < num_moves; move_p++) {
            ChessBoard new_board = make_move(board, moves[move_p]);
            if (is_legal(&new_board, attackers(&new_board, new_board.side),
                         !new_board.side)) {
                total_moves += perft(new_board, depth - 1);
            }
        }
    }

    return total_moves;
}

u64 _test_zobrist_helper(ChessBoard board, int depth) {
    if (depth == 0) return 1;

    u64 total_moves = 0;
    u64 moves[256];
    int num_moves;

    MoveGenStage stage[] = {promotions, captures, castling, quiets};
    int len = sizeof(stage) / sizeof(stage[0]);
    for (int i = 0; i < len; i++) {
        num_moves = generate_moves(&board, moves,
                                   attackers(&board, !board.side), stage[i]);

        for (int move_p = 0; move_p < num_moves; move_p++) {
            ChessBoard new_board = make_move(board, moves[move_p]);
            assert(new_board.hash == manual_compute_hash(&new_board));
            if (is_legal(&new_board, attackers(&new_board, new_board.side),
                         !new_board.side)) {
                total_moves += perft(new_board, depth - 1);
            }
        }
    }

    return total_moves;
}

u64 hash_hits = 0, greater_hits = 0, hash_stores = 0;
u64 _test_hash_table_helper(ChessBoard board, int depth) {
    // load
    u64 entry;
    if ((entry = probe(board.hash)) && (int)(entry & 0xFF) >= depth) {
        if ((entry & 0xFF) == depth) {
            hash_hits++;
            return (entry >> 8) & 0xFFFFFFFFFFFFFF;
        }
        greater_hits++;
    }

    if (depth == 0) return 1;

    u64 total_moves = 0;
    u64 moves[256];
    int num_moves;

    MoveGenStage stage[] = {promotions, captures, castling, quiets};
    int len = sizeof(stage) / sizeof(stage[0]);
    u64 attack_mask = attackers(&board, !board.side);
    for (int i = 0; i < len; i++) {
        num_moves = generate_moves(&board, moves, attack_mask, stage[i]);

        for (int move_p = 0; move_p < num_moves; move_p++) {
            ChessBoard new_board = make_move(board, moves[move_p]);
            if (is_legal(&new_board, attackers(&new_board, new_board.side),
                         !new_board.side)) {
                total_moves += _test_hash_table_helper(new_board, depth - 1);
            }
        }
    }

    raw_store(board.hash, (depth & 0xFF) | ((total_moves << 8) &
                                            0xFFFFFFFFFFFFFF));  // store
    hash_stores++;
    return total_moves;
}

u64 _test_incremental_eval(ChessBoard board, int depth) {
    if (depth == 0) return 1;

    u64 total_moves = 0;
    u64 moves[256];
    int num_moves;

    MoveGenStage stage[] = {promotions, captures, castling, quiets};
    int len = sizeof(stage) / sizeof(stage[0]);
    for (int i = 0; i < len; i++) {
        num_moves = generate_moves(&board, moves,
                                   attackers(&board, !board.side), stage[i]);

        for (int move_p = 0; move_p < num_moves; move_p++) {
            ChessBoard new_board = make_move(board, moves[move_p]);

            // check for right scores
            int mg[2], eg[2], game_phase;
            memcpy(mg, new_board.mg, sizeof(new_board.mg));
            memcpy(eg, new_board.eg, sizeof(new_board.eg));
            game_phase = new_board.game_phase;

            manual_score_gen(&new_board);
            assert(mg[0] == new_board.mg[0] && mg[1] == new_board.mg[1] &&
                   eg[0] == new_board.eg[0] && eg[1] == new_board.eg[1] &&
                   game_phase == new_board.game_phase);

            if (is_legal(&new_board, attackers(&new_board, new_board.side),
                         !new_board.side)) {
                total_moves += _test_incremental_eval(new_board, depth - 1);
            }
        }
    }

    return total_moves;
}

u64 divide(ChessBoard board, int depth) {
    if (depth == 0) return 1;

    u64 total_moves = 0;
    u64 moves[256];
    int num_moves;

    MoveGenStage stage[] = {promotions, captures, castling, quiets};
    int len = sizeof(stage) / sizeof(stage[0]);
    for (int i = 0; i < len; i++) {
        num_moves = generate_moves(&board, moves,
                                   attackers(&board, !board.side), stage[i]);

        for (int move_p = 0; move_p < num_moves; move_p++) {
            ChessBoard new_board = make_move(board, moves[move_p]);
            if (is_legal(&new_board, attackers(&new_board, new_board.side),
                         !new_board.side)) {
                u64 nodes = perft(new_board, depth - 1);
                char ascii_move[6];
                move_to_uci(moves[move_p], ascii_move);
                printf("%s: %llu\n", ascii_move, nodes);
                total_moves += nodes;
            }
        }
    }

    return total_moves;
}

int main(void) {
    global_init();

    ChessBoard board;
    ChessBoard_from_FEN(
        &board,
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    // ChessBoard_from_FEN(&board, "k7/8/8/8/8/8/8/K7 w - - 0 1");

    for (int depth = 1; depth <= 5; depth++) {
        // u64 nodes = perft(board, depth);
        hash_hits = 0;
        hash_stores = 0;
        u64 nodes = _test_hash_table_helper(board, depth);
        printf("Depth %d: %llu, Hits: %llu, Stores: %llu\n", depth, nodes,
               hash_hits, hash_stores);
    }

    return 0;
}
