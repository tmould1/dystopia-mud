# Stance System

This document details the combat stance system.

## Overview

Stances are combat styles that provide various bonuses. Each stance must be trained through combat to increase its level.

## Stance Data

**Data source:** `ch->stance[1-17]`

Stances are stored in an array with specific indices for each stance type.

## Basic Stances (Indices 1-5)

Available immediately with no prerequisites.

| Index | Stance | Category | Primary Benefit |
|-------|--------|----------|-----------------|
| 1 | Viper | Offensive | Extra attacks (50% chance +1) |
| 2 | Crane | Defensive | Parry bonus (+level * 0.25) |
| 3 | Crab | Defensive | Damage resistance |
| 4 | Mongoose | Defensive | Dodge bonus (+level * 0.25) |
| 5 | Bull | Offensive | Damage bonus |

## Advanced Stances (Indices 6-10)

**Location:** `do_stance()` in [kav_fight.c:616-651](../../src/combat/kav_fight.c#L616-L651)

Each advanced stance requires two basic stances at level 200.

| Index | Stance | Prerequisites | Category | Primary Benefit |
|-------|--------|---------------|----------|-----------------|
| 6 | Mantis | Crane 200, Viper 200 | Balanced | Parry bonus, extra attacks |
| 7 | Dragon | Bull 200, Crab 200 | Balanced | Damage bonus + resistance |
| 8 | Tiger | Bull 200, Viper 200 | Offensive | Damage bonus, extra attacks |
| 9 | Monkey | Crane 200, Mongoose 200 | Offensive | Percentage damage bonus |
| 10 | Swallow | Crab 200, Mongoose 200 | Defensive | Dodge bonus, resistance |

### Stance Prerequisite Tree

```
        Viper ----+---- Crane
          |    \  |  /    |
          |     Mantis    |
          |               |
        Tiger   Monkey    |
          |       |       |
          +---+---+       |
              |           |
        Bull -+- Crab     |
          |    \ |  /     |
          |    Dragon     |
          |               |
          +---Swallow-----+
                |
             Mongoose
```

## Wolf Stance (Index 11)

**Location:** [kav_fight.c:682-690](../../src/combat/kav_fight.c#L682-L690)

Special werewolf-only stance requiring all five advanced stances at 200 plus `DISC_WERE_WOLF > 4`.

| Prerequisite | Required Level |
|--------------|----------------|
| Tiger | 200 |
| Swallow | 200 |
| Mantis | 200 |
| Dragon | 200 |
| Monkey | 200 |
| DISC_WERE_WOLF | > 4 |

### Wolf Stance Benefits

Wolf stance is essentially the ultimate stance, combining multiple benefits:

| Benefit | Effect | Location |
|---------|--------|----------|
| Extra Attacks | +2 attacks (50% chance) | [fight.c:987](../../src/combat/fight.c#L987) |
| Damage Bonus | +dam * (level/100) when level > 100 | [fight.c:4344-4345](../../src/combat/fight.c#L4344-L4345) |
| Damcap Bonus | +250 to max damage | [fight.c:1873](../../src/combat/fight.c#L1873) |
| Bypass | Bypasses parry/dodge powers | [fight.c:1752](../../src/combat/fight.c#L1752) |

Wolf stance provides:
- **+2 extra attacks** (same as superstance SPEED, better than Tiger/Viper/Mantis which give +1)
- **Damage multiplier** (same formula as Bull/Dragon/Tiger)
- **+250 damcap** (same as Dragon, better than Bull/Tiger at +200)
- **Bypass ability** (ignores enemy parry/dodge stance powers)

## Super Stances (Indices 19-23)

**Location:** `do_setstance()` in [jobo_act.c:653-852](../../src/systems/jobo_act.c#L653-L852)

Super stances are customizable advanced stances with selectable powers.

### Prerequisites

**Location:** [jobo_act.c:675-680](../../src/systems/jobo_act.c#L675-L680)

To unlock super stances, you must first max (level 200) all five advanced normal stances:
- Tiger
- Swallow
- Monkey
- Mantis
- Dragon

Additionally, each super stance requires the previous one to be maxed:

**Location:** [jobo_act.c:693-697](../../src/systems/jobo_act.c#L693-L697)

| Stance | Index | Prerequisite | Power Limits |
|--------|-------|--------------|--------------|
| SS1 | 19 | 5 normal stances at 200 | 3 lesser |
| SS2 | 20 | SS1 at 200 | 4 lesser, 1 greater |
| SS3 | 21 | SS2 at 200 | 4 lesser, 2 greater |
| SS4 | 22 | SS3 at 200 | 4 lesser, 4 greater, 1 supreme |
| SS5 | 23 | SS4 at 200 | 4 lesser, 4 greater, 2 supreme |

### Superstance Powers

Powers are selected via `setstance` command and stored in `ch->stance[18]` as bit flags.

**Tiered Powers** (lesser/greater/supreme):

| Power | Lesser Cost | Greater Cost | Supreme Cost |
|-------|-------------|--------------|--------------|
| DAMAGE | 20M exp | 40M exp | 60M exp |
| RESIST | 20M exp | 40M exp | 60M exp |
| DAMCAP | 20M exp | 40M exp | 60M exp |
| REV_DAMCAP | 20M exp | 40M exp | 60M exp |

**Flat Powers** (cost escalates: 20M, 40M, 60M, 80M):

| Power | Effect |
|-------|--------|
| SPEED | Increase fighting speed |
| PARRY | Increase parry ability |
| DODGE | Increase dodge ability |
| BYPASS | Bypass opponent's parry/dodge powers |

### Minimum/Maximum Costs

| Stance | Min Cost | Max Cost |
|--------|----------|----------|
| SS1 | 40M exp | 240M exp |
| SS2 | 80M exp | 280M exp |
| SS3 | 120M exp | 300M exp |
| SS4 | 160M exp | 360M exp |
| SS5 | 200M exp | 380M exp |

## Stance Effects in Combat

### Offensive Bonuses (Damage)

**Location:** Damage calculation in [fight.c](../../src/combat/fight.c)

**Level 100+ requirement for multipliers.**

| Stance | Effect vs NPC | Effect vs PC |
|--------|---------------|--------------|
| Monkey | +25% * (level+1)/200 | +25% * (level+1)/200 |
| Bull | +dam * (level/100) | +dam * (level/100) |
| Dragon | +dam * (level/100) | +dam * (level/100) |
| Wolf | +dam * (level/100) | +dam * (level/100) |
| Tiger | +dam * (level/100) | +dam * (level/100) |

### Superstance Damage Powers

| Power | vs NPC Bonus | vs PC Bonus |
|-------|--------------|-------------|
| STANCEPOWER_DAMAGE_1 | +1x damage | +0.17x damage |
| STANCEPOWER_DAMAGE_2 | +2x damage | +0.33x damage |
| STANCEPOWER_DAMAGE_3 | +1.5x damage | +0.5x damage |

### Defensive Bonuses (Damage Reduction)

**Level 100+ requirement.**

| Stance | Resistance |
|--------|------------|
| Crab | dam / (level/100) |
| Dragon | dam / (level/100) |
| Swallow | dam / (level/100) |

### Superstance Resistance Powers

| Power | Damage Multiplier |
|-------|-------------------|
| STANCEPOWER_RESIST_1 | 0.9x |
| STANCEPOWER_RESIST_2 | 0.8x |
| STANCEPOWER_RESIST_3 | 0.7x |

### Parry Bonuses

**Location:** [fight.c:2328-2585](../../src/combat/fight.c#L2328-L2585)

| Stance | Parry Bonus |
|--------|-------------|
| Crane | +stance_level * 0.25 |
| Mantis | +stance_level * 0.25 |
| Superstance PARRY | +stance_level * 0.25 |

### Dodge Bonuses

**Location:** [fight.c:2601-2790](../../src/combat/fight.c#L2601-L2790)

| Stance | Dodge Bonus |
|--------|-------------|
| Mongoose | +stance_level * 0.25 |
| Swallow | +stance_level * 0.25 |
| Superstance DODGE | +stance_level * 0.25 |

### Extra Attacks

**Location:** [fight.c:966-1070](../../src/combat/fight.c#L966-L1070) - `number_attacks()`

| Stance | Attack Bonus |
|--------|--------------|
| Viper | +1 (50% chance) |
| Mantis | +1 (50% chance) |
| Tiger | +1 (50% chance) |
| Wolf | +2 (50% chance) |
| Superstance | +2 |

### AC Penalties (Applied to Victim)

When attacker has superstance damage powers:

| Power | Victim AC Penalty |
|-------|-------------------|
| STANCEPOWER_DAMAGE_1 | +100 |
| STANCEPOWER_DAMAGE_2 | +200 |
| STANCEPOWER_DAMAGE_3 | +300 |

## Special Moves (Level 200+)

**Location:** [kav_fight.c:2354-2549](../../src/combat/kav_fight.c#L2354-L2549)

When stance level reaches 200+, there's a 5% chance per combat round to trigger a special finishing move.

### Available Special Moves

| Move | Effect |
|------|--------|
| Stomach punch | Damage + victim stunned |
| Leg sweep | Damage + 25% broken leg + stunned |
| Back elbow combo | Flip + elbow strike + stunned |
| Somersault back kick | 25% broken spine + stunned |
| Headbutt combo | 25% nose/jaw/neck damage + hurl + stunned |
| Double knee combo | 25% + 25% damage + stunned |
| Elbow strike combo | Multiple hits + stunned |

### Special Move Damage

```c
dam = number_range(5, 10) + char_damroll(ch)
```

All special moves also stun the victim.

## Stance Training

### improve_stance()

Called after successful attacks in combat. Stance experience increases based on:
- Combat activity
- Current stance level
- Opponent difficulty

### Level Thresholds

| Level | Effect Unlocked |
|-------|-----------------|
| 1+ | Basic stance active |
| 100+ | Damage/resistance multipliers |
| 200+ | Special finishing moves (5% chance) |

## Stance Selection Strategy

### Offensive Focus
- **Monkey:** Percentage-based damage, scales well
- **Bull/Tiger:** Raw damage multiplier + extra attacks
- **Viper/Mantis:** Extra attacks for more hits

### Defensive Focus
- **Crane:** Parry-focused defense
- **Mongoose:** Dodge-focused defense
- **Crab/Swallow:** Damage reduction

### Balanced
- **Dragon:** Both offense and defense bonuses
- **Swallow:** Dodge + resistance

## Related Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `improve_stance()` | [fight.c](../../src/combat/fight.c) | Train stance in combat |
| `do_level()` | [clan.c:34-199](../../src/classes/clan.c#L34-L199) | Display stance levels |
| `do_setstance()` | [jobo_act.c:653-852](../../src/systems/jobo_act.c#L653-L852) | Configure super stance powers |
