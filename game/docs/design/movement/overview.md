# Movement / Navigation System Overview

Characters move between rooms through 6 directional exits, portals, and special commands. Movement is gated by position, physical state, terrain type, flight requirements, door state, magical walls, and movement points. Rooms are organized into areas and connected by `EXIT_DATA` links.

**Location:** [act_move.c](../../src/commands/act_move.c)

## Core Movement: move_char()

**Location:** [act_move.c:49](../../src/commands/act_move.c#L49)

The central function called by all directional commands. Validates and executes a single room transition.

### Validation Chain

Checks are performed in this order — first failure stops movement:

| # | Check | Failure Message |
|---|-------|-----------------|
| 1 | Exit exists and has valid destination | "Alas, you cannot go that way" |
| 2 | No wall objects blocking the direction | "You are unable to pass the wall" |
| 3 | Door not closed (werewolf boar discipline can smash through, except prismatic) | "The $d is closed" |
| 4 | Not charmed and separated from master | "What? And leave your beloved master?" |
| 5 | Not a riderless mount | — |
| 6 | Destination not private (admins bypass) | — |
| 7 | Physical capability — not crippled (both legs + both arms broken/lost) | — |
| 8 | Sector requirements — flight for air, boat for deep water | "You need a boat for that" / "You can't fly" |
| 9 | Fish polymorph restricted to water sectors | — |
| 10 | Movement points sufficient | "You are too Exhausted" |
| 11 | Magical wall survival — fire/sword/ash walls deal damage on passage | — |

### Movement Point Cost

**Location:** [act_move.c:35](../../src/commands/act_move.c#L35) — `movement_loss[]`

Cost = `movement_loss[from_sector] + movement_loss[to_sector]`

| Sector | Constant | Cost |
|--------|----------|------|
| Inside | `SECT_INSIDE` (0) | 1 |
| City | `SECT_CITY` (1) | 2 |
| Field | `SECT_FIELD` (2) | 2 |
| Forest | `SECT_FOREST` (3) | 3 |
| Hills | `SECT_HILLS` (4) | 4 |
| Mountain | `SECT_MOUNTAIN` (5) | 6 |
| Water (swim) | `SECT_WATER_SWIM` (6) | 4 |
| Water (no swim) | `SECT_WATER_NOSWIM` (7) | 1 |
| Air | `SECT_AIR` (9) | 10 |
| Desert | `SECT_DESERT` (10) | 6 |

Heroes (`IS_HERO`) have zero movement cost. Mounted characters pay the cost, not the mount.

### Departure and Arrival Messages

Messages are customized based on character state:

| State | Verb |
|-------|------|
| Low HP | crawl / limp / stagger |
| `POLY_FISH` | swims |
| `POLY_SERPENT` | slithers |
| `POLY_WOLF` | stalks |
| `POLY_FROG` | hops |
| `AFF_ETHEREAL` | floats |
| Mounted | rides on \<mount\> |
| Drunk | staggers |
| Missing head / broken spine | crawls |
| `AFF_SNEAK` | no message (invisible movement) |
| Default | leaves / arrives |

### Post-Move

After a successful move:
1. Deduct movement points (if not mounted)
2. Set 1 wait state
3. Execute `do_look` for the new room
4. Move followers and mounts
5. Add tracking data ([act_move.c:4036](../../src/commands/act_move.c#L4036))
6. Trigger `drow_hate()` if applicable

## Directions

Six directions with reverse mapping:

| Direction | Constant | Reverse |
|-----------|----------|---------|
| North | `DIR_NORTH` (0) | South (2) |
| East | `DIR_EAST` (1) | West (3) |
| South | `DIR_SOUTH` (2) | North (0) |
| West | `DIR_WEST` (3) | East (1) |
| Up | `DIR_UP` (4) | Down (5) |
| Down | `DIR_DOWN` (5) | Up (4) |

Reverse direction table at [act_move.c:31](../../src/commands/act_move.c#L31): `rev_dir[] = { 2, 3, 0, 1, 5, 4 }`

### Directional Commands

All require `POS_STANDING`, trust 0. Each checks for `AFF_WEBBED` (blocks all movement), calls `move_char()`, adds tracking, and triggers `drow_hate()`.

| Command | Function | Line |
|---------|----------|------|
| `north` | [do_north](../../src/commands/act_move.c#L506) | 506 |
| `east` | [do_east](../../src/commands/act_move.c#L533) | 533 |
| `south` | [do_south](../../src/commands/act_move.c#L559) | 559 |
| `west` | [do_west](../../src/commands/act_move.c#L586) | 586 |
| `up` | [do_up](../../src/commands/act_move.c#L612) | 612 |
| `down` | [do_down](../../src/commands/act_move.c#L638) | 638 |

## Exit System

### EXIT_DATA Structure

**Location:** [merc.h:2505](../../src/core/merc.h#L2505)

```c
EXIT_DATA {
    EXIT_DATA *prev, *next;         // linked list
    EXIT_DATA *rexit;               // reverse exit pointer
    ROOM_INDEX_DATA *to_room;       // destination
    char *keyword;                  // door keywords ("door gate")
    char *description;              // exit description
    int vnum;                       // destination vnum
    int rvnum;                      // reverse exit vnum
    int exit_info;                  // door state flags (EX_*)
    int key;                        // key item vnum (-1 = no lock)
    sh_int vdir;                    // direction (0-5)
};
```

### Exit Flags

| Flag | Value | Effect |
|------|-------|--------|
| `EX_ISDOOR` | 1 | Exit can be opened/closed |
| `EX_CLOSED` | 2 | Currently closed |
| `EX_LOCKED` | 4 | Currently locked |
| `EX_PICKPROOF` | 32 | Cannot be picked |
| `EX_NOPASS` | 64 | Cannot pass even if open |
| `EX_EASY` | 128 | Easy to pick |
| `EX_HARD` | 256 | Hard to pick |
| `EX_INFURIATING` | 512 | Very hard to pick |
| `EX_ICE_WALL` | 4096 | Ice wall (requires daemon Gelu disc >= 5 to pass) |
| `EX_FIRE_WALL` | 8192 | Fire wall (6d50 damage on passage) |
| `EX_SWORD_WALL` | 16384 | Sword wall (6d70 damage on passage) |
| `EX_PRISMATIC_WALL` | 32768 | Prismatic wall (blocks werewolf smash) |
| `EX_IRON_WALL` | 65536 | Iron wall (passable with `AFF_PASS_DOOR`) |
| `EX_MUSHROOM_WALL` | 131072 | Mushroom wall (always blocks) |
| `EX_CALTROP_WALL` | 262144 | Caltrop wall |
| `EX_ASH_WALL` | 524288 | Ash wall (50% HP + 50% move cost) |
| `EX_WARDING` | 1048576 | Warding |

## Door Commands

All require `POS_SITTING`, trust 0. Door operations apply to both sides of the exit (forward and reverse).

### find_door()

**Location:** [act_move.c:664](../../src/commands/act_move.c#L664)

Parses direction shorthand (`n`, `north`, `e`, `east`, etc.) or searches all 6 exits by keyword. Validates that the exit has `EX_ISDOOR` set.

### Commands

| Command | Function | What It Does |
|---------|----------|--------------|
| `open` | [do_open](../../src/commands/act_move.c#L930) | Remove `EX_CLOSED` from both sides. Also opens containers and books. |
| `close` | [do_close](../../src/commands/act_move.c#L1000) | Set `EX_CLOSED` on both sides. Resets book page to 0. |
| `lock` | [do_lock](../../src/commands/act_move.c#L1137) | Requires closed door + matching key in inventory. Sets `EX_LOCKED` both sides. |
| `unlock` | [do_unlock](../../src/commands/act_move.c#L1215) | Requires closed + locked door + matching key. Removes `EX_LOCKED` both sides. |
| `pick` | [do_pick](../../src/commands/act_move.c#L1293) | Skill check against `gsn_pick_lock`. Blocked by `EX_PICKPROOF` and nearby high-level guard NPCs. |

Key matching uses `has_key()` which checks if the character holds an object whose vnum matches `exit->key`.

## Portals

**Location:** [act_move.c:757](../../src/commands/act_move.c#L757) — `do_enter()`

The `enter` command handles `ITEM_PORTAL` and `ITEM_WGATE` objects in the room.

### Portal Values

| Field | Purpose |
|-------|---------|
| `value[0]` | Destination room vnum |
| `value[1]` | Usage count (0 = infinite; decrements each use; portal vanishes at 0) |
| `value[2]` | State: 2 or 3 = closed (blocks entry) |
| `value[3]` | Origin room vnum |

Portals link bidirectionally — on arrival, the system looks for a reverse portal in the destination room whose `value[0]` matches the origin's `value[3]`. Shadowplane state is checked: both the portal and destination must match the character's shadowplane affection.

`ITEM_WGATE` is simpler — one-way teleport to `value[0]` with no reverse linking.

Mounts auto-teleport with their rider.

## Recall and Home

### do_recall()

**Location:** [act_move.c:1688](../../src/commands/act_move.c#L1688)

Teleports the player to their home room (`ch->home`).

**Blocked by:** `ROOM_NO_RECALL`, `AFF_CURSE`, `ROOM_NO_CHANT`, `AFF_TOTALBLIND`

In combat: 50% success rate with 4 wait states on failure. Auto-teleports mount.

### do_home()

**Location:** [act_move.c:1758](../../src/commands/act_move.c#L1758)

Sets current room as the player's home. Syntax: `home here`. Cannot set in `ROOM_NO_RECALL` or `ROOM_SAFE` rooms.

### do_escape()

**Location:** [act_move.c:1787](../../src/commands/act_move.c#L1787)

Emergency teleport to `ROOM_VNUM_TEMPLE`. Heroes only. Blocked in `ROOM_ARENA`. Resets move and mana to 0 as a penalty.

## Flight and Water Requirements

### Air Sectors

`SECT_AIR` rooms require one of:

- `AFF_FLYING`
- Drow with `DPOWER_LEVITATION`
- Vampire with `VAM_FLYING`
- Demon with `DEM_UNFOLDED`
- Mounted on a flying creature

### Deep Water

`SECT_WATER_NOSWIM` rooms require a boat or flight:

- Any `ITEM_BOAT` in inventory
- `VAM_FLYING`, `POLY_SERPENT`, or `AFF_SHADOWPLANE` for vampires
- Drow levitation
- Flying mount

### Fish Polymorph

Characters in `POLY_FISH` form can only move between water sectors (`SECT_WATER_SWIM` and `SECT_WATER_NOSWIM`).

## Tracking System

**Location:** [act_move.c:4036](../../src/commands/act_move.c#L4036) — `add_tracks()`

Each room stores up to 5 track entries (`track[0-4]` and `track_dir[0-4]` in `ROOM_INDEX_DATA`). When a player moves, their name and direction are recorded in the destination room, cycling out old entries.

### Track Exemptions

These characters leave no tracks:

| Class | Condition |
|-------|-----------|
| Werewolf | `DISC_WERE_LYNX > 0` |
| Ninja | Sora power >= 4 |
| Any | `ITEMA_STALKER` flag |

### NPC Hunting

**Location:** [act_move.c:4091](../../src/commands/act_move.c#L4091) — `check_track()`

NPCs with `ch->hunting` set follow tracks that match their target name. Returns the direction of the trail for the NPC to follow.

### Drow Hate

**Location:** [act_move.c:4242](../../src/commands/act_move.c#L4242) — `drow_hate()`

Drow with `NEW_DROWHATE` flag have a 25% chance to auto-attack any non-Drow in the room upon entry.

## Room Flags Affecting Movement

| Flag | Value | Effect |
|------|-------|--------|
| `ROOM_NO_RECALL` | 8192 | Blocks recall and teleport |
| `ROOM_NO_TELEPORT` | 16384 | Blocks teleport-type spells |
| `ROOM_NO_CHANT` | 8388608 | Blocks recall chanting |
| `ROOM_PRIVATE` | 512 | Blocks entry (admin bypass) |
| `ROOM_SAFE` | 1024 | No fighting; blocks home setting |
| `ROOM_ARENA` | 131072 | PvP arena; blocks escape |

## Map System

**Location:** [act_map.c](../../src/commands/act_map.c)

### do_map()

**Location:** [act_map.c:317](../../src/commands/act_map.c#L317)

Displays an ASCII map of surrounding rooms using BFS exploration.

- **Radius:** 1-5 (default 2 → 5x5 grid)
- **Center:** player's current room marked with `@`
- **Symbols:** `[ ]` = room, `[!]` = dead-end, `-` = normal exit, `~` = warped exit (destination doesn't reciprocate)
- **Vertical indicators:** `^` = up only, `v` = down only, `*` = both up and down
- **Detects non-Euclidean topology** — marks exits where the destination's reverse exit doesn't point back

### do_amap()

**Location:** [act_map.c:362](../../src/commands/act_map.c#L362)

Admin variant (trust 7+) that shows room VNUMs instead of symbols, with current room name and area.

## Position Commands

| Command | Function | Transition |
|---------|----------|------------|
| `stand` | `do_stand()` | → `POS_STANDING` |
| `rest` | `do_rest()` | → `POS_RESTING` |
| `sit` | `do_sit()` | → `POS_SITTING` |
| `meditate` | `do_meditate()` | → `POS_MEDITATING` |
| `sleep` | `do_sleep()` | → `POS_SLEEPING` |
| `wake` | `do_wake()` | sleeping → standing |
| `sneak` | `do_sneak()` | Toggle `AFF_SNEAK` (hides movement messages) |
| `hide` | `do_hide()` | Toggle `AFF_HIDE` |

## Source Files

| File | Contents |
|------|----------|
| [act_move.c](../../src/commands/act_move.c) | `move_char()`, directional commands, doors, recall, home, escape, portals, tracking, position commands |
| [act_map.c](../../src/commands/act_map.c) | `do_map()`, `do_amap()` — ASCII map display |
| [merc.h:2505](../../src/core/merc.h#L2505) | `EXIT_DATA` structure |
| [merc.h:2589](../../src/core/merc.h#L2589) | `ROOM_INDEX_DATA` structure (exits, tracks, sector, flags) |
