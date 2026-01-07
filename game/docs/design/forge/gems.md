# Gemstone Forging

Gemstones can be forged onto weapons and armor to add spell effects. Forging a gem replaces any existing spell effect on the item.

## Available Gems

| Gem | VNUM | Spell Effect |
|-----|------|--------------|
| Diamond | 30053 | Sanctuary |
| Emerald | 30054 | Acid Shield |
| Sapphire | 30055 | Ice Shield |
| Ruby | 30056 | Fire Shield |
| Topaz | 30059 | Lightning Shield |
| Jade | 30057 | Protection |
| Pearl | 30058 | Invisibility |
| Amethyst | 30060 | Fly |
| Onyx | 30061 | Sneak |
| Opal | 30062 | Pass Door |
| Lazuli | 30065 | Detect Invis |

## Usage

```
forge diamond <item>
forge emerald <item>
forge sapphire <item>
forge ruby <item>
forge jade <item>
forge pearl <item>
forge topaz <item>
forge amethyst <item>
forge onyx <item>
forge opal <item>
forge lazuli <item>
```

## Restrictions

- **One gem per item** - An item can only have one gemstone
- **Replaces existing spell** - Forging a gem overwrites any previous gem effect
- **Works on weapons and armor** - Both item types accept gem forging

## Special Cases

### Jade and Alignment

Jade adapts its protection based on the forger's alignment:
- **Good/Neutral characters**: Protection from evil (spell ID 7)
- **Evil characters**: Protection from good (spell ID 139)

This ensures the wearer gets protection against their opposing alignment.

## Technical Details

### Item Type

```c
#define ITEM_GEMSTONE    45
```

### Tracking Flag

```c
#define SITEM_GEMSTONE   (G)
```

### Effect Application

Gems use a spell encoding system in `value[0]` (weapons) and `value[3]` (armor).

For weapons, the encoding stores the spell ID in the thousands digit:
- `value[0] / 1000` extracts the gem's spell ID
- `value[0] % 1000` preserves any hilt spell in the lower digits

```c
// Strip old gem effect (values >= 1000 indicate a gem)
if (obj->value[0] >= 1000)
    obj->value[0] -= ((obj->value[0] / 1000) * 1000);

// Add new gem encoding (e.g., diamond = 8000, emerald = 13000)
obj->value[0] += gem_encoding;
```

For armor, gems set `value[3]` directly to the spell encoding:

```c
obj->value[3] = gem_spell_id;  // e.g., diamond = 8
```

### Gem Spell Encodings

| Gem | Encoding | Spell ID |
|-----|----------|----------|
| Diamond | 8000 | 8 |
| Emerald | 13000 | 13 |
| Sapphire | 12000 | 12 |
| Ruby | 11000 | 11 |
| Topaz | 10000 | 10 |
| Onyx | 9000 | 9 |
| Jade | 7000 | 7 (139 for evil) |
| Opal | 6000 | 6 |
| Pearl | 5000 | 5 |
| Amethyst | 3000 | 3 |
| Lazuli | 2000 | 2 |

### Validation Check

```c
if (IS_SET(obj->spectype, SITEM_GEMSTONE))
{
    send_to_char("That item is already forged with a gemstone.\n\r", ch);
    return;
}
```

## Source Files

- Command implementation: `game/src/commands/kav_wiz.c` (lines 154-477)
- Spell application: `game/src/combat/fight.c` (line 577)
- Object definitions: `gamedata/area/kavir.are` (VNUMs 30053-30062, 30065)
