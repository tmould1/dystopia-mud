# PvP Systems Overview

This document provides an overview of the Player versus Player (PK) systems in Dystopia MUD.

## Core Concept

PvP combat is enabled for "avatar" characters (trust level 3-12). The system includes multiple engagement modes, protections for new players, and rewards for successful kills.

## Key Macro

**CAN_PK(ch)** - Defined in [merc.h:3104](../../src/core/merc.h#L3104)
```c
#define CAN_PK(ch)   (get_trust(ch)>= 3 && get_trust(ch)<= 12)
```
Only avatars with trust 3-12 can engage in PvP combat.

## System Components

| System | Purpose | Key File |
|--------|---------|----------|
| [PK Flags](pk-flags.md) | Track kills, deaths, bounties | [merc.h](../../src/core/merc.h) |
| [Timers](timers.md) | Cooldowns between PK engagements | [fight.c](../../src/combat/fight.c) |
| [Arena](arena.md) | Organized group PvP events | [arena.c](../../src/combat/arena.c) |
| [Bounty](bounty.md) | Place QP bounties on players | [act_comm.c](../../src/commands/act_comm.c) |
| [Ragnarok](ragnarok.md) | Global PvP event mode | [jobo_act.c](../../src/systems/jobo_act.c) |
| [Protections](protections.md) | Safe rooms, peace effects, newbie protection | [fight.c](../../src/combat/fight.c) |

## PK Scoring

The PK ratio system rewards winning records and penalizes high death counts.

**Function:** `get_ratio()` in [jobo_util.c:300-311](../../src/systems/jobo_util.c#L300-L311)

**Formula:**
- If no kills or deaths: ratio = 0
- If kills > 0: `ratio = pkill * 100 * ((pkill^2) - (pdeath^2)) / ((pkill + pdeath)^2)`
- Otherwise: `ratio = 100 * ((pkill^2) - (pdeath^2)) / ((pkill + pdeath)^2)`

## Quick Reference

### Commands

| Command | Description | Source |
|---------|-------------|--------|
| `bounty <player> <amount>` | Place QP bounty | [act_comm.c:45-107](../../src/commands/act_comm.c#L45-L107) |
| `bountylist` | List players with bounties | [jobo_act.c:37-58](../../src/systems/jobo_act.c#L37-L58) |
| `ragnarok <amount>` | Bid QPs toward ragnarok event | [jobo_act.c:1200-1251](../../src/systems/jobo_act.c#L1200-L1251) |
| `timer` | Show PvP event countdowns | [jobo_act.c:1253-1286](../../src/systems/jobo_act.c#L1253-L1286) |
| `arenajoin` | Join active arena | [arena.c:124-183](../../src/combat/arena.c#L124-L183) |
| `arenastats` | View arena participants | [arena.c:35-71](../../src/combat/arena.c#L35-L71) |
| `resign` | Exit arena (records loss) | [arena.c:185-234](../../src/combat/arena.c#L185-L234) |

### Key Restrictions

- Both players must be avatars (trust 3-12)
- Cannot attack players in safe rooms (except during ragnarok)
- Cannot attack players with active fight timers
- Cannot attack players with safety counters (post-avatar protection)
- Cannot attack AFK or linkdead players
- Newbies (age < 2) cannot be decapitated
