#include "hash_table.h"
#include "rng.h"
#include <string.h>

hash_entry_t hash_table[HASH_TABLE_SIZE];

void init_hash_table(void) {
	memset(hash_table, 0, sizeof(hash_table));
}

// helpers
hash_flag_t flag(u64 entry) {
	return entry & 0x3;
}

i16 score(u64 entry) {
	return entry >> 2 & 0xFFFF;
}

u16 depth(u64 entry) {
	return entry >> 18 & 0x7F;
}

u64 move(u64 entry) {
	return entry >> 25 & 0x7FFFFFFFFFULL;
}

// hash fns
u64 probe(u64 hash) {
	// We use a simple hash table, no buckets, always replace
	// TODO Update to bucketed version
	u64 ind = hash & HASH_TABLE_AND;
	return hash_table[ind].hash == hash ? hash_table[ind].entry : 0;
}

void store(u64 hash, hash_flag_t flag, i16 score, u16 depth, u64 move) {
	// always replace TODO update
	u64 ind = hash & HASH_TABLE_AND;
	u64 entry = flag | score << 2 | depth << 18 | move << 25;
	hash_table[ind].hash = hash;
	hash_table[ind].entry = entry;
}

void raw_store(u64 hash, u64 entry) {
	u64 ind = hash & HASH_TABLE_AND;
	hash_table[ind].hash = hash;
	hash_table[ind].entry = entry;
}

/*
    // Score table
    int table_score, table_depth;
    ScoreTable::Flag flag;
    if (score_table.probe(pos.get_hash(), table_score, flag, table_depth) 
     && table_depth >= std::max(depth, 0)) {
        hash_hits++;
        if (flag == ScoreTable::Flag::exact) {
            hash_cutoffs++;
            return table_score;
        } 
        if (flag == ScoreTable::Flag::lower)
            alpha = std::max(alpha, table_score);
        else if (flag == ScoreTable::Flag::upper)
            beta = std::min(beta, table_score);
        if (alpha >= beta) {
            hash_cutoffs++;
            return table_score;
        }
    }


    // Get hash move else IID in PV
    Move hash_move;
    bool hash_move_valid = move_table.probe(pos.get_hash(), hash_move);
    if (hash_move_valid)
        hash_move_found++;


    // Hash move / IID move
    if (hash_move_valid) {
        move_counter++;
        make_move(pos, hash_move);
        int score = -alphabeta(true, pos, depth - 1, -beta, -alpha, -1, ply + 1);
        unmake_move(pos, hash_move);

        if (score >= beta) {
            hashmove_cutoffs++;
            first_move_cuts++;
            moves_until_cutoff += move_counter;
            beta_cutoffs++;
            score_table.store(pos.get_hash(), score, ScoreTable::Flag::lower, depth);
            move_table.store(pos.get_hash(), hash_move);
            return beta;
        } 
        if (score > alpha) {
            alpha = score;
            alpha_raised = true;
            best_move = hash_move;
            hash_move_alpha_raised++;
        } else {
            hash_move_failure++;
        }
    }


*/
