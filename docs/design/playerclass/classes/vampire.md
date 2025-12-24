# Vampire Class Design

## Overview

Vampires are undead creatures of the night who survive by feeding on the blood of the living. They possess a wide array of supernatural disciplines and exist within a hierarchical generation system.

**Source File**: `src/vamp.c`
**Class Constant**: `CLASS_VAMPIRE` (8)

## Core Mechanics

### Blood (Thirst)

Vampires require blood to survive and power their abilities:

```c
ch->pcdata->condition[COND_THIRST]  // Blood pool
```

- Blood is gained by feeding on victims
- Blood is spent to activate disciplines
- Low blood causes weakness and triggers the Beast

### The Beast

The Beast represents the vampire's primal hunger:

```c
ch->beast   // Beast level (0-100)
ch->rage    // Current rage state
```

- Beast increases when hungry or provoked
- High beast can cause frenzy (loss of control)
- Controlled through willpower or feeding

### Generation

Generation determines power hierarchy:

```c
ch->pcdata->generation  // Lower = more powerful
```

- Lower generation = older, more powerful vampire
- Affects maximum discipline levels
- Determines siring capability

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

Tracked in `ch->vampaff_a`:

```c
#define VAM_DISGUISED    // Currently disguised
#define VAM_CLAWS        // Claws extended
#define VAM_FANGS        // Fangs extended
#define VAM_NIGHTSIGHT   // Enhanced night vision
#define VAM_FLYING       // Currently flying
// ... etc
```

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

### Obfuscate Example
| Level | Ability |
|-------|---------|
| 1 | Cloak of Shadows - Hide in darkness |
| 2 | Unseen Presence - Move while hidden |
| 3 | Mask of Faces - Change appearance |
| 4 | Vanish - Combat invisibility |
| 5 | Cloak the Gathering - Hide others |

### Potence
| Level | Effect |
|-------|--------|
| 1-10 | +damage per level in combat |

### Fortitude
| Level | Effect |
|-------|--------|
| 1-10 | Damage reduction per level |

### Celerity
| Level | Effect |
|-------|--------|
| 1-10 | Extra attacks/dodges per level |

## Weaknesses

- **Sunlight**: Severe damage in daylight
- **Fire**: Vulnerable to fire damage
- **Stakes**: Can be paralyzed by staking
- **Holy**: Damaged by holy items/water
- **Invitation**: May require invitation to enter

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

Vampires can be incapacitated:

```c
// Remove vampire status when staked
REMOVE_BIT(victim->class, CLASS_VAMPIRE);
```

## Siring (Creating New Vampires)

The embrace command creates new vampires:

```c
void do_embrace(CHAR_DATA *ch, char *argument) {
    // Check generation allows siring
    // Drain victim's blood
    // Feed victim vampire blood
    // Set victim's class to vampire
    SET_BIT(victim->class, CLASS_VAMPIRE);
    victim->pcdata->generation = ch->pcdata->generation + 1;
}
```

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
