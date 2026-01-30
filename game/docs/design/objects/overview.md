# Object Manipulation System Overview

Objects exist in one of three locations: a room, a character's inventory, or inside another object (container). All transfers between these states go through core handler functions that maintain weight tracking, carry limits, and linked list integrity. Equipment uses 27 wear slots with class, alignment, and form gating.

**Location:** [act_obj.c](../../src/commands/act_obj.c)

## Object Data Structures

### OBJ_DATA (Runtime Instance)

**Location:** [merc.h:2456-2500](../../src/core/merc.h#L2456-L2500)

Key fields:

| Field | Purpose |
|-------|---------|
| `carried_by` | Character holding it (mutually exclusive with `in_room`, `in_obj`) |
| `in_room` | Room it's on the ground in |
| `in_obj` | Container it's inside |
| `wear_loc` | Equipment slot (`WEAR_NONE` = inventory) |
| `item_type` | `ITEM_*` type constant |
| `extra_flags` | `ITEM_GLOW`, `ITEM_NODROP`, etc. |
| `wear_flags` | `ITEM_WEAR_*` bitmask of equippable slots |
| `value[4]` | Type-dependent values (charges, capacity, spell, damage, etc.) |
| `weight` | Base weight |
| `timer` | Decay countdown (corpses, food) |
| `questowner` | Owner-locked name string |
| `questmaker` | Creator name |
| `points` | Quest point value (refunded on sacrifice) |
| `chobj` | Character possessing this object (living object) |

### OBJ_INDEX_DATA (Template)

**Location:** [merc.h:2414-2451](../../src/core/merc.h#L2414-L2451)

The prototype that instances are created from. Same fields as OBJ_DATA plus `count` (how many instances exist) and `area` (zone reference).

## Item Types

**Location:** [merc.h:1263-1306](../../src/core/merc.h#L1263-L1306)

| Type | ID | Usage |
|------|----|-------|
| `ITEM_LIGHT` | 1 | Light sources; increments room light on equip |
| `ITEM_SCROLL` | 2 | Cast up to 3 spells via `recite` |
| `ITEM_WAND` | 3 | Single-target spell with charges via `zap` |
| `ITEM_STAFF` | 4 | Area-effect spell with charges via `brandish` |
| `ITEM_WEAPON` | 5 | Melee weapon; damage in `value[]` |
| `ITEM_TREASURE` | 8 | Valuables |
| `ITEM_ARMOR` | 9 | Wearable gear; AC in `value[3]` |
| `ITEM_POTION` | 10 | Consumable via `quaff`; up to 3 spells |
| `ITEM_FURNITURE` | 12 | Scene objects |
| `ITEM_CONTAINER` | 15 | Holds other objects; weight capacity in `value[0]` |
| `ITEM_DRINK_CON` | 17 | Liquid container; fillable from fountains |
| `ITEM_KEY` | 18 | Opens locks (vnum-matched) |
| `ITEM_FOOD` | 19 | Consumable via `eat` |
| `ITEM_MONEY` | 20 | Auto-converts to gold on pickup |
| `ITEM_CORPSE_NPC` | 23 | Dead mob; acts as container for loot |
| `ITEM_CORPSE_PC` | 24 | Dead player; player-restricted access |
| `ITEM_FOUNTAIN` | 25 | Infinite liquid source for `fill` |
| `ITEM_PILL` | 26 | Consumable like potions |
| `ITEM_PORTAL` | 27 | Teleportation; see [Movement System](../movement/overview.md) |
| `ITEM_VOODOO` | 29 | Voodoo doll mechanics |
| `ITEM_STAKE` | 30 | Vampire staking weapon |
| `ITEM_MISSILE` | 31 | Ranged ammo |
| `ITEM_AMMO` | 32 | Damage ammo with min/max |
| `ITEM_QUEST` | 33 | Quest items; cannot be sacrificed |
| `ITEM_BOOK` | 37 | Readable; pages via `turn` |
| `ITEM_TOOL` | 39 | Crafting tools |
| `ITEM_WALL` | 40 | Blocks exits; see [Movement System](../movement/overview.md) |
| `ITEM_COPPER`-`ADAMANTITE` | 41-44 | Crafting materials |
| `ITEM_GEMSTONE` | 45 | Crafting component |
| `ITEM_HILT` | 46 | Weapon crafting part |
| `ITEM_WGATE` | 52 | Ward gate portal |

## Item Flags

### Extra Flags

**Location:** [merc.h:1339-1416](../../src/core/merc.h#L1339-L1416)

| Flag | Value | Effect |
|------|-------|--------|
| `ITEM_GLOW` | 1 | Cosmetic glowing aura |
| `ITEM_HUM` | 2 | Cosmetic humming |
| `ITEM_THROWN` | 4 | Throwable weapon |
| `ITEM_KEEP` | 8 | Retained on death |
| `ITEM_VANISH` | 16 | Disintegrates on timer |
| `ITEM_INVIS` | 32 | Invisible |
| `ITEM_MAGIC` | 64 | Magical aura |
| `ITEM_NODROP` | 128 | Cannot be dropped or given |
| `ITEM_BLESS` | 256 | Holy/blessed |
| `ITEM_ANTI_GOOD` | 512 | Rejects good alignment on equip |
| `ITEM_ANTI_EVIL` | 1024 | Rejects evil alignment on equip |
| `ITEM_ANTI_NEUTRAL` | 2048 | Rejects neutral alignment on equip |
| `ITEM_NOREMOVE` | 4096 | Cannot be removed once worn |
| `ITEM_INVENTORY` | 8192 | Quest-bound |
| `ITEM_LOYAL` | 16384 | Owner-locked (rejects non-owner on wield) |
| `ITEM_SHADOWPLANE` | 32768 | Exists on shadow plane only |

### Special Item Flags

Class-restricted equipment flags:

| Flag | Class |
|------|-------|
| `SITEM_MAGE` | Mage only |
| `SITEM_DROW` | Drow only |
| `SITEM_WOLFWEAPON` | Werewolf (wieldable in non-wolf form) |
| `SITEM_UNIQUE` | Only one can exist |
| `SITEM_INDESTRUCTABLE` | Cannot be destroyed |
| `SITEM_FLYING` | Grants flight |

### Container Flags

**Location:** [merc.h:1526-1529](../../src/core/merc.h#L1526-L1529)

| Flag | Value | Effect |
|------|-------|--------|
| `CONT_CLOSEABLE` | 1 | Can be opened/closed |
| `CONT_PICKPROOF` | 2 | Cannot be lockpicked |
| `CONT_CLOSED` | 4 | Currently closed (blocks access) |
| `CONT_LOCKED` | 8 | Currently locked |

Container `value[]`: `[0]` = max weight capacity, `[1]` = flags, `[2]` = key vnum.

## Wear Location System

**Location:** [merc.h:1664-1691](../../src/core/merc.h#L1664-L1691)

27 equipment slots:

| Slot | ID | Wear Flag | Dual? |
|------|----|-----------|-------|
| `WEAR_LIGHT` | 0 | — | No |
| `WEAR_FINGER_L` | 1 | `ITEM_WEAR_FINGER` | Yes → `WEAR_FINGER_R` |
| `WEAR_FINGER_R` | 2 | `ITEM_WEAR_FINGER` | Yes |
| `WEAR_NECK_1` | 3 | `ITEM_WEAR_NECK` | Yes → `WEAR_NECK_2` |
| `WEAR_NECK_2` | 4 | `ITEM_WEAR_NECK` | Yes |
| `WEAR_BODY` | 5 | `ITEM_WEAR_BODY` | No |
| `WEAR_HEAD` | 6 | `ITEM_WEAR_HEAD` | No |
| `WEAR_LEGS` | 7 | `ITEM_WEAR_LEGS` | No |
| `WEAR_FEET` | 8 | `ITEM_WEAR_FEET` | No |
| `WEAR_HANDS` | 9 | `ITEM_WEAR_HANDS` | No |
| `WEAR_ARMS` | 10 | `ITEM_WEAR_ARMS` | No |
| `WEAR_SHIELD` | 11 | `ITEM_WEAR_SHIELD` | No |
| `WEAR_ABOUT` | 12 | `ITEM_WEAR_ABOUT` | No |
| `WEAR_WAIST` | 13 | `ITEM_WEAR_WAIST` | No |
| `WEAR_WRIST_L` | 14 | `ITEM_WEAR_WRIST` | Yes → `WEAR_WRIST_R` |
| `WEAR_WRIST_R` | 15 | `ITEM_WEAR_WRIST` | Yes |
| `WEAR_WIELD` | 16 | `ITEM_WIELD` | No |
| `WEAR_HOLD` | 17 | `ITEM_HOLD` | No |
| `WEAR_THIRD` | 18 | — | No |
| `WEAR_FOURTH` | 19 | — | No |
| `WEAR_FACE` | 20 | `ITEM_WEAR_FACE` | No |
| `WEAR_SCABBARD_L` | 21 | — | No (Samurai) |
| `WEAR_SCABBARD_R` | 22 | — | No (Samurai) |
| `WEAR_SPECIAL` | 23 | `ITEM_WEAR_SPECIAL` | No |
| `WEAR_FLOAT` | 24 | `ITEM_WEAR_FLOAT` | No |
| `WEAR_MEDAL` | 25 | `ITEM_WEAR_MEDAL` | No |
| `WEAR_BODYART` | 26 | `ITEM_WEAR_BODYART` | No |

### Slot Filling Priority

`wear_obj()` at [act_obj.c:1184](../../src/commands/act_obj.c#L1184) selects the slot:

1. **Weapons** — try `WEAR_WIELD` → `WEAR_HOLD` → `WEAR_THIRD` → `WEAR_FOURTH` → `WEAR_LIGHT`
2. **Dual-slot items** (finger, neck, wrist) — try left slot → right slot
3. **Single-slot armor** — check if slot is empty; if occupied and `fReplace=TRUE`, unequip existing item first

Monks cannot equip weapons to `WEAR_WIELD` or `WEAR_HOLD` — their hands must stay free for combat.

## Equipment Functions

### equip_char()

**Location:** [handler.c:926](../../src/core/handler.c#L926)

Called when equipping an item to a slot:

1. **Alignment check** — `ITEM_ANTI_GOOD/EVIL/NEUTRAL` rejects and drops the item
2. **Class check** — `SITEM_MAGE`, `SITEM_DROW`, class-specific vnum ranges reject non-matching classes (item zaps and drops)
3. **Apply affects** — stat modifiers from `obj->affected[]`
4. **AC update** — `apply_ac()` from `value[3]`
5. **Light source** — increment `room->light` if `ITEM_LIGHT`
6. **Power script** — execute `obj->chpoweron` flavor text
7. Set `obj->wear_loc = iWear`

### unequip_char()

**Location:** [handler.c:1386](../../src/core/handler.c#L1386)

Reverse of equip: removes affects, restores AC, decrements room light, runs `chpoweroff` script, sets `wear_loc = WEAR_NONE`.

## Core Transfer Functions

All in [handler.c](../../src/core/handler.c). An object is always in exactly one of three states: `carried_by` (inventory), `in_room` (ground), or `in_obj` (container).

| Function | Transfer | Key Side Effects |
|----------|----------|------------------|
| `obj_to_char()` | → inventory | `carry_number++`, `carry_weight += weight` |
| `obj_from_char()` | inventory → | `carry_number--`, `carry_weight -= weight`; auto-unequips if worn |
| `obj_to_room()` | → ground | Added to `room->contents` |
| `obj_from_room()` | ground → | Removed from `room->contents` |
| `obj_to_obj()` | → container | Propagates weight to all ancestor carriers |
| `obj_from_obj()` | container → | Propagates weight reduction to ancestors |

## Commands

### Inventory Transfer

| Command | Aliases | Function | Position | Transfer | Notes |
|---------|---------|----------|----------|----------|-------|
| `get` | `take` | [do_get](../../src/commands/act_obj.c#L331) | Sitting | Room/Container → Char | Blocked by `AFF_ETHEREAL`; money auto-converts to gold |
| `drop` | | [do_drop](../../src/commands/act_obj.c#L582) | Sitting | Char → Room | Blocked by `ITEM_NODROP`; auto-sets `ITEM_SHADOWPLANE` if on shadow plane |
| `put` | | [do_put](../../src/commands/act_obj.c#L468) | Sitting | Char → Container | Weight capacity check; no artifacts in containers |
| `give` | | [do_give](../../src/commands/act_obj.c#L650) | Sitting | Char → Char | `LOG_ALWAYS`; 6 pulse wait state; recipient capacity check |
| `sacrifice` | `junk` | [do_sacrifice](../../src/commands/act_obj.c#L1724) | Sitting | Char → Void | Refunds quest points; blocks quest items, artifacts, money |

### Equipment

| Command | Aliases | Function | Position | Notes |
|---------|---------|----------|----------|-------|
| `wear` | `wield`, `hold` | [do_wear](../../src/commands/act_obj.c#L1617) | Sitting | Form/class/alignment gating; auto-replaces occupied slot |
| `remove` | | [do_remove](../../src/commands/act_obj.c#L1664) | Sitting | Blocked by `ITEM_NOREMOVE` |

### Consumption

| Command | Function | Position | Item Type | Effect |
|---------|----------|----------|-----------|--------|
| `eat` | [do_eat](../../src/commands/act_obj.c#L1018) | Sitting | `ITEM_FOOD`, `ITEM_PILL` | Applies spells from `value[1-3]`; destroys item |
| `drink` | [do_drink](../../src/commands/act_obj.c#L791) | Sitting | `ITEM_DRINK_CON`, `ITEM_FOUNTAIN` | Liquid effects (drunk, full, thirst); vampires can only drink blood |
| `fill` | [do_fill](../../src/commands/act_obj.c#L718) | Sitting | `ITEM_DRINK_CON` | Fills from `ITEM_FOUNTAIN` in room; liquid type must match |
| `quaff` | [do_quaff](../../src/commands/act_obj.c#L1809) | Sitting | `ITEM_POTION` | Casts up to 3 spells at `value[0]` level; destroys item |

### Spell Items

| Command | Function | Position | Item Type | Targeting | Charges |
|---------|----------|----------|-----------|-----------|---------|
| `recite` | [do_recite](../../src/commands/act_obj.c#L1853) | Sitting | `ITEM_SCROLL` | Single target (char or obj) | 1 use (destroyed) |
| `brandish` | [do_brandish](../../src/commands/act_obj.c#L1906) | Sitting | `ITEM_STAFF` | Area (all in room) | `value[2]` charges; destroyed when depleted |
| `zap` | [do_zap](../../src/commands/act_obj.c#L1992) | Sitting | `ITEM_WAND` | Single target | `value[2]` charges; destroyed when depleted |

Spell items use `value[0]` for caster level, `value[1-3]` for spell indices, and `value[2]` for remaining charges (wands/staves).

## Special Mechanics

### Shadow Plane Sync

Objects have an `ITEM_SHADOWPLANE` flag. Characters on the shadow plane can only interact with shadow plane items, and vice versa. `do_drop()` auto-sets the flag based on the character's plane, and `get_obj()` auto-removes it on pickup.

### Ethereal State

`AFF_ETHEREAL` blocks most physical object interactions: `get`, `fill`, `give` (to ethereal recipients). Ethereal characters can only drink from held items.

### Owner-Locked Weapons

Items with `ITEM_LOYAL` or specific vnums check `obj->questowner` on equip. Non-owners get a "leaps out of your hand" rejection.

### Artifact Protection

`QUEST_ARTIFACT` items cannot be:
- Picked up by NPCs
- Put in containers
- Given to NPCs
- Sacrificed

### Form Restrictions

Shape-shifted characters (`IS_POLYMORPH`) are blocked from wearing equipment, with exceptions for Angel, Werewolf, Demon, and high-level Droid forms. Vampire `ZULOFORM` requires `DISC_VAMP_OBEA >= 9` to wear items.

### Monk Hand Restriction

Monks cannot equip items to `WEAR_WIELD` or `WEAR_HOLD` — their hands must stay free for martial combat.

### Player Corpses

`ITEM_CORPSE_PC` containers restrict access to player characters only (and immortals). The owner name is extracted from the corpse's short description.

## Source Files

| File | Contents |
|------|----------|
| [act_obj.c](../../src/commands/act_obj.c) | All object commands: get, put, drop, give, wear, remove, eat, drink, fill, quaff, recite, brandish, zap, sacrifice |
| [handler.c](../../src/core/handler.c) | Core transfer functions: `obj_to_char`, `obj_from_char`, `obj_to_room`, `obj_from_room`, `obj_to_obj`, `obj_from_obj`, `equip_char`, `unequip_char`, `get_eq_char` |
| [merc.h:1263-1306](../../src/core/merc.h#L1263-L1306) | `ITEM_*` type constants |
| [merc.h:1339-1416](../../src/core/merc.h#L1339-L1416) | Item flags and wear capability flags |
| [merc.h:1526-1529](../../src/core/merc.h#L1526-L1529) | Container flags |
| [merc.h:1664-1691](../../src/core/merc.h#L1664-L1691) | `WEAR_*` slot constants |
| [merc.h:2414-2500](../../src/core/merc.h#L2414-L2500) | `OBJ_INDEX_DATA` and `OBJ_DATA` structures |
