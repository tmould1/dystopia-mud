# Spider Droid Class Design

## Overview

Spider Droids are the ultimate champions of Lloth - cybernetic driders with mechanical body parts. They are an **upgrade class** obtained by upgrading from Drow.

**Source Files**: `src/classes/spiderdroid.c`, `src/classes/spiderdroid.h`
**Class Constant**: `CLASS_DROID` (8192)
**Upgrades From**: Drow

## Core Mechanics

### Implant System

Implants are upgraded via the `implant` command (`spiderdroid.c:33-186`). Three areas stored in `ch->pcdata->powers[]`:

| Index | Area | Max Level | Purpose |
|-------|------|-----------|---------|
| CYBORG_FACE (0) | Face | 5 | Vision enhancements |
| CYBORG_LIMBS (1) | Legs | 5 | Combat speed/skill |
| CYBORG_BODY (2) | Body | 6 | Armor, stingers, regen |

**Improvement Cost** (in drider power points):
- Level 1: 12,500
- Level 2: 25,000
- Level 3: 50,000
- Level 4: 100,000
- Level 5: 200,000
- Level 6: 400,000

**Prerequisites**:
- Body implants require at least 1 leg implant
- Face implant 5 requires body implant 4

### Combat Bonuses

**Attack Count** (`fight.c:1008`, `fight.c:1077`):
- +CYBORG_LIMBS attacks per round

**Damage Multiplier** (`fight.c:1435-1441`):
- CYBORG_LIMBS 1+: 1.3x damage
- CYBORG_LIMBS 2+: Additional 1.25x (1.625x total)
- CYBORG_LIMBS 3+: Additional 1.25x (2.03x total)

**Damage Cap Bonus** (`fight.c:1817-1822`):
- CYBORG_LIMBS 1+: +450 max damage
- Additional: +50 * CYBORG_BODY

**Damage Reduction** (`fight.c:1632-1641`):
- CYBORG_BODY 1+: 42.9% reduction (divides by 1.75)
- vs Shapeshifter: 85% damage dealt
- vs Samurai: 90% damage dealt

**Dodge/Parry Bonus** (`fight.c:2403`, `fight.c:2436`, `fight.c:2546-2547`, `fight.c:2630`, `fight.c:2661`, `fight.c:2693-2694`):
- -10% enemy hit chance per CYBORG_LIMBS level (dodge)
- -4% per CYBORG_LIMBS level (parry)

## Face Implants (CYBORG_FACE)

| Level | Ability | Effect |
|-------|---------|--------|
| 1 | Nightsight | Can see in the dark (`spiderdroid.c:328-339`) |
| 2 | Shadowsight | See through shadows (`spiderdroid.c:313-327`) |
| 3+ | Holylight | Full truesight (PLR_HOLYLIGHT) (`spiderdroid.c:302-312`) |
| 4 | Aura Shield | Your aura is shielded |
| 5 | Sensor | See stats and location of others |

**Command**: `llothsight` - Toggle vision abilities

## Leg Implants (CYBORG_LIMBS)

| Level | Ability | Effect |
|-------|---------|--------|
| 1 | Mechanical Legs | +1 attack, +10% dodge, 1.3x damage |
| 2 | Superior Speed | +1 attack, additional speed bonus |
| 3 | Champion | Enhanced fighting skills |
| 4 | True Warrior | Supreme fighting skills |
| 5 | Poison Spit | Unlocks `venomspit` command |

## Body Implants (CYBORG_BODY)

| Level | Ability | Effect |
|-------|---------|--------|
| 1 | Armor | +50 max damage cap |
| 2 | Absorption | Can absorb certain attacks |
| 3 | Body Slam | Extra attack when initiating combat (`fight.c:875-878`) |
| 4 | Cloak | Invisibility ability (enables darkness) |
| 5 | Stingers | Unlocks `venomspit`, enables `avataroflloth` |
| 6 | Regeneration | 2x werewolf_regen per tick (`update.c:530`) |

## Commands

### Combat Commands

| Command | Requirement | Effect | Code |
|---------|-------------|--------|------|
| `venomspit` | CYBORG_BODY 5, CYBORG_LIMBS 5 | 3x stuntube attack + poison + fire (1000 move) | `spiderdroid.c:188-227` |
| `avataroflloth` | CYBORG_BODY 5 | Toggle +250 hit/damroll (2000 mana/move) | `spiderdroid.c:229-282` |

### Venomspit Details (`spiderdroid.c:188-227`)
- 3 stuntube attacks
- Applies AFF_POISON
- Applies AFF_FLAMING
- Costs 1000 move

### Avatar of Lloth Details (`spiderdroid.c:229-282`)
- Toggle form transformation
- +250 hitroll, +250 damroll
- Activation cost: 2000 mana, 2000 move
- While active in cubeform: Extra stuntube attack per round (`fight.c:875-878`)

### Vision Commands

| Command | Requirement | Effect | Code |
|---------|-------------|--------|------|
| `llothsight` | CYBORG_FACE 1+ | Toggle vision modes | `spiderdroid.c:285-342` |

### Equipment

| Command | Effect | Code |
|---------|--------|------|
| `dridereq <piece>` | Create drider equipment (150 primal) | `spiderdroid.c:344-403` |
| `implant [area] [improve]` | View/upgrade implants | `spiderdroid.c:33-186` |

## Drider Armor

Create equipment via `dridereq <piece>` (`spiderdroid.c:344-403`):

**Cost**: 150 primal per piece

**Available pieces** (vnums 33140-33152): whip, ring, collar, armor, helmet, leggings, boots, gloves, sleeves, cloak, belt, bracer, mask

## Regeneration

**Base**: Home rooms only (vnum 93350-93359): 1x werewolf_regen (`update.c:1582-1589`)

**CYBORG_BODY 6**: 2x werewolf_regen + limb regen (`update.c:530`, `update.c:1535-1541`)

## Inherited from Drow

Spider Droids retain Drow abilities:
- `web` - Entangle enemies
- `scry` - Remote viewing
- `readaura` - View target stats
- `darkness` - Create globe of darkness (requires CYBORG_BODY 4+)

## Class Interactions

**Bonus against** (`fight.c:1629-1630`):
- vs Tanarri: Deals 80% damage (Angel killer training)

**Reduced damage from** (`fight.c:1632-1641`):
- Shapeshifter: Takes 85% damage
- Samurai: Takes 90% damage

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_DROID bit |
| face | ch->pcdata->powers[0] | Face implant level (0-5) |
| limbs | ch->pcdata->powers[1] | Leg implant level (0-5) |
| body | ch->pcdata->powers[2] | Body implant level (0-6) |
| drider_power | ch->pcdata->stats[8] | Points for implant upgrades |
| cubeform | ch->newbits | NEW_CUBEFORM for avatar mode |
