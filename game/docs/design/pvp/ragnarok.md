# Ragnarok System

This document details the ragnarok global PvP event.

## Overview

Ragnarok is a server-wide PvP event that temporarily removes safe room protections and encourages open combat.

## Global Variables

| Variable | Purpose |
|----------|---------|
| `ragnarok` | bool - Whether ragnarok is currently active |
| `ragnarok_cost` | QPs remaining until ragnarok triggers (starts at 3000) |
| `ragnarok_on_timer` | Ticks remaining in current ragnarok |
| `ragnarok_safe_timer` | Cooldown ticks before ragnarok can be triggered again |

## Command

### ragnarok

**Location:** [jobo_act.c:1200-1251](../../src/systems/jobo_act.c#L1200-L1251)

**Syntax:** `ragnarok <amount>`

**Requirements:**
- Amount must be 100-1000 QPs per bid
- `ragnarok_safe_timer` must be 0 (cooldown expired)
- Player must not have active `fight_timer`
- Player must have sufficient QPs

**Mechanics:**
1. Player bids QPs (100-1000 at a time)
2. QPs deducted from player
3. `ragnarok_cost` reduced by bid amount
4. When `ragnarok_cost <= 0`, ragnarok activates

## Activation

When `ragnarok_cost` reaches 0 ([jobo_act.c:1236-1244](../../src/systems/jobo_act.c#L1236-L1244)):

1. `ragnarok = TRUE`
2. `ragnarok_on_timer = PULSE_RAGNAROK` (15 minutes)
3. `ragnarok_cost` reset to 3000
4. Global announcement broadcast

## Effects During Ragnarok

### Safe Room Override

**Location:** [fight.c:2294](../../src/combat/fight.c#L2294)

Safe rooms (`ROOM_SAFE`) do NOT protect players during ragnarok. The check:
```c
if (ch->in_room->room_flags & ROOM_SAFE && !ragnarok)
```

### Reduced Safety Counter

**Location:** [act_move.c:3293-3294](../../src/commands/act_move.c#L3293-L3294)

New avatars receive only 3 ticks of safety instead of 10 during ragnarok.

### Upgrade/Non-Upgrade Restrictions

**Location:** [fight.c:2279-2288](../../src/combat/fight.c#L2279-L2288)

During ragnarok, additional restrictions apply:
- Upgrades cannot attack non-upgrades
- Non-upgrades cannot attack upgrades

This prevents farming between different power tiers during the event.

## Deactivation

When `ragnarok_on_timer` expires:

1. `ragnarok = FALSE`
2. `ragnarok_safe_timer` set to 120 minutes cooldown
3. Global announcement broadcast

## Timer Display

**Location:** [jobo_act.c:1253-1286](../../src/systems/jobo_act.c#L1253-L1286)

The `timer` command shows:
- Current ragnarok cost remaining
- Time until ragnarok ends (if active)
- Time until ragnarok can be triggered again (if on cooldown)

## Timeline

| Phase | Duration |
|-------|----------|
| Bidding | Until 3000 QPs collected |
| Active | 15 minutes (`PULSE_RAGNAROK`) |
| Cooldown | 120 minutes |

## Command Registration

**Location:** [interp.c:1016](../../src/core/interp.c#L1016)
