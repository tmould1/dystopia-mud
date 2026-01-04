# Vampire Class Design

## Overview

Vampires are undead creatures of the night who survive by feeding on the blood of the living. They possess a wide array of supernatural disciplines and exist within a hierarchical generation system.

**Source Files**: `src/classes/vamp.c` (4200+ lines), `src/systems/clan.c` (fangs, claws, age ranks), `src/systems/update.c` (sunlight, feeding loop)
**Class Constant**: `CLASS_VAMPIRE` (8)

## Core Mechanics

### Blood (Thirst)

Vampires require blood to survive and power their abilities:

```c
ch->pcdata->condition[COND_THIRST]  // Blood pool
```

**Blood Pool Maximum** (from `clan.c`):
- With Thaumaturgy Tide power: `3000 / generation`
- Without Tide: `2000 / generation`
- Example: Gen 3 vampire = 666-1000 max blood

**Mechanics**:
- Blood is gained by feeding (30-42 per pulse)
- Blood is spent to activate powers
- Low blood (<=15) triggers Beast takeover (`update.c:265-270`)
- Vampires fully restore HP/mana/move at sunset (`update.c:710-740`)

### The Beast

The Beast represents the vampire's primal hunger:

```c
ch->beast   // Beast level (0-100)
ch->rage    // Current rage state
```

- Beast increases when hungry or provoked
- High beast can cause frenzy (loss of control)
- While rage > 0: Cannot retract claws/fangs, cannot use nightsight
- Controlled through willpower or feeding

### Generation

Generation determines power hierarchy:

```c
ch->pcdata->generation  // Lower = more powerful
```

- Lower generation = older, more powerful vampire
- Affects blood pool maximum and discipline caps
- Determines siring capability (childer are generation + 1)

### Age Ranks

Vampires progress through ranks based on age/experience (from `merc.h`):

| Rank | Title | Notes |
|------|-------|-------|
| 0 | Childe | Default starting rank |
| 1 | Neonate | AGE_NEONATE |
| 2 | Ancilla | AGE_ANCILLA |
| 3 | Elder | AGE_ELDER - Can become Inconnu |
| 4 | Methuselah | AGE_METHUSELAH |
| 5 | La Magra | AGE_LA_MAGRA |
| 6 | TrueBlood | AGE_TRUEBLOOD |

**Inconnu**: Special vampire society. Requires Elder rank + 1,000,000 exp. Command: `inconnu` (sets `SPC_INCONNU` flag)

## Disciplines

Vampires have access to 20 disciplines:

| Discipline | Index | Description |
|------------|-------|-------------|
| Animalism | DISC_VAMP_ANIM | Beast control, animal communication |
| Auspex | DISC_VAMP_AUSP | Enhanced senses, aura reading |
| Celerity | DISC_VAMP_CELE | Supernatural speed |
| Chimerstry | DISC_VAMP_CHIM | Illusion creation |
| Daimoinon | DISC_VAMP_DAIM | Dark thaumaturgy |
| Dominate | DISC_VAMP_DOMI | Mind control |
| Fortitude | DISC_VAMP_FORT | Physical resilience |
| Melpominee | DISC_VAMP_MELP | Voice powers |
| Necromancy | DISC_VAMP_NECR | Death magic |
| Obeah | DISC_VAMP_OBEA | Healing powers |
| Obfuscate | DISC_VAMP_OBFU | Invisibility, stealth |
| Obtenebration | DISC_VAMP_OBTE | Shadow manipulation |
| Potence | DISC_VAMP_POTE | Supernatural strength |
| Presence | DISC_VAMP_PRES | Emotional manipulation |
| Protean | DISC_VAMP_PROT | Shape-changing |
| Quietus | DISC_VAMP_QUIE | Assassination arts |
| Serpentis | DISC_VAMP_SERP | Serpent powers |
| Thanatosis | DISC_VAMP_THAN | Death aspect |
| Thaumaturgy | DISC_VAMP_THAU | Blood magic |
| Vicissitude | DISC_VAMP_VICI | Flesh crafting |

## Vampire Affinity Flags

Tracked in `ch->vampaff_a` (from `merc.h`):

| Flag | Effect |
|------|--------|
| VAM_FANGS | Fangs extended |
| VAM_CLAWS | Claws extended |
| VAM_NIGHTSIGHT | Enhanced night vision active |
| VAM_DISGUISED | Currently disguised (mask) |
| VAM_FLYING | Currently flying |
| VAM_SONIC | Detect all creatures |
| VAM_CHANGED | Shape-changed via vampire power |

**Discipline Flags** (active discipline effects):
VAM_PROTEAN, VAM_CELERITY, VAM_FORTITUDE, VAM_POTENCE, VAM_OBFUSCATE, VAM_AUSPEX, VAM_OBTENEBRATION, VAM_SERPENTIS, VAM_DOMINATE, VAM_PRESENCE, VAM_VICISSITUDE, VAM_THAU, VAM_ANIMAL, VAM_QUIETUS

**Vicissitude Body Mods**:
VAM_HEAD, VAM_TAIL, VAM_EXOSKELETON, VAM_HORNS, VAM_WINGS

## Key Commands

### Feeding
- `feed` - Bite and drain blood from victim
- `bloodlet` - Shed blood for rituals

### Combat
- `claws` - Extend vampiric claws
- `fangs` - Extend fangs for biting
- `shadowstep` - Teleport through shadows (Obtenebration)

### Utility
- `disciplines` - View discipline levels
- `mask` - Disguise appearance (Obfuscate)
- `nightsight` - Toggle enhanced vision
- `embrace` - Create a new vampire (sire)

## Discipline Abilities by Level

### Passive Combat Disciplines

These disciplines provide passive bonuses without active commands:

| Discipline | Effect |
|------------|--------|
| Potence | +damage per level in combat |
| Fortitude | Damage reduction per level |
| Celerity | Extra attacks/dodges per level |

### Animalism (DISC_VAMP_ANIM)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `beckon` | Summon animals to aid you |
| 2 | `serenity` | Calm animals and soothe presence |
| 3 | `pigeon` | Scry through pigeon eyes |
| 4 | `share` | Share senses with an animal |
| 5 | `frenzy` | Enter a berserk frenzy state |

### Auspex (DISC_VAMP_AUSP)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `truesight` | See through illusions and deceptions |
| 2 | `readaura` | Read target's aura and intentions |
| 3 | `scry` | Remote viewing of locations/targets |
| 4 | `astralwalk` | Project your astral form |
| 5 | `unveil` | Reveal hidden truths |

### Chimerstry (DISC_VAMP_CHIM)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `mirror` | Create a mirror image illusion |
| 2 | `formillusion` | Form complex illusions |
| 4 | `controlclone` | Control illusory clones |

### Daimoinon (DISC_VAMP_DAIM)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `guardian` | Summon a spirit guardian |
| 2 | `fear` | Force opponent to flee in terror |
| 2 | `bloodwall` | Create a wall of blood |
| 3 | `portal` / `gate` | Create a teleportation portal |
| 5 | `vtwist` | Modify properties of objects |
| 5 | `baal` | Summon Baal spirit |
| 6 | `inferno` | Summon infernal flames |

### Dominate (DISC_VAMP_DOMI)
| Level | Command | Effect |
|-------|---------|--------|
| 3 | `possession` | Possess another body |
| 4 | `acid` | Acid control/attack |
| 8 | `forget` | Make target forget events |

### Melpominee (DISC_VAMP_MELP)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `scream` | Emit powerful sonic scream that stuns enemies |

### Necromancy (DISC_VAMP_NECR)
| Level | Command | Effect |
|-------|---------|--------|
| 2 | `preserve` | Preserve objects from decay |
| 3 | `spiritgate` | Travel through spirit portals to corpses |
| 4 | `spiritguard` | Awaken/dismiss spirit guardian |
| 5 | `zombie` | Raise the dead as zombies |
| 5 | `bloodwater` | Turn enemy's blood to water |

### Obeah (DISC_VAMP_OBEA)
| Level | Command | Effect |
|-------|---------|--------|
| 7 | `purify` | Purify mind/body of corruption |

### Obfuscate (DISC_VAMP_OBFU)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `vanish` | Fade from visibility and become hidden |
| 2 | `mask` | Change appearance to look like another |
| 3 | `shield` | Shield presence from detection |
| 4 | `dub` | Rename/disguise items |
| 5 | `conceal` | Make items invisible |

### Obtenebration (DISC_VAMP_OBTE)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `shroud` | Shroud yourself in shadow |
| 2 | `shadowsight` | See through shadows and darkness |
| 3 | `shadowplane` | Enter shadow plane for travel |
| 4 | `shadowstep` | Step through shadows to teleport |
| 5 | `lamprey` | Shadow tentacle attack |
| 8 | `grab` | Shadow grasp to immobilize |
| 10 | `shadowgaze` | Gaze through shadow barriers |

### Presence (DISC_VAMP_PRES)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `awe` | Inspire awe in others |
| 3 | `entrance` | Entrance/mesmerize target |
| 4 | `summon` | Summon willing followers |
| 5 | `majesty` | Emit overwhelming majesty/dominance |

### Protean (DISC_VAMP_PROT)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `nightsight` | See perfectly in darkness |
| 2 | `claws` | Manifest supernatural claws |
| 3 | `change` | Change/shift form |
| 4 | `earthmeld` | Meld into earth for concealment |
| 5 | `flamehands` | Hands wreathed in flames |
| 8 | `healing` | Heal wounds through blood magic |

### Quietus (DISC_VAMP_QUIE)
| Level | Command | Effect |
|-------|---------|--------|
| 2 | `infirmity` | Inflict debilitating infirmity |
| 3 | `bloodagony` | Cause blood agony to target |
| 4 | `assassinate` | Lethal assassination strike |
| 5 | `vsilence` | Silence/death power |
| 9 | `flash` | Flash teleportation ability |

### Serpentis (DISC_VAMP_SERP)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `darkheart` | Infuse darkness into heart |
| 2 | `serpent` | Assume serpent form/traits |
| 3 | `poison` | Infuse poison into blood/fangs |
| 4 | `tendrils` | Manifest serpent tendrils as weapons |
| 4 | `cserpent` | Become serpent creature |
| 5 | `scales` | Gain serpent scales for protection |
| 8 | `coil` | Coil around/bind opponent |

### Thanatosis (DISC_VAMP_THAN)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `hagswrinkles` | Temporarily age/wither appearance |
| 2 | `putrefaction` | Cause target's flesh to rot (requires Vicissitude 2) |
| 4 | `withering` | Wither target over time |
| 5 | `drainlife` | Drain life force from opponent |

### Thaumaturgy (DISC_VAMP_THAU)
| Level | Command | Effect |
|-------|---------|--------|
| 1 | `taste` | Taste blood to read information |
| 2 | `cauldron` | Blood cauldron ritual |
| 4 | `theft` | Steal power/essence from opponent |
| 5 | `tide` | Control blood flow/tides |
| 6 | `spew` | Spew blood as weapon |
| 8 | `gourge` | Feed on small creatures for blood |

### Vicissitude (DISC_VAMP_VICI)
| Level | Command | Effect |
|-------|---------|--------|
| 2 | `fleshcraft` | Reshape/craft flesh |
| 2 | `zuloform` | Shapeshift into bestial form |
| 3 | `bonemod` | Modify bone structure |
| 4 | `dragonform` | Shapeshift into dragon form |
| 5 | `plasma` | Convert body to plasma |

## Weaknesses

Verified weaknesses with code references:

| Weakness | Effect | Source |
|----------|--------|--------|
| Sunlight | 5-10 damage/tick outdoors in daylight (2-4 in serpent form) | `update.c:409-423` |
| Blood Thirst | Beast takes over when blood <= 15 | `update.c:265-270` |

**Note**: Fire, holy items, and invitation requirements are NOT implemented in the current codebase.

## Combat Bonuses

```c
// Potence adds to damage
damage += ch->power[DISC_VAMP_POTE] * multiplier;

// Fortitude reduces damage taken
damage -= ch->power[DISC_VAMP_FORT] * multiplier;

// Celerity grants extra attacks
extra_attacks = ch->power[DISC_VAMP_CELE];
```

## Staking Mechanics

Staking allows incapacitating a mortally wounded vampire (`clan.c:1922-2005`).

**Requirements**:
- Attacker must hold/wield an ITEM_STAKE
- Target must be a vampire player in POS_MORTAL

**Effects** (blocked by IMM_STAKE):
- Removes all active powers: disguise, shield, shadowplane, fangs, claws, nightsight, shadowsight, truesight, changed form, serpent form
- Resets rage to 0
- Drains all blood (COND_THIRST = 0)
- Stake transfers to victim's inventory
- Attacker gains 1000 exp

## Embrace (Feeding)

The `embrace` command initiates feeding on an NPC (`vamp.c:2830`):
- Target must be an NPC (cannot embrace players)
- Target must be stunned, sleeping, or dead
- Sets up continuous blood drain via `ch->embracing` / `victim->embraced`
- Blood is drained over time in the update loop

**Note**: There is no player-to-player siring mechanic. Players become vampires through the class selection system.

## Diablerie

Vampires can consume other vampires via `diablerise` command:
- Target must be mortally wounded (POS_MORTAL)
- Both vampires extend fangs during the process
- Drains the victim's essence

## Transformation Forms

Vampires can assume various forms via Protean/Vicissitude disciplines:

| Form | Flag | Notes |
|------|------|-------|
| Bat | POLY_BAT | Flight capability |
| Wolf | POLY_WOLF | Enhanced speed/tracking |
| Mist | POLY_MIST | Intangible, can pass through barriers |
| Serpent | POLY_SERPENT | Reduced sunlight damage (2-4 vs 5-10) |
| Zuloform | POLY_ZULOFORM | Bestial combat form |
| Dragon | EXTRA_DRAGON | Via `dragonform` (Vicissitude 4) |

## Baal Spirit

The `baal` command (Dominate 5) summons the Spirit of Baal (`vamp.c:1095-1122`):
- Requires maximum blood pool, which is fully consumed
- Grants +2 to Potence, Celerity, and Fortitude (can exceed normal cap of 10)
- Expires at sunset, removing the bonuses (`update.c:734-741`)

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_VAMPIRE bit |
| disciplines | ch->power[] | Discipline levels |
| blood | ch->pcdata->condition[COND_THIRST] | Blood pool |
| beast | ch->beast | Beast/frenzy level |
| rage | ch->rage | Current rage |
| generation | ch->pcdata->generation | Power hierarchy |
| vampaff_a | ch->vampaff_a | State flags |
