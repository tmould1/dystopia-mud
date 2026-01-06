# Ninja Class Design

## Overview

Ninjas are warriors trained in the dark art of Ninjutsu. They fight using stealth and quickness, with three principle paths (Sora, Chikyu, Ningenno). Must use `michi` to focus Ki for regeneration. This is a **base class** that upgrades to Samurai.

**Source Files**: `src/classes/ninja.c`, `src/classes/ninja.h`
**Class Constant**: `CLASS_NINJA` (128)
**Upgrades To**: Samurai

## Core Mechanics

### Michi (Ki Focus)

Ninjas CANNOT regenerate without being in Michi state (`ninja.c:236-270`):

```c
ch->rage  // Ki stored in rage field: >= 100 = Michi active
```

**Command**: `michi` (POS_FIGHTING only)
- Cost: 500 movement points
- Sets Ki (rage) += 100
- Cooldown: 12 pulses

**Regeneration** (`update.c:1675`):
- Special dojo rooms (vnum 93460-93469) grant regen while in Michi
- Uses werewolf_regen() with 1-3x multiplier
- Ki increases in combat, decreases out of combat

### Belt System

Belt ranks provide combat scaling (`merc.h:2031-2040`):

| Belt | Value | Damage Multiplier |
|------|-------|-------------------|
| BELT_ONE | 7 | 1.1x |
| BELT_TWO | 8 | 1.2x |
| BELT_THREE | 9 | 1.3x |
| BELT_FOUR | 10 | 1.4x |
| BELT_FIVE | 11 | 1.5x |
| BELT_SIX | 12 | 1.6x |
| BELT_SEVEN | 13 | 1.7x |
| BELT_EIGHT | 14 | 1.8x (2x circle) |
| BELT_NINE | 15 | 1.9x (2.5x circle) |
| BELT_TEN | 16 | 2.0x (3x circle) |

**Dodge Bonus**: Each belt reduces enemy hit chance by 2% (up to -20% at BELT_TEN)

### Principle Paths

Three paths, each with 6 levels. Stored in `ch->pcdata->powers[]`:

| Path | Index | Focus |
|------|-------|-------|
| Sora | NPOWER_SORA (1) | Finding, observing, locating |
| Chikyu | NPOWER_CHIKYU (2) | Battle preparation |
| Ningenno | NPOWER_NINGENNO (3) | Battle/assassination |

**Cost**: (level+1) * 10 primal points to advance

## Sora Path (Sky/Air)

| Level | Name | Command | Effect |
|-------|------|---------|--------|
| 1 | Mitsukeru | `mitsukeru <target>` | Scry/locate enemies anywhere (`clan.c:4356`) |
| 2 | Koryou | `koryou <target>` | Read aura - view stats, HP, equipment (`clan.c:4407`) |
| 3 | Kakusu | `kakusu` | Enhanced hide, only ninjas can see (500 move) (`ninja.c:272`) |
| 4 | Uro-Uro | (passive) | Leave no footprints |
| 5 | Kanzuite | `kanzuite` | Toggle truesight (PLR_HOLYLIGHT), 500 move (`ninja.c:315`) |
| 6 | Bomuzite | `bomuzite <target>` | Sleep gas - put target to sleep, 500 move (`ninja.c:441`) |

## Chikyu Path (Earth)

| Level | Name | Effect |
|-------|------|--------|
| 1 | Tsuyoku | (passive) Damage resistance/toughness |
| 2 | Songai | (passive) Enhanced damage output |
| 3 | Isogu | (passive) Extra attacks per round (`fight.c:1044`) |
| 4 | Tsuiseki | Fast hunting via `hunt` command |
| 5 | Sakeru | (passive) Enhanced dodge/AC bonus (`fight.c:2512`) |
| 6 | Harakiri | `harakiri` - Sacrifice HP for damage cap (`ninja.c:534`) |

### Harakiri Details

Blood power sacrifice (`ninja.c:534-559`):
- Sets `ch->pcdata->powers[HARA_KIRI]` = ch->hit / 500 (minimum 5)
- Reduces HP, Mana, Move to 1
- Enhanced damage while power counter > 0
- Decrements over time (`update.c:271-275`)

## Ningenno Path (Human/Body)

| Level | Name | Command | Effect |
|-------|------|---------|--------|
| 1 | Tsume | `tsume` | Toggle iron claws (VAM_CLAWS) (`ninja.c:514`) |
| 2 | Hakunetsu | `hakunetsu <target>` | Super backstab - 4 hits on 85% success (`ninja.c:630`) |
| 3 | Mienaku | `mienaku` | Never-fail flee, 200 move (`ninja.c:352`) |
| 4 | Shiroken | (passive) | Extra throwing star attacks (`fight.c:821`) |
| 5 | Dokuyaku | (passive) | Poison added to Shiroken attacks (`fight.c:309`) |
| 6 | Circle | `circle [target]` | Backstab while fighting (`fight.c:5407`) |

### Circle Details

Combat backstab (`fight.c:5407-5449`):
- Requires piercing weapon in WEAR_WIELD or WEAR_HOLD
- Guaranteed hit + 25% chance for second hit
- Damage scales by belt: BELT_EIGHT 2x, BELT_NINE 2.5x, BELT_TEN 3x
- Cooldown: 8 pulses

### Hakunetsu Details

Super backstab (`ninja.c:630-684`):
- Target must be at full HP
- 15% failure: 1 backstab hit, 24 pulse wait
- 85% success: 4 backstab hits, 12 pulse wait

## Support Commands

| Command | Effect |
|---------|--------|
| `stalk <target>` | Shadow tracking - teleport to target (500 move) (`ninja.c:29`) |
| `principles` | Display/improve principle paths (`ninja.c:108`) |
| `ninjaarmor` | Create ninja equipment (60 primal) (`ninja.c:561`) |

## Ninja Armor

Create equipment via `ninjaarmor <piece>` (`ninja.c:561-628`):

**Cost**: 60 primal per piece

**Available**: mask, dagger, sword, ring, collar, robe, cap, leggings, boots, gloves, sleeves, cloak, belt, bracer

## Combat Mechanics

### Attack Bonuses

- Belt rank: 10% per belt level (1.1x to 2.0x)
- CHIKYU 3 (Isogu): Extra attacks per round
- NINGENNO 4 (Shiroken): Extra throwing star attacks
- NINGENNO 5 (Dokuyaku): Poison on Shiroken

### Defense Bonuses

- Belt rank: -2% enemy hit chance per belt
- CHIKYU 1 (Tsuyoku): Damage resistance
- CHIKYU 5 (Sakeru): Enhanced dodge

### Backstab Scaling

- gsn_backstab multiplied by belt rank damage
- BELT_EIGHT: 2x backstab
- BELT_NINE: 2.5x backstab
- BELT_TEN: 3x backstab

## Stealth Mechanics

- Kakusu (Sora 3): AFF_HIDE only visible to other ninjas
- Uro-Uro (Sora 4): Leave no tracks
- Mienaku (Ningenno 3): Guaranteed escape from combat

## Weaknesses

**Verified Restrictions**:
- Cannot regenerate without Michi state
- Stalk blocked to/from astral rooms
- Stalk blocked to private/protected rooms
- Target cannot have IMM_TRAVEL for stalk

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_NINJA bit |
| rank | ch->pcdata->rank | Belt rank (BELT_ONE to BELT_TEN) |
| sora | ch->pcdata->powers[NPOWER_SORA] | Sky path (0-6) |
| chikyu | ch->pcdata->powers[NPOWER_CHIKYU] | Earth path (0-6) |
| ningenno | ch->pcdata->powers[NPOWER_NINGENNO] | Human path (0-6) |
| hara_kiri | ch->pcdata->powers[HARA_KIRI] | Blood power counter |
| ki | ch->rage | Ki/Michi state (>= 100 = active) |
| claws | ch->pcdata->stats[UNI_AFF] | VAM_CLAWS for Tsume |
