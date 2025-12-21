# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dystopia MUD is a classic text-based multiplayer game based on the Diku/Merc MUD codebase. This fork has been modernized to compile with current GCC and adds Windows support.

## Build Commands

All builds use Makefiles in the `build/` directory. Object files are placed in `build/obj/`, keeping `src/` clean. Run from the project root.

### Linux (GCC)
```bash
make -f build/Makefile clean && make -f build/Makefile
./dystopia &   # Or use ./startup.sh for auto-restart on port 8888
```
Requires: `build-essential` package (gcc, make) plus `libz-dev`, `libcrypt-dev`, `libpthread` libraries.

### Windows (MinGW/MSYS2)
```bash
make -f build/Makefile.mingw clean && make -f build/Makefile.mingw
```
Produces `dystopia.exe` in project root. Requires `zlib1.dll` in same directory as executable.

### Windows (MSVC)
```cmd
nmake /f build\Makefile.msvc clean
nmake /f build\Makefile.msvc
```
Requires `zlib.lib` in src folder.

## Architecture

### Core Files
- **merc.h** - Master header defining all structures (CHAR_DATA, OBJ_DATA, ROOM_INDEX_DATA, etc.), constants, and function prototypes. Include this in all source files.
- **compat.h/compat.c** - Windows/POSIX compatibility layer providing threading, crypt, signals, and socket abstractions.
- **comm.c** - Network I/O, game loop, descriptor handling. Data flow: `Game_loop -> Read_from_descriptor -> Read_from_buffer` and `Game_loop -> Process_Output -> Write_to_descriptor`.
- **db.c** - Database loading, area file parsing, path management. Initializes `mud_*_dir` paths based on executable location.
- **interp.c** - Command interpreter and command table (`cmd_table`).
- **handler.c** - Object/character manipulation, affect handling.

### Character Classes (Races)
Each class has its own source file with powers and abilities:
- `vamp.c` - Vampires
- `ww.c` - Werewolves (uses `garou.h`)
- `demon.c` - Demons
- `angel.c` - Angels
- `mage.c` - Mages
- `ninja.c` - Ninjas
- `monk.c`, `monk2.c` - Monks
- `samurai.c` - Samurai
- `drow.c` - Drow
- `lich.c` - Liches
- `shapeshifter.c` - Shapeshifters
- `tanarri.c` - Tanarri
- `undead_knight.c` - Undead Knights
- `spiderdroid.c` - Spider Droids

### Game Systems
- **fight.c, kav_fight.c, jobo_fight.c** - Combat system
- **update.c** - Game tick updates (regeneration, weather, etc.)
- **save.c** - Player file persistence
- **olc.c, olc_act.c, olc_save.c** - Online creation system for building areas
- **build.c** - Area/room/object/mobile building commands
- **clan.c** - Clan system
- **arena.c** - PvP arena

### Data Directories
- `area/` - Area files (`.are`), loaded via `area/area.lst`
- `player/` - Player save files
- `txt/` - Text files (motd, help, etc.)
- `log/` - Server logs

## Code Conventions

- All source files include `merc.h` first
- Use `snprintf` instead of `sprintf` for buffer safety
- String comparisons: use `!strcmp()` or `str_cmp()`, not `==`
- Check first character of strings: `arg[0] == '\0'`, not `arg == '\0'`
- Windows builds define `WIN32` and use `compat.h` abstractions

## Key Data Structures (in merc.h)
- `CHAR_DATA` - Characters (players and NPCs)
- `OBJ_DATA` - Objects/items
- `ROOM_INDEX_DATA` - Rooms
- `DESCRIPTOR_DATA` - Network connections
- `AREA_DATA` - Areas/zones
- `AFFECT_DATA` - Spell/status effects
