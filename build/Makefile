# Linux/Unix Makefile for Dystopia MUD
# Run from project root: make -f build/Makefile
#
# Parallel compilation: make -j$(nproc) -f build/Makefile

CC = gcc

# Enable parallel-safe output (GNU Make 4.0+)
MAKEFLAGS += --output-sync=target

# Directories
SRC_DIR = src
OBJ_DIR = build/obj

# Compiler and linker flags
C_FLAGS = -Wall -O2
L_FLAGS = -lz -lcrypt -lpthread

# Source files
SOURCES = act_comm.c act_info.c act_move.c act_obj.c act_wiz.c bit.c mccp.c \
          board.c build.c clan.c comm.c daemon.c db.c angel.c socials.c \
          demon.c drow.c fight.c kav_wiz.c handler.c kav_fight.c kav_info.c \
          interp.c mage.c magic.c mem.c monk.c monk2.c ninja.c olc.c upgrade.c \
          olc_act.c olc_save.c save.c special.c string.c arena.c \
          update.c vamp.c ww.c shapeshifter.c tanarri.c undead_knight.c \
          samurai.c wizutil.c const.c spiderdroid.c lich.c jope.c \
          jobo_king.c jobo_data.c jobo_fight.c jobo_act.c jobo_wiz.c jobo_util.c \
          config.c

# Object files (in build/obj directory)
O_FILES = $(addprefix $(OBJ_DIR)/,$(SOURCES:.c=.o))

# Target executable (in project root)
TARGET = dystopia

all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET): $(O_FILES)
	$(CC) -o $(TARGET) $(O_FILES) $(L_FLAGS)
	chmod g+w $(TARGET)

# Pattern rule for .c to .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/merc.h
	$(CC) -c $(C_FLAGS) -I$(SRC_DIR) $< -o $@

clean:
	rm -f $(O_FILES) $(TARGET)
	rm -rf $(OBJ_DIR)

.PHONY: clean all
