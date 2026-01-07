# Hilt Forging

Hilts can be forged onto weapons to add spell effects on hit. Hilts only work on weapons, not armor.

## Available Hilts

| Hilt | VNUM | Spell Effect |
|------|------|--------------|
| Ivory | 30063 | Curse |
| Ebony | 30064 | Blind |
| Crystal | 30066 | Dispel Evil |
| Marble | 30067 | Drain |
| Gold | 30068 | Lightning |
| Bronze | 30069 | Acid |
| Sandstone | 30070 | Fire |
| Limestone | 30071 | Poison |

## Usage

```
forge ivory <weapon>
forge ebony <weapon>
forge crystal <weapon>
forge marble <weapon>
forge gold <weapon>
forge bronze <weapon>
forge sandstone <weapon>
forge limestone <weapon>
```

## Restrictions

- **Weapons only** - Hilts cannot be forged onto armor
- **One hilt per weapon** - A weapon can only have one hilt
- **Cannot replace** - Once forged, the hilt cannot be changed

## Spell Effects in Combat

When attacking with a hilt-forged weapon, the spell triggers on hit:

| Hilt | Combat Effect |
|------|---------------|
| Ivory | Curses the target |
| Ebony | Blinds the target |
| Crystal | Dispels evil from target |
| Marble | Drains life from target |
| Gold | Lightning damage |
| Bronze | Acid damage |
| Sandstone | Fire damage |
| Limestone | Poisons the target |

## Technical Details

### Item Type

```c
#define ITEM_HILT    46
```

### Tracking Flag

```c
#define SITEM_HILT   (H)
```

### Effect Application

Hilts use a spell encoding system. The hilt's spell ID is added to weapon `value[0]`:

```c
obj->value[0] += hilt_spell_id;
```

The spell is extracted during combat via:
```c
sn = wield->value[0] - ((wield->value[0] / 1000) * 1000);
```

This extracts the lower digits (hilt spell) while ignoring the thousands digit (gem encoding).

### Hilt Spell Encodings

| Hilt | Spell ID |
|------|----------|
| Bronze | 1 |
| Ebony | 4 |
| Ivory | 24 |
| Crystal | 30 |
| Marble | 34 |
| Sandstone | 36 |
| Gold | 48 |
| Limestone | 53 |

### Validation Checks

```c
// Check for existing hilt
if (IS_SET(obj->spectype, SITEM_HILT))
{
    send_to_char("That item is already forged with a hilt.\n\r", ch);
    return;
}

// Check item is a weapon
if (obj->item_type != ITEM_WEAPON)
{
    send_to_char("You can only add hilts to weapons.\n\r", ch);
    return;
}
```

## Source Files

- Command implementation: `game/src/commands/kav_wiz.c` (lines 478-709)
- Spell trigger: `game/src/combat/fight.c` (lines 574-582)
- Object definitions: `gamedata/area/kavir.are` (VNUMs 30063-30064, 30066-30071)
