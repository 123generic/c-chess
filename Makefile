# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -Wshadow -Wfloat-equal \
         -Wundef -Wformat=2 -Wswitch-default \
		 -Wstrict-overflow=5 \
		 -fsanitize=address -fsanitize=undefined \
		 -pedantic -std=c99 -g

# Target executable names
CHESS_EXEC = chess
TEST_EXEC = test_program

# Source files
CHESS_SRC = board.c lookup.c makemove.c movegen.c rng.c
TEST_SRC = test.c

all: $(CHESS_SRC)
	$(CC) $(CFLAGS) -o $(CHESS_EXEC) $(CHESS_SRC)
	./$(CHESS_EXEC)

test: $(TEST_SRC)
	$(CC) $(CFLAGS) -o $(TEST_EXEC) $(TEST_SRC) $(CHESS_SRC)
	./$(TEST_EXEC)

# magic_debug: magic.c
# 	$(CC) $(CFLAGS) -o $(TEST_EXEC) magic.c $(CHESS_SRC)
# 	./$(TEST_EXEC)

clean:
	rm -rf $(CHESS_EXEC) $(TEST_EXEC) $(TEST_EXEC).dSYM __pycache__ fens.txt moves.txt resultant.txt
