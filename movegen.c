#include "movegen.h"

#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "common.h"
#include "lookup.h"

// Utilities
U64 move_from_uci(ChessBoard *board, char *uci) {
    // uci: rank-file rank-file [promotion]
    // TODO: promotion
    int from, to, piece, captured;
    from = 7 - (uci[0] - 'a') + 8 * (uci[1] - '1');
    to = 7 - (uci[2] - 'a') + 8 * (uci[3] - '1');

    piece = ChessBoard_piece_at(board, from);
    captured = ChessBoard_piece_at(board, to);
    if (piece == empty) {
        fprintf(stderr, "Error: No piece at %s [%s(%s):%d]\n", uci, __FILE__,
                __func__, __LINE__);
        exit(1);
    }
    return from | to << 6 | piece << 12 | captured << 16 | UNKNOWN << 20;
}

// uci must have length 4+1 (or more)
void move_to_uci(U64 move, char *uci) {
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
U64 get_pawn_moves(ChessBoard *board, PawnMoveType move_type) {
    U64 pawns, moved_pawns, mask, sq;
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

int extract_pawn_moves(ChessBoard *board, U64 *moves, int move_p,
                       U64 pawn_moves, PawnMoveType move_type) {
    int ind, to, from, captured;
    int num_moves = 0;
    int wtm = board->side == white;

    switch (move_type) {
        case SINGLE_PUSH:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 8 : ind + 8;
                moves[move_p + num_moves] = from | to << 6 | pawn << 12 |
                                            empty << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case DOUBLE_PUSH:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 16 : ind + 16;
                moves[move_p + num_moves] = from | to << 6 | pawn << 12 |
                                            empty << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case CAPTURE_LEFT:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 9 : ind + 7;
                captured = ChessBoard_piece_at(board, to);
                moves[move_p + num_moves] = from | to << 6 | pawn << 12 |
                                            captured << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case CAPTURE_RIGHT:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 7 : ind + 9;
                captured = ChessBoard_piece_at(board, to);
                moves[move_p + num_moves] = from | to << 6 | pawn << 12 |
                                            captured << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case PAWN_PROMOTION:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 8 : ind + 8;

                int promote[4] = {queen, rook, bishop, knight};
                for (int i = 0; i < 4; i++) {
                    moves[move_p + num_moves] =
                        from | to << 6 | pawn << 12 | empty << 16 |
                        PROMOTION << 20 | promote[i] << 24;
                    num_moves++;
                }
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case PROMOTION_CAPTURE_LEFT:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
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
            while ((ind = rightmost_set(pawn_moves)) != -1) {
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

        case EN_PASSANT_LEFT:
            if (pawn_moves == 0) break;
            ind = rightmost_set(pawn_moves);
            to = ind;
            from = wtm ? ind - 9 : ind + 7;

            moves[move_p + num_moves] = from | to << 6 | pawn << 12 |
                                        pawn << 16 | EN_PASSANT << 20;
            num_moves++;
            break;

        case EN_PASSANT_RIGHT:
            if (pawn_moves == 0) break;
            ind = rightmost_set(pawn_moves);
            to = ind;
            from = wtm ? ind - 7 : ind + 9;

            moves[move_p + num_moves] = from | to << 6 | pawn << 12 |
                                        pawn << 16 | EN_PASSANT << 20;
            num_moves++;
            break;
    }

    return num_moves;
}

// Non-pawn move extraction
U64 get_attacks(ChessBoard *board, LookupTable *lookup, int sq, Piece p) {
    U64 pieces, mask, magic, moves;
    int ind, shift_amt;

    pieces = board->bitboards[all_pieces + all];
    switch (p) {
        case rook:
            mask = lookup->rook_mask[sq];
            magic = lookup->rook_magic[sq];
            shift_amt = 64 - 12;

            ind = ((pieces & mask) * magic) >> shift_amt;
            moves = lookup->rook_move[4096 * sq + ind];
            break;

        case bishop:
            mask = lookup->bishop_mask[sq];
            magic = lookup->bishop_magic[sq];
            shift_amt = 64 - 9;

            ind = ((pieces & mask) * magic) >> shift_amt;
            moves = lookup->bishop_move[512 * sq + ind];
            break;

        case queen:
            moves = get_attacks(board, lookup, sq, rook) |
                    get_attacks(board, lookup, sq, bishop);
            break;

        case knight:
            moves = lookup->knight_move[sq];
            break;

        case king:
            moves = lookup->king_move[sq];
            break;

        default:
            fprintf(stderr, "Error: Invalid piece [%s(%s):%d]\n", __FILE__,
                    __func__, __LINE__);
            exit(1);
    }

    return moves;
}

U64 get_moves(ChessBoard *board, LookupTable *lookup, int sq, Piece p) {
    U64 friendlies = board->bitboards[all_pieces + board->side];
    return get_attacks(board, lookup, sq, p) & ~friendlies;
}

int extract_moves(ChessBoard *board, U64 *moves, int move_p, U64 move_bb,
                  int sq, Piece p, int quiet) {
    int ind, to, captured;
    int num_moves = 0;

    while ((ind = rightmost_set(move_bb)) != -1) {
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
int extract_all_moves(ChessBoard *board, LookupTable *table, U64 *moves,
                      int move_p, Piece p) {
    int sq, num_moves;
    U64 bb, move_bb, enemies;

    num_moves = 0;
	bb = board->bitboards[board->side + p];
	enemies = board->bitboards[all_pieces + !board->side];

    while ((sq = rightmost_set(bb)) != -1) {
        move_bb = get_moves(board, table, sq, p);

        num_moves += extract_moves(board, moves, move_p + num_moves,
                                   move_bb & enemies, sq, p, 0);
        num_moves += extract_moves(board, moves, move_p + num_moves,
                                   move_bb & ~enemies, sq, p, 1);

        BB_CLEAR(bb, sq);
    }

    return num_moves;
}

// Castling
int generate_castling(ChessBoard *board, U64 *moves, U64 attacked, int move_p) {
    int sq, king_to, num_moves = 0;
    U64 castle_mask, check_mask;

    if (board->side == white) {
        sq = rightmost_set(board->bitboards[white + king]);

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
        sq = rightmost_set(board->bitboards[black + king]);

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
U64 attackers(ChessBoard *board, LookupTable *lookup, Side side) {
    U64 attack = 0;

    // Pawns
    U64 moved_pawns;
	const U64 pawns = board->bitboards[side + pawn];

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
    Piece pieces[] = {rook, bishop, queen, knight, king};
    int len = sizeof(pieces) / sizeof(pieces[0]);

    for (int i = 0; i < len; i++) {
        Piece p = pieces[i];
		U64 bb = board->bitboards[side + p];
        int ind;

        while ((ind = rightmost_set(bb)) != -1) {
            attack |= get_attacks(board, lookup, ind, p);
            BB_CLEAR(bb, ind);
        }
    }

    return attack;
}

int is_legal(ChessBoard *board, U64 attacked, Side side) {
    U64 king_bb = board->bitboards[side + king];
    U64 king_attackers = attacked & king_bb;

    return king_attackers == 0;
}

// Move generation
int generate_promotions(ChessBoard *board, U64 *moves) {
    U64 pawn_moves;
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

int generate_normal_moves_pawn(ChessBoard *board, U64 *moves, int quiet) {
    U64 pawn_moves;
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

int generate_normal_moves(ChessBoard *board, LookupTable *table, U64 *moves,
                          int quiet) {
    int num_moves = 0;

    // Pawns
    num_moves += generate_normal_moves_pawn(board, moves, quiet);

    // Others
    U64 enemies = board->bitboards[all_pieces + !board->side];
    Piece pieces[] = {rook, bishop, queen, knight, king};
    int len = sizeof(pieces) / sizeof(pieces[0]);

    // iterate through piece types
    for (int i = 0; i < len; i++) {
        Piece p = pieces[i];
		U64 piece_bb = board->bitboards[board->side + p];

        // iterate through piece locations)
        while (piece_bb) {
            int sq = rightmost_set(piece_bb);
            U64 move_bb = get_moves(board, table, sq, p);
            move_bb &= (quiet ? ~enemies : enemies);  // **masking**

            num_moves +=
                extract_moves(board, moves, num_moves, move_bb, sq, p, 0);

            BB_CLEAR(piece_bb, sq);
        }
    }

    return num_moves;
}

int generate_moves(ChessBoard *board, LookupTable *table, U64 *moves,
                   U64 attackers, MoveGenStage stage) {
    switch (stage) {
        case promotions:
            return generate_promotions(board, moves);

        case captures:
        case losing:
            return generate_normal_moves(board, table, moves, 0);

        case castling:
            return generate_castling(board, moves, attackers, 0);

        case quiets:
            return generate_normal_moves(board, table, moves, 1);
    }
}
