# Discipline System Design

## Overview

The discipline system provides a power progression framework for three classes: **Vampire**, **Werewolf**, and **Demon**. Each of these classes has a set of disciplines (power trees) that can be learned and advanced, stored in the `ch->power[]` array.

**Note:** Other classes use different power systems:
- **Mage**: Spheres (`ch->spl[]` - elemental magic colors)
- **Angel**: Spheres (`ch->pcdata->powers[]` - Peace, Love, Justice, Harmony)
- **Ninja**: Powers (`ch->pcdata->powers[]` - Sora, Chikyu, Ningenno paths)
- **Monk**: Mantras (`ch->pcdata->powers[]` - Spirit, Body, Awareness, Combat)
- **Drow**: Powers/Gifts (bit flags for Lolth-granted abilities)
- **Lich**: Lores/Paths (Necromantic, Life, Death, Chaos, Conjuring)
- **Samurai**: Focus/Martial techniques
- **Shapeshifter**: Forms (Bull, Hydra, Tiger, Faerie)
- **Spider Droid**: Implants (cybernetic upgrades)
- **Tanarri**: Rank-based powers
- **Undead Knight**: Paths/Auras (Necromancy, Invocation, Spirit)

## Architecture

### Storage

Disciplines are stored in the `power[]` array on CHAR_DATA:

```c
#define MAX_DISCIPLINES 44

struct char_data {
    int power[MAX_DISCIPLINES];  // Discipline levels
    // ...
};
```

### Discipline States

| Value | Meaning |
|-------|---------|
| -2 | Unavailable (cannot learn) |
| -1 | Reserved/special state |
| 0 | Learnable (not yet trained) |
| 1+ | Trained level |

## Discipline Constants

Defined in `merc.h`, organized by class:

### Vampire Disciplines (20 total)

```c
#define DISC_VAMP_FORT   3   // Fortitude - physical resilience
#define DISC_VAMP_CELE   2   // Celerity - supernatural speed
#define DISC_VAMP_OBTE   4   // Obtenebration - shadow manipulation
#define DISC_VAMP_PRES   5   // Presence - emotional manipulation
#define DISC_VAMP_QUIE   6   // Quietus - assassination arts
#define DISC_VAMP_THAU   7   // Thaumaturgy - blood magic
#define DISC_VAMP_AUSP   8   // Auspex - enhanced senses
#define DISC_VAMP_DOMI   9   // Dominate - mind control
#define DISC_VAMP_OBFU  10   // Obfuscate - invisibility/stealth
#define DISC_VAMP_POTE  11   // Potence - supernatural strength
#define DISC_VAMP_PROT  12   // Protean - shape-changing
#define DISC_VAMP_SERP  13   // Serpentis - serpent powers
#define DISC_VAMP_VICI  14   // Vicissitude - flesh crafting
#define DISC_VAMP_DAIM  15   // Daimoinon - dark thaumaturgy
#define DISC_VAMP_ANIM  16   // Animalism - beast control
#define DISC_VAMP_CHIM  39   // Chimerstry - illusions
#define DISC_VAMP_MELP  43   // Melpominee - voice powers
#define DISC_VAMP_NECR  42   // Necromancy - death magic
#define DISC_VAMP_THAN  40   // Thanatosis - death aspect
#define DISC_VAMP_OBEA  41   // Obeah - healing
```

### Werewolf Disciplines (12 total)

```c
#define DISC_WERE_BEAR  18   // Bear totem
#define DISC_WERE_LYNX  19   // Lynx totem
#define DISC_WERE_BOAR  20   // Boar totem
#define DISC_WERE_OWL   21   // Owl totem
#define DISC_WERE_SPID  22   // Spider totem
#define DISC_WERE_WOLF  23   // Wolf totem
#define DISC_WERE_HAWK  24   // Hawk totem
#define DISC_WERE_MANT  25   // Mantis totem
#define DISC_WERE_RAPT  26   // Raptor totem
#define DISC_WERE_LUNA  27   // Luna (moon powers)
#define DISC_WERE_PAIN  28   // Pain tolerance
#define DISC_WERE_CONG  29   // Congregation
```

### Demon Disciplines (9 total)

```c
#define DISC_DAEM_HELL  30   // Hellfire
#define DISC_DAEM_ATTA  31   // Attack powers (claws, etc.)
#define DISC_DAEM_TEMP  32   // Temptation
#define DISC_DAEM_MORP  33   // Morphing
#define DISC_DAEM_CORR  34   // Corruption
#define DISC_DAEM_GELU  35   // Gelugon (ice demon)
#define DISC_DAEM_DISC  36   // Discord
#define DISC_DAEM_NETH  37   // Nether powers
#define DISC_DAEM_IMMU  38   // Immunities
```

## Initialization

### set_learnable_disciplines() (handler.c)

Called when a character gains a class or on character load:

```c
void set_learnable_disciplines(CHAR_DATA *ch) {
    int i;

    // Mark all as unavailable
    for (i = 0; i < MAX_DISCIPLINES; i++)
        ch->power[i] = -2;

    // Vampire disciplines
    if (IS_SET(ch->class, CLASS_VAMPIRE)) {
        ch->power[DISC_VAMP_FORT] = 0;
        ch->power[DISC_VAMP_CELE] = 0;
        ch->power[DISC_VAMP_OBTE] = 0;
        ch->power[DISC_VAMP_PRES] = 0;
        ch->power[DISC_VAMP_QUIE] = 0;
        ch->power[DISC_VAMP_THAU] = 0;
        ch->power[DISC_VAMP_AUSP] = 0;
        ch->power[DISC_VAMP_DOMI] = 0;
        ch->power[DISC_VAMP_OBFU] = 0;
        ch->power[DISC_VAMP_POTE] = 0;
        ch->power[DISC_VAMP_PROT] = 0;
        ch->power[DISC_VAMP_SERP] = 0;
        ch->power[DISC_VAMP_VICI] = 0;
        ch->power[DISC_VAMP_DAIM] = 0;
        ch->power[DISC_VAMP_ANIM] = 0;
        ch->power[DISC_VAMP_CHIM] = 0;
        ch->power[DISC_VAMP_MELP] = 0;
        ch->power[DISC_VAMP_NECR] = 0;
        ch->power[DISC_VAMP_THAN] = 0;
        ch->power[DISC_VAMP_OBEA] = 0;
    }

    // Werewolf disciplines
    if (IS_SET(ch->class, CLASS_WEREWOLF)) {
        ch->power[DISC_WERE_BEAR] = 0;
        ch->power[DISC_WERE_LYNX] = 0;
        // ... etc
    }

    // Demon disciplines
    if (IS_SET(ch->class, CLASS_DEMON)) {
        ch->power[DISC_DAEM_HELL] = 0;
        ch->power[DISC_DAEM_ATTA] = 0;
        // ... etc
    }
}
```

## Checking Discipline Requirements

### In Command Functions

```c
void do_claws(CHAR_DATA *ch, char *argument) {
    // Check discipline level
    if (ch->power[DISC_DAEM_ATTA] < 1) {
        send_to_char("You need Attack level 1 to use claws.\n\r", ch);
        return;
    }

    // Use ability...
}
```

### In Command Table (interp.c)

Commands can specify discipline requirements:

```c
struct cmd_type {
    char *name;
    DO_FUN *do_fun;
    sh_int position;
    sh_int level;
    sh_int log;
    int class;           // Required class (CLASS_* constant)
    int discipline;      // Required discipline (DISC_* constant)
    int disc_level;      // Minimum discipline level
};
```

Example entries:
```c
{ "claws",     do_claws,     POS_SITTING, 3, LOG_NORMAL, CLASS_DEMON,   DISC_DAEM_ATTA, 1 },
{ "shadowstep",do_shadowstep,POS_STANDING,3, LOG_NORMAL, CLASS_VAMPIRE, DISC_VAMP_OBTE, 3 },
```

## Discipline Advancement

### Methods

1. **Admin Grant**: Immortals can set discipline levels directly
2. **In-Game Training**: Some classes have NPCs or mechanics for training
3. **Experience/Points**: Spending class-specific resources

### Level Caps

Most disciplines have soft caps in ability code:

```c
// Example from vampire code
if (ch->power[DISC_VAMP_POTE] > 10)
    ch->power[DISC_VAMP_POTE] = 10;  // Hard cap at 10
```

## Discipline Effects

### Combat Modifiers

Disciplines commonly affect:
- Damage output (Potence, Attack)
- Defense (Fortitude, Immunities)
- Speed/actions (Celerity)
- Special attacks (Thaumaturgy, Hellfire)

### Ability Unlocks

Higher discipline levels unlock new abilities:

```c
// Obfuscate example
switch (ch->power[DISC_VAMP_OBFU]) {
    case 1: // Cloak of Shadows - basic hiding
    case 2: // Unseen Presence - move while hidden
    case 3: // Mask of a Thousand Faces - disguise
    case 4: // Vanish from Mind's Eye - combat invisibility
    case 5: // Cloak the Gathering - hide others
}
```

## Class-Specific Power Systems

Some classes have additional power systems beyond disciplines:

### Werewolf Gifts

Separate from disciplines, stored in `ch->gifts[]`:
```c
int gifts[21];  // 21 gift slots
```

### Demon Warps

Mutation powers stored as bitfield:
```c
int warp;       // Bitfield of active warps
int warpcount;  // Number of warps (max 18)
```

### Monk Abilities

Four ability tracks:
```c
sh_int monkab[4];  // SPIRIT, BODY, MIND, CHI levels
```

### Drow Powers

Profession-based with hierarchy:
```c
// In pc_data
int powers[20];  // Powers[1] = granted powers bitfield
```

## Display Commands

Players can view their discipline levels:

```c
// Vampire: "disciplines" command
void do_disciplines(CHAR_DATA *ch, char *argument) {
    sprintf(buf, "Celerity: %d  Fortitude: %d  Potence: %d\n\r",
        ch->power[DISC_VAMP_CELE],
        ch->power[DISC_VAMP_FORT],
        ch->power[DISC_VAMP_POTE]);
    send_to_char(buf, ch);
    // ... etc
}
```

## Adding New Disciplines

### Steps

1. **Define constant** in `merc.h`:
   ```c
   #define DISC_NEW_POWER  44  // Next available slot
   #define MAX_DISCIPLINES 45  // Increase if needed
   ```

2. **Initialize** in `handler.c`:
   ```c
   if (IS_SET(ch->class, CLASS_NEWCLASS)) {
       ch->power[DISC_NEW_POWER] = 0;
   }
   ```

3. **Implement abilities** in class source file

4. **Register commands** in `interp.c`:
   ```c
   { "newpower", do_newpower, POS_STANDING, 3, LOG_NORMAL, CLASS_NEWCLASS, DISC_NEW_POWER, 1 },
   ```

## Design Notes

### Strengths
- Unified progression system across all classes
- Easy to add new disciplines
- Clear level requirements for abilities
- Flexible enough for diverse class designs

### Limitations
- Fixed array size requires code changes for expansion
- No sub-discipline or specialization trees
- All disciplines level linearly (no branching)

### Future Considerations
- Dynamic discipline registration
- Discipline prerequisites (require X before learning Y)
- Discipline synergies (bonus for multiple disciplines)
