# Codebase Heritage & Modernization

This document traces the lineage of the Dystopia MUD codebase and assesses which layers are original, which have been replaced, and where the codebase stands today.

## Lineage

```
DikuMUD (1990-91)
  │  Core abstractions: CHAR_DATA, OBJ_DATA, ROOM_INDEX_DATA,
  │  AFFECT_DATA, EXIT_DATA. Game loop, area files, command interpreter.
  │
  └─► Merc 2.1 (1992-93)
        │  Rewrote I/O layer, added OLC stubs, hash-table lookups,
        │  descriptor handling, string interning (fread_string).
        │
        └─► GodWars (mid-1990s)
              │  PvP focus: generation system, class powers (Vampire,
              │  Werewolf, Demon), disciplines, stances, weapon skills,
              │  fight timer, multi_hit() combat, arena.
              │
              └─► Dystopia (late 1990s-2000s)
                    │  14 classes: 7 base (Vampire, Werewolf, Demon,
                    │  Drow, Monk, Ninja, Mage) + 7 upgrades (Undead
                    │  Knight, Shapeshifter, Tanarri, Spider Droid,
                    │  Angel, Samurai, Lich). Upgrade system, kingdoms,
                    │  forging, mastery, boards, Ragnarok.
                    │
                    └─► Dystopia+ (2024-present)
                          14 new classes, modern C compiler support,
                          Windows port, SQLite persistence, UTF-8,
                          test harness, CI/CD, cross-platform layer,
                          tiered intro system, config system, tooling.
```

## What Each Layer Contributed

| Layer | Era | Contribution | Still Present |
|-------|-----|-------------|---------------|
| DikuMUD | 1990 | Data structures (`CHAR_DATA`, `OBJ_DATA`, `ROOM_INDEX_DATA`), game loop skeleton, area file format, basic commands | Data structures remain (heavily extended). Game loop rewritten. Area format migrated to SQLite. |
| Merc | 1992 | I/O model (`Read_from_descriptor` / `Write_to_descriptor`), `fread_*` parsers, string management, help system, `sh_int` type | I/O model intact. String functions intact. Help migrated to SQLite. `sh_int` removed (Phase 1). |
| GodWars | ~1995 | Combat engine (`multi_hit`, `one_hit`, `violence_update`), generation system, disciplines, stances, weapon proficiencies, PvP systems | Combat core intact. Stance/weapon systems intact. PvP heavily extended. |
| Dystopia | ~2000 | 14 character classes (7 base + 7 upgrade), upgrade system, forging, kingdoms, mastery, boards, arenas, Ragnarok | All gameplay systems intact and actively maintained. |
| Dystopia+ | 2024+ | 14 new character classes (7 base + 7 upgrade), cross-platform layer, SQLite, UTF-8, test harness, CI/CD, tiered intros, config system, Python tools | Active development. |

## Source Organization

**141 source files** across 7 subdirectories under `game/src/`:

| Directory | Files | Purpose |
|-----------|-------|---------|
| [core/](../../../src/core/) | 15c, 7h | Engine foundation: game loop, database, command interpreter, character handling, compatibility layer, UTF-8, intro system, config, intrusive list library |
| [classes/](../../../src/classes/) | 32c, 21h | All 28 character classes + class armor, clan system |
| [systems/](../../../src/systems/) | 18c, 11h | Game subsystems: tick updates, save, boards, client protocols (MCCP, GMCP, MXP, MSSP, MCMP), profiling |
| [combat/](../../../src/combat/) | 6c | Combat engine, magic, arena, special procedures |
| [commands/](../../../src/commands/) | 12c | Player/admin command handlers (act_*.c, socials, wizutil) |
| [db/](../../../src/db/) | 7c, 7h | SQLite persistence: area, game, player, class databases + sqlite3 amalgamation |
| [world/](../../../src/world/) | 4c, 1h | Online creation (OLC): building, editing, saving areas |

### Key Files by Size

| File | Lines | Origin | Role |
|------|-------|--------|------|
| [merc.h](../../../src/core/merc.h) | 4,830 | Diku/Merc, heavily extended | Master header: all data structures, constants, prototypes |
| [comm.c](../../../src/core/comm.c) | 2,597 | Merc, modernized | Game loop, network I/O, boot sequence, test APIs |
| [handler.c](../../../src/core/handler.c) | 2,551 | Merc, extended | Object/character manipulation, affects |
| [interp.c](../../../src/core/interp.c) | 2,133 | Merc/GodWars, extended | Command table (724 commands), interpreter loop |
| [db.c](../../../src/core/db.c) | 2,070 | Merc, modernized | Boot sequence, path management, area loading orchestration |

## Modernization Inventory

### Replaced / New Systems (Dystopia+)

| System | Replaces | Key Files | Notes |
|--------|----------|-----------|-------|
| **SQLite persistence** | Text-based `.are` files | [db_sql.h](../../../src/db/db_sql.h), [db_sql.c](../../../src/db/db_sql.c) | Per-area `.db` files, two-phase loading, cross-area linking |
| **Cross-platform layer** | Linux-only codebase | [compat.h](../../../src/core/compat.h), [compat.c](../../../src/core/compat.c) | Threading, crypto, sockets, signals, time abstraction |
| **UTF-8 support** | ASCII-only | [utf8.h](../../../src/core/utf8.h), [utf8.c](../../../src/core/utf8.c) | Full Unicode: decode/encode, display width, CJK, name validation, confusable detection |
| **Tiered intro system** | Static login banner | [intro.h](../../../src/core/intro.h), [intro.c](../../../src/core/intro.c) | Capability detection → client-appropriate welcome |
| **Config system** | Hardcoded values + `balance` file | [cfg.h](../../../src/core/cfg.h), [cfg.c](../../../src/core/cfg.c) | Type-safe enum-keyed O(1) lookup, database-backed |
| **Test harness** | None | [test_framework.h](../../../tests/test_framework.h), [test_main.c](../../../tests/test_main.c) | Headless engine init, output capture, CI integration |
| **Build system** | Manual Makefiles | [GenerateProjectFiles.bat](../../../build/GenerateProjectFiles.bat) | Auto-discovers sources, generates Makefile + VS projects |
| **CI/CD** | None | [.github/workflows/](../../../../.github/workflows/) | Dual-platform build + test on push, AWS deploy |
| **Python tools** | None | [game/tools/](../../../tools/) | Area editor (mudedit), test bot (mudbot), constant sourcing from C headers |
| **Client protocols** | Basic telnet | [gmcp.c](../../../src/systems/gmcp.c), [mccp.c](../../../src/systems/mccp.c), [mxp.c](../../../src/systems/mxp.c), [mssp.c](../../../src/systems/mssp.c) | GMCP, MCCP, MXP, MSSP, NAWS, TTYPE, MCMP |
| **Accessibility** | None | [mcmp.c](../../../src/systems/mcmp.c) | Screen reader mode, audio protocol |
| **Game tables in DB** | Hardcoded C arrays | [db_tables.c](../../../src/db/db_tables.c) | Class registry, display data, armor tables queryable from SQLite |
| **Output capture** | None | [output.c](../../../src/core/output.c) | Test-mode character output interception |
| **Class armor DB** | Hardcoded per-class | [class_armor.c](../../../src/classes/class_armor.c) | Database-driven armor definitions |
| **Headless boot** | Requires network | [comm.c:312](../../../src/core/comm.c#L312) | `boot_headless()` initializes engine without sockets |
| **Decoupled tick** | Tied to I/O loop | [comm.c:672](../../../src/core/comm.c#L672) | `game_tick()` callable independently from game loop |
| **Intrusive list library** | Singly-linked lists + LINK/UNLINK macros | [list.h](../../../src/core/list.h) | Type-safe doubly-linked intrusive lists for all global object chains |
| **Code modernization** | Pre-ANSI C compatibility layer | Throughout | 7-phase cleanup: types, allocators, lists, macros, buffer safety (see below) |

### Modernization Phases (Code Quality)

Systematic removal of legacy C patterns inherited from the Diku/Merc era:

| Phase | Scope | What Changed |
|-------|-------|-------------|
| 1 | Types & constants | `sh_int` → `int`, letter macros (`#define A (1<<0)`) → `(1u << N)` bit shifts, typed `uint32_t` flag fields |
| 2 | Memory | Free-list recycling allocators → direct `calloc`/`free`, bucketed string allocator removed, string pool removed |
| 3 | *(deferred)* | Struct field decomposition (readability, ~8700 lines) |
| 4 | Data structures | Intrusive doubly-linked list library ([list.h](../../../src/core/list.h)) adopted across all major global chains (72 files). `LINK`/`UNLINK` macros removed |
| 5 | Compatibility macros | `args()` pre-ANSI prototype wrapper removed (638 sites/32 files). `free_string`/`str_empty` string pool remnants removed (604 sites/45 files) |
| 6 | Legacy wrappers | `STRALLOC`/`STRFREE`/`DISPOSE`/`QUICKLINK`/`QUICKMATCH` macros inlined. `CD`/`OD`/`MID`/`OID`/`RID`/`SF`/`ED` shorthand typedefs replaced with full type names |
| 7 | Buffer safety | All `sprintf` → `snprintf` with `sizeof` (1,614 calls/56 files). `vsprintf` → `vsnprintf`. Zero `sprintf` calls remain in codebase |

### Intact Heritage Systems

These systems retain their original architecture with incremental extensions:

| System | Origin | Key Files | Status |
|--------|--------|-----------|--------|
| Core data structures | Diku/Merc | [merc.h](../../../src/core/merc.h) | Extended with new fields, same shape |
| Combat engine | GodWars | [fight.c](../../../src/combat/fight.c) | `multi_hit`/`one_hit`/`damage` chain intact |
| Command interpreter | Merc | [interp.c](../../../src/core/interp.c) | Same table-driven dispatch, extended to 724 commands |
| String utilities | Merc | [string.c](../../../src/core/string.c) | `fread_string`, `str_cmp`, `str_prefix` unchanged |
| Affect system | Diku | [handler.c](../../../src/core/handler.c) | Same linked-list model |
| Object/character ops | Merc | [handler.c](../../../src/core/handler.c) | `obj_to_char`, `char_from_room`, etc. unchanged |
| Tick/pulse system | Merc | [update.c](../../../src/systems/update.c) | Same pulse-counter model, extended with class updates |
| Memory allocation | Merc | [mem.c](../../../src/core/mem.c) | Direct `calloc`/`free` (free-list recycling removed in Phase 2) |
| Special procedures | Diku | [special.c](../../../src/combat/special.c) | `spec_fun` callback pattern unchanged |

## The Ship of Theseus Assessment

The codebase occupies a middle ground:

**What's been replaced:** The entire persistence layer (text → SQLite), the platform layer (Linux-only → cross-platform), the build system (manual → auto-generated), the intro/login experience, the configuration system, the development workflow (no tests → CI/CD with dual-platform testing), and the C idiom layer (pre-ANSI compatibility macros, unsafe string functions, recycling allocators, singly-linked lists — all modernized across 7 systematic phases).

**What remains:** The fundamental data model (CHAR_DATA with its 100+ fields), the combat engine's attack-resolution chain, the command interpreter's table-driven dispatch, the pulse-based tick system, and the string utilities (`str_cmp`, `str_prefix`, `fread_string`). These are still recognizably Diku/Merc/GodWars code, though extended.

**What's new:** 14 additional character classes, the full client protocol stack, accessibility features, UTF-8 support, Python tooling, the test infrastructure, and the intrusive list library have no equivalent in the ancestor codebases.

The infrastructure is modern. The C code quality is modern (no `sprintf`, no pre-ANSI macros, proper memory management). The game engine core is evolutionary — extended rather than rewritten. The gameplay systems are a mix of inherited (combat, commands) and original (most classes, forging, mastery).

## Related Documents

- [Database Layer](database.md) — SQLite persistence architecture
- [Build System](build-system.md) — Build pipeline, CI/CD, deployment
- [Testing](testing.md) — Test infrastructure and APIs
- [Client Protocols](client-protocols.md) — Protocol stack and capability detection
- [Platform & Config](platform-config.md) — Cross-platform layer and configuration system
