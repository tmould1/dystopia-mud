# Player Class System Overview

## Architecture Summary

The Dystopia MUD class system uses a **bitfield-based architecture** where each class is represented by a power-of-2 constant. Classes are divided into **base classes** (player-selectable) and **upgrade classes** (achieved through progression).

## Terminology

In this codebase, **"class"** and **"race"** are used interchangeably:
- **Code-level**: Everything uses the term "class" (`ch->class`, `CLASS_VAMPIRE`, etc.)
- **Gameplay-level**: Players often refer to these as "races" (vampires, werewolves, etc.)
- There is no separate race system - the class IS the character's supernatural nature

## Class Tiers

### Base Classes (Player-Selectable)
Available via `selfclass` command at level 3 (Avatar):
- Demon
- Werewolf
- Drow
- Ninja
- Vampire
- Monk
- Battlemage (requires 5K mana and 100 in all spell colors)
- Dirgesinger
- Psion
- Dragonkin
- Artificer
- Cultist
- Chronomancer
- Shaman

### Upgrade Classes
Achieved by upgrading from a maxed base class:

| Base Class | Upgrades To |
|------------|-------------|
| Demon | Tanar'ri |
| Werewolf | Shapeshifter |
| Drow | Spider Droid |
| Ninja | Samurai |
| Vampire | Undead Knight |
| Monk | Angel |
| Battlemage | Lich |
| Dirgesinger | Siren |
| Psion | Mindflayer |
| Dragonkin | Wyrm |
| Artificer | Mechanist |
| Cultist | Voidborn |
| Chronomancer | Paradox |
| Shaman | Spirit Lord |

**Source**: `src/systems/upgrade.c`, `gamedata/db/game/base_help.db` (UPGRADE help entry)

## Upgrade System

### First Upgrade Requirements (upgrade.c:141-191)

To upgrade from base class to upgrade class:
- **50K HP**
- **35K Mana**
- **35K Move**
- **40K Quest Points**
- **Generation 1**
- **PK Score Penalty**: For each 100 pkscore below 1000, pay extra 3K in hp/move/mana (max 30K extra)

Location: Temple Altar of Midgaard (room 3054)

### Upgrade Effects

When upgrading:
1. Class changes from base to upgrade class
2. All stats reset to 5K across (hp/mana/move)
3. All class powers reset
4. `set_learnable_disciplines()` called for new class
5. Generation reset to 5

**Source**: `clearshit()` function in `src/upgrade.c:207-322`

### Additional Upgrade Levels (UPGRADE2)

Upgrade classes can upgrade further (up to level 5):

| Level | HP/Mana/Move | Quest Points | PK Score |
|-------|--------------|--------------|----------|
| 2 | 80K across | 40K | 2000 |
| 3 | 90K across | 80K | 2500 |
| 4 | 100K across | 120K | 3000 |
| 5 | 110K across | 160K | 3500 |

Additional requirements:
- All superstances must be set (stance[19-23] != -1)
- Stats reset to 5K across each upgrade
- Gain additional damcap/dodge/parry/attacks

**Source**: `upgrade2()` function in `src/upgrade.c:53-135`

### Checking Upgrade Status

```c
// From upgrade.c - now DB-driven via class_registry table
bool is_upgrade(CHAR_DATA *ch) {
    const CLASS_REGISTRY_ENTRY *reg;
    if (IS_NPC(ch)) return FALSE;
    reg = db_class_get_registry_by_id(ch->class);
    if (reg == NULL) return FALSE;
    return !IS_BASE_CLASS(reg);
}
```

## Core Data Structures

### CHAR_DATA (merc.h)

The primary character structure contains all class-related fields:

```c
struct char_data {
    // Class identification
    int         class;                      // Bitfield of CLASS_* constants

    // Discipline/power levels
    int         power[MAX_DISCIPLINES];     // 44 discipline slots
    int         gifts[21];                  // Werewolf gifts

    // Class-specific resources
    sh_int      gnosis[2];                  // Werewolf: [current, maximum]
    sh_int      rage;                       // Vampire beast rage
    sh_int      chi[2];                     // Monk: [current, maximum]

    // Class-specific flags/state
    sh_int      monkstuff;                  // Monk ability flags
    sh_int      monkab[4];                  // Monk ability levels
    int         warp;                       // Demon warp power bitfield
    int         warpcount;                  // Number of demon warps
    int         form;                       // Current shapeshifted form
    int         polyaff;                    // Polymorphic affinity bits
    int         vampaff_a;                  // Vampire affinity bits
    int         beast;                      // Vampire beast level
    // ...
};
```

### PC_DATA (merc.h)

Player-specific data extending CHAR_DATA:

```c
struct pc_data {
    // Generic class power storage
    int         powers[20];                 // Class-specific power levels
    int         stats[12];                  // Class statistics

    // Hierarchy/generation
    sh_int      generation;                 // Class generation (lower = more powerful)

    // Upgrade tracking
    int         upgrade_level;              // Current upgrade level (0-4)

    // Class-specific
    sh_int      demonic_a;                  // Demon affinity flags
    sh_int      wolfform[2];                // Werewolf form tracking
    // ...
};
```

## Class Constants (class.h)

```c
// Base Classes
#define CLASS_DEMON           1
#define CLASS_MAGE            2
#define CLASS_WEREWOLF        4
#define CLASS_VAMPIRE         8
#define CLASS_DROW           32
#define CLASS_MONK           64
#define CLASS_NINJA         128

// Upgrade Classes
#define CLASS_SAMURAI        16
#define CLASS_LICH          256
#define CLASS_SHAPESHIFTER  512
#define CLASS_TANARRI      1024
#define CLASS_ANGEL        2048
#define CLASS_UNDEAD_KNIGHT 4096
#define CLASS_DROID        8192
#define CLASS_SIREN       32768

// Base Classes (continued)
#define CLASS_DIRGESINGER 16384
#define CLASS_PSION       65536
#define CLASS_DRAGONKIN  262144
#define CLASS_ARTIFICER 1048576

// Upgrade Classes (continued)
#define CLASS_MINDFLAYER 131072
#define CLASS_WYRM       524288
#define CLASS_MECHANIST 2097152

// Base Classes (continued)
#define CLASS_CULTIST   4194304

// Upgrade Classes (continued)
#define CLASS_VOIDBORN  8388608

// Base Classes (continued)
#define CLASS_CHRONOMANCER 16777216

// Upgrade Classes (continued)
#define CLASS_PARADOX   33554432
```

### Checking Class Membership

```c
// Primary macro for class checking
#define IS_CLASS(ch, CLASS)  (IS_SET((ch)->class, CLASS) && (ch->level >= LEVEL_AVATAR))
```

**Important**: Classes only become "active" at level 3 (LEVEL_AVATAR).

## Character Creation Flow

### State Machine (comm.c - nanny function)

```
CON_GET_NAME -> CON_CONFIRM_NEW_NAME -> CON_GET_NEW_PASSWORD
    -> CON_CONFIRM_NEW_PASSWORD -> CON_GET_NEW_SEX -> CON_GET_NEW_ANSI
    -> [Enter game at level 1 with ch->class = 0]
    -> [Reach level 3 and use 'selfclass' command]
```

### Class Selection (act_info.c - do_classself)

Class selection is now DB-driven via the `class_registry` table in `class.db`. Requirements vary by class:
- **Most classes**: Just need to be Avatar (level 3)
- **Battlemage**: Requires 5K mana AND 100+ in all five spell colors

Source: `src/commands/act_info.c`

## Class Power Storage

Different classes use `ch->pcdata->powers[]` for different purposes:

| Class | powers[] Usage | Header File |
|-------|---------------|-------------|
| Angel | ANGEL_PEACE, ANGEL_LOVE, ANGEL_JUSTICE, ANGEL_HARMONY | angel.h |
| Lich | CON_LORE, LIFE_LORE, DEATH_LORE, CHAOS_MAGIC, NECROMANTIC | lich.h |
| Tanar'ri | TANARRI_POWER (bitfield), TANARRI_POWER_COUNTER | tanarri.h |
| Undead Knight | NECROMANCY, INVOCATION, SPIRIT | undead_knight.h |
| Dirgesinger | Buff durations, DOT stacks, training levels (0-13) | dirgesinger.h |
| Siren | Echoshield, Crescendo stage (6-7) | dirgesinger.h |
| Psion | Shield durations, training levels (10-12) | psion.h |
| Mindflayer | Hivemind, thrall count, training levels (10-12) | psion.h |
| Dragonkin | Breath cooldowns, attunement, training levels (10-12) | dragonkin.h |
| Wyrm | Ancient powers, breath mastery, training levels (10-12) | dragonkin.h |
| Artificer | Turret count, buff durations, cooldowns, training levels (10-12) | artificer.h |
| Mechanist | Drone count, implant types, cooldowns, training levels (10-12) | artificer.h |
| Cultist | Grasp/constrict/gibbering ticks, training levels (10-12) | cultist.h |
| Voidborn | Phase shift, void shape, final form, rift, training levels (10-12) | cultist.h |
| Chronomancer | Quicken, blur, foresight, hindsight, echo, slow ticks, training (10-12) | chronomancer.h |
| Paradox | Past self, time loop, split, age, eternity, echo count, training (10-12) | chronomancer.h |

## File Organization

### Core System Files
- `src/core/class.h` - Class constant definitions
- `src/core/merc.h` - Data structures
- `src/systems/upgrade.c` - Upgrade system
- `src/core/handler.c` - Discipline initialization
- `src/core/interp.c` - Command table with class requirements
- `src/commands/act_info.c` - Player self-class selection (do_classself)
- `gamedata/db/game/class.db` - Class display, brackets, armor, and score data
- `gamedata/db/game/base_help.db` - In-game help entries for all classes

### Class Implementation Files

**Base Classes:**
- `src/vamp.c` - Vampire powers
- `src/ww.c`, `src/garou.h` - Werewolf powers
- `src/demon.c` - Demon powers
- `src/mage.c` - Battlemage spells
- `src/drow.c` - Drow powers
- `src/monk.c`, `src/monk2.c` - Monk abilities
- `src/ninja.c` - Ninja abilities
- `src/dirgesinger.c`, `src/dirgesinger.h` - Dirgesinger powers
- `src/classes/psion.c`, `src/classes/psion.h` - Psion powers
- `src/classes/dragonkin.c`, `src/classes/dragonkin.h` - Dragonkin powers
- `src/classes/artificer.c`, `src/classes/artificer.h` - Artificer powers
- `src/classes/cultist.c`, `src/classes/cultist.h` - Cultist powers

**Upgrade Classes:**
- `src/tanarri.c`, `src/tanarri.h` - Tanar'ri powers
- `src/shapeshifter.c` - Shapeshifter forms
- `src/spiderdroid.c` - Spider Droid systems
- `src/samurai.c` - Samurai abilities
- `src/undead_knight.c`, `src/undead_knight.h` - Undead Knight abilities
- `src/angel.c`, `src/angel.h` - Angel powers
- `src/lich.c`, `src/lich.h` - Lich powers
- `src/siren.c` - Siren powers
- `src/classes/mindflayer.c` - Mindflayer powers
- `src/classes/wyrm.c` - Wyrm powers
- `src/classes/mechanist.c` - Mechanist powers
- `src/classes/voidborn.c` - Voidborn powers
- `src/classes/chronomancer.c` - Chronomancer powers
- `src/classes/paradox.c` - Paradox powers

## Design Considerations

### Strengths
- **Bitfield flexibility**: Easy to check and modify class membership
- **Unified structure**: All classes use same base data structures
- **Clear upgrade path**: Each base class has one upgrade destination
- **Clear separation**: Each class has dedicated source file(s) and header

### Limitations
- **Single class design**: While bitfield allows multiple classes, system assumes one primary
- **Hardcoded upgrade paths**: Cannot change upgrade destinations without code changes
- **Mixed storage**: Class data split between CHAR_DATA, PC_DATA, and power arrays
