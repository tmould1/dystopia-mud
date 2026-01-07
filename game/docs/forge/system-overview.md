# Forge System Overview

This document covers the technical implementation of the forge system.

## Data Structures

### Object Tracking

Forged modifications are tracked using the `spectype` bitfield on `OBJ_DATA`:

```c
// In merc.h
#define SITEM_COPPER        (A)   // Copper forged
#define SITEM_IRON          (B)   // Iron forged
#define SITEM_STEEL         (C)   // Steel forged
#define SITEM_ADAMANTITE    (D)   // Adamantite forged
#define SITEM_GEMSTONE      (G)   // Gemstone forged
#define SITEM_HILT          (H)   // Hilt forged
```

Use `IS_SET(obj->spectype, SITEM_*)` to check if a modification is applied.

### Item Types

Forging materials use dedicated item types:

```c
#define ITEM_COPPER      41   // Copper slab
#define ITEM_IRON        42   // Iron slab
#define ITEM_STEEL       43   // Steel slab
#define ITEM_ADAMANTITE  44   // Adamantite slab
#define ITEM_GEMSTONE    45   // Any gemstone
#define ITEM_HILT        46   // Any hilt
```

### Effect Storage

| Material Type | Effect Storage | Details |
|--------------|----------------|---------|
| Metals | `AFFECT_DATA` list | `forge_affect()` adds APPLY_HITROLL and APPLY_DAMROLL |
| Gems | `value[0]` (weapons) | Magic number encoding: `gem_value * 1000` |
| Gems | `value[3]` (armor) | Direct numeric value |
| Hilts | `value[0]` | Direct addition to weapon value |

## Command Flow

The `do_forge()` function in `kav_wiz.c` handles all forging:

```
1. Parse arguments: forge <material> <target_item>
2. Validate player has both items in inventory
3. Check material's item_type matches expected type
4. Check target item doesn't already have that modification type
5. Apply modification:
   - SET_BIT(obj->spectype, SITEM_*) to mark as forged
   - Apply stat bonuses or value modifications
6. extract_obj(material) - consume the forging material
7. Send feedback via act() to player and room
```

## Metal Forging

Metals add permanent affects via `forge_affect()`:

```c
void forge_affect(OBJ_DATA *obj, int value)
{
    AFFECT_DATA paf;

    paf.type      = 0;
    paf.duration  = -1;  // Permanent
    paf.location  = APPLY_HITROLL;
    paf.modifier  = value;
    paf.bitvector = 0;
    affect_to_obj(obj, &paf);

    paf.location  = APPLY_DAMROLL;
    paf.modifier  = value;
    affect_to_obj(obj, &paf);
}
```

## Gem Forging

Gems use a magic number encoding in `value[0]` for weapons:

```c
// Strip old gem effect (values >= 1000 indicate gem)
if (obj->value[0] >= 1000)
    obj->value[0] -= ((obj->value[0] / 1000) * 1000);

// Add new gem effect
obj->value[0] += gem_value * 1000;  // e.g., diamond = 8000
```

For armor, gems set `value[3]` directly to the gem's power level.

## Hilt Forging

Hilts add directly to weapon `value[0]`:

```c
obj->value[0] += hilt_value;  // Range: 1-53
```

Hilts can only be applied to `ITEM_WEAPON` types.

## Material Distribution

Random mob spawns in `create_mobile()` (db.c):

```c
if (number_percent() < 2)  // 2% chance
{
    obj = create_object(get_obj_index(number_range(30049, 30071)), 0);

    // Adamantite has extra rarity check
    if (obj->pIndexData->vnum == 30052) {
        if (number_range(1,5) == 2)  // 20% of 2% = 0.4%
            obj_to_char(obj, mob);
        else
            extract_obj(obj);
    }
    else
        obj_to_char(obj, mob);
}
```

## Validation Rules

1. **Metal exclusivity**: Only one metal type per item
   - Checked via: `IS_SET(obj->spectype, SITEM_COPPER | SITEM_IRON | SITEM_STEEL | SITEM_ADAMANTITE)`

2. **Gem exclusivity**: Only one gem per item
   - Checked via: `IS_SET(obj->spectype, SITEM_GEMSTONE)`

3. **Hilt restrictions**:
   - Only one hilt per item: `IS_SET(obj->spectype, SITEM_HILT)`
   - Weapons only: `obj->item_type == ITEM_WEAPON`

## Source Files

| File | Lines | Purpose |
|------|-------|---------|
| `game/src/commands/kav_wiz.c` | 32-712 | `do_forge()` command |
| `game/src/systems/jobo_util.c` | 530-547 | `forge_affect()` helper |
| `game/src/core/db.c` | 2223-2239 | Random material distribution |
| `game/src/core/merc.h` | 1338-1343 | Item type definitions |
| `gamedata/area/kavir.are` | 3100-3478 | Material object definitions |
