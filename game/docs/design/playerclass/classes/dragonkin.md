# Dragonkin Class Design

## Overview

Dragonkin are mortals with draconic blood who can manifest the power of dragons. They shift between four elemental forms (Flame, Frost, Storm, Venom), each granting different abilities, resistances, and vulnerabilities. Mastering all forms and knowing when to shift is key to reaching their full potential. This is a **base class** that upgrades to Wyrm.

**Source Files**: `src/classes/dragonkin.c`, `src/classes/dragonkin.h` (TBD)
**Class Constant**: `CLASS_DRAGONKIN` (TBD - next available power-of-2)
**Upgrades To**: Wyrm

## Color Scheme

Dragonkin uses a regal gold-crimson palette evoking draconic nobility:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x160` | Deep crimson | Bracket tildes, decorative accents |
| Primary | `#x220` | Bright gold | Class name, titles, ability highlights |
| Bracket open | `#x160~#x220[` | Crimson~Gold[ | Who list open bracket |
| Bracket close | `#x220]#x160~` | Gold]Crimson~ | Who list close bracket |
| Room tag | `#x220(#nDragonkin#x220)` | Gold parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Dragon Heir | `#x160~#x220[#x220Dragon Heir#n#x220]#x160~` |
| 2 | Draconic | `#x160~#x220[#x220Draconic#n#x220]#x160~` |
| 3 | Dragonkin | `#x160~#x220[#x220Dragonkin#n#x220]#x160~` |
| 4 | Scaled One | `#x160~#x220[#x220Scaled One#n#x220]#x160~` |
| 5 | Wyrmling | `#x160~#x220[#x220Wyrmling#n#x220]#x160~` |
| default | Hatchling | `#x160~#x220[#x220Hatchling#n#x220]#x160~` |

## Core Mechanics

### Draconic Fury

Draconic Fury is the combat resource that powers Dragonkin abilities, stored in `ch->rage`:

```c
ch->rage  // Current fury (0-100 for Dragonkin, 0-150 for Wyrm)
```

**Building Fury**:
- Combat: +3 per game tick while fighting (draconic bloodlust)
- Taking damage: +2 per hit received (dragon's wrath)
- Dealing damage: +1 per successful hit
- Breath attacks: +5 on use
- Maximum: 100

**Fury Decay** (out of combat):
- -4 per tick when not fighting (rapid cooldown)
- Dragon blood runs hot in battle, cools quickly after

**Peak Tracking**: Highest fury reached stored in `ch->pcdata->stats[DRAGON_FURY_PEAK]`

### Fury Display

Command: `fury`
- Shows current/max fury
- Shows current form and active buffs
- Lists form-specific abilities available
- Available to both Dragonkin and Wyrm

### Elemental Forms

The core mechanic of Dragonkin. Four elemental forms, each with:
- Unique abilities (form-locked)
- Elemental resistance (+50%)
- Elemental vulnerability (-25%)
- Visual changes to description and combat messages

| Form | Element | Resistance | Vulnerability | Theme |
|------|---------|------------|---------------|-------|
| Flame Form | Fire | Fire +50% | Cold -25% | Aggressive, DoT focus |
| Frost Form | Cold | Cold +50% | Fire -25% | Control, slow effects |
| Storm Form | Lightning | Lightning +50% | Acid -25% | Burst damage, speed |
| Venom Form | Acid | Acid +50% | Lightning -25% | Debuffs, armor shred |

### Form Shifting

Command: `shift <form>` (flame/frost/storm/venom)

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 30 | `dragonkin.shift.mana_cost` |
| Cooldown | 4 pulses | `dragonkin.shift.cooldown` |
| Combat Shift | Yes, with cooldown | |

**Tactical Element**: Shifting mid-combat to exploit enemy weaknesses or cover your own is essential to mastery.

### Universal Abilities

Some abilities work in any form (Draconic Body training):
- Scale Armor, Tail Sweep, Dragon Fear

## Training Categories

Dragonkin abilities are organized into 5 trainable categories - 4 elemental masteries and 1 universal. Stored in `ch->pcdata->powers[]`:

| Index | Category | Theme | Requires Form | Abilities |
|-------|----------|-------|---------------|-----------|
| DRAGON_TRAIN_FLAME (10) | Flame Mastery | Fire attacks | Flame Form | Inferno Breath, Burning Claws, Immolate |
| DRAGON_TRAIN_FROST (11) | Frost Mastery | Cold attacks | Frost Form | Blizzard Breath, Frozen Scales, Glaciate |
| DRAGON_TRAIN_STORM (12) | Storm Mastery | Lightning attacks | Storm Form | Thunder Breath, Static Charge, Chain Lightning |
| DRAGON_TRAIN_VENOM (13) | Venom Mastery | Acid attacks | Venom Form | Corrosive Breath, Toxic Fangs, Dissolve |
| DRAGON_TRAIN_BODY (14) | Draconic Body | Universal | Any Form | Scale Armor, Tail Sweep, Dragon Fear |

**Training Cost**: `(current_level + 1) * 40` primal per level

**Training Command**: `dragontrain` - Display current levels and improve categories

### Flame Mastery (Fire Form)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `infernobreath` | Cone fire damage + burn DoT |
| 2 | `burningclaws` | Melee attacks gain fire damage |
| 3 | `immolate` | Engulf target in flames, heavy DoT |

### Frost Mastery (Frost Form)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `blizzardbreath` | Cone cold damage + slow |
| 2 | `frozenscales` | Defensive buff, attackers take cold damage |
| 3 | `glaciate` | Encase target in ice, stun + damage |

### Storm Mastery (Storm Form)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `thunderbreath` | Cone lightning damage + chance to stun |
| 2 | `staticcharge` | Build charge, next attack deals bonus damage |
| 3 | `chainlightning` | Bouncing lightning hits multiple targets |

### Venom Mastery (Venom Form)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `corrosivebreath` | Cone acid damage + armor reduction |
| 2 | `toxicfangs` | Melee attacks apply poison DoT |
| 3 | `dissolve` | Destroy target's armor temporarily |

### Draconic Body (Universal)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `scalearmor` | Damage absorption shield |
| 2 | `tailsweep` | AoE knockback attack |
| 3 | `dragonfear` | Terrify enemies, chance to flee |

## Abilities

### Shift - Form Change

Changes between elemental forms.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 30 | `dragonkin.shift.mana_cost` |
| Cooldown | 4 pulses | `dragonkin.shift.cooldown` |

**Forms**: flame, frost, storm, venom

### Inferno Breath - Fire Cone (Flame Form)

Breathes a cone of fire, burning all enemies.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `dragonkin.infernobreath.mana_cost` |
| Fury Gain | +5 | `dragonkin.infernobreath.fury_gain` |
| Cooldown | 6 pulses | `dragonkin.infernobreath.cooldown` |
| Base Damage | 120-220 + (fury × 2) | |
| Burn DoT | 30 damage/tick for 4 ticks | `dragonkin.infernobreath.burn_damage` |
| Requires | Flame Form, Fighting, Flame Mastery 1 | |

### Burning Claws - Melee Enhancement (Flame Form)

Wreath claws in fire, adding fire damage to melee attacks.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `dragonkin.burningclaws.mana_cost` |
| Duration | 10 ticks | `dragonkin.burningclaws.duration` |
| Fire Damage | +40 per hit | `dragonkin.burningclaws.bonus_damage` |
| Requires | Flame Form, Flame Mastery 2 | |

### Immolate - Heavy Fire DoT (Flame Form)

Engulfs the target in dragonfire, dealing massive damage over time.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `dragonkin.immolate.mana_cost` |
| Fury Required | 40 | `dragonkin.immolate.fury_req` |
| Fury Cost | 20 | `dragonkin.immolate.fury_cost` |
| Cooldown | 12 pulses | `dragonkin.immolate.cooldown` |
| Duration | 6 ticks | `dragonkin.immolate.duration` |
| Tick Damage | 80-120 + (fury × 1) | |
| Requires | Flame Form, Fighting, Flame Mastery 3 | |

### Blizzard Breath - Cold Cone (Frost Form)

Breathes freezing wind, damaging and slowing enemies.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `dragonkin.blizzardbreath.mana_cost` |
| Fury Gain | +5 | `dragonkin.blizzardbreath.fury_gain` |
| Cooldown | 6 pulses | `dragonkin.blizzardbreath.cooldown` |
| Base Damage | 100-180 + (fury × 2) | |
| Slow Effect | -20% attack speed for 4 ticks | |
| Requires | Frost Form, Fighting, Frost Mastery 1 | |

### Frozen Scales - Defensive Aura (Frost Form)

Coats scales in ice, damaging attackers.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `dragonkin.frozenscales.mana_cost` |
| Duration | 8 ticks | `dragonkin.frozenscales.duration` |
| Reflect Damage | 25 cold damage to melee attackers | |
| Cold Resistance | Additional +15% | |
| Requires | Frost Form, Frost Mastery 2 | |

### Glaciate - Ice Prison (Frost Form)

Encases target in ice, stunning and damaging them.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `dragonkin.glaciate.mana_cost` |
| Fury Required | 40 | `dragonkin.glaciate.fury_req` |
| Fury Cost | 20 | `dragonkin.glaciate.fury_cost` |
| Cooldown | 14 pulses | `dragonkin.glaciate.cooldown` |
| Damage | 150-250 + (fury × 2) | |
| Stun Duration | 2 rounds | `dragonkin.glaciate.stun_duration` |
| Requires | Frost Form, Fighting, Frost Mastery 3 | |

### Thunder Breath - Lightning Cone (Storm Form)

Breathes crackling lightning, with chance to stun.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `dragonkin.thunderbreath.mana_cost` |
| Fury Gain | +5 | `dragonkin.thunderbreath.fury_gain` |
| Cooldown | 6 pulses | `dragonkin.thunderbreath.cooldown` |
| Base Damage | 130-230 + (fury × 2) | |
| Stun Chance | 20% (1 round) | `dragonkin.thunderbreath.stun_chance` |
| Requires | Storm Form, Fighting, Storm Mastery 1 | |

### Static Charge - Damage Buff (Storm Form)

Builds electrical charge for a devastating next attack.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `dragonkin.staticcharge.mana_cost` |
| Fury Cost | 10 | `dragonkin.staticcharge.fury_cost` |
| Duration | 3 attacks or 6 ticks | |
| Bonus Damage | +100 lightning per attack | `dragonkin.staticcharge.bonus_damage` |
| Requires | Storm Form, Storm Mastery 2 | |

### Chain Lightning - Bouncing Attack (Storm Form)

Lightning that bounces between multiple enemies.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 90 | `dragonkin.chainlightning.mana_cost` |
| Fury Required | 50 | `dragonkin.chainlightning.fury_req` |
| Fury Cost | 30 | `dragonkin.chainlightning.fury_cost` |
| Cooldown | 10 pulses | `dragonkin.chainlightning.cooldown` |
| Primary Damage | 200-300 + (fury × 3) | |
| Bounce Damage | 70% of previous hit | |
| Max Bounces | 3 | `dragonkin.chainlightning.max_bounces` |
| Requires | Storm Form, Fighting, Storm Mastery 3 | |

### Corrosive Breath - Acid Cone (Venom Form)

Breathes caustic acid, reducing enemy armor.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `dragonkin.corrosivebreath.mana_cost` |
| Fury Gain | +5 | `dragonkin.corrosivebreath.fury_gain` |
| Cooldown | 6 pulses | `dragonkin.corrosivebreath.cooldown` |
| Base Damage | 100-180 + (fury × 2) | |
| Armor Reduction | +20 AC (worse armor) for 5 ticks | |
| Requires | Venom Form, Fighting, Venom Mastery 1 | |

### Toxic Fangs - Poison Melee (Venom Form)

Coats fangs and claws with venom, applying poison on hit.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `dragonkin.toxicfangs.mana_cost` |
| Duration | 10 ticks | `dragonkin.toxicfangs.duration` |
| Poison Damage | 20/tick for 5 ticks (stacks to 3) | |
| Requires | Venom Form, Venom Mastery 2 | |

### Dissolve - Armor Destruction (Venom Form)

Highly concentrated acid that destroys armor protection.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `dragonkin.dissolve.mana_cost` |
| Fury Required | 40 | `dragonkin.dissolve.fury_req` |
| Fury Cost | 20 | `dragonkin.dissolve.fury_cost` |
| Cooldown | 15 pulses | `dragonkin.dissolve.cooldown` |
| Duration | 8 ticks | `dragonkin.dissolve.duration` |
| Effect | Target's AC set to 100 (terrible) | |
| Requires | Venom Form, Fighting, Venom Mastery 3 | |

### Scale Armor - Damage Shield (Universal)

Hardens scales into protective armor.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `dragonkin.scalearmor.mana_cost` |
| Duration | 10 ticks | `dragonkin.scalearmor.duration` |
| Absorb Amount | 600 HP | `dragonkin.scalearmor.absorb_amount` |
| Requires | Draconic Body 1 | |

### Tail Sweep - AoE Knockback (Universal)

Sweeps tail in an arc, hitting all enemies.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `dragonkin.tailsweep.mana_cost` |
| Fury Cost | 15 | `dragonkin.tailsweep.fury_cost` |
| Cooldown | 8 pulses | `dragonkin.tailsweep.cooldown` |
| Damage per Target | 80-140 + (fury × 1) | |
| Knockdown Chance | 30% (lose 1 attack) | |
| Requires | Fighting, Draconic Body 2 | |

### Dragon Fear - Terror Aura (Universal)

Unleashes the terrifying presence of a dragon.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `dragonkin.dragonfear.mana_cost` |
| Fury Required | 50 | `dragonkin.dragonfear.fury_req` |
| Fury Cost | 25 | `dragonkin.dragonfear.fury_cost` |
| Cooldown | 15 pulses | `dragonkin.dragonfear.cooldown` |
| Flee Chance | 40% per NPC | `dragonkin.dragonfear.flee_chance` |
| Debuff | -15% damage for 5 ticks (if doesn't flee) | |
| Requires | Fighting, Draconic Body 3 | |

## Dragonkin Armor

Create equipment via `dragonkinarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| dragon fang | TBD | Wield |
| scale ring | TBD | Finger |
| dragon tooth necklace | TBD | Neck |
| dragon scale vest | TBD | Torso |
| horned helm | TBD | Head |
| scale leggings | TBD | Legs |
| clawed boots | TBD | Feet |
| talon gauntlets | TBD | Hands |
| wing-bone bracers | TBD | Arms |
| dragon hide cloak | TBD | About Body |
| scale belt | TBD | Waist |
| claw bracer | TBD | Wrist |
| dragon visage | TBD | Face |

**Equipment Stats** (proposed):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_DRAGONKIN and CLASS_WYRM only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += balance.damcap_dragonkin_base;                    // +150 flat
max_dam += ch->rage * balance.damcap_dragonkin_fury_mult;    // +6 per fury
// Form-specific bonuses
if ( flame_form && burning_claws active )
    max_dam += balance.damcap_dragonkin_flame;                // +100
if ( storm_form && static_charge active )
    max_dam += balance.damcap_dragonkin_storm;                // +150
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Base | +150 flat | `damcap_dragonkin_base` (default: 150) |
| Fury | +6 per point (max +600) | `damcap_dragonkin_fury_mult` (default: 6) |
| Flame (Burning Claws) | +100 flat | `damcap_dragonkin_flame` (default: 100) |
| Storm (Static Charge) | +150 flat | `damcap_dragonkin_storm` (default: 150) |

### Elemental Resistance/Vulnerability

Processed in damage calculation:
```c
if (IS_CLASS(victim, CLASS_DRAGONKIN)) {
    switch(victim->pcdata->powers[DRAGON_FORM]) {
        case FORM_FLAME:
            if (dam_type == DAM_FIRE) dam *= 0.50;      // 50% resistance
            if (dam_type == DAM_COLD) dam *= 1.25;      // 25% vulnerability
            break;
        // ... other forms
    }
}
```

### Tick Update

Per-tick processing:
1. Fury build (+3 fighting) or decay (-4 not fighting)
2. Peak fury tracking
3. Burning Claws/Frozen Scales/Static Charge/Toxic Fangs duration
4. Burn/Poison/Slow DoT effects on enemies
5. Scale Armor duration countdown
6. Immolate tick damage
7. Form-specific passive effects

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_DRAGONKIN bit (TBD) |
| fury | ch->rage | Current draconic fury (0-100) |
| current_form | ch->pcdata->powers[0] | Current form (0=none, 1=flame, 2=frost, 3=storm, 4=venom) |
| burning_claws | ch->pcdata->powers[1] | Burning Claws ticks remaining |
| frozen_scales | ch->pcdata->powers[2] | Frozen Scales ticks remaining |
| static_charge | ch->pcdata->powers[3] | Static Charge attacks/ticks remaining |
| toxic_fangs | ch->pcdata->powers[4] | Toxic Fangs ticks remaining |
| scale_armor | ch->pcdata->powers[5] | Scale Armor ticks remaining |
| flame_mastery | ch->pcdata->powers[10] | Flame Mastery training level (0-3) |
| frost_mastery | ch->pcdata->powers[11] | Frost Mastery training level (0-3) |
| storm_mastery | ch->pcdata->powers[12] | Storm Mastery training level (0-3) |
| venom_mastery | ch->pcdata->powers[13] | Venom Mastery training level (0-3) |
| draconic_body | ch->pcdata->powers[14] | Draconic Body training level (0-3) |
| scale_armor_hp | ch->pcdata->stats[0] | Current scale armor HP |
| peak_fury | ch->pcdata->stats[1] | Highest fury this session |
| form_shifts | ch->pcdata->stats[2] | Total form shifts (tracking) |
