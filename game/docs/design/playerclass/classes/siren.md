# Siren Class Design

## Overview

Sirens are beings of supernatural vocal power who have transcended their Dirgesinger origins. They wield domination, mass charm, devastating sonic attacks, and soul-rending abilities. Their voice can shatter minds, compel obedience, and unmake magical protections. This is an **upgrade class** obtained by upgrading from Dirgesinger.

**Source Files**: `src/classes/siren.c`, `src/classes/dirgesinger.h`
**Class Constant**: `CLASS_SIREN` (32768)
**Upgrades From**: Dirgesinger

## Color Scheme

Siren uses a deep ocean-violet palette evoking otherworldly enchantment and hypnotic power:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x039` | Deep teal/cyan | Bracket tildes, decorative accents |
| Primary | `#x147` | Soft lavender-blue | Class name, titles, ability highlights |
| Bracket open | `#x039~#x147(` | Teal~Lavender( | Who list open bracket |
| Bracket close | `#x147)#x039~` | Lavender)Teal~ | Who list close bracket |
| Room tag | `#x147(#nSiren#x147)` | Lavender parens | Room display prefix |

**Who List Titles** (`act_info.c:3037-3047`):

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Archsiren | `#x039~#x147(#x147Archsiren#n#x147)#x039~` |
| 2 | Diva of Doom | `#x039~#x147(#x147Diva of Doom#n#x147)#x039~` |
| 3 | Songweaver | `#x039~#x147(#x147Songweaver#n#x147)#x039~` |
| 4 | Voice of Ruin | `#x039~#x147(#x147Voice of Ruin#n#x147)#x039~` |
| default | Siren | `#x039~#x147(#x147Siren#n#x147)#x039~` |

## Core Mechanics

### Resonance (Enhanced)

Siren inherits the Resonance system from Dirgesinger with a higher cap and faster build rate:

```c
ch->rage  // Current resonance (0-150 for Siren)
```

**Building Resonance** (`siren.c:578-586`):
- Combat: +3 per game tick while fighting (vs +2 for Dirgesinger)
- Maximum: 150 (vs 100 for Dirgesinger)

**Resonance Decay** (out of combat):
- -2 per tick when not fighting (slower decay than Dirgesinger's -3)

### Shared Dirgesinger Abilities

Siren retains access to the `resonance` status display command. All other Dirgesinger abilities are replaced by Siren-specific powers (Siren is a separate class, not an extension).

### Upgrade Path

Dirgesinger upgrades to Siren via `do_upgrade` (`upgrade.c:200-202`):
- Requires standard upgrade costs (quest points, HP, mana)
- Class changes from `CLASS_DIRGESINGER` to `CLASS_SIREN`
- All abilities reset; Siren abilities become available

## Training Categories

Siren abilities are organized into 3 trainable categories. Stored in `ch->pcdata->powers[]`:

| Index | Category | Max Level | Theme | Abilities |
|-------|----------|-----------|-------|-----------|
| SIREN_TRAIN_DEVASTATION (10) | Devastation | 3 | Raw sonic destruction | Banshee Wail, Soulrend, Crescendo |
| SIREN_TRAIN_DOMINATION (11) | Domination | 4 | Mind control and compulsion | Enthrall, Siren Song, Command Voice, Mesmerize |
| SIREN_TRAIN_UNMAKING (12) | Unmaking | 3 | Defense stripping and disruption | Echoshield, Cacophony, Aria of Unmaking |

**Training Cost**: `(current_level + 1) * 50` primal per level

**Training Command**: `voicetrain` - Display current levels and improve categories

### Devastation (Raw Damage)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `bansheewail` | Devastating AoE, instakills weak mobs |
| 2 | `soulrend` | Spirit damage bypassing physical resistance |
| 3 | `crescendo` | Multi-stage building finisher |

### Domination (Mind Control)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `enthrall` | Charm an NPC to fight for you |
| 2 | `sirensong` | Mass pacification of room |
| 3 | `commandvoice` | Force target to flee or drop weapon |
| 4 | `mesmerize` | Stun/paralyze through hypnotic melody |

### Unmaking (Defense/Disruption)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `echoshield` | Reflects portion of damage as sonic |
| 2 | `cacophony` | AoE madness/confusion + damage |
| 3 | `ariaofunmaking` | Strip buffs and protections |

## Abilities

### Banshee Wail - Devastating AoE (`siren.c:21-71`)

Massive area attack that can instantly kill weak NPCs. The signature Siren offensive ability.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `siren.bansheewail.mana_cost` |
| Resonance Required | 80 | `siren.bansheewail.resonance_req` |
| Resonance Cost | 60 | `siren.bansheewail.resonance_cost` |
| Cooldown | 15 pulses | `siren.bansheewail.cooldown` |
| Damage per Target | 300-600 + (resonance × 4) | |
| Instakill Threshold | NPCs with < 500 max HP | `siren.bansheewail.instakill_threshold` |
| Requires | Fighting | |

### Soulrend - Spirit Damage (`siren.c:76-115`)

Bypasses all physical resistance and armor, dealing direct HP damage. Can kill targets outright.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `siren.soulrend.mana_cost` |
| Resonance Cost | 20 | `siren.soulrend.resonance_cost` |
| Cooldown | 8 pulses | `siren.soulrend.cooldown` |
| Damage | 250-500 + (resonance × 3) | |
| Special | Bypasses armor (direct `victim->hit` reduction) | |
| Requires | Fighting | |

**Warning**: Soulrend uses `raw_kill()` if target drops below -10 HP, bypassing normal death processing.

### Crescendo - Building Finisher (`siren.c:120-176`)

Multi-round attack that builds through 3 stages before unleashing a devastating finale that consumes all resonance.

| Stage | Name | Damage | Effect |
|-------|------|--------|--------|
| 1 | Piano | 0 | Building phase |
| 2 | Mezzo Forte | 50-100 | Moderate hit |
| 3 | Forte | 100-200 | Strong hit |
| Finale | **Fortissimo** | (400-800) × 3 + (resonance × 5) | All resonance consumed |

| Property | Value | Config Key |
|----------|-------|------------|
| Mana per Stage | 25 | `siren.crescendo.mana_per_stage` |
| Finale Multiplier | 3× | `siren.crescendo.finale_dam_mult` |
| Requires | Fighting | |

**Timeout**: Crescendo resets if the Siren leaves combat (`siren.c:600-603`).

### Cacophony - AoE Madness (`siren.c:181-227`)

Area damage with chance to cause confusion (flee) on NPCs.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `siren.cacophony.mana_cost` |
| Resonance Cost | 40 | `siren.cacophony.resonance_cost` |
| Cooldown | 12 pulses | `siren.cacophony.cooldown` |
| Damage per Target | 150-350 + (resonance × 3) | |
| NPC Flee Chance | 20% | |
| Requires | Fighting | |

### Enthrall - Charm NPC (`siren.c:232-289`)

Charm an NPC to fight for you. Sets AFF_CHARM, master, and leader.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `siren.enthrall.mana_cost` |
| Resonance Cost | 50 | `siren.enthrall.resonance_cost` |
| Max Followers | 2 | `siren.enthrall.max_followers` |
| Duration | 30 ticks | `siren.enthrall.duration` |
| Restriction | NPCs only | |

### Siren Song - Mass Pacification (`siren.c:294-329`)

Stops all NPCs in the room from fighting. Does not charm, only pacifies.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `siren.sirensong.mana_cost` |
| Resonance Cost | 80 | `siren.sirensong.resonance_cost` |
| Cooldown | 20 pulses | `siren.sirensong.cooldown` |
| Effect | All NPCs stop fighting | |

### Command Voice - Forced Action (`siren.c:334-392`)

Compel a target to perform a specific action (flee or drop weapon).

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `siren.commandvoice.mana_cost` |
| Resonance Cost | 30 | `siren.commandvoice.resonance_cost` |
| Cooldown | 10 pulses | `siren.commandvoice.cooldown` |

| Subcommand | Effect |
|------------|--------|
| `commandvoice <target> flee` | Forces target to flee |
| `commandvoice <target> drop` | Forces target to drop wielded weapon |

### Mesmerize - Stun/Paralyze (`siren.c:397-427`)

Hypnotic melody that stuns the target for multiple combat rounds.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `siren.mesmerize.mana_cost` |
| Resonance Cost | 35 | `siren.mesmerize.resonance_cost` |
| Cooldown | 12 pulses | `siren.mesmerize.cooldown` |
| Stun Duration | 3 rounds | `siren.mesmerize.stun_duration` |
| Requires | Fighting | |

### Echoshield - Damage Reflection (`siren.c:432-451`)

Surrounds the Siren with a sonic shield that reflects a portion of incoming damage back at attackers.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `siren.echoshield.mana_cost` |
| Duration | 10 ticks | `siren.echoshield.duration` |
| Reflect % | 20% of damage | `siren.echoshield.reflect_pct` |
| Damcap Bonus | +150 | `balance.damcap_siren_echoshield` |

**Reflection Mechanic** (`fight.c:1900-1907`):
```c
int reflect = dam * acfg("siren.echoshield.reflect_pct") / 100;
hurt_person( victim, ch, reflect );
```

### Aria of Unmaking - Buff Strip (`siren.c:456-513`)

Strips magical protections and affects from the target.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `siren.ariaofunmaking.mana_cost` |
| Resonance Cost | 60 | `siren.ariaofunmaking.resonance_cost` |
| Cooldown | 15 pulses | `siren.ariaofunmaking.cooldown` |
| Max Affects Stripped | 3 | |
| Also Removes | AFF_SANCTUARY, AFF_PROTECT | |
| Requires | Fighting | |

## Siren Armor

Create equipment via `sirenarmor <piece>` (`siren.c:518-573`):

**Cost**: 60 primal per piece (`acfg: dirgesinger.armor.primal_cost`)

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| scepter | 33340 | Wield |
| ring | 33341 | Finger |
| choker | 33342 | Neck |
| gown | 33343 | Torso |
| diadem | 33344 | Head |
| greaves | 33345 | Legs |
| slippers | 33346 | Feet |
| gloves | 33347 | Hands |
| armlets | 33348 | Arms |
| mantle | 33349 | About Body |
| sash | 33350 | Waist |
| bangle | 33351 | Wrist |
| veil | 33352 | Face |

**Equipment Stats** (from `classeq.db`):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions** (`handler.c`): Vnum range 33340-33359 restricted to CLASS_SIREN only.

## Combat Mechanics

### Damage Cap Bonuses (`fight.c:1709-1713`)

```c
max_dam += ch->rage * balance.damcap_siren_res_mult;        // +7 per resonance
if ( echoshield active )
    max_dam += balance.damcap_siren_echoshield;               // +150
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Resonance | +7 per point (max +1050) | `damcap_siren_res_mult` (default: 7) |
| Echoshield | +150 flat | `damcap_siren_echoshield` (default: 150) |

### Extra Attacks (`fight.c:1004-1006`)

Siren inherits the cadence extra attack mechanic (if cadence is active from Dirgesinger carry-over, though Siren has its own tick update).

### Ironsong Barrier (`fight.c:1886-1898`)

Shared with Dirgesinger. Absorbs incoming damage before it reaches HP.

### Echoshield Reflection (`fight.c:1900-1907`)

Reflects 20% of incoming damage back at attackers as sonic energy.

### Tick Update (`siren.c:578-604`)

Called from `update.c:509`. Per-tick processing:
1. Resonance build (+3 fighting, cap 150) or decay (-2 not fighting)
2. Peak resonance tracking
3. Echoshield duration countdown
4. Crescendo timeout (resets if not fighting)

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_SIREN bit (32768) |
| resonance | ch->rage | Current resonance (0-150) |
| echoshield | ch->pcdata->powers[6] | Echoshield ticks remaining |
| crescendo | ch->pcdata->powers[7] | Crescendo build stage (0-3) |
| devastation | ch->pcdata->powers[10] | Devastation training level (0-3) |
| domination | ch->pcdata->powers[11] | Domination training level (0-4) |
| unmaking | ch->pcdata->powers[12] | Unmaking training level (0-3) |
| barrier_hp | ch->pcdata->stats[0] | Current sonic barrier HP (shared) |
| peak_res | ch->pcdata->stats[1] | Highest resonance this session |
