# The Level Command (do_level)

This document details the `level` command which displays a player's combat proficiencies.

## Location

**Function:** `do_level()` in [clan.c:34-199](../../src/systems/clan.c#L34-L199)

## Overview

The `level` command displays three categories of combat proficiency:
1. Weapon levels (13 weapon types)
2. Stance levels (10 normal + 5 super stances)
3. Spell levels (5 magic schools)

Plus miscellaneous character information.

## Weapon Levels

**Data source:** `ch->wpn[0-12]`

Weapon skills improve through combat use. Each weapon type has its own proficiency.

| Index | Weapon Type | Damage Type |
|-------|-------------|-------------|
| 0 | Unarmed | TYPE_HIT + 0 |
| 1 | Slice | TYPE_HIT + 1 |
| 2 | Stab | TYPE_HIT + 2 |
| 3 | Slash | TYPE_HIT + 3 |
| 4 | Whip | TYPE_HIT + 4 |
| 5 | Claw | TYPE_HIT + 5 |
| 6 | Blast | TYPE_HIT + 6 |
| 7 | Pound | TYPE_HIT + 7 |
| 8 | Crush | TYPE_HIT + 8 |
| 9 | Grep | TYPE_HIT + 9 |
| 10 | Bite | TYPE_HIT + 10 |
| 11 | Pierce | TYPE_HIT + 11 |
| 12 | Suck | TYPE_HIT + 12 |

### Weapon Skill Effects

- **To-Hit:** Higher skill reduces THAC0 (level = wpn/5, capped at 40)
- **Damage:** Multiplier of `(wpn_skill+1) / 60` added to base damage
- **Parry:** Defender's weapon skill adds to parry chance
- **Critical:** Weapon skill affects critical hit chance

## Stance Levels

**Data source:** `ch->stance[1-17]`

Stances provide combat bonuses when active during combat.

### Normal Stances (indices 1-10)

| Index | Stance | Primary Benefit |
|-------|--------|-----------------|
| 1 | Viper | Extra attacks |
| 2 | Crane | Parry bonus |
| 3 | Crab | Damage resistance |
| 4 | Mongoose | Dodge bonus |
| 5 | Bull | Damage bonus |
| 6 | Mantis | Parry bonus, extra attacks |
| 7 | Dragon | Damage bonus + resistance |
| 8 | Tiger | Damage bonus, extra attacks |
| 9 | Monkey | Percentage damage bonus |
| 10 | Swallow | Dodge bonus, resistance |

### Super Stances (indices 13-17)

| Index | Stance | Effect |
|-------|--------|--------|
| 13 | SS1 | Enhanced combat bonuses |
| 14 | SS2 | Enhanced combat bonuses |
| 15 | SS3 | Enhanced combat bonuses |
| 16 | SS4 | Enhanced combat bonuses |
| 17 | SS5 | Enhanced combat bonuses |

### Stance Level Effects

- Level 200+: Unlocks special finishing moves (5% chance per round)
- Level 100+: Enables stance damage/resistance multipliers
- Formula: `damage * (stance_level / 100)` for offensive stances

## Spell Levels

**Data source:** `ch->spl[0-4]`

Magic proficiency by school. Used for spell damage calculations.

| Index | School | Color |
|-------|--------|-------|
| 0 | Purple Magic | Purple |
| 1 | Red Magic | Red |
| 2 | Blue Magic | Blue |
| 3 | Green Magic | Green |
| 4 | Yellow Magic | Yellow |

### Entropy Bonus

If character has `ITEMAFF_AFFENTROPY` set, each spell school displays with +20 bonus.

### Spell Damage Formula

For mageshield attacks:
```
dam = dice(2 * avg_spell_level, 3 * avg_spell_level)
```
Where `avg_spell_level = (spl[0] + spl[1] + spl[2] + spl[3] + spl[4]) / 5`

## Miscellaneous Information

The `level` command also displays:

### Recall Room
- **Data:** `ch->home`
- The room VNUM where the character will return on recall

### Hand Preference
- **Flags:** `PLR_RIGHTHAND`, `PLR_LEFTHAND`
- Affects which weapon slot is primary
- "Balanced" if neither flag set

### Generation
- **Data:** `ch->generation`
- Vampire power rank indicator
- Lower generation = more powerful

### Class Information

For certain classes, additional rank/type information is shown:

**Tanarri:** Displays rank and type information

**Vampire:** Displays generation and clan information

## Display Format

The command outputs in a formatted table showing:
```
Weapon levels:
  Unarmed: XXX    Slice: XXX    Stab: XXX    ...

Stance levels:
  Viper: XXX    Crane: XXX    Crab: XXX    ...

Spell levels:
  Purple: XXX    Red: XXX    Blue: XXX    ...

Recall room: XXXXX
Hand preference: [Right/Left/Balanced]
Generation: X
```

## Related Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `improve_wpn()` | [fight.c](../../src/combat/fight.c) | Increases weapon skill on hit |
| `improve_stance()` | [fight.c](../../src/combat/fight.c) | Increases stance skill in combat |
