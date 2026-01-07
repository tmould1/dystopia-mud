# Defense Mechanics

This document details parry, dodge, and armor class calculations.

## Armor Class

### char_ac() Function

**Location:** [act_info.c:562+](../../src/commands/act_info.c#L562)

### Base Calculation

```c
ac = GET_AC(ch)  // Sum of equipped armor values
if (awake) ac += dex_app[get_curr_dex(ch)].defensive
```

### Class Adjustments

| Class | Adjustment |
|-------|------------|
| Samurai | `-wpn[1]` (weapon skill improves AC) |

### Cap

```c
ac = UMIN(ac, 7000)  // Maximum 7000 AC
```

### AC in Combat

In `one_hit()`, AC is converted for THAC0 calculation:
```c
victim_ac = UMAX(-100, char_ac(victim) / 10)
```

### Superstance AC Penalties

When attacker has superstance damage powers:

| Power | Victim AC Penalty |
|-------|-------------------|
| STANCEPOWER_DAMAGE_1 | +100 |
| STANCEPOWER_DAMAGE_2 | +200 |
| STANCEPOWER_DAMAGE_3 | +300 |

## Parry

### check_parry() Function

**Location:** [fight.c:2328-2585](../../src/combat/fight.c#L2328-L2585)

Returns TRUE if victim successfully parries the attack.

### Base Calculation

```c
chance = 0
chance -= attacker_wpn_skill * 0.1   // Attacker weapon reduces parry
chance += victim_wpn_skill * 0.5     // Victim weapon helps parry
```

### Stance Bonuses (Victim)

| Stance | Bonus |
|--------|-------|
| STANCE_CRANE | +stance_level * 0.25 |
| STANCE_MANTIS | +stance_level * 0.25 |
| Superstance PARRY | +stance_level * 0.25 |

### Attacker Penalties (Reduces Victim Parry)

These make the attacker harder to parry:

| Class/Effect | Penalty |
|--------------|---------|
| Shapeshifter forms | -2 to -11 per level |
| Tanarri powers | -5 to -17 |
| Angel Justice | -9 per point |
| Vampire Celerity | -3 per point |
| Ninja skill | -2 to -25 |
| Demon Speed | -25 |
| Monk Chi | -7 per point |
| Undead Knight | -3.5 per point |
| Mage Beast form | -30 |
| Drow Arms | -30 |
| Werewolf Mantis | -3 per point |
| Samurai | -25 |
| Droid limbs | -4 per point |
| Droid upgrade | -10 per level |
| Lich | -40 |
| ITEMAFF_AFFMANTIS | -12 |
| NEW_MONKFLAME | -3 |

### Victim Bonuses (Improves Parry)

| Class/Effect | Bonus |
|--------------|-------|
| Shapeshifter | +2 to +6 per level |
| Tanarri powers | +5 to +17 |
| Angel Peace | +9 per point |
| Vampire Celerity | +3 per point |
| Ninja skill | +2 to +25 |
| Demon Speed | +25 |
| Monk Chi | +8 per point |
| Undead Knight | +4-6 per point |
| Mage Deflector | +40 |
| Samurai | +25 |
| Droid limbs | +4 per point |
| Lich | +40 |
| NEW_MONKFLAME | +4 |
| Monkblock counter | +monkblock/4 |

### Claw Parry (Unarmed)

Special case for parrying without a weapon:

**Eligible if:**
- Werewolf with DISC_WERE_BEAR > 2
- Monk with NEW_MONKADAM
- Shapeshifter

Provides hitroll bonus instead of weapon skill bonus.

### Final Chance

```c
chance = URANGE(20, chance, 80)  // Clamped to 20-80%
```

### Special Cases

- **JFLAG_BULLY:** Maximum 20% parry chance
- **Drow Sacrificial Weapons:** 50% chance to bypass all defenses

## Dodge

### check_dodge() Function

**Location:** [fight.c:2601-2790](../../src/combat/fight.c#L2601-L2790)

Returns TRUE if victim successfully dodges the attack.

### Base Calculation

```c
chance = 0
chance -= attacker_wpn_skill * 0.1   // Attacker weapon reduces dodge
chance += victim_wpn_unarmed * 0.5   // Victim unarmed skill helps dodge
```

### Stance Bonuses (Victim)

| Stance | Bonus |
|--------|-------|
| STANCE_MONGOOSE | +stance_level * 0.25 |
| STANCE_SWALLOW | +stance_level * 0.25 |
| Superstance DODGE | +stance_level * 0.25 |

### Attacker Penalties

Similar to parry, ranging from -9 to -25 per ability level.

### Victim Bonuses

| Class/Effect | Bonus |
|--------------|-------|
| Shapeshifter | +6 to +11 per level |
| Tanarri | +5 to +17 |
| Angel | +9 per point |
| Vampire | +3 per point |
| Ninja | +2 to +30 |
| Demon | +25 |
| Monk | +3 per chi point |
| Werewolf Mantis | +3.5 per point |
| Drow Speed | +20 to +50 |
| Samurai | +25 |
| Undead Knight | +4.5 per point |
| Droid | +10 per limb |
| Lich | +40 |
| ITEMAFF_AFFMANTIS | +15 |
| Monkblock | +monkblock/2 |

### Final Chance

```c
chance = URANGE(20, chance, 90)  // Clamped to 20-90%
```

### Special Cases

Same JFLAG_BULLY check as parry (max 20%).

## Defense Summary

| Defense | Base Skill | Key Stances | Range |
|---------|------------|-------------|-------|
| Parry | Weapon skill | Crane, Mantis | 20-80% |
| Dodge | Unarmed skill | Mongoose, Swallow | 20-90% |
| AC | Equipment | - | -100 to 7000 |

## Combat Resolution Order

1. Roll to hit (THAC0 vs AC)
2. If hit, check parry
3. If not parried, check dodge
4. If not dodged, apply damage
