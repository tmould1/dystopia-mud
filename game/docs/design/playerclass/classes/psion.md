# Psion Class Design

## Overview

Psions are masters of mental energy who wield psionic powers to attack minds, manipulate objects with thought, and project psychic barriers. They build Focus through concentration and meditation, spending it to fuel devastating mental attacks that bypass physical armor. This is a **base class** that upgrades to Mindflayer.

**Source Files**: `src/classes/psion.c`, `src/classes/psion.h`
**Class Constant**: `CLASS_PSION` (65536)
**Upgrades To**: Mindflayer

## Color Scheme

Psion uses a blue/cyan palette evoking focused thought, mental clarity, and human psionic discipline:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x033` | Blue | Bracket chevron, decorative accents |
| Primary | `#x039` | Cyan | Class name, titles, ability highlights |
| Bracket open | `#x039<#x033|` | Cyan<Blue| | Who list open bracket (focused beam) |
| Bracket close | `#x033|#x039>` | Blue|Cyan> | Who list close bracket |
| Room tag | `#x039(#nPsion#x039)` | Cyan parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Mind Lord | `#x039<#x033|#x039Mind Lord#n#x033|#x039>` |
| 2 | Psychic Master | `#x039<#x033|#x039Psychic Master#n#x033|#x039>` |
| 3 | Psion | `#x039<#x033|#x039Psion#n#x033|#x039>` |
| 4 | Mentalist | `#x039<#x033|#x039Mentalist#n#x033|#x039>` |
| 5 | Adept | `#x039<#x033|#x039Adept#n#x033|#x039>` |
| default | Awakened | `#x039<#x033|#x039Awakened#n#x033|#x039>` |

## Core Mechanics

### Focus

Focus is the concentration resource that powers Psion abilities, stored in `ch->rage`:

```c
ch->rage  // Current focus (0-100 for Psion, 0-150 for Mindflayer)
```

**Building Focus** (`psion.c:651-667`):
- Meditation: `psimeditate` command grants +10 Focus (out of combat only)
- Combat: +1 per game tick while fighting (passive concentration)
- Thought Shield: +3 when absorbing damage
- Maximum: 100

**Focus Decay** (out of combat):
- -2 per tick when not fighting and not meditating
- Meditation prevents decay

**Peak Tracking**: Highest focus reached stored in `ch->pcdata->stats[PSION_PEAK_FOCUS]` (`psion.c:669-671`)

### Focus Display

Command: `psifocus` (`psion.c:20-62`)
- Shows current/max focus
- Lists all active psionic effects with remaining durations
- Available to both Psion and Mindflayer

**Note**: Named `psifocus` to avoid conflict with Samurai's `focus` command.

### Mental Damage

Psion abilities deal **mental damage** which:
- Bypasses physical armor (AC has no effect)
- Is reduced by target's willpower (Intelligence-based resistance)
- Cannot be blocked or parried
- Shows as "psychic" damage type in combat messages

## Training Categories

Psion abilities are organized into 3 trainable categories. Each category has 3 levels, unlocking abilities progressively. Stored in `ch->pcdata->powers[]`:

| Index | Category | Theme | Abilities |
|-------|----------|-------|-----------|
| PSION_TRAIN_TELEPATHY (10) | Telepathy | Detection/communication | Mindscan, Thought Shield, Mental Link |
| PSION_TRAIN_TELEKINESIS (11) | Telekinesis | Physical manipulation | Force Push, Levitate, Kinetic Barrier |
| PSION_TRAIN_COMBAT (12) | Psychic Combat | Direct mental damage | Mind Spike, Psychic Scream, Brain Burn |

**Training Cost**: `(current_level + 1) * 40` primal per level

**Training Command**: `psitrain` (`psion.c:99-187`) - Display current levels and improve categories

### Telepathy (Detection/Defense)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `mindscan` | Detect hidden/invisible creatures, read surface thoughts |
| 2 | `thoughtshield` | Absorb mental damage, builds Focus |
| 3 | `mentallink` | Telepathic communication, share buffs with linked ally |

### Telekinesis (Physical Manipulation)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `forcepush` | Knockback attack, chance to disarm |
| 2 | `levitate` | Flight, avoid ground-based attacks |
| 3 | `kineticbarrier` | Physical damage absorption shield |

### Psychic Combat (Offensive)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `mindspike` | Basic mental damage attack |
| 2 | `psychicscream` | AoE mental damage |
| 3 | `brainburn` | Heavy single-target, chance to stun |

## Abilities

### Psimeditate - Focus Recovery (`psion.c:67-94`)

Out-of-combat ability to build Focus through concentration. Command: `psimeditate`

**Note**: Named `psimeditate` to avoid conflict with the existing `meditate` command in act_move.c.

| Property | Value | Config Key |
|----------|-------|------------|
| Focus Gain | +10 | `psion.meditate.focus_gain` |
| Cooldown | 6 pulses | `psion.meditate.cooldown` |
| Requirement | Not fighting | |

**Mechanic**: Cannot be interrupted. Prevents Focus decay while active. Standing or sitting only.

### Mindscan - Mental Detection (`psion.c:192-248`)

Reveals hidden creatures and reads surface thoughts of a target.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 30 | `psion.mindscan.mana_cost` |
| Focus Required | 10 | `psion.mindscan.focus_req` |
| Duration | 8 ticks (detection aura) | `psion.mindscan.duration` |
| Requires | Telepathy 1 | |

**Effects**:
- Grants AFF_DETECT_HIDDEN and AFF_DETECT_INVIS for duration
- When targeting a specific creature: reveals their current HP%, mana%, and fighting target

### Thought Shield - Mental Defense (`psion.c:253-284`)

Creates a psychic barrier that absorbs mental damage and builds Focus.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `psion.thoughtshield.mana_cost` |
| Duration | 10 ticks | `psion.thoughtshield.duration` |
| Absorb Amount | 500 mental damage | `psion.thoughtshield.absorb_amount` |
| Focus Gain | +3 per hit absorbed | `psion.thoughtshield.focus_gain` |
| Requires | Telepathy 2 | |

**Mechanic**: Only absorbs mental/psychic damage types. Physical damage passes through normally.

### Mental Link - Telepathic Bond (`psion.c:289-341`)

Creates a mental bond with an ally, enabling telepathic communication and buff sharing.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `psion.mentallink.mana_cost` |
| Focus Cost | 20 | `psion.mentallink.focus_cost` |
| Duration | 15 ticks | `psion.mentallink.duration` |
| Range | Same area | |
| Requires | Telepathy 3 | |

**Effects**:
- Linked players can communicate telepathically (`think <message>`)
- When Psion casts a self-buff, linked ally receives 50% duration version
- Only one link active at a time

### Force Push - Telekinetic Attack (`psion.c:346-396`)

Slams the target with telekinetic force, dealing damage and potentially knocking them back.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `psion.forcepush.mana_cost` |
| Focus Cost | 5 | `psion.forcepush.focus_cost` |
| Cooldown | 4 pulses | `psion.forcepush.cooldown` |
| Base Damage | 80-160 + (focus × 2) | |
| Knockback Chance | 25% (forces flee) | `psion.forcepush.knockback_chance` |
| Disarm Chance | 15% | `psion.forcepush.disarm_chance` |
| Requires | Fighting, Telekinesis 1 | |

### Levitate - Telekinetic Flight (`psion.c:401-432`)

Lifts the Psion into the air, granting flight and evasion bonuses.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `psion.levitate.mana_cost` |
| Focus Cost | 15 | `psion.levitate.focus_cost` |
| Duration | 12 ticks | `psion.levitate.duration` |
| Dodge Bonus | +15% | `psion.levitate.dodge_bonus` |
| Requires | Telekinesis 2 | |

**Effects**:
- Grants AFF_FLYING
- Immune to ground-based attacks (earthquake, etc.)
- Cannot be tripped or knocked down

### Kinetic Barrier - Force Shield (`psion.c:438-478`)

Creates a telekinetic shield that absorbs physical damage.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `psion.kineticbarrier.mana_cost` |
| Focus Required | 30 | `psion.kineticbarrier.focus_req` |
| Focus Cost | 20 | `psion.kineticbarrier.focus_cost` |
| Duration | 10 ticks | `psion.kineticbarrier.duration` |
| Absorb Amount | 800 HP | `psion.kineticbarrier.absorb_amount` |
| Requires | Telekinesis 3 | |

**Mechanic**: Only absorbs physical damage. Mental damage passes through. Cannot stack with Thought Shield (must choose one).

### Mind Spike - Basic Mental Attack (`psion.c:483-524`)

Drives a spike of psychic energy into the target's mind.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 45 | `psion.mindspike.mana_cost` |
| Focus Gain | +4 | `psion.mindspike.focus_gain` |
| Cooldown | 4 pulses | `psion.mindspike.cooldown` |
| Base Damage | 100-200 + (focus × 2) | |
| Damage Type | Mental (bypasses armor) | |
| Requires | Fighting, Psychic Combat 1 | |

### Psychic Scream - AoE Mental Attack (`psion.c:529-582`)

Unleashes a wave of psychic energy hitting all enemies in combat with the Psion.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 90 | `psion.psychicscream.mana_cost` |
| Focus Required | 35 | `psion.psychicscream.focus_req` |
| Focus Cost | 15 | `psion.psychicscream.focus_cost` |
| Cooldown | 10 pulses | `psion.psychicscream.cooldown` |
| Damage per Target | 120-240 + (focus × 2) | |
| Damage Type | Mental (bypasses armor) | |
| Requires | Fighting, Psychic Combat 2 | |

### Brain Burn - Heavy Mental Strike (`psion.c:587-639`)

Overloads the target's mind with psychic fire, dealing heavy damage with a chance to stun.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `psion.brainburn.mana_cost` |
| Focus Required | 50 | `psion.brainburn.focus_req` |
| Focus Cost | 25 | `psion.brainburn.focus_cost` |
| Cooldown | 12 pulses | `psion.brainburn.cooldown` |
| Base Damage | 300-500 + (focus × 4) | |
| Stun Chance | 20% (2 rounds) | `psion.brainburn.stun_chance` |
| Damage Type | Mental (bypasses armor) | |
| Requires | Fighting, Psychic Combat 3 | |

## Psion Armor

Create equipment via `psionarmor <piece>` (`psion.c:644-646`):

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| psychic focus | (classeq.db) | Wield |
| ring | (classeq.db) | Finger |
| amulet | (classeq.db) | Neck |
| mindweave robe | (classeq.db) | Torso |
| psionic circlet | (classeq.db) | Head |
| leggings | (classeq.db) | Legs |
| sandals | (classeq.db) | Feet |
| gloves | (classeq.db) | Hands |
| bracers | (classeq.db) | Arms |
| cloak | (classeq.db) | About Body |
| sash | (classeq.db) | Waist |
| wristband | (classeq.db) | Wrist |
| third eye gem | (classeq.db) | Face |

**Equipment Stats** (from `classeq.db`):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_PSION and CLASS_MINDFLAYER only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += balance.damcap_psion_base;                    // +150 flat
max_dam += ch->rage * balance.damcap_psion_focus_mult;   // +6 per focus
if ( thought_shield active )
    max_dam += balance.damcap_psion_thoughtshield;        // +100
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Base | +150 flat | `damcap_psion_base` (default: 150) |
| Focus | +6 per point (max +600) | `damcap_psion_focus_mult` (default: 6) |
| Thought Shield | +100 flat | `damcap_psion_thoughtshield` (default: 100) |

### Mental Damage Processing

Mental damage abilities:
1. Ignore target's AC completely
2. Reduced by `target_int / 10`% (Intelligence-based resistance)
3. Cannot be blocked, parried, or dodged
4. Not affected by sanctuary (pure mental assault)

### Tick Update (`psion.c:651-716`)

Per-tick processing:
1. Focus build (+1 fighting) or decay (-2 not fighting, unless meditating)
2. Thought Shield duration countdown
3. Kinetic Barrier duration countdown
4. Levitate duration countdown
5. Mental Link duration countdown
6. Mindscan detection duration countdown

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_PSION bit (65536) |
| focus | ch->rage | Current focus (0-100) |
| thought_shield | ch->pcdata->powers[0] | Thought Shield ticks remaining |
| kinetic_barrier | ch->pcdata->powers[1] | Kinetic Barrier ticks remaining |
| levitate | ch->pcdata->powers[2] | Levitate ticks remaining |
| mental_link | ch->pcdata->powers[3] | Mental Link ticks remaining |
| mindscan | ch->pcdata->powers[4] | Mindscan detection ticks remaining |
| linked_player | ch->pcdata->powers[5] | Mental Link target (player ID) |
| telepathy | ch->pcdata->powers[10] | Telepathy training level (0-3) |
| telekinesis | ch->pcdata->powers[11] | Telekinesis training level (0-3) |
| psychic_combat | ch->pcdata->powers[12] | Psychic Combat training level (0-3) |
| thought_shield_hp | ch->pcdata->stats[0] | Current thought shield HP |
| kinetic_barrier_hp | ch->pcdata->stats[1] | Current kinetic barrier HP |
| peak_focus | ch->pcdata->stats[2] | Highest focus this session |
