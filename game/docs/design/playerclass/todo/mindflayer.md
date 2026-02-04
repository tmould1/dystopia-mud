# Mindflayer Class Design

## Overview

Mindflayers are beings of supreme psionic power who have transcended their Psion origins to become masters of mental domination and consumption. They can enthral enemies to fight for them, consume memories and intellect, and unleash devastating psionic storms that shatter minds across entire areas. This is an **upgrade class** obtained by upgrading from Psion.

**Source Files**: `src/classes/mindflayer.c`, `src/classes/psion.h`
**Class Constant**: `CLASS_MINDFLAYER` (131072)
**Upgrades From**: Psion

## Color Scheme

Mindflayer uses a green/teal palette evoking alien intellect, aberration, and Far Realm horror:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x029` | Dark green | Bracket braces, decorative accents |
| Primary | `#x035` | Teal | Class name, titles, ability highlights |
| Bracket open | `#x035~#x029{` | Teal~DarkGreen{ | Who list open bracket (tentacle/brace) |
| Bracket close | `#x029}#x035~` | DarkGreen}Teal~ | Who list close bracket |
| Room tag | `#x035(#nMindflayer#x035)` | Teal parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Elder Brain | `#x035~#x029{#x035Elder Brain#n#x029}#x035~` |
| 2 | Mind Tyrant | `#x035~#x029{#x035Mind Tyrant#n#x029}#x035~` |
| 3 | Illithid | `#x035~#x029{#x035Illithid#n#x029}#x035~` |
| 4 | Brain Eater | `#x035~#x029{#x035Brain Eater#n#x029}#x035~` |
| default | Mindflayer | `#x035~#x029{#x035Mindflayer#n#x029}#x035~` |

## Core Mechanics

### Focus (Enhanced)

Mindflayer inherits the Focus system from Psion with a higher cap and faster build rate:

```c
ch->rage  // Current focus (0-150 for Mindflayer)
```

**Building Focus**:
- Combat: +2 per game tick while fighting (vs +1 for Psion)
- Mind Feed: +15 on successful consumption
- Meditation: +15 (vs +10 for Psion)
- Maximum: 150 (vs 100 for Psion)

**Focus Decay** (out of combat):
- -1 per tick when not fighting (slower decay than Psion's -2)

### Shared Psion Abilities

Mindflayer retains access to the `psifocus` status display command and `psimeditate`. All other Psion abilities are replaced by Mindflayer-specific powers.

**Note**: Commands are named `psifocus` and `psimeditate` to avoid conflicts with existing Samurai `focus` and act_move.c `meditate` commands.

### Upgrade Path

Psion upgrades to Mindflayer via `do_upgrade`:
- Requires standard upgrade costs (50K hp, 35K mana/move, 40K qp, gen 1)
- Class changes from `CLASS_PSION` to `CLASS_MINDFLAYER`
- All abilities reset; Mindflayer abilities become available

## Training Categories

Mindflayer abilities are organized into 3 trainable categories. Stored in `ch->pcdata->powers[]`:

| Index | Category | Max Level | Theme | Abilities |
|-------|----------|-----------|-------|-----------|
| MIND_TRAIN_DOMINATION (10) | Domination | 4 | Mind control | Enthral, Puppet, Hivemind, Mass Domination |
| MIND_TRAIN_CONSUMPTION (11) | Consumption | 3 | Mental drain | Mind Feed, Memory Drain, Intellect Devour |
| MIND_TRAIN_PSIONIC (12) | Psionic Storm | 3 | Area devastation | Psychic Maelstrom, Mind Blast, Reality Fracture |

**Training Cost**: `(current_level + 1) * 50` primal per level

**Training Command**: `mindtrain` - Display current levels and improve categories

### Domination (Mind Control)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `enthral` | Charm an NPC to fight for you |
| 2 | `puppet` | Take direct control of an NPC's actions |
| 3 | `hivemind` | Link multiple thralls for coordinated attacks |
| 4 | `massdomination` | Attempt to charm all NPCs in room |

### Consumption (Mental Drain)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `mindfeed` | Drain mental energy, heal self, build focus |
| 2 | `memorydrain` | Strip target's buffs and abilities temporarily |
| 3 | `intellectdevour` | Massive damage that can reduce target's max HP |

### Psionic Storm (Area Devastation)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `psychicmaelstrom` | AoE mental damage with confusion |
| 2 | `psiblast` | Cone attack that stuns and damages |
| 3 | `realityfracture` | Ultimate AoE that tears at sanity itself |

## Abilities

### Enthral - Mind Control

Dominates an NPC's mind, forcing them to serve you. Sets AFF_CHARM, master, and leader.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `mindflayer.enthral.mana_cost` |
| Focus Required | 40 | `mindflayer.enthral.focus_req` |
| Focus Cost | 30 | `mindflayer.enthral.focus_cost` |
| Max Thralls | 3 | `mindflayer.enthral.max_thralls` |
| Duration | 40 ticks | `mindflayer.enthral.duration` |
| Restriction | NPCs only, not bosses | |
| Requires | Domination 1 | |

**Mechanic**: Higher level NPCs have a chance to resist based on their level vs Mindflayer's generation.

### Puppet - Direct Control

Take over an enthralled NPC completely, issuing commands they must obey.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `mindflayer.puppet.mana_cost` |
| Focus Cost | 20 | `mindflayer.puppet.focus_cost` |
| Duration | 5 ticks | `mindflayer.puppet.duration` |
| Requires | Target must be enthralled, Domination 2 | |

**Commands**: `puppet <thrall> <command>` - thrall executes the command (attack, flee, get, drop, etc.)

### Hivemind - Coordinated Thralls

Links all enthralled NPCs into a mental network, improving their combat effectiveness.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `mindflayer.hivemind.mana_cost` |
| Focus Cost | 40 | `mindflayer.hivemind.focus_cost` |
| Duration | 10 ticks | `mindflayer.hivemind.duration` |
| Requires | At least 2 thralls, Domination 3 | |

**Effects**:
- All thralls gain +20% damage
- Thralls coordinate attacks (if one attacks, all attack same target)
- Thralls share damage taken (distributed across all)

### Mass Domination - AoE Charm

Attempt to enthral all NPCs in the room simultaneously.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 200 | `mindflayer.massdomination.mana_cost` |
| Focus Required | 100 | `mindflayer.massdomination.focus_req` |
| Focus Cost | 80 | `mindflayer.massdomination.focus_cost` |
| Cooldown | 30 pulses | `mindflayer.massdomination.cooldown` |
| Success Rate | 40% per NPC | `mindflayer.massdomination.success_rate` |
| Requires | Domination 4 | |

**Mechanic**: Each NPC rolls separately. Those who succeed are enthralled. Max thralls limit still applies.

### Mind Feed - Mental Consumption

Drains psychic energy from a target, healing the Mindflayer and building Focus.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `mindflayer.mindfeed.mana_cost` |
| Focus Gain | +15 | `mindflayer.mindfeed.focus_gain` |
| Cooldown | 6 pulses | `mindflayer.mindfeed.cooldown` |
| Damage | 150-300 + (focus × 2) | |
| Heal Amount | 50% of damage dealt | `mindflayer.mindfeed.heal_pct` |
| Damage Type | Mental | |
| Requires | Fighting, Consumption 1 | |

### Memory Drain - Buff Strip

Tears through the target's memories, removing active buffs and temporarily sealing abilities.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `mindflayer.memorydrain.mana_cost` |
| Focus Required | 50 | `mindflayer.memorydrain.focus_req` |
| Focus Cost | 35 | `mindflayer.memorydrain.focus_cost` |
| Cooldown | 12 pulses | `mindflayer.memorydrain.cooldown` |
| Buffs Stripped | Up to 4 | `mindflayer.memorydrain.max_strips` |
| Ability Seal | 3 ticks | `mindflayer.memorydrain.seal_duration` |
| PvP Resist | 50% | `mindflayer.memorydrain.pvp_resist` |
| Requires | Fighting, Consumption 2 | |

**Ability Seal**: Target cannot use class abilities for duration (mana costs doubled, cooldowns extended).

### Intellect Devour - Ultimate Consumption

Consumes a portion of the target's very intellect, dealing massive damage and potentially reducing their maximum HP.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 150 | `mindflayer.intellectdevour.mana_cost` |
| Focus Required | 80 | `mindflayer.intellectdevour.focus_req` |
| Focus Cost | 60 | `mindflayer.intellectdevour.focus_cost` |
| Cooldown | 20 pulses | `mindflayer.intellectdevour.cooldown` |
| Damage | 400-700 + (focus × 5) | |
| Max HP Reduction | 5% (temporary, 10 ticks) | `mindflayer.intellectdevour.maxhp_reduction` |
| Damage Type | Mental | |
| Requires | Fighting, Consumption 3 | |

**Max HP Reduction**: NPCs only. Reduces target's maximum HP by 5% for duration. Does not stack.

### Psychic Maelstrom - AoE Mental Storm

Unleashes a whirlwind of psychic energy hitting all enemies in combat, with a chance to confuse.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `mindflayer.psychicmaelstrom.mana_cost` |
| Focus Cost | 30 | `mindflayer.psychicmaelstrom.focus_cost` |
| Cooldown | 10 pulses | `mindflayer.psychicmaelstrom.cooldown` |
| Damage per Target | 180-320 + (focus × 3) | |
| Confusion Chance | 25% (causes flee) | `mindflayer.psychicmaelstrom.confuse_chance` |
| Damage Type | Mental | |
| Requires | Fighting, Psionic Storm 1 | |

### Psiblast - Cone Stun

Projects a cone of psionic force that stuns and damages all enemies in front of the Mindflayer. Command: `psiblast`

**Note**: Named `psiblast` to avoid conflict with Vampire's `mindblast` command in vamp.c.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 140 | `mindflayer.psiblast.mana_cost` |
| Focus Required | 60 | `mindflayer.psiblast.focus_req` |
| Focus Cost | 45 | `mindflayer.psiblast.focus_cost` |
| Cooldown | 14 pulses | `mindflayer.psiblast.cooldown` |
| Damage per Target | 250-450 + (focus × 4) | |
| Stun Duration | 2 rounds | `mindflayer.psiblast.stun_duration` |
| Damage Type | Mental | |
| Requires | Fighting, Psionic Storm 2 | |

### Reality Fracture - Ultimate Psionic Attack

Tears at the fabric of reality around the Mindflayer's enemies, dealing devastating mental damage and potentially driving them mad.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 200 | `mindflayer.realityfracture.mana_cost` |
| Focus Required | 120 | `mindflayer.realityfracture.focus_req` |
| Focus Cost | 100 | `mindflayer.realityfracture.focus_cost` |
| Cooldown | 25 pulses | `mindflayer.realityfracture.cooldown` |
| Damage per Target | 400-700 + (focus × 5) | |
| Madness Effect | Attack random targets for 3 rounds | |
| Damage Type | Mental | |
| Requires | Fighting, Psionic Storm 3 | |

**Madness**: Affected NPCs attack random targets in the room (including each other) for duration.

## Mindflayer Armor

Create equipment via `mindflayerarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| mind scepter | TBD | Wield |
| ring of thoughts | TBD | Finger |
| neural collar | TBD | Neck |
| flayer robes | TBD | Torso |
| cerebral crown | TBD | Head |
| leggings | TBD | Legs |
| hovering sandals | TBD | Feet |
| tentacle gloves | TBD | Hands |
| psionic vambraces | TBD | Arms |
| mind shroud | TBD | About Body |
| elder sash | TBD | Waist |
| brain bangle | TBD | Wrist |
| third eye lens | TBD | Face |

**Equipment Stats** (proposed):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_MINDFLAYER only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += ch->rage * balance.damcap_mindflayer_focus_mult;   // +8 per focus
if ( hivemind active )
    max_dam += balance.damcap_mindflayer_hivemind;             // +200
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Focus | +8 per point (max +1200) | `damcap_mindflayer_focus_mult` (default: 8) |
| Hivemind | +200 flat | `damcap_mindflayer_hivemind` (default: 200) |

### Thrall Management

- Maximum of 3 thralls at once
- Thralls persist until duration expires, they die, or Mindflayer dismisses them (`dismiss <thrall>`)
- Thralls do not grant XP when they kill
- Thralls share Mindflayer's enemy list

### Tick Update

Per-tick processing:
1. Focus build (+2 fighting, cap 150) or decay (-1 not fighting)
2. Peak focus tracking
3. Thrall duration countdown (each thrall)
4. Hivemind duration countdown
5. Memory seal effect countdown on targets

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_MINDFLAYER bit (131072) |
| focus | ch->rage | Current focus (0-150) |
| hivemind | ch->pcdata->powers[0] | Hivemind ticks remaining |
| thrall_count | ch->pcdata->powers[1] | Number of active thralls |
| domination | ch->pcdata->powers[10] | Domination training level (0-4) |
| consumption | ch->pcdata->powers[11] | Consumption training level (0-3) |
| psionic_storm | ch->pcdata->powers[12] | Psionic Storm training level (0-3) |
| peak_focus | ch->pcdata->stats[0] | Highest focus this session |

**Note**: Thrall tracking uses the existing follower/charm system (AFF_CHARM, ch->master, ch->leader).
