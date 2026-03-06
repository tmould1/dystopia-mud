# Build System, CI/CD & Deployment

The build system supports Linux (GCC) and Windows (MSVC) with auto-generated project files, parallel compilation, and automated CI/CD through GitHub Actions.

## Project File Generation

**Location:** [GenerateProjectFiles.bat](../../../build/GenerateProjectFiles.bat) (Windows) / [GenerateProjectFiles.sh](../../../build/GenerateProjectFiles.sh) (Linux)

Both scripts scan all subdirectories under `game/src/` for `.c` and `.h` files and generate build files. Run these whenever source files are added, removed, or renamed.

| Script | Generates | Backs Up |
|--------|-----------|----------|
| `GenerateProjectFiles.bat` | Makefile, dystopia.sln, dystopia.vcxproj, dystopia.vcxproj.filters | Yes, to `game/build/backup/` |
| `GenerateProjectFiles.sh` | Makefile only | Yes, to `game/build/backup/` |

### Scanned Subdirectories

```
SUBDIRS = core classes combat commands world systems db
```

Each subdirectory under `game/src/` is scanned for `.c` and `.h` files. The generated Makefile creates corresponding object file targets with the directory structure preserved.

## Build Scripts

### Linux — build.sh

**Location:** [game/build/build.sh](../../../build/build.sh) (49 lines)

```bash
cd game/build
./build.sh              # Build with parallel compilation
./build.sh clean        # Remove build artifacts
```

Features:
- Auto-detects CPU core count via `nproc`
- Parallel `make -j` compilation
- Timestamped build logs in `game/build/logs/`
- Output binary: `gamedata/dystopia`

### Windows — build.bat

**Location:** [game/build/build.bat](../../../build/build.bat) (71 lines)

```cmd
cmd.exe /c "game\build\build.bat"           # Release build
cmd.exe /c "game\build\build.bat debug"     # Debug build
cmd.exe /c "game\build\build.bat clean"     # Clean
```

Features:
- Auto-discovers MSBuild via `vswhere.exe` (VS 2017+)
- Parallel compilation with `/m:%NUMBER_OF_PROCESSORS%`
- Release config: `/MD /GL /O2` (optimized, whole-program optimization)
- Timestamped build logs in `game\build\logs\`
- Output binary: `gamedata\dystopia.exe`

## Object File Layout

| Platform | Object Directory | Structure |
|----------|-----------------|-----------|
| Linux | `game/build/linux/obj/` | Hierarchical: `core/`, `classes/`, `combat/`, etc. |
| Windows | `game/build/win64/obj/Release/` | Flat: all `.obj` files in one directory |

This difference matters for the test build scripts, which auto-discover game object files from these directories.

## CI/CD Pipelines

Three GitHub Actions workflows in [.github/workflows/](../../../../.github/workflows/):

### ci.yml — Continuous Integration

**Trigger:** Push or PR to `main` (only when `game/src/`, `game/tests/`, or `game/build/` change)

```
ci.yml
├─ build-and-test-linux (ubuntu-latest)
│   ├─ Install: build-essential, zlib1g-dev, libcrypt-dev
│   ├─ GenerateProjectFiles.sh
│   ├─ make -j$(nproc)
│   └─ cd game/tests && make clean && make && ./run_tests
│
└─ build-and-test-windows (windows-latest)
    ├─ Setup MSBuild (microsoft/setup-msbuild@v2)
    ├─ GenerateProjectFiles.bat
    ├─ msbuild dystopia.vcxproj /p:Configuration=Release /p:Platform=x64
    └─ game\tests\build_tests.bat
```

Both platforms must pass. Test failures block the pipeline.

### test.yml — Manual Test Run

**Trigger:** Manual (workflow_dispatch) with platform selection (both/linux/windows)

Same build + test pipeline as CI, but manually triggered for ad-hoc testing.

### release-linux.yml / release-windows.yml — Build & Release

**Trigger:** Push of version tag (`v*`) or manual dispatch with optional tag input

Each platform has its own independent workflow. Both trigger on the same tag and attach their artifact to the same GitHub Release.

```
release-linux.yml                          release-windows.yml
├─ build (amazonlinux:2023)                ├─ build (windows-latest)
│   ├─ GenerateProjectFiles.sh → make      │   ├─ GenerateProjectFiles.bat → MSBuild
│   ├─ Run tests                           │   ├─ Run tests
│   └─ Package → tar.gz                   │   └─ Package → zip
│                                          │
└─ release (on tag only)                   └─ release (on tag only)
    └─ Create/update GitHub Release            └─ Create/update GitHub Release
        └─ dystopia-linux-amd64.tar.gz             └─ dystopia-windows-amd64.zip
```

### Release Archive Contents

Each archive is self-contained — extract and run:

| Component | Contents |
|-----------|----------|
| `gamedata/dystopia` (or `.exe`) | Server binary |
| `gamedata/db/areas/*.db` | World data (area databases) |
| `gamedata/db/game/*.db` | Game data (help, classes, tables) |
| `gamedata/doc/` | License files and documentation |
| `startup.sh` / `startup.bat` | Auto-restart startup script |

Runtime-generated data (`game.db`, `live_help.db`, player saves, logs) is not included — the server creates these on first boot.

## Docker

**Location:** [Dockerfile](../../../../Dockerfile) (46 lines)

```
Base:     debian:latest
Build:    game/build → make -f Makefile
Port:     8888
Volumes:  gamedata/db/{players,game,areas}, gamedata/log, game/src
Entry:    Rebuild from source + startup.sh + tail -f /dev/null
```

The Docker container mounts source code and database volumes, allowing development without a full local build environment. The entrypoint rebuilds from source on container start, then launches the game via `startup.sh`.

## Startup & Copyover

### Linux — startup.sh

**Location:** [startup.sh](../../../../startup.sh) (49 lines)

Auto-restart loop with crash recovery:

```bash
while true; do
    gamedata/dystopia $port
    [ -e gamedata/txt/shutdown.txt ] && exit 0   # Clean shutdown
    sleep 2                                        # Crash recovery delay
done
```

### Windows — startup.bat

**Location:** [startup.bat](../../../../startup.bat)

Same pattern with Windows exit code convention:
- Exit code 99 = copyover (hot restart — parent exits, child continues serving players)
- Shutdown file check for clean exit

### Copyover Mechanism

The MUD supports hot restarts ("copyover") where the server re-execs itself while keeping player connections open. On Linux this uses `execl()`; on Windows it uses `_spawnl(_P_OVERLAY, ...)` via the `compat.h` abstraction.

## Related Documents

- [Testing](testing.md) — Test harness that runs in CI
- [Database](database.md) — SQLite files included in deployment packages
- [Heritage](heritage.md) — Build system as a modernization
