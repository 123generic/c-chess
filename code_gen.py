file = 'rnbqkbnr'
board = file + 'p' * 8 + '.' * 32 + 'P' * 8 + file.upper()

def isolate(string, val):
    chs = ['1' if ch == val else '0' for ch in string]
    string = ''.join(chs)
    bits = int(string, base=2)
    return bits

def print_init_bitboards():
    pieces = 'prnbqkPRNBQK'
    names = ('white_pawns white_rooks white_knights'
             ' white_bishops white_queens white_king'
             ' black_pawns black_rooks black_knights'
             ' black_bishops black_queens black_king').split()
    for p, n in zip(pieces, names):
        h = hex(isolate(board, p))
        print(f'board->{n} = {h};')

def gen_rank(rank_n):
    empty = '0' * 8
    full = '1' * 8
    string = empty * (8 - rank_n) + full + empty * (rank_n - 1)
    return hex(int(string, base=2))

def gen_file(file_n):
    empty = '0' * 8
    full = '1' * 8
    rank_n = 8 - file_n + 1

    string = empty * (8 - rank_n) + full + empty * (rank_n - 1)
    rows = [string[x:x+8] for x in range(0, 64, 8)]
    transpose = list(zip(*rows))
    string = ''.join(''.join(x) for x in transpose)
    return hex(int(string, base=2))


def gen_rank_macros():
    for i in range(1, 9):
        print(f'#define RANK_{i} {gen_rank(i)}')

def gen_file_macros():
    for i in range(1, 9):
        print(f'#define FILE_{i} {gen_file(i)}')

def bb_to_str(bb):
    b = bin(bb)[2:].zfill(64)
    s = '\n'.join(b[x:x+8] for x in range(0, 64, 8))
    return s