#ifndef EVAL_H
#define EVAL_H

#include "board.h"

extern int gamephase_inc[6];
extern int mg_table[2][6][64], eg_table[2][6][64];

void init_tables(void);
void manual_score_gen(ChessBoard *board);
int eval(ChessBoard *board);

#endif  // EVAL_H
