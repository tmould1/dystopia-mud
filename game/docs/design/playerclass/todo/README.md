# Proposed Player Classes

This folder contains design documents for proposed class pairs. When a class is implemented, its design doc is copied to the main `classes/` folder.

## Overview

| Base Class | Upgrade Class | Fantasy Niche | Status |
|------------|---------------|---------------|--------|
| [Cultist](../classes/cultist.md) | [Voidborn](../classes/voidborn.md) | Eldritch horror | **Implemented** |
| [Chronomancer](../classes/chronomancer.md) | [Paradox](../classes/paradox.md) | Time manipulation | **Implemented** |
| [Shaman](shaman.md) | [Spirit Lord](spiritlord.md) | Spirit world / totems | **Design** |

## Gaps Being Filled

The current class roster covers undead (Vampire, Lich), primal beasts (Werewolf, Shapeshifter), infernal (Demon, Tanar'ri), divine (Monk, Angel), martial (Ninja, Samurai), arcane (Mage), dark elves (Drow, Spider Droid), sonic/bard (Dirgesinger, Siren), psionic (Psion, Mindflayer), draconic (Dragonkin, Wyrm), and technology (Artificer, Mechanist).

These new classes address remaining missing archetypes:

| Gap | Solution | Why It's Unique |
|-----|----------|-----------------|
| **No Lovecraftian horror** | Cultist/Voidborn | Risk/reward Corruption mechanic with self-damage |
| **No temporal magic** | Chronomancer/Paradox | Unique flux balance mechanic |
| **No spirit/ancestor magic** | Shaman/Spirit Lord | Tether balance between material and spirit worlds, totem placement evolving to internalized auras |

## Resource Systems

Each class pair has a unique resource that defines its playstyle:

### Corruption (Cultist/Voidborn)
- **Risk/Reward**: High corruption boosts damage but causes self-damage
- 0-49: Safe zone
- 50-74: 1% max HP/tick self-damage
- 75-99: 2% max HP/tick self-damage
- 100+: 3% max HP/tick self-damage (Voidborn only)
- Forces active resource management unlike any other class

### Temporal Flux (Chronomancer/Paradox)
- **Balance Mechanic**: Starts at center (50/75), drifts back toward center
- Acceleration abilities push flux UP, Deceleration abilities push flux DOWN
- At extremes: One type becomes stronger, the other weaker
- Forces tactical decisions about which abilities to use when

### Spirit Tether (Shaman/Spirit Lord)
- **Dual-World Balance**: Position between material and spirit worlds (0-100/0-150)
- Material abilities (totems) push tether DOWN, Spirit abilities push tether UP
- Grounded: Material abilities stronger, spirit weaker
- Unmoored: Spirit abilities stronger, material weaker
- Shaman places external totems; Spirit Lord internalizes them as personal auras

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

### Spirit Tether Balance (Shaman/Spirit Lord)

```
0----25----50----75----100
  GROUND  |BAL| SPIRIT
```

- **Grounded (0-19)**: Material +30%, Spirit -50%
- **Earthbound (20-39)**: Material +15%, Spirit -25%
- **Balanced (40-60)**: All abilities normal
- **Spirit-Touched (61-80)**: Spirit +15%, Material -25%
- **Unmoored (81-100)**: Spirit +30%, Material -50%

Placing totems (material) pushes tether down, strengthening future totems but weakening spirit attacks. Calling spirits pushes tether up. Spirit Lords expand the range to 0-150 and internalize totems as personal auras that move with them — "becoming the totem" rather than placing it.

### Thrall/Summon Systems

Several proposed classes feature minion mechanics:

| Class | Minion Type | Max Count | Notable Feature |
|-------|-------------|-----------|-----------------|
| Voidborn | Void Creatures | 2 | Summoned from beyond |
| Paradox | Past Self | 1 | Echo of yourself from another timeline |
| Spirit Lord | Spirit Warriors | 3 | Ancestral warriors summoned via Spirit Army |

## Color Schemes

Each class has a distinct two-color palette for visual identity:

| Class | Brackets | Accent | Primary | Theme |
|-------|----------|--------|---------|-------|
| Cultist | `{~ ~}` | `#x064` Dark olive | `#x120` Sickly lime | Corruption and decay |
| Voidborn | `*( )*` | `#x055` Dark violet | `#x097` Eldritch purple | Void between stars |
| Chronomancer | `[> <]` | `#x130` Deep copper | `#x215` Warm amber | Clockwork brass |
| Paradox | `>( )<` | `#x160` Deep crimson | `#x210` Warm rose | Temporal danger |
| Shaman | `\|~ ~\|` | `#t1A7A6A` Deep teal | `#t5EC4B0` Jade green | Sacred waters, healing stones |
| Spirit Lord | `{{ }}` | `#t4060A0` Spectral indigo | `#t90B8E0` Luminous sky | Ethereal spirit world |

## Implementation Priority

Suggested order based on uniqueness and complexity:

1. **Cultist/Voidborn** - Corruption risk/reward is novel but self-contained
2. **Chronomancer/Paradox** - Flux balance and timeline abilities are most complex
3. **Shaman/Spirit Lord** - Tether balance with totem-to-aura upgrade progression

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
2. Assign class constant (next power-of-2 after 134217728, which is CLASS_SPIRITLORD)
3. Assign equipment vnum range (next available: 33620+)
4. Create source files following the documented structure
5. Move the design doc from `todo/` to `classes/` when complete
6. Update `README.md` and `overview.md` in the parent folder
