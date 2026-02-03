# Dirgesinger Class Design

## Overview

Dirgesingers are martial bards who channel sonic energy through war chants, battle hymns, and mournful dirges. They build Resonance during combat, spending it to fuel devastating sonic attacks and empowering songs. This is a **base class** that upgrades to Siren.

**Source Files**: `src/classes/dirgesinger.c`, `src/classes/dirgesinger.h`
**Class Constant**: `CLASS_DIRGESINGER` (16384)
**Upgrades To**: Siren

## Color Scheme

Dirgesinger uses a warm bronze-gold palette evoking brass instruments and war horns:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x136` | Dark bronze/copper | Bracket tildes, decorative accents |
| Primary | `#x178` | Warm gold | Class name, titles, ability highlights |
| Bracket open | `#x136~#x178[` | Bronze~Gold[ | Who list open bracket |
| Bracket close | `#x178]#x136~` | Gold]Bronze~ | Who list close bracket |
| Room tag | `#x178(#nDirgesinger#x178)` | Gold parens | Room display prefix |

**Who List Titles** (`act_info.c:3024-3036`):

| Generation | Title | Display |
|------------|-------|---------|
| 1 | War Cantor | `#x136~#x178[#x178War Cantor#n#x178]#x136~` |
| 2 | Battle Bard | `#x136~#x178[#x178Battle Bard#n#x178]#x136~` |
| 3 | Dirgesinger | `#x136~#x178[#x178Dirgesinger#n#x178]#x136~` |
| 4 | War Chanter | `#x136~#x178[#x178War Chanter#n#x178]#x136~` |
| 5 | Chanter | `#x136~#x178[#x178Chanter#n#x178]#x136~` |
| default | Hummer | `#x136~#x178[#x178Hummer#n#x178]#x136~` |

## Core Mechanics

### Resonance

Resonance is the combat resource that powers Dirgesinger abilities, stored in `ch->rage`:

```c
ch->rage  // Current resonance (0-100 for Dirgesinger, 0-150 for Siren)
```

**Building Resonance** (`dirgesinger.c:460-468`):
- Combat: +2 per game tick while fighting
- Warcry: +5 per use (`acfg: dirgesinger.warcry.resonance_gain`)
- Dirge: +3 per stack applied
- Maximum: 100

**Resonance Decay** (out of combat):
- -3 per tick when not fighting
- Drops to 0 naturally

**Peak Tracking**: Highest resonance reached stored in `ch->pcdata->stats[DIRGE_RESONANCE_PEAK]` (`dirgesinger.c:471-472`)

### Resonance Display

Command: `resonance` (`dirgesinger.c:20-54`)
- Shows current/max resonance
- Lists all active buffs with remaining durations
- Available to both Dirgesinger and Siren

## Training Categories

Dirgesinger abilities are organized into 4 trainable categories. Each category has 3 levels, unlocking abilities progressively. Stored in `ch->pcdata->powers[]`:

| Index | Category | Theme | Abilities |
|-------|----------|-------|-----------|
| DIRGE_TRAIN_WARCHANTS (10) | War Chants | Offensive sonic attacks | Warcry, Shatter, Thunderclap |
| DIRGE_TRAIN_BATTLESONGS (11) | Battle Songs | Self/group combat buffs | Battlehymn, Cadence, Warsong |
| DIRGE_TRAIN_DIRGES (12) | Dirges | Debuffs and damage-over-time | Dirge, Dissonance |
| DIRGE_TRAIN_IRONVOICE (13) | Iron Voice | Defense and group support | Ironsong, Rally |

**Training Cost**: `(current_level + 1) * 40` primal per level

**Training Command**: `songtrain` - Display current levels and improve categories

### War Chants (Offensive)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `warcry` | Basic sonic attack, builds resonance |
| 2 | `shatter` | Heavy single-target burst + disarm |
| 3 | `thunderclap` | AoE sonic shockwave |

### Battle Songs (Buffs)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `battlehymn` | Self-buff: sonic damage on melee attacks |
| 2 | `cadence` | Haste: extra attacks per round |
| 3 | `warsong` | Toggle aura: group combat bonuses (sustained mana) |

### Dirges (Debuffs)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `dirge` | Stacking DOT that weakens target |
| 2 | `dissonance` | Debuff reducing enemy attack effectiveness |

### Iron Voice (Defense/Support)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `ironsong` | Sonic barrier absorbing damage |
| 2 | `rally` | Group heal through inspiring war-song |

## Abilities

### Warcry - Basic Sonic Attack (`dirgesinger.c:59-92`)

Basic combat ability that deals sonic damage and builds resonance.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `dirgesinger.warcry.mana_cost` |
| Cooldown | 4 pulses | `dirgesinger.warcry.cooldown` |
| Base Damage | 50-120 + (resonance × 2) | `dirgesinger.warcry.dam_bonus_min/max` |
| Resonance Gain | +5 | `dirgesinger.warcry.resonance_gain` |
| Requires | Fighting, War Chants 1 | |

### Shatter - Heavy Sonic Burst (`dirgesinger.c:98-144`)

Powerful single-target attack with disarm chance. Never destroys equipment.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `dirgesinger.shatter.mana_cost` |
| Resonance Required | 25 | `dirgesinger.shatter.resonance_req` |
| Resonance Cost | 10 | `dirgesinger.shatter.resonance_cost` |
| Cooldown | 8 pulses | `dirgesinger.shatter.cooldown` |
| Base Damage | 200-400 + (resonance × 3) | |
| Disarm Chance | 30% | `dirgesinger.shatter.disarm_chance` |
| Requires | Fighting, War Chants 2 | |

### Thunderclap - AoE Sonic Shockwave (`dirgesinger.c:215-257`)

Area attack hitting all enemies currently fighting the Dirgesinger.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `dirgesinger.thunderclap.mana_cost` |
| Resonance Required | 40 | `dirgesinger.thunderclap.resonance_req` |
| Resonance Cost | 20 | `dirgesinger.thunderclap.resonance_cost` |
| Cooldown | 10 pulses | `dirgesinger.thunderclap.cooldown` |
| Damage per Target | 150-300 + (resonance × 2) | |
| Requires | Fighting, War Chants 3 | |

### Battlehymn - Melee Damage Buff (`dirgesinger.c:149-168`)

Self-buff adding bonus sonic damage to melee attacks. Increases damage cap.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `dirgesinger.battlehymn.mana_cost` |
| Duration | 10 ticks | `dirgesinger.battlehymn.duration` |
| Damage Bonus | +50 per hit | `dirgesinger.battlehymn.dam_bonus` |
| Damcap Bonus | +200 | `balance.damcap_dirgesinger_battlehymn` |
| Requires | Battle Songs 1 | |

### Cadence - Haste Buff (`dirgesinger.c:287-306`)

Grants extra attacks per combat round.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `dirgesinger.cadence.mana_cost` |
| Duration | 8 ticks | `dirgesinger.cadence.duration` |
| Extra Attacks | +2 | `dirgesinger.cadence.extra_attacks` |
| Requires | Battle Songs 2 | |

### Warsong - Sustained Group Aura (`dirgesinger.c:377-396`)

Toggle aura providing combat bonuses to self and group. Drains mana each tick.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Drain | 25/tick | `dirgesinger.warsong.mana_drain_per_tick` |
| Effect | Group combat bonuses | |
| Toggle | On/Off | |
| Auto-off | When mana depleted | |
| Requires | Battle Songs 3 | |

### Dirge - Stacking DOT (`dirgesinger.c:173-210`)

Damage-over-time that stacks progressively, increasing damage each tick.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `dirgesinger.dirge.mana_cost` |
| Max Stacks | 5 | `dirgesinger.dirge.max_stacks` |
| Duration | 6 ticks | `dirgesinger.dirge.duration` |
| Tick Damage | 50 × stacks | `dirgesinger.dirge.tick_damage` |
| Resonance Gain | +3 per stack | |
| Requires | Fighting, Dirges 1 | |

### Dissonance - Enemy Debuff (`dirgesinger.c:311-334`)

Reduces enemy attack effectiveness for a duration.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `dirgesinger.dissonance.mana_cost` |
| Cooldown | 8 pulses | `dirgesinger.dissonance.cooldown` |
| Duration | 10 ticks | `dirgesinger.dissonance.duration` |
| Requires | Fighting, Dirges 2 | |

### Ironsong - Sonic Barrier (`dirgesinger.c:262-282`)

Creates an absorptive barrier that blocks incoming damage.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `dirgesinger.ironsong.mana_cost` |
| Duration | 12 ticks | `dirgesinger.ironsong.duration` |
| Absorb Amount | 1000 HP | `dirgesinger.ironsong.absorb_amount` |
| Requires | Iron Voice 1 | |

**Absorption Mechanic** (`fight.c:1886-1898`):
- Damage <= barrier HP: fully absorbed, barrier reduced
- Damage > barrier HP: barrier breaks, remaining damage applied

### Rally - Group Heal (`dirgesinger.c:339-372`)

Heals all group members in the room.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `dirgesinger.rally.mana_cost` |
| Resonance Required | 50 | `dirgesinger.rally.resonance_req` |
| Resonance Cost | 30 | `dirgesinger.rally.resonance_cost` |
| Heal Amount | 500 HP | `dirgesinger.rally.heal_amount` |
| Requires | Iron Voice 2 | |

## Dirgesinger Armor

Create equipment via `dirgesingerarmor <piece>` (`dirgesinger.c:401-455`):

**Cost**: 60 primal per piece (`acfg: dirgesinger.armor.primal_cost`)

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| warhorn | 33320 | Wield |
| ring | 33321 | Finger |
| collar | 33322 | Neck |
| battleplate | 33323 | Torso |
| warhelm | 33324 | Head |
| greaves | 33325 | Legs |
| warboots | 33326 | Feet |
| gauntlets | 33327 | Hands |
| vambraces | 33328 | Arms |
| warcape | 33329 | About Body |
| belt | 33330 | Waist |
| bracer | 33331 | Wrist |
| warmask | 33332 | Face |

**Equipment Stats** (from `classeq.db`):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions** (`handler.c`): Vnum range 33320-33339 restricted to CLASS_DIRGESINGER only.

## Combat Mechanics

### Damage Cap Bonuses (`fight.c:1704-1708`)

```c
max_dam += balance.damcap_dirgesinger_base;                    // +200 flat
max_dam += ch->rage * balance.damcap_dirgesinger_res_mult;    // +7 per resonance
if ( battlehymn active )
    max_dam += balance.damcap_dirgesinger_battlehymn;          // +200
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Base | +200 flat | `damcap_dirgesinger_base` (default: 200) |
| Resonance | +7 per point (max +700) | `damcap_dirgesinger_res_mult` (default: 7) |
| Battlehymn | +200 flat | `damcap_dirgesinger_battlehymn` (default: 200) |

### Extra Attacks (`fight.c:1004-1006`)

Cadence buff grants +2 extra attacks per combat round when active.

### Ironsong Barrier (`fight.c:1886-1898`)

Absorbs incoming damage before it reaches HP. Shared with Siren class.

### Tick Update (`dirgesinger.c:460-518`)

Called from `update.c:508`. Per-tick processing:
1. Resonance build (+2 fighting) or decay (-3 not fighting)
2. Dirge DOT damage (50 × stacks per tick)
3. Battlehymn duration countdown
4. Ironsong barrier duration countdown
5. Cadence duration countdown
6. Dissonance duration countdown
7. Warsong mana drain (10/tick, auto-off if depleted)

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_DIRGESINGER bit (16384) |
| resonance | ch->rage | Current resonance (0-100) |
| warsong | ch->pcdata->powers[0] | Warsong toggle (0/1) |
| battlehymn | ch->pcdata->powers[1] | Battlehymn ticks remaining |
| ironsong | ch->pcdata->powers[2] | Ironsong ticks remaining |
| cadence | ch->pcdata->powers[3] | Cadence ticks remaining |
| dirge_ticks | ch->pcdata->powers[4] | Dirge DOT ticks remaining |
| dirge_stacks | ch->pcdata->powers[5] | Dirge DOT stacks (1-5) |
| dissonance | ch->pcdata->powers[8] | Dissonance ticks remaining |
| warchants | ch->pcdata->powers[10] | War Chants training level (0-3) |
| battlesongs | ch->pcdata->powers[11] | Battle Songs training level (0-3) |
| dirges | ch->pcdata->powers[12] | Dirges training level (0-2) |
| ironvoice | ch->pcdata->powers[13] | Iron Voice training level (0-2) |
| barrier_hp | ch->pcdata->stats[0] | Current sonic barrier HP |
| peak_res | ch->pcdata->stats[1] | Highest resonance this session |
