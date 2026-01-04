# Tanar'ri Class Design

## Overview

The Tanar'ri are a fierce breed of demonkind who spend eternity fighting in the Blood Wars against their enemies the Baatezu. They are an **upgrade class** obtained by upgrading from Demon.

**Source Files**: `src/classes/tanarri.c`, `src/classes/tanarri.h`
**Class Constant**: `CLASS_TANARRI` (1024)
**Upgrades From**: Demon

## Core Mechanics

### Rank System

Tanar'ri progression is based on rank, stored in `ch->pcdata->rank`:

| Rank | Value | Name |
|------|-------|------|
| 1 | TANARRI_FODDER | Fodder |
| 2 | TANARRI_FIGHTER | Fighter |
| 3 | TANARRI_ELITE | Elite |
| 4 | TANARRI_CAPTAIN | Captain |
| 5 | TANARRI_WARLORD | Warlord |
| 6 | TANARRI_BALOR | Balor |

### Power System

Powers are stored as bitflags in `ch->pcdata->powers[TANARRI_POWER]`. Acquired via `bloodsacrifice` command (`tanarri.c:224-394`).

**Sacrifice Cost**: 10000 * (2^(rank-1)) tanarri points per power
- Rank 1: 20,000 points per power
- Rank 2: 40,000 points per power
- Rank 3: 80,000 points per power
- etc.

**Powers per Rank**: 3 powers available at each rank level

### Combat Bonuses

**Extra Attacks** (`fight.c:790-794`):
- TANARRI_FANGS: +1 fang attack per round
- TANARRI_HEAD: +1 attack (heads attack)

**Attack Count Bonus** (`fight.c:1007`, `fight.c:1099-1104`):
- +rank extra attacks per round
- TANARRI_SPEED: +2 additional attacks

**Damage Multiplier** (`fight.c:1354-1357`):
- TANARRI_MIGHT: 1.5x damage

**Damage Cap Bonus** (`fight.c:1774-1779`):
- TANARRI_MIGHT: +500 max damage
- TANARRI_FIERY: +150 * rank max damage

**Damage Reduction** (`fight.c:1626-1629`):
- TANARRI_EXOSKELETON: 80% damage reduction (takes only 20% damage)

**Dodge/Parry Bonus** (`fight.c:2414-2451`, `fight.c:2640-2677`):
- TANARRI_HEAD: -15% enemy hit chance
- TANARRI_SPEED: -5% enemy hit chance
- TANARRI_FURY_ON: -5% enemy hit chance

**Flee Prevention** (`fight.c:4137-4141`):
- TANARRI_TENDRILS: 70% chance to prevent enemy flee

## Powers by Rank

### Rank 1 - Fodder (`tanarri.c:253-274`)

| Power | Flag | Effect |
|-------|------|--------|
| Truesight | TANARRI_TRUESIGHT (4) | Enhanced vision |
| Claws | TANARRI_CLAWS (2) | Toggle claws for unarmed combat |
| Earthquake | TANARRI_EARTHQUAKE (8) | AoE ground attack (1000 mana) |

### Rank 2 - Fighter (`tanarri.c:276-298`)

| Power | Flag | Effect |
|-------|------|--------|
| Exoskeleton | TANARRI_EXOSKELETON (16) | 80% damage reduction |
| Fangs | TANARRI_FANGS (32) | Extra fang attack per round |
| Tornado | TANARRI_TORNADO (64) | AoE lightning storm on flying targets (1500 mana) |

### Rank 3 - Elite (`tanarri.c:299-321`)

| Power | Flag | Effect |
|-------|------|--------|
| Speed | TANARRI_SPEED (128) | +2 attacks, dodge bonus |
| Might | TANARRI_MIGHT (256) | 1.5x damage, +500 dam cap |
| Chaosgate | TANARRI_CHAOSGATE (512) | Teleport to target (1000 move, 6.7% random destination) |

### Rank 4 - Captain (`tanarri.c:322-344`)

| Power | Flag | Effect |
|-------|------|--------|
| Fiery | TANARRI_FIERY (1024) | Fire aura, +150*rank dam cap |
| Fury | TANARRI_FURY (2048) | Toggle +250 hit/damroll |
| Head | TANARRI_HEAD (4096) | Extra head attack, dodge bonus |

### Rank 5 - Warlord (`tanarri.c:345-367`)

| Power | Flag | Effect |
|-------|------|--------|
| Booming | TANARRI_BOOMING (8192) | Voice attack with 25% stun chance |
| Enrage | TANARRI_ENRAGE (16384) | Force target into berserk (60% success) |
| Flames | TANARRI_FLAMES (32768) | AoE 2x fireball on all in room (2000 mana) |

### Rank 6 - Balor (`tanarri.c:368-391`)

| Power | Flag | Effect |
|-------|------|--------|
| Tendrils | TANARRI_TENDRILS (65536) | 70% chance to prevent enemy flee |
| Lava | TANARRI_LAVA (131072) | 3x magma attack + set target on fire (1000 mana/move) |
| Enmity | TANARRI_EMNITY (262144) | Force two players to attack each other (60% each) |

## Commands

### Combat Commands

| Command | Requirement | Effect | Code |
|---------|-------------|--------|------|
| `earthquake` | TANARRI_EARTHQUAKE | AoE attack on non-flying enemies (1000 mana) | `tanarri.c:575-612` |
| `tornado` | TANARRI_TORNADO | AoE lightning on flying enemies (1500 mana) | `tanarri.c:614-652` |
| `infernal` | TANARRI_FLAMES | AoE 2x fireball on all enemies (2000 mana) | `tanarri.c:654-699` |
| `chaossurge <target>` | - | Alignment-based damage (PC only) | `tanarri.c:89-142` |
| `booming <target>` | TANARRI_BOOMING | Voice attack, 25% stun chance | `tanarri.c:501-543` |
| `lavablast` | TANARRI_LAVA | 3x magma hit + ignite (1000 mana/move) | `tanarri.c:396-430` |
| `enrage <target>` | TANARRI_ENRAGE | Force target berserk (60% success) | `tanarri.c:189-222` |
| `enmity <target1> <target2>` | TANARRI_EMNITY | Force two PCs to fight (60% each) | `tanarri.c:144-187` |

### Chaossurge Details (`tanarri.c:89-142`)

Damage based on target alignment:
- Good (>500): 1500 damage
- Neutral (>0): 1000 damage
- Slightly evil (>-500): 500 damage
- Cannot target evil (<-500)

### Toggle Commands

| Command | Requirement | Effect | Code |
|---------|-------------|--------|------|
| `fury` | TANARRI_FURY | Toggle +250 hitroll/damroll | `tanarri.c:545-573` |

### Utility Commands

| Command | Requirement | Effect | Code |
|---------|-------------|--------|------|
| `chaosgate <target>` | TANARRI_CHAOSGATE | Teleport (1000 move, 6.7% random) | `tanarri.c:432-499` |
| `bloodsacrifice` | Rank 1+ | Spend tanarri points for powers | `tanarri.c:224-394` |
| `taneq <piece>` | - | Create class equipment (150 primal) | `tanarri.c:33-87` |

## Tanar'ri Armor

Create equipment via `taneq <piece>` (`tanarri.c:33-87`):

**Cost**: 150 primal per piece

**Available pieces** (vnums 33200-33212): claymore, bracer, collar, ring, plate, helmet, leggings, boots, gauntlets, sleeves, cloak, belt, visor

## Regeneration

Home rooms (vnum 93330-93339): 1x werewolf_regen bonus (`update.c:1574-1578`)

Note: Tanarri have no inherent regen beyond home rooms. Relies on ITEMA_REGENERATE artifacts.

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_TANARRI bit |
| rank | ch->pcdata->rank | Current rank (1-6) |
| powers | ch->pcdata->powers[1] | Unlocked powers bitfield |
| power_counter | ch->pcdata->powers[2] | Number of sacrifices made |
| fury_on | ch->pcdata->powers[3] | Fury toggle state (0/1) |
| tpoints | ch->pcdata->stats[8] | Tanarri points for sacrifices |
