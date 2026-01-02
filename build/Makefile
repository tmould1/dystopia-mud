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

# Subdirectories
CORE_DIR     = $(SRC_DIR)/core
CLASSES_DIR  = $(SRC_DIR)/classes
COMBAT_DIR   = $(SRC_DIR)/combat
COMMANDS_DIR = $(SRC_DIR)/commands
WORLD_DIR    = $(SRC_DIR)/world
SYSTEMS_DIR  = $(SRC_DIR)/systems

# Include paths (so #include "merc.h" works from any source file)
INCLUDES = -I$(CORE_DIR) -I$(CLASSES_DIR) -I$(WORLD_DIR) -I$(SYSTEMS_DIR)

# Compiler and linker flags
C_FLAGS = -Wall -O2 $(INCLUDES)
L_FLAGS = -lz -lcrypt -lpthread

# Source files by directory
CORE_SRC = comm.c db.c mem.c handler.c interp.c config.c const.c string.c daemon.c
CLASSES_SRC = vamp.c ww.c demon.c angel.c mage.c ninja.c monk.c monk2.c samurai.c \
              drow.c lich.c shapeshifter.c tanarri.c undead_knight.c spiderdroid.c
COMBAT_SRC = fight.c kav_fight.c jobo_fight.c magic.c arena.c special.c
COMMANDS_SRC = act_comm.c act_info.c act_move.c act_obj.c act_map.c socials.c \
               act_wiz.c kav_wiz.c jobo_wiz.c kav_info.c wizutil.c
WORLD_SRC = olc.c olc_act.c olc_save.c build.c
SYSTEMS_SRC = update.c save.c clan.c board.c bit.c upgrade.c mccp.c \
              jobo_king.c jobo_data.c jobo_util.c jobo_act.c jope.c

# Object files with subdirectory paths
CORE_OBJ     = $(addprefix $(OBJ_DIR)/core/,$(CORE_SRC:.c=.o))
CLASSES_OBJ  = $(addprefix $(OBJ_DIR)/classes/,$(CLASSES_SRC:.c=.o))
COMBAT_OBJ   = $(addprefix $(OBJ_DIR)/combat/,$(COMBAT_SRC:.c=.o))
COMMANDS_OBJ = $(addprefix $(OBJ_DIR)/commands/,$(COMMANDS_SRC:.c=.o))
WORLD_OBJ    = $(addprefix $(OBJ_DIR)/world/,$(WORLD_SRC:.c=.o))
SYSTEMS_OBJ  = $(addprefix $(OBJ_DIR)/systems/,$(SYSTEMS_SRC:.c=.o))

O_FILES = $(CORE_OBJ) $(CLASSES_OBJ) $(COMBAT_OBJ) $(COMMANDS_OBJ) $(WORLD_OBJ) $(SYSTEMS_OBJ)

# Object directory structure
OBJ_DIRS = $(OBJ_DIR)/core $(OBJ_DIR)/classes $(OBJ_DIR)/combat \
           $(OBJ_DIR)/commands $(OBJ_DIR)/world $(OBJ_DIR)/systems

# Target executable (in project root)
TARGET = dystopia

all: $(OBJ_DIRS) $(TARGET)

# Create all object subdirectories
$(OBJ_DIRS):
	mkdir -p $@

$(TARGET): $(O_FILES)
	$(CC) -o $(TARGET) $(O_FILES) $(L_FLAGS)
	chmod g+w $(TARGET)

# Pattern rules for each source directory
$(OBJ_DIR)/core/%.o: $(CORE_DIR)/%.c $(CORE_DIR)/merc.h
	$(CC) -c $(C_FLAGS) $< -o $@

$(OBJ_DIR)/classes/%.o: $(CLASSES_DIR)/%.c $(CORE_DIR)/merc.h
	$(CC) -c $(C_FLAGS) $< -o $@

$(OBJ_DIR)/combat/%.o: $(COMBAT_DIR)/%.c $(CORE_DIR)/merc.h
	$(CC) -c $(C_FLAGS) $< -o $@

$(OBJ_DIR)/commands/%.o: $(COMMANDS_DIR)/%.c $(CORE_DIR)/merc.h
	$(CC) -c $(C_FLAGS) $< -o $@

$(OBJ_DIR)/world/%.o: $(WORLD_DIR)/%.c $(CORE_DIR)/merc.h $(WORLD_DIR)/olc.h
	$(CC) -c $(C_FLAGS) $< -o $@

$(OBJ_DIR)/systems/%.o: $(SYSTEMS_DIR)/%.c $(CORE_DIR)/merc.h
	$(CC) -c $(C_FLAGS) $< -o $@

clean:
	rm -f $(O_FILES) $(TARGET)
	rm -rf $(OBJ_DIR)

.PHONY: clean all
