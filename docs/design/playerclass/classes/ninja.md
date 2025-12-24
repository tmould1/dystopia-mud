# Ninja Class Design

## Overview

Ninjas are warriors trained in the dark art of Ninjutsu. They fight using stealth and quickness, making them deadly foes. This is a **base class** that upgrades to Samurai.

**Source File**: `src/ninja.c`
**Class Constant**: `CLASS_NINJA` (128)
**Upgrades To**: Samurai

## Lore (from help.are)

> Ninja's are warriors trained in the dark art of Ninjutsu. They fight using stealth and quickness making them a deadly foe. Ninja's have few, yet powerful powers at their disposal, and when these powers are mastered a ninja can be a most powerful adversary. Through stealth a ninja can sneak upon a victim and do a deadly amount of damage, and if this is not enough they can slink away and hide in the shadows.

## Core Mechanics

### Michi (Focus)
It is vital for a ninja to focus their Ki using the `michi` command. A ninja CANNOT regen while not in a state of michi.

### Belt System
Ninjas are ranked with a belt system. Each belt adds more power to combat abilities:
- More damage
- Better dodge
- More attacks
- etc.

Belts are trained with: `train beltX` where X is the current belt to learn.

## Principles System (from help.are)

Ninjas have three principle paths, each with 6 levels. Improved with:
```
principles <sora/ningenno/chikyu> improve
```

### SORA (Sky/Air Principles)
| Level | Name | Effect | Syntax |
|-------|------|--------|--------|
| 1 | Mitsukeru | Scry type power | mitsukeru target |
| 2 | Koryo | Readaura type power | koryo target |
| 3 | Kakusu | Class invisibility | kakusu |
| 4 | Uro-Uro | Super sneak, leave no tracks | automatic |
| 5 | Kanzuite | Truesight power | kanzuite |
| 6 | Bomuzite | Sleep gas, puts victim to sleep | bomuzite target |

### CHIKYU (Earth Principles)
| Level | Name | Effect | Syntax |
|-------|------|--------|--------|
| 1 | Tsuyoku | Damage resistance | automatic |
| 2 | Songai | Enhanced damage | automatic |
| 3 | Isogu | Adds one attack | automatic |
| 4 | Tsuiseki | Fast hunting | hunt target |
| 5 | Sakeru | Enhanced dodging | automatic |
| 6 | Harakiri | Hurt yourself to gain damcap | harakiri *self* |

### NINGENNO (Human/Body Principles)
| Level | Name | Effect | Syntax |
|-------|------|--------|--------|
| 1 | Tsume | Iron claws worn on wrist | tsume |
| 2 | Hakunetsu | Super backstab | hakunetsu target |
| 3 | Mienaku | Improved flee | mienaku |
| 4 | Shiroken | Added attacks per round | automatic |
| 5 | Dokuyaku | Adds poison to Shiroken | automatic |
| 6 | Circle | Backstab while in combat | circle target |

## Combat Style

### Strengths
- High stealth capability
- Powerful backstab mechanics
- Fast movement and escape
- Poison attacks

### Automatic Bonuses
Several principles provide passive bonuses:
- Tsuyoku: Damage resistance
- Songai: Enhanced damage
- Isogu: Extra attack
- Uro-Uro: Leave no tracks
- Sakeru: Enhanced dodge
- Shiroken: Extra attacks
- Dokuyaku: Poison on attacks

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_NINJA bit |
| belt | training system | Belt rank |
| sora | principle levels | Sky path |
| chikyu | principle levels | Earth path |
| ningenno | principle levels | Human path |

## Notes

- Must use `michi` to regenerate
- Stealth abilities allow approaching victims unseen
- Can circle backstab in combat (Ningenno level 6)
- Upgrades to Samurai when requirements met
