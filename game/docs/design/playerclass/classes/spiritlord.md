# Spirit Lord Class Design

## Overview

Spirit Lords have transcended the boundary between material and spirit. Where Shamans place external totems, Spirit Lords have internalized them — they ARE the totem. Spirits are imbued within their very being, granting persistent auras that move with them, the ability to command and summon spirit armies, and ultimately the power to shed their physical form entirely. Their enhanced Spirit Tether range (0-150) allows deeper communion with both worlds. This is an **upgrade class** from Shaman.

**Source Files**: `src/classes/spiritlord.c`, `src/classes/shaman.h`
**Class Constant**: `CLASS_SPIRITLORD` (134217728)
**Upgrades From**: Shaman

## Color Scheme

Spirit Lord uses a spectral blue/silver palette evoking the ethereal spirit world and ancestral light. Colors use true color (`#tRRGGBB`) with automatic xterm-256 fallback:

| Element | Code | Fallback | Color | Usage |
|---------|------|----------|-------|-------|
| Accent | `#t4060A0` | `#x061` | Spectral indigo | Bracket decorations, spirit accents |
| Primary | `#t90B8E0` | `#x117` | Luminous sky | Class name, titles, ability highlights |
| Bracket open | `#t4060A0{{#t90B8E0` | | Indigo `{{` Sky | Who list open bracket |
| Bracket close | `#t90B8E0#t4060A0}}` | | Sky `}}` Indigo | Who list close bracket |
| Room tag | `#t90B8E0(#nSpirit Lord#t90B8E0)` | | Sky parens | Room display prefix |

**Who List Titles** (gradient from spectral indigo → luminous sky):

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Ancestral God | `#t4060A0{{#t4060A0Anc#t587CC0est#t70A0D0ral #t90B8E0God#n#t90B8E0#t4060A0}}` |
| 2 | Spirit King | `#t4060A0{{#t4060A0Spi#t587CC0rit #t70A0D0Ki#t90B8E0ng#n#t90B8E0#t4060A0}}` |
| 3 | Spirit Lord | `#t4060A0{{#t4060A0Spi#t587CC0rit #t70A0D0Lo#t90B8E0rd#n#t90B8E0#t4060A0}}` |
| 4 | Seer | `#t4060A0{{#t4060A0Se#t688CB0e#t90B8E0r#n#t90B8E0#t4060A0}}` |
| default | Whisperer | `#t4060A0{{#t4060A0Whi#t587CC0spe#t70A0D0re#t90B8E0r#n#t90B8E0#t4060A0}}` |

## Core Mechanics

### Enhanced Spirit Tether

Spirit Lords have an expanded tether range, stored in `ch->rage`:

```c
ch->rage  // Current tether (0-150 for Spirit Lord, vs 0-100 for Shaman)
```

**Starting Tether**: 75 (balanced center)

**Tether Movement**:
- Embodiment abilities (internalized totems): DECREASE tether (grounds you)
- Dominion abilities (spirit command): INCREASE tether (ascends)
- Passive: Drifts toward 75 (+/-1 per tick — slower than Shaman's ±2)

### The Balance Mechanic (Enhanced)

| Tether | Zone | Effect |
|--------|------|--------|
| 0-29 | Anchored | Material abilities +40% power, Spirit -60% power, immune to spirit attacks |
| 30-59 | Earthbound | Material +20% power, Spirit -30% power |
| 60-90 | Balanced | All abilities normal power, can use Embodiment |
| 91-120 | Spirit-Touched | Spirit +20% power, Material -30% power |
| 121-150 | Ascended | Spirit +40% power, Material -60% power, phase through walls |

**Upgrade Advantage**: The wider range (0-150 vs 0-100) allows Spirit Lords to push deeper into each extreme, achieving more powerful bonuses at the cost of greater risk.

### "Become the Totem"

The defining upgrade mechanic: where Shamans place totems externally in rooms, Spirit Lords have internalized the totems as personal auras:

| Shaman (External) | Spirit Lord (Internal) |
|-------------------|----------------------|
| Totems placed in a room | Auras imbued within self |
| Only one active at a time | Multiple auras via progression |
| Stationary — room-locked | Mobile — moves with you |
| Full power (100%) | Reduced power (70%) unless fused |
| Anyone can destroy the totem | Cannot be dispelled |

### Spirit Manifestation (Enhanced)

At extreme tether levels (0-15 or 135-150), there's a 10% chance per tick of spirit manifestation:

| Effect | Description |
|--------|-------------|
| Ancestral Blessing | Heal 10% max HP |
| Spirit Strike | A spirit attacks you (take damage) |
| Guidance | +20% dodge for 1 round |
| Interference | Lose 1 attack this round |

### Tether Display

Command: `tether`
- Shows current tether with visual indicator of position (0-150 scale)
- Shows current zone (Anchored/Earthbound/Balanced/Spirit-Touched/Ascended)
- Lists power modifiers for each ability type
- Shows active auras, spirit summons, and effects with remaining durations
- Available to both Shaman and Spirit Lord

## Training Categories

Spirit Lord abilities are organized into 3 trainable categories, replacing the Shaman's categories upon upgrade. Stored in `ch->pcdata->powers[]`:

| Index | Category | Max Level | Theme | Abilities |
|-------|----------|-----------|-------|-----------|
| SL_TRAIN_EMBODY (10) | Embodiment | 3 | Internalized totems | Embody, Ancestral Form, Spirit Fusion |
| SL_TRAIN_DOMINION (11) | Dominion | 4 | Spirit command | Compel, Possess, Spirit Army, Soul Storm |
| SL_TRAIN_TRANSCEND (12) | Transcendence | 3 | Ultimate powers | Ancestral Wisdom, Spirit Cleanse, Ascension |

**Training Cost**: `(current_level + 1) * 45` primal per level (`ability.sl.train.cost_mult`)

**Training Command**: `lordtrain` - Display current levels and improve categories

### Embodiment (Internalized Totems — Decreases Tether)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `embody` | Toggle a totem aura on/off |
| 2 | `ancestralform` | Transform into a spirit warrior |
| 3 | `spiritfusion` | Merge all three auras simultaneously |

### Dominion (Spirit Command — Increases Tether)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `compel` | Compel nearby spirits to attack |
| 2 | `possess` | Briefly dominate an NPC's actions |
| 3 | `spiritarmy` | Summon multiple spirit warriors |
| 4 | `soulstorm` | Devastating spiritual AoE |

### Transcendence (Ultimate Powers — Neutral)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `ancestralwisdom` | Channel all ancestors at once |
| 2 | `spiritcleanse` | Purify with ancestral power |
| 3 | `ascension` | Become a pure spirit |

## Abilities

### Embody - Totem Aura Toggle (Embodiment)

Internalizes a totem's power as a personal aura that moves with you.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `ability.sl.embody.mana_cost` |
| Tether Change | -8 on activation | `ability.sl.embody.tether_change` |
| Duration | Until toggled off or tether hits 0 | |
| Requires | Embodiment 1 | |

**Usage**: `embody ward` / `embody wrath` / `embody spirit`

**Effects**:
- Ward Aura: +10% dodge, +7% parry (70% of Ward Totem's power)
- Wrath Aura: 70-140 damage per tick to fighting target
- Spirit Aura: Drain 21 mana/tick from target, restore 10 to self
- Only one aura at a time at Embodiment level 1
- Aura is always active and moves with you (no room restriction)

### Ancestral Form - Spirit Warrior (Embodiment)

Transforms into a spectral warrior, channeling ancestral battle prowess.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `ability.sl.ancestralform.mana_cost` |
| Tether Change | -20 | `ability.sl.ancestralform.tether_change` |
| Duration | 8 ticks | `ability.sl.ancestralform.duration` |
| Cooldown | 20 pulses | `ability.sl.ancestralform.cooldown` |
| Extra Attacks | +3 | `ability.sl.ancestralform.extra_attacks` |
| Dodge Bonus | +20% | `ability.sl.ancestralform.dodge_bonus` |
| Damage Bonus | +20% | `ability.sl.ancestralform.damage_bonus` |
| Requires | Embodiment 2 | |

**Effects**:
- +3 extra attacks, +20% dodge, +20% damage
- Spectral appearance visible in room description
- Can maintain 2 embodied auras simultaneously while in this form
- Spectral warriors visible to those with spirit sight

### Spirit Fusion - Triple Aura (Embodiment)

Merges all three totem auras simultaneously at full power.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 150 | `ability.sl.spiritfusion.mana_cost` |
| Tether Required | > 30 | |
| Tether Change | -30 | `ability.sl.spiritfusion.tether_change` |
| Duration | 6 ticks | `ability.sl.spiritfusion.duration` |
| Cooldown | 25 pulses | `ability.sl.spiritfusion.cooldown` |
| Mana per Tick | 10 | `ability.sl.spiritfusion.mana_tick` |
| Requires | Embodiment 3 | |

**Effects**:
- All three totem effects active at 100% power simultaneously:
  - Ward: +15% dodge, +10% parry
  - Wrath: 100-200 damage per tick to fighting target
  - Spirit: Drain 30 mana/tick from target, restore 15 to self
- Costs 10 mana per tick to maintain
- Can only be activated when tether > 30 (not already too grounded)
- Overrides individual Embody aura (replaces it for duration)

### Compel - Spirit Assault (Dominion)

Compels nearby spirits in the area to attack a target.

**Note:** Renamed from "command" to avoid conflict with Vampire's `do_command` in `act_comm.c`.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `ability.sl.compel.mana_cost` |
| Tether Change | +12 | `ability.sl.compel.tether_change` |
| Cooldown | 6 pulses | `ability.sl.compel.cooldown` |
| Base Damage | 200-400 | `ability.sl.compel.base_damage` |
| Requires | Fighting, Dominion 1 | |

**Effects**:
- Spirits in room attack target for 200-400 damage
- Cannot be dodged (spirit realm attack)
- More spirits appear at higher tether levels: +50% damage above tether 90

**Tether Scaling**: At tether 91-150, damage gains +50% bonus.

### Possess - NPC Domination (Dominion)

Briefly dominates an NPC's mind with overwhelming spirit energy.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `ability.sl.possess.mana_cost` |
| Tether Change | +18 | `ability.sl.possess.tether_change` |
| Duration | 4 rounds | `ability.sl.possess.duration` |
| Cooldown | 20 pulses | `ability.sl.possess.cooldown` |
| Requires | Dominion 2 | |

**Effects**:
- Target NPC fights on your side for duration
- If target dies while possessed, Tether +20 (spirit released back to you)
- Only works on NPCs, not players
- Possessed NPC uses its own attacks and abilities against your targets

### Spirit Army - Summon Warriors (Dominion)

Summons multiple spirit warriors from the ancestral plane.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 130 | `ability.sl.spiritarmy.mana_cost` |
| Tether Change | +25 | `ability.sl.spiritarmy.tether_change` |
| Duration | 6 ticks | `ability.sl.spiritarmy.duration` |
| Cooldown | 25 pulses | `ability.sl.spiritarmy.cooldown` |
| Warrior Count | 3 | `ability.sl.spiritarmy.count` |
| Requires | Dominion 3 | |

**Effects**:
- Summon 3 spirit warriors that fight alongside you
- Each warrior: 30% of Spirit Lord's damage, 20% of max HP
- Warriors automatically engage the Spirit Lord's current target
- Warriors are immune to physical damage (spirit forms) but can be banished
- If Spirit Lord flees, warriors vanish

### Soul Storm - Spiritual Devastation (Dominion)

Unleashes all accumulated spirit energy in a devastating storm.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 160 | `ability.sl.soulstorm.mana_cost` |
| Tether Required | > 100 | |
| Cooldown | 30 pulses | `ability.sl.soulstorm.cooldown` |
| Base Damage | 400-700 + (tether - 75) × 5 | |
| Tether Aftermath | Moves 30 toward center | |
| Requires | Fighting, Dominion 4 | |

**Effects**:
- All enemies in room take 400-700 + (tether - 75) × 5 damage
- Strips 1 buff from each target hit
- Tether moves 30 points toward center afterward (spiritual exhaustion)
- Can only be activated when tether > 100 (deep in spirit territory)

**Damage at Max Tether (150)**: 400-700 + 375 = 775-1075 damage

### Ancestral Wisdom - Channel Ancestors (Transcendence)

Opens yourself to the combined wisdom of all ancestors at once.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `ability.sl.ancestralwisdom.mana_cost` |
| Duration | 10 ticks | `ability.sl.ancestralwisdom.duration` |
| Cooldown | 15 pulses | `ability.sl.ancestralwisdom.cooldown` |
| Damage Bonus | +15% | `ability.sl.ancestralwisdom.damage_bonus` |
| Dodge Bonus | +10% | `ability.sl.ancestralwisdom.dodge_bonus` |
| Parry Bonus | +10% | `ability.sl.ancestralwisdom.parry_bonus` |
| Regen | 2% max HP per tick | `ability.sl.ancestralwisdom.regen_pct` |
| Requires | Transcendence 1 | |

**Effects**:
- +15% to all damage, +10% dodge, +10% parry
- Regenerate 2% max HP per tick
- Power scales with tether balance (closer to center = stronger)
- At perfect balance (tether 70-80): All bonuses increased by 50%

### Spirit Cleanse - Ancestral Purification (Transcendence)

Purifies body and spirit with concentrated ancestral power.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `ability.sl.spiritcleanse.mana_cost` |
| Cooldown | 20 pulses | `ability.sl.spiritcleanse.cooldown` |
| Heal | 20% max HP | `ability.sl.spiritcleanse.heal_pct` |
| Requires | Transcendence 2 | |

**Effects**:
- Remove all debuffs from self
- Heal 20% max HP
- Reset tether to center (75) — ancestors restore balance
- Powerful emergency reset when tether is at a dangerous extreme

### Ascension - Pure Spirit Form (Transcendence)

Sheds the physical form entirely, becoming a being of pure spirit.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 250 | `ability.sl.ascension.mana_cost` |
| Tether Cost | Resets to 75 | |
| Duration | 6 ticks | `ability.sl.ascension.duration` |
| Cooldown | 60 pulses | `ability.sl.ascension.cooldown` |
| Aftermath | 2 rounds stunned | `ability.sl.ascension.aftermath` |
| Requires | Transcendence 3 | |

**Effects**:
- Immune to all physical damage (spirits cannot be touched)
- All spirit-type abilities cost 0 mana, 0 cooldown
- Cannot use material/totem/embodiment abilities (you ARE spirit)
- Physical attacks deal 50% damage (partially phased)
- Spirit bolt auto-fires every round for free (internal mechanic in `update_spiritlord()`, not the Shaman's `do_spiritbolt` — Spirit Lord replaces all Shaman abilities on upgrade)
- When Ascension ends: Cannot act for 2 rounds (re-materializing)

**Strategic Choice**: Devastating burst of spirit power, but the aftermath leaves you vulnerable. Time it carefully or die during re-materialization.

## Spirit Lord Armor

Create equipment via `lordarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| ethereal scepter | 33600 | Wield |
| phantom ring | 33601 | Finger |
| ancestor's pendant | 33602 | Neck |
| spirit-woven robes | 33603 | Torso |
| crown of spirits | 33604 | Head |
| spectral greaves | 33605 | Legs |
| ghost-step boots | 33606 | Feet |
| ethereal gauntlets | 33607 | Hands |
| spirit-forged vambraces | 33608 | Arms |
| cloak of ancestors | 33609 | About Body |
| phantom sash | 33610 | Waist |
| soul bracer | 33611 | Wrist |
| visage of the beyond | 33612 | Face |
| mastery item | 33615 | Hold |

**Equipment Stats**:
- Weapons: +45 hitroll, +45 damroll
- Armor: +40 hitroll, +40 damroll, -45 AC
- Mastery: +75 hitroll, +75 damroll

**Restrictions**: Equipment restricted to CLASS_SPIRITLORD only.

## Combat Mechanics

### Damage Cap Bonuses

```c
// Tether position bonus (extremes grant more power)
int tether_extreme = abs(ch->rage - 75);  // 0-75 range
max_dam += tether_extreme * balance.damcap_sl_extreme;      // +5 per point from center
if ( ancestral_form active )
    max_dam += balance.damcap_sl_ancestral;                  // +150
if ( spirit_army active )
    max_dam += balance.damcap_sl_army;                       // +100
if ( ascension active )
    max_dam /= 2;                                            // -50% (phased)
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Tether Extreme | +5 per point from center (max +375) | `combat.damcap.sl.extreme_mult` (default: 5) |
| Ancestral Form | +150 flat | `combat.damcap.sl.ancestral` (default: 150) |
| Spirit Army | +100 flat | `combat.damcap.sl.army` (default: 100) |
| Ascension | -50% (phased) | N/A |

### Tether-Based Ability Scaling

```c
int get_sl_power_mod(CHAR_DATA *ch, bool is_material) {
    int tether = ch->rage;
    if (is_material) {
        if (tether <= 29) return 140;      // +40%
        if (tether <= 59) return 120;      // +20%
        if (tether <= 90) return 100;      // normal
        if (tether <= 120) return 70;      // -30%
        return 40;                          // -60%
    } else { // spirit
        if (tether <= 29) return 40;       // -60%
        if (tether <= 59) return 70;       // -30%
        if (tether <= 90) return 100;      // normal
        if (tether <= 120) return 120;     // +20%
        return 140;                         // +40%
    }
}
```

### Tick Update

Per-tick processing:
1. Tether drift toward 75 (+/-1 per tick, slower than Shaman)
2. Spirit Manifestation check at extremes (0-15 or 135-150)
3. Embody aura effects (damage, drain, or defense)
4. Ancestral Form duration countdown
5. Spirit Fusion duration countdown + mana cost
6. Spirit Army duration countdown + warrior attacks
7. Possess duration countdown
8. Ancestral Wisdom duration countdown + HP regen
9. Ascension duration + aftermath countdown
10. Soul Storm tether correction

## Mob VNUMs

Spirit Army warriors are mob entities (like Artificer turrets) that fight alongside the Spirit Lord:

| Define | Vnum | Purpose |
|--------|------|---------|
| `VNUM_SL_SPIRIT_WARRIOR` | 33616 | Spirit warrior summon (placed in room via `create_mobile()`) |

Warrior stats are set programmatically at spawn time (30% of Spirit Lord's damage, 20% of max HP). The mob template in `classeq.db` is minimal.

## Score Display Stats

Custom stats shown in the `score` command via `class_score_stats` table:

| Label | Source | Notes |
|-------|--------|-------|
| Spirit Tether | STAT_RAGE | Current tether position (0-150) |
| Active Aura | Custom | Which aura is embodied (Ward/Wrath/Spirit/None) |
| Embodiment | Custom | Embodiment training level (0-3) |
| Dominion | Custom | Dominion training level (0-4) |
| Transcendence | Custom | Transcendence training level (0-3) |

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_SPIRITLORD bit (134217728) |
| tether | ch->rage | Current spirit tether (0-150) |
| embody_type | ch->pcdata->powers[0] | Current aura (0=none, 1=ward, 2=wrath, 3=spirit) |
| ancestral_form | ch->pcdata->powers[1] | Ancestral Form ticks remaining |
| spirit_fusion | ch->pcdata->powers[2] | Spirit Fusion ticks remaining |
| spirit_army | ch->pcdata->powers[3] | Spirit Army ticks remaining |
| possess_target | ch->pcdata->powers[4] | Possess target NPC ID |
| possess_ticks | ch->pcdata->powers[5] | Possess rounds remaining |
| ascension_ticks | ch->pcdata->powers[6] | Ascension ticks remaining |
| ascension_aftermath | ch->pcdata->powers[7] | Ascension aftermath rounds |
| wisdom_ticks | ch->pcdata->powers[8] | Ancestral Wisdom ticks remaining |
| (reserved) | ch->pcdata->powers[9] | |
| embody_training | ch->pcdata->powers[10] | Embodiment training level (0-3) |
| dominion_training | ch->pcdata->powers[11] | Dominion training level (0-4) |
| transcend_training | ch->pcdata->powers[12] | Transcendence training level (0-3) |
| (reserved) | ch->pcdata->powers[13] | |
| ancestralform_cd | ch->pcdata->powers[14] | Ancestral Form cooldown ticks remaining |
| spiritfusion_cd | ch->pcdata->powers[15] | Spirit Fusion cooldown ticks remaining |
| compel_cd | ch->pcdata->powers[16] | Compel cooldown ticks remaining |
| possess_cd | ch->pcdata->powers[17] | Possess cooldown ticks remaining |
| spiritarmy_cd | ch->pcdata->powers[18] | Spirit Army cooldown ticks remaining |
| soulstorm_cd | ch->pcdata->powers[19] | Soul Storm cooldown ticks remaining |
| ascensions_used | ch->pcdata->stats[0] | Times Ascension used (tracking) |
| wisdom_cd | ch->pcdata->stats[1] | Ancestral Wisdom cooldown ticks remaining |
| cleanse_cd | ch->pcdata->stats[2] | Spirit Cleanse cooldown ticks remaining |
| ascension_cd | ch->pcdata->stats[3] | Ascension cooldown ticks remaining |
| army_count | ch->pcdata->stats[4] | Current spirit warrior count |
