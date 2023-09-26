#include "board.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lookup.h"
#include "movegen.h"

const int all_pieces = 12;

// Bitboard helpers
int rightmost_set(U64 bb) {
    if (bb == 0) return -1;
    // count trailing zeros unsigned long long
    return __builtin_ctzll(bb);

    // Alternative implementation ...
    // int i = 0;
    // while (bb != 0 && ((bb & 1) == 0)) {
    //     bb >>= 1;
    //     i++;
    // }

    // return i;
}

void print_bb(U64 bb) {
    int i;
    for (i = 0; i < 64; i++) {
        if (i % 8 == 0 && i != 0) {
            printf("\n");
        }
        printf("%lu", (bb >> (63 - i)) & 1);
    }
    printf("\n");
}

// string must have length 64 + 1
U64 make_bitboard(char *str) {
    U64 bb;

    bb = 0;
    for (int i = 0; i < 64; i++) {
        bb = bb << 1;
        bb += (str[i] - '0');
    }

    if (str[64] != '\0') {
        fprintf(stderr, "Error: bitboard string is wrong length [%s(%s):%d]\n",
                __FILE__, __func__, __LINE__);
        exit(1);
    }

    return bb;
}

// ChessBoard
void init_ChessBoard(ChessBoard *board) {
    ChessBoard_from_FEN(
        board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

// This function will break if FEN is invalid lol
void ChessBoard_from_FEN(ChessBoard *board, char *fen) {
    // Set all 0
    memset(board, 0, sizeof(ChessBoard));

    // Set bitboards
    int i = 0, ind = 63;
    for (/* using i, ind */; fen[i] != ' '; i++, ind--) {
        switch (fen[i]) {
            case 'p':
                board->bitboards[black + pawn] |= 1ULL << ind;
                break;
            case 'r':
                board->bitboards[black + rook] |= 1ULL << ind;
                break;
            case 'n':
                board->bitboards[black + knight] |= 1ULL << ind;
                break;
            case 'b':
                board->bitboards[black + bishop] |= 1ULL << ind;
                break;
            case 'q':
                board->bitboards[black + queen] |= 1ULL << ind;
                break;
            case 'k':
                board->bitboards[black + king] |= 1ULL << ind;
                break;
            case 'P':
                board->bitboards[white + pawn] |= 1ULL << ind;
                break;
            case 'R':
                board->bitboards[white + rook] |= 1ULL << ind;
                break;
            case 'N':
                board->bitboards[white + knight] |= 1ULL << ind;
                break;
            case 'B':
                board->bitboards[white + bishop] |= 1ULL << ind;
                break;
            case 'Q':
                board->bitboards[white + queen] |= 1ULL << ind;
                break;
            case 'K':
                board->bitboards[white + king] |= 1ULL << ind;
                break;
            case '/':
                ind++;
                break;
            default:
                // we're at a number
                // Note: -1 for the ind-- in the for loop
                ind -= fen[i] - '0' - 1;
        }
    }
    i++;  // skip the space

    board->bitboards[all_pieces + white] =
        board->bitboards[white + pawn] | board->bitboards[white + rook] |
        board->bitboards[white + knight] | board->bitboards[white + bishop] |
        board->bitboards[white + queen] | board->bitboards[white + king];

    board->bitboards[all_pieces + black] =
        board->bitboards[black + pawn] | board->bitboards[black + rook] |
        board->bitboards[black + knight] | board->bitboards[black + bishop] |
        board->bitboards[black + queen] | board->bitboards[black + king];

    board->bitboards[all_pieces + all] = board->bitboards[all_pieces + white] |
                                         board->bitboards[all_pieces + black];

    // Set side to move
    board->side = fen[i] == 'w' ? white : black;
    i += 2;  // space and skip

    // Set castling rights
    // Danger: i++ used to skip forward one
    if (fen[i] == '-') {
        i += 2;  // skip the dash and the space
    } else {
        board->KC[white] = fen[i] == 'K';
        if (board->KC[white]) i++;

        board->QC[white] = fen[i] == 'Q';
        if (board->QC[white]) i++;

        board->KC[black] = fen[i] == 'k';
        if (board->KC[black]) i++;

        board->QC[black] = fen[i] == 'q';
        if (board->QC[black]) i++;

        i++;  // space
    }

    // EP square
    if (fen[i] != '-') {
        board->ep = 8 * (fen[i + 1] - '0') - (fen[i] - 'a') - 1;
        i += 3;  // skip the square and the space
    } else {
        board->ep = -1;
        i += 2;  // skip the dash and the space
    }

    // Set halfmove clock
    board->halfmove_clock = 0;
    while (fen[i] != ' ') {
        board->halfmove_clock *= 10;
        board->halfmove_clock += fen[i] - '0';
        i++;
    }
    i++;

    board->fullmove_number = 0;
    int cnt = 0;
    while (fen[i] != '\0') {
        if (cnt > 2) {
            fprintf(stderr, "Error: FEN string is invalid [%s(%s):%d]\n",
                    __FILE__, __func__, __LINE__);
            exit(1);
        }
        board->fullmove_number *= 10;
        board->fullmove_number += fen[i] - '0';
        i++;
        cnt++;
    }

    // Check that we're done, or error
    if (fen[i] != '\0') {
        fprintf(stderr, "Error: FEN string is invalid [%s(%s):%d]\n", __FILE__,
                __func__, __LINE__);
        exit(1);
    }
}

void _ChessBoard_str_helper(char *str, U64 bb, char piece) {
    int ind;
    while ((ind = rightmost_set(bb)) != -1) {
        str[63 - ind] = piece;
        BB_CLEAR(bb, ind);
    }
}

// Caller's responsibility to allocate 64 + 1 bytes in str
void ChessBoard_str(ChessBoard *board, char *str) {
    // setup empty board
    memset(str, '.', 64 + 1);
    str[64] = '\0';

    // set pieces
    char pieces[] = "PpRrNnBbQqKk";
    for (Side side = white; side <= black; side++) {
        for (Piece piece = pawn; piece <= king; piece += 2) {
            _ChessBoard_str_helper(str, board->bitboards[side + piece],
                                   pieces[side + piece]);
        }
    }
}

// str should have 128 chars allocated for roundness
void ChessBoard_to_FEN(ChessBoard *board, char *str) {
    char board_str[65];
    int str_p, run_len, ind;
    int row, col;

    memset(str, 0, 128);
    ChessBoard_str(board, board_str);

    // Write board
    str_p = 0;
    for (row = 0; row < 8; row++) {
        // Process single row
        run_len = 0;
        for (col = 0; col < 8; col++) {
            ind = row * 8 + col;

            if (board_str[ind] == '.') {
                run_len++;
            } else {
                if (run_len != 0) {
                    str[str_p++] = '0' + run_len;
                    run_len = 0;
                }

                str[str_p++] = board_str[ind];
            }
        }

        if (run_len != 0) {
            str[str_p++] = '0' + run_len;
            run_len = 0;
        }

        if (row != 7) str[str_p++] = '/';
    }

    str[str_p++] = ' ';

    // Write side to move
    str[str_p++] = board->side == white ? 'w' : 'b';

    str[str_p++] = ' ';

    // Write castling rights
    if (board->KC[white] || board->QC[white] || board->KC[black] || board->QC[black]) {
        if (board->KC[white]) str[str_p++] = 'K';
        if (board->QC[white]) str[str_p++] = 'Q';
        if (board->KC[black]) str[str_p++] = 'k';
        if (board->QC[black]) str[str_p++] = 'q';
    } else {
        str[str_p++] = '-';
    }

    str[str_p++] = ' ';

    // Write EP square
    if (board->ep != -1) {
        str[str_p++] = 'a' + 7 - board->ep % 8;
        str[str_p++] = '0' + board->ep / 8 + 1;
    } else {
        str[str_p++] = '-';
    }

    str[str_p++] = ' ';

    // Write halfmove clock
    str_p += snprintf(str + str_p, 4, "%d", board->halfmove_clock);

    str[str_p++] = ' ';

    // Write fullmove number
    str_p += snprintf(str + str_p, 4, "%d", board->fullmove_number);

    str[str_p] = '\0';
}

Piece ChessBoard_piece_at(ChessBoard *board, int ind) {
    U64 loc = 1ULL << ind;
    if ((board->bitboards[all_pieces + all] & loc) == 0) {
        return empty;
    }
    if (board->bitboards[all_pieces + white] & loc) {
        for (Piece piece = pawn; piece <= king; piece++) {
            if (board->bitboards[white + piece] & loc) {
                return piece;
            }
        }
    } else {
        for (Piece piece = pawn; piece <= king; piece++) {
            if (board->bitboards[black + piece] & loc) {
                return piece;
            }
        }
    }

    fprintf(stderr, "Error: piece_at called on empty square [%s(%s):%d]\n",
            __FILE__, __func__, __LINE__);
    exit(1);
}
