# Undead Knight Class Design

## Overview

Undead Knights are fallen Paladins cursed to live forever, never finding peace. They are an **upgrade class** obtained by upgrading from Vampire.

**Source Files**: `src/undead_knight.c`, `src/undead_knight.h`
**Class Constant**: `CLASS_UNDEAD_KNIGHT` (4096)
**Upgrades From**: Vampire

## Lore (from help.are)

> The Undead Knights where great knights fighting for good when they lived, but a tragic twist in their life made them fail on some important mission, and they have been cursed to spend eternity earthbound. These creatures command many dark forces, and study the arts of necromancy.

## Power System

Undead Knights have three power paths stored in `ch->pcdata->powers[]`:

### Power Indices (undead_knight.h)
```c
#define NECROMANCY      1   // Necromancy level (0-10)
// Additional paths defined elsewhere:
// INVOCATION      2
// SPIRIT          3
```

Necromancy maxes at level 10.

## Commands (from help.are)

| Command | Description |
|---------|-------------|
| knightarmor | Creates ancient armor from when alive |
| gain | Gain access to darker, more powerful magics |
| weaponpractice | Improve fighting skills |
| powerword | Most feared of the knight's powers |
| aura | Enable/disable fearsome auras |
| command | Command the living to do your bidding |
| unholysight | Truesight |
| bloodrite | Blood sacrifice to gain health |
| knighttalk | Class channel |
| soulsuck | Drain lifeforce of good people |

## Aura System (from help.are)

Necromancy grants access to powerful auras via the `aura` command:

| Level | Aura | Effect |
|-------|------|--------|
| 2 | Death Aura | Your mere presence hurts those around you |
| 4 | Might Aura | Increased damroll and hitroll |
| 6 | Bog Aura | Permanent swamp, hard to flee from you |
| 9 | Fear Aura | Fighting you makes warriors piss their pants |
| 10 | Cloak of Death | Regen boost when about to die |

## Training Powers

From undead_knight.c:209-330, gain command trains powers:

### Necromancy Training
- Max level: 10
- Cost: (current_level * 60 + 60) primal per level
- Syntax: `gain necromancy`

## Key Ability Requirements

From undead_knight.c:

### Aura Toggles
```c
// Death aura requires NECROMANCY level 2
if (ch->pcdata->powers[NECROMANCY] < 2)

// Might aura requires NECROMANCY level 4
if (ch->pcdata->powers[NECROMANCY] < 4)

// Bog aura requires NECROMANCY level 6
if (ch->pcdata->powers[NECROMANCY] < 6)

// Fear aura requires NECROMANCY level 9
if (ch->pcdata->powers[NECROMANCY] < 9)
```

### Soulsuck (shared with Lich)
From lich.c:153-156:
- Requires SPIRIT level 4 for Undead Knights
- Drains life from target, heals caster

## Status Display (undead_knight.c:307-310)

```c
sprintf(buf,"Necromancy: %d  Invocation: %d  Spirit: %d\n\r",
    ch->pcdata->powers[NECROMANCY],
    ch->pcdata->powers[INVOCATION],
    ch->pcdata->powers[SPIRIT]);
```

## Combat Bonuses

From update.c:278, high NECROMANCY provides regeneration bonus:
```c
if (IS_CLASS(ch, CLASS_UNDEAD_KNIGHT) && ch->pcdata->powers[NECROMANCY] > 9)
    // Enhanced regeneration at level 10
```

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_UNDEAD_KNIGHT bit |
| necromancy | ch->pcdata->powers[NECROMANCY] | Necromancy level (0-10) |
| invocation | ch->pcdata->powers[INVOCATION] | Invocation level |
| spirit | ch->pcdata->powers[SPIRIT] | Spirit level |

## Notes

- Undead Knights are a unique class made for Dystopia
- Command dark forces and study necromancy
- Have access to powerful auras that scale with Necromancy level
- Upgrade classes can further upgrade (levels 2-5) for additional combat bonuses
- Share some abilities with Liches (soulsuck)
