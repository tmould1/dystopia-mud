# Compiler and flags
CC       = gcc
C_FLAGS  = -Wall
L_FLAGS  = -lz -lcrypt -lpthread

# Object files
O_FILES  = act_comm.o act_info.o act_move.o act_obj.o act_wiz.o bit.o mccp.o \
           board.o build.o clan.o comm.o daemon.o db.o angel.o socials.o \
           demon.o drow.o fight.o kav_wiz.o handler.o kav_fight.o kav_info.o \
           interp.o mage.o magic.o mem.o monk.o monk2.o ninja.o olc.o upgrade.o \
           olc_act.o olc_save.o save.o special.o string.o arena.o \
           update.o vamp.o ww.o shapeshifter.o tanarri.o undead_knight.o \
           samurai.o wizutil.o const.o spiderdroid.o lich.o jope.o \
           jobo_king.o jobo_data.o jobo_fight.o jobo_act.o jobo_wiz.o jobo_util.o

# Target executable
TARGET   = dystopia

# Default target
all: $(TARGET)

# Link the object files to create the final executable
$(TARGET): $(O_FILES)
	$(CC) -o $(TARGET) $(O_FILES) $(L_FLAGS)
	chmod g+w $(TARGET)
	chmod g+w $(O_FILES)

# Pattern rule to compile .c files into .o files
%.o: %.c merc.h
	$(CC) -c $(C_FLAGS) $< -o $@

# Clean up build files
clean:
	rm -f *.o $(TARGET)

# Phony targets to avoid conflicts with files named 'clean' or 'all'
.PHONY: clean all
