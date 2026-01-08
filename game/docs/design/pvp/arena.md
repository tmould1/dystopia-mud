# Arena System

This document details the arena PvP event system.

## Overview

The arena provides organized group PvP events where multiple players compete until one remains.

## Global Variables

**Location:** [interp.c:213-226](../../src/core/interp.c#L213-L226)

| Variable | Type | Purpose |
|----------|------|---------|
| `arenaopen` | bool | Whether arena is accepting entrants |
| `arena_base` | bool | If true, restricts to base classes only |
| `pulse_arena` | int | Ticks until next arena event |

## Arena Rooms

- VNUM range: 50-67
- Maximum 6 players per arena

## Commands

### arenastats

**Location:** [arena.c:35-71](../../src/combat/arena.c#L35-L71)

Displays current health, stamina, and mana of all arena participants.

### arenajoin

**Location:** [arena.c:124-183](../../src/combat/arena.c#L124-L183)

Join an active arena event.

**Requirements:**
- Arena must be open (`arenaopen == TRUE`)
- Player must not have active `fight_timer`
- Player must be age 2+ (not a newbie)
- If `arena_base == TRUE`, player's `upgrade_level` must be 0
- Arena must have fewer than 6 players

**On Join:**
- Player teleported to random arena room (VNUM 50-67)
- Player given `PLR_FREEZE` flag (cannot act until arena starts)
- Message broadcast to all players

### resign

**Location:** [arena.c:185-234](../../src/combat/arena.c#L185-L234)

Exit the arena early, recording a loss.

**Effects:**
- `pcdata->alosses++` incremented
- Player teleported out of arena
- If this leaves only one player, that player wins

## Arena Lifecycle

### Opening

**Function:** `open_arena()` at [arena.c:73-88](../../src/combat/arena.c#L73-L88)

- 70% chance: All classes allowed (`arena_base = FALSE`)
- 30% chance: Base classes only (`arena_base = TRUE`)
- Sets `arenaopen = TRUE`
- Broadcasts announcement to all players

### Closing

**Function:** `close_arena()` at [arena.c:90-122](../../src/combat/arena.c#L90-L122)

- Sets `arenaopen = FALSE`
- Unfreezes all participants (removes `PLR_FREEZE`)
- If multiple players remain: Broadcasts "Let the games begin!"
- If one player remains: Awards win and prize

### Winning

When only one player remains in the arena:
- `pcdata->awins++` incremented
- Prize awarded via `win_prize()` ([jobo_util.c:68-104](../../src/systems/jobo_util.c#L68-L104))
- Winner teleported to altar
- Winner fully restored

## Prizes

**Location:** [jobo_util.c:68-104](../../src/systems/jobo_util.c#L68-L104)

Prize items are defined in [classeq.are](../../../../gamedata/area/classeq.are) (VNUMs 33851-33857).

| Roll | Chance | VNUM | Prize | Effect |
|------|--------|------|-------|--------|
| 1-49 | 49% | 30037 | Prize Token | Quest item worth 100-300 QP |
| 50-64 | 15% | 33851 | Wand of Brimstone | 4 charges of "imp fireball" |
| 65-74 | 10% | 33852 | Wand of Lloth | 5 charges of "web" |
| 75-79 | 5% | 33853 | Wand of Wonder | 4 charges of "imp teleport" |
| 80-84 | 5% | 33854 | Fatal Scroll | Casts desanct, curse, web |
| 85-89 | 5% | 33855 | Scroll of the Nethereese | Casts dispel magic, curse, web |
| 90-94 | 5% | 33856 | Floating Ion Stone | +7 hit, +9 dam, +2 saves |
| 95-100 | 6% | 33857 | 1st Prize Medal | +8 hit, +4 dam, -12 AC |

The most common prize (~49%) is a prize token exchangeable for quest points. The rarest prizes (Ion Stone and 1st Prize Medal) provide permanent stat bonuses.

## Player Data

Arena statistics tracked in `PC_DATA`:

| Field | Purpose |
|-------|---------|
| `awins` | Total arena victories |
| `alosses` | Total arena losses (including resignations) |

## Event Timing

The arena opens periodically based on `pulse_arena`:
- Decrements each tick
- When reaches 0, `open_arena()` called
- Event interval: 60 ticks between arena events (approximate)

## Restrictions

| Restriction | Reason |
|-------------|--------|
| Fight timer active | Cannot join while recently in combat |
| Newbie (age < 2) | New players protected from PvP |
| Upgrade in base arena | Prevents skill tier imbalance |
| Max 6 players | Room/balance limitation |
