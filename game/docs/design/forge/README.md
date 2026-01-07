# Forge System

The forge system allows players to enhance weapons and armor using materials dropped from mobs. Three types of materials can be applied to equipment:

- **[Metals](metals.md)** - Add hitroll and damroll bonuses
- **[Gems](gems.md)** - Add spell effects to weapons and armor
- **[Hilts](hilts.md)** - Add spell effects to weapons only

## Command Syntax

```
forge <material> <item>
```

Examples:
- `forge copper sword` - Forge a copper slab onto your sword
- `forge diamond armor` - Forge a diamond onto your armor
- `forge ivory blade` - Forge an ivory hilt onto your blade

## Stacking Rules

Items can have **one of each type** applied:
- One metal (copper OR iron OR steel OR adamantite)
- One gem (any gemstone)
- One hilt (weapons only)

A fully forged weapon could have: steel + emerald + gold hilt

## Material Acquisition

Forging materials drop randomly from mobs. Better metals are rarer:

| Material | Drop Rate |
|----------|-----------|
| Copper | 4% |
| Iron | 2% |
| Steel | 1% |
| Adamantite | 0.4% |
| Gems | 2% |
| Hilts | 2% |

Kill mobs and loot their corpses to collect materials.

## Quick Reference

### Metals (Hitroll/Damroll)
| Metal | Hitroll | Damroll |
|-------|---------|---------|
| Copper | +3 | +3 |
| Iron | +6 | +6 |
| Steel | +9 | +9 |
| Adamantite | +12 | +12 |

### Gems (Spell Effects)
| Gem | Spell Effect |
|-----|--------------|
| Diamond | Sanctuary |
| Emerald | Acid Shield |
| Sapphire | Ice Shield |
| Ruby | Fire Shield |
| Topaz | Lightning Shield |
| Jade | Protection |
| Pearl | Invisibility |
| Amethyst | Fly |
| Onyx | Sneak |
| Opal | Pass Door |
| Lazuli | Detect Invis |

### Hilts (Weapon Spell Effects)
| Hilt | Spell Effect |
|------|--------------|
| Ivory | Curse |
| Ebony | Blind |
| Crystal | Dispel Evil |
| Marble | Drain |
| Gold | Lightning |
| Bronze | Acid |
| Sandstone | Fire |
| Limestone | Poison |

## Source Files

- Command: [kav_wiz.c](../../src/commands/kav_wiz.c) (lines 32-712)
- Affect function: [jobo_util.c](../../src/systems/jobo_util.c) (lines 530-547)
- Random drops: [db.c](../../src/core/db.c) (lines 2223-2239)
- Item definitions: [kavir.are](../../../gamedata/area/kavir.are) (VNUMs 30049-30071)
