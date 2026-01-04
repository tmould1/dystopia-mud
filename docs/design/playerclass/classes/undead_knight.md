# Undead Knight Class Design

## Overview

Undead Knights are fallen Paladins cursed to spend eternity earthbound. They command dark forces and study the arts of necromancy. This is an **upgrade class** obtained by upgrading from Vampire.

**Source Files**: `src/classes/undead_knight.c`, `src/classes/undead_knight.h`
**Class Constant**: `CLASS_UNDEAD_KNIGHT` (4096)
**Upgrades From**: Vampire

## Core Mechanics

### Power Paths

Undead Knights have four trainable power paths stored in `ch->pcdata->powers[]`:

| Index | Path | Max Level | Purpose |
|-------|------|-----------|---------|
| NECROMANCY (1) | Necromancy | 10 | Auras and death powers |
| INVOCATION (2) | Invocation | 5 | Powerword attacks |
| UNDEAD_SPIRIT (3) | Spirit | 10 | Toughness/resilience |
| WEAPONSKILL (4) | Weapon Skill | 10 | Combat proficiency |

**Training Cost**: (current_level * 60 + 60) primal per level

### Aura System

Auras are toggled via the `aura` command (`undead_knight.c:190`). Stored in `ch->pcdata->powers[AURAS]` as bitflags:

| Necromancy | Aura | Flag | Effect |
|------------|------|------|--------|
| 2 | Death | DEATH_AURA (2) | Extra attacks vs enemies (`fight.c:925`) |
| 4 | Might | MIGHT_AURA (8) | +300 hitroll, +300 damroll (`undead_knight.c:246`) |
| 6 | Bog | BOG_AURA (1) | 70% chance to prevent enemy flee (`fight.c:4166`) |
| 9 | Fear | FEAR_AURA (4) | -20 damroll, -20 hitroll debuff on attacker (`fight.c:930`) |

## Commands

### Combat Commands

| Command | Requirement | Effect | Code |
|---------|-------------|--------|------|
| `ride <target>` | - | Teleport to target on skeleton steed, 600 move (`undead_knight.c:34`) |
| `unholyrite` | - | Sacrifice 500 mana to heal 500-1000 HP (`undead_knight.c:167`) |

### Powerwords (INVOCATION)

Powerwords have a cooldown stored in `ch->pcdata->powers[POWER_TICK]`:

| Command | Invocation | Effect | Cooldown |
|---------|------------|--------|----------|
| `powerword blind` | 1 | Blinds target, removes truesight (`undead_knight.c:462`) | 3 ticks |
| `powerword kill` | 3 | 10% of target HP damage (max 1500 PC, 5000 NPC) (`undead_knight.c:502`) | 2 ticks |
| `powerword flames` | 4 | 2x fireball hits on all enemies in room (`undead_knight.c:557`) | 2 ticks |
| `powerword stun` | 5 | 24-round WAIT_STATE on target (`undead_knight.c:432`) | 4 ticks |

### Training Commands

| Command | Effect |
|---------|--------|
| `gain` | Display/improve Necromancy, Invocation, Spirit paths (`undead_knight.c:294`) |
| `weaponpractice` | Improve Weapon Skill (costs HP/mana/move set to 1) (`undead_knight.c:377`) |

### Equipment

| Command | Cost | Effect |
|---------|------|--------|
| `knightarmor <piece>` | 150 primal | Create unholy armor (vnums 29975-29988) (`undead_knight.c:110`) |

**Available pieces**: plate, ring, bracer, collar, helmet, leggings, boots, gauntlets, chains, cloak, belt, visor, longsword, shortsword

## Combat Flags

Aura flags stored in `ch->pcdata->powers[AURAS]`:

| Flag | Value | Effect |
|------|-------|--------|
| BOG_AURA | 1 | 70% chance to prevent enemy flee |
| DEATH_AURA | 2 | Extra deathaura attacks in combat |
| FEAR_AURA | 4 | -20 hit/damroll debuff on attackers |
| MIGHT_AURA | 8 | +300 hitroll, +300 damroll |

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_UNDEAD_KNIGHT bit |
| necromancy | ch->pcdata->powers[1] | Necromancy level (0-10) |
| invocation | ch->pcdata->powers[2] | Invocation level (0-5) |
| spirit | ch->pcdata->powers[3] | Spirit level (0-10) |
| weaponskill | ch->pcdata->powers[4] | Weapon Skill level (0-10) |
| auras | ch->pcdata->powers[5] | Active aura bitflags |
| power_tick | ch->pcdata->powers[6] | Powerword cooldown timer |
