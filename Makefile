# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -Wshadow -Wfloat-equal \
         -Wundef -Wformat=2 -Wswitch-default \
		 -Wstrict-overflow=5 \
		 -fsanitize=address -fsanitize=undefined \
		 -pedantic -std=c99 -g

CFASTFLAGS = -std=c99 -O3
PROF_FLAGS = -g -pg

# Source files
CHESS_SRC = board.c lookup.c makemove.c movegen.c rng.c hash_table.c eval.c
TEST_SRC = test.c
PERFT_SRC = perft.c

# Trash
TRASH = chess test perft_prof perft_test perft *.dSYM __pycache__ gmon.out

all: chess
	$(CC) $(CFASTFLAGS) -o chess $(CHESS_SRC)
	./chess

test: test
	$(CC) $(CFLAGS) -o test $(TEST_SRC) $(CHESS_SRC)
	./test

perft_prof: $(CHESS_SRC) $(PERFT_SRC)
	$(CC) $(CFASTFLAGS) -o perft_prof $(CHESS_SRC) $(PERFT_SRC) $(PROF_FLAGS)
	./perft_prof

perft_test: $(CHESS_SRC) $(PERFT_SRC)
	$(CC) $(CFLAGS) -o perft_test $(CHESS_SRC) $(PERFT_SRC) -D DEBUG
	./perft_test

perft: $(CHESS_SRC) $(PERFT_SRC)
	$(CC) $(CFASTFLAGS) -o perft $(CHESS_SRC) $(PERFT_SRC)
	./perft

clean:
	rm -rf $(TRASH)