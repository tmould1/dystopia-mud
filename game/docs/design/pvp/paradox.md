# Paradox System

This document details the paradox anti-griefing system for unfair PK.

## Overview

Paradox is a punishment mechanic that triggers when players repeatedly kill opponents in unfair fights (where `fair_fight()` returns FALSE). It prevents stronger players from farming weaker ones.

## Data Storage

**Location:** [merc.h](../../src/core/merc.h)

| Field | Type | Line | Purpose |
|-------|------|------|---------|
| `mean_paradox_counter` | sh_int | [2415](../../src/core/merc.h#L2415) | Current paradox points (in PC_DATA) |
| `paradox[3]` | int | [2360](../../src/core/merc.h#L2360) | Total/Current/Ticker (in CHAR_DATA) |

### Paradox Array

```c
paradox[0] = Total paradox (lifetime)
paradox[1] = Current paradox
paradox[2] = Paradox ticker
```

## How It Works

### Triggering Paradox Counter

**Location:** [fight.c:4919-4936](../../src/combat/fight.c#L4919-L4936)

When a player kills another player and `fair_fight()` returns FALSE:

1. The kill still happens (victim is beheaded)
2. Attacker's `mean_paradox_counter` is incremented by 1
3. Attacker gains 60-120 bounty (more than fair kills which give 30-80)
4. Death message shows "(Paradox Counter)" tag

```c
if (!can_decap)
{
    behead(victim);
    ch->pcdata->mean_paradox_counter++;
    ch->pcdata->bounty += number_range(60, 120);
    // Death info shows "Paradox Counter" tag
}
```

### Paradox Punishment

**Location:** [fight.c:4931-4935](../../src/combat/fight.c#L4931-L4935)

When `mean_paradox_counter` exceeds 2 (3+ unfair kills):

```c
if (ch->pcdata->mean_paradox_counter > 2)
{
    ch->pcdata->mean_paradox_counter = 0;
    do_paradox(ch, "self");
}
```

### Reducing Paradox Counter

**Location:** [fight.c:4980](../../src/combat/fight.c#L4980)

Fair kills reduce the paradox counter:

```c
if (ch->pcdata->mean_paradox_counter > 0)
    ch->pcdata->mean_paradox_counter--;
```

## paradox() Function

**Location:** [act_wiz.c:1231-1249](../../src/commands/act_wiz.c#L1231-L1249)

When paradox triggers, the following happens:

1. Messages sent to player and broadcast globally
2. HP set to -10 (dying state)
3. Max HP reduced by 10%: `max_hit = (max_hit * 90) / 100`
4. Position updated (to dying)
5. Forced to escape combat
6. Set TIED_UP, GAGGED, and BLINDFOLDED flags

```c
void paradox(CHAR_DATA *ch)
{
    send_to_char("The sins of your past strike back!\n\r", ch);
    send_to_char("The paradox has come for your soul!\n\r", ch);
    // Global broadcast
    ch->hit = -10;
    ch->max_hit = (ch->max_hit * 90) / 100;
    update_pos(ch);
    do_escape(ch, "");
    SET_BIT(ch->extra, TIED_UP);
    SET_BIT(ch->extra, BLINDFOLDED);
    SET_BIT(ch->extra, GAGGED);
}
```

## Paradox Counter Display

**Location:** [act_info.c:2516-2518](../../src/commands/act_info.c#L2516-L2518)

Players can see their current paradox counter in their score/info:

```c
if (ch->pcdata->mean_paradox_counter > 0) {
    sprintf(buf, "[You have %d point(s) on your paradox counter]\n\r",
            ch->pcdata->mean_paradox_counter);
}
```

## Fair Fight Check

The `fair_fight()` function (see [protections.md](protections.md)) determines if a fight is fair based on:
- Both players having minimum 150 might
- Power ratio checks based on attacker's might level

When `fair_fight()` returns FALSE, kills increment the paradox counter instead of being normal PK.

## Admin Command

**Location:** [act_wiz.c:1209-1229](../../src/commands/act_wiz.c#L1209-L1229)

Immortals (level 12+) can manually trigger paradox on players:

```
paradox <player>
```

## Summary

| Counter | Trigger | Effect |
|---------|---------|--------|
| 1 | First unfair kill | Counter incremented, bounty +60-120 |
| 2 | Second unfair kill | Counter incremented, bounty +60-120 |
| 3+ | Third unfair kill | **PARADOX TRIGGERS** |

| Paradox Effect | Description |
|----------------|-------------|
| HP | Set to -10 (dying) |
| Max HP | Permanently reduced by 10% |
| Status | Tied up, gagged, blindfolded |
| Combat | Forced to escape |

The system discourages power imbalance griefing while still allowing the kills to happen - but with escalating consequences.
