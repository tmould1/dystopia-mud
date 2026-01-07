# Damage Calculation

This document details how damage is calculated in combat.

## Overview

Damage flows through several stages:
1. Base damage (weapon dice or natural attacks)
2. Damroll bonus
3. Class modifiers
4. Stance modifiers
5. PvP/upgrade adjustments
6. Resistance checks

## char_damroll() Function

**Location:** [act_info.c:491-561](../../src/commands/act_info.c#L491-L561)

Calculates total damage bonus from all sources.

### Base Components

```
damroll = ch->damroll + str_app[get_curr_str(ch)].todam + ch->xdamroll
```

| Source | Description |
|--------|-------------|
| `ch->damroll` | Base damroll from equipment/effects |
| `str_app[].todam` | Strength bonus to damage |
| `ch->xdamroll` | Extra damroll modifier |

### Class-Specific Bonuses (Avatars Only)

These bonuses only apply if `level >= LEVEL_AVATAR`:

| Class | Bonus | Condition |
|-------|-------|-----------|
| Vampire | `+ch->rage` | When rage > 0 |
| Monk | +200 | If `ITEMAFF_CHAOSHANDS` |
| Monk | +50 | If `NEW_MONKFAVOR` |
| Ninja | `+ch->rage` | When rage > 0 |
| Ninja | +50 | If `NPOWER_CHIKYU >= 6` and `HARA_KIRI > 0` |
| Werewolf | `+ch->rage` | If `SPC_WOLFMAN` and rage > 99 |
| Demon | `+ch->rage` | Always |
| Demon | `+(stats[DEMON_POWER])^2` | Power squared |
| Drow | `+(stats[DEMON_POWER])^2` | Power squared |
| Samurai | +500 or `+wpn[1]/2` | If wpn[1] >= 500, else half |
| Samurai | `+total_wpn/12` | Average of all weapon skills |

## char_hitroll() Function

**Location:** [act_info.c:425-490](../../src/commands/act_info.c#L425-L490)

Calculates total to-hit bonus.

### Base Components

```
hitroll = ch->hitroll + str_app[get_curr_str(ch)].tohit
```

### Class-Specific Bonuses

Similar structure to damroll, with same class conditions.

## one_hit() Damage Calculation

**Location:** [fight.c:1156-1500+](../../src/combat/fight.c#L1156)

### Step 1: Base Damage

**For NPCs:**
```c
dam = number_range(ch->level/2, ch->level*3/2)
if (wielding_weapon) dam *= 1.5
```

**For PCs:**

| Condition | Damage Dice |
|-----------|-------------|
| Mageshield spell | `dice(2*avg_spl, 3*avg_spl)` |
| Wielding weapon | `dice(wield->value[1], wield->value[2])` |
| VAM_CLAWS + WOLF_RAZORCLAWS | `dice(25, 35)` |
| VAM_CLAWS only | `dice(10, 20)` |
| NEW_MONKADAM | `dice(10, 25)` |
| Default (unarmed) | `dice(4, 10)` |

**Ninja Belt Bonuses:**

| Belt | Bonus |
|------|-------|
| BELT_SEVEN | `+dice(1, 5)` |
| BELT_EIGHT | `+dice(6, 10)` |
| BELT_NINE | `+dice(11, 15)` |
| BELT_TEN | `+dice(16, 20)` |

### Step 2: Add Damroll

```c
dam += char_damroll(ch)
```

### Step 3: Situational Modifiers

| Condition | Modifier |
|-----------|----------|
| Victim sleeping | dam *= 2 |
| PC with weapon | dam += dam * (wpn_skill+1) / 60 |

### Step 4: Resistance Check

```c
if (ITEMAFF_RESISTANCE) dam = dam * 3/4
```

### Step 5: PvP Adjustment

```c
if (both_are_PCs) dam /= 2
```

### Step 6: NPC Damage Cap

```c
if (NPC_attacker && dam > 2000) dam = 2000 + (dam-2000)/2
```

### Step 7: Upgrade Level Scaling

| Upgrade Level | Multiplier |
|---------------|------------|
| 1 | 1.05x |
| 2 | 1.10x |
| 3 | 1.15x |
| 4 | 1.20x |
| 5 | 1.25x |

### Step 8: Class Damage Multipliers

| Class | Multiplier | Condition |
|-------|------------|-----------|
| Vampire (vs NPC) | 1 + DISC_VAMP_POTE/15 | Potence discipline |
| Tanarri | 1.5x | TANARRI_MIGHT |
| Angel | 1 + ANGEL_JUSTICE/10 | Justice power |
| Demon | 1.1x | DEM_MIGHT |
| Undead Knight | 1.2x | vs Shapeshifter |
| Mage | 1.4x | Mageshield + PINVOKE > 6 or 9 |
| Ninja | 1.1x - 2.0x | BELT_ONE through BELT_TEN |
| Demon | 1.2x | STRONGARMS |
| Shapeshifter | 1.4x base | Default form |
| Shapeshifter | 1.5x | Tiger form |
| Shapeshifter | 1.2x | Faerie form |
| Shapeshifter | 1.6x | Hydra form |
| Shapeshifter | 1.7x | Bull form |
| Droid | 1.3x or 1.5x | 1+ or 2+ limbs |
| Undead Knight | 1.2x or 1.3x | WEAPONSKILL > 4 or > 8 |
| Werewolf | 1.2x | DISC_WERE_BEAR > 5 |

### Step 9: Stance Modifiers

See [stances.md](stances.md) for detailed stance effects.

**Offensive (vs NPC):**
- Up to +1.5x for STANCEPOWER_DAMAGE_3

**Offensive (vs PC):**
- Up to +0.5x for STANCEPOWER_DAMAGE_3

**Defensive (victim):**
- Down to 0.7x for STANCEPOWER_RESIST_3

**Superstance:**
- 1x, 2x, or 3x multiplier based on power level

### Step 10: Minimum Damage

```c
dam = UMAX(5, dam)
```

## Critical Hits

**Location:** [kav_fight.c:1034-1300+](../../src/combat/kav_fight.c#L1034)

### Critical Chance Calculation

```
critical = 0
if (attacker is NPC): critical += (level + 1) / 5
if (attacker is PC): critical += (wpn_skill + 1) / 10
if (victim is NPC): critical -= (level + 1) / 5
if (victim is PC): critical -= (higher_wpn_skill + 1) / 10
```

Special: `NEW_REND` gives 100% critical on 1/25 roll.

### Critical Effects

23 possible outcomes including:
- Eye, ear, nose loss
- Facial disfigurement
- Arm, leg, finger loss
- Equipment damage

**Protections:**
- Face/head armor reduces critical damage
- Stone skin prevents body part loss
- Samurai has 33% chance to avoid critical

## Damage Cap

Final damage is clamped to `damcap[0]` to prevent infinite damage exploits.
