# Platform Compatibility & Configuration

## Cross-Platform Abstraction Layer

The codebase targets both Linux (GCC) and Windows (MSVC). Platform differences are handled by a compatibility layer in two files:

**Location:** [compat.h](../../../src/core/compat.h) (179 lines) / [compat.c](../../../src/core/compat.c) (353 lines)

On Linux, most abstractions are no-ops or direct includes. On Windows (`#ifdef WIN32`), each POSIX facility is mapped to a Windows equivalent:

### Abstraction Table

| Facility | POSIX (Linux) | Windows Mapping | compat.h Section |
|----------|--------------|-----------------|------------------|
| **Threading** | `pthread_create()` | `_beginthreadex()` wrapper | Section 3 |
| **Mutex** | `pthread_mutex_lock()` | `EnterCriticalSection()` | Section 3 |
| **Condition vars** | `pthread_cond_wait()` | `SleepConditionVariableCS()` | Section 3 |
| **Password hash** | `crypt()` | SHA256 via Windows CNG (`bcrypt.h`) | Section 4 |
| **Sockets** | `write()` | `send()` via `socket_write()` | Section 7 |
| **DNS lookup** | `gethostbyaddr_r()` | `getnameinfo()` wrapper | Section 7 |
| **Time** | `gettimeofday()` | `QueryPerformanceCounter()` | Section 8 |
| **Process** | `fork()`, `getpid()` | `-1` stub / `GetCurrentProcessId()` | Section 5 |
| **Signals** | `signal()`, SIGPIPE | Stubs + SEH crash handler | Section 5 |
| **Exec** | `execl()` | `_spawnl(_P_OVERLAY, ...)` | Section 9 |
| **File ops** | `unlink()` | `_unlink()` | Section 6 |
| **Directories** | `mkdir()` | `_mkdir()` via `ensure_directory()` | Section 6 |

### Threading Implementation

Windows threading wraps `_beginthreadex()` with a trampoline to match the POSIX `void *(*)(void *)` function signature:

```c
// compat.c:100-113 — Thread trampoline
typedef struct { void *(*start_routine)(void *); void *arg; } thread_wrapper_t;

static unsigned __stdcall thread_start_wrapper(void *param) {
    thread_wrapper_t *wrapper = (thread_wrapper_t *)param;
    wrapper->start_routine(wrapper->arg);
    free(wrapper);
    return 0;
}
```

Detached threads (`PTHREAD_CREATE_DETACHED`) close their handle immediately, matching POSIX detach behavior.

### Password Hashing

**Location:** [compat.c:24-93](../../../src/core/compat.c#L24-L93)

Linux uses standard `crypt()` from `<crypt.h>`. Windows uses the CNG API (`BCryptOpenAlgorithmProvider` with `BCRYPT_SHA256_ALGORITHM`) and encodes the hash as base64 with a `$6$` prefix to match the Linux crypt format.

### Crash Handling

**Location:** [compat.c:294-321](../../../src/core/compat.c#L294-L321)

Linux uses POSIX signals (`SIGSEGV` handler). Windows uses Structured Exception Handling (SEH) via `SetUnhandledExceptionFilter()`. On crash, the handler logs the event, calls `dump_last_command()`, and creates a crash marker file at `gamedata/run/crash.txt`.

### Platform-Independent Utilities

| Function | Location | Purpose |
|----------|----------|---------|
| `socket_write()` | [compat.h:121](../../../src/core/compat.h#L121) | Abstracts `write()` vs `send()` for socket I/O |
| `ensure_directory()` | [compat.h:114](../../../src/core/compat.h#L114) | Create directory if it doesn't exist (both platforms) |
| `mud_path()` | [db.c:42-56](../../../src/core/db.c#L42-L56) | Build file paths with platform-correct separators |

## Unified Configuration System

The config system provides type-safe access to all tunable game parameters, replacing both the legacy `balance` file and scattered hardcoded values.

### Architecture

Three files form the system:

| File | Purpose |
|------|---------|
| [cfg_keys.h](../../../src/core/cfg_keys.h) | Single source of truth: X-Macro defining all config entries |
| [cfg.h](../../../src/core/cfg.h) | Public API: `cfg()`, `cfg_set()`, `cfg_reset()`, iteration, load/save |
| [cfg.c](../../../src/core/cfg.c) | Implementation: array-backed O(1) lookup, string-to-enum conversion |

### X-Macro Pattern

All config entries are defined once in `cfg_keys.h` using an X-Macro:

```c
#define CFG_ENTRIES \
    CFG_X(COMBAT_BASE_DAMCAP,       "combat.base_damcap",       1000) \
    CFG_X(COMBAT_PVP_DAMAGE_DIVISOR, "combat.pvp_damage_divisor", 2) \
    ...
```

This single definition generates both the `cfg_key_t` enum and the lookup table, ensuring they can never go out of sync.

### API

**Location:** [cfg.h](../../../src/core/cfg.h)

```c
int  cfg( cfg_key_t key );                    // Get value — O(1)
void cfg_set( cfg_key_t key, int value );      // Set value
void cfg_reset( cfg_key_t key );               // Reset to default
void cfg_reset_all( void );                    // Reset all to defaults
cfg_key_t cfg_key_from_string( const char *key ); // String → enum (for admin commands)
const char *cfg_key_to_string( cfg_key_t key );   // Enum → string (for display/DB)
void load_cfg( void );                         // Init defaults, load overrides from game.db
void save_cfg( void );                         // Save non-default values to game.db
```

### Categories

| Prefix | Scope | Examples |
|--------|-------|---------|
| `combat.*` | Global combat parameters | `combat.base_damcap`, `combat.pvp_damage_divisor`, `combat.wpn_dam_divisor` |
| `combat.damcap.*` | Per-class damage caps | `combat.damcap.demon.base`, `combat.damcap.angel.per_power` |
| `progression.*` | Upgrade/generation bonuses | Upgrade damage multipliers, generation damcap bonuses |
| `world.*` | Time, weather, world settings | Time scale, weather parameters |
| `ability.*` | Per-class ability parameters | Cooldowns, costs, resource rates |

### Indexed Lookup Helpers

For array-style configs (e.g., "upgrade level 1-10 damage multiplier"), convenience functions avoid manual enum construction:

| Helper | Purpose |
|--------|---------|
| `cfg_upgrade_dmg(level)` | Upgrade damage multiplier for level 1-10 |
| `cfg_ninja_belt_mult(tier)` | Ninja belt damage multiplier for tier 1-10 |
| `cfg_gen_damcap(gen)` | Generation damcap bonus for gen 1-5 |
| `cfg_superstance_cap(tier)` | Superstance damcap bonus for tier 1-3 |

### Regeneration

Config keys are auto-generated from C constant definitions:

```
python game/tools/generate_cfg_keys.py
```

This reads class ability parameters and generates the `cfg_keys.h` file.

## Output System

**Location:** [output.c](../../../src/core/output.c)

The output system handles message formatting and delivery. Key functions:

| Function | Line | Purpose |
|----------|------|---------|
| `stc()` | [output.c:62](../../../src/core/output.c#L62) | Send raw text to character (primary output function) |
| `cent_to_char()` | [output.c:76](../../../src/core/output.c#L76) | Center-aligned output using NAWS terminal width |
| `col_str_len()` | [output.c:72](../../../src/core/output.c#L72) | Visible string length (delegates to `utf8_visible_width()`) |

### Test Output Capture

**Location:** [output.c:29-57](../../../src/core/output.c#L29-L57)

The output system includes a capture mode for the test harness. When active, `stc()` diverts output to a growable buffer instead of the network descriptor:

| Function | Purpose |
|----------|---------|
| `test_output_start(ch)` | Begin capturing output for character `ch` |
| `test_output_get()` | Return captured output as a string |
| `test_output_clear()` | Clear the capture buffer |
| `test_output_stop()` | Stop capturing and free the buffer |

This allows tests to assert on the exact text a player would see, without requiring a network connection.

## Related Documents

- [Heritage](heritage.md) — What these systems replaced
- [Testing](testing.md) — How test output capture is used
- [Client Protocols](client-protocols.md) — Protocol negotiation that uses the platform layer
