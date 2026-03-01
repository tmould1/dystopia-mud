# Test Infrastructure

The test harness provides unit testing for the MUD engine without requiring network connections or a running game server. Tests recompile key source files with a `TEST_BUILD` guard to exclude the production `main()`, then link against the game's object files.

## Framework Overview

All test files are in [game/tests/](../../../tests/):

| File | Purpose |
|------|---------|
| [test_framework.h](../../../tests/test_framework.h) | Assertion macros and test runner (111 lines) |
| [test_main.c](../../../tests/test_main.c) | Entry point, global counters, suite dispatch |
| [test_helpers.h](../../../tests/test_helpers.h) / [test_helpers.c](../../../tests/test_helpers.c) | Factory functions for test objects |
| [test_dice.c](../../../tests/test_dice.c) | RNG, dice rolls, interpolation |
| [test_strings.c](../../../tests/test_strings.c) | String comparison and manipulation |
| [test_stats.c](../../../tests/test_stats.c) | Character stat calculation |
| [Makefile](../../../tests/Makefile) | Linux build (auto-discovers game .o and test_*.c files) |
| [build_tests.bat](../../../tests/build_tests.bat) | Windows build (MSVC, matches Release CRT flags) |

## Assertion Macros

**Location:** [test_framework.h:24-89](../../../tests/test_framework.h#L24-L89)

| Macro | Purpose |
|-------|---------|
| `TEST_ASSERT(cond)` | Assert condition is true |
| `TEST_ASSERT_EQ(a, b)` | Assert integer equality (prints both values on failure) |
| `TEST_ASSERT_NEQ(a, b)` | Assert integer inequality |
| `TEST_ASSERT_TRUE(cond)` | Alias for `TEST_ASSERT` |
| `TEST_ASSERT_FALSE(cond)` | Assert condition is false |
| `TEST_ASSERT_STR_EQ(a, b)` | Assert string equality (handles NULL) |
| `TEST_ASSERT_RANGE(val, lo, hi)` | Assert value is in `[lo, hi]` inclusive |

All macros increment global counters and print `FAIL: file:line: message` on failure.

## Test Runner Macros

**Location:** [test_framework.h:93-106](../../../tests/test_framework.h#L93-L106)

| Macro | Purpose |
|-------|---------|
| `RUN_TEST(fn)` | Run a test function, print PASS/FAIL with failure count |
| `RUN_SUITE(name, fn)` | Run a test suite with a header banner |

## Global Counters

**Defined in:** [test_main.c:13-16](../../../tests/test_main.c#L13-L16)
**Declared extern in:** [test_framework.h:17-19](../../../tests/test_framework.h#L17-L19)

```c
int test_passes;          // Incremented by each passing assertion
int test_failures;        // Incremented by each failing assertion
int test_total;           // Total assertions executed
const char *current_test_name;  // Name of currently running test
```

`test_summary()` prints final results and returns exit code 0 (all pass) or 1 (any failures).

## Test Helpers

**Location:** [test_helpers.h](../../../tests/test_helpers.h) / [test_helpers.c](../../../tests/test_helpers.c)

| Function | Purpose |
|----------|---------|
| `make_test_player()` | Create minimal `CHAR_DATA` with `PC_DATA` (calloc'd, all zeros/NULL) |
| `make_test_npc()` | Create minimal NPC (`ACT_IS_NPC` set, no pcdata) |
| `free_test_char(ch)` | Free a test character and its pcdata |
| `seed_rng(seed)` | Set `current_time` to seed value and call `init_mm()` for deterministic tests |

## TEST_BUILD Guard

**Location:** [comm.c:247](../../../src/core/comm.c#L247), [comm.c:350](../../../src/core/comm.c#L350)

The production `main()` in `comm.c` is wrapped in `#ifndef TEST_BUILD`. When compiling with `-DTEST_BUILD`, the game's `main()` is excluded, allowing the test runner's `main()` to link instead.

The test build recompiles `comm.c` with `-DTEST_BUILD` to produce a separate `comm_test.o`, while all other game object files are reused as-is.

## Key Test APIs

Three APIs enable testing game logic without network connections:

### boot_headless()

**Location:** [comm.c:312](../../../src/core/comm.c#L312)

Initializes the game engine without opening network sockets. Calls `boot_db()` to load areas, helps, classes, and all game data. Used for Tier 2+ tests that need the full game world.

```c
void boot_headless( const char *exe_path );
```

### game_tick()

**Location:** [comm.c:672](../../../src/core/comm.c#L672)

Executes one pulse of autonomous game updates (violence, mobile AI, weather, etc.) without requiring the network game loop. Allows tests to advance game state deterministically.

```c
void game_tick( void );
```

### test_output_start/get/clear/stop()

**Location:** [output.c:29-57](../../../src/core/output.c#L29-L57)

Captures raw character output before color processing. When active, `stc()` diverts text to a growable buffer instead of the network.

```c
void test_output_start( CHAR_DATA *ch );  // Begin capture for ch
const char *test_output_get( void );       // Get captured text
void test_output_clear( void );            // Clear buffer
void test_output_stop( void );             // Stop capture, free buffer
```

## Test Tiers

| Tier | Boot Required | APIs Used | What It Tests |
|------|--------------|-----------|---------------|
| **Tier 1** | None | Assertion macros, `seed_rng()`, `make_test_player()` | Pure functions: RNG, string ops, stat calculation |
| **Tier 2** | `boot_headless()` | All above + `game_tick()`, `test_output_*()` | Game logic: commands, combat, output formatting |

Current test suites are Tier 1. Tier 2 tests can use `boot_headless()` to load the full game world and then exercise commands, captures output, and verify behavior.

## Build Process

### Linux

```bash
cd game/tests
make clean && make
./run_tests
```

The [Makefile](../../../tests/Makefile) auto-discovers game `.o` files from `game/build/linux/obj/{core,classes,...}/` and test files matching `test_*.c`. No manual file list maintenance needed.

Key details:
- Excludes `comm.o` from game objects (has production `main()`)
- Recompiles `comm.c` with `-DTEST_BUILD` → `comm_test.o`
- Flags: `-Wall -g -O0 -DTEST_BUILD` (debug build for tests)
- Links: `-lz -lcrypt -lpthread -ldl`

### Windows

```cmd
game\tests\build_tests.bat
```

Uses MSVC with Release-matching CRT flags (`/MD /GL /O2`) to ensure compatibility with game object files. Self-discovering — finds game `.obj` files in `game/build/win64/obj/Release/`.

## Adding a New Test

1. Create `game/tests/test_<topic>.c`
2. Include `test_framework.h` and `test_helpers.h`
3. Write test functions using assertion macros
4. Create a suite function that calls `RUN_TEST()` for each test
5. Add `extern void suite_<topic>(void);` to `test_main.c`
6. Add `RUN_SUITE("Topic Name", suite_<topic>);` to `main()`
7. Build and run — the Makefile/bat auto-discovers the new `.c` file

Example:

```c
// test_example.c
#include "test_framework.h"
#include "test_helpers.h"

void test_something( void ) {
    TEST_ASSERT_EQ( 2 + 2, 4 );
}

void suite_example( void ) {
    RUN_TEST( test_something );
}
```

## CI Integration

Tests run automatically on every push/PR via GitHub Actions:

- **Linux:** `make clean && make && ./run_tests` in [ci.yml](../../../../.github/workflows/ci.yml)
- **Windows:** `build_tests.bat` (builds and runs) in [ci.yml](../../../../.github/workflows/ci.yml)

Both platforms must pass for CI to succeed. See [Build System](build-system.md) for full CI/CD details.

## Related Documents

- [Build System](build-system.md) — CI/CD pipeline and build scripts
- [Platform & Config](platform-config.md) — Test output capture details
- [Heritage](heritage.md) — Test harness as a modernization
