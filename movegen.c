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
    if (piece == 0) {
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
        p = (move >> 24) & 0x7;
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

int get_piece(Piece p, int wtm) {
    switch (p) {
        case pawn:
            return wtm ? WHITE_PAWN : BLACK_PAWN;
        case rook:
            return wtm ? WHITE_ROOK : BLACK_ROOK;
        case knight:
            return wtm ? WHITE_KNIGHT : BLACK_KNIGHT;
        case bishop:
            return wtm ? WHITE_BISHOP : BLACK_BISHOP;
        case queen:
            return wtm ? WHITE_QUEEN : BLACK_QUEEN;
        case king:
            return wtm ? WHITE_KING : BLACK_KING;
    }
}

U64 get_bb(ChessBoard *board, Piece p, int wtm) {
    switch (p) {
        case pawn:
            return wtm ? board->white_pawns : board->black_pawns;
        case rook:
            return wtm ? board->white_rooks : board->black_rooks;
        case knight:
            return wtm ? board->white_knights : board->black_knights;
        case bishop:
            return wtm ? board->white_bishops : board->black_bishops;
        case queen:
            return wtm ? board->white_queens : board->black_queens;
        case king:
            return wtm ? board->white_king : board->black_king;
    }
}

// Pawn generation
// move_p points to the end of the moves array
// moves is assumed to be large enough to hold all moves (256)
U64 get_pawn_moves(ChessBoard *board, PawnMoveType move_type) {
    U64 pawns, moved_pawns, mask, sq;
    int wtm;

    moved_pawns = 0;
    wtm = board->white_to_move;
    pawns = wtm ? board->white_pawns : board->black_pawns;

    switch (move_type) {
        case SINGLE_PUSH:
            // no promotions
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & ~mask;

            // shift one rank up/down
            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;

            // don't push into pieces
            moved_pawns = moved_pawns & ~board->all_pieces;
            break;

        case DOUBLE_PUSH:
            // only starting rank
            mask = wtm ? RANK_2 : RANK_7;
            moved_pawns = pawns & mask;

            // shift up/down
            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;

            // don't push into pieces
            moved_pawns &= ~board->all_pieces;

            // repeat
            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;
            moved_pawns &= ~board->all_pieces;
            break;

        // Note: left relative to white player
        case CAPTURE_LEFT:
            // no promotions
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & ~mask;

            // Not file A
            moved_pawns = moved_pawns & ~FILE_1;

            // shift
            moved_pawns = wtm ? moved_pawns << 9 : moved_pawns >> 7;

            // mask enemies
            moved_pawns &= wtm ? board->black_pieces : board->white_pieces;
            break;

        // Note: right relative to white player
        case CAPTURE_RIGHT:
            // no promotions
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & ~mask;

            // Not file H
            moved_pawns = moved_pawns & ~FILE_8;

            // shift
            moved_pawns = wtm ? moved_pawns << 7 : moved_pawns >> 9;

            // mask enemies
            moved_pawns &= wtm ? board->black_pieces : board->white_pieces;
            break;

        case PAWN_PROMOTION:
            // promotion rank
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & mask;

            // shift
            moved_pawns = wtm ? moved_pawns << 8 : moved_pawns >> 8;

            // mask pieces
            moved_pawns &= ~(board->all_pieces);
            break;

        case PROMOTION_CAPTURE_LEFT:
            // promotion rank
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & mask;

            // shift left
            moved_pawns = wtm ? moved_pawns << 9 : moved_pawns >> 7;

            // mask enemies
            moved_pawns &= wtm ? board->black_pieces : board->white_pieces;
            break;

        case PROMOTION_CAPTURE_RIGHT:
            // promotion rank
            mask = wtm ? RANK_7 : RANK_2;
            moved_pawns = pawns & mask;

            // shift left
            moved_pawns = wtm ? moved_pawns << 7 : moved_pawns >> 9;

            // mask enemies
            moved_pawns &= wtm ? board->black_pieces : board->white_pieces;
            break;

        case EN_PASSANT_LEFT:
            // ep only
            if (board->ep == -1) {
                moved_pawns = 0;
            } else {
                sq = wtm ? board->ep - 9 : board->ep + 7;
                mask = wtm ? RANK_5 : RANK_4;

                moved_pawns = pawns & (1ULL << sq);
                moved_pawns &= mask;
                moved_pawns = wtm ? moved_pawns << 9 : moved_pawns >> 7;
            }
            break;

        case EN_PASSANT_RIGHT:
            // ep only
            if (board->ep == -1) {
                moved_pawns = 0;
            } else {
                sq = wtm ? board->ep - 7 : board->ep + 9;
                mask = wtm ? RANK_5 : RANK_4;

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
    int ind, to, from, piece, captured;
    int num_moves = 0;
    int wtm = board->white_to_move;

    switch (move_type) {
        case SINGLE_PUSH:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 8 : ind + 8;
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = EMPTY_SQ;
                moves[move_p + num_moves] = from | to << 6 | piece << 12 |
                                            captured << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case DOUBLE_PUSH:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 16 : ind + 16;
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = EMPTY_SQ;
                moves[move_p + num_moves] = from | to << 6 | piece << 12 |
                                            captured << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case CAPTURE_LEFT:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 9 : ind + 7;
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = ChessBoard_piece_at(board, to);
                moves[move_p + num_moves] = from | to << 6 | piece << 12 |
                                            captured << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case CAPTURE_RIGHT:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 7 : ind + 9;
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = ChessBoard_piece_at(board, to);
                moves[move_p + num_moves] = from | to << 6 | piece << 12 |
                                            captured << 16 | NORMAL << 20;
                num_moves++;
                BB_CLEAR(pawn_moves, ind);
            }
            break;

        case PAWN_PROMOTION:
            while ((ind = rightmost_set(pawn_moves)) != -1) {
                to = ind;
                from = wtm ? ind - 8 : ind + 8;
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = EMPTY_SQ;

                int promote[4] = {queen, rook, bishop, knight};
                for (int i = 0; i < 4; i++) {
                    moves[move_p + num_moves] =
                        from | to << 6 | piece << 12 | captured << 16 |
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
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = ChessBoard_piece_at(board, to);

                int promote[4] = {queen, rook, bishop, knight};
                for (int i = 0; i < 4; i++) {
                    moves[move_p + num_moves] =
                        from | to << 6 | piece << 12 | captured << 16 |
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
                piece = wtm ? WHITE_PAWN : BLACK_PAWN;
                captured = ChessBoard_piece_at(board, to);

                int promote[4] = {queen, rook, bishop, knight};
                for (int i = 0; i < 4; i++) {
                    moves[move_p + num_moves] =
                        from | to << 6 | piece << 12 | captured << 16 |
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
            piece = wtm ? WHITE_PAWN : BLACK_PAWN;
            captured = wtm ? BLACK_PAWN : WHITE_PAWN;

            moves[move_p + num_moves] = from | to << 6 | piece << 12 |
                                        captured << 16 | EN_PASSANT << 20;
            num_moves++;
            break;

        case EN_PASSANT_RIGHT:
            if (pawn_moves == 0) break;
            ind = rightmost_set(pawn_moves);
            to = ind;
            from = wtm ? ind - 7 : ind + 9;
            piece = wtm ? WHITE_PAWN : BLACK_PAWN;
            captured = wtm ? BLACK_PAWN : WHITE_PAWN;

            moves[move_p + num_moves] = from | to << 6 | piece << 12 |
                                        captured << 16 | EN_PASSANT << 20;
            num_moves++;
            break;
    }

    return num_moves;
}

// Magic Generation
U64 get_magic_moves_sq(ChessBoard *board, LookupTable *lookup, int sq,
                       Piece piece) {
    U64 pieces, mask, magic, moves, friendlies;
    int ind, shift_amt;

    pieces = board->all_pieces;
    if (piece == rook) {
        mask = lookup->rook_mask[sq];
        magic = lookup->rook_magic[sq];
        shift_amt = 64 - 12;

        ind = ((pieces & mask) * magic) >> shift_amt;
        moves = lookup->rook_move[4096 * sq + ind];
    } else {
        mask = lookup->bishop_mask[sq];
        magic = lookup->bishop_magic[sq];
        shift_amt = 64 - 9;

        ind = ((pieces & mask) * magic) >> shift_amt;
        moves = lookup->bishop_move[512 * sq + ind];
    }

    friendlies =
        board->white_to_move ? board->white_pieces : board->black_pieces;
    moves &= (~friendlies);
    return moves;
}

int extract_magic_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                           U64 magic_moves, int sq, Piece p) {
    int ind, to, piece, captured;
    int num_moves = 0;

    piece = get_piece(p, board->white_to_move);

    while ((ind = rightmost_set(magic_moves)) != -1) {
        // this means the move is ind + 8 -> ind
        to = ind;
        captured = ChessBoard_piece_at(board, ind);
        moves[move_p + num_moves] =
            sq | to << 6 | piece << 12 | captured << 16 | NORMAL << 20;
        num_moves++;
        BB_CLEAR(magic_moves, ind);
    }

    return num_moves;
}

int extract_magic_moves(ChessBoard *board, LookupTable *lookup, U64 *moves,
                        int move_p, Piece p) {
    int sq, num_moves;
    U64 magic_moves, bb;

    num_moves = 0;
    bb = get_bb(board, p, board->white_to_move);

    while ((sq = rightmost_set(bb)) != -1) {
        magic_moves = get_magic_moves_sq(board, lookup, sq, p);
        num_moves += extract_magic_moves_sq(board, moves, move_p + num_moves,
                                            magic_moves, sq, p);

        BB_CLEAR(bb, sq);
    }

    return num_moves;
}

// Queen generation
U64 get_queen_moves_sq(ChessBoard *board, LookupTable *lookup, int sq) {
    return get_magic_moves_sq(board, lookup, sq, rook) |
           get_magic_moves_sq(board, lookup, sq, bishop);
}

int extract_queen_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                           U64 move_bb, int sq) {
    int ind, to, piece, captured;
    int num_moves = 0;

    piece = get_piece(queen, board->white_to_move);

    while ((ind = rightmost_set(move_bb)) != -1) {
        to = ind;
        captured = ChessBoard_piece_at(board, ind);
        moves[move_p + num_moves] =
            sq | to << 6 | piece << 12 | captured << 16 | NORMAL << 20;
        num_moves++;
        BB_CLEAR(move_bb, ind);
    }

    return num_moves;
}

int extract_queen_moves(ChessBoard *board, LookupTable *lookup, U64 *moves,
                        int move_p) {
    int sq, num_moves;
    U64 magic_moves, bb;

    num_moves = 0;
    bb = get_bb(board, queen, board->white_to_move);

    while ((sq = rightmost_set(bb)) != -1) {
        magic_moves = get_queen_moves_sq(board, lookup, sq);
        num_moves += extract_magic_moves_sq(board, moves, move_p + num_moves,
                                            magic_moves, sq, queen);

        BB_CLEAR(bb, sq);
    }

    return num_moves;
}

// King generation
U64 get_king_moves_sq(ChessBoard *board, LookupTable *lookup, int sq) {
    U64 moves, friendlies;

    moves = lookup->king_move[sq];
    friendlies =
        board->white_to_move ? board->white_pieces : board->black_pieces;
    moves &= ~friendlies;

    return moves;
}

int extract_king_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                          U64 move_bb, int sq) {
    int ind, to, piece, captured;
    int num_moves = 0;

    piece = get_piece(king, board->white_to_move);

    while ((ind = rightmost_set(move_bb)) != -1) {
        // this means the move is ind + 8 -> ind
        to = ind;
        captured = ChessBoard_piece_at(board, ind);
        moves[move_p + num_moves] =
            sq | to << 6 | piece << 12 | captured << 16 | NORMAL << 20;
        num_moves++;
        BB_CLEAR(move_bb, ind);
    }

    return num_moves;
}

int extract_king_moves(ChessBoard *board, LookupTable *lookup, U64 *moves,
                       int move_p) {
    int sq, num_moves;
    U64 king_moves, bb;

    num_moves = 0;
    bb = get_bb(board, king, board->white_to_move);

    while ((sq = rightmost_set(bb)) != -1) {
        king_moves = get_king_moves_sq(board, lookup, sq);
        num_moves += extract_king_moves_sq(board, moves, move_p + num_moves,
                                           king_moves, sq);

        BB_CLEAR(bb, sq);
    }

    return num_moves;
}

// Knight generation
U64 get_knight_moves_sq(ChessBoard *board, LookupTable *lookup, int sq) {
    U64 moves, friendlies;

    moves = lookup->knight_move[sq];
    friendlies =
        board->white_to_move ? board->white_pieces : board->black_pieces;
    moves &= ~friendlies;

    return moves;
}

int extract_knight_moves_sq(ChessBoard *board, U64 *moves, int move_p,
                            U64 move_bb, int sq) {
    int ind, to, piece, captured;
    int num_moves = 0;

    piece = get_piece(knight, board->white_to_move);

    while ((ind = rightmost_set(move_bb)) != -1) {
        // this means the move is ind + 8 -> ind
        to = ind;
        captured = ChessBoard_piece_at(board, ind);
        moves[move_p + num_moves] =
            sq | to << 6 | piece << 12 | captured << 16 | NORMAL << 20;
        num_moves++;
        BB_CLEAR(move_bb, ind);
    }

    return num_moves;
}

int extract_knight_moves(ChessBoard *board, LookupTable *lookup, U64 *moves,
                         int move_p) {
    int sq, num_moves;
    U64 knight_moves, bb;

    num_moves = 0;
    bb = get_bb(board, knight, board->white_to_move);

    while ((sq = rightmost_set(bb)) != -1) {
        knight_moves = get_knight_moves_sq(board, lookup, sq);
        num_moves += extract_knight_moves_sq(board, moves, move_p + num_moves,
                                             knight_moves, sq);

        BB_CLEAR(bb, sq);
    }

    return num_moves;
}

// Castling
int gen_castling(ChessBoard *board, U64 *moves, U64 attacked, int move_p) {
    int sq, king_to, piece, num_moves = 0;
    U64 castle_mask;

    if (board->white_to_move) {
        piece = WHITE_KING;
        sq = rightmost_set(board->white_king);

        if (board->KC) {
            king_to = sq - 2;
            castle_mask = BB_SQUARE(sq - 1) | BB_SQUARE(sq - 2);

            if (!(castle_mask & (board->all_pieces | attacked))) {
                moves[move_p + num_moves] = sq | king_to << 6 | piece << 12 |
                                            EMPTY_SQ << 16 | CASTLE_KING << 20;
                num_moves++;
            }
        }

        if (board->QC) {
            king_to = sq + 2;
            castle_mask =
                BB_SQUARE(sq + 1) | BB_SQUARE(sq + 2) | BB_SQUARE(sq + 3);

            if (!(castle_mask & (board->all_pieces | attacked))) {
                moves[move_p + num_moves] = sq | king_to << 6 | piece << 12 |
                                            EMPTY_SQ << 16 | CASTLE_QUEEN << 20;
                num_moves++;
            }
        }
    } else {
        piece = BLACK_KING;
        sq = rightmost_set(board->black_king);

        if (board->kc) {
            king_to = sq - 2;
            castle_mask = BB_SQUARE(sq - 1) | BB_SQUARE(sq - 2);

            if (!(castle_mask & (board->all_pieces | attacked))) {
                moves[move_p + num_moves] = sq | king_to << 6 | piece << 12 |
                                            EMPTY_SQ << 16 | CASTLE_KING << 20;
                num_moves++;
            }
        }

        if (board->qc) {
            king_to = sq + 2;
            castle_mask =
                BB_SQUARE(sq + 1) | BB_SQUARE(sq + 2) | BB_SQUARE(sq + 3);

            if (!(castle_mask & (board->all_pieces | attacked))) {
                moves[move_p + num_moves] = sq | king_to << 6 | piece << 12 |
                                            EMPTY_SQ << 16 | CASTLE_QUEEN << 20;
                num_moves++;
            }
        }
    }

	return num_moves;
}

// Attackers
U64 attackers(ChessBoard *board, LookupTable *lookup) {
	const int side = board->white_to_move ? BLACK : WHITE;
	U64 attack = 0;

	// Pawns
	U64 moved_pawns;
	const U64 pawns = get_bb(board, pawn, side);

	// left
	moved_pawns = pawns & ~FILE_1;
	moved_pawns = side ? moved_pawns << 9 : moved_pawns >> 7;
	attack |= moved_pawns;

	// right
	moved_pawns = pawns & ~FILE_8;
	moved_pawns = side ? moved_pawns << 7 : moved_pawns >> 9;
	attack |= moved_pawns;

	// EP
	if (board->ep != -1) {
		int sq;
		
		// left
		sq = side ? board->ep - 9 : board->ep + 7;
		moved_pawns = pawns & BB_SQUARE(sq);
		moved_pawns = side ? moved_pawns << 9 : moved_pawns >> 7;
		attack |= moved_pawns;

		// right
		sq = side ? board->ep - 7 : board->ep + 9;
		moved_pawns = pawns & BB_SQUARE(sq);
		moved_pawns = side ? moved_pawns << 7 : moved_pawns >> 9;
		attack |= moved_pawns;
	}

	// Other pieces
	Piece pieces[] = {rook, bishop, queen, knight, king};
	int len = sizeof(pieces) / sizeof(pieces[0]);

	for (int i = 0; i < len; i++) {
		Piece p = pieces[i];
		U64 bb = get_bb(board, p, side);
		int ind;

		while ((ind = rightmost_set(bb)) != -1) {
			switch (p) {
				case rook:
					attack |= get_magic_moves_sq(board, lookup, ind, rook);
					break;
				case bishop:
					attack |= get_magic_moves_sq(board, lookup, ind, bishop);
					break;
				case queen:
					attack |= get_magic_moves_sq(board, lookup, ind, queen);
					break;
				case knight:
					attack |= get_knight_moves_sq(board, lookup, ind);
					break;
				case king:
					attack |= get_king_moves_sq(board, lookup, ind);
					break;
				default:
					fprintf(stderr, "Error: Invalid piece [%s(%s):%d]\n",
							__FILE__, __func__, __LINE__);
					exit(1);
			}
			BB_CLEAR(bb, ind);
		}
	}

	return attack;
}
