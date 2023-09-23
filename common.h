#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

// Types
typedef uint64_t U64;

// Constant Macros
#define RANK_1 0xff
#define RANK_2 0xff00
#define RANK_3 0xff0000
#define RANK_4 0xff000000
#define RANK_5 0xff00000000
#define RANK_6 0xff0000000000
#define RANK_7 0xff000000000000
#define RANK_8 0xff00000000000000

#define FILE_1 0x8080808080808080
#define FILE_2 0x4040404040404040
#define FILE_3 0x2020202020202020
#define FILE_4 0x1010101010101010
#define FILE_5 0x808080808080808
#define FILE_6 0x404040404040404
#define FILE_7 0x202020202020202
#define FILE_8 0x101010101010101

typedef enum {
	white = 0,
	black = 1,
	all = 2,  // used for bitboard indexing
} Side;

// Piece enum (must fit in nibble)
typedef enum {
	empty = 15,
    pawn = 0,
    rook = 2,
    knight = 4,
    bishop = 6,
    queen = 8,
    king = 10,
} Piece;

extern const int all_pieces;  // used for bitboard indexing (12, defined in board.c)

#endif  // COMMON_H
