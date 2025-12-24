# Angel Class Design

## Overview

Angels are God's avengers, sent to punish the wicked. They are an **upgrade class** obtained by upgrading from Monk.

**Source Files**: `src/angel.c`, `src/angel.h`
**Class Constant**: `CLASS_ANGEL` (2048)
**Upgrades From**: Monk

## Lore (from help.are)

> Angels, being the creation of the holy God, were created perfect without any flaw or sin. They are considered holy due to the cause of their creation, smithing evil everywhere and being the messengers of God. The word of an angel is the direct word of God, and must be considered as a divine command by all worshippers.

## Power System

Angels have four power paths stored in `ch->pcdata->powers[]`:

### Power Indices (angel.h)
```c
#define ANGEL_PEACE         1   // Peace path level (0-5)
#define ANGEL_LOVE          2   // Love path level (0-5)
#define ANGEL_JUSTICE       3   // Justice path level (0-5)
#define ANGEL_HARMONY       4   // Harmony path level (0-5)
#define ANGEL_POWERS        5   // Bitfield for toggleable powers
#define ANGEL_PEACE_COUNTER 6   // Cooldown counter for houseofgod
```

### Toggle Flags (angel.h)
```c
#define ANGEL_WINGS     1   // Wings manifested
#define ANGEL_HALO      2   // Halo visible
#define ANGEL_AURA      4   // Angelic aura active
#define ANGEL_EYE       8   // Eye for an Eye active
```

## Power Paths (from help.are)

### PEACE Path
| Level | Ability |
|-------|---------|
| 1 | gpeace - God's peace protection |
| 2 | spiritform - Transform to pure energy |
| 3 | innerpeace - Healing through God's love |
| 4 | (none yet) |
| 5 | houseofgod - No fighting in God's house |

### LOVE Path
| Level | Ability |
|-------|---------|
| 1 | gsenses - God's Senses (truesight) |
| 2 | gfavor - God's Favor buff |
| 3 | forgiveness - Forgive sins |
| 4 | regeneration - Enhanced healing |
| 5 | martyr - Suffer pain of others |

### HARMONY Path
| Level | Ability |
|-------|---------|
| 1 | (none yet) |
| 2 | angelicaura - Aura of justice |
| 3 | gbanish - Banish evil creatures |
| 4 | (none yet) |
| 5 | harmony - Wreck body with imbalance |

### JUSTICE Path
| Level | Ability |
|-------|---------|
| 1 | swoop/awings - Fly with wings |
| 2 | halo - Divine radiance |
| 3 | sinsofthepast - Punish for sins |
| 4 | touchofgod - Strike down unbelievers |
| 5 | eyeforaneye - Retribution (they break your arm, you cut both theirs off) |

## Key Abilities

### Spiritform (angel.c:33-58)
Toggles AFF_ETHEREAL flag. Requires PEACE level 2.
- Become pure spiritual matter
- Can regain form and become solid again

### Gpeace (angel.c:60-85)
Toggles AFF_PEACE flag. Requires PEACE level 1.
- God's protection from harm
- Toggle on/off for combat

### Innerpeace (angel.c:87-112)
Self-heal ability. Requires PEACE level 3.
- Costs 1500 mana
- Heals (PEACE level * 500) HP

### Houseofgod (angel.c:114-141)
Room peace effect. Requires PEACE level 5.
- 50-tick cooldown (ANGEL_PEACE_COUNTER)
- Temporarily stops all fighting in room

### Angelicaura (angel.c:143-167)
Toggle damage aura. Requires HARMONY level 2.
- Extra damage vs evil targets
- Stored in ANGEL_POWERS bitfield

### Gbanish (angel.c:169-240)
Banish evil creatures. Requires HARMONY level 3.
- Damage based on target alignment:
  - Alignment > 0: 500 damage
  - Alignment > -500: 1000 damage
  - Alignment <= -500: 1500 damage
- Cannot target good (alignment > 500)
- 30% chance to teleport target to Hell

## Equipment Creation

Angels can create angelic armor with `angelicarmor <piece>`.
Also have WINGS and a HALO as innate features.

## Commands (from help.are)

| Command | Description |
|---------|-------------|
| angelicaura | Aura of justice to strike down evil |
| martyr | Suffer the pain of others |
| innerpeace | Let God's love heal your wounds |
| touchofgod | Strike down unbelievers |
| gpeace | No harm to those protected by God |
| eyeforaneye | Justice of God (retribution) |
| gfavor | Fill with God's love |
| gsenses | See through God's eyes |
| swoop | Fly when you have wings |
| houseofgod | No fighting in house of God |
| gbanish | Banish pure EVIL creatures |
| forgiveness | Forgive sins of another |
| sinsofthepast | Punish for sins |
| spiritform | Return to true form |
| harmony | Let prey's imbalance wreck their body |

## Data Storage Summary

| Field | Location | Purpose |
|-------|----------|---------|
| class | ch->class | CLASS_ANGEL bit |
| peace | ch->pcdata->powers[ANGEL_PEACE] | Peace path level |
| love | ch->pcdata->powers[ANGEL_LOVE] | Love path level |
| justice | ch->pcdata->powers[ANGEL_JUSTICE] | Justice path level |
| harmony | ch->pcdata->powers[ANGEL_HARMONY] | Harmony path level |
| toggles | ch->pcdata->powers[ANGEL_POWERS] | WINGS/HALO/AURA/EYE bits |
| cooldown | ch->pcdata->powers[ANGEL_PEACE_COUNTER] | Houseofgod cooldown |
| ethereal | AFF_ETHEREAL in affected_by | Spiritform active |
| peace_aura | AFF_PEACE in affected_by | Gpeace active |

## Notes

- True angels master: PEACE, LOVE, HARMONY and GOD'S JUSTICE
- Upgrade classes can further upgrade (levels 2-5) for additional combat bonuses
- Unique class made for Dystopia
