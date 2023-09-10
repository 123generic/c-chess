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

// Helper Macros
#define BB_SQUARE(square) (1ULL << (square))
#define BB_SET(bb, square) (bb |= BB_SQUARE(square))
#define BB_GET(bb, square) (bb & BB_SQUARE(square))
#define BB_CLEAR(bb, square) (bb &= ~BB_SQUARE(square))

// Board get macros
#define BOARD_GET(side, piece) (board->side##_##piece)

// Coordinate system interchange
// TODO

// Bitboard Helpers
int rightmost_set(U64 bb);
void print_bb(U64 bb);
U64 make_bitboard(char *str);

// Bitboard
typedef struct {
    // BitBoards
    U64 white_pawns;
    U64 white_rooks;
    U64 white_knights;
    U64 white_bishops;
    U64 white_queens;
    U64 white_king;

    U64 black_pawns;
    U64 black_rooks;
    U64 black_knights;
    U64 black_bishops;
    U64 black_queens;
    U64 black_king;

    U64 white_pieces;
    U64 black_pieces;
    U64 all_pieces;

    // Board State
    int white_to_move;

    // Castling
    int KC, QC, kc, qc;

    // EP
    // bitboard location
    int ep;  // square value 0-63, else -1

    // Halfmove Clock
    int halfmove_clock;

    // Fullmove Number
    int fullmove_number;
} ChessBoard;

void init_ChessBoard(ChessBoard *board);
void ChessBoard_from_FEN(ChessBoard *board, char *fen);
void ChessBoard_str(ChessBoard *board, char *str);
void ChessBoard_to_FEN(ChessBoard *board, char *str);
int ChessBoard_piece_at(ChessBoard *board, int ind);

// Move generation
// U64 single_pawn_push_bb(ChessBoard* board, int side);
// U64 extract_single_pawn_push_
int single_pawn_pushes(ChessBoard *board, U64 *moves, int move_p);
U64 move_from_uci(ChessBoard *board, char *uci);

// Magics
typedef struct {
    U64 magic[64];
    U64 occupancy_mask[64];
    U64 move[64 * 4096];  // index as 4096 * square + magic_ind
} MagicTable;  // TODO
extern MagicTable rook_magic;
extern MagicTable bishop_magic;

// Rooks
void gen_occupancy_rook(MagicTable *magic_table);
U64 rook_moves(ChessBoard *board, MagicTable* magic_table, int square);
U64 manual_gen_rook_moves(U64 bb, int square);
U64 find_magic_rook(U64 *occupancy_mask_table, int sq);
void fill_rook_moves(U64 *move, U64 occupancy_mask, U64 magic, int sq);
void init_magics_rook(MagicTable *magic_table);

// TODO
// void gen_occupancy_bishop(MagicTable *magic_table);
