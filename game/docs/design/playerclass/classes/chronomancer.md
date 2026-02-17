# Chronomancer Class Design

## Overview

Chronomancers are mages who have mastered the flow of time itself. They can accelerate allies, slow enemies, glimpse the future, and even rewind moments. Their unique resource, Temporal Flux, must be carefully balanced - too high causes instability, too low limits power. Mastering the flow between acceleration and deceleration is key. This is a **base class** that upgrades to Paradox.

**Source Files**: `src/classes/chronomancer.c`, `src/classes/chronomancer.h`
**Class Constant**: `CLASS_CHRONOMANCER` = 16777216 (2^24)
**Upgrades To**: Paradox

## Color Scheme

Chronomancer uses a copper/amber palette evoking clockwork brass and hourglass sand:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x130` | Deep copper (RGB 175,95,0) | Bracket arrows, decorative accents |
| Primary | `#x215` | Warm amber (RGB 255,175,95) | Class name, titles, ability highlights |
| Bracket open | `#x130[>#x215` | Copper[>Amber | Who list open bracket |
| Bracket close | `#x215<#x130]` | Amber<Copper] | Who list close bracket |
| Room tag | `#x215(#nChronomancer#x215)` | Amber parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Time Lord | `#x130[>#x215Time Lord#n#x215<#x130]` |
| 2 | Temporal Master | `#x130[>#x215Temporal Master#n#x215<#x130]` |
| 3 | Chronomancer | `#x130[>#x215Chronomancer#n#x215<#x130]` |
| 4 | Time Weaver | `#x130[>#x215Time Weaver#n#x215<#x130]` |
| 5 | Initiate | `#x130[>#x215Initiate#n#x215<#x130]` |
| default | Novice | `#x130[>#x215Novice#n#x215<#x130]` |

## Core Mechanics

### Temporal Flux

Temporal Flux is the balance-based resource that powers Chronomancer abilities, stored in `ch->rage`:

```c
ch->rage  // Current flux (0-100 for Chronomancer, 0-150 for Paradox)
```

**Starting Flux**: 50 (balanced)

**Flux Movement**:
- Acceleration abilities: INCREASE flux (+10-20)
- Deceleration abilities: DECREASE flux (-10-20)
- Combat: Drifts toward 50 (+/-2 per tick, toward center)

**The Balance Mechanic**:

| Flux Level | State | Effect |
|------------|-------|--------|
| 0-19 | Deep Slow | Acceleration abilities +30% power, Deceleration -50% power |
| 20-39 | Slow | Acceleration +15% power, Deceleration -25% power |
| 40-60 | Balanced | All abilities normal power |
| 61-80 | Fast | Deceleration +15% power, Acceleration -25% power |
| 81-100 | Deep Fast | Deceleration +30% power, Acceleration -50% power |

**Strategic Depth**: Push flux to extremes for powerful effects, but abilities of the opposite type become weaker. Stay balanced for versatility.

### Flux Instability

At extreme flux levels (0-10 or 90-100), there's a 10% chance per tick of temporal instability:

| Effect | Description |
|--------|-------------|
| Time Skip | Lose 1 combat round (stunned) |
| Echo | Random ability triggers twice |
| Rewind | Last damage taken is undone |
| Accelerate | Gain 1 extra attack this round |

### Flux Display

Command: `flux`
- Shows current flux with visual indicator of position (0-100 scale)
- Shows current state (Deep Slow/Slow/Balanced/Fast/Deep Fast)
- Lists power modifiers for each ability type
- Available to both Chronomancer and Paradox

## Training Categories

Chronomancer abilities are organized into 3 trainable categories. Each category has 3 levels. Stored in `ch->pcdata->powers[]`:

| Index | Category | Theme | Flux Direction | Abilities |
|-------|----------|-------|----------------|-----------|
| CHRONO_TRAIN_ACCEL (10) | Acceleration | Speed/haste | INCREASES flux | Quicken, Time Slip, Blur |
| CHRONO_TRAIN_DECEL (11) | Deceleration | Slow/control | DECREASES flux | Slow, Time Trap, Stasis |
| CHRONO_TRAIN_SIGHT (12) | Temporal Sight | Prediction/utility | NEUTRAL | Foresight, Hindsight, Temporal Echo |

**Training Cost**: `(current_level + 1) * 40` primal per level (`ability.chrono.train.cost_mult`)

**Training Command**: `timetrain` - Display current levels and improve categories

### Acceleration (Speed - Increases Flux)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `quicken` | Self haste buff |
| 2 | `timeslip` | Dodge next attack + counter |
| 3 | `blur` | Extreme speed, multiple attacks |

### Deceleration (Control - Decreases Flux)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `slow` | Reduce enemy attack speed |
| 2 | `timetrap` | Create a zone of slowed time |
| 3 | `stasis` | Freeze target completely |

### Temporal Sight (Utility - Neutral)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `foresight` | See enemy's next attack, dodge bonus |
| 2 | `hindsight` | Learn from past, damage bonus after being hit |
| 3 | `temporalecho` | Create a time-delayed copy of your attack |

## Abilities

### Quicken - Self Haste (Acceleration)

Accelerates personal time, gaining extra attacks.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `ability.chrono.quicken.mana_cost` |
| Flux Change | +15 | `ability.chrono.quicken.flux_change` |
| Duration | 8 ticks | `ability.chrono.quicken.duration` |
| Extra Attacks | +2 | `ability.chrono.quicken.extra_attacks` |
| Requires | Acceleration 1 | |

**Flux Scaling**: At flux 0-39, extra attacks become +3.

### Time Slip - Evasive Counter (Acceleration)

Slips forward in time to avoid an attack and counter.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `ability.chrono.timeslip.mana_cost` |
| Flux Change | +10 | `ability.chrono.timeslip.flux_change` |
| Duration | 1 attack | |
| Cooldown | 6 pulses | `ability.chrono.timeslip.cooldown` |
| Counter Damage | 100-200 + (50 - flux) × 2 | |
| Requires | Acceleration 2 | |

**Mechanic**: Next attack against you automatically misses, and you counter-attack. Lower flux = stronger counter.

### Blur - Extreme Speed (Acceleration)

Time accelerates to a blur, becoming nearly untouchable.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `ability.chrono.blur.mana_cost` |
| Flux Required | < 60 | |
| Flux Change | +25 | `ability.chrono.blur.flux_change` |
| Duration | 5 ticks | `ability.chrono.blur.duration` |
| Cooldown | 15 pulses | `ability.chrono.blur.cooldown` |
| Requires | Acceleration 3 | |

**Effects**:
- +4 extra attacks per round
- +40% dodge chance
- 25% chance for attacks to miss you entirely
- Can only be activated when flux < 60 (not already fast)

### Slow - Enemy Debuff (Deceleration)

Slows the target's personal time.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `ability.chrono.slow.mana_cost` |
| Flux Change | -12 | `ability.chrono.slow.flux_change` |
| Duration | 8 ticks | `ability.chrono.slow.duration` |
| Requires | Fighting, Deceleration 1 | |

**Effects**:
- Target loses 2 attacks per round
- Target's movement speed halved
- Target's dodge reduced by 20%

**Flux Scaling**: At flux 61-100, duration extends to 12 ticks.

### Time Trap - Zone of Slow (Deceleration)

Creates an area where time moves slowly.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `ability.chrono.timetrap.mana_cost` |
| Flux Change | -18 | `ability.chrono.timetrap.flux_change` |
| Duration | 6 ticks | `ability.chrono.timetrap.duration` |
| Cooldown | 12 pulses | `ability.chrono.timetrap.cooldown` |
| Requires | Deceleration 2 | |

**Effects**:
- All enemies in room affected by Slow effect
- Enemies entering room are automatically slowed
- Does not affect Chronomancer or allies
- Only one trap active at a time

### Stasis - Complete Freeze (Deceleration)

Stops time for a single target completely.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `ability.chrono.stasis.mana_cost` |
| Flux Required | > 40 | |
| Flux Change | -25 | `ability.chrono.stasis.flux_change` |
| Duration | 3 rounds | `ability.chrono.stasis.duration` |
| Cooldown | 20 pulses | `ability.chrono.stasis.cooldown` |
| Requires | Fighting, Deceleration 3 | |

**Effects**:
- Target cannot act for duration
- Target takes no damage while frozen (time stopped)
- Damage applied when stasis ends (stored)
- Max stored damage: 2000
- Can only be activated when flux > 40 (not already slow)

**Stored Damage**: All damage dealt while target is frozen is stored and applied instantly when stasis ends.

### Foresight - Combat Prediction (Temporal Sight)

Glimpse the immediate future to anticipate attacks.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `ability.chrono.foresight.mana_cost` |
| Duration | 10 ticks | `ability.chrono.foresight.duration` |
| Dodge Bonus | +20% | `ability.chrono.foresight.dodge_bonus` |
| Requires | Temporal Sight 1 | |

**Effects**:
- Shows enemy's target and intended attack type
- +20% dodge chance
- Warning message before big attacks

### Hindsight - Learning from Past (Temporal Sight)

After being hit, gain insight into the attacker's patterns.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `ability.chrono.hindsight.mana_cost` |
| Duration | 8 ticks | `ability.chrono.hindsight.duration` |
| Requires | Temporal Sight 2 | |

**Effects**:
- After being hit, gain stacking damage bonus against that attacker
- +5% damage per hit taken (max +25%)
- Stacks reset if you change targets
- "Learning" combat messages

### Temporal Echo - Delayed Attack (Temporal Sight)

Your attack echoes through time, hitting again moments later.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `ability.chrono.temporalecho.mana_cost` |
| Cooldown | 8 pulses | `ability.chrono.temporalecho.cooldown` |
| Requires | Fighting, Temporal Sight 3 | |

**Mechanic**:
1. Make a normal attack
2. 2 rounds later, an echo of that attack hits for 75% damage
3. Echo cannot be dodged or blocked (it's from the past)

**Synergy**: Works with any melee attack. The echo repeats whatever attack triggered it.

## Chronomancer Armor

Create equipment via `chronoarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| temporal staff | 33540 | Wield |
| hourglass ring | 33541 | Finger |
| timepiece amulet | 33542 | Neck |
| flux robes | 33543 | Torso |
| chrono circlet | 33544 | Head |
| time-touched pants | 33545 | Legs |
| phasing boots | 33546 | Feet |
| temporal gloves | 33547 | Hands |
| clockwork bracers | 33548 | Arms |
| cloak of moments | 33549 | About Body |
| temporal sash | 33550 | Waist |
| wrist sundial | 33551 | Wrist |
| mask of ages | 33552 | Face |
| mastery item | 33555 | Hold |

**Equipment Stats**:
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC
- Mastery: +50 hitroll, +50 damroll

**Restrictions**: Equipment restricted to CLASS_CHRONOMANCER and CLASS_PARADOX only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += balance.damcap_chrono_base;                    // +150 flat
// Flux position bonus (extremes grant more power)
int flux_extreme = abs(ch->rage - 50);  // 0-50 range
max_dam += flux_extreme * balance.damcap_chrono_extreme;  // +4 per point from center
if ( quicken active )
    max_dam += balance.damcap_chrono_quicken;              // +100
if ( blur active )
    max_dam += balance.damcap_chrono_blur;                 // +150
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Base | +150 flat | `combat.damcap.chrono.base` (default: 150) |
| Flux Extreme | +4 per point from center (max +200) | `combat.damcap.chrono.extreme_mult` (default: 4) |
| Quicken | +100 flat | `combat.damcap.chrono.quicken` (default: 100) |
| Blur | +150 flat | `combat.damcap.chrono.blur` (default: 150) |

### Flux-Based Ability Scaling

```c
int get_chrono_power_mod(CHAR_DATA *ch, bool is_accel) {
    int flux = ch->rage;
    if (is_accel) {
        if (flux <= 19) return 130;      // +30%
        if (flux <= 39) return 115;      // +15%
        if (flux <= 60) return 100;      // normal
        if (flux <= 80) return 75;       // -25%
        return 50;                        // -50%
    } else { // deceleration
        if (flux <= 19) return 50;       // -50%
        if (flux <= 39) return 75;       // -25%
        if (flux <= 60) return 100;      // normal
        if (flux <= 80) return 115;      // +15%
        return 130;                       // +30%
    }
}
```

### Tick Update

Per-tick processing:
1. Flux drift toward 50 (+/-2 per tick)
2. Instability check at extremes (0-10 or 90-100)
3. Quicken duration countdown
4. Blur duration countdown
5. Slow/Time Trap duration on enemies
6. Stasis stored damage tracking
7. Foresight/Hindsight duration countdown
8. Temporal Echo trigger check

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_CHRONOMANCER = 16777216 |
| flux | ch->rage | Current temporal flux (0-100) |
| quicken | ch->pcdata->powers[0] | Quicken ticks remaining |
| blur | ch->pcdata->powers[1] | Blur ticks remaining |
| time_slip | ch->pcdata->powers[2] | Time Slip ready (0/1) |
| foresight | ch->pcdata->powers[3] | Foresight ticks remaining |
| hindsight | ch->pcdata->powers[4] | Hindsight ticks remaining |
| hindsight_stacks | ch->pcdata->powers[5] | Hindsight damage stacks |
| echo_pending | ch->pcdata->powers[6] | Temporal Echo rounds until trigger |
| echo_damage | ch->pcdata->powers[7] | Temporal Echo stored damage |
| acceleration | ch->pcdata->powers[10] | Acceleration training level (0-3) |
| deceleration | ch->pcdata->powers[11] | Deceleration training level (0-3) |
| temporal_sight | ch->pcdata->powers[12] | Temporal Sight training level (0-3) |
| stasis_damage | ch->pcdata->stats[0] | Stasis stored damage |
| instability_count | ch->pcdata->stats[1] | Times instability triggered (tracking) |
