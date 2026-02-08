# Proposed Player Classes

This folder contains design documents for **4 new class pairs** (8 classes total) that fill fantasy gaps in the current Dystopia class roster. These are proposals awaiting implementation - when ready, documents can be moved to the main `classes/` folder.

## Overview

| Base Class | Upgrade Class | Fantasy Niche | Status |
|------------|---------------|---------------|--------|
| [Cultist](cultist.md) | [Voidborn](voidborn.md) | Eldritch horror | Proposed |
| [Artificer](artificer.md) | [Mechanist](mechanist.md) | Technology/Gadgets | Proposed |
| [Chronomancer](chronomancer.md) | [Paradox](paradox.md) | Time manipulation | Proposed |

## Gaps Being Filled

The current class roster covers undead (Vampire, Lich), primal beasts (Werewolf, Shapeshifter), infernal (Demon, Tanar'ri), divine (Monk, Angel), martial (Ninja, Samurai), arcane (Mage), dark elves (Drow, Spider Droid), sonic/bard (Dirgesinger, Siren), and psionic (Psion, Mindflayer).

These new classes address remaining missing archetypes:

| Gap | Solution | Why It's Unique |
|-----|----------|-----------------|
| **No Lovecraftian horror** | Cultist/Voidborn | Risk/reward Corruption mechanic with self-damage |
| **No tech class** (Spider Droid is Drow-locked) | Artificer/Mechanist | Gadgets, turrets, drones, cybernetic implants |
| **No temporal magic** | Chronomancer/Paradox | Unique flux balance mechanic |

## Resource Systems

Each class pair has a unique resource that defines its playstyle:

### Corruption (Cultist/Voidborn)
- **Risk/Reward**: High corruption boosts damage but causes self-damage
- 0-49: Safe zone
- 50-74: 1% max HP/tick self-damage
- 75-99: 2% max HP/tick self-damage
- 100+: 3% max HP/tick self-damage (Voidborn only)
- Forces active resource management unlike any other class

### Power Cells (Artificer/Mechanist)
- Builds through combat and the `charge` command
- Spent on gadgets, turrets, and tech abilities
- `overcharge` pushes beyond limits for bonus damage at risk of backfire
- Mechanist has no decay (internal reactor)

### Temporal Flux (Chronomancer/Paradox)
- **Balance Mechanic**: Starts at center (50/75), drifts back toward center
- Acceleration abilities push flux UP, Deceleration abilities push flux DOWN
- At extremes: One type becomes stronger, the other weaker
- Forces tactical decisions about which abilities to use when

## Unique Mechanics

### Corruption Risk/Reward (Cultist/Voidborn)

Unlike standard resources, Corruption actively punishes high values:
- Damage scales with corruption level (+corruption/10% damage)
- But above 50, you take periodic self-damage
- Creates tension: Push for power and risk death, or stay safe and deal less damage
- `purge` command lets you dump corruption at an HP cost

### Temporal Flux Balance (Chronomancer/Paradox)

```
0----25----50----75----100
     SLOW  |BAL|  FAST
```

- **Deep Slow (0-19)**: Acceleration +30%, Deceleration -50%
- **Slow (20-39)**: Acceleration +15%, Deceleration -25%
- **Balanced (40-60)**: All abilities normal
- **Fast (61-80)**: Deceleration +15%, Acceleration -25%
- **Deep Fast (81-100)**: Deceleration +30%, Acceleration -50%

Using Quicken (acceleration) pushes you toward Fast, making your next Slow ability weaker. Strategic play involves riding the edges or staying balanced for versatility.

### Implant System (Mechanist)

Mechanist has 3 cybernetic slots with swappable implants:

| Slot | Options | Theme |
|------|---------|-------|
| Neural | Combat Processor, Targeting Suite, Threat Analyzer | Reaction/accuracy |
| Servo | Power Arms, Multi-Tool, Shield Generator | Damage/utility |
| Core | Armored Chassis, Regenerator, Power Core | Defense/sustain |

Implants provide passive bonuses and can be changed out of combat.

### Thrall/Summon Systems

Several proposed classes feature minion mechanics:

| Class | Minion Type | Max Count | Notable Feature |
|-------|-------------|-----------|-----------------|
| Artificer | Turrets | 2 | Stationary, room-bound |
| Mechanist | Combat Drones | 4 | Follow between rooms |
| Voidborn | Void Creatures | 2 | Summoned from beyond |
| Paradox | Past Self | 1 | Echo of yourself from another timeline |

## Color Schemes

Each class has a distinct two-color palette for visual identity:

| Class | Accent | Primary | Theme |
|-------|--------|---------|-------|
| Cultist | Dark teal | Sickly green | Cosmic horror |
| Voidborn | Void black | Eldritch purple | Space between stars |
| Artificer | Electric blue | Chrome silver | Technology |
| Mechanist | Neon orange | Bright chrome | Advanced tech |
| Chronomancer | Deep blue | Bright silver | Flow of time |
| Paradox | Shifting violet | Iridescent white | Broken time |

## Implementation Priority

Suggested order based on uniqueness and complexity:

1. **Cultist/Voidborn** - Corruption risk/reward is novel but self-contained
2. **Artificer/Mechanist** - Turrets/drones need entity management, moderate complexity
4. **Chronomancer/Paradox** - Flux balance and timeline abilities are most complex

## Document Structure

Each class document follows the established format:

1. **Overview** - Theme, source files (TBD), class constant (TBD), upgrade relationship
2. **Color and Bracket Scheme** - Visual identity for who list and combat messages
3. **Core Mechanics** - Resource system, build/decay, unique mechanics
4. **Training Categories** - 3-5 categories with 2-4 abilities each
5. **Abilities** - Detailed tables with costs, cooldowns, damage formulas, requirements
6. **Class Armor** - Equipment pieces (vnums TBD)
7. **Combat Mechanics** - Damcap bonuses, special damage processing
8. **Data Storage Summary** - `pcdata->powers[]` and `pcdata->stats[]` usage

## Next Steps

To implement a class from this folder:

1. Read the [Implementing a New Class](../implementing-a-new-class.md) guide
2. Assign class constant (next power-of-2 after 131072, which is CLASS_MINDFLAYER)
3. Assign equipment vnum range (non-overlapping)
4. Create source files following the documented structure
5. Move the design doc from `todo/` to `classes/` when complete
6. Update `README.md` and `overview.md` in the parent folder
