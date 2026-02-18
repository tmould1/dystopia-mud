# Proposed Player Classes

This folder contains design documents for proposed class pairs. When a class is implemented, its design doc is copied to the main `classes/` folder.

## Overview

| Base Class | Upgrade Class | Fantasy Niche | Status |
|------------|---------------|---------------|--------|
| ~~Shaman~~ | ~~Spirit Lord~~ | Spirit world / totems | **Implemented** (moved to classes/) |

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

### Spirit Tether (Shaman/Spirit Lord)
- **Dual-World Balance**: Position between material and spirit worlds (0-100/0-150)
- Material abilities (totems) push tether DOWN, Spirit abilities push tether UP
- Grounded: Material abilities stronger, spirit weaker
- Unmoored: Spirit abilities stronger, material weaker
- Shaman places external totems; Spirit Lord internalizes them as personal auras

## Unique Mechanics

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
| Spirit Lord | Spirit Warriors | 3 | Ancestral warriors summoned via Spirit Army |

## Color Schemes

Each class has a distinct two-color palette for visual identity:

| Class | Brackets | Accent | Primary | Theme |
|-------|----------|--------|---------|-------|

| Shaman | `\|~ ~\|` | `#t1A7A6A` Deep teal | `#t5EC4B0` Jade green | Sacred waters, healing stones |
| Spirit Lord | `{{ }}` | `#t4060A0` Spectral indigo | `#t90B8E0` Luminous sky | Ethereal spirit world |

## Implementation Priority

Suggested order based on uniqueness and complexity:

1. **Shaman/Spirit Lord** - Tether balance with totem-to-aura upgrade progression

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
