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

#define EMPTY_SQ 0
#define WHITE_PAWN 1
#define WHITE_ROOK 2
#define WHITE_KNIGHT 3
#define WHITE_BISHOP 4
#define WHITE_QUEEN 5
#define WHITE_KING 6
#define BLACK_PAWN 7
#define BLACK_ROOK 8
#define BLACK_KNIGHT 9
#define BLACK_BISHOP 10
#define BLACK_QUEEN 11
#define BLACK_KING 12

// board.white_to_move = 1 means we can do side = board.white_to_move in fn
// calls
#define WHITE 1
#define BLACK 0

#endif  // COMMON_H
