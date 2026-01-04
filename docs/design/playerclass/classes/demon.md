# Demon Class Design

## Overview

Demons are infernal beings who gain power through corruption, mutations (warps), and discipline advancement. They can grant permanent gifts to themselves and other demons using demon points.

**Source Files**: `src/classes/demon.c`, `src/classes/demon.h`, `src/core/daemon.c`
**Class Constant**: `CLASS_DEMON` (1)

## Core Mechanics

### Demon Points

The primary resource for advancement (`demon.h`):

```c
ch->pcdata->stats[DEMON_CURRENT]  // stats[8] - Available points to spend
ch->pcdata->stats[DEMON_TOTAL]    // stats[9] - Lifetime points earned
ch->pcdata->stats[DEMON_POWER]    // stats[10] - Armor bonus from gifts
ch->pcdata->stats[DEMON_PPOWER]   // stats[11] - Armor bonus from others' gifts
```

**Uses**:
- Purchase warps (15,000 points each)
- Grant gifts to self/others (costs vary, 25x multiplier for others)
- Create demonic armor (60 primal points per piece)

### Beast/Rage System

Demons share the rage system with werewolves (`clan.c:4138-4260`):

```c
ch->beast  // Beast counter (0-100)
ch->rage   // Rage value (caps at 125)
```

**Rage Command** (Attack 3):
- Increases beast counter: (beast+1)% chance to gain +1 (caps at 100)
- Adds rage: generation to 25 points

**Calm Command** (Attack 4):
- Reduces rage by: 40 - (beast/3) + 1d10 points
- Cannot retract claws/fangs while rage > 0

### Warp System

Warps are permanent mutations obtained via `obtain` command (`demon.c:114-171`):

```c
ch->warp       // Bitfield of active warps
ch->warpcount  // Number of warps (max 18)
```

**Cost**: 15,000 demon points (both CURRENT and TOTAL required)

### Positive Warps

| Warp | Constant | Effect |
|------|----------|--------|
| 1 | WARP_CBODY | Body protected by indestructible crystal shell |
| 2 | WARP_SBODY | Skin hard as steel |
| 3 | WARP_STRONGARMS | Incredibly strong arms |
| 4 | WARP_STRONGLEGS | Incredibly strong legs |
| 5 | WARP_VENOMTONG | Long venomous tongue |
| 6 | WARP_SPIKETAIL | Tail fires deadly spikes in combat |
| 7 | WARP_BADBREATH | Breath is putrid and deadly |
| 8 | WARP_QUICKNESS | Incredible speed |
| 9 | WARP_STAMINA | Increased stamina, damage reduction |
| 10 | WARP_HUNT | Heightened senses enable hunting |
| 11 | WARP_DEVOUR | Ability to devour opponents (sends to Hell pits) |
| 12 | WARP_TERROR | Horrifying appearance that stuns viewers |
| 13 | WARP_REGENERATE | Incredibly fast regeneration |
| 14 | WARP_STEED | Mounts transform into hideous demons |
| 15 | WARP_WEAPON | Power to transform into deadly battle axe |
| 16 | WARP_MAGMA | Body composed of deadly magma |
| 17 | WARP_SHARDS | Skin covered with shards of ice |
| 18 | WARP_WINGS | Pair of leathery wings protrude from back |

### Negative Warps (Random Penalties)

| Warp | Effect |
|------|--------|
| WARP_INFIRMITY | Afflicted by terrible infirmity |
| WARP_GBODY | Fragile glass skin |
| WARP_SCARED | Incredibly scared of combat |
| WARP_WEAK | Muscles severely weakened |
| WARP_SLOW | Body moves very slowly |
| WARP_VULNER | Skin very vulnerable to magic |
| WARP_CLUMSY | Incredibly clumsy (easily disarmed) |
| WARP_STUPID | Intelligence extremely low (cannot cast) |

## Disciplines

Demons have 9 disciplines stored in `ch->power[]`:

| Discipline | Index | Theme |
|------------|-------|-------|
| Hellfire | DISC_DAEM_HELL (30) | Fire attacks |
| Attack | DISC_DAEM_ATTA (31) | Combat abilities |
| Temptation | DISC_DAEM_TEMP (32) | Hellish charm, NPC manipulation |
| Morphing | DISC_DAEM_MORP (33) | Shape alteration |
| Corruption | DISC_DAEM_CORR (34) | Weapon enhancement |
| Gelugon | DISC_DAEM_GELU (35) | Ice demon powers |
| Discord | DISC_DAEM_DISC (36) | Chaos sowing |
| Nether | DISC_DAEM_NETH (37) | Death/life manipulation |
| Immunities | DISC_DAEM_IMMU (38) | Passive damage reduction |

## Discipline Abilities by Level

### Hellfire (DISC_DAEM_HELL)

| Level | Command | Effect |
|-------|---------|--------|
| 2 | `immolate` | Coat weapon in flame (+WEAPON_FLAMING, 1% backfire) (`daemon.c:761`) |
| 3 | `inferno` | Last resort: 1500-2000 damage to room when HP <= 0 (`daemon.c:814`) |
| 7 | `daemonseed` | Plant explosive seeds in items (explode after timer) (`daemon.c:704`) |
| 8 | `hellfire` | Summon fire walls in all 4 directions (`daemon.c:565`) |

### Attack (DISC_DAEM_ATTA)

| Level | Command | Effect |
|-------|---------|--------|
| 1 | `claws` | Toggle demonic claws for combat (`clan.c:1503`) |
| 2 | `fangs` | Toggle sharp fangs for combat (`clan.c:1569`) |
| 3 | `rage` | Increase beast counter and rage value (`clan.c:4138`) |
| 4 | `calm` | Reduce rage value (`clan.c:4197`) |
| 5 | `graft` | Graft additional arms (up to 4 total) (`daemon.c:1235`) |
| 7 | `blink` | Disappear from combat, reappear next round (`daemon.c:1174`) |

### Temptation (DISC_DAEM_TEMP)

| Level | Command | Effect |
|-------|---------|--------|
| 10 | `dominate` | Take control of NPC, bend their will to yours (`clan.c:3912`) |

### Corruption (DISC_DAEM_CORR)

| Level | Command | Effect |
|-------|---------|--------|
| 4 | `caust` | Coat weapon in corrosive venom (+WEAPON_POISON) (`daemon.c:364`) |

### Gelugon (DISC_DAEM_GELU) - Ice Demon

| Level | Command | Effect |
|-------|---------|--------|
| 1 | `freezeweapon` | Coat weapon in ice crystals (+WEAPON_FROST) (`daemon.c:503`) |
| 2 | `frostbreath` | Breathe freezing air at target(s) (`daemon.c:1408`) |
| 6 | `entomb` | Create ice walls blocking all directions, heal 10% HP (`daemon.c:116`) |
| 7 | `gust` | Blow target in direction with damaging wind (`daemon.c:199`) |

### Discord (DISC_DAEM_DISC)

| Level | Command | Effect |
|-------|---------|--------|
| 1 | `unnerve` | Force target into combat stance (`daemon.c:470`) |
| 2 | `evileye` | Configure custom evil eye effects on look (`act_wiz.c:5594`) |
| 4 | `chaosportal` | Teleport to random location (66% success in combat) (`daemon.c:415`) |

### Nether (DISC_DAEM_NETH)

| Level | Command | Effect |
|-------|---------|--------|
| 2 | `deathsense` | Toggle unholy sight (holylight vision) (`daemon.c:35`) |
| 4 | `leech` | Life drain from target, heal self (max 300 + damage/10) (`daemon.c:60`) |

### Immunities (DISC_DAEM_IMMU) - Passive

Provides damage reduction in combat (`fight.c:1700-1702`):

```c
damage *= (100 - (power[DISC_DAEM_IMMU] * 4)) / 100;
```

Each level = 4% damage reduction (e.g., level 10 = 40% reduction)

## Gift System (Inpart)

Demons can grant permanent gifts via `inpart` command (`demon.c:173-335`):

| Gift | Cost | DEM_* Flag |
|------|------|------------|
| Fangs | 2,500 | DEM_FANGS |
| Claws | 2,500 | DEM_CLAWS |
| Horns | 2,500 | DEM_HORNS |
| Hooves | 1,500 | DEM_HOOVES |
| Nightsight | 3,000 | DEM_EYES |
| Wings | 1,000 | DEM_WINGS |
| Might | 7,500 | DEM_MIGHT |
| Toughness | 7,500 | DEM_TOUGH |
| Speed | 7,500 | DEM_SPEED |
| Travel | 1,500 | DEM_TRAVEL |
| Scry | 7,500 | DEM_SCRY |
| Truesight | 7,500 | DEM_TRUESIGHT |
| Shield | 20,000 | DEM_SHIELD |
| Graft | 20,000 | DEM_GRAFT |
| Immolate | 2,500 | DEM_IMMOLATE |
| Inferno | 20,000 | DEM_INFERNO |
| Caust | 3,000 | DEM_CAUST |
| Unnerve | 5,000 | DEM_UNNERVE |
| Freezeweapon | 3,000 | DEM_FREEZEWEAPON |
| Entomb | 20,000 | DEM_ENTOMB |
| Leech | 15,000 | DEM_LEECH |
| Demonform | 25,000 | DEM_FORM |
| Blink | 15,000 | DEM_BLINK |

**Granting to Others**: Cost multiplied by 25x

## Special Abilities

### Weapon/Object Transformation

- `weaponform` - Transform into demonic weapon vnum 33120 (`demon.c:704-753`)
- `humanform` - Return to human form (`demon.c:755-782`)

Requirements: DPOWER_OBJ_VNUM > 0, not polymorphed, not webbed, no fight timer

### Eye Spy

- `eyespy` - Pluck out eyeball to create familiar spy (`demon.c:816-863`)
- Creates MOB_VNUM_EYE creature linked to caster

### Demon Armor

- `demonarmour` - Create demonic armor pieces (`demon.c:337-394`)
- Cost: 60 primal points per piece
- Pieces: Ring, Collar, Plate, Helmet, Leggings, Boots, Gauntlets, Sleeves, Cape, Belt, Bracer, Visor, Longsword, Shortsword

## Regeneration

### Standard Regen (`update.c:1723-1739`)

- In Hell rooms (vnum 93420-93426): Uses werewolf_regen(ch, 2)
- Calls regen_limb() for missing body part recovery

### With WARP_REGENERATE

- Faster regeneration every tick
- Uses werewolf_regen(ch, 2)
- Automatic limb recovery via regen_limb()

## Weaknesses

### Verified Restrictions

- `IMM_TRAVEL` - Blocks demon travel ability
- `IMM_SUMMON` - Blocks demongate summon
- `ROOM_ASTRAL` - Cannot travel to/from astral rooms
- `AFF_WEBBED` - Cannot cast while webbed
- Cannot use abilities while polymorphed
- Cannot use demongate/chaosportal in arena
- Rage cannot exceed 125 while active

### Negative Warp Effects

- WARP_VULNER - Increased magic vulnerability
- WARP_WEAK - Reduced combat effectiveness
- WARP_SLOW - Reduced speed advantages
- WARP_CLUMSY - Easier to disarm
- WARP_STUPID - Cannot cast spells

**Note**: Holy damage, blessed weapon bonuses, and exorcism mechanics are NOT implemented in the current codebase.

## DEM_* Power Flags (demon.h)

```c
DEM_FANGS (1)         - Fangs extended
DEM_CLAWS (2)         - Claws extended
DEM_HORNS (4)         - Horns extended
DEM_HOOVES (8)        - Hooves present
DEM_EYES (16)         - Eyes modified (nightsight)
DEM_WINGS (32)        - Wings present
DEM_MIGHT (64)        - Strength bonus
DEM_TOUGH (128)       - Damage reduction
DEM_SPEED (256)       - Speed bonus
DEM_TRAVEL (512)      - Teleportation ability
DEM_SCRY (1024)       - Scrying ability
DEM_SHADOWSIGHT (2048) - Planar sight
DEM_TRUESIGHT (1048576) - True sight
DEM_SHIELD (524288)   - Protection gift
DEM_GRAFT (2097152)   - Graft limbs
DEM_IMMOLATE (4194304) - Weapon flame
DEM_INFERNO (8388608) - Inferno spell
DEM_CAUST (16777216)  - Weapon venom
DEM_ENTOMB (33554432) - Ice walls
DEM_FREEZEWEAPON (67108864) - Weapon frost
DEM_UNNERVE (121601760) - Stance force
DEM_LEECH (243203520) - Life drain
DEM_BLINK (486407040) - Combat teleport
```

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_DEMON bit |
| disciplines | ch->power[] | Discipline levels (indices 30-38) |
| warp | ch->warp | Warp bitfield |
| warpcount | ch->warpcount | Total warps (max 18) |
| demon_current | ch->pcdata->stats[8] | Spendable points |
| demon_total | ch->pcdata->stats[9] | Lifetime points |
| demon_power | ch->pcdata->stats[10] | Armor from own gifts |
| demon_ppower | ch->pcdata->stats[11] | Armor from others' gifts |
| beast | ch->beast | Beast counter (0-100) |
| rage | ch->rage | Rage value (max 125) |
| powers[] | ch->pcdata->powers[] | Gift flags and states |
