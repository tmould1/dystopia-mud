# Combat System Overview

This document provides an overview of the combat systems in Dystopia MUD.

## Core Concept

Combat is resolved through a THAC0-based system with weapon skills, stances, and class-specific modifiers affecting damage and hit chance.

## Main Combat Loop

**Location:** [fight.c:60-254](../../src/combat/fight.c#L60-L254) - `violence_update()`

Called each game tick to process all active fights. For each character in combat:
1. Calls `multi_hit()` to execute attacks
2. Processes fight timers
3. Handles combat state changes

## System Components

| System | Purpose | Key File |
|--------|---------|----------|
| [do_level Command](do-level.md) | Display weapon/stance/spell levels | [clan.c](../../src/systems/clan.c) |
| [Damage Calculation](damage-calculation.md) | How damage is computed | [fight.c](../../src/combat/fight.c) |
| [Defense Mechanics](defense.md) | Parry, dodge, armor class | [fight.c](../../src/combat/fight.c) |
| [Stance System](stances.md) | Combat stances and bonuses | [fight.c](../../src/combat/fight.c) |
| [Attack System](attacks.md) | Multi-hit and attack counts | [fight.c](../../src/combat/fight.c) |

## Key Character Stats

### Combat Attributes

| Stat | Source | Purpose |
|------|--------|---------|
| `hitroll` | Items/buffs | Bonus to hit chance |
| `damroll` | Items/buffs | Bonus to damage |
| `wpn[0-12]` | Training | Weapon type proficiencies |
| `stance[1-17]` | Training | Stance proficiencies |
| `spl[0-4]` | Training | Spell school levels |

### Derived Stats

| Function | Location | Purpose |
|----------|----------|---------|
| `char_hitroll()` | [act_info.c:425-490](../../src/commands/act_info.c#L425-L490) | Total to-hit bonus |
| `char_damroll()` | [act_info.c:491-561](../../src/commands/act_info.c#L491-L561) | Total damage bonus |
| `char_ac()` | [act_info.c:562+](../../src/commands/act_info.c#L562) | Total armor class |

## Combat Flow

```
violence_update() [fight.c:60]
    └── multi_hit() [fight.c:259-650]
            ├── Select weapon(s) from equipped slots
            ├── Determine number of attacks
            ├── For each attack:
            │       └── one_hit() [fight.c:1156-1500+]
            │               ├── Calculate THAC0
            │               ├── Roll to hit
            │               ├── check_parry() [fight.c:2328]
            │               ├── check_dodge() [fight.c:2601]
            │               ├── Calculate damage
            │               └── damage() → dam_message()
            └── improve_wpn() / improve_stance()
```

## Key Constants

**Location:** [merc.h](../../src/core/merc.h)

| Constant | Value | Purpose |
|----------|-------|---------|
| `TYPE_HIT` | 1000 | Base attack type offset |
| `SKILL_THAC0_00` | 6 | THAC0 at level 0 |
| `SKILL_THAC0_32` | 18 | THAC0 at level 32+ |

## Weapon Types

Indexed 0-12, corresponding to `TYPE_HIT + index`:

| Index | Type | Attack Message |
|-------|------|----------------|
| 0 | Unarmed | hit |
| 1 | Slice | slice |
| 2 | Stab | stab |
| 3 | Slash | slash |
| 4 | Whip | whip |
| 5 | Claw | claw |
| 6 | Blast | blast |
| 7 | Pound | pound |
| 8 | Crush | crush |
| 9 | Grep | grep |
| 10 | Bite | bite |
| 11 | Pierce | pierce |
| 12 | Suck | suck |

## PvP Adjustments

- All damage halved in PC vs PC combat
- Fight timer prevents re-engagement (see [PvP docs](../pvp/timers.md))
- Fair fight system prevents power imbalance decapitations
