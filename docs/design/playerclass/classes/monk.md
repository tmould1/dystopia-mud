# Monk Class Design

## Overview

Monks are followers of God who have devoted their lives to protect and help the good. They fight against evil to destroy and banish it from the world. This is a **base class** that upgrades to Angel.

**Source Files**: `src/monk.c`, `src/monk2.c`
**Class Constant**: `CLASS_MONK` (64)
**Upgrades To**: Angel

## Lore (from help.are)

> These Followers of god, have devoted their lives to protect and help the good. They will fight against evil anytime to destroy and banish the evil from this world. With them they have been granted with the power of god. These powers are called mantras.

## Core Mechanics

### Chi System
A monk's chi is essential for combat (from help.are):
- Higher chi = better parry and dodge
- Provides strength and power
- More damage dealt, less received
- Maintaining chi requires plenty of vitality

### Commands
- `mantra` - List current monk powers
- `monkarmor` - Create special equipment bestowed by the gods
- `learn` - Learn monk techniques and abilities

## Abilities System (from help.are)

Monks have four special abilities granted by God. Each has four levels costing 500k exp each.

### Spirit Ability
- Blinding agony
- Healingtouch
- Deathtouch

### Body Ability
- Adamantium hands
- Spiritpower

### Awareness Ability
- Nightsight
- Shadowsight
- Truesight
- Scry

### Combat Ability
- Combat bonuses per level

## Mantras (from help.are)

| Mantra | Effect |
|--------|--------|
| Eyes of God | Truesight |
| Shield | Protection |
| Readaura | Read auras |
| Scry | Remote viewing |
| Sacred Invisibility | Monk invisibility |
| Heart of the Oak | Toughness |
| Adamantium Hands | Advanced parry |
| Flaming Hands | Sets opponent on fire |
| Skin of Steel | Reduced damage |
| The Almighty Favor | Adds strength |
| Dark Blaze | Totally blinds opponent |
| Celestial Path | Portal to players/mobs |
| Prayer of the Ages | (unknown) |
| Cloak of Life | Regen boost when hurt badly |
| Gods Heal | God heals you |
| Gods Hold | God holds players when they flee |

To train a mantra: `Mantra Power Improve`

## Combo System (from help.are)

By using special techniques in a certain order, monks perform combos. Each combo has different effects.

### Available Combos
| Combo | Effect |
|-------|--------|
| Lightning Kick | Series of powerful kicks |
| Tornado Kick | Strikes everyone in room with kicks |
| Choyoken | Devastating attack targeting stamina |
| Raptor Strike | Syphons mystical energies of target |
| Nerve Pinch | Attacks central nerves, paralysing |

Techniques used for combos include thrust kick and others performed in specific order.

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_MONK bit |
| chi | ch->chi[2] | Current/max chi |
| spirit | ch->monkab[SPIRIT] | Spirit ability level |
| body | ch->monkab[BODY] | Body ability level |
| awareness | ch->monkab[AWARENESS] | Awareness level |
| combat | ch->monkab[COMBAT] | Combat ability level |
| monkstuff | ch->monkstuff | State flags |

## Notes

- Monks spend much of their life perfecting combos
- Combo effects are mysterious and must be discovered
- Chi maintenance is strenuous and requires vitality
- Upgrades to Angel when requirements met
