# Tanar'ri Class Design

## Overview

The Tanar'ri are a fierce breed of demonkind who spend eternity fighting in the Blood Wars against their enemies the Baatezu. They are an **upgrade class** obtained by upgrading from Demon.

**Source Files**: `src/tanarri.c`, `src/tanarri.h`
**Class Constant**: `CLASS_TANARRI` (1024)
**Upgrades From**: Demon

## Lore (from help.are)

> The Tanar'ri are a fierce breed of demonkind, who spends eternity fighting in the Blood Wars, a neverending struggle between the Tanar'ri and their abyssal enemies the Baatezu. Life for a Tanar'ri is mostly about wrecking havoc, killing Baatezu, and the occasional raids on the prime material plane.

## Power System

Tanar'ri use a rank-based power system stored in `ch->pcdata->powers[]`:

### Power Indices (tanarri.h)
```c
#define TANARRI_POWER           1   // Bitfield of unlocked powers
#define TANARRI_POWER_COUNTER   2   // Number of sacrifices made
#define TANARRI_FURY_ON         3   // Fury toggle state
```

### Ranks (tanarri.h)
```c
#define TANARRI_FODDER   1   // Starting rank
#define TANARRI_FIGHTER  2
#define TANARRI_ELITE    3
#define TANARRI_CAPTAIN  4
#define TANARRI_WARLORD  5
#define TANARRI_BALOR    6   // Highest rank
```

## Power Acquisition

Powers are gained through the `bloodsacrifice` command (`src/tanarri.c:228-392`).

Each sacrifice adds to `TANARRI_POWER_COUNTER`. When counter reaches rank thresholds (rank * 3), new powers are granted in order based on rank:

### Rank 1 Powers (Fodder)
1. TANARRI_TRUESIGHT - Enhanced vision
2. TANARRI_CLAWS - Grow claws
3. TANARRI_EARTHQUAKE - Earth damage

### Rank 2 Powers (Fighter)
1. TANARRI_EXOSKELETON - Damage resistance
2. TANARRI_FANGS - Extra attack
3. TANARRI_TORNADO - Lightning storm

### Rank 3 Powers (Elite)
1. TANARRI_SPEED - Enhanced speed
2. TANARRI_MIGHT - Damage bonus
3. TANARRI_CHAOSGATE - Portal ability

### Rank 4 Powers (Captain)
1. TANARRI_FIERY - Fire aura
2. TANARRI_FURY - Beastial rage mode
3. TANARRI_HEAD - Extra head attack

### Rank 5 Powers (Warlord)
1. TANARRI_BOOMING - Fear effect
2. TANARRI_ENRAGE - Force others to fight
3. TANARRI_FLAMES - Infernal flames

### Rank 6 Powers (Balor)
1. TANARRI_TENDRILS - Extra attacks
2. TANARRI_LAVA - Lava blast
3. TANARRI_EMNITY - Cause discord

## Commands (from help.are)

| Command | Description |
|---------|-------------|
| bloodsacrifice | Sacrifice to gain powers |
| tornado | Summon a powerful lightning storm |
| infernalflames | Summon abyssal flames to strike enemy |
| earthquake | Quake the earth and damage landdwellers |
| boomingvoice | Strike fear into mortals |
| enmity | Cause discord among friends |
| enrage | Remove sanity, make them fight like beasts |
| truesight | Enhanced vision |
| web | Web mortals in demonic strands |
| claws | Grow claws to tear limbs |
| chaosgate | Open a portal to far away places |
| fury | Let beastial side take over |
| chaossurge | Destroy the pure of heart with chaos |
| lavablast | Shower enemy with lava |
| tantalk | Class channel |
| taneq | Create class equipment |

## Key Abilities

### Chaossurge (tanarri.c:89-142)
Damages based on target's alignment:
- Alignment > 500 (good): 1500 damage
- Alignment > 0: 1000 damage
- Alignment > -500: 500 damage
- Cannot target evil (alignment < -500)

Also usable by Liches with CHAOS_MAGIC >= 3.

### Enmity (tanarri.c:144-187)
Forces two players to attack each other (60% success chance each).
Requires TANARRI_EMNITY power.

### Fury Toggle (tanarri.c:548-575)
Toggle ability that puts Tanar'ri in beastial combat mode.
Stored in `powers[TANARRI_FURY_ON]`.

### Infernal Flames (tanarri.c:656-684)
Fire attack that costs 500 mana.
Also usable by Liches with CON_LORE >= 2.

## Equipment Creation

`taneq` command (tanarri.c:33-87) creates class armor for 150 primal:
- Claymore, Bracer, Collar, Ring, Plate, Helmet
- Leggings, Boots, Gauntlets, Sleeves, Cloak, Belt, Visor

## Power Flag Definitions (tanarri.h)

```c
#define TANARRI_WEB             1
#define TANARRI_CLAWS           2
#define TANARRI_TRUESIGHT       4
#define TANARRI_EARTHQUAKE      8
#define TANARRI_EXOSKELETON     16
#define TANARRI_FANGS           32
#define TANARRI_TORNADO         64
#define TANARRI_SPEED           128
#define TANARRI_MIGHT           256
#define TANARRI_CHAOSGATE       512
#define TANARRI_FIERY           1024
#define TANARRI_FURY            2048
#define TANARRI_HEAD            4096
#define TANARRI_BOOMING         8192
#define TANARRI_ENRAGE          16384
#define TANARRI_FLAMES          32768
#define TANARRI_TENDRILS        65536
#define TANARRI_LAVA            131072
#define TANARRI_EMNITY          262144
```

## Checking Powers

```c
// Check if has a specific power
if (IS_SET(ch->pcdata->powers[TANARRI_POWER], TANARRI_CLAWS)) {
    // Has claws power
}

// Grant a power
SET_BIT(ch->pcdata->powers[TANARRI_POWER], TANARRI_CLAWS);
```

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_TANARRI bit |
| powers | ch->pcdata->powers[TANARRI_POWER] | Unlocked powers bitfield |
| sacrifice_count | ch->pcdata->powers[TANARRI_POWER_COUNTER] | Number of sacrifices |
| fury_on | ch->pcdata->powers[TANARRI_FURY_ON] | Fury toggle state |
| rank | ch->pcdata->rank | Current Tanar'ri rank |

## Notes

- Tanar'ri is a unique class made for Dystopia
- Upgrade classes can further upgrade (levels 2-5) for additional combat bonuses
- Strive to become a True Tanar'ri Balor (rank 6)
