# Mechanist Class Design

## Overview

Mechanists are cyborg warriors who have integrated technology directly into their bodies. They command drone swarms, fire devastating heavy weapons, and augment themselves with cybernetic implants. Having transcended the limitations of their Artificer origins, they are walking weapons platforms capable of overwhelming force. This is an **upgrade class** obtained by upgrading from Artificer.

**Source Files**: `src/classes/mechanist.c`, `src/classes/artificer.h` (TBD)
**Class Constant**: `CLASS_MECHANIST` (TBD - next available power-of-2)
**Upgrades From**: Artificer

## Color Scheme

Mechanist uses a neon orange-chrome palette evoking advanced technology and danger:

| Element | Code | Color | Usage |
|---------|------|-------|-------|
| Accent | `#x208` | Neon orange | Bracket tildes, decorative accents |
| Primary | `#x253` | Bright chrome | Class name, titles, ability highlights |
| Bracket open | `#x208~#x253(` | Orange~Chrome( | Who list open bracket |
| Bracket close | `#x253)#x208~` | Chrome)Orange~ | Who list close bracket |
| Room tag | `#x253(#nMechanist#x253)` | Chrome parens | Room display prefix |

**Who List Titles**:

| Generation | Title | Display |
|------------|-------|---------|
| 1 | War Machine | `#x208~#x253(#x253War Machine#n#x253)#x208~` |
| 2 | Cyborg Lord | `#x208~#x253(#x253Cyborg Lord#n#x253)#x208~` |
| 3 | Mechanist | `#x208~#x253(#x253Mechanist#n#x253)#x208~` |
| 4 | Augmented | `#x208~#x253(#x253Augmented#n#x253)#x208~` |
| default | Cyborg | `#x208~#x253(#x253Cyborg#n#x253)#x208~` |

## Core Mechanics

### Power Cells (Enhanced)

Mechanist inherits the Power Cell system from Artificer with a higher cap and integrated generation:

```c
ch->rage  // Current power cells (0-150 for Mechanist)
```

**Building Power Cells**:
- Combat: +3 per game tick while fighting (vs +2 for Artificer)
- Internal reactor: +1 per tick out of combat (no decay!)
- Kills: +10 per enemy killed (cybernetic energy harvesting)
- Maximum: 150 (vs 100 for Artificer)

**Power Cell Decay**: None! Mechanist's internal reactor maintains charge. However, some abilities have high sustained drain.

### Implant System

Mechanist has 3 cybernetic implant slots that provide passive bonuses:

| Slot | Location | Theme |
|------|----------|-------|
| Neural | Head | Combat processing, reaction time |
| Servo | Arms | Strength, damage output |
| Core | Torso | Defense, regeneration |

Implants are selected via the `implant` command and can be changed out of combat. Each slot has 3 implant options (unlocked via training).

### Shared Artificer Abilities

Mechanist retains access to the `power` command. All other Artificer abilities are replaced by Mechanist-specific powers, though some are enhanced versions.

### Upgrade Path

Artificer upgrades to Mechanist via `do_upgrade`:
- Requires standard upgrade costs (50K hp, 35K mana/move, 40K qp, gen 1)
- Class changes from `CLASS_ARTIFICER` to `CLASS_MECHANIST`
- All abilities reset; Mechanist abilities become available
- Default implants installed

## Training Categories

Mechanist abilities are organized into 3 trainable categories. Stored in `ch->pcdata->powers[]`:

| Index | Category | Max Level | Theme | Abilities |
|-------|----------|-----------|-------|-----------|
| MECH_TRAIN_CYBER (10) | Cybernetics | 3 | Self-augmentation | Neural Jack, Servo Arms, Reactive Plating |
| MECH_TRAIN_DRONE (11) | Drone Swarm | 4 | Minion control | Combat Drone, Repair Swarm, Bomber Drone, Drone Army |
| MECH_TRAIN_ORDNANCE (12) | Heavy Ordnance | 3 | Devastating weapons | Railgun, EMP Burst, Orbital Strike |

**Training Cost**: `(current_level + 1) * 50` primal per level

**Training Command**: `cybtrain` - Display current levels and improve categories

### Cybernetics (Self-Augmentation)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `neuraljack` | Neural implant options + reflex boost |
| 2 | `servoarms` | Arm implant options + strength boost |
| 3 | `reactiveplating` | Core implant options + armor boost |

### Drone Swarm (Minion Control)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `combatdrone` | Deploy attack drone |
| 2 | `repairswarm` | Deploy healing nanobots |
| 3 | `bomberdrone` | Deploy explosive drone |
| 4 | `dronearmy` | Mass drone deployment |

### Heavy Ordnance (Devastating Weapons)

| Level | Ability | Unlock |
|-------|---------|--------|
| 1 | `railgun` | High-damage single target |
| 2 | `empburst` | Disable enemy tech + damage |
| 3 | `orbitalstrike` | Ultimate AoE devastation |

## Implant System Details

### Neural Implants (Cybernetics 1 required)

| Implant | Effect | Passive Bonus |
|---------|--------|---------------|
| Combat Processor | +15% dodge | Reduced cooldowns by 1 pulse |
| Targeting Suite | +20 hitroll | Blaster/Railgun damage +20% |
| Threat Analyzer | See enemy HP/buffs | Warning before big attacks |

### Servo Implants (Cybernetics 2 required)

| Implant | Effect | Passive Bonus |
|---------|--------|---------------|
| Power Arms | +30 damroll | Melee attacks deal +50 damage |
| Multi-Tool | Extra attack +1 | Can use grapple while fighting |
| Shield Generator | +200 damcap | Force field recharges 50/tick |

### Core Implants (Cybernetics 3 required)

| Implant | Effect | Passive Bonus |
|---------|--------|---------------|
| Armored Chassis | -50 AC | 10% damage resistance |
| Regenerator | +100 HP/tick regen | Repair bot heals double |
| Power Core | +25 max power cells | +1 power/tick generation |

## Abilities

### Neural Jack - Neural Augmentation

Activates neural enhancement systems for combat advantage.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `mechanist.neuraljack.mana_cost` |
| Power Cost | 30 | `mechanist.neuraljack.power_cost` |
| Duration | 12 ticks | `mechanist.neuraljack.duration` |
| Requires | Cybernetics 1 | |

**Effects**:
- +20% dodge chance
- Cooldown reduction: All abilities -1 pulse cooldown
- Enhanced reaction messages in combat

### Servo Arms - Strength Augmentation

Activates servo-assisted arm systems for enhanced damage.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `mechanist.servoarms.mana_cost` |
| Power Cost | 30 | `mechanist.servoarms.power_cost` |
| Duration | 12 ticks | `mechanist.servoarms.duration` |
| Requires | Cybernetics 2 | |

**Effects**:
- +50 damage per melee hit
- +1 extra attack per round
- Can lift/throw heavy objects

### Reactive Plating - Armor Augmentation

Activates reactive armor plating for defense.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 80 | `mechanist.reactiveplating.mana_cost` |
| Power Cost | 30 | `mechanist.reactiveplating.power_cost` |
| Duration | 12 ticks | `mechanist.reactiveplating.duration` |
| Requires | Cybernetics 3 | |

**Effects**:
- 15% damage resistance (all types)
- -75 AC
- Reflects 10% of melee damage back to attacker

### Combat Drone - Attack Unit

Deploys an armed combat drone to assist in battle.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 70 | `mechanist.combatdrone.mana_cost` |
| Power Cost | 25 | `mechanist.combatdrone.power_cost` |
| Duration | 15 ticks | `mechanist.combatdrone.duration` |
| Max Drones | 4 | `mechanist.combatdrone.max_drones` |
| Requires | Drone Swarm 1 | |

**Drone Stats**:
- HP: 400
- Damage: 60-100 per round
- Follows Mechanist between rooms
- Attacks Mechanist's fighting target

### Repair Swarm - Healing Nanobots

Releases a swarm of repair nanobots that heal over time.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 90 | `mechanist.repairswarm.mana_cost` |
| Power Cost | 35 | `mechanist.repairswarm.power_cost` |
| Duration | 10 ticks | `mechanist.repairswarm.duration` |
| Heal per Tick | 100 HP | `mechanist.repairswarm.heal_per_tick` |
| Requires | Drone Swarm 2 | |

**Effects**:
- Also repairs drones: +50 HP/tick to all active drones
- Removes bleeding and poison effects
- Only one swarm active at a time

### Bomber Drone - Explosive Unit

Deploys a drone loaded with explosives that detonates on command.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `mechanist.bomberdrone.mana_cost` |
| Power Cost | 40 | `mechanist.bomberdrone.power_cost` |
| Duration | 8 ticks (or until detonated) | |
| Requires | Drone Swarm 3 | |

**Deployment**: `bomberdrone` - deploys the drone
**Detonation**: `detonate` - explodes the drone

| Property | Value | Config Key |
|----------|-------|------------|
| Explosion Damage | 300-500 + (power × 3) | |
| AoE | All enemies in room | |
| Drone Destroyed | Yes | |

### Drone Army - Mass Deployment

Deploys maximum drones instantly and empowers all active drones.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 150 | `mechanist.dronearmy.mana_cost` |
| Power Required | 100 | `mechanist.dronearmy.power_req` |
| Power Cost | 80 | `mechanist.dronearmy.power_cost` |
| Cooldown | 30 pulses | `mechanist.dronearmy.cooldown` |
| Duration | 10 ticks | `mechanist.dronearmy.duration` |
| Requires | Drone Swarm 4 | |

**Effects**:
- Instantly spawns drones to max (4)
- All drones gain +50% damage for duration
- All drones gain +200 HP for duration
- Repair swarm effect on all drones

### Railgun - Heavy Single Target

Fires a magnetically accelerated projectile with devastating force.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 100 | `mechanist.railgun.mana_cost` |
| Power Required | 50 | `mechanist.railgun.power_req` |
| Power Cost | 40 | `mechanist.railgun.power_cost` |
| Cooldown | 8 pulses | `mechanist.railgun.cooldown` |
| Base Damage | 400-600 + (power × 4) | |
| Armor Pierce | 50% (ignores half of target's AC) | |
| Requires | Fighting, Heavy Ordnance 1 | |

### EMP Burst - Technological Disruption

Releases an electromagnetic pulse that damages and disables.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 120 | `mechanist.empburst.mana_cost` |
| Power Cost | 50 | `mechanist.empburst.power_cost` |
| Cooldown | 15 pulses | `mechanist.empburst.cooldown` |
| Damage | 200-350 + (power × 2) | |
| AoE | All enemies in room | |
| Requires | Fighting, Heavy Ordnance 2 | |

**Disruption Effects**:
- Removes enemy force fields and tech shields
- Stuns constructs/golems for 3 rounds
- Disables enemy turrets/drones temporarily (5 ticks)
- Against tech-based enemies: +50% damage

### Orbital Strike - Ultimate Weapon

Calls down a devastating strike from an orbital weapons platform.

| Property | Value | Config Key |
|----------|-------|------------|
| Mana Cost | 200 | `mechanist.orbitalstrike.mana_cost` |
| Power Required | 120 | `mechanist.orbitalstrike.power_req` |
| Power Cost | 100 | `mechanist.orbitalstrike.power_cost` |
| Cooldown | 45 pulses | `mechanist.orbitalstrike.cooldown` |
| Charge Time | 2 rounds | `mechanist.orbitalstrike.charge_time` |
| Requires | Fighting, Heavy Ordnance 3 | |

**Mechanic**: Takes 2 rounds to charge (announced to room). On round 3:
- Primary target: 600-1000 + (power × 6) damage
- All other enemies: 50% of primary damage
- Room description temporarily changes to show devastation
- Cannot be used indoors (requires sky access)

## Mechanist Armor

Create equipment via `mechanistarmor <piece>`:

**Cost**: 60 primal per piece

| Piece | Vnum | Wear Slot |
|-------|------|-----------|
| plasma cutter | TBD | Wield |
| data ring | TBD | Finger |
| neural collar | TBD | Neck |
| combat chassis | TBD | Torso |
| targeting helm | TBD | Head |
| servo leggings | TBD | Legs |
| thruster boots | TBD | Feet |
| power gauntlets | TBD | Hands |
| weapon mounts | TBD | Arms |
| drone harness | TBD | About Body |
| ammo belt | TBD | Waist |
| holo-display | TBD | Wrist |
| tactical visor | TBD | Face |

**Equipment Stats** (proposed):
- Weapons: +35 hitroll, +35 damroll
- Armor: +30 hitroll, +30 damroll, -35 AC

**Restrictions**: Equipment restricted to CLASS_MECHANIST only.

## Combat Mechanics

### Damage Cap Bonuses

```c
max_dam += ch->rage * balance.damcap_mechanist_power_mult;   // +7 per power
if ( servo_arms active )
    max_dam += balance.damcap_mechanist_servo;                // +200
if ( drone_army active )
    max_dam += balance.damcap_mechanist_army;                 // +150
// Per active drone bonus
max_dam += drone_count * balance.damcap_mechanist_per_drone; // +30 per drone
```

| Source | Bonus | Balance Key |
|--------|-------|-------------|
| Power | +7 per point (max +1050) | `damcap_mechanist_power_mult` (default: 7) |
| Servo Arms | +200 flat | `damcap_mechanist_servo` (default: 200) |
| Drone Army | +150 flat | `damcap_mechanist_army` (default: 150) |
| Per Drone | +30 per active drone | `damcap_mechanist_per_drone` (default: 30) |

### Drone Management

- Maximum 4 combat drones + 1 bomber drone active
- Drones follow Mechanist between rooms
- Drones can be targeted and destroyed by enemies
- `recall` command: All drones return and despawn (no refund)
- `status` command: Shows all drone HP and status

### Implant Swapping

- `implant <slot> <type>` - Install a specific implant
- `implant list` - Show available implants
- Cannot swap while fighting
- Swapping has 10 pulse cooldown

### Tick Update

Per-tick processing:
1. Power cell generation (+3 fighting, +1 not fighting)
2. Neural Jack/Servo Arms/Reactive Plating duration countdown
3. Drone attack processing (all combat drones)
4. Repair swarm healing (Mechanist + drones)
5. Bomber drone duration countdown
6. Drone Army buff duration countdown
7. Orbital strike charge processing
8. Implant passive effects

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_MECHANIST bit (TBD) |
| power_cells | ch->rage | Current power cells (0-150) |
| neural_jack | ch->pcdata->powers[0] | Neural Jack ticks remaining |
| servo_arms | ch->pcdata->powers[1] | Servo Arms ticks remaining |
| reactive_plating | ch->pcdata->powers[2] | Reactive Plating ticks remaining |
| drone_count | ch->pcdata->powers[3] | Number of active combat drones |
| repair_swarm | ch->pcdata->powers[4] | Repair Swarm ticks remaining |
| bomber_active | ch->pcdata->powers[5] | Bomber drone deployed (0/1) |
| drone_army | ch->pcdata->powers[6] | Drone Army buff ticks remaining |
| orbital_charge | ch->pcdata->powers[7] | Orbital Strike charge rounds (0-2) |
| neural_implant | ch->pcdata->powers[8] | Current neural implant (0-3) |
| servo_implant | ch->pcdata->powers[9] | Current servo implant (0-3) |
| cybernetics | ch->pcdata->powers[10] | Cybernetics training level (0-3) |
| drone_swarm | ch->pcdata->powers[11] | Drone Swarm training level (0-4) |
| heavy_ordnance | ch->pcdata->powers[12] | Heavy Ordnance training level (0-3) |
| core_implant | ch->pcdata->powers[13] | Current core implant (0-3) |
| peak_power | ch->pcdata->stats[0] | Highest power this session |
| drones_deployed | ch->pcdata->stats[1] | Total drones deployed (tracking) |
