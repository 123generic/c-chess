#include "hash_table.h"
#include "rng.h"
#include <string.h>
#include <assert.h>

hash_entry_t hash_table[HASH_TABLE_SIZE];

void init_hash_table(void) {
	memset(hash_table, 0, sizeof(hash_table));
}

// helpers
hash_flag_t hf_flag(u64 entry) {
	return entry & 0x3;
}

i16 hf_score(u64 entry) {
	return entry >> 2 & 0xFFFF;
}

u16 hf_depth(u64 entry) {
	return entry >> 18 & 0xFFFF;
}

u64 hf_move(u64 entry) {
	return entry >> 34 & 0x3FFFFFFFULL;
}

// hash fns
u64 probe(u64 hash) {
	// We use a simple hash table, no buckets, always replace
	// TODO Update to bucketed version
	u64 ind = hash & HASH_TABLE_AND;
	if (hash_table[ind].hash == hash && hf_depth(hash_table[ind].entry) == UINT16_MAX) {
		// bad entry
		assert(0);
	}
	return hash_table[ind].hash == hash ? hash_table[ind].entry : 0;
}

void store(u64 hash, hash_flag_t flag, i16 score, u16 depth, u64 move) {
	// always replace TODO update
	u64 ind = hash & HASH_TABLE_AND;
	move &= 0x3FFFFFFFULL;
	// Note bit magic: score & 0xFFFF causes score to promote to unsigned without sign extension
	u64 entry = (u64)flag | (u64)(score & 0xFFFF) << 2 | (u64)depth << 18 | (u64)move << 34;
	hash_table[ind].hash = hash;
	hash_table[ind].entry = entry;
	if (hf_depth(hash_table[ind].entry) == UINT16_MAX) {
		// bad entry
		assert(0);
	}
}

void raw_store(u64 hash, u64 entry) {
	u64 ind = hash & HASH_TABLE_AND;
	hash_table[ind].hash = hash;
	hash_table[ind].entry = entry;
}
