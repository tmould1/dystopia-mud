# Paradox Class Design

## Overview

Paradoxes are temporal anomalies who have transcended the normal flow of time. They can rewind damage, summon echoes of themselves from other timelines, create time loops, and ultimately unleash entropy itself. Having mastered and then broken the rules of time, they exist as living contradictions - beings who shouldn't exist yet wield tremendous power. This is an **upgrade class** obtained by upgrading from Chronomancer.

**Source Files**: `src/classes/paradox.c`, `src/classes/chronomancer.h` (TBD)
**Class Constant**: `CLASS_PARADOX` (TBD - next available power-of-2)
**Upgrades From**: Chronomancer

## Color Scheme

Paradox uses a fractured iridescent palette evoking broken time and impossible colors:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x129` | Shifting violet | Bracket tildes, decorative accents |
| Primary | `#x231` | Iridescent white | Class name, titles, ability highlights |
| Bracket open | `#x129~#x231(` | Violet~Iridescent( | Who list open bracket |
| Bracket close | `#x231)#x129~` | Iridescent)Violet~ | Who list close bracket |
| Room tag | `#x231(#nParadox#x231)` | Iridescent parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Temporal God | `#x129~#x231(#x231Temporal God#n#x231)#x129~` |
| 2 | Time Breaker | `#x129~#x231(#x231Time Breaker#n#x231)#x129~` |
| 3 | Paradox | `#x129~#x231(#x231Paradox#n#x231)#x129~` |
| 4 | Anomaly | `#x129~#x231(#x231Anomaly#n#x231)#x129~` |
| default | Glitch | `#x129~#x231(#x231Glitch#n#x231)#x129~` |

## Core Mechanics

### Temporal Flux (Enhanced)

Paradox inherits the Temporal Flux system from Chronomancer with expanded limits and new mechanics:

```c
ch->rage  // Current flux (0-150 for Paradox)
```

**Starting Flux**: 75 (center of expanded range)

**Expanded Range**: 0-150 (vs 0-100 for Chronomancer)

**Flux Movement**:
- Acceleration abilities: INCREASE flux (+15-30)
- Deceleration abilities: DECREASE flux (-15-30)
- Combat: Drifts toward 75 (+/-1 per tick, slower drift)

**Enhanced Balance Mechanic**:

| Flux Level | State | Effect |
|------------|-------|--------|
| 0-29 | Temporal Anchor | Acceleration +40% power, Deceleration -60% power, Immune to time effects |
| 30-59 | Slow | Acceleration +20% power, Deceleration -30% power |
| 60-90 | Balanced | All abilities normal power, can use Timeline abilities |
| 91-120 | Fast | Deceleration +20% power, Acceleration -30% power |
| 121-150 | Temporal Storm | Deceleration +40% power, Acceleration -60% power, Extra random effects |

### Controlled Instability

Unlike Chronomancer's random instability, Paradox can trigger instability effects voluntarily:

Command: `destabilize`
- Manually triggers an instability effect
- Costs 20 flux (can push toward center)
- 10 pulse cooldown

**Instability Effects** (expanded):
| Effect | Description |
|--------|-------------|
| Time Skip | Target loses 1 round (stunned) |
| Echo Strike | Your next attack hits twice |
| Temporal Rewind | Undo last 200 damage taken |
| Accelerate | Gain 2 extra attacks this round |
| Duplicate | Create a temporary clone (3 ticks) |
| Freeze Frame | All enemies skip 1 attack |

### Shared Chronomancer Abilities

Paradox retains access to the `flux` command. Chronomancer abilities are replaced by enhanced Paradox versions.

### Upgrade Path

Chronomancer upgrades to Paradox via `do_upgrade`:
- Requires standard upgrade costs (50K hp, 35K mana/move, 40K qp, gen 1)
- Class changes from `CLASS_CHRONOMANCER` to `CLASS_PARADOX`
- All abilities reset; Paradox abilities become available
- Flux range expands to 0-150

## Training Categories

Paradox abilities are organized into 3 trainable categories. Stored in `ch->pcdata->powers[]`:

| Index | Category | Max Level | Theme | Abilities |
|-------|----------|-----------|-------|-----------|
| PARA_TRAIN_TIMELINE (10) | Timeline | 3 | Major manipulation | Rewind, Split Timeline, Convergence |
| PARA_TRAIN_COMBAT (11) | Temporal Combat | 4 | Combat applications | Future Strike, Past Self, Time Loop, Paradox Strike |
| PARA_TRAIN_ENTROPY (12) | Entropy | 3 | Ultimate powers | Age, Temporal Collapse, Eternity |

**Training Cost**: `(current_level + 1) * 50` primal per level

**Training Command**: `paratrain` - Display current levels and improve categories

### Timeline (Major Manipulation)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `rewind` | Undo recent damage to self |
| 2 | `splittimeline` | Create branching reality, choose outcome |
| 3 | `convergence` | Collapse timelines for massive damage |

### Temporal Combat (Combat Applications)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `futurestrike` | Attack with damage from future hit |
| 2 | `pastself` | Summon echo of yourself to fight |
| 3 | `timeloop` | Enemy repeats same action, you counter |
| 4 | `paradoxstrike` | Attack that hits before it's made |

### Entropy (Ultimate Powers)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `age` | Rapidly age target, debuffs |
| 2 | `temporalcollapse` | AoE time destruction |
| 3 | `eternity` | Ultimate - exist outside time |

## Abilities

### Rewind - Undo Damage

Reverses time for yourself, undoing recent damage.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `paradox.rewind.mana_cost` |
| Flux Required | 40-110 (balanced range) | |
| Cooldown | 20 pulses | `paradox.rewind.cooldown` |
| Rewind Amount | Last 5 ticks of damage (max 1500) | `paradox.rewind.max_heal` |
| Requires | Timeline 1 | |

**Mechanic**: Tracks damage taken over last 5 ticks. Rewind restores that HP (up to max). Does not restore resources spent.

### Split Timeline - Branching Reality

Create two possible futures, then choose which one becomes real.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `paradox.splittimeline.mana_cost` |
| Flux Required | 50-100 (balanced) | |
| Duration | 3 rounds | `paradox.splittimeline.duration` |
| Cooldown | 30 pulses | `paradox.splittimeline.cooldown` |
| Requires | Timeline 2 | |

**Mechanic**:
1. Activate Split Timeline - two realities begin tracking
2. Fight normally for 3 rounds - both outcomes are recorded
3. Timeline A: What actually happens
4. Timeline B: Simulated alternative (RNG rerolled)
5. After 3 rounds, choose: `timeline A` or `timeline B`
6. Chosen timeline becomes real, other is erased

**Use Case**: If you take heavy damage in reality, choose the alternate timeline where you might have dodged.

### Convergence - Timeline Collapse

Collapse all possible timelines into a single point of destruction.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 180 | `paradox.convergence.mana_cost` |
| Flux Cost | 50 (toward center) | `paradox.convergence.flux_cost` |
| Cooldown | 35 pulses | `paradox.convergence.cooldown` |
| Base Damage | 500-800 | |
| Requires | Fighting, Timeline 3 | |

**Damage Calculation**:
- Base damage multiplied by number of "echoes" active
- +1 echo for each temporal ability used in last 10 ticks
- Maximum 5 echoes = 5× damage
- All echoes consumed on use

### Future Strike - Temporal Attack

Strike with the force of a blow that hasn't happened yet.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `paradox.futurestrike.mana_cost` |
| Flux Change | +15 | `paradox.futurestrike.flux_change` |
| Cooldown | 6 pulses | `paradox.futurestrike.cooldown` |
| Damage | 200-350 + (flux × 2) | |
| Requires | Fighting, Temporal Combat 1 | |

**Mechanic**: Damage is dealt now, but counts as coming from 2 rounds in the future. If the Paradox dies before then, damage is undone (enemy heals).

### Past Self - Summon Echo

Summon an echo of yourself from the recent past to fight alongside you.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `paradox.pastself.mana_cost` |
| Flux Change | -20 | `paradox.pastself.flux_change` |
| Duration | 8 ticks | `paradox.pastself.duration` |
| Cooldown | 25 pulses | `paradox.pastself.cooldown` |
| Requires | Temporal Combat 2 | |

**Past Self Stats**:
- HP: 40% of Paradox's max HP
- Damage: 60% of Paradox's damage
- Uses Paradox's hitroll/damroll
- Cannot use abilities (only basic attacks)
- Maximum 1 past self active

### Time Loop - Forced Repetition

Trap the enemy in a time loop, forcing them to repeat the same action.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 90 | `paradox.timeloop.mana_cost` |
| Flux Required | 40-110 (balanced) | |
| Duration | 3 rounds | `paradox.timeloop.duration` |
| Cooldown | 20 pulses | `paradox.timeloop.cooldown` |
| Requires | Fighting, Temporal Combat 3 | |

**Mechanic**:
- Enemy's first action in the loop is recorded
- For the next 3 rounds, enemy repeats that exact action
- Paradox knows what's coming - +30% dodge, +20% damage against looped enemy
- Looped enemy cannot use new abilities

### Paradox Strike - Impossible Attack

An attack that hits before it's even made - damage arrives before the swing.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `paradox.paradoxstrike.mana_cost` |
| Flux Cost | 30 (toward center) | |
| Cooldown | 12 pulses | `paradox.paradoxstrike.cooldown` |
| Damage | 350-550 + (flux deviation × 4) | |
| Requires | Fighting, Temporal Combat 4 | |

**Flux Deviation Bonus**: Damage increases based on distance from center (75). At flux 0 or 150, bonus is +300 damage.

**Special**: Cannot be dodged, blocked, or parried (the damage already happened).

### Age - Temporal Decay

Rapidly ages the target, weakening them severely.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `paradox.age.mana_cost` |
| Flux Change | -25 | `paradox.age.flux_change` |
| Duration | 10 ticks | `paradox.age.duration` |
| Cooldown | 15 pulses | `paradox.age.cooldown` |
| Requires | Fighting, Entropy 1 | |

**Effects**:
- -20% damage dealt
- -20% attack speed (lose 1 attack)
- -15% dodge
- Regeneration halved
- Visual: Target appears ancient and withered

### Temporal Collapse - Time Destruction

Collapse time itself in the area, dealing massive damage to all enemies.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 160 | `paradox.temporalcollapse.mana_cost` |
| Flux Required | < 30 OR > 120 (extreme) | |
| Flux Change | Moves 40 toward center | |
| Cooldown | 30 pulses | `paradox.temporalcollapse.cooldown` |
| Damage per Target | 400-700 + (flux extreme × 5) | |
| Requires | Fighting, Entropy 2 | |

**Flux Extreme Bonus**: Distance from center (75). At 0 or 150 flux, bonus is +375 damage.

**Effects**:
- All enemies in room take damage
- Removes all temporal buffs from enemies
- Strips 2 ticks from all enemy buff durations
- Room description temporarily distorted

### Eternity - Exist Outside Time

Step outside the flow of time entirely.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 250 | `paradox.eternity.mana_cost` |
| Flux Cost | All (resets to 75) | |
| Duration | 6 ticks | `paradox.eternity.duration` |
| Cooldown | 60 pulses | `paradox.eternity.cooldown` |
| Requires | Entropy 3 | |

**Eternity Effects**:
- Immune to all damage
- Immune to all crowd control
- Cannot be targeted by single-target abilities
- All your abilities cost 0 mana
- All your abilities have 0 cooldown
- Cannot gain or lose flux (frozen at 75)
- Attacks deal 50% damage (phased out of reality)
- When duration ends: Cannot act for 2 rounds (reintegrating)

## Paradox Armor

Create equipment via `paradoxarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| infinity staff | TBD | Wield |
| mobius ring | TBD | Finger |
| hourglass pendant | TBD | Neck |
| timeweave robes | TBD | Torso |
| crown of moments | TBD | Head |
| paradox leggings | TBD | Legs |
| boots of ages | TBD | Feet |
| temporal gauntlets | TBD | Hands |
| chrono-bracers | TBD | Arms |
| cloak of eternities | TBD | About Body |
| loop belt | TBD | Waist |
| timeline bracer | TBD | Wrist |
| mask of the void | TBD | Face |

**Equipment Stats** (proposed):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_PARADOX only.

## Combat Mechanics

### Damage Cap Bonuses

```c
int flux_extreme = abs(ch->rage - 75);  // 0-75 range for Paradox
max_dam += flux_extreme * balance.damcap_paradox_extreme;   // +5 per point from center
if ( past_self active )
    max_dam += balance.damcap_paradox_pastself;              // +100
if ( time_loop active )
    max_dam += balance.damcap_paradox_loop;                  // +150
if ( eternity active )
    max_dam -= max_dam / 2;                                  // -50% during Eternity
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Flux Extreme | +5 per point from center (max +375) | `damcap_paradox_extreme` (default: 5) |
| Past Self | +100 flat | `damcap_paradox_pastself` (default: 100) |
| Time Loop | +150 flat | `damcap_paradox_loop` (default: 150) |
| Eternity | -50% (phased) | N/A |

### Echo Tracking

Paradox tracks "echoes" - temporal resonance from ability use:
- Each temporal ability used adds +1 echo
- Echoes decay at -1 per 10 ticks
- Maximum 5 echoes
- Convergence consumes all echoes for multiplied damage

### Tick Update

Per-tick processing:
1. Flux drift toward 75 (+/-1 per tick)
2. Damage tracking for Rewind
3. Split Timeline recording
4. Past Self duration and combat
5. Time Loop tracking
6. Future Strike delayed effects
7. Age debuff duration
8. Echo count decay
9. Eternity duration and aftermath

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_PARADOX bit (TBD) |
| flux | ch->rage | Current temporal flux (0-150) |
| past_self | ch->pcdata->powers[0] | Past Self ticks remaining |
| time_loop | ch->pcdata->powers[1] | Time Loop rounds remaining |
| time_loop_target | ch->pcdata->powers[2] | Time Loop target ID |
| split_active | ch->pcdata->powers[3] | Split Timeline active (0/1) |
| age_target | ch->pcdata->powers[4] | Age effect target ID |
| age_ticks | ch->pcdata->powers[5] | Age ticks remaining |
| eternity | ch->pcdata->powers[6] | Eternity ticks remaining |
| eternity_aftermath | ch->pcdata->powers[7] | Eternity aftermath ticks |
| echo_count | ch->pcdata->powers[8] | Current echo count (0-5) |
| future_strike_target | ch->pcdata->powers[9] | Future Strike target ID |
| timeline | ch->pcdata->powers[10] | Timeline training level (0-3) |
| temporal_combat | ch->pcdata->powers[11] | Temporal Combat training level (0-4) |
| entropy | ch->pcdata->powers[12] | Entropy training level (0-3) |
| damage_history | ch->pcdata->stats[0-4] | Last 5 ticks damage for Rewind |
| eternities_used | ch->pcdata->stats[5] | Total Eternities used (tracking) |
