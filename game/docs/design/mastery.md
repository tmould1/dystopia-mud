# Class Mastery System

## Overview

Class mastery is an end-game achievement milestone that rewards players who have demonstrated proficiency across all combat fundamentals. Upon completion, players receive a unique class-specific mastery item and a permanent power bonus.

**Source Files**: [jobo_act.c:574-651](../src/systems/jobo_act.c#L574-L651), [merc.h:1976](../src/core/merc.h#L1976), [interp.c:308](../src/core/interp.c#L308)

**Command**: `mastery`

**Position Required**: Standing

**Logging**: `LOG_ALWAYS` (sensitive command)

## Requirements

Players must achieve grandmaster status (200 skill) in all three combat categories:

### Weapons (13 types)

All weapon skills must reach 200 in `ch->wpn[]`:

| Index | Weapon Type |
|-------|-------------|
| 0 | Hit |
| 1 | Slice |
| 2 | Stab |
| 3 | Slash |
| 4 | Whip |
| 5 | Claw |
| 6 | Blast |
| 7 | Pound |
| 8 | Crush |
| 9 | Grep |
| 10 | Bite |
| 11 | Pierce |
| 12 | Suck |

### Spell Colors (5 types)

All spell colors must reach 200 (Grand Sorcerer) in `ch->spl[]`:

| Index | Spell Color | Constant |
|-------|-------------|----------|
| 0 | Purple | `PURPLE_MAGIC` |
| 1 | Red | `RED_MAGIC` |
| 2 | Blue | `BLUE_MAGIC` |
| 3 | Green | `GREEN_MAGIC` |
| 4 | Yellow | `YELLOW_MAGIC` |

### Stances (10 types)

All combat stances must reach 200 in `ch->stance[]`:

| Index | Stance | Constant |
|-------|--------|----------|
| 1 | Viper | `STANCE_VIPER` |
| 2 | Crane | `STANCE_CRANE` |
| 3 | Crab | `STANCE_CRAB` |
| 4 | Mongoose | `STANCE_MONGOOSE` |
| 5 | Bull | `STANCE_BULL` |
| 6 | Mantis | `STANCE_MANTIS` |
| 7 | Dragon | `STANCE_DRAGON` |
| 8 | Tiger | `STANCE_TIGER` |
| 9 | Monkey | `STANCE_MONKEY` |
| 10 | Swallow | `STANCE_SWALLOW` |

## Class Weapon Skill Caps

Some classes can train weapon skills beyond the standard 200 cap. These higher caps provide additional combat bonuses (e.g., Samurai gains +175 damcap per weapon at 1000 skill).

| Class | Weapon Cap | Balance Variable |
|-------|------------|------------------|
| Default | 200 | `wpn_cap_default` |
| Drow | 300 | `wpn_cap_drow` |
| Werewolf | 350 | `wpn_cap_werewolf` |
| Lich | 350 | `wpn_cap_lich` |
| Shapeshifter | 400 | `wpn_cap_shape` |
| Droid | 450 | `wpn_cap_droid` |
| Angel | 500 | `wpn_cap_angel` |
| Monk | 800 | `wpn_cap_monk` |
| Samurai | 1000 | `wpn_cap_samurai` |
| Hard Cap | 1100 | `wpn_cap_hard_cap` |

*Source*: [balance.c:228-237](../src/core/balance.c#L228-L237)

**Note**: The mastery command currently requires only 200 in all skills regardless of class. Classes with higher caps have additional progression opportunities beyond mastery.

## Rewards

### Mastery Flag

Upon completion, the `NEW_MASTERY` bit (33554432) is set in `ch->newbits`:

```c
#define NEW_MASTERY  33554432
```

This flag persists across sessions and can only be earned once per character.

### Might Bonus

Characters with the mastery flag receive a +2 bonus to their might calculation:

```c
if ( IS_SET( ch->newbits, NEW_MASTERY ) ) might += 2;
```

*Source*: [jobo_util.c:464](../src/systems/jobo_util.c#L464)

### Class Mastery Items

Each class receives a unique mastery item. The item's `questowner` is set to the player's name, making it soulbound.

| Class | Item Vnum |
|-------|-----------|
| Mage | 33014 |
| Monk | 33032 |
| Vampire | 33054 |
| Drow | 33074 |
| Ninja | 33094 |
| Werewolf | 33112 |
| Demon | 33134 |
| Droid | 33153 |
| Shapeshifter | 33174 |
| Samurai | 33177 |
| Angel | 33193 |
| Tanarri | 33213 |
| Lich | 33233 |
| Dirgesinger | 33333 |
| Siren | 33353 |
| Psion | 33293 |
| Mindflayer | 33313 |
| Undead Knight | 29989 |

## Administrative Commands

### Viewing Mastery Status

The `jope` command displays the mastery flag in a player's newbits:

```
jope <player> newbits
```

### Granting/Removing Mastery

Administrators can toggle the mastery flag via:

```
jope <player> newbits mastery
```

*Source*: [jope.c:95-106](../src/systems/jope.c#L95-L106)

## Data Storage

| Data | Location | Description |
|------|----------|-------------|
| Mastery flag | `ch->newbits` | Bit flag `NEW_MASTERY` (33554432) |
| Weapon skills | `ch->wpn[0-12]` | 13 weapon proficiencies |
| Spell skills | `ch->spl[0-4]` | 5 spell color masteries |
| Stance skills | `ch->stance[1-10]` | 10 combat stance proficiencies |

## Achievement Broadcast

When a player achieves mastery, an info message is broadcast to all players:

```c
sprintf( buf, "%s has achieved mastery.", ch->name );
do_info( ch, buf );
```
