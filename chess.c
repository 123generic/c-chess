#include "chess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf("%llu", (bb >> (63 - i)) & 1);
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
    board->white_pawns = 0xff000000000000;
    board->white_rooks = 0x8100000000000000;
    board->white_knights = 0x4200000000000000;
    board->white_bishops = 0x2400000000000000;
    board->white_queens = 0x1000000000000000;
    board->white_king = 0x800000000000000;
    board->black_pawns = 0xff00;
    board->black_rooks = 0x81;
    board->black_knights = 0x42;
    board->black_bishops = 0x24;
    board->black_queens = 0x10;
    board->black_king = 0x8;

    board->white_to_move = 1;  // True
}

// This function will break if FEN is invalid lol
void ChessBoard_from_FEN(ChessBoard *board, char *fen) {
    // Set all 0
    memset(board, 0, sizeof(ChessBoard));

// Set bitboards
#define SET(bb, ind) board->bb |= 1ULL << ind
    int i = 0, ind = 63;
    for (/* using i, ind */; fen[i] != ' '; i++, ind--) {
        switch (fen[i]) {
            case 'p':
                SET(black_pawns, ind);
                break;
            case 'r':
                SET(black_rooks, ind);
                break;
            case 'n':
                SET(black_knights, ind);
                break;
            case 'b':
                SET(black_bishops, ind);
                break;
            case 'q':
                SET(black_queens, ind);
                break;
            case 'k':
                SET(black_king, ind);
                break;
            case 'P':
                SET(white_pawns, ind);
                break;
            case 'R':
                SET(white_rooks, ind);
                break;
            case 'N':
                SET(white_knights, ind);
                break;
            case 'B':
                SET(white_bishops, ind);
                break;
            case 'Q':
                SET(white_queens, ind);
                break;
            case 'K':
                SET(white_king, ind);
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
#undef SET

    board->white_pieces = board->white_pawns | board->white_rooks |
                          board->white_knights | board->white_bishops |
                          board->white_queens | board->white_king;
    board->black_pieces = board->black_pawns | board->black_rooks |
                          board->black_knights | board->black_bishops |
                          board->black_queens | board->black_king;
    board->all_pieces = board->white_pieces | board->black_pieces;

    // Set side to move
    board->white_to_move = fen[i] == 'w';
    i += 2;  // space and skip

    // Set castling rights
    // Danger: i++ used to skip forward one
    if (fen[i] == '-') {
        i += 2;  // skip the dash and the space
    } else {
        board->KC = fen[i++] == 'K';
        board->QC = fen[i++] == 'Q';
        board->kc = fen[i++] == 'k';
        board->qc = fen[i++] == 'q';
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

    // Set fullmove clock
    board->halfmove_clock = fen[i] - '0';
    i += 2;  // skip the number and the space

    // Set halfmove clock
    board->fullmove_number = fen[i] - '0';
    i++;  // skip the number

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
    _ChessBoard_str_helper(str, board->white_pawns, 'P');
    _ChessBoard_str_helper(str, board->white_rooks, 'R');
    _ChessBoard_str_helper(str, board->white_knights, 'N');
    _ChessBoard_str_helper(str, board->white_bishops, 'B');
    _ChessBoard_str_helper(str, board->white_queens, 'Q');
    _ChessBoard_str_helper(str, board->white_king, 'K');
    _ChessBoard_str_helper(str, board->black_pawns, 'p');
    _ChessBoard_str_helper(str, board->black_rooks, 'r');
    _ChessBoard_str_helper(str, board->black_knights, 'n');
    _ChessBoard_str_helper(str, board->black_bishops, 'b');
    _ChessBoard_str_helper(str, board->black_queens, 'q');
    _ChessBoard_str_helper(str, board->black_king, 'k');
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
    str[str_p++] = board->white_to_move ? 'w' : 'b';

    str[str_p++] = ' ';

    // Write castling rights
    if (board->KC || board->QC || board->kc || board->qc) {
        if (board->KC) str[str_p++] = 'K';
        if (board->QC) str[str_p++] = 'Q';
        if (board->kc) str[str_p++] = 'k';
        if (board->qc) str[str_p++] = 'q';
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

int ChessBoard_piece_at(ChessBoard *board, int ind) {
    U64 loc = 1ULL << ind;
    if ((board->all_pieces & loc) == 0) {
        return EMPTY_SQ;
    }
    if (board->white_pieces & loc) {
        if (board->white_pawns & loc)
            return WHITE_PAWN;
        else if (board->white_rooks & loc)
            return WHITE_ROOK;
        else if (board->white_knights & loc)
            return WHITE_KNIGHT;
        else if (board->white_bishops & loc)
            return WHITE_BISHOP;
        else if (board->white_queens & loc)
            return WHITE_QUEEN;
        return WHITE_KING;
    } else {
        if (board->black_pawns & loc)
            return BLACK_PAWN;
        else if (board->black_rooks & loc)
            return BLACK_ROOK;
        else if (board->black_knights & loc)
            return BLACK_KNIGHT;
        else if (board->black_bishops & loc)
            return BLACK_BISHOP;
        else if (board->black_queens & loc)
            return BLACK_QUEEN;
        return BLACK_KING;
    }
}

// Move generation
// moves are packed into a U64 (LSB -> MSB)
// 0-5: from square
// 6-11: to square
// 12-15: piece
// 16-19: capture piece
// 20-23: move type

U64 move_from_uci(ChessBoard *board, char *uci) {
    // uci: rank-file rank-file [promotion]
    // TODO: promotion
    int from, to, piece, captured;
    from = 7 - (uci[0] - 'a') + 8 * (uci[1] - '1');
    to = 7 - (uci[2] - 'a') + 8 * (uci[3] - '1');

    piece = ChessBoard_piece_at(board, from);
    captured = ChessBoard_piece_at(board, to);
    if (piece == 0) {
        fprintf(stderr, "Error: No piece at %s [%s(%s):%d]\n", uci, __FILE__,
                __func__, __LINE__);
        exit(1);
    }
    return from | to << 6 | piece << 12 | captured << 16 | UNKNOWN << 20;
}

// move_p points to the end of the moves array
// moves is assumed to be large enough to hold all moves (256)
U64 get_pawn_moves(ChessBoard *board, MoveType move_type) {
    U64 pawns, moved_pawns, rank_mask;
    int wtm = board->white_to_move;

    switch (move_type) {
        case SINGLE_PUSH:
            pawns = wtm ? board->white_pawns : board->black_pawns;

            // no promotions
            rank_mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & ~rank_mask;

            // shift one rank up/down
            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;

            // don't push into pieces
            moved_pawns = moved_pawns & ~board->all_pieces;
            break;

        default:
            printf("Error: Unimplemented pawn move type [%s(%s):%d]\n",
                   __FILE__, __func__, __LINE__);
            exit(1);
    }

    return moved_pawns;
}

int extract_pawn_moves(ChessBoard *board, U64 *moves, int move_p,
                       U64 pawn_moves, MoveType move_type) {
    int ind, to, from, piece, captured;
    int num_moves = 0;
    int wtm = board->white_to_move;

    switch (move_type) {
        case SINGLE_PUSH:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                // this means the move is ind + 8 -> ind
                to = ind;
                from = wtm ? ind - 8 : ind + 8;
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = EMPTY_SQ;
                moves[move_p + num_moves] =
                    from | to << 6 | piece << 12 | captured << 16 | move_type << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        default:
            printf("Error: Unimplemented pawn move type [%s(%s):%d]\n",
                   __FILE__, __func__, __LINE__);
            exit(1);
    }

    return num_moves;
}

// board[sq] should, of course, have a rook on it
U64 get_rook_moves(ChessBoard *board, MagicTable *magic_table, int sq) {
    U64 pieces, mask, magic, moves;
    int ind;

    pieces = board->all_pieces;
    mask = magic_table->occupancy_mask[sq];
    magic = magic_table->magic[sq];
    ind = ((pieces & mask) * magic) >> 52;

    moves = magic_table->move[4096 * sq + ind];
    return moves;
}

int extract_rook_moves(ChessBoard *board, U64 *moves, int move_p,
                       U64 rook_moves, int sq) {
    int ind, to, piece, captured;
    int wtm = board->white_to_move;
    int num_moves = 0;

    while ((ind = rightmost_set(rook_moves)) != -1) {
        // this means the move is ind + 8 -> ind
        to = ind;
        piece = wtm ? WHITE_ROOK : BLACK_ROOK;
        captured = EMPTY_SQ;
        moves[move_p + num_moves] = sq | to << 6 | piece << 12 | captured << 16;
        num_moves++;
        BB_CLEAR(rook_moves, ind);
    }

    return -1;  // TODO
}
