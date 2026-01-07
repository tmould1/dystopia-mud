# Stance System

This document details the combat stance system.

## Overview

Stances are combat styles that provide various bonuses. Each stance must be trained through combat to increase its level.

## Stance Data

**Data source:** `ch->stance[1-17]`

Stances are stored in an array with specific indices for each stance type.

## Normal Stances (Indices 1-10)

| Index | Stance | Category | Primary Benefit |
|-------|--------|----------|-----------------|
| 1 | Viper | Offensive | Extra attacks (50% chance +1) |
| 2 | Crane | Defensive | Parry bonus (+level * 0.25) |
| 3 | Crab | Defensive | Damage resistance |
| 4 | Mongoose | Defensive | Dodge bonus (+level * 0.25) |
| 5 | Bull | Offensive | Damage bonus |
| 6 | Mantis | Balanced | Parry bonus, extra attacks |
| 7 | Dragon | Balanced | Damage bonus + resistance |
| 8 | Tiger | Offensive | Damage bonus, extra attacks |
| 9 | Monkey | Offensive | Percentage damage bonus |
| 10 | Swallow | Defensive | Dodge bonus, resistance |

## Super Stances (Indices 13-17)

| Index | Stance | Power Level |
|-------|--------|-------------|
| 13 | SS1 | Basic super stance |
| 14 | SS2 | Enhanced |
| 15 | SS3 | Advanced |
| 16 | SS4 | Expert |
| 17 | SS5 | Master |

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
| `do_level()` | [clan.c:34-199](../../src/systems/clan.c#L34-L199) | Display stance levels |
