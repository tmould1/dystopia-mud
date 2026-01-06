# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dystopia MUD is a classic text-based multiplayer game based on the Diku/Merc MUD codebase. This fork has been modernized to compile with current GCC and adds Windows support.

## Project Structure

```
dystopia-mud/
├── gamedata/           # Runtime data + executable (deployable package)
│   ├── dystopia(.exe)  # Server executable (built here for easy deployment)
│   ├── area/           # Area files (.are), loaded via area.lst
│   ├── doc/            # Documentation and licenses
│   ├── log/            # Server logs
│   ├── notes/          # Player notes
│   ├── player/         # Player save files
│   └── txt/            # Config/text files (motd, help, etc.)
│
├── game/               # Development
│   ├── src/            # Source code (organized by subsystem)
│   ├── lib/            # Libraries (linux/, win64/)
│   ├── build/          # Build system and IDE configs
│   ├── tools/          # Python utilities
│   └── docs/           # Development documentation
│
├── startup.sh          # Server startup script (Linux)
├── startup.bat         # Server startup script (Windows)
└── Dockerfile          # Container definition
```

## Build Commands

All builds use Makefiles in `game/build/`. Object files are placed in `game/build/obj/`, binaries output to `gamedata/` for easy deployment.

### Linux (GCC)
```bash
cd game/build
make -f Makefile clean && make -f Makefile
../../gamedata/dystopia &   # Or use ./startup.sh for auto-restart
```
Requires: `build-essential`, `zlib1g-dev`, `libcrypt-dev` packages (libpthread is included in glibc).

### Windows (MinGW/MSYS2)
**Preferred**: Use `game\build\build.bat` which handles paths and parallel compilation:
```cmd
cd game\build
build.bat clean   # Clean build artifacts
build.bat         # Build with parallel jobs
build.bat install # Build dystopia.exe (fresh install)
```

Or manually:
```bash
cd game/build
make -f Makefile.mingw clean && make -f Makefile.mingw
```
Produces `gamedata/dystopia_new.exe` (or `dystopia.exe` with install target). The build automatically copies `zlib1.dll` from `game/lib/win64/bin/` to `gamedata/`.

### Windows (MSVC)
Open `game/build/dystopia.sln` in Visual Studio, or:
```cmd
cd game\build
msbuild dystopia.vcxproj /p:Configuration=Release
```

## Source Directory Structure

Source files are organized into functional subdirectories under `game/src/`:

```
game/src/
├── core/       - Core engine (merc.h, comm.c, db.c, handler.c, interp.c, etc.)
├── classes/    - Character classes (vamp.c, ww.c, demon.c, angel.c, etc.)
├── combat/     - Combat systems (fight.c, kav_fight.c, magic.c, arena.c)
├── commands/   - Player/admin commands (act_*.c, socials.c, wizutil.c)
├── world/      - OLC/building (olc.c, olc_act.c, olc_save.c, build.c)
└── systems/    - Game subsystems (update.c, save.c, clan.c, board.c, etc.)
```

Object files mirror this structure in `game/build/obj/`.

## Architecture

### Core Files (game/src/core/)
- **merc.h** - Master header defining all structures (CHAR_DATA, OBJ_DATA, ROOM_INDEX_DATA, etc.), constants, and function prototypes. Include this in all source files.
- **compat.h/compat.c** - Windows/POSIX compatibility layer providing threading, crypt, signals, and socket abstractions.
- **comm.c** - Network I/O, game loop, descriptor handling. Data flow: `Game_loop -> Read_from_descriptor -> Read_from_buffer` and `Game_loop -> Process_Output -> Write_to_descriptor`.
- **db.c** - Database loading, area file parsing, path management. Initializes `mud_*_dir` paths based on executable location (executable is in `gamedata/`, data dirs are siblings).
- **interp.c** - Command interpreter and command table (`cmd_table`).
- **handler.c** - Object/character manipulation, affect handling.

### Character Classes (game/src/classes/)
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

### Combat Systems (game/src/combat/)
- **fight.c, kav_fight.c, jobo_fight.c** - Combat system
- **magic.c** - Spell casting
- **arena.c** - PvP arena
- **special.c** - Special mob procedures

### Game Systems (game/src/systems/)
- **update.c** - Game tick updates (regeneration, weather, etc.)
- **save.c** - Player file persistence
- **clan.c** - Clan system
- **board.c** - Message boards
- **mccp.c** - MUD Client Compression Protocol

### World Building (game/src/world/)
- **olc.c, olc_act.c, olc_save.c** - Online creation system for building areas
- **build.c** - Area/room/object/mobile building commands

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
