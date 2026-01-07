# Metal Forging

Metal slabs can be forged onto weapons and armor to add permanent hitroll and damroll bonuses.

## Available Metals

| Metal | VNUM | Hitroll | Damroll | Drop Rate |
|-------|------|---------|---------|-----------|
| Copper | 30049 | +3 | +3 | 4% |
| Iron | 30050 | +6 | +6 | 2% |
| Steel | 30051 | +9 | +9 | 1% |
| Adamantite | 30052 | +12 | +12 | 0.4% |

## Usage

```
forge copper <item>
forge iron <item>
forge steel <item>
forge adamantite <item>
```

## Restrictions

- **One metal per item** - An item can only have one metal type forged onto it
- **Cannot replace** - Once forged, the metal cannot be changed or removed
- **Works on weapons and armor** - Both item types accept metal forging

## Technical Details

### Item Types

```c
#define ITEM_COPPER      41
#define ITEM_IRON        42
#define ITEM_STEEL       43
#define ITEM_ADAMANTITE  44
```

### Tracking Flags

```c
#define SITEM_COPPER      (A)
#define SITEM_IRON        (B)
#define SITEM_STEEL       (C)
#define SITEM_ADAMANTITE  (D)
```

### Effect Application

Metals use `forge_affect()` to add permanent affects:

```c
void forge_affect(OBJ_DATA *obj, int value)
{
    AFFECT_DATA paf;

    // Add hitroll bonus
    paf.type      = 0;
    paf.duration  = -1;  // Permanent
    paf.location  = APPLY_HITROLL;
    paf.modifier  = value;
    paf.bitvector = 0;
    affect_to_obj(obj, &paf);

    // Add damroll bonus
    paf.location  = APPLY_DAMROLL;
    affect_to_obj(obj, &paf);
}
```

### Validation Check

```c
if (IS_SET(obj->spectype, SITEM_COPPER) ||
    IS_SET(obj->spectype, SITEM_IRON) ||
    IS_SET(obj->spectype, SITEM_STEEL) ||
    IS_SET(obj->spectype, SITEM_ADAMANTITE))
{
    send_to_char("That item is already forged.\n\r", ch);
    return;
}
```

## Drop Rates

Metals drop randomly from mobs via `create_mobile()` in db.c with tiered rarity:

| Metal | Drop Rate |
|-------|-----------|
| Copper | 4% |
| Iron | 2% |
| Steel | 1% |
| Adamantite | 0.4% |

Better metals are rarer to find, reflecting their superior bonuses.

## Source Files

- Command implementation: `game/src/commands/kav_wiz.c` (lines 54-153)
- Effect function: `game/src/systems/jobo_util.c` (lines 530-547)
- Object definitions: `gamedata/area/kavir.are` (VNUMs 30049-30052)
