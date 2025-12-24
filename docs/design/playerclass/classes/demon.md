# Demon Class Design

## Overview

Demons are infernal beings who gain power through corruption and mutation. They spend demon points to acquire "warps" - permanent mutations that grant special abilities.

**Source File**: `src/demon.c`
**Class Constant**: `CLASS_DEMON` (1)

## Core Mechanics

### Demon Points

The primary resource for advancement:

```c
ch->pcdata->stats[DEMON_TOTAL]    // Total points ever earned
ch->pcdata->stats[DEMON_CURRENT]  // Current spendable points
```

- Earned through combat and corruption
- Spent on warps (15,000 points each)
- Total tracks lifetime progress

### Warp System

Warps are permanent mutations:

```c
ch->warp       // Bitfield of active warps
ch->warpcount  // Number of warps (max 18)
```

Each warp costs 15,000 demon points and grants permanent abilities.

## Warp Types

| Warp | Constant | Effect |
|------|----------|--------|
| Cybernetic Body | WARP_CBODY | Enhanced durability |
| Strong Body | WARP_SBODY | Increased strength |
| Strong Arms | WARP_STRONGARMS | Melee damage bonus |
| Venom Tongue | WARP_VENOMTONG | Poison attacks |
| Spiked Tail | WARP_STAIL | Tail attack |
| Clawed Hands | WARP_CLAWS | Natural weapons |
| Hooves | WARP_HOOVES | Kick damage |
| Wings | WARP_WINGS | Flight capability |
| Horns | WARP_HORNS | Gore attack |
| Armored Hide | WARP_ARMORHIDE | Natural armor |
| Infernal Sight | WARP_SIGHT | Enhanced vision |
| Hellfire Glands | WARP_HELLFIRE | Breath weapon |
| ... | ... | (18 total warps) |

## Disciplines

Demons have 9 disciplines:

| Discipline | Index | Theme |
|------------|-------|-------|
| Hellfire | DISC_DAEM_HELL | Fire attacks |
| Attack | DISC_DAEM_ATTA | Combat abilities |
| Temptation | DISC_DAEM_TEMP | Corruption, seduction |
| Morphing | DISC_DAEM_MORP | Shape alteration |
| Corruption | DISC_DAEM_CORR | Soul corruption |
| Gelugon | DISC_DAEM_GELU | Ice demon powers |
| Discord | DISC_DAEM_DISC | Chaos sowing |
| Nether | DISC_DAEM_NETH | Nether realm powers |
| Immunities | DISC_DAEM_IMMU | Damage resistances |

## Demon Affinity Flags

```c
ch->pcdata->demonic_a  // Demon-specific flags
```

Tracks active states and abilities.

## Key Commands

### Warps
- `warps` - View current warps
- `warp <type>` - Purchase a warp (costs 15,000 points)
- `unwarp <type>` - Remove a warp (if allowed)

### Combat
- `claws` - Extend demon claws (requires DISC_DAEM_ATTA)
- `hellfire` - Breathe hellfire (requires warp + discipline)
- `tailwhip` - Tail attack (requires WARP_STAIL)
- `gore` - Horn attack (requires WARP_HORNS)

### Abilities
- `demonform` - Transform to demon appearance
- `fly` - Take flight (requires WARP_WINGS)
- `infravision` - Enhanced sight (requires WARP_SIGHT)

### Status
- `demon` - View demon status and points

## Warp Mechanics

### Purchasing Warps

```c
void do_warp(CHAR_DATA *ch, char *argument) {
    if (ch->pcdata->stats[DEMON_CURRENT] < 15000) {
        send_to_char("You need 15,000 demon points.\n\r", ch);
        return;
    }

    if (ch->warpcount >= 18) {
        send_to_char("You have the maximum warps.\n\r", ch);
        return;
    }

    SET_BIT(ch->warp, warp_flag);
    ch->warpcount++;
    ch->pcdata->stats[DEMON_CURRENT] -= 15000;
}
```

### Warp Bonuses

Each warp provides specific bonuses:

```c
// Wings grant flight
if (IS_SET(ch->warp, WARP_WINGS)) {
    // Can use fly command
}

// Armored hide reduces damage
if (IS_SET(ch->warp, WARP_ARMORHIDE)) {
    damage_taken -= armor_reduction;
}

// Strong arms add damage
if (IS_SET(ch->warp, WARP_STRONGARMS)) {
    damage_dealt += strength_bonus;
}
```

## Combat Bonuses

### Discipline-Based
```c
// Attack discipline adds damage
damage += ch->power[DISC_DAEM_ATTA] * multiplier;

// Immunities reduce damage types
if (ch->power[DISC_DAEM_IMMU] >= fire_immunity_level) {
    fire_damage = 0;
}
```

### Warp-Based
```c
// Claws enable claw attacks
if (IS_SET(ch->warp, WARP_CLAWS)) {
    attacks += claw_attack;
}

// Tail adds extra attack
if (IS_SET(ch->warp, WARP_STAIL)) {
    attacks += tail_attack;
}
```

## Earning Demon Points

Points are earned through:
- Killing enemies
- Corrupting mortals
- Completing demonic objectives
- Special events

```c
// Award demon points
ch->pcdata->stats[DEMON_CURRENT] += points_earned;
ch->pcdata->stats[DEMON_TOTAL] += points_earned;
```

## Hellfire Abilities

Hellfire discipline provides fire-based attacks:

| Level | Ability |
|-------|---------|
| 1 | Minor flames |
| 2 | Fireball |
| 3 | Hellfire breath (requires warp) |
| 4 | Inferno |
| 5 | Immolate |

## Immunities Progression

Immunities discipline grants resistances:

| Level | Immunity |
|-------|----------|
| 1 | Fire resistance |
| 2 | Cold resistance |
| 3 | Poison immunity |
| 4 | Magic resistance |
| 5 | Holy resistance (partial) |

## Weaknesses

- **Holy**: Damaged by holy attacks
- **Blessed Weapons**: Extra damage
- **Holy Ground**: May be restricted
- **Exorcism**: Vulnerability to banishment

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_DEMON bit |
| disciplines | ch->power[] | Discipline levels |
| warp | ch->warp | Warp bitfield |
| warpcount | ch->warpcount | Total warps |
| demon_total | ch->pcdata->stats[DEMON_TOTAL] | Lifetime points |
| demon_current | ch->pcdata->stats[DEMON_CURRENT] | Spendable points |
| demonic_a | ch->pcdata->demonic_a | State flags |
