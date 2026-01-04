# Samurai Class Design

## Overview

Samurai are ancient warriors and masters of armed combat. They are an **upgrade class** obtained by upgrading from Ninja.

**Source Files**: `src/classes/samurai.c`, `src/classes/samurai.h`
**Class Constant**: `CLASS_SAMURAI` (16)
**Upgrades From**: Ninja

## Core Mechanics

### Focus System

Focus is the core mechanic for Samurai combat, stored in `ch->pcdata->powers[SAMURAI_FOCUS]`.

**Building Focus**: Each martial art move adds focus points:

| Move | Focus Added | Code |
|------|-------------|------|
| `slide` | +1 | `samurai.c:211` |
| `sidestep` | +2 | `samurai.c:246` |
| `block` | +4 | `samurai.c:281` |
| `countermove` | +8 | `samurai.c:316` |

**Focus Management**:
- Cannot use moves when focus > 40 (exhaustion)
- Use `focus` command to reduce focus (randomly reduces 1 to current focus) (`samurai.c:167-184`)

### Combo System

When focus hits specific thresholds, automatic combo attacks trigger (`samurai.c:326-417`):

| Focus | Effect | Requirements |
|-------|--------|--------------|
| 10 | 3x lightningslash attacks + taunt | Cannot repeat same combo |
| 15 | Disarm opponent (67% success) | Cannot repeat same combo |
| 20 | Paralyze target (24 WAIT_STATE) | Cannot repeat same combo |
| 25 | Hurl opponent (67% success) | Cannot repeat same combo |
| 30 | Self-heal 2000-4000 HP | Cannot repeat same combo |
| 35 | 5-hit combo (backfist + thrustkick + monksweep + jumpkick + lightningslash) | Cannot repeat same combo |

**Last Combo Tracking**: Stored in `ch->pcdata->powers[SAMURAI_LAST]` - prevents using same combo twice in a row.

### Martial Arts

Learn martial art moves via `martial <move>` command (`samurai.c:419-512`).

**Cost**: 150 million exp per move

| Move | Flag | Cost |
|------|------|------|
| `slide` | SAM_SLIDE (1) | 150M exp |
| `sidestep` | SAM_SIDESTEP (2) | 150M exp |
| `block` | SAM_BLOCK (4) | 150M exp |
| `countermove` | SAM_COUNTERMOVE (8) | 150M exp |

Stored as bitflags in `ch->pcdata->powers[SAMURAI_MARTIAL]`.

### Combat Bonuses

**Attack Count** (`fight.c:1064`, `fight.c:1127`):
- +5 attacks per round baseline (twice)

**Extra Attacks** (`fight.c:836-837`):
- NEW_BLADESPIN: Extra bladespin multi_hit attack per round

**Damage Cap Bonus** (`fight.c:1859-1867`):
- Sum of all weapon proficiencies / 200 (when wielding weapon)

**Damage Reduction** (`fight.c:1647-1650`):
- vs Shapeshifter: Takes 62.5% damage (1/1.6)

**Deals Reduced Damage** (`fight.c:1635`, `fight.c:1713`):
- vs Spider Droid: Deals 90% damage
- vs Undead Knight: Deals 70% damage

**Dodge/Parry Bonus** (`fight.c:2431`, `fight.c:2460`, `fight.c:2544-2545`, `fight.c:2657`, `fight.c:2686`, `fight.c:2731-2732`):
- -25% enemy hit chance (dodge)
- +25% dodge as defender
- -15% enemy hit chance (parry)
- +15% parry as defender

**Charge Resistance** (`fight.c:421`):
- Angel charge: Lower success (3-5 damage instead of 2-5)

## Commands

### Combat Commands

| Command | Requirement | Effect | Code |
|---------|-------------|--------|------|
| `slide` | SAM_SLIDE | +1 focus, lightningslash, check combo | `samurai.c:186-219` |
| `sidestep` | SAM_SIDESTEP | +2 focus, lightningslash, check combo | `samurai.c:221-254` |
| `block` | SAM_BLOCK | +4 focus, lightningslash, check combo | `samurai.c:256-289` |
| `countermove` | SAM_COUNTERMOVE | +8 focus, lightningslash, check combo | `samurai.c:291-324` |
| `focus` | SAMURAI_FOCUS >= 10 | Reduce focus counter | `samurai.c:167-184` |
| `bladespin` | wpn[3], wpn[0], wpn[1] >= 1000 | Toggle bladespin mode | `samurai.c:60-88` |

### Bladespin Details (`samurai.c:60-88`)

Requirements: Weapon skills (slash, hit, pierce) all >= 1000
Toggle: NEW_BLADESPIN bit in ch->newbits
Effect: Extra bladespin multi_hit attack each combat round

### Movement Commands

| Command | Effect | Code |
|---------|--------|------|
| `ancestralpath <target>` | Teleport to target (1000 move) | `samurai.c:90-165` |

### Equipment

| Command | Effect | Code |
|---------|--------|------|
| `katana` | Create katana weapon (250 primal) | `samurai.c:31-58` |
| `martial <move>` | Learn martial art moves (150M exp) | `samurai.c:419-512` |

## Samurai Katana

Create weapon via `katana` command (`samurai.c:31-58`):

**Cost**: 250 primal

**Stats** (vnum 33176):
- Damage: 65d115
- QUEST_RELIC flag
- Condition/Toughness: 100
- Resistance: 1

## Regeneration

No inherent regen system. No update_samurai function exists.

Relies on ITEMA_REGENERATE artifacts or other sources.

## Class Interactions

**Takes extra damage from** (`fight.c:421`):
- Angel charge: Random 3-5 damage (vs 2-5 for others)

**Takes reduced damage from**:
- None specific

**Deals reduced damage to** (`fight.c:1635`, `fight.c:1713`):
- Spider Droid: 90%
- Undead Knight: 70%

**Bonus against** (`fight.c:1647-1650`):
- vs Shapeshifter: Takes only 62.5% damage

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_SAMURAI bit |
| focus | ch->pcdata->powers[1] | Current focus counter |
| last_combo | ch->pcdata->powers[2] | Last combo used (prevents repeats) |
| martial | ch->pcdata->powers[3] | Learned martial arts bitflags |
| bladespin | ch->newbits | NEW_BLADESPIN toggle |
