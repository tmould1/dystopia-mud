# Lich Class Design

## Overview

Liches are evil sorcerers who have cheated death and returned as undead. They are an **upgrade class** obtained by upgrading from Mage (Battlemage).

**Source Files**: `src/classes/lich.c`, `src/classes/lich.h`
**Class Constant**: `CLASS_LICH` (256)
**Upgrades From**: Mage

## Core Mechanics

### Lore System

Liches have five lore paths stored in `ch->pcdata->powers[]`:

| Index | Lore | Max Level | Focus |
|-------|------|-----------|-------|
| CON_LORE (1) | Conjuring | 5 | Golems, portals, summons |
| LIFE_LORE (2) | Life | 5 | Regeneration, healing, mana |
| DEATH_LORE (3) | Death | 5 | Pain, cold, insects |
| CHAOS_MAGIC (4) | Chaos | 5 | Chaos effects, shields |
| NECROMANTIC (5) | Necromantic | 5 | Souls, planes, undeath |

**Training Cost**: 10M exp per level (`lich.c:723-821`)
- Level 1: 10M exp
- Level 2: 20M exp
- Level 3: 30M exp
- Level 4: 40M exp
- Level 5: 50M exp

### Combat Bonuses

**Attack Count** (`fight.c:1021`, `fight.c:1098`):
- +5 attacks per round baseline

**Extra Attacks** (`fight.c:809-812`):
- CON_LORE 5+: Extra fireball attack per round
- NECROMANTIC 5+: Extra chillhand attack per round

**Damage Reduction** (`fight.c:1617-1620`):
- 80% damage reduction (takes only 20% damage)

**Damage Cap Bonus** (`fight.c:1779-1787`):
- Base: +450 max damage
- CON_LORE 5+: +350 max damage
- DEATH_LORE 5+: +350 max damage
- LIFE_LORE 5+: +350 max damage
- NECROMANTIC 5+: +350 max damage
- CHAOS_MAGIC 5+: +350 max damage
- **Max bonus**: +2200 at all lores maxed

**Dodge/Parry Bonus** (`fight.c:2405-2438`, `fight.c:2467-2468`, `fight.c:2631-2663`, `fight.c:2695-2696`):
- -40% enemy hit chance on dodge
- -15% additional parry bonus

## Conjuring Lore (CON_LORE)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 2 | `fireball` | AoE 2x fireball (2000 mana) | `tanarri.c:654-699` (shared) |
| 3 | `planartravel <target>` | Create portal to target (500 mana) | `lich.c:595-661` |
| 4 | `golemsummon <type>` | Summon fire/clay/stone/iron golem | `lich.c:824-987` |
| 5 | `pentagram <target>` | Summon player to you (1500 mana) | `lich.c:60-131` |
| 5 | (passive) | Extra fireball attack per round | `fight.c:811` |

### Golem Summons (`lich.c:824-987`)

| Type | Magic Stat | HP/Hitroll/Damroll Formula |
|------|------------|---------------------------|
| Fire | RED_MAGIC | 100 * spl[RED], spl[RED] hit/dam |
| Clay | YELLOW_MAGIC | 100 * spl[YELLOW], spl[YELLOW] hit/dam |
| Stone | GREEN_MAGIC | 100 * spl[GREEN], spl[GREEN] hit/dam |
| Iron | BLUE_MAGIC | 100 * spl[BLUE], spl[BLUE] hit/dam |

One of each type can be summoned at a time. Tracked in `ch->pcdata->powers[GOLEMS_SUMMON]`.

## Life Lore (LIFE_LORE)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 1+ | (passive) | Enables regeneration via werewolf_regen 2x | `update.c:545` |
| 3 | `earthswallow <target>` | Send PC to altar (5000 mana) | `lich.c:255-316` |
| 4 | `powertransfer` | Convert 5000 mana to 7500-10000 HP | `lich.c:510-540` |
| 5 | `polarity [target]` | Drain 2000-4000 mana, heal for half | `lich.c:468-508` |

## Death Lore (DEATH_LORE)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 3 | `chillhand [target]` | 750-1250 damage (1/3 vs PC), -3 STR debuff | `lich.c:422-466` |
| 4 | `painwreck <target>` | 100-200 damage, 33% chance to stun | `lich.c:318-367` |
| 5 | `creepingdoom` | AoE damage = your HP across all mobs (half mana) | `lich.c:369-420` |

## Necromantic Lore (NECROMANTIC)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 3 | `soulsuck <target>` | 250-1000 damage, heal for same (evil only) | `lich.c:133-196` |
| 4 | `planarstorm` | AoE 7500-12500 damage, half to self (half mana) | `lich.c:542-591` |
| 5 | `planeshift` | Toggle ethereal form | `lich.c:32-57` |

## Chaos Magic (CHAOS_MAGIC)

| Level | Command | Effect | Code |
|-------|---------|--------|------|
| 3 | `chaossurge <target>` | Alignment damage (shared with Tanarri) | `tanarri.c:89-142` |
| 5 | `chaosshield` | Toggle aura shielding | `lich.c:663-685` |

### Chaossurge Details

Damage based on target alignment:
- Good (>500): 1500 damage
- Neutral (>0): 1000 damage
- Slightly evil (>-500): 500 damage
- Cannot target evil (<-500)

## Support Commands

| Command | Effect | Code |
|---------|--------|------|
| `lore` | Display current lore levels | `lich.c:687-721` |
| `studylore <lore>` | Train a lore path (costs exp) | `lich.c:723-821` |
| `licharmor <piece>` | Create lich equipment (150 primal) | `lich.c:198-253` |

## Lich Armor

Create equipment via `licharmor <piece>` (`lich.c:198-253`):

**Cost**: 150 primal per piece

**Available pieces** (vnums 33220-33232): scythe, bracer, amulet, ring, plate, helmet, leggings, boots, gauntlets, sleeves, cloak, belt, mask

## Regeneration

**Life Lore Regen** (`update.c:545`, `update.c:1592-1598`):
- LIFE_LORE 1+: 2x werewolf_regen

**Home rooms** (vnum 93360-93369): Additional 1x regen bonus (`update.c:1602-1606`)

**Meditation** (`update.c:1607-1612`):
- POS_MEDITATING: +1000-2000 mana per tick

## Golem Respawn (`update.c:1613-1631`)

Random chance each tick to allow re-summoning each golem type:
- 2% chance per tick to reset fire/stone/clay/iron golem cooldown

## Shared Abilities

Liches share abilities with other classes:
- `chaossurge` - Also usable by Tanar'ri
- `soulsuck` - Also usable by Undead Knights (SPIRIT level 4)
- `fireball` (infernal) - Also usable by Tanar'ri

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_LICH bit |
| conjuring | ch->pcdata->powers[1] | Conjuring lore (0-5) |
| life | ch->pcdata->powers[2] | Life lore (0-5) |
| death | ch->pcdata->powers[3] | Death lore (0-5) |
| chaos | ch->pcdata->powers[4] | Chaos magic (0-5) |
| necromantic | ch->pcdata->powers[5] | Necromantic lore (0-5) |
| golems | ch->pcdata->powers[6] | Summoned golem flags |
