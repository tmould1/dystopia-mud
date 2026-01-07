# Bounty System

This document details the player bounty system.

## Overview

Players can place QP (Quest Point) bounties on other players. The bounty is collected by whoever kills the target.

## Data Storage

**Location:** [merc.h:2468](../../src/core/merc.h#L2468)

```c
int bounty;  // in CHAR_DATA->pcdata
```

The bounty value is stored in quest points.

## Commands

### bounty

**Location:** [act_comm.c:45-107](../../src/commands/act_comm.c#L45-L107)

**Syntax:** `bounty <player> <amount>`

**Requirements:**
- Amount must be >= 100 QPs
- Player must have sufficient QPs
- Target must be a player (not NPC)
- Target must not be self
- Target must not be immortal (level >= 7)
- Placer must be avatar (age 2+)

**Effects:**
- Deducts `amount` QPs from placer
- Adds `amount` to target's `bounty` field
- Broadcasts announcement to all players

### bountylist

**Location:** [jobo_act.c:37-58](../../src/systems/jobo_act.c#L37-L58)

**Syntax:** `bountylist`

Displays all players with active bounties.

**Information shown:**
- Player name
- Bounty amount (QPs)
- PK ratio (from `get_ratio()`)
- Generation level
- Upgrade level

**Filters:**
- Only shows players level <= 6 (avatars, not immortals)
- Only shows players visible to the viewer

## Bounty Collection

**Location:** [fight.c:2181-2188](../../src/combat/fight.c#L2181-L2188)

When a player kills another player with a bounty:

1. Killer receives all QPs from the bounty
2. Victim's bounty reset to 0
3. Message displayed: "You receive a [X] QP bounty for killing [victim]"

## Command Registration

| Command | Line in interp.c |
|---------|------------------|
| `bounty` | [993](../../src/core/interp.c#L993) |
| `bountylist` | [994](../../src/core/interp.c#L994) |

## Gameplay Notes

- Bounties are cumulative - multiple players can add to the same target's bounty
- The full bounty goes to the killer regardless of who placed it
- Bounties persist until the target is killed or the bounty is otherwise cleared
- Minimum 100 QP prevents trivial bounties
