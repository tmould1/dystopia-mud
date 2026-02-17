# Cultist Class Design

## Overview

Cultists are devotees of the void who channel eldritch energies from beyond reality. They wield forbidden knowledge, summon tentacles of void-stuff, and drive their enemies mad with whispers from the outer dark. Their power comes with a price: Corruption builds as they use void abilities, granting increased power but causing periodic self-damage. This is a **base class** that upgrades to Voidborn.

**Source Files**: `src/classes/cultist.c`, `src/classes/cultist.h`
**Class Constant**: `CLASS_CULTIST` (4194304)
**Upgrades To**: Voidborn

## Color Scheme

Cultist uses a sickly olive-green palette evoking corruption, decay, and unnatural energies:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x064` | Dark olive | Bracket decorations, ritual accents |
| Primary | `#x120` | Sickly lime green | Class name, titles, ability highlights |
| Bracket open | `#x064{~#x120[` | Olive{~Green[ | Who list open bracket |
| Bracket close | `#x120]#x064~}` | Green]Olive~} | Who list close bracket |
| Room tag | `#x120(#nCultist#x120)` | Green parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | High Priest | `#x064{~#x120[#x120High Priest#n#x120]#x064~}` |
| 2 | Void Touched | `#x064{~#x120[#x120Void Touched#n#x120]#x064~}` |
| 3 | Cultist | `#x064{~#x120[#x120Cultist#n#x120]#x064~}` |
| 4 | Initiate | `#x064{~#x120[#x120Initiate#n#x120]#x064~}` |
| 5 | Seeker | `#x064{~#x120[#x120Seeker#n#x120]#x064~}` |
| default | Acolyte | `#x064{~#x120[#x120Acolyte#n#x120]#x064~}` |

## Core Mechanics

### Corruption

Corruption is the risk/reward resource that powers Cultist abilities, stored in `ch->rage`:

```c
ch->rage  // Current corruption (0-100 for Cultist, 0-150 for Voidborn)
```

**Building Corruption**:
- Void abilities: +5-20 per ability used (varies by power)
- Kills: +5 per enemy killed
- Passive: +1 per tick while fighting (void seeps in)
- Maximum: 100

**Corruption Decay** (out of combat):
- -2 per tick when not fighting
- Can use `purge` command to rapidly drain corruption (costs HP)

### Corruption Risk/Reward

**Power Scaling**: Many abilities deal bonus damage based on current corruption:
- Damage bonus: `corruption / 10`% extra damage
- At 100 corruption: +10% damage to all void abilities

**Self-Damage Penalty**: Above 50 corruption, take void damage each tick:

| Corruption | Damage per Tick | Description |
|------------|-----------------|-------------|
| 0-49 | None | Safe zone |
| 50-74 | 1% max HP | Light corruption burn |
| 75-99 | 2% max HP | Moderate corruption burn |
| 100 | 3% max HP | Heavy corruption burn |

**Strategic Choice**: Push corruption high for maximum damage, but risk killing yourself. Manage it carefully or suffer the consequences.

### Corruption Display

Command: `corruption`
- Shows current/max corruption with color-coded warning
- Lists corruption damage tier
- Shows all active void effects with remaining durations
- Available to both Cultist and Voidborn

### Purge Command

Emergency corruption management:

| Property | Value | Config Key |
|----------|-------|------------|
| HP Cost | 5% max HP | `cultist.purge.hp_cost` |
| Corruption Removed | -25 | `cultist.purge.corruption_removed` |
| Cooldown | 8 pulses | `cultist.purge.cooldown` |

## Training Categories

Cultist abilities are organized into 3 trainable categories. Each category has 3 levels, unlocking abilities progressively. Stored in `ch->pcdata->powers[]`:

| Index | Category | Theme | Abilities |
|-------|----------|-------|-----------|
| CULT_TRAIN_LORE (10) | Forbidden Lore | Knowledge/utility | Eldritch Sight, Whispers, Unravel |
| CULT_TRAIN_TENTACLE (11) | Tentacle Arts | Physical void attacks | Void Tendril, Grasp, Constrict |
| CULT_TRAIN_MADNESS (12) | Madness | Sanity-draining | Maddening Gaze, Gibbering, Insanity |

**Training Cost**: `(current_level + 1) * 40` primal per level

**Training Command**: `voidtrain` - Display current levels and improve categories

### Forbidden Lore (Knowledge/Utility)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `eldritchsight` | See hidden, detect magic, sense void creatures |
| 2 | `whispers` | Voices reveal target's weaknesses |
| 3 | `unravel` | Strip magical protections from target |

### Tentacle Arts (Physical Attacks)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `voidtendril` | Basic void damage attack |
| 2 | `grasp` | Grapple target, preventing flee |
| 3 | `constrict` | Crushing damage over time |

### Madness (Sanity Attacks)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `maddeninggaze` | Single-target sanity damage + debuff |
| 2 | `gibbering` | AoE confusion effect |
| 3 | `insanity` | Heavy sanity damage, chance of self-attack |

## Abilities

### Eldritch Sight - Void Detection

Opens the mind's eye to perceive what others cannot see.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 30 | `cultist.eldritchsight.mana_cost` |
| Corruption Gain | +3 | `cultist.eldritchsight.corruption_gain` |
| Duration | 10 ticks | `cultist.eldritchsight.duration` |
| Requires | Forbidden Lore 1 | |

**Effects**:
- Grants AFF_DETECT_HIDDEN, AFF_DETECT_INVIS, AFF_DETECT_MAGIC
- Can sense void creatures (shows "void-touched" flag on look)
- Reveals cursed/corrupted items

### Whispers - Weakness Detection

The void speaks, revealing the target's vulnerabilities.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `cultist.whispers.mana_cost` |
| Corruption Gain | +5 | `cultist.whispers.corruption_gain` |
| Debuff Duration | 8 ticks | `cultist.whispers.debuff_duration` |
| Requires | Fighting, Forbidden Lore 2 | |

**Effects**:
- Reveals target's HP%, resistances, and current buffs
- Applies "Exposed" debuff: target takes +10% damage from all sources
- Corruption scaling: At 50+ corruption, debuff increases to +15%

### Unravel - Dispel Magic

Tears apart magical protections with void energy.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `cultist.unravel.mana_cost` |
| Corruption Gain | +10 | `cultist.unravel.corruption_gain` |
| Cooldown | 10 pulses | `cultist.unravel.cooldown` |
| Max Affects Removed | 3 | `cultist.unravel.max_removes` |
| Requires | Fighting, Forbidden Lore 3 | |

**Effects**:
- Removes up to 3 magical affects from target
- Removes AFF_SANCTUARY, AFF_PROTECT if present
- Corruption scaling: At 75+ corruption, removes up to 5 affects

### Void Tendril - Basic Attack

Lashes out with a tendril of pure void-stuff.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `cultist.voidtendril.mana_cost` |
| Corruption Gain | +5 | `cultist.voidtendril.corruption_gain` |
| Cooldown | 4 pulses | `cultist.voidtendril.cooldown` |
| Base Damage | 80-150 + (corruption × 2) | |
| Damage Type | Void (cold/negative hybrid) | |
| Requires | Fighting, Tentacle Arts 1 | |

**Corruption Bonus**: Damage increased by `corruption / 10`%

### Grasp - Void Grapple

A void tendril wraps around the target, preventing escape.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `cultist.grasp.mana_cost` |
| Corruption Gain | +8 | `cultist.grasp.corruption_gain` |
| Duration | 4 ticks | `cultist.grasp.duration` |
| Damage per Tick | 30 | `cultist.grasp.tick_damage` |
| Requires | Fighting, Tentacle Arts 2 | |

**Effects**:
- Target cannot flee or be rescued
- Target cannot use movement abilities
- Damage each tick while grasped
- Corruption scaling: At 50+ corruption, duration extends to 6 ticks

### Constrict - Crushing Damage

The void tendril squeezes with crushing force.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `cultist.constrict.mana_cost` |
| Corruption Gain | +12 | `cultist.constrict.corruption_gain` |
| Cooldown | 8 pulses | `cultist.constrict.cooldown` |
| Duration | 5 ticks | `cultist.constrict.duration` |
| Tick Damage | 60-100 + (corruption × 1) | |
| Requires | Target must be grasped, Tentacle Arts 3 | |

**Mechanic**: Can only be used on a target currently affected by Grasp. Adds additional DoT.

### Maddening Gaze - Sanity Attack

Locks eyes with the target, forcing them to see the void.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `cultist.maddeninggaze.mana_cost` |
| Corruption Gain | +7 | `cultist.maddeninggaze.corruption_gain` |
| Cooldown | 6 pulses | `cultist.maddeninggaze.cooldown` |
| Base Damage | 100-180 + (corruption × 2) | |
| Debuff | -15% hitroll for 5 ticks | |
| Damage Type | Mental/Void | |
| Requires | Fighting, Madness 1 | |

### Gibbering - AoE Confusion

Speaks in tongues that no mortal mind can comprehend.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 90 | `cultist.gibbering.mana_cost` |
| Corruption Gain | +15 | `cultist.gibbering.corruption_gain` |
| Cooldown | 12 pulses | `cultist.gibbering.cooldown` |
| Confusion Duration | 3 ticks | `cultist.gibbering.confusion_duration` |
| NPC Flee Chance | 30% | `cultist.gibbering.flee_chance` |
| Requires | Fighting, Madness 2 | |

**Confusion Effect**: Affected targets have 25% chance each round to attack random target (including allies).

### Insanity - Ultimate Madness

Completely shatters the target's grip on reality.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `cultist.insanity.mana_cost` |
| Corruption Gain | +20 | `cultist.insanity.corruption_gain` |
| Cooldown | 15 pulses | `cultist.insanity.cooldown` |
| Base Damage | 250-400 + (corruption × 3) | |
| Self-Attack Chance | 20% (NPC attacks self for 3 rounds) | `cultist.insanity.selfattack_chance` |
| Damage Type | Mental/Void | |
| Requires | Fighting, Madness 3 | |

**Self-Attack**: Affected NPCs attack themselves for duration, dealing normal melee damage.

## Cultist Armor

Create equipment via `cultistarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| void staff | 33500 | Wield |
| sigil ring | 33501 | Finger |
| eldritch amulet | 33502 | Neck |
| cultist robes | 33503 | Torso |
| hood of whispers | 33504 | Head |
| ritual leggings | 33505 | Legs |
| void-touched boots | 33506 | Feet |
| sacrificial gloves | 33507 | Hands |
| bracers of binding | 33508 | Arms |
| shroud of madness | 33509 | About Body |
| ritual sash | 33510 | Waist |
| tentacle bangle | 33511 | Wrist |
| mask of the deep | 33512 | Face |
| mastery item | 33515 | Hold |

**Equipment Stats** (proposed):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_CULTIST and CLASS_VOIDBORN only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += balance.damcap_cultist_base;                      // +150 flat
max_dam += ch->rage * balance.damcap_cultist_corrupt_mult;   // +5 per corruption
// High corruption bonus
if (ch->rage >= 75)
    max_dam += balance.damcap_cultist_high_corrupt;          // +150 at 75+ corruption
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Base | +150 flat | `damcap_cultist_base` (default: 150) |
| Corruption | +5 per point (max +500) | `damcap_cultist_corrupt_mult` (default: 5) |
| High Corruption | +150 at 75+ | `damcap_cultist_high_corrupt` (default: 150) |

### Void Damage Type

Void damage:
- Hybrid of cold and negative energy
- Reduced by cold resistance (50% effect)
- Reduced by negative resistance (50% effect)
- Not affected by sanctuary (void bypasses divine protection)

### Corruption Self-Damage

Processed in tick update:
```c
if (ch->rage >= 50) {
    int tier = (ch->rage >= 100) ? 3 : (ch->rage >= 75) ? 2 : 1;
    int damage = ch->max_hit * tier / 100;  // 1%, 2%, or 3%
    ch->hit -= damage;
    // Cannot kill, leaves at 1 HP
    if (ch->hit < 1) ch->hit = 1;
}
```

### Tick Update

Per-tick processing:
1. Corruption build (+1 fighting) or decay (-2 not fighting)
2. Corruption self-damage (if above 50)
3. Eldritch Sight duration countdown
4. Whispers debuff duration countdown
5. Grasp duration/damage
6. Constrict duration/damage
7. Gibbering confusion duration countdown

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_CULTIST bit (4194304) |
| corruption | ch->rage | Current corruption (0-100) |
| eldritch_sight | ch->pcdata->powers[0] | Eldritch Sight ticks remaining |
| grasp_target | ch->pcdata->powers[1] | Grasp target (char ID) |
| grasp_ticks | ch->pcdata->powers[2] | Grasp ticks remaining |
| constrict_ticks | ch->pcdata->powers[3] | Constrict ticks remaining |
| forbidden_lore | ch->pcdata->powers[10] | Forbidden Lore training level (0-3) |
| tentacle_arts | ch->pcdata->powers[11] | Tentacle Arts training level (0-3) |
| madness | ch->pcdata->powers[12] | Madness training level (0-3) |
| peak_corruption | ch->pcdata->stats[0] | Highest corruption this session |
| void_damage_dealt | ch->pcdata->stats[1] | Total void damage dealt (tracking) |
