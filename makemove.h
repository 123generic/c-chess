#ifndef MAKEMOVE_H
#define MAKEMOVE_H

#include "board.h"

ChessBoard make_move(ChessBoard board, u64 move);
ChessBoard null_move(ChessBoard board);
int zugzwang(ChessBoard *board, u64 attack_mask);

// Move selection TODO
u64 select_move(u64 *moves, int num_moves);

// Move representation
// moves are packed into a u64 (LSB -> MSB)
// 0-5: from square
// 6-11: to square
// 12-15: piece
// 16-19: capture piece
// 20-23: move type
// 24-27: promote type

// unimportant below
// 28-43: score

// move utilities
int from(u64 move);
int to(u64 move);
int piece(u64 move);
int captured(u64 move);
int move_type(u64 move);
int promote_type(u64 move);

#endif  // MAKEMOVE_H
