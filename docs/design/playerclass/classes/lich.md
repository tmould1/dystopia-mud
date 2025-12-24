# Lich Class Design

## Overview

Liches are evil sorcerers who have cheated death and returned as undead. They are an **upgrade class** obtained by upgrading from Battlemage.

**Source Files**: `src/lich.c`, `src/lich.h`
**Class Constant**: `CLASS_LICH` (256)
**Upgrades From**: Battlemage (Mage)

## Lore (from help.are)

> Lichs are powerful sorcerers that have cheated death by entering a state of living death. No longer subject to the affects of time, they use their time to explore other planes and perfect their magic. The lichs grow in power, far beyond that of most mortal mages, since they have so much more time to perfect their art.

## Power System

Liches have five lore paths stored in `ch->pcdata->powers[]`:

### Power Indices (lich.h)
```c
#define CON_LORE     1   // Conjuring lore (0-5)
#define LIFE_LORE    2   // Life lore (0-5)
#define DEATH_LORE   3   // Death lore (0-5)
#define CHAOS_MAGIC  4   // Chaos magic (0-5)
#define NECROMANTIC  5   // Necromantic lore (0-5)
```

All paths max at level 5.

## Lore Paths (from help.are)

### Conjuring (CON_LORE)
- Objectgate
- Fireball
- Planartravel
- Golemsummon
- Pentagram (level 5)

### Death (DEATH_LORE)
- Readaura
- Chillhand
- Painwreck (level 3)
- Creeping Doom (level 5)

### Life (LIFE_LORE)
- Regeneration (provides regen bonus)
- Earthswallow (level 4)
- Powertransfer (level 5)
- Polarity (level 3)

### Necromantic (NECROMANTIC)
- Zombie (level 1)
- Soulsuck (level 3)
- Planarstorm (level 4)
- Planeshift (level 5)

### Chaos Magic (CHAOS_MAGIC)
- Chaosmagic (level 1)
- Chaossurge (level 3) - shared with Tanar'ri
- Chaosshield (level 5)

## Training Lores

Lores are trained using exp via the `gain` command (lich.c:722-822):
- Cost: 10M exp per level (10M for level 1, 20M for level 2, etc.)
- Each lore maxes at level 5
- Syntax: `gain <conjuring|death|life|necromantic|chaos>`

## Key Abilities

### Planeshift (lich.c:32-57)
Toggle ethereal form. Requires NECROMANTIC level 5.
- Phase into plane of spirits
- Toggle on/off

### Pentagram (lich.c:60-131)
Summon player to you. Requires CON_LORE level 5.
- Costs 1500 mana
- Target must have IMM_SUMMON enabled

### Soulsuck (lich.c:133-196)
Drain life and heal. Requires NECROMANTIC level 3.
- Damage: 250-1000
- Heals caster for same amount
- Also usable by Undead Knights (SPIRIT level 4)
- Requires evil alignment

### Polarity (lich.c:267-329)
Attack based on alignment. Requires LIFE_LORE level 3.
- Good target: 1500 damage
- Neutral target: 1000 damage
- Evil target: 500 damage

### Painwreck (lich.c:331-383)
Torment attack. Requires DEATH_LORE level 4.

### Creeping Doom (lich.c:385-436)
Powerful attack. Requires DEATH_LORE level 5.

### Powertransfer (lich.c:480-521)
Transfer power to ally. Requires LIFE_LORE level 5.

### Earthswallow (lich.c:523-560)
Ground-based attack. Requires LIFE_LORE level 4.

### Planarstorm (lich.c:562-608)
Planar damage. Requires NECROMANTIC level 4.

### Golemsummon (lich.c:610-668)
Summon golem. Requires CON_LORE level 3.

### Chaosshield (lich.c:671-706)
Toggle chaos protection. Requires CHAOS_MAGIC level 5.

## Combat Bonuses (from fight.c:1782-1786)

Each lore at level 5 adds 350 to max damage cap:
```c
if (ch->pcdata->powers[CON_LORE] > 4) max_dam += 350;
if (ch->pcdata->powers[DEATH_LORE] > 4) max_dam += 350;
if (ch->pcdata->powers[LIFE_LORE] > 4) max_dam += 350;
if (ch->pcdata->powers[NECROMANTIC] > 4) max_dam += 350;
if (ch->pcdata->powers[CHAOS_MAGIC] > 4) max_dam += 350;
```

Level 5 in all lores at level 5 provides extra attacks (fight.c:811-813):
- CON_LORE > 4: Fireball attack
- NECROMANTIC > 4: Chillhand attack
- DEATH_LORE > 4: Deathaura attack

## Shared Abilities

Liches share some abilities with other classes:
- `chaossurge` - Also usable by Tanar'ri
- `soulsuck` - Also usable by Undead Knights
- `zombie` (from vamp.c) - Requires NECROMANTIC level 1
- `infernalflames` - Requires CON_LORE level 2

## Status Display (lich.c:708-720)

The `gain` command with no argument shows current lore levels:
```
Conjuring : X    Death : X    Life : X
Necromantic : X    Chaos Magic : X
```

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_LICH bit |
| conjuring | ch->pcdata->powers[CON_LORE] | Conjuring lore (0-5) |
| life | ch->pcdata->powers[LIFE_LORE] | Life lore (0-5) |
| death | ch->pcdata->powers[DEATH_LORE] | Death lore (0-5) |
| chaos | ch->pcdata->powers[CHAOS_MAGIC] | Chaos magic (0-5) |
| necromantic | ch->pcdata->powers[NECROMANTIC] | Necromantic lore (0-5) |

## Notes

- Liches are a unique class made for Dystopia
- Masters of conjuring, death, life, necromantics and chaos
- Upgrade classes can further upgrade (levels 2-5) for additional combat bonuses
- Regeneration is enhanced when LIFE_LORE > 0 (update.c:545)
