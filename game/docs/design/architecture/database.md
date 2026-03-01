# Database / Persistence Layer

The codebase has been migrated from legacy flat-file persistence (Diku/Merc `.are` text files) to a SQLite-backed system. Area data, game state, class definitions, reference tables, and player saves all use SQLite databases stored under `gamedata/db/`.

## File Layout

```
gamedata/db/
├── areas/              Per-area databases
│   ├── midgaard.db     One .db per area (replaces .are text files)
│   ├── limbo.db
│   └── ...
├── game/               Global game databases
│   ├── base_help.db    Read-only help entries (shipped with release)
│   ├── live_help.db    Runtime help additions/overrides (server-local)
│   ├── game.db         Config, kingdoms, boards, bans, leaderboards, etc.
│   ├── class.db        Class registry: display names, brackets, armor, scores
│   └── tables.db       Reference data: socials, slays, liquids, calendar, wear locations
└── player/             Per-player save databases
```

## Source Modules

All database code is in [game/src/db/](../../../src/db/):

| Module | Lines | Purpose |
|--------|-------|---------|
| [db_sql.h](../../../src/db/db_sql.h) / [db_sql.c](../../../src/db/db_sql.c) | API + 1364 | Area persistence: load/save/scan per-area `.db` files |
| [db_game.h](../../../src/db/db_game.h) / [db_game.c](../../../src/db/db_game.c) | API + impl | Global game data: helps, config, kingdoms, boards, bans, names, audio |
| [db_class.h](../../../src/db/db_class.h) / [db_class.c](../../../src/db/db_class.c) | API + impl | Class registry: names, brackets, armor, score layout |
| [db_tables.h](../../../src/db/db_tables.h) / [db_tables.c](../../../src/db/db_tables.c) | API + impl | Reference data: socials, slays, liquids, wear locations, calendar |
| [db_player.h](../../../src/db/db_player.h) / [db_player.c](../../../src/db/db_player.c) | API + impl | Player save/load with async backup threads |
| [db_util.h](../../../src/db/db_util.h) / [db_util.c](../../../src/db/db_util.c) | API + impl | Shared SQLite helpers (open, prepare, bind, step) |
| [sqlite3.h](../../../src/db/sqlite3.h) / [sqlite3.c](../../../src/db/sqlite3.c) | 636K + 8.8M | Full SQLite amalgamation (no external dependency) |

## Boot Sequence

**Location:** [db.c:356-484](../../../src/core/db.c#L356-L484) — `boot_db()`

The boot sequence initializes databases in dependency order:

```
boot_db()
  ├─ String space, RNG, profiling, time/weather    (legacy Merc init)
  ├─ Skill GSN assignment                          (legacy Merc init)
  │
  ├─ db_game_init()          ← Opens game.db, base_help.db, live_help.db
  ├─ db_class_init()         ← Opens class.db, loads class registry
  ├─ db_tables_init()        ← Opens tables.db, loads reference data
  ├─ db_game_load_helps()    ← Loads help entries into memory
  ├─ intro_load()            ← Caches intro help text pointers
  │
  ├─ Phase 1: Load areas     ← db_sql_scan_areas() + db_sql_load_area() per file
  │   Loads: area metadata, mobiles (MOB_INDEX_DATA), objects (OBJ_INDEX_DATA),
  │          rooms (ROOM_INDEX_DATA), exits, extra descriptions
  │
  ├─ Phase 2: Link areas     ← db_sql_link_area() per file
  │   Loads: resets, shops, specials (may reference vnums from other areas)
  │
  ├─ fix_exits()             ← Resolve cross-area exit vnum references
  ├─ area_update()           ← Initial area reset
  └─ Load remaining data     ← Notes, config, kingdoms, bans, etc.
```

### Two-Phase Area Loading

Areas are loaded in two passes because resets, shops, and specials may reference vnums (mobiles, objects, rooms) defined in other areas. Phase 1 populates all the hash tables; Phase 2 resolves cross-references.

**Phase 1 — Load:** [db_sql.c:928](../../../src/db/db_sql.c#L928) — `db_sql_load_area()`
- Opens area's `.db` file
- Loads area metadata into `AREA_DATA`
- Loads mobiles into `MOB_INDEX_DATA` hash table
- Loads objects into `OBJ_INDEX_DATA` hash table
- Loads rooms into `ROOM_INDEX_DATA` hash table (with exits, extra descriptions)

**Phase 2 — Link:** [db_sql.c:997](../../../src/db/db_sql.c#L997) — `db_sql_link_area()`
- Loads resets (which reference mob/obj/room vnums that may be in other areas)
- Loads shops (attached to mob vnums)
- Loads specials (attached to mob vnums)
- All vnum lookups now succeed because all areas are loaded

## Area Database API

**Location:** [db_sql.h](../../../src/db/db_sql.h)

| Function | Purpose |
|----------|---------|
| `db_sql_init()` | Initialize the `gamedata/db/areas/` directory path |
| `db_sql_scan_areas()` | Scan directory for `.db` files, return sorted filename list |
| `db_sql_load_area()` | Phase 1: load one area's metadata, mobs, objects, rooms |
| `db_sql_link_area()` | Phase 2: load resets, shops, specials with cross-area linking |
| `db_sql_save_area()` | Save one area to its `.db` file (creates or overwrites) |
| `db_sql_free_scan()` | Free the filename list from `db_sql_scan_areas()` |
| `db_sql_area_exists()` | Check if a `.db` file exists for a given area filename |

## Game Database API

**Location:** [db_game.h](../../../src/db/db_game.h)

The game database (`game.db`) stores all non-area persistent data:

| Category | Load Function | Save Function | Storage |
|----------|--------------|---------------|---------|
| Help entries | `db_game_load_helps()` | `db_game_save_helps()` | base_help.db + live_help.db |
| Game config | `db_game_load_gameconfig()` | `db_game_save_gameconfig()` | game.db |
| Unified config | `db_game_load_cfg()` | `db_game_save_cfg()` | game.db |
| Topboard | `db_game_load_topboard()` | `db_game_save_topboard()` | game.db |
| Leaderboard | `db_game_load_leaderboard()` | `db_game_save_leaderboard()` | game.db |
| Kingdoms | `db_game_load_kingdoms()` | `db_game_save_kingdoms()` | game.db |
| Board notes | `db_game_load_notes()` | `db_game_save_board_notes()` | game.db |
| Bug reports | — | `db_game_append_bug()` | game.db |
| Bans | `db_game_load_bans()` | `db_game_save_bans()` | game.db |
| Disabled commands | `db_game_load_disabled()` | `db_game_save_disabled()` | game.db |
| Super admins | `db_game_is_super_admin()` | `db_game_add_super_admin()` | game.db |
| Pretitles | `db_game_load_pretitles()` | `db_game_set_pretitle()` | game.db |
| Forbidden names | `db_game_load_forbidden_names()` | `db_game_save_forbidden_names()` | game.db |
| Profanity filters | `db_game_load_profanity_filters()` | `db_game_save_profanity_filters()` | game.db |
| Confusables | `db_game_load_confusables()` | — | game.db |
| Audio config | `db_game_load_audio_config()` | — | game.db |

### Help System

The help system uses a two-database split:

- **base_help.db** — Shipped with the release, read-only at runtime. Contains all standard help entries.
- **live_help.db** — Server-local overrides and additions. Created at runtime. Entries here take precedence over base_help.db entries with the same keyword.

This allows the release to ship updated help without clobbering server-specific customizations.

## Reference Tables

**Location:** [db_tables.h](../../../src/db/db_tables.h)

The `tables.db` database replaces hardcoded C arrays that were previously scattered across source files:

| Table | Replaces | Content |
|-------|----------|---------|
| socials | `social_table[]` | Social command messages (emote templates) |
| slays | `slay_table[]` | Immortal slay option messages |
| liquids | Hardcoded liquid data | Liquid properties: name, color, hunger/thirst/drunk effects |
| wear_locations | Hardcoded strings | Equipment slot display strings (including screen reader variants) |
| calendar | Hardcoded day/month names | In-game day and month names |

Pre-populated via `seed_tables_db.py` at build time. Loaded read-only at boot, cached in memory, database connection closed afterward.

## Player Persistence

**Location:** [db_player.h](../../../src/db/db_player.h) / [db_player.c](../../../src/db/db_player.c)

Player saves use SQLite with async backup threads. The database is written periodically (every `PULSE_DB_DUMP` = 30 minutes) and on character save events.

## Python Tool Integration

The Python tools ([game/tools/](../../../tools/)) access SQLite databases directly:

- **mudlib/merc_constants.py** — Parses C headers at import time for `#define` constants
- **mudedit/db/repository.py** — Reads `class.db` via `load_class_names()` for the area editor UI
- **mudlib/models.py** — Single import point combining C-sourced constants with DB-sourced data

This means C header changes propagate automatically to Python tools, while database-backed data (class names, display strings) is read directly from SQLite.

## Related Documents

- [Heritage](heritage.md) — Migration context and what was replaced
- [Build System](build-system.md) — How databases are seeded and deployed
- [Platform & Config](platform-config.md) — Unified config system backed by game.db
