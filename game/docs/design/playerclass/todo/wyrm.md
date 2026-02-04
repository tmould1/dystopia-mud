# Wyrm Class Design

## Overview

Wyrms are true dragons who have fully awakened their draconic heritage. They can assume a massive Wyrm Form for devastating combat, wield prismatic breath that combines all elements, and unleash ancient powers that shake the earth itself. While they retain the elemental forms of their Dragonkin origins, they transcend such limitations with multi-elemental mastery. This is an **upgrade class** obtained by upgrading from Dragonkin.

**Source Files**: `src/classes/wyrm.c`, `src/classes/dragonkin.h` (TBD)
**Class Constant**: `CLASS_WYRM` (TBD - next available power-of-2)
**Upgrades From**: Dragonkin

## Color Scheme

Wyrm uses a majestic platinum-azure palette evoking ancient draconic majesty:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x027` | Deep azure | Bracket tildes, decorative accents |
| Primary | `#x255` | Platinum white | Class name, titles, ability highlights |
| Bracket open | `#x027~#x255(` | Azure~Platinum( | Who list open bracket |
| Bracket close | `#x255)#x027~` | Platinum)Azure~ | Who list close bracket |
| Room tag | `#x255(#nWyrm#x255)` | Platinum parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Ancient Wyrm | `#x027~#x255(#x255Ancient Wyrm#n#x255)#x027~` |
| 2 | Elder Dragon | `#x027~#x255(#x255Elder Dragon#n#x255)#x027~` |
| 3 | Great Wyrm | `#x027~#x255(#x255Great Wyrm#n#x255)#x027~` |
| 4 | Dragon | `#x027~#x255(#x255Dragon#n#x255)#x027~` |
| default | Wyrm | `#x027~#x255(#x255Wyrm#n#x255)#x027~` |

## Core Mechanics

### Draconic Fury (Enhanced)

Wyrm inherits the Draconic Fury system from Dragonkin with a higher cap and improved generation:

```c
ch->rage  // Current fury (0-150 for Wyrm)
```

**Building Fury**:
- Combat: +4 per game tick while fighting (vs +3 for Dragonkin)
- Taking damage: +3 per hit received (vs +2 for Dragonkin)
- Dealing damage: +2 per successful hit (vs +1 for Dragonkin)
- Breath attacks: +8 on use (vs +5 for Dragonkin)
- Maximum: 150 (vs 100 for Dragonkin)

**Fury Decay** (out of combat):
- -3 per tick when not fighting (slower than Dragonkin's -4)

### Elemental Forms (Enhanced)

Wyrm retains access to all four elemental forms with enhanced resistances:

| Form | Element | Resistance | Vulnerability | Enhancement |
|------|---------|------------|---------------|-------------|
| Flame Form | Fire | Fire +65% | Cold -15% | Burn DoT +50% |
| Frost Form | Cold | Cold +65% | Fire -15% | Slow duration +50% |
| Storm Form | Lightning | Lightning +65% | Acid -15% | Stun chance +10% |
| Venom Form | Acid | Acid +65% | Lightning -15% | Armor shred +50% |

### Wyrm Form

The signature ability - transform into a massive true dragon:

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 150 | `wyrm.wyrmform.mana_cost` |
| Fury Required | 75 | `wyrm.wyrmform.fury_req` |
| Duration | 10 ticks | `wyrm.wyrmform.duration` |
| Cooldown | 30 pulses | `wyrm.wyrmform.cooldown` |

**Wyrm Form Effects**:
- Size increases (description changes)
- +4 extra attacks per round
- +50% damage to all abilities
- +25% resistance to all elements
- Access to Wing Buffet and Dragon Flight
- Cannot use elemental form-specific abilities (universal dragon)
- Fury decay paused

### Shared Dragonkin Abilities

Wyrm retains access to the `fury` and `shift` commands. Elemental abilities are enhanced versions. Universal abilities (Scale Armor, Tail Sweep, Dragon Fear) remain available.

### Upgrade Path

Dragonkin upgrades to Wyrm via `do_upgrade`:
- Requires standard upgrade costs (50K hp, 35K mana/move, 40K qp, gen 1)
- Class changes from `CLASS_DRAGONKIN` to `CLASS_WYRM`
- All abilities reset; Wyrm abilities become available
- All elemental form training carries over conceptually

## Training Categories

Wyrm abilities are organized into 3 trainable categories. Stored in `ch->pcdata->powers[]`:

| Index | Category | Max Level | Theme | Abilities |
|-------|----------|-----------|-------|-----------|
| WYRM_TRAIN_DRAGON (10) | True Dragon | 3 | Full transformation | Wyrm Form, Wing Buffet, Dragon Flight |
| WYRM_TRAIN_PRISMATIC (11) | Prismatic Power | 3 | Multi-element | Prismatic Breath, Elemental Fusion, Chromatic Scales |
| WYRM_TRAIN_ANCIENT (12) | Ancient Wrath | 3 | Legendary abilities | Dragon Roar, Cataclysm, Ascension |

**Training Cost**: `(current_level + 1) * 50` primal per level

**Training Command**: `wyrmtrain` - Display current levels and improve categories

### True Dragon (Transformation)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `wyrmform` | Transform into true dragon form |
| 2 | `wingbuffet` | Wing attack (Wyrm Form only) |
| 3 | `dragonflight` | Sustained flight (Wyrm Form only) |

### Prismatic Power (Multi-Element)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `prismaticbreath` | Breath all elements simultaneously |
| 2 | `elementalfusion` | Combine two element attacks |
| 3 | `chromaticscales` | Resist all elements |

### Ancient Wrath (Legendary)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `dragonroar` | Terrifying roar, mass debuff |
| 2 | `cataclysm` | Massive AoE devastation |
| 3 | `ascension` | Ultimate form - draconic god |

## Abilities

### Wyrm Form - True Transformation

Transform into a massive true dragon.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 150 | `wyrm.wyrmform.mana_cost` |
| Fury Required | 75 | `wyrm.wyrmform.fury_req` |
| Duration | 10 ticks | `wyrm.wyrmform.duration` |
| Cooldown | 30 pulses | `wyrm.wyrmform.cooldown` |
| Requires | True Dragon 1 | |

**Effects**:
- +4 extra attacks per round
- +50% damage to all abilities
- +25% resistance to all damage types
- Unlocks Wing Buffet and Dragon Flight
- Cannot use elemental form abilities
- Exits current elemental form

### Wing Buffet - Wing Attack (Wyrm Form)

Powerful wing strike that hits all enemies and knocks them down.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `wyrm.wingbuffet.mana_cost` |
| Fury Cost | 25 | `wyrm.wingbuffet.fury_cost` |
| Cooldown | 8 pulses | `wyrm.wingbuffet.cooldown` |
| Damage per Target | 200-350 + (fury × 3) | |
| Knockdown | 100% (all targets lose 2 attacks) | |
| Requires | Wyrm Form, Fighting, True Dragon 2 | |

### Dragon Flight - Sustained Flight (Wyrm Form)

Take to the skies, gaining mobility and evasion.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `wyrm.dragonflight.mana_cost` |
| Duration | Until landing or Wyrm Form ends | |
| Fury Drain | 5 per tick | `wyrm.dragonflight.fury_drain` |
| Requires | Wyrm Form, True Dragon 3 | |

**Effects**:
- Immune to ground-based attacks
- +30% dodge chance
- Can use `dive` for bonus damage attack (+50% next attack)
- Cannot use Wing Buffet while flying
- `land` command or automatic when fury depleted

### Prismatic Breath - All-Element Breath

Breathes a devastating blast of all four elements simultaneously.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `wyrm.prismaticbreath.mana_cost` |
| Fury Required | 60 | `wyrm.prismaticbreath.fury_req` |
| Fury Cost | 40 | `wyrm.prismaticbreath.fury_cost` |
| Cooldown | 12 pulses | `wyrm.prismaticbreath.cooldown` |
| Damage per Target | 250-400 + (fury × 4) | |
| Requires | Fighting, Prismatic Power 1 | |

**Effects**:
- Deals all four damage types (fire/cold/lightning/acid)
- Target takes full damage from whichever type they're LEAST resistant to
- Applies: Burn (2 ticks), Slow (2 ticks), Stun (25% chance), Armor Reduction
- Can be used in any form

### Elemental Fusion - Combined Elements

Fuses two elements for a devastating hybrid attack.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `wyrm.elementalfusion.mana_cost` |
| Fury Cost | 30 | `wyrm.elementalfusion.fury_cost` |
| Cooldown | 10 pulses | `wyrm.elementalfusion.cooldown` |
| Requires | Fighting, Prismatic Power 2 | |

**Usage**: `fusion <element1> <element2>`

| Fusion | Effect | Bonus |
|--------|--------|-------|
| Fire + Cold | Steam Blast | Blinds target (50% miss, 3 ticks) |
| Fire + Lightning | Plasma Burst | Armor pierce 75% |
| Fire + Acid | Napalm | DoT 100/tick for 6 ticks |
| Cold + Lightning | Cryo-shock | 3 round stun |
| Cold + Acid | Permafrost | Healing reduced 75%, 5 ticks |
| Lightning + Acid | Corrosive Storm | AoE + armor shred all targets |

Base damage: 200-300 + (fury × 3)

### Chromatic Scales - Multi-Element Resistance

Scales shift through all colors, granting universal protection.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `wyrm.chromaticscales.mana_cost` |
| Fury Cost | 35 | `wyrm.chromaticscales.fury_cost` |
| Duration | 8 ticks | `wyrm.chromaticscales.duration` |
| Requires | Prismatic Power 3 | |

**Effects**:
- +40% resistance to ALL elemental damage types
- Removes elemental vulnerabilities
- Attackers take 20 damage of random element on hit
- Stacks with form resistances

### Dragon Roar - Terrifying Presence

A roar that shakes the very foundations of reality.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `wyrm.dragonroar.mana_cost` |
| Fury Required | 50 | `wyrm.dragonroar.fury_req` |
| Fury Cost | 30 | `wyrm.dragonroar.fury_cost` |
| Cooldown | 15 pulses | `wyrm.dragonroar.cooldown` |
| Requires | Fighting, Ancient Wrath 1 | |

**Effects**:
- All enemies: -25% damage for 6 ticks
- All enemies: -20 hitroll for 6 ticks
- NPCs: 50% flee chance
- Allies: +10% damage for 6 ticks (inspiring)

### Cataclysm - Mass Destruction

Unleashes the fury of an ancient dragon upon all foes.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 180 | `wyrm.cataclysm.mana_cost` |
| Fury Required | 100 | `wyrm.cataclysm.fury_req` |
| Fury Cost | 75 | `wyrm.cataclysm.fury_cost` |
| Cooldown | 30 pulses | `wyrm.cataclysm.cooldown` |
| Damage per Target | 400-650 + (fury × 5) | |
| Requires | Fighting, Ancient Wrath 2 | |

**Effects**:
- Massive AoE hitting all enemies
- Room description temporarily changes (devastation)
- Destroys all enemy summons/pets instantly
- Removes all enemy shields/barriers
- Earthquake effect: Ground-based enemies take +50% damage

### Ascension - Ultimate Form

Briefly transcends mortality to become a draconic god.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 250 | `wyrm.ascension.mana_cost` |
| Fury Required | 125 | `wyrm.ascension.fury_req` |
| Fury Cost | All (depletes to 0) | |
| Duration | 5 ticks | `wyrm.ascension.duration` |
| Cooldown | 60 pulses | `wyrm.ascension.cooldown` |
| Requires | Ancient Wrath 3 | |

**Ascended Form Effects**:
- +8 extra attacks per round
- +100% damage to all abilities
- 50% damage resistance (all types)
- Immune to all crowd control
- All abilities cost no mana (fury only, which is frozen at 0)
- Regenerate 5% max HP per tick
- Cannot be healed by external sources
- When duration ends: Stunned for 2 rounds, fury locked at 0 for 10 ticks

## Wyrm Armor

Create equipment via `wyrmarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| dragon's fang blade | TBD | Wield |
| prismatic ring | TBD | Finger |
| dragon heart amulet | TBD | Neck |
| wyrm scale armor | TBD | Torso |
| crown of the wyrm | TBD | Head |
| dragon scale greaves | TBD | Legs |
| clawed wyrm boots | TBD | Feet |
| talon gauntlets | TBD | Hands |
| wing-membrane bracers | TBD | Arms |
| ancient wyrm cloak | TBD | About Body |
| dragon scale belt | TBD | Waist |
| claw bracer | TBD | Wrist |
| wyrm visage | TBD | Face |

**Equipment Stats** (proposed):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_WYRM only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += ch->rage * balance.damcap_wyrm_fury_mult;      // +8 per fury
if ( wyrm_form active )
    max_dam += balance.damcap_wyrm_form;                   // +400
if ( chromatic_scales active )
    max_dam += balance.damcap_wyrm_chromatic;              // +150
if ( ascension active )
    max_dam += balance.damcap_wyrm_ascension;              // +600
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Fury | +8 per point (max +1200) | `damcap_wyrm_fury_mult` (default: 8) |
| Wyrm Form | +400 flat | `damcap_wyrm_form` (default: 400) |
| Chromatic Scales | +150 flat | `damcap_wyrm_chromatic` (default: 150) |
| Ascension | +600 flat | `damcap_wyrm_ascension` (default: 600) |

### Enhanced Elemental Processing

Wyrm form abilities ignore elemental vulnerabilities (only apply resistances).

### Tick Update

Per-tick processing:
1. Fury build (+4 fighting, cap 150) or decay (-3 not fighting)
2. Peak fury tracking
3. Wyrm Form duration countdown
4. Dragon Flight fury drain
5. Chromatic Scales duration countdown
6. Ascension duration and aftermath processing
7. Enhanced elemental form buff durations

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_WYRM bit (TBD) |
| fury | ch->rage | Current draconic fury (0-150) |
| current_form | ch->pcdata->powers[0] | Current form (0=none, 1-4=elemental, 5=wyrm) |
| wyrm_form | ch->pcdata->powers[1] | Wyrm Form ticks remaining |
| dragon_flight | ch->pcdata->powers[2] | Dragon Flight active (0/1) |
| chromatic_scales | ch->pcdata->powers[3] | Chromatic Scales ticks remaining |
| ascension | ch->pcdata->powers[4] | Ascension ticks remaining |
| ascension_aftermath | ch->pcdata->powers[5] | Ascension aftermath ticks remaining |
| true_dragon | ch->pcdata->powers[10] | True Dragon training level (0-3) |
| prismatic_power | ch->pcdata->powers[11] | Prismatic Power training level (0-3) |
| ancient_wrath | ch->pcdata->powers[12] | Ancient Wrath training level (0-3) |
| peak_fury | ch->pcdata->stats[0] | Highest fury this session |
| ascensions_used | ch->pcdata->stats[1] | Total Ascensions used (tracking) |
