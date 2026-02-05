# Player Class/Race System Design

This directory contains design documentation for the Dystopia MUD player character class system.

## Overview

The class system in Dystopia MUD defines the fundamental nature of player characters. Each "class" (also referred to as "race" in gameplay terms) represents a supernatural creature type with unique abilities, powers, and progression mechanics.

Classes are divided into **base classes** (player-selectable at Avatar) and **upgrade classes** (achieved through the upgrade system).

## Documentation Index

### Core Architecture
- [System Overview](overview.md) - High-level architecture, data structures, and upgrade system
- [Discipline System](disciplines.md) - Power progression and discipline mechanics (Vampire/Werewolf/Demon)

### Guides
- [Implementing a New Class](implementing-a-new-class.md) - Step-by-step checklist, file inventory, and pitfalls

### Base Class Documentation
- [Vampire](classes/vampire.md) - Blood disciplines, generation system
- [Werewolf](classes/werewolf.md) - Totem spirits and gnosis
- [Demon](classes/demon.md) - Warps and demon points
- [Battlemage](classes/mage.md) - Five-color magic system
- [Drow](classes/drow.md) - Lloth worship and professions
- [Monk](classes/monk.md) - Chi, mantras, and combos
- [Ninja](classes/ninja.md) - Principles and belts
- [Dirgesinger](classes/dirgesinger.md) - Sonic martial bard
- [Psion](classes/psion.md) - Mental powers, Focus resource

### Upgrade Class Documentation
- [Tanar'ri](classes/tanarri.md) - Upgraded from Demon, Blood Wars powers
- [Shapeshifter](classes/shapeshifter.md) - Upgraded from Werewolf, beast forms
- [Spider Droid](classes/spiderdroid.md) - Upgraded from Drow, cybernetic drider
- [Samurai](classes/samurai.md) - Upgraded from Ninja, weapon mastery
- [Undead Knight](classes/undead_knight.md) - Upgraded from Vampire, necromancy
- [Angel](classes/angel.md) - Upgraded from Monk, divine powers
- [Lich](classes/lich.md) - Upgraded from Battlemage, undead sorcery
- [Siren](classes/siren.md) - Upgraded from Dirgesinger, voice of domination
- [Mindflayer](classes/mindflayer.md) - Upgraded from Psion, mental domination

## Upgrade Paths

| Base Class | Upgrades To | Upgrade Requirements |
|------------|-------------|---------------------|
| Demon | Tanar'ri | 50K hp, 35K mana/move, 40K qp, gen 1 |
| Werewolf | Shapeshifter | 50K hp, 35K mana/move, 40K qp, gen 1 |
| Drow | Spider Droid | 50K hp, 35K mana/move, 40K qp, gen 1 |
| Ninja | Samurai | 50K hp, 35K mana/move, 40K qp, gen 1 |
| Vampire | Undead Knight | 50K hp, 35K mana/move, 40K qp, gen 1 |
| Monk | Angel | 50K hp, 35K mana/move, 40K qp, gen 1 |
| Battlemage | Lich | 50K hp, 35K mana/move, 40K qp, gen 1 |
| Dirgesinger | Siren | 50K hp, 35K mana/move, 40K qp, gen 1 |
| Psion | Mindflayer | 50K hp, 35K mana/move, 40K qp, gen 1 |

## Quick Reference

### Class Constants (class.h)

**Base Classes:**
| Class | Constant | Bit Value |
|-------|----------|-----------|
| Demon | CLASS_DEMON | 1 |
| Mage | CLASS_MAGE | 2 |
| Werewolf | CLASS_WEREWOLF | 4 |
| Vampire | CLASS_VAMPIRE | 8 |
| Drow | CLASS_DROW | 32 |
| Monk | CLASS_MONK | 64 |
| Ninja | CLASS_NINJA | 128 |
| Dirgesinger | CLASS_DIRGESINGER | 16384 |
| Psion | CLASS_PSION | 65536 |

**Upgrade Classes:**
| Class | Constant | Bit Value |
|-------|----------|-----------|
| Samurai | CLASS_SAMURAI | 16 |
| Lich | CLASS_LICH | 256 |
| Shapeshifter | CLASS_SHAPESHIFTER | 512 |
| Tanarri | CLASS_TANARRI | 1024 |
| Angel | CLASS_ANGEL | 2048 |
| Undead Knight | CLASS_UNDEAD_KNIGHT | 4096 |
| Spider Droid | CLASS_DROID | 8192 |
| Siren | CLASS_SIREN | 32768 |
| Mindflayer | CLASS_MINDFLAYER | 131072 |

### Key Source Files
- `src/class.h` - Class constants and definitions
- `src/merc.h` - Core data structures (CHAR_DATA, PC_DATA)
- `src/upgrade.c` - Upgrade system implementation
- `src/handler.c` - Discipline initialization
- `src/wizutil.c` - Player self-selection (do_classself)
- `area/help.are` - In-game help entries for all classes
