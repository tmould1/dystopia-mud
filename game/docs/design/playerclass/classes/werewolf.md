# Werewolf Class Design

## Overview

Werewolves (Garou) are shapechangers who draw power from primal totem spirits and the moon. They use Gnosis as their spiritual resource and can transform into a devastating war form when their rage builds. This is a **base class** that upgrades to Shapeshifter.

**Source Files**: `src/classes/ww.c`, `src/classes/garou.h`, `src/classes/clan.c` (transform functions)
**Class Constant**: `CLASS_WEREWOLF` (4)
**Upgrades To**: Shapeshifter

## Core Mechanics

### Gnosis

Gnosis is the spiritual energy that powers werewolf abilities (`garou.h:44-45`):

```c
ch->gnosis[GCURRENT]   // Current gnosis points (index 0)
ch->gnosis[GMAXIMUM]   // Maximum gnosis pool (index 1)
```

**Regeneration**: +1 gnosis per game tick when below maximum (`update.c:259`)

**Gnosis Costs**:
- Disquiet: 1 point
- Cocoon: 2 points
- Wither: 3 points

### Rage System

Rage controls werewolf transformation (`update.c:1696-1721`, `clan.c:3776-3911`):

```c
ch->rage  // Current rage level (0-300)
```

**Building Rage**:
- Combat: +5-10 per tick while fighting
- Wolf 4+: Additional +5-10 per tick (max +20)
- `rage` command: +40-60 manually
- Maximum: 300

**Werewolf Form Trigger** (`do_werewolf`):
- Automatic at rage >= 100 during combat
- Manual via `rage` command
- Sets `SPC_WOLFMAN` flag

**Form Benefits**:
- +50 hitroll, +50 damroll
- Auto-extends nightsight, fangs, claws
- Name morphs to "[name] the werewolf"
- 2x multi_hit attacks on all visible targets

**Untransform** (rage < 100 out of combat):
- Removes bonuses
- Retracts claws/fangs/nightsight

### Silver Weakness

Silver weapons deal bonus damage to werewolves (`fight.c:1892-1896`):

```c
int silver_tol = (victim->siltol * 2.5);
if (IS_CLASS(victim, CLASS_WEREWOLF) && IS_ITEMAFF(ch, ITEMA_RIGHT_SILVER))
    max_dam += (250 - silver_tol);
if (IS_CLASS(victim, CLASS_WEREWOLF) && IS_ITEMAFF(ch, ITEMA_LEFT_SILVER))
    max_dam += (250 - silver_tol);
```

- Base: +250 damage per silver weapon
- Reduced by silver tolerance (`ch->siltol`)
- Dual silver: +500 max damage (without tolerance)
- Silver boots with `SITEM_SILVER`: double damage (`fight.c:4295`)

### Regeneration

Werewolves heal faster than other classes (`update.c:1826-1879`):

**Sleeping**:
- +1200-1700 HP per tick
- +600-850 mana per tick
- +1200-1700 move per tick

**Fighting**:
- +275-475 HP per tick
- +138-238 mana per tick
- +275-475 move per tick

**Hell Rooms** (vnum 93430-93439): 2x multiplier

## Disciplines (Totems)

Werewolves have 12 totem disciplines stored in `ch->power[]`:

| Totem | Index | Theme |
|-------|-------|-------|
| Bear | DISC_WERE_BEAR (18) | Strength, endurance, regeneration |
| Lynx | DISC_WERE_LYNX (19) | Stealth, hunting |
| Boar | DISC_WERE_BOAR (20) | Ferocity, equipment destruction |
| Owl | DISC_WERE_OWL (21) | Manipulation, deception, shields |
| Spider | DISC_WERE_SPID (22) | Webbing |
| Wolf | DISC_WERE_WOLF (23) | Pack tactics, claws, fangs |
| Hawk | DISC_WERE_HAWK (24) | Perception, awareness |
| Mantis | DISC_WERE_MANT (25) | Precision, martial arts |
| Raptor | DISC_WERE_RAPT (26) | Predation, ferocity |
| Luna | DISC_WERE_LUNA (27) | Moon powers, healing |
| Pain | DISC_WERE_PAIN (28) | Pain tolerance |
| Congregation | DISC_WERE_CONG (29) | Pack bonding |

## Totem Abilities by Level

### Luna (DISC_WERE_LUNA) - Moon/Healing

| Level | Command | Effect |
|-------|---------|--------|
| 1 | `flameclaws` | Toggle flaming claws for fire damage (`ww.c:403`) |
| 2 | `moonarmour` | Create moonlight armor pieces (60 primal each) (`ww.c:438`) |
| 3 | `motherstouch` | Heal target 100 HP (fight) / 500 HP (non-fight) (`ww.c:336`) |
| 4 | `gmotherstouch` | Heal target 200 HP (fight) / 600 HP (non-fight) (`ww.c:261`) |
| 5 | `sclaws` | Toggle pure silver claws for enhanced damage (`ww.c:80`) |
| 6 | `moongate` | Create teleportation portals to/from target player (`ww.c:177`) |
| 8 | `moonbeam` | Alignment-based damage: Good 500, Evil 1000, Neutral 750 (`ww.c:116`) |

### Bear (DISC_WERE_BEAR) - Strength/Combat

| Level | Command | Effect |
|-------|---------|--------|
| 3+ | (passive) | Enhanced regeneration while sleeping/fighting |
| 6 | `roar` | 33% chance to make opponent flee in terror (`ww.c:620`) |
| 7 | `skin` | Toggle hardened skin (+100 AC reduction) (`ww.c:534`) |
| 8 | `slam` | Toggle shoulder slam mode for extra combat hits (`ww.c:690`) |

### Boar (DISC_WERE_BOAR) - Ferocity

| Level | Command | Effect |
|-------|---------|--------|
| 5 | `burrow` | Burrow to another werewolf player (`ww.c:1176`) |
| 7 | `rend` | Toggle equipment destruction in combat (`ww.c:506`) |

### Owl (DISC_WERE_OWL) - Manipulation/Deception

| Level | Command | Effect |
|-------|---------|--------|
| 1 | `vanish` | Toggle hiding (AFF_HIDE) (`clan.c:4066`) |
| 2 | `shield` | Shield presence from detection |
| 3 | `shadowplane` | Toggle shadow plane phase for travel (`clan.c:2546`) |
| 5 | `staredown` | Force fighting target to flee (33-100% based on level) (`ww.c:887`) |
| 6 | `disquiet` | Reduce target's hit/damroll by (Owl × 5) for Owl rounds (`ww.c:962`) |
| 7 | `reshape` | Rename/reshape carried items (`ww.c:1021`) |
| 8 | `cocoon` | Create protective epidermis shield (2 gnosis) (`ww.c:1091`) |

### Spider (DISC_WERE_SPID)

| Level | Command | Effect |
|-------|---------|--------|
| 2 | `web` | Cast web spell to trap target (`clan.c:4276`) |

### Wolf (DISC_WERE_WOLF) - Pure Form

| Level | Command | Effect |
|-------|---------|--------|
| 1 | `claws` | Toggle extended claws for unarmed combat (`clan.c:1503`) |
| 2 | `fangs` | Toggle sharp fangs for bite attacks (`clan.c:1569`) |
| 3 | `calm` | Reduce rage value (`clan.c:4197`) |
| 4 | `razorclaws` | Toggle razor-sharp edges on extended claws (`ww.c:1463`) |

### Hawk (DISC_WERE_HAWK) - Perception

| Level | Command | Effect |
|-------|---------|--------|
| 1 | `nightsight` | Toggle enhanced dark vision (`clan.c:1617`) |
| 2 | `shadowsight` | Toggle viewing between shadow planes (`clan.c:1721`) |
| 5 | `quills` | Toggle sharp bristly fur quills for defense (`ww.c:1142`) |
| 6 | `burrow` | Travel to another werewolf avatar player (`ww.c:1176`) |
| 7 | `wither` | Wither victim's arm (3 gnosis, 15-55% success) (`ww.c:1261`) |

### Raptor (DISC_WERE_RAPT) - Predation

| Level | Command | Effect |
|-------|---------|--------|
| 3 | `perception` | Toggle enhanced awareness of stealthy enemies (`ww.c:592`) |
| 5 | `devour` | Consume NPC corpse to heal 100-250 HP (`ww.c:830`) |
| 7 | `shred` | Multi-strike attack from shadowplane (2-5 hits) (`ww.c:718`) |
| 8 | `jawlock` | Toggle clamping jaw for combat (`ww.c:564`) |
| 10 | `talons` | Heavy talon attack: 400-600 vs PC, 2000-4000 vs NPC (`ww.c:790`) |

## Combat Flags

**Garou-specific flags** (`ch->garou1`):

| Flag | Effect |
|------|--------|
| WOLF_RAZORCLAWS (1) | Razor claws: 25d35 unarmed damage vs normal 10d20 (`fight.c:1296`) |
| WOLF_COCOON (2) | Cocoon shield: 50% damage reduction (`fight.c:1724`) |

**NEW_* combat flags** (`ch->newbits`):
- `NEW_REND` - Equipment destruction mode
- `NEW_SKIN` - Hardened skin (+100 AC reduction)
- `NEW_SLAM` - Shoulder slam ready
- `NEW_JAWLOCK` - Jaw lock active
- `NEW_PERCEPTION` - Enhanced perception
- `NEW_QUILLS` - Quill defense
- `NEW_SCLAWS` - Silver claws active

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_WEREWOLF bit |
| totems | ch->power[] | Totem levels (indices 18-29) |
| gnosis | ch->gnosis[2] | Current/max gnosis |
| rage | ch->rage | Rage meter (0-300) |
| garou1 | ch->garou1 | Toggle flags (WOLF_RAZORCLAWS, WOLF_COCOON) |
| siltol | ch->siltol | Silver tolerance |

**Note**: The `ch->gifts[]` array and breed/auspice/tribe constants in `garou.h` are defined but NOT implemented - they have no gameplay effect.
