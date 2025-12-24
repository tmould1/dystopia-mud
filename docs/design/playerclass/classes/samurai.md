# Samurai Class Design

## Overview

Samurai are ancient warriors and masters of armed combat. They are an **upgrade class** obtained by upgrading from Ninja.

**Source File**: `src/samurai.c`
**Class Constant**: `CLASS_SAMURAI` (16)
**Upgrades From**: Ninja

## Lore (from help.are)

> The Samurai is an ancient warrior and master of armed combat. A samurai must train for a long time before his true potential can be released. Once he has mastered the fighting weapons however, few can stand up to such power. Also the samurai is protected from most magiks by the spirits of the ancestors. Beware, the samurai may appear as only mortal, but much power resides in these warriors of old.

## Commands (from help.are)

| Command | Description |
|---------|-------------|
| ancestralpath | Class portal |
| technique | Start using ancient fighting techniques |
| katana | Create the only weapon you'll ever need |
| web | Throw webs at opponent |
| martial | Command to learn the combos |

## Focus/Combo System (from help.are)

During combat, samurai can use four different attack forms, each affecting focus:

| Attack Form | Focus Points |
|-------------|-------------|
| Slide | 1 point |
| Sidestep | 2 points |
| Block | 4 points |
| Countermove | 8 points |

### Combo Triggers
When focus hits certain thresholds, special attacks trigger automatically:
- 10 focus: Special attack
- 15 focus: Special attack
- 20 focus: Special attack
- 25 focus: Special attack
- 30 focus: Special attack
- 35 focus: Special attack

### Focus Management
- When not fighting, counter slowly drops
- While fighting, use `focus` command to make it drop
- Focus counter shown in prompt with `%k` flag

## Katana

The `katana` command creates the samurai's signature weapon - the only weapon they'll ever need.

## Magic Resistance

Samurai are protected from most magicks by the spirits of the ancestors.

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_SAMURAI bit |
| focus | tracked during combat | Combo counter |

## Notes

- Samurais are built over the highlander class/concept
- Must train extensively before true potential is released
- Once weapons are mastered, few can stand against them
- Focus system enables automatic special attacks at thresholds
- Upgrade classes can further upgrade (levels 2-5) for additional combat bonuses
