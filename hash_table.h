#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "common.h"

// 1 << 23 -> 1GB hash table
// 1 << 24 -> 2GB hash table
#define HASH_TABLE_SIZE (1 << 21)
#define HASH_TABLE_AND (0x1FFFFF)

// entry:
// low, high, exact       : 0-2
// score (-20000 - 20000) : 3-18
// depth                  : 19-25
// move                   : 26-64
typedef enum {
	lower  = 0,
	higher = 1,
	exact  = 2,
} hash_flag_t;

typedef struct {
	u64 hash;
	u64 entry;
} hash_entry_t;

extern hash_entry_t hash_table[HASH_TABLE_SIZE];

// hash table
void init_hash_table(void);
u64 probe(u64 hash);
void store(u64 hash, hash_flag_t flag, i16 score, u16 depth, u64 move);
void raw_store(u64 hash, u64 entry);

// helpers
hash_flag_t flag(u64 entry);
i16 score(u64 entry);
u16 depth(u64 entry);
u64 move(u64 entry);

#endif  // HASH_TABLE_H
