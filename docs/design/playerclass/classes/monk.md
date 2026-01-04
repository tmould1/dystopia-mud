# Monk Class Design

## Overview

Monks are followers of God who have devoted their lives to protect and help the good. They use a multi-track ability system, a chi focus mechanic, and mantra-based powers. This is a **base class** that upgrades to Angel.

**Source Files**: `src/classes/monk.c`, `src/classes/monk2.c`, `src/classes/monk.h`
**Class Constant**: `CLASS_MONK` (64)
**Upgrades To**: Angel

## Core Mechanics

### Track System (monkab[4])

Four ability tracks, each leveled 0-4 (`monk.h:52-55`):

| Index | Track | Purpose |
|-------|-------|---------|
| 0 | AWARE (Awareness) | Perception abilities |
| 1 | BODY | Physical enhancements |
| 2 | COMBAT | Combat techniques |
| 3 | SPIRIT | Mystical powers |

**Cost**: 500,000 experience per level

### Chi System (chi[2])

Chi provides combat focus and damage multiplier (`monk.h:44-45`):

```c
ch->chi[CURRENT]  // Current focus level (0-6)
ch->chi[MAXIMUM]  // Master level (max 6)
```

**Chi Combat Bonuses**:
- chi 1-2: 1.2x damage multiplier
- chi 3-5: (chi/2)x multiplier (1.5x to 2.5x)

**Chi Cost**: 500 + ((chi+1) * 20) movement per level

**Chi Decay**: 25% chance per tick to decrease by 1 when out of combat (`update.c:1653`)

### Mantra System (PMONK)

14 mantra levels stored in `ch->pcdata->powers[PMONK]`:

| Level | Mantra | Command | Effect |
|-------|--------|---------|--------|
| 1 | Eyes of God | `godseye` | Toggle PLR_HOLYLIGHT (`monk.c:781`) |
| 2 | Shield/Read Aura/Scry | - | Support powers |
| 3 | Sacred Invisibility | `sacredinvis` | Hide ability, 500 move (`monk.c:597`) |
| 4 | Wrath of God | `wrathofgod` | 4 rapid hits on NPC (`monk.c:863`) |
| 5 | Flaming Hands | `flaminghands` | Toggle fire damage (`monk.c:635`) |
| 6 | Skin of Steel | `steelskin` | Toggle defense (`monk.c:807`) |
| 7 | Almighty's Favor | `godsbless` | Combat buff, 3000 mana (`monk.c:833`) |
| 8 | Dark Blaze | `darkblaze` | Blind opponent (`monk.c:926`) |
| 9 | Chaos Hands | `chands` | Multi-element shields, req BODY 3 (`monk.c:295`) |
| 10 | Celestial Path | `celestial` | Teleport to target, 250 move (`monk.c:694`) |
| 11 | Cloak of Life | `cloak` | Toggle regen aura, 1000 move (`monk.c:547`) |
| 12 | God's Heal | `godsheal` | Heal 150-500 HP, 300-400 mana (`monk.c:966`) |
| 13 | God's Hold | `ghold` | Toggle opponent lock (`monk.c:1006`) |
| 14 | Complete Mastery | - | End of training |

**Mantra Cost**: (level+1) * 10 primal points to advance

## Track Powers

### Spirit Track (monkab[SPIRIT])

| Level | Power | Effect |
|-------|-------|--------|
| 3 | `healingtouch` | Heal self/other, or set MONK_HEAL for periodic regen (`monk.c:130`) |
| 4 | `deathtouch` | Curse target with MONK_DEATH for periodic damage (`monk.c:88`) |

**MONK_HEAL Flag** (`fight.c:167-193`):
- While HP < 50% max: heals 200-400 HP and 175-400 move per combat round
- Once HP >= 50%: 50% chance each round to expire
- Visual: "Your body emits glowing sparks"

**MONK_DEATH Flag** (`fight.c:143-165`):
- While HP > 50% max: deals 50-200 damage per combat round
- Once HP <= 50%: 50% chance each round to expire
- Visual: "You writhe in agony as magical energies tear you asunder"

### Body Track (monkab[BODY])

| Level | Power | Effect |
|-------|-------|--------|
| 1 | `adamantium` | Toggle adamantium hands (NEW_MONKADAM) (`monk.c:664`) |
| 3 | `spiritpower` | +200 damroll/hitroll, 100 move (`monk.c:174`) |

### Awareness Track (monkab[AWARE])

- Level 1+: Nightsight
- Level 2+: Shadowsight
- Level 3+: Truesight
- Level 4+: Scry

### Combat Track (monkab[COMBAT])

Passive combat bonuses per level:
- +1 extra attack per level (`fight.c:1087`)
- +100 max damage per level (`fight.c:1826`)

## Chi Focus Commands

| Command | Position | Effect |
|---------|----------|--------|
| `chi` | POS_FIGHTING | Focus chi +1 (costs movement) (`monk2.c:195`) |
| `relax` | POS_FIGHTING | Reduce chi -1 (`monk2.c:227`) |

**Chi Visual Auras**:
- Level 1: Body flickers with energy
- Level 2: Body pulses with energy
- Level 3: Body glows blue
- Level 4: Body glows bright red
- Level 5: Body flashes with power
- Level 6: Body emits sparks

## Combat Techniques

### Fight Styles (200,000 exp each)

Learned via `learn fight <style>`:
- Trip, Kick, Bash, Elbow, Knee, Headbutt
- Disarm, Bite, Dirt, Grapple, Punch, Gouge
- Rip, Stamp, Backfist, Jumpkick, Spinkick, Hurl, Sweep, Charge

### Techniques (200,000 exp each)

Learned via `learn techniques <tech>`:

| Tech | Command | Effect |
|------|---------|--------|
| TECH_THRUST | `thrustkick` | Thrusting kick attack |
| TECH_SPIN | `spinkick` | Spinning area attack (chi+1 targets) |
| TECH_BACK | `backfist` | Back fist strike |
| TECH_ELBOW | `elbow` | Elbow attack |
| TECH_PALM | `palmstrike` | Palm strike (stuns) |
| TECH_KNEE | `knee` | Knee strike |
| TECH_SWEEP | `sweep` | Leg sweep |
| TECH_SHIN | `shinkick` | Shin kick |

## Combo System

Combos are performed by executing techniques in specific order (`monk2.c`):

| Combo | Sequence | Effect | Code |
|-------|----------|--------|------|
| Lightning Kick | Thrust + Thrust + Spinkick | 1-6 hits scaling with chi level | `monk2.c:473-506` |
| Siphon | Thrust + Thrust + Sweep | Steal 50% victim's mana, heal self | `monk2.c:321-334` |
| Choyoken Pulverise | Reverse + Sweep + Reverse | Steal 50% victim's move, heal HP/mana/move | `monk2.c:264-277` |
| Nerve Pinch | Reverse + Knee + Sweep | 30-round stun (WAIT_STATE) | `monk2.c:336-346` |

**Combo Tracking**: `ch->monkcrap` bitfield tracks current combo state via COMB_* flags

## Monk Armor

Create equipment via `monkarmor` (`monk.c`):

**Cost**: 60 primal per piece

**Available** (vnums 33020-33031):
- Ring, Collar, Robe, Shorts, Helmet
- Gloves, Sleeves, Cloak, Belt, Bracer, Mask, Boots

## Regeneration

Special monk update (`update.c:1653`):

**Zone**: Vnum 93410-93419
- Triggers werewolf_regen for enhanced HP/mana/move

**Meditation**: Enhanced regeneration while meditating

## Combat Flags (ch->newbits)

| Flag | Effect |
|------|--------|
| NEW_MONKFLAME (256) | Flaming hands: 5% chance to ignite enemy (`fight.c:905`), +3% disarm resist, +4% to be disarmed |
| NEW_MONKADAM (4096) | Adamantium hands: 10d25 unarmed damage (`fight.c:1302`), immune to disarm (`fight.c:2346`) |
| NEW_MONKSKIN (8192) | Steel skin: 20% damage reduction (`fight.c:1697`) |
| NEW_MONKFAVOR (16384) | Almighty's favor: +50 damroll (`act_info.c:441`) |
| NEW_MONKCLOAK (2048) | Cloak of life: +50-150 HP regen per tick when HP < 50% (`update.c:1671`) |
| NEW_JAWLOCK | God's hold: 70% chance to prevent enemy flee (`fight.c:4152`) |

## Learning Commands

| Command | Effect |
|---------|--------|
| `mantra` | Display/improve mantras (`monk.c:437`) |
| `mantra power improve` | Advance mantra level |
| `learn` | Learn fight styles, techniques, abilities, chi (`monk2.c:581`) |
| `guide` | Convert avatar to monk (50000 exp both) (`monk.c:346`) |

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_MONK bit |
| chi | ch->chi[2] | Current/max chi (0-6) |
| monkab | ch->monkab[4] | Track levels (AWARE, BODY, COMBAT, SPIRIT) |
| pmonk | ch->pcdata->powers[PMONK] | Mantra level (0-14) |
| monkstuff | ch->monkstuff | Fight styles/techniques learned |
| monkcrap | ch->monkcrap | Current combo state |
| newbits | ch->newbits | Toggle flags |
