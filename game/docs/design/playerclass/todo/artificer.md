# Artificer Class Design

## Overview

Artificers are masters of technology and invention, wielding gadgets, energy weapons, and mechanical companions. They charge Power Cells through combat and crafting, spending them to deploy turrets, fire energy blasters, and erect force fields. Their technological prowess offers utility and versatility unmatched by magic-based classes. This is a **base class** that upgrades to Mechanist.

**Source Files**: `src/classes/artificer.c`, `src/classes/artificer.h`
**Class Constant**: `CLASS_ARTIFICER` (1048576 / 2^20)
**Upgrades To**: Mechanist

## Color Scheme

Artificer uses a light blue-silver palette evoking technology and precision:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x044` | Light blue | Bracket edges, decorative accents |
| Primary | `#x250` | Chrome silver | Class name, titles, ability highlights |
| Bracket open | `#x044[#x250=` | Blue[Silver= | Who list open bracket |
| Bracket close | `=#x250]#x044` | =Silver]Blue | Who list close bracket |
| Room tag | `#x250(#nArtificer#x250)` | Silver parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | Master Engineer | `#x044[#x250=#x250Master Engineer#n=#x250]#x044` |
| 2 | Inventor | `#x044[#x250=#x250Inventor#n=#x250]#x044` |
| 3 | Artificer | `#x044[#x250=#x250Artificer#n=#x250]#x044` |
| 4 | Tinkerer | `#x044[#x250=#x250Tinkerer#n=#x250]#x044` |
| 5 | Apprentice | `#x044[#x250=#x250Apprentice#n=#x250]#x044` |
| default | Mechanic | `#x044[#x250=#x250Mechanic#n=#x250]#x044` |

## Core Mechanics

### Power Cells

Power Cells are the energy resource that powers Artificer abilities, stored in `ch->rage`:

```c
ch->rage  // Current power cells (0-100 for Artificer, 0-150 for Mechanist)
```

**Building Power Cells**:
- Combat: +2 per game tick while fighting (combat systems generate power)
- Charge command: +15 (out of combat, uses mana)
- Successful gadget use: +3 (systems feedback)
- Maximum: 100

**Power Cell Decay** (out of combat):
- -1 per tick when not fighting (passive drain)
- No decay if within 10 of max (full charge stable)

**Overcharge Mechanic**: Can push beyond 100 to 120 for 20 ticks after using `overcharge`:
- Abilities cost 50% more but deal 30% more damage
- Decay rate increases to -3 per tick
- Risk of backfire (5% chance per ability used while overcharged)

### Power Cell Display

Command: `power`
- Shows current/max power cells
- Shows overcharge status if active
- Lists all active gadgets with remaining durations
- Available to both Artificer and Mechanist

### Charge Command

Out-of-combat power cell recovery:

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `artificer.charge.mana_cost` |
| Power Gain | +15 | `artificer.charge.power_gain` |
| Cooldown | 6 pulses | `artificer.charge.cooldown` |

### Overcharge Command

Temporarily exceed power cell limits:

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `artificer.overcharge.mana_cost` |
| Max Increase | +20 (to 120) | `artificer.overcharge.max_increase` |
| Duration | 20 ticks | `artificer.overcharge.duration` |
| Cooldown | 30 pulses | `artificer.overcharge.cooldown` |

## Training Categories

Artificer abilities are organized into 3 trainable categories. Each category has 3 levels, unlocking abilities progressively. Stored in `ch->pcdata->powers[]`:

| Index | Category | Theme | Abilities |
|-------|----------|-------|-----------|
| ART_TRAIN_GADGET (10) | Gadgetry | Utility devices | Turret, Decoy, Grapple Hook |
| ART_TRAIN_WEAPON (11) | Weaponsmithing | Combat gear | Energy Blade, Blaster, Grenade |
| ART_TRAIN_DEFENSE (12) | Defensive Tech | Protection | Force Field, Repair Bot, Cloak |

**Training Cost**: `(current_level + 1) * 40` primal per level

**Training Command**: `techtrain` - Display current levels and improve categories

### Gadgetry (Utility)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `turret` | Deploy an automated turret |
| 2 | `decoy` | Create a holographic decoy |
| 3 | `grapple` | Grappling hook for mobility and combat |

### Weaponsmithing (Offense)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `energyblade` | Summon an energy melee weapon |
| 2 | `blaster` | Ranged energy attack |
| 3 | `grenade` | Explosive AoE damage |

### Defensive Tech (Protection)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `forcefield` | Energy shield absorbs damage |
| 2 | `repairbot` | Deploy a bot that heals over time |
| 3 | `cloak` | Technological invisibility |

## Abilities

### Turret - Automated Defense

Deploys an energy turret that attacks enemies automatically.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `artificer.turret.mana_cost` |
| Power Cost | 20 | `artificer.turret.power_cost` |
| Duration | 12 ticks | `artificer.turret.duration` |
| Max Turrets | 2 | `artificer.turret.max_turrets` |
| Requires | Gadgetry 1 | |

**Turret Stats**:
- HP: 500 (destroyed if reduced to 0)
- Damage: 50-80 per round
- Targets: Attacks Artificer's fighting target
- Stationary: Cannot move, despawns if Artificer leaves room

### Decoy - Holographic Distraction

Creates a holographic copy that draws enemy attention.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `artificer.decoy.mana_cost` |
| Power Cost | 15 | `artificer.decoy.power_cost` |
| Duration | 6 ticks | `artificer.decoy.duration` |
| Requires | Gadgetry 2 | |

**Effects**:
- Enemies have 40% chance to attack decoy instead of Artificer
- Decoy has 200 HP, destroyed when depleted
- Only one decoy active at a time
- Decoy does not deal damage

### Grapple Hook - Mobility Tool

Fires a grappling hook for movement and combat utility.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 40 | `artificer.grapple.mana_cost` |
| Power Cost | 10 | `artificer.grapple.power_cost` |
| Cooldown | 6 pulses | `artificer.grapple.cooldown` |
| Requires | Gadgetry 3 | |

**Combat Mode** (`grapple <target>`):
- Pull target to you, preventing their flee for 2 ticks
- Deals 50-100 damage
- Disarm chance: 20%

**Movement Mode** (`grapple <direction>`):
- Rapid movement in that direction
- Bypasses some movement restrictions
- Cannot be used while fighting

### Energy Blade - Melee Weapon

Summons an energy blade for enhanced melee combat.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 50 | `artificer.energyblade.mana_cost` |
| Power Cost | 15 | `artificer.energyblade.power_cost` |
| Duration | 10 ticks | `artificer.energyblade.duration` |
| Requires | Weaponsmithing 1 | |

**Effects**:
- Replaces current weapon temporarily
- +30 hitroll, +30 damroll
- Deals energy damage (bypasses some armor)
- Blade disappears when duration ends

### Blaster - Ranged Energy Attack

Fires a concentrated energy blast at the target.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 45 | `artificer.blaster.mana_cost` |
| Power Cost | 12 | `artificer.blaster.power_cost` |
| Power Gain | +3 (feedback) | `artificer.blaster.power_gain` |
| Cooldown | 4 pulses | `artificer.blaster.cooldown` |
| Base Damage | 100-180 + (power × 2) | |
| Requires | Fighting, Weaponsmithing 2 | |

**Overcharge**: If overcharged, deals 30% more damage but costs 18 power.

### Grenade - Explosive AoE

Throws an explosive device that damages all enemies in the room.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `artificer.grenade.mana_cost` |
| Power Required | 30 | `artificer.grenade.power_req` |
| Power Cost | 25 | `artificer.grenade.power_cost` |
| Cooldown | 10 pulses | `artificer.grenade.cooldown` |
| Damage per Target | 150-250 + (power × 2) | |
| Requires | Fighting, Weaponsmithing 3 | |

**Friendly Fire**: Does not damage allies or the Artificer's turrets/bots.

### Force Field - Energy Shield

Projects an energy barrier that absorbs incoming damage.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 60 | `artificer.forcefield.mana_cost` |
| Power Cost | 20 | `artificer.forcefield.power_cost` |
| Duration | 10 ticks | `artificer.forcefield.duration` |
| Absorb Amount | 800 HP | `artificer.forcefield.absorb_amount` |
| Requires | Defensive Tech 1 | |

**Mechanic**: Absorbs all damage types until depleted. Shows remaining shield HP.

### Repair Bot - Healing Drone

Deploys a small drone that heals the Artificer over time.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `artificer.repairbot.mana_cost` |
| Power Cost | 25 | `artificer.repairbot.power_cost` |
| Duration | 15 ticks | `artificer.repairbot.duration` |
| Heal per Tick | 60 HP | `artificer.repairbot.heal_per_tick` |
| Requires | Defensive Tech 2 | |

**Mechanic**: Bot follows Artificer. Destroyed if Artificer takes more than 500 damage in one hit (EMP effect).

### Cloak - Technological Invisibility

Activates a cloaking device for stealth.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `artificer.cloak.mana_cost` |
| Power Cost | 30 | `artificer.cloak.power_cost` |
| Duration | 8 ticks | `artificer.cloak.duration` |
| Power Drain | 5 per tick | `artificer.cloak.power_drain` |
| Requires | Not fighting, Defensive Tech 3 | |

**Effects**:
- Grants AFF_INVISIBLE and AFF_SNEAK
- Broken by attacking or being hit
- Drains power each tick while active
- Auto-deactivates if power reaches 0

## Artificer Armor

Create equipment via `artificerarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| power wrench | 33440 | Wield |
| circuit ring | 33441 | Finger |
| tech collar | 33442 | Neck |
| engineer vest | 33443 | Torso |
| goggles | 33444 | Head |
| utility pants | 33445 | Legs |
| mag boots | 33446 | Feet |
| work gloves | 33447 | Hands |
| tool bracers | 33448 | Arms |
| equipment harness | 33449 | About Body |
| tool belt | 33450 | Waist |
| wrist computer | 33451 | Wrist |
| welding mask | 33452 | Face |

**Equipment Stats** (proposed):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_ARTIFICER and CLASS_MECHANIST only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += balance.damcap_artificer_base;                    // +150 flat
max_dam += ch->rage * balance.damcap_artificer_power_mult;   // +5 per power
if ( energy_blade active )
    max_dam += balance.damcap_artificer_blade;                // +150
if ( overcharged )
    max_dam += balance.damcap_artificer_overcharge;           // +100
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Base | +150 flat | `damcap_artificer_base` (default: 150) |
| Power | +5 per point (max +500) | `damcap_artificer_power_mult` (default: 5) |
| Energy Blade | +150 flat | `damcap_artificer_blade` (default: 150) |
| Overcharge | +100 flat | `damcap_artificer_overcharge` (default: 100) |

### Turret/Bot Management

- Turrets and repair bots are room-bound entities
- Maximum 2 turrets + 1 repair bot active
- All despawn if Artificer leaves the room
- Turrets can be destroyed by enemies (targeted or AoE)

### Overcharge Backfire

When using abilities while overcharged:
- 5% chance of backfire
- Backfire deals 100 damage to Artificer
- Destroys the ability's effect (no damage dealt)
- Ends overcharge state immediately

### Tick Update

Per-tick processing:
1. Power cell build (+2 fighting) or decay (-1 not fighting, -3 if overcharged)
2. Overcharge duration countdown
3. Turret attack processing
4. Repair bot healing
5. Force field duration countdown
6. Energy blade duration countdown
7. Cloak power drain and duration
8. Decoy duration countdown

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_ARTIFICER bit (1048576) |
| power_cells | ch->rage | Current power cells (0-100, 0-120 overcharged) |
| turret_count | ch->pcdata->powers[0] | Number of active turrets |
| forcefield | ch->pcdata->powers[1] | Force field ticks remaining |
| energy_blade | ch->pcdata->powers[2] | Energy blade ticks remaining |
| repair_bot | ch->pcdata->powers[3] | Repair bot ticks remaining |
| cloak | ch->pcdata->powers[4] | Cloak ticks remaining |
| decoy | ch->pcdata->powers[5] | Decoy ticks remaining |
| overcharge | ch->pcdata->powers[6] | Overcharge ticks remaining |
| gadgetry | ch->pcdata->powers[10] | Gadgetry training level (0-3) |
| weaponsmithing | ch->pcdata->powers[11] | Weaponsmithing training level (0-3) |
| defensive_tech | ch->pcdata->powers[12] | Defensive Tech training level (0-3) |
| forcefield_hp | ch->pcdata->stats[0] | Current force field HP |
| decoy_hp | ch->pcdata->stats[1] | Current decoy HP |
| peak_power | ch->pcdata->stats[2] | Highest power this session |
