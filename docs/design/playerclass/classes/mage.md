# Mage Class Design

## Overview

Mages are spellcasters who harness the five colors of magic. They use mana as their primary resource and can cast a wide variety of spells across different magical schools.

**Source File**: `src/mage.c`
**Class Constant**: `CLASS_MAGE` (2)

## Core Mechanics

### Mana Pool

Mana is the resource for casting spells:

```c
ch->mana        // Current mana
ch->max_mana    // Maximum mana pool
```

- Regenerates over time
- Spent to cast spells
- Maximum increases with level/training

### Color Magic System

Mages use five colors of magic:

```c
ch->spl[RED_MAGIC]     // Offensive/fire magic
ch->spl[BLUE_MAGIC]    // Defensive/water magic
ch->spl[GREEN_MAGIC]   // Nature/healing magic
ch->spl[YELLOW_MAGIC]  // Lightning/energy magic
ch->spl[PURPLE_MAGIC]  // Arcane/mystic magic
```

Each color has a level (typically 0-200+) that determines spell power in that school.

## Magic Colors

| Color | Index | Theme | Specialization |
|-------|-------|-------|----------------|
| Red | RED_MAGIC | Fire, destruction | Offensive damage |
| Blue | BLUE_MAGIC | Water, ice | Defense, crowd control |
| Green | GREEN_MAGIC | Nature, life | Healing, buffs |
| Yellow | YELLOW_MAGIC | Lightning, energy | Speed, chain effects |
| Purple | PURPLE_MAGIC | Arcane, void | Utility, metamagic |

## Battlemage Variant

A special advanced version of mage:

### Requirements
```c
// To become a battlemage:
ch->max_mana >= 5000
ch->spl[RED_MAGIC] >= 100
ch->spl[BLUE_MAGIC] >= 100
ch->spl[GREEN_MAGIC] >= 100
ch->spl[YELLOW_MAGIC] >= 100
ch->spl[PURPLE_MAGIC] >= 100
```

Battlemages gain access to enhanced combat magic.

## Key Commands

### Spellcasting
- `cast <spell> [target]` - Cast a spell
- `chant <spell>` - Begin channeled spell
- `spells` - List available spells

### Magic Status
- `slearn` - View spell color levels
- `mana` - Check mana status
- `magic` - Overall magic status

### Study
- `study <color>` - Improve a magic color
- `research` - Learn new spells

## Spell Categories by Color

### Red Magic (Offensive)
| Spell | Level Req | Effect |
|-------|-----------|--------|
| Fireball | 20 | Fire damage |
| Flamestrike | 50 | Area fire damage |
| Meteor | 100 | Massive fire damage |
| Inferno | 150 | Sustained burning |

### Blue Magic (Defensive)
| Spell | Level Req | Effect |
|-------|-----------|--------|
| Shield | 20 | Magical barrier |
| Ice Armor | 50 | Protective coating |
| Frost Nova | 80 | Area slow/freeze |
| Blizzard | 120 | Area ice damage |

### Green Magic (Restoration)
| Spell | Level Req | Effect |
|-------|-----------|--------|
| Heal | 10 | Restore health |
| Regeneration | 40 | Heal over time |
| Cure | 60 | Remove ailments |
| Resurrection | 150 | Restore dead |

### Yellow Magic (Energy)
| Spell | Level Req | Effect |
|-------|-----------|--------|
| Shock | 20 | Lightning damage |
| Chain Lightning | 60 | Multi-target |
| Haste | 80 | Speed boost |
| Thunderstorm | 120 | Area lightning |

### Purple Magic (Arcane)
| Spell | Level Req | Effect |
|-------|-----------|--------|
| Teleport | 30 | Movement |
| Dispel | 50 | Remove magic |
| Polymorph | 80 | Transform target |
| Time Stop | 150 | Freeze time |

## Spell Mechanics

### Casting Cost
```c
// Mana cost based on spell power and color level
mana_cost = base_cost - (ch->spl[spell_color] / 10);
if (mana_cost < minimum_cost)
    mana_cost = minimum_cost;
```

### Spell Power
```c
// Damage/effect scales with color level
spell_power = base_power + (ch->spl[spell_color] / 2);
```

### Success Rate
```c
// Higher color level = better success
success_chance = 50 + ch->spl[spell_color];
```

## Mana Regeneration

```c
// Base regeneration per tick
mana_regen = base_regen;

// Wisdom bonus
mana_regen += wisdom_modifier;

// Meditation bonus (if sitting/resting)
if (ch->position == POS_RESTING)
    mana_regen *= 1.5;
```

## Combat Style

### Strengths
- High burst damage potential
- Versatile spell selection
- Range advantage
- Crowd control abilities

### Weaknesses
- Low physical defense
- Mana dependence
- Vulnerable when silenced
- Spell interruption risk

## Spell Learning

Spells are learned by:
1. Training color levels through practice
2. Meeting level requirements
3. Some spells require quests or items

```c
// Check if can cast spell
if (ch->spl[required_color] < spell_requirement) {
    send_to_char("Your magic is not strong enough.\n\r", ch);
    return;
}
```

## Chaos Magic (Lich Cross-Class)

Liches can access chaos magic:

```c
ch->pcdata->powers[CHAOS_MAGIC]  // Chaos magic level
```

This represents corrupted magical power available to undead mages.

## Weaknesses

- **Anti-Magic**: Vulnerable to dispelling
- **Silence**: Cannot cast when silenced
- **Physical Combat**: Weak in melee
- **Mana Drain**: Abilities that drain mana

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_MAGE bit |
| mana | ch->mana | Current mana |
| max_mana | ch->max_mana | Maximum mana |
| red_magic | ch->spl[RED_MAGIC] | Red magic level |
| blue_magic | ch->spl[BLUE_MAGIC] | Blue magic level |
| green_magic | ch->spl[GREEN_MAGIC] | Green magic level |
| yellow_magic | ch->spl[YELLOW_MAGIC] | Yellow magic level |
| purple_magic | ch->spl[PURPLE_MAGIC] | Purple magic level |

## Design Notes

The five-color magic system provides:
- Clear specialization paths
- Meaningful progression in each school
- Synergy opportunities (multi-color spells)
- Natural balance through diversification

Battlemage requirement ensures well-rounded development before accessing advanced combat magic.
