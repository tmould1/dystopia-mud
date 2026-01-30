# Command Interpreter Overview

The interpreter dispatches player input to handler functions through a 724-entry command table. Each entry gates access by trust level, character position, class, and discipline level. Commands that don't match the table fall through to the socials system.

## Command Table Structure

**Location:** [interp.c:243-1183](../../src/core/interp.c#L243-L1183) — `cmd_table[]`

**Struct:** [merc.h:3023-3032](../../src/core/merc.h#L3023-L3032)

```c
struct cmd_type {
    char *const name;       // command name (prefix-matchable)
    DO_FUN *do_fun;         // function pointer to handler
    sh_int position;        // minimum POS_* required
    sh_int level;           // minimum trust level (0-12)
    sh_int log;             // logging mode
    int race;               // class restriction (0 = none)
    sh_int discipline;      // discipline index (0 = none)
    sh_int disclevel;       // minimum discipline level
};
```

### Example Entries

```c
// Basic command: anyone standing can use
{ "look",      do_look,      POS_MEDITATING, 0, LOG_NORMAL, 0, 0, 0 },

// Punctuation alias: ' maps to say
{ "'",         do_say,       POS_MEDITATING, 0, LOG_NORMAL, 0, 0, 0 },

// Class-gated: only vampires (class 8) with Quietus disc level 9
{ "flash",     do_flash,     POS_FIGHTING,   3, LOG_NORMAL, 8, 6, 9 },

// Admin command: trust level 7+ only
{ "goto",      do_goto,      POS_DEAD,       7, LOG_NORMAL, 0, 0, 0 },
```

## Dispatch Flow

**Location:** [interp.c:1189-1652](../../src/core/interp.c#L1189-L1652) — `interpret()`

```
Player Input
  │
  ├─ 1. Pre-checks (frozen, AFK auto-exit)
  ├─ 2. Extract command word
  │     └─ Special: non-alpha first char is a single-char command (', :, ., ;)
  ├─ 3. Wildcard listing (command ending with *)
  ├─ 4. Alias expansion → recursive interpret()
  ├─ 5. Linear search through cmd_table[]
  │     ├─ First character match
  │     ├─ Prefix match via str_prefix()
  │     ├─ Trust level check
  │     └─ State restrictions (see below)
  ├─ 6. Logging
  ├─ 7. Position verification
  ├─ 8. Disabled command check
  └─ 9. Dispatch: (*cmd_table[cmd].do_fun)(ch, argument)
        │
        └─ If no match: check_social() → "Huh?"
```

### Prefix Matching

Commands are matched by prefix — players don't need to type the full name:

- `l` → `look`
- `inv` → `inventory`
- `ki` → `kill`

The first match in the table wins, so command ordering matters. First character is compared before calling `str_prefix()` as an optimization.

### Wildcard Listing

Typing a command ending with `*` lists all matching commands the player has access to:

- `cast*` → lists all commands starting with "cast"
- Filtered by `can_interpret()` so players only see commands they can use
- Displayed in 5-column format with total count

### Alias System

**Location:** [interp.c:1278-1288](../../src/core/interp.c#L1278-L1288)

Players define aliases mapping short names to full commands. When input matches an alias in `ch->pcdata->alias`, the interpreter recursively calls `interpret()` with the expanded text.

## Permission System

**Location:** [interp.c:35-58](../../src/core/interp.c#L35-L58) — `can_interpret()`

```c
int can_interpret( CHAR_DATA *ch, int cmd )
//  Returns:  1 = allowed
//           -1 = command exists but position too low
//            0 = no permission
```

### Check Order

1. **Implementor bypass** — level 12 can use everything
2. **Trust level** — `cmd_table[cmd].level > get_trust(ch)` → denied
3. **No restrictions** — if `race == 0` and `discipline == 0` → allowed
4. **Class gate** — if `race > 0` and `ch->class == race` → allowed
5. **Discipline gate** — if `discipline > 0` and `ch->power[discipline] >= disclevel` → allowed
6. **Position check** — if `ch->position < cmd_table[cmd].position` → return -1

The three-way logic means a command can require class membership OR a discipline level, but trust level is always checked first.

## Trust Levels

**Location:** [merc.h:230-248](../../src/core/merc.h#L230-L248)

| Level | Constant | Role | Commands |
|-------|----------|------|----------|
| 0 | — | Guest/new player | 165 (23%) |
| 1 | — | Hero | 27 (4%) |
| 2 | `LEVEL_MORTAL` | Mortal | 34 (5%) |
| 3 | `LEVEL_AVATAR` | Avatar | 387 (53%) |
| 4 | `LEVEL_APPRENTICE` | Apprentice immortal | 3 |
| 5 | `LEVEL_MAGE` | Mage immortal | — |
| 6 | `LEVEL_ARCHMAGE` | Archmage | — |
| 7 | `LEVEL_BUILDER` | Builder | 22 (3%) |
| 8 | `LEVEL_QUESTMAKER` | Quest maker | 31 (4%) |
| 9 | `LEVEL_ENFORCER` | Enforcer | 8 (1%) |
| 10 | `LEVEL_JUDGE` | Judge | 17 (2%) |
| 11 | `LEVEL_HIGHJUDGE` | High Judge | 7 (1%) |
| 12 | `LEVEL_IMPLEMENTOR` | Implementor | 23 (3%) |

The bulk of gameplay commands (53%) sit at trust level 3 (Avatar) — class powers, discipline abilities, and advanced actions.

## Position Constants

**Location:** [merc.h:1817-1826](../../src/core/merc.h#L1817-L1826)

Commands require the character to be at or above a minimum position:

| Value | Constant | Meaning | Example Commands |
|-------|----------|---------|-----------------|
| 0 | `POS_DEAD` | Dead/corpse | Immortal admin tools, inventory, score |
| 1 | `POS_MORTAL` | Mortally wounded | — |
| 2 | `POS_INCAP` | Incapacitated | — |
| 3 | `POS_STUNNED` | Stunned | — |
| 4 | `POS_SLEEPING` | Sleeping | Some channels (music, miktalk) |
| 5 | `POS_MEDITATING` | Meditating | look, say, tell, emote |
| 6 | `POS_SITTING` | Sitting | yell, whisper |
| 7 | `POS_RESTING` | Resting | — |
| 8 | `POS_FIGHTING` | In combat | cast, kill, diagnose |
| 9 | `POS_STANDING` | Standing | Movement, most actions |

`POS_DEAD` is used for commands that should always work (admin tools, score, quit). Most gameplay commands require `POS_STANDING`.

## Logging Modes

**Location:** [interp.c:201-203](../../src/core/interp.c#L201-L203)

| Constant | Value | Behavior |
|----------|-------|----------|
| `LOG_NORMAL` | 0 | Log if player logging or global logging is enabled |
| `LOG_ALWAYS` | 1 | Always log (sensitive commands: order, alias, mastery) |
| `LOG_NEVER` | 2 | Never log (redacted from audit trail) |

## State Restrictions

**Location:** [interp.c:1297-1539](../../src/core/interp.c#L1297-L1539)

Beyond the standard position check, `interpret()` enforces special restrictions based on character state:

| State | Effect | Allowed Commands |
|-------|--------|-----------------|
| `LOST_HEAD` | Decapitated | say, look, score, inventory, and a small whitelist |
| `OSWITCH` | Object-switched | Same whitelist as LOST_HEAD |
| Earthmelded | Merged with earth | earthmeld, burrow, look, save, exits, wake, inventory |
| Embracing | Vampire feeding | Communication, info commands, kill, diablerize |
| Tied up | Bound | say, look, calls for help only |

These override the normal permission system — even if a command would normally be allowed, these states can block it.

## Class and Discipline Gating

The `race` and `discipline` fields in `cmd_table` control class-specific command access:

### Class-Only Commands (race > 0, discipline = 0)

The `race` field matches against `ch->class`. Used for class channels and basic class features:

```c
// Only monks (class 64) can use monktalk
{ "monktalk", do_monktalk, POS_DEAD, 3, LOG_NORMAL, 64, 0, 0 },
```

### Discipline-Gated Commands (discipline > 0)

Requires `ch->power[discipline] >= disclevel`. Used for vampire, werewolf, and daemon powers that unlock at specific discipline levels:

```c
// Requires Quietus (disc 6) at level 9
{ "flash", do_flash, POS_FIGHTING, 3, LOG_NORMAL, 8, 6, 9 },

// Requires Dominate (disc 9) at level 3
{ "command", do_command, POS_SITTING, 3, LOG_NORMAL, 8, 9, 3 },
```

## Social Fallback

**Location:** [interp.c:1655-1754](../../src/core/interp.c#L1655-L1754) — `check_social()`

When no command matches, the interpreter tries the social table before returning "Huh?":

1. **Lookup** — prefix match against `social_table[]` (same matching rules as commands)
2. **Position check** — no socials while dead/incapacitated/stunned; only `snore` while sleeping
3. **Message selection** — picks variant based on target: no argument, target found, self-target
4. **NPC reaction** — awake, uncharmed NPCs may counter-social (10/16 chance), slap, or attack

See [Communication System](../communication/overview.md) for the social data structure and full list.

## Command Statistics

**Total:** 724 commands across 12 trust levels.

| Category | Examples | Approximate Count |
|----------|---------|-------------------|
| Class powers | discipline abilities, transformations | ~390 |
| General gameplay | look, get, kill, cast, movement | ~165 |
| Admin/building | goto, transfer, aedit, redit | ~100 |
| Communication | chat, tell, say, class channels | ~30 |
| Information | score, who, help, time, areas | ~25 |
| System | quit, save, password, alias | ~15 |

## Source Files

| File | Contents |
|------|----------|
| [interp.c:243-1183](../../src/core/interp.c#L243-L1183) | `cmd_table[]` — 724 command entries |
| [interp.c:1189-1652](../../src/core/interp.c#L1189-L1652) | `interpret()` — main dispatch function |
| [interp.c:35-58](../../src/core/interp.c#L35-L58) | `can_interpret()` — permission checking |
| [interp.c:1655-1754](../../src/core/interp.c#L1655-L1754) | `check_social()` — social fallback |
| [merc.h:3023-3032](../../src/core/merc.h#L3023-L3032) | `cmd_type` struct definition |
| [merc.h:1817-1826](../../src/core/merc.h#L1817-L1826) | `POS_*` position constants |
| [merc.h:230-248](../../src/core/merc.h#L230-L248) | Trust level constants |
