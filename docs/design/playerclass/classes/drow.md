# Drow Class Design

## Overview

Drow are dark elves with a hierarchical society based on generations. They follow the Church of Lloth and can specialize in one of three professions: Warrior, Mage, or Cleric.

**Source File**: `src/drow.c`
**Class Constant**: `CLASS_DROW` (32)

## Core Mechanics

### Generation System

Drow have a hierarchical generation system:

```c
ch->pcdata->generation  // Lower = older/more powerful
```

- Higher-generation Drow can grant powers to lower
- Generation affects maximum power potential
- Hierarchy is central to Drow society

### Drow Power Pool

Resource for Drow abilities:

```c
ch->pcdata->stats[DROW_POWER]  // Current power points
```

### Professions

Drow must choose one of three professions:

```c
// Checked via special power flags
IS_DEMPOWER(ch, SPC_DROW_WAR)  // Warrior
IS_DEMPOWER(ch, SPC_DROW_MAG)  // Mage
IS_DEMPOWER(ch, SPC_DROW_CLE)  // Cleric
```

Each profession grants access to different abilities.

## Profession Details

### Warrior (SPC_DROW_WAR)
- Focus on physical combat
- Weapon mastery
- Defensive techniques
- Battle tactics

### Mage (SPC_DROW_MAG)
- Offensive magic
- Dark spellcasting
- Arcane manipulation
- Area effects

### Cleric (SPC_DROW_CLE)
- Divine magic (Lloth)
- Healing abilities
- Curses and blessings
- Spider summoning

## Power Granting System

Higher-generation Drow can grant powers to lower:

```c
void do_grant(CHAR_DATA *ch, char *argument) {
    // Check ch->generation < victim->generation
    // Grant power from ch to victim
    // Powers stored in ch->pcdata->powers[1] as bitfield
}
```

### Grantable Powers
Powers can be shared down the generation chain:
- Combat abilities
- Magical knowledge
- Special techniques
- Lloth's blessings

## Power Storage

```c
ch->pcdata->powers[1]  // Bitfield of granted powers
```

Powers are granted as bits that can be checked:

```c
// Check if has a specific power
if (IS_SET(ch->pcdata->powers[1], POWER_BIT)) {
    // Has the power
}
```

## Church of Lloth

Drow belong to the Church of Lloth:

```c
void do_church(CHAR_DATA *ch, char *argument) {
    // Lists all online Drow
    // Shows hierarchy and generations
    // Church membership display
}
```

### Church Features
- View all Drow online
- See generation hierarchy
- Track lineage
- Communal worship

## Key Commands

### Profession
- `profession <war|mag|cle>` - Choose profession (one-time)
- `powers` - View available powers

### Power System
- `grant <player> <power>` - Grant power to lower-gen Drow
- `drow` - View Drow status

### Church
- `church` - View Church of Lloth members
- `lloth` - Pray to Lloth

### Combat (Profession-Based)
- Warrior: Physical combat commands
- Mage: Dark magic spells
- Cleric: Divine spells and curses

## Generation Mechanics

### Hierarchy
```
Generation 1: Ancient Drow (most powerful)
Generation 2: Elder Drow
Generation 3: Mature Drow
...
Generation N: Newest Drow
```

### Power Scaling
```c
// Higher generation = less maximum power
max_power_level = base_max - (ch->pcdata->generation - 1);
```

### Granting Rules
- Can only grant to lower generation (higher number)
- Cannot grant powers you don't have
- Some powers require minimum generation

## Profession Abilities

### Warrior Abilities
| Ability | Description |
|---------|-------------|
| Bladework | Enhanced melee damage |
| Parry | Defensive blocking |
| Riposte | Counter-attack |
| Frenzy | Combat rage |

### Mage Abilities
| Ability | Description |
|---------|-------------|
| Darkfire | Shadow flame spell |
| Webspell | Entangle targets |
| Levitation | Float above ground |
| Dispel | Remove magic |

### Cleric Abilities
| Ability | Description |
|---------|-------------|
| Heal | Restore health |
| Curse | Apply debuffs |
| Summon Spider | Call spider ally |
| Blessing | Apply buffs |

## Combat Bonuses

### Profession-Based
```c
// Warriors get melee bonus
if (IS_DEMPOWER(ch, SPC_DROW_WAR)) {
    damage += warrior_bonus;
}

// Mages get spell bonus
if (IS_DEMPOWER(ch, SPC_DROW_MAG)) {
    spell_power += mage_bonus;
}

// Clerics get healing bonus
if (IS_DEMPOWER(ch, SPC_DROW_CLE)) {
    heal_amount += cleric_bonus;
}
```

### Innate Drow Abilities
- Infravision (dark sight)
- Magic resistance
- Light sensitivity (weakness)

## Weaknesses

- **Light**: Penalties in bright areas
- **Surface Dwelling**: Less powerful above ground
- **Inter-Drow Conflict**: Political dangers
- **Lloth's Disfavor**: Disobedience punished

## Special Features

### Spider Affinity
Drow have connection to spiders:
- Spider companions (Cleric)
- Spider-themed abilities
- Web-based powers

### Faerie Fire
Signature Drow ability:
- Outlines invisible creatures
- Reduces target defense
- Dark purple glow

## Relationship with Other Classes

| Class | Relationship |
|-------|--------------|
| Angel | Mortal enemies |
| Surface races | Generally hostile |
| Demon | Potential allies |
| Vampire | Uneasy coexistence |

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_DROW bit |
| generation | ch->pcdata->generation | Hierarchy position |
| powers | ch->pcdata->powers[1] | Granted power bitfield |
| profession | SPC_DROW_* flags | Chosen profession |
| drow_power | ch->pcdata->stats[DROW_POWER] | Resource pool |

## Design Notes

The Drow class features:
- Unique hierarchical society system
- Power inheritance through generations
- Three-way profession choice
- Strong community/church integration

The generation and granting system creates social gameplay where elder Drow mentor and empower newer ones, building a hierarchical community structure.
