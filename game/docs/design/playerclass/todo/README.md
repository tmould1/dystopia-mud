# Proposed Player Classes

This folder contains design documents for **5 new class pairs** (10 classes total) that fill fantasy gaps in the current Dystopia class roster. These are proposals awaiting implementation - when ready, documents can be moved to the main `classes/` folder.

## Overview

| Base Class | Upgrade Class | Fantasy Niche | Status |
|------------|---------------|---------------|--------|
| [Psion](psion.md) | [Mindflayer](mindflayer.md) | Psionic/Mental powers | Proposed |
| [Cultist](cultist.md) | [Voidborn](voidborn.md) | Eldritch horror | Proposed |
| [Artificer](artificer.md) | [Mechanist](mechanist.md) | Technology/Gadgets | Proposed |
| [Dragonkin](dragonkin.md) | [Wyrm](wyrm.md) | Draconic heritage | Proposed |
| [Chronomancer](chronomancer.md) | [Paradox](paradox.md) | Time manipulation | Proposed |

## Gaps Being Filled

The current class roster covers undead (Vampire, Lich), primal beasts (Werewolf, Shapeshifter), infernal (Demon, Tanar'ri), divine (Monk, Angel), martial (Ninja, Samurai), arcane (Mage), dark elves (Drow, Spider Droid), and sonic/bard (Dirgesinger, Siren).

These new classes address missing archetypes:

| Gap | Solution | Why It's Unique |
|-----|----------|-----------------|
| **No mind-based combat** | Psion/Mindflayer | Mental damage bypasses physical armor entirely |
| **No Lovecraftian horror** | Cultist/Voidborn | Risk/reward Corruption mechanic with self-damage |
| **No tech class** (Spider Droid is Drow-locked) | Artificer/Mechanist | Gadgets, turrets, drones, cybernetic implants |
| **No dragon theme** | Dragonkin/Wyrm | Elemental form-shifting during combat |
| **No temporal magic** | Chronomancer/Paradox | Unique flux balance mechanic |

## Resource Systems

Each class pair has a unique resource that defines its playstyle:

### Focus (Psion/Mindflayer)
- Builds through meditation and combat concentration
- Powers mental attacks that ignore armor
- Higher focus = stronger psychic abilities
- Standard build/spend mechanic

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

### Draconic Fury (Dragonkin/Wyrm)
- Builds rapidly in combat (+3/tick + damage dealt/taken)
- Decays rapidly out of combat (-4/tick)
- Fuels breath weapons and draconic abilities
- Combined with **Elemental Forms** for tactical depth

### Temporal Flux (Chronomancer/Paradox)
- **Balance Mechanic**: Starts at center (50/75), drifts back toward center
- Acceleration abilities push flux UP, Deceleration abilities push flux DOWN
- At extremes: One type becomes stronger, the other weaker
- Forces tactical decisions about which abilities to use when

## Unique Mechanics

### Elemental Forms (Dragonkin/Wyrm)

Dragonkin shift between 4 elemental forms mid-combat:

| Form | Element | Resistance | Vulnerability | Playstyle |
|------|---------|------------|---------------|-----------|
| Flame | Fire | +50% Fire | -25% Cold | Aggressive DoTs |
| Frost | Cold | +50% Cold | -25% Fire | Control/Slows |
| Storm | Lightning | +50% Lightning | -25% Acid | Burst/Stuns |
| Venom | Acid | +50% Acid | -25% Lightning | Debuffs/Armor shred |

Each form locks access to specific abilities. Mastery requires knowing when to shift to exploit enemy weaknesses or cover your own vulnerabilities.

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

Several classes feature minion mechanics:

| Class | Minion Type | Max Count | Notable Feature |
|-------|-------------|-----------|-----------------|
| Mindflayer | Enthralled NPCs | 3 | Hivemind links them for coordinated attacks |
| Artificer | Turrets | 2 | Stationary, room-bound |
| Mechanist | Combat Drones | 4 | Follow between rooms |
| Voidborn | Void Creatures | 2 | Summoned from beyond |
| Paradox | Past Self | 1 | Echo of yourself from another timeline |

## Color Schemes

Each class has a distinct two-color palette for visual identity:

| Class | Accent | Primary | Theme |
|-------|--------|---------|-------|
| Psion | Deep violet | Bright purple | Mental energy |
| Mindflayer | Dark indigo | Pale violet-gray | Alien intellect |
| Cultist | Dark teal | Sickly green | Cosmic horror |
| Voidborn | Void black | Eldritch purple | Space between stars |
| Artificer | Electric blue | Chrome silver | Technology |
| Mechanist | Neon orange | Bright chrome | Advanced tech |
| Dragonkin | Deep crimson | Bright gold | Draconic nobility |
| Wyrm | Deep azure | Platinum white | Ancient majesty |
| Chronomancer | Deep blue | Bright silver | Flow of time |
| Paradox | Shifting violet | Iridescent white | Broken time |

## Implementation Priority

Suggested order based on uniqueness and complexity:

1. **Psion/Mindflayer** - Mental damage is completely new, relatively straightforward mechanics
2. **Cultist/Voidborn** - Corruption risk/reward is novel but self-contained
3. **Artificer/Mechanist** - Turrets/drones need entity management, moderate complexity
4. **Dragonkin/Wyrm** - Form system requires significant UI work for form-locked abilities
5. **Chronomancer/Paradox** - Flux balance and timeline abilities are most complex

## Document Structure

Each class document follows the established format:

1. **Overview** - Theme, source files (TBD), class constant (TBD), upgrade relationship
2. **Color Scheme** - Visual identity for who list and combat messages
3. **Core Mechanics** - Resource system, build/decay, unique mechanics
4. **Training Categories** - 3-5 categories with 2-4 abilities each
5. **Abilities** - Detailed tables with costs, cooldowns, damage formulas, requirements
6. **Class Armor** - Equipment pieces (vnums TBD)
7. **Combat Mechanics** - Damcap bonuses, special damage processing
8. **Data Storage Summary** - `pcdata->powers[]` and `pcdata->stats[]` usage

## Next Steps

To implement a class from this folder:

1. Read the [Implementing a New Class](../implementing-a-new-class.md) guide
2. Assign class constant (next power-of-2 after 32768)
3. Assign equipment vnum range (non-overlapping)
4. Create source files following the documented structure
5. Move the design doc from `todo/` to `classes/` when complete
6. Update `README.md` and `overview.md` in the parent folder
