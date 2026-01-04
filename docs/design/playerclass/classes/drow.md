# Drow Class Design

## Overview

Drow are dark elves following the Church of Lloth. They use a hierarchical generation system where higher-generation drow can grant powers to lower. Three professions (Warrior, Mage, Cleric) provide different ability sets.

**Source Files**: `src/classes/drow.c`, `src/classes/drow.h`
**Class Constant**: `CLASS_DROW` (32)

## Core Mechanics

### Generation System

Generation determines hierarchy and power granting:

```c
ch->pcdata->generation  // Lower = more powerful, can grant to higher numbers
```

- Generation < 3: Can be granted professions
- Generation 2-3: Can grant professions to lower-generation drow
- Generation > 2 + Level < 6: Cannot change another drow's existing profession

### Drow Points

Resource for granting powers (`drow.h`):

```c
ch->pcdata->stats[DROW_POWER]  // stats[8] - Points for granting powers
ch->pcdata->stats[DROW_MAGIC]  // stats[11] - Magic resistance stat
```

### Power Storage

All powers stored as bitfield in `ch->pcdata->powers[1]`:

```c
IS_SET(ch->pcdata->powers[1], DPOWER_*) // Check for power
```

### Professions

Stored as flags in `ch->special`:

| Profession | Flag | Focus |
|------------|------|-------|
| Warrior | SPC_DROW_WAR (128) | Physical combat |
| Mage | SPC_DROW_MAG (256) | Chaos magic |
| Cleric | SPC_DROW_CLE (512) | Healing, Lloth worship |

## Powers by Cost

| Power | Cost | Flag | Effect |
|-------|------|------|--------|
| Levitation | 1,000 | DPOWER_LEVITATION | Avoid fall damage |
| Drowfire | 2,500 | DPOWER_DROWFIRE | Fire spell (100 mana) |
| Drowpoison | 2,500 | DPOWER_DROWPOISON | Poison resistance |
| Confuse | 2,500 | DPOWER_CONFUSE | 25% force flee (75 move) |
| Dgarotte | 2,500 | DPOWER_DGAROTTE | Dark garotte (requires darkness) |
| Drowsight | 5,000 | DPOWER_DROWSIGHT | Toggle holylight vision |
| Drowshield | 5,000 | DPOWER_DROWSHIELD | Toggle IMM_SHIELDED |
| Garotte | 5,000 | DPOWER_GAROTTE | Whip garrotte attack |
| Web | 5,000 | DPOWER_WEB | Web spell to trap |
| Glamour | 5,000 | DPOWER_GLAMOUR | Modify item appearance |
| Speed | 7,500 | DPOWER_SPEED | +3-5 extra attacks |
| Toughskin | 7,500 | DPOWER_TOUGHSKIN | Damage reduction |
| Darkness | 7,500 | DPOWER_DARKNESS | Toggle darkness globe (500 mana) |
| Earthshatter | 7,500 | DPOWER_EARTHSHATTER | AoE damage (150 mana) |
| Shadowwalk | 10,000 | DPOWER_SHADOWWALK | Teleport to drow (250 move) |
| Fightdance | 10,000 | DPOWER_FIGHTDANCE | 50%+ parry with whip |
| Drowhate | 20,000 | DPOWER_DROWHATE | Toggle +650 max damage |
| Spiderarms | 25,000 | DPOWER_ARMS | Spider arms |
| Spiderform | 25,000 | DPOWER_SPIDERFORM | Transform (+400 hit/dam, -1000 AC) |
| Darktendrils | 25,000 | DPOWER_DARKTENDRILS | Toggle 20% damage negation |

**Note**: Granting to other drow multiplies cost by 5x

## Commands by Power

### Toggle Powers

| Command | Power | Effect |
|---------|-------|--------|
| `drowsight` | DPOWER_DROWSIGHT | Toggle PLR_HOLYLIGHT (`drow.c:569`) |
| `drowshield` | DPOWER_DROWSHIELD | Toggle IMM_SHIELDED (`drow.c:591`) |
| `drowhate` | DPOWER_DROWHATE | Toggle +650 damage (`drow.c:452`) |
| `darkness` | DPOWER_DARKNESS | Toggle darkness globe, 500 mana (`drow.c:715`) |
| `darktendrils` | DPOWER_DARKTENDRILS | Toggle 20% damage negation (`drow.c:472`) |
| `fightdance` | DPOWER_FIGHTDANCE | Toggle dance parry with whip (`drow.c:492`) |
| `spiderform` | DPOWER_SPIDERFORM | Transform to giant mylochar (`drow.c:512`) |

### Active Powers

| Command | Power | Cost | Effect |
|---------|-------|------|--------|
| `drowfire` | DPOWER_DROWFIRE | 100 mana | Fire damage + debuffs (`drow.c:321`) |
| `garotte` | DPOWER_GAROTTE | - | Whip strangle attack (`fight.c:5246`) |
| `dgarotte` | DPOWER_DGAROTTE | Requires darkness | 5-hit assassination (`fight.c:5312`) |
| `web` | DPOWER_WEB | - | Trap target in web (`magic.c:5296`) |
| `confuse` | DPOWER_CONFUSE | 75 move | 25% force flee (`drow.c:828`) |
| `shadowwalk` | DPOWER_SHADOWWALK | 250 move | Teleport to drow (`drow.c:385`) |
| `glamour` | DPOWER_GLAMOUR | - | Rename/restyle items (`drow.c:765`) |
| `earthshatter` | DPOWER_EARTHSHATTER | 150 mana | AoE damage spell (`drow.c:865`) |

### Profession-Based Commands

| Command | Profession | Cost | Effect |
|---------|------------|------|--------|
| `chaosblast` | SPC_DROW_MAG | 750 mana | Chaos damage spell (`drow.c:214`) |
| `heal` | SPC_DROW_CLE (or gen <= 2) | 750 mana | Self-heal BLUE_MAGIC*3 (`drow.c:362`) |

### Utility Commands

| Command | Effect |
|---------|--------|
| `grant` | Grant powers/professions to lower-gen drow (`drow.c:57`) |
| `drowpowers` | Display all granted powers and stats |
| `lloth` | View Church of Lloth (all drow online) |
| `drowcreate` | Create drow equipment (60 primal) (`drow.c:253`) |

## Spiderform Details

Transform to giant mylochar (`drow.c:512-566`):

**Requirements**: DPOWER_SPIDERFORM, not already polymorphed

**Bonuses**:
- +400 hitroll
- +400 damroll
- -1000 AC (better armor)
- THIRD_HAND and FOURTH_HAND (extra attacks)

**Visual**: Name changes to "[name] the giant mylochar"

## Combat Mechanics

### Attack Bonuses (`fight.c`)

- DPOWER_SPEED: +3-5 extra attacks per round
- NEW_DROWHATE: +650 max damage
- NEW_DFORM (spiderform): +650 max damage

### Defense Bonuses

- DPOWER_SPEED: -50% enemy to-hit, +50% drow to-hit, +20 vs others
- NEW_DARKTENDRILS: 20% chance to negate damage (up to 5 times)
- NEW_FIGHTDANCE + whip: 50%+ parry chance
- DPOWER_TOUGHSKIN: Damage reduction

### Magic Resistance

`ch->pcdata->stats[DROW_MAGIC]` provides spell damage reduction:
- Random % roll against DROW_MAGIC stat
- On success: 50% damage reduction from hostile spells

## Drowfire Spell

Cast via `drowfire` command (`magic.c:5254-5293`):

**Effects Applied**:
- AFF_DROWFIRE, AFF_CURSE, AFF_BLIND
- AC penalty: +200 (worse)
- STR penalty: -2
- HITROLL penalty: -10
- Duration: 2 ticks

## Dark Garotte

Special assassination attack (`fight.c:5312-5394`):

**Requirements**:
- DPOWER_GAROTTE + DPOWER_DGAROTTE
- NEW_DARKNESS active
- Target not fighting
- Target at full HP
- Not in arena

**Effect**: Removes darkness, delivers 5 backstab hits

## Regeneration

Special drow homeland regeneration (`update.c:1741-1749`):

**Zone**: Vnum 93440-93446
- Triggers werewolf_regen(ch, 1) when below max stats
- Restores HP, Mana, Move points

## Equipment Creation

`drowcreate` creates enchanted equipment (`drow.c:253-319`):

**Cost**: 60 primal per piece

**Available** (vnums 33060-33073):
- Whip, Dagger, Ring, Amulet, Armor, Helmet
- Leggings, Boots, Gauntlets, Sleeves, Belt, Bracer, Mask, Cloak

## Weaknesses

**Note**: Sunlight damage and surface penalties are NOT implemented in current codebase.

**Verified Restrictions**:
- Cannot shadowwalk to/from astral rooms
- Cannot shadowwalk to IMM_TRAVEL targets
- Darkness blocked during fight_timer > 0

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_DROW bit |
| generation | ch->pcdata->generation | Hierarchy position |
| powers | ch->pcdata->powers[1] | DPOWER_* bitfield |
| profession | ch->special | SPC_DROW_WAR/MAG/CLE |
| drow_power | ch->pcdata->stats[8] | Points for granting |
| drow_magic | ch->pcdata->stats[11] | Magic resistance |
| newbits | ch->newbits | Toggle states (darkness, hate, etc.) |
