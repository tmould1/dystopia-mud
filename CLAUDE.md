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
│   ├── player/         # Player save files
│   └── run/            # Runtime files (copyover, crash, shutdown)
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

### Regenerating Build Files

When source files are added, removed, or renamed, regenerate the build files using the appropriate script:

**Windows** (`GenerateProjectFiles.bat`):
```cmd
Find absolute path to GenerateProjectFiles.bat
use 'cmd.exe /c ' with that path to invoke
```
Generates all 4 build files: `Makefile`, `dystopia.sln`, `dystopia.vcxproj`, and `dystopia.vcxproj.filters`. Backs up existing files to `game/build/backup/`.

**Linux** (`GenerateProjectFiles.sh`):
```bash
cd game
./build/GenerateProjectFiles.sh
```
Generates `Makefile` only (run from `game/` directory). For Visual Studio files, use the `.bat` on Windows. Backs up existing files to `game/build/backup/`.

Both scripts scan all subdirectories under `game/src/` (core, classes, combat, commands, world, systems, db) for `.c` and `.h` files.

### Linux (GCC)
**Preferred**: Use `game/build/build.sh` which handles parallel compilation and logging:
```bash
cd game/build
./build.sh              # Build with parallel compilation
./build.sh clean        # Remove build artifacts
```

Or manually:
```bash
cd game/build
make -f Makefile clean && make -f Makefile
../../gamedata/dystopia &   # Or use ./startup.sh for auto-restart
```
Requires: `build-essential`, `zlib1g-dev`, `libcrypt-dev` packages (libpthread is included in glibc).

### Windows (MSVC)
**Preferred**: Use `game\build\build.bat` which handles MSBuild discovery, parallel compilation, and logging:
```cmd
cmd.exe /c "d:\Github\Dystopia\dystopia-mud\game\build\build.bat"
```

Or open `game/build/dystopia.sln` in Visual Studio, or:
```cmd
cd game\build
msbuild dystopia.vcxproj /p:Configuration=Release /p:Platform=x64
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
├── systems/    - Game subsystems (update.c, save.c, board.c, etc.)
└── db/         - SQLite database layer (db_sql.h, sqlite3 amalgamation)
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
- `clan.c` - Vampire clan system

### Combat Systems (game/src/combat/)
- **fight.c, kav_fight.c, jobo_fight.c** - Combat system
- **magic.c** - Spell casting
- **arena.c** - PvP arena
- **special.c** - Special mob procedures

### Game Systems (game/src/systems/)
- **update.c** - Game tick updates (regeneration, weather, etc.)
- **save.c** - Player file persistence
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

## Python Tools (game/tools/)

### Constant Sourcing Architecture

Game constants flow from C headers to Python at import time — no manual synchronization:

```
C headers (merc.h, class.h, db_class.h)
    │
    ├── merc_constants.py (parses #define and enum at import time)
    │       │
    │       └── models.py (single import point for all consumers)
    │               │
    │               ├── mudedit/panels/*.py
    │               ├── mudedit/viz/layout.py
    │               └── mudlib/area_*.py
    │
    └── class.db (SQLite) ──► repository.py:load_class_names()
                                    └── mudedit/panels/class_*.py
```

- **`mudlib/merc_constants.py`** — Parses `merc.h`, `class.h`, and `db_class.h` at import time. Exports raw `{value: 'name'}` dicts for flags, types, enums. Handles letter-macro expansion (`#define ACT_IS_NPC (A)`), overlapping `ITEM_` prefix disambiguation, and stray define filtering.
- **`mudlib/models.py`** — The **single import point** for all game constants. Re-exports simple dicts from `merc_constants.py` and builds enriched `{value: ('ENUM_NAME', 'Display Name')}` tuple formats for editor UI. Also contains area data classes (`Room`, `Mobile`, `Object`, etc.).
- **`mudedit/db/repository.py`** — DB-backed data access. `load_class_names()` queries `class_registry` table. All other game constants come from `models.py`, not from repository.

When adding new `#define` constants to C headers, they automatically appear in the Python tools on next editor launch. Display name overrides can be added to `models.py` for human-friendly labels.
