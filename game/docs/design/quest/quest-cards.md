# Quest Card Collection System

Quest cards are a collection minigame where players create cards and hunt for specific items to complete them.

## Overview

1. Player casts `quest` spell, spending primal energy to create a card
2. Card is populated with 4 random items from game areas
3. Player finds and collects those items
4. Player uses `complete` command to check items off the card
5. Completed cards can be turned in for rewards

## Creating Quest Cards

### Spell: Quest

```
cast 'quest'
```

**Function:** `spell_quest()` in magic.c:4439-4468

**Requirements:**
- At least 1 primal energy (practice points)
- Player must be in a room

**Cost:**
- Uses primal energy equal to desired card level
- Maximum 100 primal per card
- Card level affects item difficulty

**Process:**
1. Creates card from VNUM 30039 (`OBJ_VNUM_QUESTCARD`)
2. Calls `quest_object()` to populate required items
3. Sets `questmaker` and `questowner` to player name

```c
// magic.c:4459-4460
if (ch->practice >= 100) {ch->practice -= 100; obj->level = 100;}
else {obj->level = ch->practice; ch->practice = 0;}
```

## Card Requirements

### Item Selection

**Function:** `quest_object()` in act_obj.c:3677-3734

Each card has 4 item slots stored in `value[0-3]`. Items are randomly selected from a pool of 150 objects:

```c
// act_obj.c:3679-3701
static const int quest_selection[] =
{
     /* New items */
     93004, 93005, 32001, 14028, 14030, 14014, 14023, 14031,
     14029, 31065, 31067, 31057, 31059, 31052, 50000, 50008, 50007,
     50201, 50204, 77011,
     /* Old items */
     1330, 9105,
     5005, 5011, 5012, 5013, 2902, 1352, 2348, 2361, 9231, 5011,
     5012, 5013, 2902, 1352, 2348, 2361, 2371,  300,  303,  307,
     7216, 1100,  100,30315, 5110, 6001, 3050,  301, 5230,30302,
      663, 7303, 2915, 2275, 8600, 8601, 8602, 8603, 5030, 9321,
     6010, 1304, 1307, 1332, 1333, 1342, 1356, 1361, 2304, 2322,
     2331, 2382, 8003, 8005, 5300, 5302, 5309, 5310, 5311, 4000,
     2624,  311, 7203, 7206, 5214,
     5223, 5228, 2804, 1612, 5207, 9302, 5301, 5224, 7801, 9313,
     6304, 2003, 3425, 3423, 608,  1109,30319, 8903, 9317, 9307,
     4050,  5028, 4100, 3428,  310, 2102, 3402, 5319, 6512,
     11005, 30316, 2106, 8007, 6601, 2333, 3610, 2015, 5022,
     1394, 2202, 1401, 6005, 1614, 1388, 9311, 3604, 4701,
    30325, 6106, 2003, 7190, 9322, 1384, 3412, 2342, 1374, 2210,
     2332, 2901, 7200, 7824, 3410, 2013, 1510, 8306, 3414, 2005
};
```

### Level-Based Selection

Card level affects which items can be selected:

```c
// act_obj.c:3706-3709
if( obj->level <=50 )
    object = number_range(obj->level, obj->level + 100);
else
    object = number_range(75, 150);
```

| Card Level | Item Range |
|------------|------------|
| 1-50 | level to level+100 |
| 51-100 | 75-150 (harder items) |

## Completing Quest Cards

### Check Status

```
complete <card>
```

Shows which items are still needed:

```
You still need to find the following:
     A short sword.
     A leather vest.
     ...
```

### Submit Item

```
complete <card> <item>
```

**Function:** `do_complete()` in act_obj.c:3736-3883

**Process:**
1. Validates card ownership (must match `questowner` or be `QUEST_ARTIFACT`)
2. Checks if card is already complete (all slots = -1)
3. Matches item by `short_descr` against required VNUMs
4. If match found:
   - Sets slot value to -1 (completed)
   - Extracts (destroys) the submitted item
   - Reports remaining items needed

**Restrictions:**
- Cannot use protoplasm (VNUM 30037)
- Cannot use VNUM 30041
- Items are consumed on successful match

### Completion States

Slots use -1 to indicate completion:

```c
// act_obj.c:3771-3774
if (qobj->value[0] == -1) count += 1;
if (qobj->value[1] == -1) count += 1;
if (qobj->value[2] == -1) count += 1;
if (qobj->value[3] == -1) count += 1;
```

| Slots Complete | Message |
|----------------|---------|
| 0 | "requires three more objects" |
| 1 | "requires two more objects" |
| 2 | "requires one more object" |
| 3 | "has been completed!" |
| 4 | Card fully done |

## Data Structures

### Quest Card Object

| Field | Purpose |
|-------|---------|
| `item_type` | ITEM_QUESTCARD (34) |
| `level` | Card difficulty (1-100) |
| `value[0]` | Required item 1 VNUM (-1 if complete) |
| `value[1]` | Required item 2 VNUM (-1 if complete) |
| `value[2]` | Required item 3 VNUM (-1 if complete) |
| `value[3]` | Required item 4 VNUM (-1 if complete) |
| `questmaker` | Player who created card |
| `questowner` | Player who owns card |

### Related Constants (merc.h)

```c
#define OBJ_VNUM_QUESTCARD   30039   // Quest card template
#define ITEM_QUESTCARD       34      // Quest card item type
```

## Gameplay Flow

```
1. Earn primal energy (practice points)
         |
         v
2. cast 'quest' - Creates card, consumes primal
         |
         v
3. complete <card> - View required items
         |
         v
4. Hunt items across game areas
         |
         v
5. complete <card> <item> - Submit each item
         |
         v
6. Repeat until all 4 slots complete
         |
         v
7. Turn in completed card for rewards
```

## Source Files

| File | Lines | Function |
|------|-------|----------|
| [magic.c](../../src/combat/magic.c) | 4439-4468 | `spell_quest()` |
| [act_obj.c](../../src/commands/act_obj.c) | 3677-3734 | `quest_object()` |
| [act_obj.c](../../src/commands/act_obj.c) | 3736-3883 | `do_complete()` |
| [merc.h](../../src/core/merc.h) | 1276 | OBJ_VNUM_QUESTCARD |
| [merc.h](../../src/core/merc.h) | 1331 | ITEM_QUESTCARD |
