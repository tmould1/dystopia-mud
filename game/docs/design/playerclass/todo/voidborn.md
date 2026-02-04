# Voidborn Class Design

## Overview

Voidborn are beings who have surrendered their humanity to the void, becoming vessels for eldritch power. They can phase through reality, tear dimensional rents, transform into aberrant forms, and summon entities from beyond. Their corruption tolerance is higher, but so are the stakes - they walk the razor's edge between ultimate power and self-annihilation. This is an **upgrade class** obtained by upgrading from Cultist.

**Source Files**: `src/classes/voidborn.c`, `src/classes/cultist.h` (TBD)
**Class Constant**: `CLASS_VOIDBORN` (TBD - next available power-of-2)
**Upgrades From**: Cultist

## Color Scheme

Voidborn uses an otherworldly black-purple palette evoking the space between stars:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x016` | Void black | Bracket tildes, decorative accents |
| Primary | `#x097` | Eldritch purple | Class name, titles, ability highlights |
| Bracket open | `#x016~#x097(` | Black~Purple( | Who list open bracket |
| Bracket close | `#x097)#x016~` | Purple)Black~ | Who list close bracket |
| Room tag | `#x097(#nVoidborn#x097)` | Purple parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Void Incarnate | `#x016~#x097(#x097Void Incarnate#n#x097)#x016~` |
| 2 | Aberration | `#x016~#x097(#x097Aberration#n#x097)#x016~` |
| 3 | Voidspawn | `#x016~#x097(#x097Voidspawn#n#x097)#x016~` |
| 4 | Void Touched | `#x016~#x097(#x097Void Touched#n#x097)#x016~` |
| default | Voidborn | `#x016~#x097(#x097Voidborn#n#x097)#x016~` |

## Core Mechanics

### Corruption (Enhanced)

Voidborn inherits the Corruption system from Cultist with a higher cap and modified mechanics:

```c
ch->rage  // Current corruption (0-150 for Voidborn)
```

**Building Corruption**:
- Combat: +2 per game tick while fighting (vs +1 for Cultist)
- Void abilities: Higher gains than Cultist versions
- Kills: +8 per enemy killed (vs +5 for Cultist)
- Maximum: 150 (vs 100 for Cultist)

**Corruption Decay** (out of combat):
- -1 per tick when not fighting (slower than Cultist's -2)

### Enhanced Risk/Reward

**Power Scaling**: Damage bonus increased:
- Damage bonus: `corruption / 8`% extra damage (vs /10 for Cultist)
- At 150 corruption: +18.75% damage to all void abilities

**Self-Damage Penalty**: Extended tiers for higher corruption:

| Corruption | Damage per Tick | Description |
|------------|-----------------|-------------|
| 0-49 | None | Safe zone |
| 50-74 | 1% max HP | Light corruption burn |
| 75-99 | 2% max HP | Moderate corruption burn |
| 100-124 | 3% max HP | Heavy corruption burn |
| 125-150 | 4% max HP | Catastrophic corruption burn |

**Void Resistance**: Voidborn gain 25% resistance to their own corruption damage (effectively 0.75%, 1.5%, 2.25%, 3% per tier).

### Shared Cultist Abilities

Voidborn retains access to the `corruption` and `purge` commands. All other Cultist abilities are replaced by Voidborn-specific powers.

### Upgrade Path

Cultist upgrades to Voidborn via `do_upgrade`:
- Requires standard upgrade costs (50K hp, 35K mana/move, 40K qp, gen 1)
- Class changes from `CLASS_CULTIST` to `CLASS_VOIDBORN`
- All abilities reset; Voidborn abilities become available

## Training Categories

Voidborn abilities are organized into 3 trainable categories. Stored in `ch->pcdata->powers[]`:

| Index | Category | Max Level | Theme | Abilities |
|-------|----------|-----------|-------|-----------|
| VOID_TRAIN_WARP (10) | Reality Warp | 3 | Space distortion | Phase Shift, Dimensional Rend, Unmake |
| VOID_TRAIN_FORM (11) | Elder Form | 3 | Transformation | Void Shape, Aberrant Growth, Final Form |
| VOID_TRAIN_COSMIC (12) | Cosmic Horror | 3 | Ultimate powers | Summon Thing, Star Spawn, Entropy |

**Training Cost**: `(current_level + 1) * 50` primal per level

**Training Command**: `voidtrain` - Display current levels and improve categories

### Reality Warp (Space Distortion)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `phaseshift` | Become partially ethereal, dodge bonus |
| 2 | `dimensionalrend` | Tear reality, dealing damage and creating hazard |
| 3 | `unmake` | Attempt to erase target from reality |

### Elder Form (Transformation)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `voidshape` | Gain tentacles, extra attacks |
| 2 | `aberrantgrowth` | Mutate for combat bonuses |
| 3 | `finalform` | Complete transformation into aberration |

### Cosmic Horror (Ultimate Powers)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `summonthing` | Summon a void creature to fight |
| 2 | `starspawn` | Call down cosmic energy attack |
| 3 | `entropy` | Ultimate ability - accelerate decay |

## Abilities

### Phase Shift - Ethereal State

Shifts partially out of reality, becoming harder to hit.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `voidborn.phaseshift.mana_cost` |
| Corruption Gain | +10 | `voidborn.phaseshift.corruption_gain` |
| Duration | 8 ticks | `voidborn.phaseshift.duration` |
| Dodge Bonus | +25% | `voidborn.phaseshift.dodge_bonus` |
| Requires | Reality Warp 1 | |

**Effects**:
- Physical attacks have 25% miss chance against Voidborn
- Can pass through closed doors
- Cannot be grappled or held

### Dimensional Rend - Reality Tear

Tears a rift in reality, dealing damage and leaving a hazardous zone.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `voidborn.dimensionalrend.mana_cost` |
| Corruption Gain | +15 | `voidborn.dimensionalrend.corruption_gain` |
| Cooldown | 10 pulses | `voidborn.dimensionalrend.cooldown` |
| Initial Damage | 200-350 + (corruption × 3) | |
| Hazard Duration | 5 ticks | `voidborn.dimensionalrend.hazard_duration` |
| Hazard Damage | 50 per tick to all enemies | |
| Requires | Fighting, Reality Warp 2 | |

**Rift Hazard**: Leaves a tear in the room that damages all enemies each tick. Only one rift active at a time.

### Unmake - Reality Erasure

Attempts to erase the target from existence.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 180 | `voidborn.unmake.mana_cost` |
| Corruption Required | 100 | `voidborn.unmake.corruption_req` |
| Corruption Cost | 50 | `voidborn.unmake.corruption_cost` |
| Cooldown | 25 pulses | `voidborn.unmake.cooldown` |
| Base Damage | 500-800 + (corruption × 5) | |
| Instakill Threshold | NPCs < 1000 max HP | `voidborn.unmake.instakill_threshold` |
| Requires | Fighting, Reality Warp 3 | |

**Instakill**: NPCs below the HP threshold are erased instantly. Others take massive damage. Players cannot be instakilled but take full damage.

### Void Shape - Tentacle Form

Sprouts void tentacles, granting extra attacks.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `voidborn.voidshape.mana_cost` |
| Corruption Gain | +12 | `voidborn.voidshape.corruption_gain` |
| Duration | 10 ticks | `voidborn.voidshape.duration` |
| Extra Attacks | +2 | `voidborn.voidshape.extra_attacks` |
| Requires | Elder Form 1 | |

**Visual**: Voidborn's description shows writhing tentacles.

### Aberrant Growth - Combat Mutation

Mutates the body for enhanced combat capability.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `voidborn.aberrantgrowth.mana_cost` |
| Corruption Gain | +18 | `voidborn.aberrantgrowth.corruption_gain` |
| Duration | 8 ticks | `voidborn.aberrantgrowth.duration` |
| Damage Bonus | +25% | `voidborn.aberrantgrowth.damage_bonus` |
| Resistance | +15% all damage | `voidborn.aberrantgrowth.resistance` |
| Requires | Elder Form 2 | |

**Stacks with Void Shape**: Can be active simultaneously.

### Final Form - Complete Aberration

Transforms completely into an eldritch aberration.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 150 | `voidborn.finalform.mana_cost` |
| Corruption Required | 75 | `voidborn.finalform.corruption_req` |
| Corruption Gain | +30 | `voidborn.finalform.corruption_gain` |
| Duration | 6 ticks | `voidborn.finalform.duration` |
| Cooldown | 30 pulses | `voidborn.finalform.cooldown` |
| Requires | Elder Form 3 | |

**Effects**:
- +4 extra attacks
- +50% damage
- +25% resistance
- Immune to fear, charm, stun
- Cannot use non-Voidborn abilities
- Corruption decay paused (builds faster instead)

### Summon Thing - Void Creature

Tears open a rift and pulls through a creature from the void.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `voidborn.summonthing.mana_cost` |
| Corruption Gain | +20 | `voidborn.summonthing.corruption_gain` |
| Duration | 15 ticks | `voidborn.summonthing.duration` |
| Max Summons | 2 | `voidborn.summonthing.max_summons` |
| Requires | Cosmic Horror 1 | |

**Void Creature Stats**:
- HP: 30% of Voidborn's max HP
- Damage: Moderate void damage
- Abilities: Basic attack, chance to inflict madness
- Set as follower with AFF_CHARM

### Star Spawn - Cosmic Strike

Calls down a bolt of energy from the void between stars.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 140 | `voidborn.starspawn.mana_cost` |
| Corruption Gain | +25 | `voidborn.starspawn.corruption_gain` |
| Cooldown | 12 pulses | `voidborn.starspawn.cooldown` |
| Damage | 350-550 + (corruption × 4) | |
| AoE | Hits primary target + all enemies in room | |
| Secondary Damage | 50% to non-primary targets | |
| Requires | Fighting, Cosmic Horror 2 | |

### Entropy - Ultimate Decay

Accelerates the natural decay of all things around the Voidborn.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 200 | `voidborn.entropy.mana_cost` |
| Corruption Required | 125 | `voidborn.entropy.corruption_req` |
| Corruption Cost | 75 | `voidborn.entropy.corruption_cost` |
| Cooldown | 30 pulses | `voidborn.entropy.cooldown` |
| Duration | 5 ticks | `voidborn.entropy.duration` |
| Requires | Fighting, Cosmic Horror 3 | |

**Effects**:
- All enemies in room take 100-200 damage per tick
- Enemy regeneration reduced to 0 for duration
- Enemy buffs tick down twice as fast
- Voidborn also takes 50% of the tick damage (before resistance)

## Voidborn Armor

Create equipment via `voidbornarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| void scepter | TBD | Wield |
| ring of unmaking | TBD | Finger |
| collar of stars | TBD | Neck |
| aberrant robes | TBD | Torso |
| crown of madness | TBD | Head |
| void-woven leggings | TBD | Legs |
| phasing boots | TBD | Feet |
| tentacle gauntlets | TBD | Hands |
| dimensional bracers | TBD | Arms |
| shroud of entropy | TBD | About Body |
| sash of binding | TBD | Waist |
| rift bangle | TBD | Wrist |
| mask of the void | TBD | Face |

**Equipment Stats** (proposed):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_VOIDBORN only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += ch->rage * balance.damcap_voidborn_corrupt_mult;   // +7 per corruption
if ( final_form active )
    max_dam += balance.damcap_voidborn_finalform;              // +300
if ( aberrant_growth active )
    max_dam += balance.damcap_voidborn_aberrant;               // +150
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Corruption | +7 per point (max +1050) | `damcap_voidborn_corrupt_mult` (default: 7) |
| Final Form | +300 flat | `damcap_voidborn_finalform` (default: 300) |
| Aberrant Growth | +150 flat | `damcap_voidborn_aberrant` (default: 150) |

### Void Creature Management

- Maximum of 2 void creatures at once
- Creatures persist until duration expires or they die
- Creatures do not grant XP when they kill
- Creatures despawn if Voidborn leaves the area

### Tick Update

Per-tick processing:
1. Corruption build (+2 fighting, cap 150) or decay (-1 not fighting)
2. Corruption self-damage (with 25% resistance)
3. Peak corruption tracking
4. Phase Shift duration countdown
5. Dimensional Rend hazard processing
6. Void Shape/Aberrant Growth/Final Form duration countdown
7. Void creature duration countdown
8. Entropy AoE damage processing

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_VOIDBORN bit (TBD) |
| corruption | ch->rage | Current corruption (0-150) |
| phase_shift | ch->pcdata->powers[0] | Phase Shift ticks remaining |
| void_shape | ch->pcdata->powers[1] | Void Shape ticks remaining |
| aberrant_growth | ch->pcdata->powers[2] | Aberrant Growth ticks remaining |
| final_form | ch->pcdata->powers[3] | Final Form ticks remaining |
| rift_room | ch->pcdata->powers[4] | Room vnum of active rift |
| rift_ticks | ch->pcdata->powers[5] | Dimensional Rend rift ticks remaining |
| entropy_ticks | ch->pcdata->powers[6] | Entropy effect ticks remaining |
| summon_count | ch->pcdata->powers[7] | Number of active void creatures |
| reality_warp | ch->pcdata->powers[10] | Reality Warp training level (0-3) |
| elder_form | ch->pcdata->powers[11] | Elder Form training level (0-3) |
| cosmic_horror | ch->pcdata->powers[12] | Cosmic Horror training level (0-3) |
| peak_corruption | ch->pcdata->stats[0] | Highest corruption this session |
| void_damage_dealt | ch->pcdata->stats[1] | Total void damage dealt (tracking) |
