# Shapeshifter Class Design

## Overview

Shapeshifters are powerful creatures able to take any form they wish. They are an **upgrade class** obtained by upgrading from Werewolf. Known as Malaugrym - ancient shapeshifting beings from the plane of shadows.

**Source Files**: `src/classes/shapeshifter.c`, `src/classes/shapeshifter.h`
**Class Constant**: `CLASS_SHAPESHIFTER` (512)
**Upgrades From**: Werewolf

## Core Mechanics

### Power Paths

Shapeshifters have 5 trainable paths stored in `ch->pcdata->powers[]`:

| Index | Path | Max Level | Purpose |
|-------|------|-----------|---------|
| TIGER_LEVEL (4) | Tiger | 5 | Fast/agile predator form |
| HYDRA_LEVEL (2) | Hydra | 5 | Multi-headed fire breather |
| FAERIE_LEVEL (3) | Faerie | 5 | Evasive magic-focused form |
| BULL_LEVEL (1) | Bull | 5 | Brutal trampling form |
| SHAPE_POWERS (7) | Shiftpowers | 5 | General shapeshifting |

**Training Cost**: (current_level * 80 + 80) primal per level (`shapeshifter.c:567-688`)

### Form System

Forms are toggled via the `shift` command (`shapeshifter.c:364-537`). Current form stored in `ch->pcdata->powers[SHAPE_FORM]`:

| Form   | Flag            | Damroll | Hitroll | AC   | Appearance         |
|--------|-----------------|---------|---------|------|--------------------|
| Tiger  | TIGER_FORM (8)  | +300    | +450    | -200 | "huge phase tiger" |
| Hydra  | HYDRA_FORM (2)  | +450    | +450    | -50  | "horrific hydra"   |
| Bull   | BULL_FORM (1)   | +400    | +400    | -50  | "black bull"       |
| Faerie | FAERIE_FORM (4) | +250    | +250    | -500 | "small pixie"      |

**Shifting Healing**: First 2 shifts per cycle heal 10% max HP (up to 5000) (`shapeshifter.c:402-406`)

**Shift Fatigue**: Each shift adds 10 to SHAPE_COUNTER. Cannot shift if counter > 35 (`shapeshifter.c:381-385`)

### Combat Bonuses

**Base damage multiplier**: 1.4x for all forms (`fight.c:1428`)

**Form-specific bonuses** (`fight.c:1429-1432`):
| Form | Damage Mult | Requires |
|------|-------------|----------|
| Tiger | 1.5x | Tiger Level 2+ |
| Hydra | 1.6x | Hydra Level 2+ |
| Bull | 1.7x | Bull Level 2+ |
| Faerie | 1.2x | Faerie Level 2+ |

**Damage cap bonuses** (`fight.c:1796-1801`):
| Form | Max Dam Bonus |
|------|---------------|
| Base | +800 |
| Tiger | +325 per level |
| Hydra | +350 per level |
| Bull | +300 per level |
| Faerie | +275 per level |

### Form Combat Attacks

Each form grants automatic extra attacks (`fight.c:752-777`):

**Tiger Form**: Claws (level 1+), Fangs (level 2+)
**Hydra Form**: Multiple fang attacks based on heads (2-5 based on level)
**Bull Form**: Headbutt (level 1+), Hooves (level 2+)
**Faerie Form**: Random magic attacks on NPCs

### Damage Reduction

Shapeshifters get base 60% damage reduction (`fight.c:1659`), plus form bonuses:

| Form | Reduction per Level |
|------|---------------------|
| Faerie | -9% per level |
| Tiger | -8% per level |
| Hydra | -10% per level |
| Bull | -12% per level |

## Beast Forms

### Tiger Form (TIGER_LEVEL)

| Level | Ability | Effect |
|-------|---------|--------|
| 1 | Razor Claws | Extra claw attacks in combat |
| 2 | Pointy Teeth | Extra fang attacks in combat |
| 3 | Stronger Paws | Enhanced damage |
| 4 | `roar` | 50% chance to make opponent flee (`shapeshifter.c:841-881`) |
| 5 | `phase` | Phase out of reality, 10-tick invuln counter (`shapeshifter.c:180-207`) |

### Hydra Form (HYDRA_LEVEL)

| Level | Ability | Effect |
|-------|---------|--------|
| 0 | Base | 1 fire breath attack |
| 1 | 2 Heads | 2 fire breath attacks (`shapeshifter.c:172`) |
| 2 | 3 Heads | 3 fire breath attacks (`shapeshifter.c:173`) |
| 3 | Stronger Limbs | Enhanced melee damage |
| 4 | 4 Heads | 4 fire breath attacks (`shapeshifter.c:174`) |
| 5 | 5 Heads | 5 fire breath attacks (`shapeshifter.c:175`) |

**Breath Command**: `breath` - Fire breath attack, requires Hydra form (`shapeshifter.c:148-178`)

### Faerie Form (FAERIE_LEVEL)

| Level | Ability | Effect |
|-------|---------|--------|
| 1 | Fast Dodge | Enhanced evasion in combat |
| 2 | Small Wings | Can cast spells in any form |
| 3 | Magic Control | Enhanced magical attacks |
| 4 | `faeriecurse` | Web + Curse on target (1000 mana, 500 move) (`shapeshifter.c:93-146`) |
| 5 | `fblink` | Vanish and backstrike (2500 mana) (`shapeshifter.c:697-734`) |

### Bull Form (BULL_LEVEL)

| Level | Ability | Effect |
|-------|---------|--------|
| 1 | Pointy Horns | Extra headbutt attacks |
| 2 | Strong Hooves | Extra hoof attacks |
| 3 | Iron Muscles | Enhanced toughness |
| 4 | `charge` | Headbutt + 2 hooves, 33% stun (2000 move) (`shapeshifter.c:884-928`) |
| 5 | `stomp` | 500 damage + 40% chance to sever arm (`shapeshifter.c:33-91`) |

## Shiftpowers Path

General shapeshifting abilities (`shapeshifter.c:664-688`):

| Level | Ability | Command | Effect |
|-------|---------|---------|--------|
| 1 | Truesight | (passive) | See through illusions |
| 2 | Mistwalk | `mistwalk <target>` | Teleport to target (250 move) (`shapeshifter.c:266-319`) |
| 3 | Hatform | `hatform` | Transform into a hat object (`shapeshifter.c:321-362`) |
| 4 | Mask | `mask` | Disguise as another player |
| 5 | Shapeshift | `shapeshift <name>` | Transform into anything (`shapeshifter.c:799-839`) |

## Support Commands

| Command | Effect |
|---------|--------|
| `formlearn` | Display/improve form levels (`shapeshifter.c:539-695`) |
| `shapearmor <piece>` | Create shapeshifter equipment (150 primal) (`shapeshifter.c:209-264`) |
| `camouflage <item> <short/name> <text>` | Rename/redescribe items (`shapeshifter.c:736-797`) |

## Shapeshifter Armor

Create equipment via `shapearmor <piece>` (`shapeshifter.c:209-264`):

**Cost**: 150 primal per piece

**Available pieces** (vnums 33160-33173): knife, kane, bands, necklace, ring, jacket, helmet, pants, boots, gloves, shirt, cloak, belt, visor

## Class Interactions

**Bonus damage against** (`fight.c:1366`):
- Undead Knight vs Shapeshifter: +20% damage

**Reduced damage against** (`fight.c:1634-1650`):
- vs Droid: Shapeshifter deals 85% damage
- vs Samurai: Shapeshifter deals 62.5% damage

## Regeneration

Uses werewolf_regen() with 2x multiplier (`update.c:1567`)

**Home rooms** (vnum 93310-93319): Additional 1x regen bonus (`update.c:1559-1562`)

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_SHAPESHIFTER bit |
| bull_level | ch->pcdata->powers[1] | Bull form level (0-5) |
| hydra_level | ch->pcdata->powers[2] | Hydra form level (0-5) |
| faerie_level | ch->pcdata->powers[3] | Faerie form level (0-5) |
| tiger_level | ch->pcdata->powers[4] | Tiger form level (0-5) |
| shape_form | ch->pcdata->powers[5] | Current form flag |
| shape_counter | ch->pcdata->powers[6] | Shift fatigue counter |
| shape_powers | ch->pcdata->powers[7] | Shiftpowers level (0-5) |
| phase_counter | ch->pcdata->powers[8] | Phase ability cooldown |
