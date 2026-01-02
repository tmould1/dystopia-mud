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
**Preferred**: Use `build\build.bat` which handles paths and parallel compilation automatically:
```cmd
build\build.bat clean   # Clean build artifacts
build\build.bat         # Build with parallel jobs
build\build.bat install # Build dystopia.exe (fresh install)
```

Or manually:
```bash
make -f build/Makefile.mingw clean && make -f build/Makefile.mingw
```
Produces `dystopia_new.exe` (or `dystopia.exe` with install target). Requires `zlib1.dll` in same directory as executable.

### Windows (MSVC)
```cmd
nmake /f build\Makefile.msvc clean
nmake /f build\Makefile.msvc
```
Requires `zlib.lib` in src folder.

## Source Directory Structure

Source files are organized into functional subdirectories under `src/`:

```
src/
├── core/       - Core engine (merc.h, comm.c, db.c, handler.c, interp.c, etc.)
├── classes/    - Character classes (vamp.c, ww.c, demon.c, angel.c, etc.)
├── combat/     - Combat systems (fight.c, kav_fight.c, magic.c, arena.c)
├── commands/   - Player/admin commands (act_*.c, socials.c, wizutil.c)
├── world/      - OLC/building (olc.c, olc_act.c, olc_save.c, build.c)
└── systems/    - Game subsystems (update.c, save.c, clan.c, board.c, etc.)
```

Object files mirror this structure in `build/obj/`.

## Architecture

### Core Files (src/core/)
- **merc.h** - Master header defining all structures (CHAR_DATA, OBJ_DATA, ROOM_INDEX_DATA, etc.), constants, and function prototypes. Include this in all source files.
- **compat.h/compat.c** - Windows/POSIX compatibility layer providing threading, crypt, signals, and socket abstractions.
- **comm.c** - Network I/O, game loop, descriptor handling. Data flow: `Game_loop -> Read_from_descriptor -> Read_from_buffer` and `Game_loop -> Process_Output -> Write_to_descriptor`.
- **db.c** - Database loading, area file parsing, path management. Initializes `mud_*_dir` paths based on executable location.
- **interp.c** - Command interpreter and command table (`cmd_table`).
- **handler.c** - Object/character manipulation, affect handling.

### Character Classes (src/classes/)
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

### Combat Systems (src/combat/)
- **fight.c, kav_fight.c, jobo_fight.c** - Combat system
- **magic.c** - Spell casting
- **arena.c** - PvP arena
- **special.c** - Special mob procedures

### Game Systems (src/systems/)
- **update.c** - Game tick updates (regeneration, weather, etc.)
- **save.c** - Player file persistence
- **clan.c** - Clan system
- **board.c** - Message boards
- **mccp.c** - MUD Client Compression Protocol

### World Building (src/world/)
- **olc.c, olc_act.c, olc_save.c** - Online creation system for building areas
- **build.c** - Area/room/object/mobile building commands

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
