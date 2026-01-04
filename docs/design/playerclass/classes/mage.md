# Mage Class Design

## Overview

Mages are spellcasters who harness the five colors of magic (spheres). They use mana as their primary resource and an invoke system for shields and special powers.

**Source Files**: `src/classes/mage.c`, `src/classes/mage.h`
**Class Constant**: `CLASS_MAGE` (2)

## Core Mechanics

### Mana Pool

Mana is the resource for casting spells:

```c
ch->mana        // Current mana
ch->max_mana    // Maximum mana pool
```

### Sphere/Color Magic System

Mages use five spheres of magic stored in `ch->spl[]` (`merc.h:2751-2755`):

| Index | Sphere | Theme |
|-------|--------|-------|
| 0 | PURPLE_MAGIC | Underdark/Dark Magic |
| 1 | RED_MAGIC | Fire/Hell Magic |
| 2 | BLUE_MAGIC | Storm/Lightning Magic |
| 3 | GREEN_MAGIC | Nature/Earth Magic |
| 4 | YELLOW_MAGIC | Acid/Poison Magic |

Each sphere level contributes to total magic_power for spell calculations.

### Invoke System (PINVOKE)

The invoke system (`mage.c:528-757`) manages learned powers via `ch->pcdata->powers[PINVOKE]` (0-10 progression):

| Level | Primal Cost | Unlocks |
|-------|-------------|---------|
| 1 | 20 | Teleport |
| 2 | 40 | Mageshield (25 primal to activate) |
| 3 | 60 | Scry |
| 4 | 80 | Discharge |
| 5 | 100 | Deflector (5 primal) |
| 6 | 120 | Steelshield (5 primal) |
| 7 | 140 | Deeper understanding |
| 8 | 160 | Illusions (5 primal) |
| 9 | 180 | Beast (10 primal) |
| 10 | 200 | Master - invoke all at once |

## Commands

### Core Commands

| Command | Requirements | Effect |
|---------|--------------|--------|
| `magics` | - | Display active shields/invokes (`mage.c:762`) |
| `invoke` | - | Learn/activate magical powers (`mage.c:528`) |
| `chant` | POS_MEDITATING | Cast multi-sphere spells (`mage.c:200`) |
| `magearmor` | 60 primal | Create enchanted armor (`mage.c:78`) |
| `teleport` | PINVOKE >= 1, 250 mana | Teleport to target (`mage.c:845`) |
| `scry` | PINVOKE >= 3 | Remote viewing (`clan.c:2797`) |
| `discharge` | PINVOKE >= 4, mageshield active | AoE damage blast (`mage.c:786`) |
| `reveal` | 5000 mana | Expose all hidden creatures (`mage.c:33`) |
| `chaosmagic` | 1500 mana, RED_MAGIC level | Random spell casting (`mage.c:138`) |
| `objectgate` | - | Summon objects from elsewhere (`mage.c:903`) |

### Invoke Subcommands

| Command | PINVOKE Req | Primal Cost | Effect |
|---------|-------------|-------------|--------|
| `invoke learn` | - | Variable | Advance to next level |
| `invoke mageshield` | 2 | 25 | Mystic shield layer |
| `invoke deflector` | 5 | 5 | Dodge/parry bonus |
| `invoke steelshield` | 6 | 5 | Hardened skin defense |
| `invoke illusions` | 8 | 5 | Summon mirror copies |
| `invoke beast` | 9 | 10 | Beast form attack boost |
| `invoke all` | 9 | 50 total | Activate all shields |

## Chant System

The chant command (`mage.c:200-522`) combines all 5 spheres for powerful effects:

### Chant Heal (1500 mana)
- Healing: magic_power * 1.5 HP
- magic_power = sum of all 5 sphere levels
- +100 if ITEMA_AFFENTROPY active

### Chant Damage (1000 mana)
- 5 sequential elemental attacks (one per sphere)
- Each deals 4-5x sphere level + damroll damage
- Player damage capped at 1000 per hit vs non-NPCs

### Chant Bless (2500 mana)
- Applies up to 5 buffs (one per sphere not already affected)
- Per sphere: +HIT (level*2), -AC (level/2), +MOVE (level*2), +MANA (level*2), +HITROLL (level/3), +DAMROLL (level/3)
- Duration: sphere_level / 4 ticks

### Chant Curse (2500 mana)
- Same as bless but negative effects on target
- Target must be level >= 3

## Chaos Magic

Available to Mages and Liches with CHAOS_MAGIC power >= 1 (`mage.c:138-195`):

- **Mana Cost**: 1500
- **Power Level**: ch->spl[RED_MAGIC] / 4

Casts one random spell from:
1. spirit kiss
2. desanct
3. imp heal
4. imp fireball
5. imp faerie fire
6. imp teleport
7. change sex
8. shield
9. readaura
10. earthquake
11. gate
12. dispel magic

## Shield Item Affections

Defined in `merc.h:992-998`:

| Shield | Constant | Primal | Effect |
|--------|----------|--------|--------|
| Mageshield | ITEMA_MAGESHIELD | 25 | Primary defense layer |
| Steelshield | ITEMA_STEELSHIELD | 5 | Toughness/AC improvement |
| Deflector | ITEMA_DEFLECTOR | 5 | Dodge/parry bonus |
| Illusions | ITEMA_ILLUSIONS | 5 | Attack evasion copies |
| Beast | ITEMA_BEAST | 10 | Extra attacks/damage |
| Affentropy | ITEMA_AFFENTROPY | - | +100 magic power |

## Reveal Command

Exposes all hidden creatures in room (`mage.c:33-76`):

- **Cost**: 5000 mana
- **Removes from all non-immortals**:
  - invis, mass invis, sneak, hide
  - PLR_WIZINVIS, shift, earthmeld
  - shadowplane, peace, ethereal
  - drow darkness vision

## Mage Armor

Creates enchanted equipment (`mage.c:78-136`):

- **Cost**: 60 primal per piece
- **Available Pieces** (vnums 33000-33013):
  - Staff, Dagger, Ring, Collar, Robe, Cap
  - Leggings, Boots, Gloves, Sleeves, Cape, Belt, Bracer, Mask

## Regeneration

Special mage update mechanics (`update.c:1790-1804`):

**Special Zone** (vnum 93400-93406):
- Triggers werewolf_regen(ch, 1) for enhanced HP/mana/move

**Meditation Regen** (POS_MEDITATING):
- +1000-2000 mana per tick
- Capped at max_mana

## Teleport

Travel to target location (`mage.c:845-898`):

- **Requirements**: PINVOKE >= 1
- **Cost**: 250 mana
- **Restrictions**:
  - Cannot teleport to/from astral plane
  - Target cannot have IMM_TRAVEL immunity

## Discharge

Area damage blast from active mageshield (`mage.c:786-839`):

- **Requirements**: PINVOKE >= 4, ITEMA_MAGESHIELD active
- **Effect**: Removes mageshield, damages all non-safe creatures in room
- **Damage**: 3.5-4.5x total magic_power + damroll
- Reduced 50% vs sanctuary
- Does not damage mounts

## Objectgate

Summon objects from elsewhere (`mage.c:903-965`):

- **Requirements**: Mage or Lich (with CON_LORE >= 1)
- Object must be in a room (not carried/contained)
- Object must have ITEM_TAKE flag
- Cannot gate artifacts or relics
- Neither room can be astral

## Weaknesses

**Verified in code**:
- Mana-dependent (cannot cast without mana)
- Low physical combat ability
- Shield-dependent for defense

**Note**: Anti-magic, silence, and mana drain mechanics are NOT specifically implemented for mages in the current codebase.

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_MAGE bit |
| mana | ch->mana | Current mana |
| max_mana | ch->max_mana | Maximum mana |
| spl[] | ch->spl[0-4] | Sphere levels |
| pinvoke | ch->pcdata->powers[PINVOKE] | Invoke progression (0-10) |
| itemaffect | ch->itemaffect | Shield flags (ITEMA_*) |
