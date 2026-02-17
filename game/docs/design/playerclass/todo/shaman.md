# Shaman Class Design

## Overview

Shamans are spiritual conduits who straddle the material and spirit worlds. They place totems to create zones of power, channel ancestral spirits for offense and defense, and commune with the dead for wisdom and healing. Their unique resource, Spirit Tether, represents their position between the material and spirit worlds — too far toward either extreme enhances certain abilities while weakening others. This is a **base class** that upgrades to Spirit Lord.

**Source Files**: `src/classes/shaman.c`, `src/classes/shaman.h`
**Class Constant**: `CLASS_SHAMAN` (67108864)
**Upgrades To**: Spirit Lord

## Color Scheme

Shaman uses a teal/jade palette evoking sacred waters, healing stones, and natural spirits. Colors use true color (`#tRRGGBB`) with automatic xterm-256 fallback:

| Element | Code | Fallback | Color | Usage |
|---------|------|----------|-------|-------|
| Accent | `#t1A7A6A` | `#x030` | Deep teal | Bracket decorations, ritual accents |
| Primary | `#t5EC4B0` | `#x079` | Jade green | Class name, titles, ability highlights |
| Bracket open | `#t1A7A6A\|~#t5EC4B0` | | Teal `\|~` Jade | Who list open bracket |
| Bracket close | `#t5EC4B0~#t1A7A6A\|` | | Jade `~\|` Teal | Who list close bracket |
| Room tag | `#t5EC4B0(#nShaman#t5EC4B0)` | | Jade parens | Room display prefix |

**Who List Titles** (gradient from deep teal → jade):

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Spirit Elder | `#t1A7A6A\|~#t1A7A6ASpi#t2E9080rit #t42AA96Eld#t5EC4B0er#n#t5EC4B0~#t1A7A6A\|` |
| 2 | Elder Shaman | `#t1A7A6A\|~#t1A7A6AEld#t309888er #t46B09ESha#t5EC4B0man#n#t5EC4B0~#t1A7A6A\|` |
| 3 | Shaman | `#t1A7A6A\|~#t1A7A6ASha#t3CA290ma#t5EC4B0n#n#t5EC4B0~#t1A7A6A\|` |
| 4 | Spirit Talker | `#t1A7A6A\|~#t1A7A6ASpi#t2E9080rit #t42AA96Tal#t5EC4B0ker#n#t5EC4B0~#t1A7A6A\|` |
| 5 | Initiate | `#t1A7A6A\|~#t1A7A6AIni#t3CA290tia#t5EC4B0te#n#t5EC4B0~#t1A7A6A\|` |
| default | Novice | `#t1A7A6A\|~#t1A7A6ANov#t3CA290ic#t5EC4B0e#n#t5EC4B0~#t1A7A6A\|` |

## Core Mechanics

### Spirit Tether

Spirit Tether represents the Shaman's position between the material and spirit worlds, stored in `ch->rage`:

```c
ch->rage  // Current tether (0-100 for Shaman, 0-150 for Spirit Lord)
```

**Starting Tether**: 50 (balanced between worlds)

**Tether Movement**:
- Material abilities (totems, physical): DECREASE tether (grounds you)
- Spirit abilities (spirit attacks, commune): INCREASE tether (unmoored)
- Passive: Drifts toward 50 (+/-2 per tick, toward center)

### The Balance Mechanic

| Tether | Zone | Effect |
|--------|------|--------|
| 0-19 | Grounded | Material abilities +30% power, Spirit -50% power |
| 20-39 | Earthbound | Material +15% power, Spirit -25% power |
| 40-60 | Balanced | All abilities normal power |
| 61-80 | Spirit-Touched | Spirit +15% power, Material -25% power |
| 81-100 | Unmoored | Spirit +30% power, Material -50% power, see invisible spirits |

**Strategic Depth**: Push tether toward material for powerful totems or toward spirit for devastating spirit attacks, but the opposite type weakens. Stay balanced for versatility.

### Spirit Manifestation

At extreme tether levels (0-10 or 90-100), there's a 10% chance per tick of spirit manifestation:

| Effect | Description |
|--------|-------------|
| Ancestral Blessing | Heal 10% max HP |
| Spirit Strike | A spirit attacks you (take damage) |
| Guidance | +20% dodge for 1 round |
| Interference | Lose 1 attack this round |

### Tether Display

Command: `tether`
- Shows current tether with visual indicator of position (0-100 scale)
- Shows current zone (Grounded/Earthbound/Balanced/Spirit-Touched/Unmoored)
- Lists power modifiers for each ability type
- Shows active totems and spirit effects with remaining durations
- Available to both Shaman and Spirit Lord

## Training Categories

Shaman abilities are organized into 3 trainable categories. Each category has 3 levels, unlocking abilities progressively. Stored in `ch->pcdata->powers[]`:

| Index | Category | Theme | Tether Direction | Abilities |
|-------|----------|-------|------------------|-----------|
| SHAMAN_TRAIN_TOTEM (10) | Totems | Zone control | DECREASES tether | Ward Totem, Wrath Totem, Spirit Totem |
| SHAMAN_TRAIN_SPIRIT (11) | Spirits | Offense/defense | INCREASES tether | Spirit Bolt, Spirit Ward, Spirit Walk |
| SHAMAN_TRAIN_COMMUNE (12) | Communion | Utility/healing | NEUTRAL | Ancestor Call, Spirit Sight, Soul Link |

**Training Cost**: `(current_level + 1) * 40` primal per level (`ability.shaman.train.cost_mult`)

**Training Command**: `spirittrain` - Display current levels and improve categories

### Totems (Material — Decreases Tether)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `wardtotem` | Place a defensive totem in the room |
| 2 | `wrathtotem` | Place an offensive totem |
| 3 | `spirittotem` | Place a draining totem |

### Spirits (Spirit — Increases Tether)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `spiritbolt` | Call a spirit to strike an enemy |
| 2 | `spiritward` | Surround yourself with protective spirits |
| 3 | `spiritwalk` | Step partially into the spirit plane |

### Communion (Utility — Neutral)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `ancestorcall` | Heal from ancestral wisdom |
| 2 | `spiritsight` | See through spirit eyes |
| 3 | `soullink` | Link your spirit to a target |

## Abilities

### Ward Totem - Defensive Zone (Totems)

Places a ward totem in the room that protects all allies.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `ability.shaman.wardtotem.mana_cost` |
| Tether Change | -12 | `ability.shaman.wardtotem.tether_change` |
| Duration | 8 ticks | `ability.shaman.wardtotem.duration` |
| Dodge Bonus | +15% | `ability.shaman.wardtotem.dodge_bonus` |
| Parry Bonus | +10% | `ability.shaman.wardtotem.parry_bonus` |
| Requires | Totems 1 | |

**Effects**:
- All allies in room gain +15% dodge, +10% parry
- Totem has HP and can be destroyed by enemies attacking it
- Only one totem active at a time (placing a new totem removes the old one)
- Totem remains in the room even if Shaman leaves (but duration still ticks)

**Tether Scaling**: At tether 0-39, dodge bonus increases to +20%.

### Wrath Totem - Offensive Zone (Totems)

Places a totem of wrath that damages all enemies in the room.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `ability.shaman.wrathtotem.mana_cost` |
| Tether Change | -15 | `ability.shaman.wrathtotem.tether_change` |
| Duration | 6 ticks | `ability.shaman.wrathtotem.duration` |
| Cooldown | 10 pulses | `ability.shaman.wrathtotem.cooldown` |
| Tick Damage | 100-200 + power_mod | `ability.shaman.wrathtotem.tick_damage` |
| Requires | Totems 2 | |

**Effects**:
- Deals damage to all enemies in room each tick
- Damage scales with material power modifier (stronger when grounded)
- Only one totem active at a time

**Tether Scaling**: At tether 0-39, tick damage gains +30% bonus.

### Spirit Totem - Draining Zone (Totems)

Places a totem that drains the essence of nearby enemies.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `ability.shaman.spirittotem.mana_cost` |
| Tether Change | -20 | `ability.shaman.spirittotem.tether_change` |
| Duration | 6 ticks | `ability.shaman.spirittotem.duration` |
| Cooldown | 15 pulses | `ability.shaman.spirittotem.cooldown` |
| Mana Drain | 30 per tick per enemy | `ability.shaman.spirittotem.mana_drain` |
| Mana Restore | 15 per tick to Shaman | `ability.shaman.spirittotem.mana_restore` |
| Requires | Totems 3 | |

**Effects**:
- Drains 30 mana per tick from each enemy in the room
- Restores 15 mana per tick to the Shaman
- Only works while the Shaman is in the same room as the totem
- Only one totem active at a time

### Spirit Bolt - Spirit Strike (Spirits)

Calls a spirit from the other world to strike an enemy.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `ability.shaman.spiritbolt.mana_cost` |
| Tether Change | +10 | `ability.shaman.spiritbolt.tether_change` |
| Cooldown | 4 pulses | `ability.shaman.spiritbolt.cooldown` |
| Base Damage | 150-300 + (tether × 2) | |
| Damage Type | Spirit | |
| Requires | Fighting, Spirits 1 | |

**Effects**:
- Cannot be dodged (spirit realm attack bypasses physical defenses)
- Higher tether = stronger damage (spirits answer more readily)

**Tether Scaling**: At tether 61-100, damage gains spirit power bonus.

### Spirit Ward - Protective Spirits (Spirits)

Surrounds yourself with protective ancestor spirits.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `ability.shaman.spiritward.mana_cost` |
| Tether Change | +15 | `ability.shaman.spiritward.tether_change` |
| Duration | 8 ticks | `ability.shaman.spiritward.duration` |
| Charges | 3 | `ability.shaman.spiritward.charges` |
| Reflect Damage | 50% | `ability.shaman.spiritward.reflect_pct` |
| Requires | Spirits 2 | |

**Effects**:
- Absorb next 3 attacks (each negated entirely, spirit takes the hit)
- Each absorbed attack deals 50% of the original damage back to attacker
- Duration expires when all charges are consumed or timer runs out

**Tether Scaling**: At tether 61-100, gains 1 additional charge (4 total).

### Spirit Walk - Phase Shift (Spirits)

Steps partially into the spirit plane, existing between worlds.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `ability.shaman.spiritwalk.mana_cost` |
| Tether Required | < 70 | |
| Tether Change | +20 | `ability.shaman.spiritwalk.tether_change` |
| Duration | 4 ticks | `ability.shaman.spiritwalk.duration` |
| Cooldown | 20 pulses | `ability.shaman.spiritwalk.cooldown` |
| Dodge Chance | 50% | `ability.shaman.spiritwalk.dodge_chance` |
| Requires | Spirits 3 | |

**Effects**:
- 50% chance to avoid any attack (phased between worlds)
- Can see all hidden/invisible entities
- Movement costs 0 (spirit plane travel)
- Can only be activated when tether < 70 (not already too spirit-heavy)

### Ancestor Call - Spirit Healing (Communion)

Calls upon ancestral wisdom for healing.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `ability.shaman.ancestorcall.mana_cost` |
| Heal Amount | 300-500 + balance bonus | `ability.shaman.ancestorcall.base_heal` |
| Requires | Communion 1 | |

**Effects**:
- Instant heal with no cooldown
- Healing scales with tether balance (closer to 50 = more healing)
- At perfect balance (tether 45-55): +50% healing bonus
- Ancestors favor harmony between the worlds

### Spirit Sight - Spirit Vision (Communion)

Opens the spirit eyes to perceive what others cannot.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `ability.shaman.spiritsight.mana_cost` |
| Duration | 10 ticks | `ability.shaman.spiritsight.duration` |
| Requires | Communion 2 | |

**Effects**:
- Grants see invisible, see hidden, detect alignment
- See enemy HP percentage
- Warning message before enemy special attacks
- Can detect spirits and spirit-touched entities

### Soul Link - Spirit Bond (Communion)

Links your spirit to another being, sharing fate.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `ability.shaman.soullink.mana_cost` |
| Duration | 6 ticks | `ability.shaman.soullink.duration` |
| Cooldown | 15 pulses | `ability.shaman.soullink.cooldown` |
| Damage Share | 60% | `ability.shaman.soullink.damage_share` |
| Link Death Damage | 500 | `ability.shaman.soullink.death_damage` |
| Requires | Fighting, Communion 3 | |

**Effects**:
- You and target share damage (each takes 60% of damage dealt to either)
- If target dies while linked, Shaman takes 500 damage
- Powerful with allies (split incoming damage), risky with enemies
- Only one link active at a time

## Shaman Armor

Create equipment via `shamanarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| spirit staff | 33580 | Wield |
| bone ring | 33581 | Finger |
| totem necklace | 33582 | Neck |
| spirit hides | 33583 | Torso |
| antler headdress | 33584 | Head |
| hide leggings | 33585 | Legs |
| moccasins | 33586 | Feet |
| spirit-etched gloves | 33587 | Hands |
| bone bracers | 33588 | Arms |
| feathered cloak | 33589 | About Body |
| spirit sash | 33590 | Waist |
| ancestral bracelet | 33591 | Wrist |
| spirit mask | 33592 | Face |
| mastery item | 33595 | Hold |

**Equipment Stats**:
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC
- Mastery: +50 hitroll, +50 damroll

**Restrictions**: Equipment restricted to CLASS_SHAMAN and CLASS_SPIRITLORD only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += balance.damcap_shaman_base;                      // +150 flat
// Tether position bonus (extremes grant more power)
int tether_extreme = abs(ch->rage - 50);  // 0-50 range
max_dam += tether_extreme * balance.damcap_shaman_extreme;  // +4 per point from center
if ( wrath_totem active )
    max_dam += balance.damcap_shaman_wrath;                  // +100
if ( spirit_walk active )
    max_dam += balance.damcap_shaman_spiritw;                // +100
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Base | +150 flat | `combat.damcap.shaman.base` (default: 150) |
| Tether Extreme | +4 per point from center (max +200) | `combat.damcap.shaman.extreme_mult` (default: 4) |
| Wrath Totem | +100 flat | `combat.damcap.shaman.wrath` (default: 100) |
| Spirit Walk | +100 flat | `combat.damcap.shaman.spiritw` (default: 100) |

### Tether-Based Ability Scaling

```c
int get_shaman_power_mod(CHAR_DATA *ch, bool is_material) {
    int tether = ch->rage;
    if (is_material) {
        if (tether <= 19) return 130;      // +30%
        if (tether <= 39) return 115;      // +15%
        if (tether <= 60) return 100;      // normal
        if (tether <= 80) return 75;       // -25%
        return 50;                          // -50%
    } else { // spirit
        if (tether <= 19) return 50;       // -50%
        if (tether <= 39) return 75;       // -25%
        if (tether <= 60) return 100;      // normal
        if (tether <= 80) return 115;      // +15%
        return 130;                         // +30%
    }
}
```

### Tick Update

Per-tick processing:
1. Tether drift toward 50 (+/-2 per tick)
2. Spirit Manifestation check at extremes (0-10 or 90-100)
3. Ward Totem duration countdown + HP check
4. Wrath Totem duration countdown + AoE damage
5. Spirit Totem duration countdown + mana drain/restore
6. Spirit Ward charge tracking
7. Spirit Walk duration countdown
8. Spirit Sight duration countdown
9. Soul Link duration countdown + damage sharing
10. Ancestor Call balance bonus calculation

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_SHAMAN bit (67108864) |
| tether | ch->rage | Current spirit tether (0-100) |
| ward_totem | ch->pcdata->powers[0] | Ward Totem ticks remaining |
| wrath_totem | ch->pcdata->powers[1] | Wrath Totem ticks remaining |
| spirit_totem | ch->pcdata->powers[2] | Spirit Totem ticks remaining |
| spirit_ward | ch->pcdata->powers[3] | Spirit Ward charges remaining |
| spirit_walk | ch->pcdata->powers[4] | Spirit Walk ticks remaining |
| spirit_sight | ch->pcdata->powers[5] | Spirit Sight ticks remaining |
| soul_link | ch->pcdata->powers[6] | Soul Link ticks remaining |
| soul_link_target | ch->pcdata->powers[7] | Soul Link target ID |
| totem_training | ch->pcdata->powers[10] | Totem training level (0-3) |
| spirit_training | ch->pcdata->powers[11] | Spirit training level (0-3) |
| commune_training | ch->pcdata->powers[12] | Communion training level (0-3) |
| totem_hp | ch->pcdata->stats[0] | Current totem HP remaining |
| manifestation_count | ch->pcdata->stats[1] | Times manifestation triggered (tracking) |
