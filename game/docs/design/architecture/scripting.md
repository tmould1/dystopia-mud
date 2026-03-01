# Scripting System Design — Mob, Object, and Room Programs

## Current Capabilities

### Special Functions (`spec_fun`) — Mob AI Only

37 hardcoded C functions in `game/src/combat/special.c` attached to mobs via function pointer (`SPEC_FUN` typedef in `types.h`). Called each AI tick in `update.c`; returning TRUE skips normal AI for that mob.

**OLC support**: `medit spec <name>` / `medit spec none`. **Area file storage**: `#SPECIALS` section.

Available functions include breath attacks (acid, fire, frost, gas, lightning), spellcasters (adept, cleric, judge, mage, undead), behaviors (fido, guard, janitor, mayor, thief), class-specific guards (werewolf, dragon, vampire, mage, demon, drow, ninja, monk, highlander, cyborg), and combat specialists (assassin, executioner, rogue, zombie_lord).

**Strengths**: Zero-overhead execution, type-safe, debuggable in C, full OLC support.

**Weaknesses**: Requires recompilation to add new behaviors; builders can only pick from existing set, not create custom logic.

### RoomText — Room Triggers Only

Pattern-matching trigger system on `ROOMTEXT_DATA` struct (`room.h`). Fires on room entry (`>ENTER<`) and player say commands. Can output text, spawn mobs/objects, cast spells, teleport, create portals.

Pattern syntax: `&word` (contains), `word*` (prefix), `*word` (suffix), `|word1|word2|` (alternatives).

Trigger types: `RT_SAY`, `RT_LIGHTS`, `RT_OBJECT`, `RT_MOBILE`, `RT_SPELL`, `RT_PORTAL`, `RT_TELEPORT`, `RT_ACTION`, `RT_ENTER`, `RT_CAST`, `RT_THROWOUT`, `RT_OPEN_LIFT`, `RT_CLOSE_LIFT`, `RT_MOVE_LIFT`.

**No conditional logic, no variables, no loops** — purely reactive. **No OLC support** — must hand-edit area files.

### Vestigial Infrastructure

`SUB_MPROG_EDIT` substate exists in `merc.h` (line buffer limit of 48 in `build.c`) but no mobprog system is implemented.

### Gap Summary

| Capability | Mobs | Objects | Rooms |
|---|---|---|---|
| Hardcoded C behaviors | 37 spec_funs | None | None |
| Trigger-based scripting | None | None | RoomText (limited) |
| In-game editable | Pick from list only | None | None |
| Conditional logic | None | None | None |

---

## Survey of Approaches

### Classic MobProgs (ROM/SMAUG)

Custom interpreted language with `if/else/endif`, `if_check` conditions, `mob_cmd` actions. Triggers: GREET, SPEECH, DEATH, FIGHT, RAND, etc.

- **Pros**: Proven in dozens of MUDs, builders already know it, lightweight
- **Cons**: Fragile parser, limited expressiveness, hard to debug, poor error messages, no real data structures, security issues with `mob_command` executing arbitrary commands

### Lua Integration

Embeddable scripting via the Lua C API. Used by Aardwolf MUD, CoffeeMUD (moved to Lua), many modern MUD frameworks.

- **Pros**: Mature ecosystem, tiny footprint (~300KB), fast (LuaJIT even faster), excellent C interop, sandboxable, real programming language with tables/closures/coroutines, cross-platform (pure ANSI C), builders can learn it quickly
- **Cons**: Another dependency to bundle, learning curve for builders unfamiliar with programming, requires careful API surface design

### LPC (MudOS/FluffOS)

Full object-oriented language where everything (rooms, mobs, objects) is a program.

- **Cons**: Requires rewriting the entire engine architecture — not feasible for a Diku derivative

### JavaScript/Python Embedding

V8 or CPython embedded in the game loop.

- **Cons**: Massive dependencies, GIL issues (Python), complex build requirements, overkill for MUD scripting

---

## Recommendation: Lua

Lua is the clear choice for Dystopia MUD:

1. **Pure ANSI C** — compiles with both GCC and MSVC without changes
2. **Tiny** — Lua 5.4 is ~30 source files, can be built as part of the project (no external dependency)
3. **Fast** — sub-microsecond function calls, negligible impact on game loop
4. **Sandboxable** — remove `io`, `os`, `debug` libraries; set instruction count hooks to prevent infinite loops
5. **Proven** — the dominant choice for MUD scripting in modern codebases
6. **Coexists** — existing spec_funs and RoomText continue working unchanged

---

## Implementation Design

### New Source Directory: `game/src/script/`

| File | Purpose |
|---|---|
| `script.h` | Trigger types, SCRIPT_DATA struct, public API |
| `script_lua.c` | Lua state management, sandbox setup, C-to-Lua bridge |
| `script_trigger.c` | Trigger dispatch — fires Lua callbacks from game events |
| `script_api.c` | C functions exposed to Lua (`mud.send`, `mud.damage`, etc.) |
| `script_olc.c` | OLC commands for editing scripts in-game (`scriptedit`) |

### Lua Source: `game/lib/lua/`

Bundle Lua 5.4 source (MIT license) directly — no external dependency needed.

### Trigger Types

```c
/* script.h */
#define TRIG_NONE           0

/* Mob triggers */
#define TRIG_GREET          (1 <<  0)   /* Player enters room */
#define TRIG_SPEECH         (1 <<  1)   /* Player says something */
#define TRIG_FIGHT          (1 <<  2)   /* Each combat round */
#define TRIG_DEATH          (1 <<  3)   /* Mob dies */
#define TRIG_RAND           (1 <<  4)   /* Random chance each tick */
#define TRIG_HIT            (1 <<  5)   /* Mob is hit */
#define TRIG_GIVE           (1 <<  6)   /* Player gives item to mob */

/* Object triggers */
#define TRIG_GET            (1 <<  8)   /* Object is picked up */
#define TRIG_DROP           (1 <<  9)   /* Object is dropped */
#define TRIG_USE            (1 << 10)   /* Object is used/activated */
#define TRIG_WEAR           (1 << 11)   /* Object is worn */
#define TRIG_REMOVE         (1 << 12)   /* Object is removed */
#define TRIG_TIMER          (1 << 13)   /* Timer expires on object */

/* Room triggers */
#define TRIG_ENTER          (1 << 16)   /* Player enters room */
#define TRIG_EXIT           (1 << 17)   /* Player leaves room */
#define TRIG_SAY            (1 << 18)   /* Player speaks in room */
#define TRIG_COMMAND        (1 << 19)   /* Custom command in room */
```

### Script Data Structure

```c
typedef struct script_data {
    int           vnum;         /* Unique script ID */
    char         *name;         /* Builder-friendly name */
    uint32_t      trigger;      /* Trigger type bitmask */
    char         *code;         /* Lua source code */
    char         *pattern;      /* Optional pattern match (for SPEECH/SAY) */
    int           chance;       /* Percent chance to fire (1-100) */
    list_node_t   node;         /* Intrusive list linkage */
} SCRIPT_DATA;
```

Attached via `list_head_t scripts` field added to `MOB_INDEX_DATA` (`char.h`), `OBJ_INDEX_DATA` (`object.h`), and `ROOM_INDEX_DATA` (`room.h`).

### What Builders Write (Lua)

```lua
-- Mob greet trigger: welcome new players, direct experienced ones
function on_greet(mob, ch)
    if ch:level() < 5 then
        mob:say("Welcome, young adventurer!")
        mob:emote("hands you a torch.")
        mob:give(ch, 3050)  -- vnum of torch object
    else
        mob:say("You look experienced. Seek the temple to the north.")
    end
end

-- Mob speech trigger: respond to quest keyword
function on_speech(mob, ch, text)
    if text:find("quest") then
        mob:say("I need the Dragon's Eye gem. Bring it to me!")
        ch:quest_set("dragon_eye", 1)
    end
end
```

### C Functions Exposed to Lua

**Character methods** (via metatable on lightuserdata):
- `ch:name()`, `ch:level()`, `ch:class()`, `ch:race()`, `ch:hp()`, `ch:max_hp()`
- `ch:in_room()`, `ch:is_npc()`, `ch:is_immortal()`
- `ch:has_item(vnum)`, `ch:gold()`, `ch:alignment()`
- `ch:send(text)` — send text to this character
- `ch:quest_get(key)`, `ch:quest_set(key, value)` — per-character quest state

**Mob/NPC actions** (only available on NPC userdata):
- `mob:say(text)`, `mob:emote(text)`, `mob:yell(text)`
- `mob:give(ch, obj_vnum)`, `mob:take(ch, obj_vnum)`
- `mob:cast(spell, target)`, `mob:damage(ch, amount)`
- `mob:follow(ch)`, `mob:goto_room(vnum)`, `mob:force(command)`

**Room methods**:
- `room:vnum()`, `room:name()`, `room:sector()`
- `room:chars()` — iterator over characters in room
- `room:send(text)` — send text to all characters in room

**Object methods**:
- `obj:vnum()`, `obj:name()`, `obj:type()`
- `obj:owner()`, `obj:in_room()`
- `obj:timer_set(seconds)`, `obj:extract()`

**Global `mud` table**:
- `mud.log(text)`, `mud.echo_area(area, text)`
- `mud.time()` — in-game time
- `mud.random(min, max)` — RNG

### Sandbox Safety

```c
/* In script_lua.c — state initialization */
static void script_sandbox(lua_State *L) {
    /* Remove dangerous libraries */
    lua_pushnil(L); lua_setglobal(L, "io");
    lua_pushnil(L); lua_setglobal(L, "os");
    lua_pushnil(L); lua_setglobal(L, "debug");
    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "require");

    /* Instruction count hook — kill runaway scripts after 10K ops */
    lua_sethook(L, script_timeout_hook, LUA_MASKCOUNT, 10000);
}
```

### Hook Points in Existing Code

| Trigger | File | Location | How |
|---|---|---|---|
| TRIG_GREET / TRIG_ENTER | `act_move.c` | After `char_to_room()` | Call `script_trigger_room_enter(ch, room)` and `script_trigger_mob_greet(ch, room)` |
| TRIG_SPEECH / TRIG_SAY | `act_comm.c` | In `do_say()` after `room_text()` | Call `script_trigger_speech(ch, argument)` |
| TRIG_DEATH | `fight.c` | In `raw_kill()` before corpse creation | Call `script_trigger_death(victim, killer)` |
| TRIG_FIGHT | `fight.c` | In `multi_hit()` or `one_hit()` | Call `script_trigger_fight(ch, victim)` |
| TRIG_RAND | `update.c` | In `mobile_update()` alongside `spec_fun` | Call `script_trigger_random(ch)` |
| TRIG_GIVE | `act_obj.c` | In `do_give()` after item transfer | Call `script_trigger_give(ch, victim, obj)` |
| TRIG_GET/DROP/WEAR/REMOVE | `act_obj.c` | Respective `do_*` functions | Call appropriate trigger |

### Storage

**Initial approach**: Store scripts in area files (new `#SCRIPTS` section):
```
#SCRIPTS
S 5001 greet_guard~
T 1
C 100
P ~
L
function on_greet(mob, ch)
    mob:say("Halt! State your business.")
end
~
S 5002 quest_giver~
T 2
C 100
P quest~
L
function on_speech(mob, ch, text)
    if text:find("quest") then
        mob:say("Seek the dragon!")
    end
end
~
#0
```

**Later**: Migrate to SQLite (`db_scripts` table) for easier editing and version tracking.

### Coexistence with Existing Systems

- **spec_funs**: Continue working unchanged. `spec_fun` fires first (in `update.c`), Lua triggers fire after. Builders can use both on the same mob.
- **RoomText**: Continue working unchanged. Room Lua triggers fire after RoomText processing. Gradual migration possible — a builder could rewrite a RoomText as a Lua script with much more power.

---

## Implementation Phases

### Phase 1: Lua Foundation
- Bundle Lua 5.4 source in `game/lib/lua/`
- Create `game/src/script/script.h` and `script_lua.c`
- Initialize/destroy Lua state in game startup/shutdown
- Sandbox setup (remove dangerous globals, instruction count hook)
- Update build system (`GenerateProjectFiles.bat/.sh` — add `script` to SUBDIRS)
- **Test**: Verify Lua state creates and simple `lua_dostring()` works

### Phase 2: Mob Triggers (GREET + SPEECH)
- Implement `SCRIPT_DATA` structure with intrusive list
- Add `list_head_t scripts` to `MOB_INDEX_DATA` (`char.h`)
- Implement C-to-Lua bridge for character userdata + metatables
- Hook `TRIG_GREET` into `act_move.c` and `TRIG_SPEECH` into `act_comm.c`
- Expose basic `ch:name()`, `ch:level()`, `mob:say()`, `mob:emote()`
- Load scripts from area file `#SCRIPTS` section
- **Test**: Create a mob that greets players and responds to keywords

### Phase 3: Full Mob + Room Triggers
- Add remaining mob triggers (DEATH, FIGHT, RAND, GIVE, HIT)
- Add room triggers (ENTER, EXIT, SAY, COMMAND)
- Add `list_head_t scripts` to `ROOM_INDEX_DATA` (`room.h`)
- Expose room methods to Lua
- **Test**: Room entry puzzle, death trigger drops quest item

### Phase 4: Object Triggers + OLC
- Add object triggers (GET, DROP, USE, WEAR, REMOVE, TIMER)
- Add `list_head_t scripts` to `OBJ_INDEX_DATA` (`object.h`)
- Implement `scriptedit` OLC command for in-game editing
- Script listing, viewing, and syntax checking via OLC
- **Test**: Object that casts a spell when worn, timer-based puzzle

### Phase 5: Quest State + Advanced Features
- Per-character persistent quest variables (backed by SQLite)
- `ch:quest_get(key)` / `ch:quest_set(key, value)` API
- Script-to-script communication (global event bus)
- Migrate willing RoomText entries to Lua equivalents
- Builder documentation and example scripts

---

## Key Files Affected

Struct locations reflect the Phase 3 merc.h decomposition (completed). `DECLARE_*_FUN` macros were removed in Phase 12 — use typedefs directly.

| File | Change |
|---|---|
| `game/lib/lua/` (new) | Lua 5.4 source (MIT, ~30 files) |
| `game/src/script/` (new) | `script.h`, `script_lua.c`, `script_trigger.c`, `script_api.c`, `script_olc.c` |
| `game/src/core/types.h` | Forward declaration for `SCRIPT_DATA` |
| `game/src/core/char.h` | Add `list_head_t scripts` to `mob_index_data` |
| `game/src/core/object.h` | Add `list_head_t scripts` to `obj_index_data` |
| `game/src/core/room.h` | Add `list_head_t scripts` to `room_index_data` |
| `game/src/core/db.c` | Load `#SCRIPTS` section from area files |
| `game/src/commands/act_move.c` | Hook `TRIG_GREET` / `TRIG_ENTER` after `char_to_room()` |
| `game/src/commands/act_comm.c` | Hook `TRIG_SPEECH` / `TRIG_SAY` in `do_say()` |
| `game/build/GenerateProjectFiles.bat` | Add `script` to SUBDIRS |
| `game/build/GenerateProjectFiles.sh` | Add `script` to SUBDIRS |
