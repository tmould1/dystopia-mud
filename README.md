# Dystopia+ MUD

A modernized fork of the classic Diku/Merc Dystopia MUD, updated for current compilers with full Windows support and modern MUD protocols.

<img src=http://i.imgur.com/hzMbJK6.jpeg>

## Improvements Over Original

### Windows Support
- Full MinGW/MSYS2 and MSVC build support
- Cross-platform compatibility layer (`compat.c`/`compat.h`)
- SHA256 password hashing via Windows BCrypt API
- Windows-compatible hot reboot (copyover)

### Modern MUD Protocols
- **GMCP** - Generic MUD Communication Protocol for JSON-based client data
- **MSSP** - MUD Server Status Protocol for MUD listing integration
- **MXP** - MUD eXtension Protocol with clickable links for items, exits, and players
- **MCCP v2** - Compression with v1 fallback
- **NAWS** - Dynamic terminal width detection

### Code Quality
- Zero compiler warnings (fixed 290+ warnings)
- Buffer safety: `sprintf` → `snprintf` throughout
- Clean builds on GCC 12+ and MinGW

### Project Organization
- Source reorganized into 6 functional subsystems:
  - `core/` - Engine (merc.h, comm.c, db.c, handler.c)
  - `classes/` - Character classes
  - `combat/` - Combat systems
  - `commands/` - Player/admin commands
  - `world/` - OLC/world building
  - `systems/` - Game subsystems
- Clean separation: `game/` (development) vs `gamedata/` (deployable runtime)
- Self-contained deployment package

### Build System
- Auto-generated Makefiles and VS project files via `GenerateProjectFiles.bat/.sh`
- Parallel compilation support
- Visual Studio 2022 solution with proper folder structure
- Docker support

## Quick Start

### Linux

```bash
# Install dependencies
sudo apt install build-essential zlib1g-dev

# Generate build files (first time or after adding source files)
cd game/build
./GenerateProjectFiles.sh

# Build
make -f Makefile

# Run (from project root)
cd ../..
./startup.sh
```

### Windows (MinGW/MSYS2)

```cmd
cd game\build

REM Generate build files (first time or after adding source files)
GenerateProjectFiles.bat

REM Build
build.bat

REM Run (from project root)
cd ..\..
startup.bat
```

### Windows (Visual Studio)

```cmd
cd game\build

REM Generate solution and project files
GenerateProjectFiles.bat

REM Open in Visual Studio
start dystopia.sln

REM After building, run from project root
cd ..\..
startup.bat
```

`GenerateProjectFiles.bat` creates `dystopia.sln`, `dystopia.vcxproj`, and `dystopia.vcxproj.filters` with folder organization matching the `src/` subdirectories.

## Project Structure

```
dystopia-mud/
├── gamedata/           # Runtime data + executable (deployable package)
│   ├── dystopia(.exe)  # Server executable
│   ├── area/           # Area files
│   ├── doc/            # Documentation and licenses
│   ├── player/         # Player save files
│   └── txt/            # Config/text files
│
├── game/               # Development
│   ├── src/            # Source code (organized by subsystem)
│   ├── lib/            # Libraries (linux/, win64/)
│   ├── build/          # Build system and IDE configs
│   └── docs/           # Development documentation
│
├── startup.sh          # Server startup script (Linux)
├── startup.bat         # Server startup script (Windows)
└── Dockerfile          # Container definition
```

## Connecting

Default port: **8888**

Recommended clients: [Mudlet](https://www.mudlet.org/), [TinTin++](https://tintin.mudhalla.net/), or any telnet client.

## License

This is a derivative of Diku MUD and Merc. See `gamedata/doc/` for original license files:
- `license.doc` - Diku license
- `license.txt` - Merc license

Original authors must be credited per license terms.
