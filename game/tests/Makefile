# Unit Test Makefile for Dystopia MUD
# Run from game/tests/: make && ./run_tests
#
# Prerequisites: game must be built first (make -f Makefile in game/build/)
# This Makefile reuses the game's object files but recompiles comm.c
# with -DTEST_BUILD to exclude the production main().
#
# Self-discovering: game .o files and test .c files are found via wildcard.
# No need to regenerate this file when source files are added/removed.

CC = gcc

# Directories
SRC_DIR  = ../src
BUILD_DIR = ../build
GAME_OBJ = $(BUILD_DIR)/linux/obj
TEST_OBJ = obj

# Include paths (same as game + test directory)
INCLUDES = -I. -I$(SRC_DIR)/core -I$(SRC_DIR)/classes -I$(SRC_DIR)/world -I$(SRC_DIR)/systems -I$(SRC_DIR)/db

# Compiler flags: debug build for tests, plus TEST_BUILD to exclude game main()
C_FLAGS = -Wall -g -O0 -DTEST_BUILD $(INCLUDES)

# Linker flags (same as game)
L_FLAGS = -lz -lcrypt -lpthread -ldl

# --- Game object files (auto-discovered from build output) ---
# All .o files from game build EXCEPT comm.o (which has main()).
# We recompile comm.c with -DTEST_BUILD instead.
GAME_OBJS = $(filter-out %/comm.o, \
	$(wildcard $(GAME_OBJ)/core/*.o) \
	$(wildcard $(GAME_OBJ)/classes/*.o) \
	$(wildcard $(GAME_OBJ)/combat/*.o) \
	$(wildcard $(GAME_OBJ)/commands/*.o) \
	$(wildcard $(GAME_OBJ)/world/*.o) \
	$(wildcard $(GAME_OBJ)/systems/*.o) \
	$(wildcard $(GAME_OBJ)/db/*.o))

# --- Test-specific objects ---
# Recompile comm.c with TEST_BUILD to exclude main()
TEST_COMM_OBJ = $(TEST_OBJ)/comm_test.o

# Auto-discover test source files (test_*.c)
TEST_SRC = $(wildcard test_*.c)
TEST_OBJS = $(addprefix $(TEST_OBJ)/,$(TEST_SRC:.c=.o))

# Target
TARGET = run_tests

all: $(TEST_OBJ) $(TARGET)

$(TEST_OBJ):
	mkdir -p $(TEST_OBJ)

$(TARGET): $(TEST_OBJS) $(TEST_COMM_OBJ) $(GAME_OBJS)
	$(CC) -o $(TARGET) $(TEST_OBJS) $(TEST_COMM_OBJ) $(GAME_OBJS) $(L_FLAGS)

# Recompile comm.c with TEST_BUILD flag
$(TEST_COMM_OBJ): $(SRC_DIR)/core/comm.c $(SRC_DIR)/core/merc.h
	$(CC) -c $(C_FLAGS) $< -o $@

# Test file compilation
$(TEST_OBJ)/%.o: %.c
	$(CC) -c $(C_FLAGS) $< -o $@

clean:
	rm -f $(TEST_OBJS) $(TEST_COMM_OBJ) $(TARGET)
	rm -rf $(TEST_OBJ)

.PHONY: clean all
