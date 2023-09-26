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

# Target executable names
CHESS_EXEC = chess
PERFT_EXEC = perft
TEST_EXEC = test_program

# Source files
CHESS_SRC = board.c lookup.c makemove.c movegen.c rng.c
TEST_SRC = test.c
PERFT_SRC = perft.c

all: $(CHESS_SRC)
	$(CC) $(CFASTFLAGS) -o $(CHESS_EXEC) $(CHESS_SRC)
	./$(CHESS_EXEC)

test: $(TEST_SRC)
	$(CC) $(CFLAGS) -o $(TEST_EXEC) $(TEST_SRC) $(CHESS_SRC)
	./$(TEST_EXEC)

perft_prof:
	$(CC) $(CFASTFLAGS) -o $(PERFT_EXEC) $(CHESS_SRC) $(PERFT_SRC) $(PROF_FLAGS)
	./$(PERFT_EXEC)

perft: 
	$(CC) $(CFASTFLAGS) -o $(PERFT_EXEC) $(CHESS_SRC) $(PERFT_SRC)
	./$(PERFT_EXEC)

clean:
	rm -rf $(CHESS_EXEC) $(TEST_EXEC) $(PERFT_EXEC) $(TEST_EXEC).dSYM __pycache__ gmon.out
