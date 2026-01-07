# Attack System

This document details the multi-hit system and attack count calculations.

## Overview

Each combat round, characters can make multiple attacks based on class, equipment, stances, and abilities.

## multi_hit() Function

**Location:** [fight.c:259-650](../../src/combat/fight.c#L259-L650)

Executes one round of combat attacks.

### Weapon Selection

Weapons are selected from equipped slots:

| Slot | Constant | Hand Type |
|------|----------|-----------|
| Primary | WEAR_WIELD | 1 |
| Secondary | WEAR_HOLD | 2 |
| Third | WEAR_THIRD | 4 |
| Fourth | WEAR_FOURTH | 8 |

When dual-wielding or more, attacks randomly select between available weapons.

### Special Attack Types

These bypass normal multi-attack logic:
- Headbutt
- Hooves
- Laser
- Tentacle
- Fangs
- Class-specific natural attacks

## number_attacks() Function

**Location:** [fight.c:966-1070](../../src/combat/fight.c#L966-L1070)

Calculates how many attacks a character gets per round.

### NPC Attack Count

Base attacks scale with level:

| Level | Attacks |
|-------|---------|
| 1-49 | 1 |
| 50-99 | 2 |
| 100-499 | 3 |
| 500-999 | 4 |
| 1000-1499 | 5 |
| 1500-1999 | 6 |
| 2000+ | 8 |

**Additional:** `+hitsizedice` (max 20)

### PC Attack Count (vs NPCs)

**Base:** 1 attack

### Stance Bonuses

| Stance | Bonus | Condition |
|--------|-------|-----------|
| Viper | +1 | 50% chance |
| Mantis | +1 | 50% chance |
| Tiger | +1 | 50% chance |
| Wolf | +2 | 50% chance |
| Superstance | +2 | Always |

### Upgrade Level Bonuses

| Upgrade Level | Extra Attacks |
|---------------|---------------|
| 1 | +1 |
| 2 | +2 |
| 3 | +3 |
| 4 | +4 |
| 5 | +5 |

### Class-Specific Attack Bonuses

| Class | Bonus | Condition |
|-------|-------|-----------|
| Vampire | +0.5-2 per Celerity | Up to 4 levels |
| Tanarri | +rank | Based on Tanarri rank |
| Droid | +cyborg limbs | Per limb installed |
| Angel | +Justice points | Per point |
| Werewolf | +2 | Lynx > 2 |
| Werewolf | +5 | Boar, if move > 40000 |
| Undead Knight | +weaponskill/2 | Half weapon skill |
| Lich | +5 | Always |
| Shapeshifter | +2 base | Always |
| Shapeshifter | +0.5-1 per form | Per form level |
| Demon | +2 | Speed power |
| Demon | +3 | Quickness power |
| Drow | +2 | War power |
| Drow | +3 | Speed power |
| Mage | +4 | Beast form |
| Ninja | +1-5 | Per belt rank |
| Ninja | +3 | Chikyu >= 3 |
| Samurai | +5 | Always |

### Item Affect Bonuses

| Effect | Bonus |
|--------|-------|
| ITEMAFF_SPEED | +2 |

### Monk Special System

Monks use a special `fightaction` system instead of standard attack counts.

## one_hit() Function

**Location:** [fight.c:1156-1500+](../../src/combat/fight.c#L1156)

Resolves a single attack within the combat round.

### Attack Resolution Flow

1. **Select weapon** based on hand type (1, 2, 4, 8, or default)

2. **Determine attack type (dt)**
   - Use weapon's `value[3]` if available (0-12 weapon types)
   - Default to `TYPE_HIT` (1000) if no weapon

3. **Calculate weapon skill level**
   ```c
   level = ch->wpn[dt-1000] / 5  // Capped at 40
   ```
   Only applies if weapon level > 5

4. **Calculate THAC0**
   ```c
   thac0 = interpolate(level, SKILL_THAC0_00, SKILL_THAC0_32) - char_hitroll(ch)
   // SKILL_THAC0_00 = 6 (level 0)
   // SKILL_THAC0_32 = 18 (level 32+)
   ```

5. **Calculate victim AC**
   ```c
   victim_ac = UMAX(-100, char_ac(victim) / 10)
   if (!can_see(ch, victim)) victim_ac -= 4
   ```

6. **Attack roll**
   ```c
   roll = number_bits(5)  // 0-19
   // Miss if: roll == 0 OR (roll != 19 AND roll < thac0 - victim_ac)
   // Hit if: roll == 19 OR roll >= thac0 - victim_ac
   ```

7. **On hit:** Calculate damage (see [damage-calculation.md](damage-calculation.md))

8. **Call damage()** with calculated amount

9. **Improve skills**
   - `improve_wpn(ch, dt, right_hand)`
   - `improve_stance(ch)`

## THAC0 System

### Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `SKILL_THAC0_00` | 6 | Base THAC0 at level 0 |
| `SKILL_THAC0_32` | 18 | THAC0 at level 32+ |
| `TYPE_HIT` | 1000 | Base attack type offset |

### Interpolation

THAC0 scales linearly from level 0 to 32:
```c
thac0 = interpolate(level, 6, 18)
```

Lower THAC0 = better chance to hit.

### Hit Chance

```
hit_chance = 1 - (thac0 - victim_ac) / 20
```

Approximately 5% per point difference.

## Weapon Skill Improvement

### improve_wpn()

Called after each successful hit. Factors affecting improvement:
- Current skill level (harder at higher levels)
- Combat difficulty
- Weapon type used

## Attack Type Reference

| Index | Type | dt Value |
|-------|------|----------|
| 0 | Unarmed | 1000 |
| 1 | Slice | 1001 |
| 2 | Stab | 1002 |
| 3 | Slash | 1003 |
| 4 | Whip | 1004 |
| 5 | Claw | 1005 |
| 6 | Blast | 1006 |
| 7 | Pound | 1007 |
| 8 | Crush | 1008 |
| 9 | Grep | 1009 |
| 10 | Bite | 1010 |
| 11 | Pierce | 1011 |
| 12 | Suck | 1012 |
