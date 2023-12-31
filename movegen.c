#include "movegen.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "common.h"
#include "eval.h"
#include "lookup.h"
#include "makemove.h"
#include "search.h"

// Utilities
u64 move_from_uci(ChessBoard *board, char *uci) {
    int from, to, piece, captured, promote_type = 0;
    MoveType move_type = UNKNOWN;

    from = 7 - (uci[0] - 'a') + 8 * (uci[1] - '1');
    to = 7 - (uci[2] - 'a') + 8 * (uci[3] - '1');

    piece = ChessBoard_piece_at(board, from);
    captured = ChessBoard_piece_at(board, to);

    if (piece == empty) {
        fprintf(stderr, "Error: No piece at %s [%s(%s):%d]\n", uci, __FILE__,
                __func__, __LINE__);
        exit(1);
    }

    // Check for CASTLE
    if (strcmp(uci, "e1g1") == 0 || strcmp(uci, "e8g8") == 0) {
        move_type = CASTLE_KING;
    } else if (strcmp(uci, "e1c1") == 0 || strcmp(uci, "e8c8") == 0) {
        move_type = CASTLE_QUEEN;
    }
    // Check for EN_PASSANT (a pawn moving diagonally to an empty square)
    else if (piece == pawn && captured == empty && (from % 8 != to % 8)) {
        move_type = EN_PASSANT;
    }
    // Check for PROMOTION
    else if (strlen(uci) == 5 && piece == pawn) {
        char promo = uci[4];
        switch (promo) {
            case 'q':
                promote_type = queen;
                break;
            case 'r':
                promote_type = rook;
                break;
            case 'b':
                promote_type = bishop;
                break;
            case 'n':
                promote_type = knight;
                break;
            default:
                fprintf(stderr, "Error: Invalid promotion %c [%s(%s):%d]\n",
                        promo, __FILE__, __func__, __LINE__);
                exit(1);
        }
        move_type = PROMOTION;
    } else {
        move_type = NORMAL;
    }

    return from | to << 6 | piece << 12 | captured << 16 | move_type << 20 |
           promote_type << 24;
}

// uci must have length 4+1 (or more)
void move_to_uci(u64 move, char *uci) {
    int from, to;
    MoveType type;
    Piece p;
    from = move & 0x3f;
    to = (move >> 6) & 0x3f;
    type = (move >> 20) & 0xf;
    if (type == PROMOTION) {
        p = (move >> 24) & 0xf;
        switch (p) {
            case queen:
                uci[4] = 'q';
                break;
            case rook:
                uci[4] = 'r';
                break;
            case bishop:
                uci[4] = 'b';
                break;
            case knight:
                uci[4] = 'n';
                break;
            default:
                fprintf(stderr, "Error: Invalid promotion piece [%s(%s):%d]\n",
                        __FILE__, __func__, __LINE__);
                exit(1);
        }

        uci[5] = '\0';
    } else {
        uci[4] = '\0';
    }

    uci[0] = 'a' + 7 - (from % 8);
    uci[1] = '1' + (from / 8);
    uci[2] = 'a' + 7 - (to % 8);
    uci[3] = '1' + (to / 8);
}

// Pawn generation
u64 get_pawn_moves(ChessBoard *board, PawnMoveType move_type) {
    u64 pawns, moved_pawns, mask, sq;
    int wtm;

    moved_pawns = 0;
    wtm = board->side == white;
    pawns = board->bitboards[board->side + pawn];

    switch (move_type) {
        case SINGLE_PUSH:
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & ~mask;
            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;
            moved_pawns = moved_pawns & ~board->bitboards[all_pieces + all];
            break;

        case DOUBLE_PUSH:
            mask = wtm ? RANK_2 : RANK_7;
            moved_pawns = pawns & mask;

            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;
            moved_pawns &= ~board->bitboards[all_pieces + all];

            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;
            moved_pawns &= ~board->bitboards[all_pieces + all];
            break;

        // Note: left relative to white player
        case CAPTURE_LEFT:
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & ~mask;
            moved_pawns = moved_pawns & ~FILE_1;
            moved_pawns = wtm ? moved_pawns << 9 : moved_pawns >> 7;
            moved_pawns &= board->bitboards[all_pieces + !board->side];
            break;

        case CAPTURE_RIGHT:
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & ~mask;
            moved_pawns = moved_pawns & ~FILE_8;
            moved_pawns = wtm ? moved_pawns << 7 : moved_pawns >> 9;
            moved_pawns &= board->bitboards[all_pieces + !board->side];
            break;

        case PAWN_PROMOTION:
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & mask;
            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;
            moved_pawns &= ~(board->bitboards[all_pieces + all]);
            break;

        case PROMOTION_CAPTURE_LEFT:
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & mask;
            moved_pawns = moved_pawns & ~FILE_1;
            moved_pawns = wtm ? moved_pawns << 9 : moved_pawns >> 7;
            moved_pawns &= board->bitboards[all_pieces + !board->side];
            break;

        case PROMOTION_CAPTURE_RIGHT:
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & mask;
            moved_pawns = moved_pawns & ~FILE_8;
            moved_pawns = wtm ? moved_pawns << 7 : moved_pawns >> 9;
            moved_pawns &= board->bitboards[all_pieces + !board->side];
            break;

        case EN_PASSANT_LEFT:
            if (board->ep == -1) {
                moved_pawns = 0;
            } else {
                sq = wtm ? board->ep - 9 : board->ep + 7;
                mask = ~FILE_1;

                moved_pawns = pawns & (1ULL << sq);
                moved_pawns &= mask;
                moved_pawns = wtm ? moved_pawns << 9 : moved_pawns >> 7;
            }
            break;

        case EN_PASSANT_RIGHT:
            if (board->ep == -1) {
                moved_pawns = 0;
            } else {
                sq = wtm ? board->ep - 7 : board->ep + 9;
                mask = ~FILE_8;

                moved_pawns = pawns & (1ULL << sq);
                moved_pawns &= mask;
                moved_pawns = wtm ? moved_pawns << 7 : moved_pawns >> 9;
            }
            break;
    }

    return moved_pawns;
}

int extract_pawn_moves(ChessBoard *board, u64 *moves, int move_p,
                       u64 pawn_moves, PawnMoveType move_type) {
    int to, from, captured;
    int num_moves = 0;
    int wtm = board->side == white;

    switch (move_type) {
        case SINGLE_PUSH:
            while (pawn_moves) {
                int ind = __builtin_ctzll(pawn_moves);
                to = ind;
                from = wtm ? ind - 8 : ind + 8;
                moves[move_p + num_moves] =
                    from | to << 6 | pawn << 12 | empty << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case DOUBLE_PUSH:
            while (pawn_moves) {
                int ind = __builtin_ctzll(pawn_moves);
                to = ind;
                from = wtm ? ind - 16 : ind + 16;
                moves[move_p + num_moves] =
                    from | to << 6 | pawn << 12 | empty << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case CAPTURE_LEFT:
            while (pawn_moves) {
                int ind = __builtin_ctzll(pawn_moves);
                to = ind;
                from = wtm ? ind - 9 : ind + 7;
                captured = ChessBoard_piece_at(board, to);
                moves[move_p + num_moves] =
                    from | to << 6 | pawn << 12 | captured << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case CAPTURE_RIGHT:
            while (pawn_moves) {
                int ind = __builtin_ctzll(pawn_moves);
                to = ind;
                from = wtm ? ind - 7 : ind + 9;
                captured = ChessBoard_piece_at(board, to);
                moves[move_p + num_moves] =
                    from | to << 6 | pawn << 12 | captured << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case PAWN_PROMOTION:
            while (pawn_moves) {
                int ind = __builtin_ctzll(pawn_moves);
                to = ind;
                from = wtm ? ind - 8 : ind + 8;

                int promote[4] = {queen, rook, bishop, knight};
                for (int i = 0; i < 4; i++) {
                    moves[move_p + num_moves] = from | to << 6 | pawn << 12 |
                                                empty << 16 | PROMOTION << 20 |
                                                promote[i] << 24;
                    num_moves++;
                }
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case PROMOTION_CAPTURE_LEFT:
            while (pawn_moves) {
                int ind = __builtin_ctzll(pawn_moves);
                to = ind;
                from = wtm ? ind - 9 : ind + 7;
                captured = ChessBoard_piece_at(board, to);

                int promote[4] = {queen, rook, bishop, knight};
                for (int i = 0; i < 4; i++) {
                    moves[move_p + num_moves] =
                        from | to << 6 | pawn << 12 | captured << 16 |
                        PROMOTION << 20 | promote[i] << 24;
                    num_moves++;
                }
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case PROMOTION_CAPTURE_RIGHT:
            while (pawn_moves) {
                int ind = __builtin_ctzll(pawn_moves);
                to = ind;
                from = wtm ? ind - 7 : ind + 9;
                captured = ChessBoard_piece_at(board, to);

                int promote[4] = {queen, rook, bishop, knight};
                for (int i = 0; i < 4; i++) {
                    moves[move_p + num_moves] =
                        from | to << 6 | pawn << 12 | captured << 16 |
                        PROMOTION << 20 | promote[i] << 24;
                    num_moves++;
                }
                BB_CLEAR(pawn_moves, ind);
            }
            break;

            int ind;
        case EN_PASSANT_LEFT:
            if (pawn_moves == 0) break;
            ind = __builtin_ctzll(pawn_moves);
            to = ind;
            from = wtm ? ind - 9 : ind + 7;

            moves[move_p + num_moves] =
                from | to << 6 | pawn << 12 | pawn << 16 | EN_PASSANT << 20;
            num_moves++;
            break;

        case EN_PASSANT_RIGHT:
            if (pawn_moves == 0) break;
            ind = __builtin_ctzll(pawn_moves);
            to = ind;
            from = wtm ? ind - 7 : ind + 9;

            moves[move_p + num_moves] =
                from | to << 6 | pawn << 12 | pawn << 16 | EN_PASSANT << 20;
            num_moves++;
            break;
    }

    return num_moves;
}

// Non-pawn move extraction
inline u64 get_attacks(ChessBoard *board, int sq, Piece p) {
    u64 pieces, mask, magic, moves;
    int ind, shift_amt;

    pieces = board->bitboards[all_pieces + all];
    switch (p) {
        case rook:
            mask = lookup.rook_mask[sq];
            magic = lookup.rook_magic[sq];
            shift_amt = 64 - 12;

            ind = ((pieces & mask) * magic) >> shift_amt;
            moves = lookup.rook_move[4096 * sq + ind];
            break;

        case bishop:
            mask = lookup.bishop_mask[sq];
            magic = lookup.bishop_magic[sq];
            shift_amt = 64 - 9;

            ind = ((pieces & mask) * magic) >> shift_amt;
            moves = lookup.bishop_move[512 * sq + ind];
            break;

        case queen:
            moves =
                get_attacks(board, sq, rook) | get_attacks(board, sq, bishop);
            break;

        case knight:
            moves = lookup.knight_move[sq];
            break;

        case king:
            moves = lookup.king_move[sq];
            break;

        default:
            fprintf(stderr, "Error: Invalid piece [%s(%s):%d]\n", __FILE__,
                    __func__, __LINE__);
            exit(1);
    }

    return moves;
}

u64 get_moves(ChessBoard *board, int sq, Piece p) {
    u64 friendlies = board->bitboards[all_pieces + board->side];
    return get_attacks(board, sq, p) & ~friendlies;
}

int extract_moves(ChessBoard *board, u64 *moves, int move_p, u64 move_bb,
                  int sq, Piece p, int quiet) {
    int to, captured;
    int num_moves = 0;

    while (move_bb) {
        int ind = __builtin_ctzll(move_bb);
        to = ind;
        captured = quiet ? empty : ChessBoard_piece_at(board, ind);
        moves[move_p + num_moves] =
            sq | to << 6 | p << 12 | captured << 16 | NORMAL << 20;
        num_moves++;
        BB_CLEAR(move_bb, ind);
    }

    return num_moves;
}

// For debugging only
int extract_all_moves(ChessBoard *board, u64 *moves, int move_p, Piece p) {
    int sq, num_moves;
    u64 bb, move_bb, enemies;

    num_moves = 0;
    bb = board->bitboards[board->side + p];
    enemies = board->bitboards[all_pieces + !board->side];

    while (bb) {
        sq = __builtin_ctzll(bb);
        move_bb = get_moves(board, sq, p);

        num_moves += extract_moves(board, moves, move_p + num_moves,
                                   move_bb & enemies, sq, p, 0);
        num_moves += extract_moves(board, moves, move_p + num_moves,
                                   move_bb & ~enemies, sq, p, 1);

        BB_CLEAR(bb, sq);
    }

    return num_moves;
}

// Castling
// attacked is squares attacked by !board->side (enemies)
int generate_castling(ChessBoard *board, u64 *moves, u64 attacked, int move_p) {
    int sq, king_to, num_moves = 0;
    u64 castle_mask, check_mask;

    if (board->side == white) {
        sq = __builtin_ctzll(board->bitboards[white + king]);

        if (board->KC[white]) {
            king_to = sq - 2;
            castle_mask = BB_SQUARE(sq - 1) | BB_SQUARE(sq - 2);
            check_mask = BB_SQUARE(sq) | BB_SQUARE(sq - 1) | BB_SQUARE(sq - 2);

            if (!(castle_mask & board->bitboards[all_pieces + all]) &&
                !(check_mask & attacked)) {
                moves[move_p + num_moves] = sq | king_to << 6 | king << 12 |
                                            empty << 16 | CASTLE_KING << 20;
                num_moves++;
            }
        }

        if (board->QC[white]) {
            king_to = sq + 2;
            castle_mask =
                BB_SQUARE(sq + 1) | BB_SQUARE(sq + 2) | BB_SQUARE(sq + 3);
            check_mask = BB_SQUARE(sq) | BB_SQUARE(sq + 1) | BB_SQUARE(sq + 2);

            if (!(castle_mask & board->bitboards[all_pieces + all]) &&
                !(check_mask & attacked)) {
                moves[move_p + num_moves] = sq | king_to << 6 | king << 12 |
                                            empty << 16 | CASTLE_QUEEN << 20;
                num_moves++;
            }
        }
    } else {
        sq = __builtin_ctzll(board->bitboards[black + king]);

        if (board->KC[black]) {
            king_to = sq - 2;
            castle_mask = BB_SQUARE(sq - 1) | BB_SQUARE(sq - 2);
            check_mask = BB_SQUARE(sq) | BB_SQUARE(sq - 1) | BB_SQUARE(sq - 2);

            if (!(castle_mask & board->bitboards[all_pieces + all]) &&
                !(check_mask & attacked)) {
                moves[move_p + num_moves] = sq | king_to << 6 | king << 12 |
                                            empty << 16 | CASTLE_KING << 20;
                num_moves++;
            }
        }

        if (board->QC[black]) {
            king_to = sq + 2;
            castle_mask =
                BB_SQUARE(sq + 1) | BB_SQUARE(sq + 2) | BB_SQUARE(sq + 3);
            check_mask = BB_SQUARE(sq) | BB_SQUARE(sq + 1) | BB_SQUARE(sq + 2);

            if (!(castle_mask & board->bitboards[all_pieces + all]) &&
                !(check_mask & attacked)) {
                moves[move_p + num_moves] = sq | king_to << 6 | king << 12 |
                                            empty << 16 | CASTLE_QUEEN << 20;
                num_moves++;
            }
        }
    }

    return num_moves;
}

// Attackers
// side: if `black`, gets squares attacked by black pieces, likewise for white
u64 attackers(ChessBoard *board, Side side) {
    u64 attack = 0;

    // Pawns
    u64 moved_pawns;
    const u64 pawns = board->bitboards[side + pawn];

    // left
    moved_pawns = pawns & ~FILE_1;
    moved_pawns = side == white ? moved_pawns << 9 : moved_pawns >> 7;
    attack |= moved_pawns;

    // right
    moved_pawns = pawns & ~FILE_8;
    moved_pawns = side == white ? moved_pawns << 7 : moved_pawns >> 9;
    attack |= moved_pawns;

    // EP
    if (board->ep != -1) {
        int sq;

        // left
        sq = side == white ? board->ep - 9 : board->ep + 7;
        moved_pawns = pawns & BB_SQUARE(sq);
        moved_pawns = side == white ? moved_pawns << 9 : moved_pawns >> 7;
        attack |= moved_pawns;

        // right
        sq = side == white ? board->ep - 7 : board->ep + 9;
        moved_pawns = pawns & BB_SQUARE(sq);
        moved_pawns = side == white ? moved_pawns << 7 : moved_pawns >> 9;
        attack |= moved_pawns;
    }

    // Other pieces
    u64 bb;

    // rook
    bb = board->bitboards[side + rook];
    while (bb) {
        int ind = __builtin_ctzll(bb);
        attack |= get_attacks(board, ind, rook);

        BB_CLEAR(bb, ind);
    }

    bb = board->bitboards[side + bishop];
    while (bb) {
        int ind = __builtin_ctzll(bb);
        attack |= get_attacks(board, ind, bishop);

        BB_CLEAR(bb, ind);
    }

    bb = board->bitboards[side + queen];
    while (bb) {
        int ind = __builtin_ctzll(bb);
        attack |= get_attacks(board, ind, queen);

        BB_CLEAR(bb, ind);
    }

    bb = board->bitboards[side + knight];
    while (bb) {
        int ind = __builtin_ctzll(bb);
        attack |= get_attacks(board, ind, knight);

        BB_CLEAR(bb, ind);
    }

    bb = board->bitboards[side + king];
    while (bb) {
        int ind = __builtin_ctzll(bb);
        attack |= get_attacks(board, ind, king);

        BB_CLEAR(bb, ind);
    }

    return attack;
}

int is_legal(ChessBoard *board, u64 attacked, Side side) {
    u64 king_bb = board->bitboards[side + king];
    u64 king_attackers = attacked & king_bb;

    return king_bb && king_attackers == 0;
}

// Move generation
int generate_promotions(ChessBoard *board, u64 *moves) {
    u64 pawn_moves;
    int num_moves = 0;

    PawnMoveType promotion_types[] = {PAWN_PROMOTION, PROMOTION_CAPTURE_LEFT,
                                      PROMOTION_CAPTURE_RIGHT};
    int len = sizeof(promotion_types) / sizeof(promotion_types[0]);

    for (int i = 0; i < len; i++) {
        pawn_moves = get_pawn_moves(board, promotion_types[i]);
        num_moves += extract_pawn_moves(board, moves, num_moves, pawn_moves,
                                        promotion_types[i]);
    }

    return num_moves;
}

int generate_normal_moves_pawn(ChessBoard *board, u64 *moves, int quiet) {
    u64 pawn_moves;
    int num_moves = 0;

    PawnMoveType capture_types[] = {CAPTURE_LEFT, CAPTURE_RIGHT,
                                    EN_PASSANT_LEFT, EN_PASSANT_RIGHT};
    PawnMoveType quiet_types[] = {SINGLE_PUSH, DOUBLE_PUSH};

    PawnMoveType *types = quiet ? quiet_types : capture_types;
    int len = quiet ? 2 : 4;

    for (int i = 0; i < len; i++) {
        pawn_moves = get_pawn_moves(board, types[i]);
        num_moves +=
            extract_pawn_moves(board, moves, num_moves, pawn_moves, types[i]);
    }

    return num_moves;
}

int generate_normal_moves(ChessBoard *board, u64 *moves, int quiet) {
    int num_moves = 0;

    // Pawns
    num_moves += generate_normal_moves_pawn(board, moves, quiet);

    // Others
    u64 enemies = board->bitboards[all_pieces + !board->side];
    Piece pieces[] = {rook, bishop, queen, knight, king};
    int len = sizeof(pieces) / sizeof(pieces[0]);

    // iterate through piece types
    for (int i = 0; i < len; i++) {
        Piece p = pieces[i];
        u64 piece_bb = board->bitboards[board->side + p];

        // iterate through piece locations)
        while (piece_bb) {
            int sq = __builtin_ctzll(piece_bb);
            u64 move_bb = get_moves(board, sq, p);
            move_bb &= (quiet ? ~enemies : enemies);  // **masking**

            num_moves +=
                extract_moves(board, moves, num_moves, move_bb, sq, p, 0);

            BB_CLEAR(piece_bb, sq);
        }
    }

    return num_moves;
}

void value_promotions(u64 *moves, int num_moves) {
    int piece_value[6] = {1, 5, 3, 3, 9, 1000};
    for (int i = 0; i < num_moves; i++) {
        u64 move = moves[i];
        Piece promotion = promote_type(move);
        u16 value = piece_value[promotion / 2];
        moves[i] = (move & 0xFFFFFFF) | (u64)value << 28;
    }
}

void value_captures(u64 attack_mask, u64 *moves, int num_moves, int losing) {
    // values by BLIND: lxh or mxm or (hxl only if l is undefended)
    int piece_value[6] = {1, 5, 3, 4, 9, 1000};
    for (int i = 0; i < num_moves; i++) {
        u64 move = moves[i];
        u16 value;

        if (piece_value[piece(move) / 2] <= piece_value[captured(move) / 2]) {
            value = 10 * piece_value[captured(move) / 2] -
                    piece_value[piece(move) / 2];
        } else if (!(BB_SQUARE(to(move)) & attack_mask)) {
            value = 10 * piece_value[captured(move) / 2] -
                    piece_value[piece(move) / 2];
        } else {
            value = 0;
        }

        if (losing && value > 0) {
            value = 0;
        } else if (losing && value == 0) {
            value = 1001 - piece_value[piece(move) / 2];
        }

        moves[i] = (move & 0xFFFFFFF) | (u64)value << 28;
    }
}

u64 archeology = 0, killed = 0, killed1 = 0, killed2 = 0, killed3 = 0, counter = 0;
void value_quiets(ChessBoard *board, u64 *moves, int num_moves,
                  KillerTable *killer_table, u64 *counter_move, u64 prev_move, int ply) {
	u64 kill1 = killer_table[ply].move1, kill2 = killer_table[ply].move2;
	u64 kill3 = 0, kill4 = 0;
	if (ply >= 2) {
		kill3 = killer_table[ply - 2].move1, kill4 = killer_table[ply - 2].move2;
	}
    kill1 &= 0xFFFFFFF;
	kill2 &= 0xFFFFFFF;
	kill3 &= 0xFFFFFFF;
	kill4 &= 0xFFFFFFF;

    // all valued as 0 except move 1 -> 2, move 2 -> 1
    // Should not move piece to attacked square if square is not defended
    for (int i = 0; i < num_moves; i++) {
        u64 move = moves[i] & 0xFFFFFFF;

        if (move == kill1) {
            moves[i] = move | (u64)(2005) << 28;
            killed++; 
		} else if (move == kill2) {
			moves[i] = move | (u64)(2004) << 28;
			killed1++;
		} else if (move == kill3) {
			moves[i] = move | (u64)(2003) << 28;
			killed2++;
		} else if (move == kill4) {
			moves[i] = move | (u64)(2002) << 28;
			killed3++;
        } else if (move == counter_move[from(prev_move) * 64 + to(prev_move)]) {
            moves[i] = move | (u64)(2001) << 28;
            counter++;
        } else {
            int move_val = mg_table[board->side][piece(move) / 2][to(move)] -
                        mg_table[board->side][piece(move) / 2][from(move)];
            move_val += 500;

            moves[i] = move | (u64)move_val << 28;
			archeology++;
        }
    }
}

// attackers is squares attacked by !board->side (enemies)
int generate_moves(ChessBoard *board, u64 *moves, u64 attackers,
                   MoveGenStage stage) {
    switch (stage) {
        case promotions:
            return generate_promotions(board, moves);

        case captures:
            return generate_normal_moves(board, moves, 0);

        case losing:
            return generate_normal_moves(board, moves, 0);

        case castling:
            return generate_castling(board, moves, attackers, 0);

        case quiets:
            return generate_normal_moves(board, moves, 1);
    }
}

void sort_moves(ChessBoard *board, u64 attack_mask, u64 *moves,
                int num_moves, KillerTable *killer_table, u64 *counter_move,
                u64 prev_move, MoveGenStage stage, int ply) {
    switch (stage) {
        case promotions:
            value_promotions(moves, num_moves);
            break;

        case captures:
            value_captures(attack_mask, moves, num_moves, 0);
            break;

        case losing:
            value_captures(attack_mask, moves, num_moves, 1);
            break;

        case castling:
            break;

        case quiets:
            value_quiets(board, moves, num_moves, killer_table, counter_move,
                         prev_move, ply);
            break;
    }
}
