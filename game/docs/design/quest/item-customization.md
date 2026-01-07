# Quest Item Customization

The `quest` command allows players to spend quest points (QP) to create and customize equipment.

## Command Syntax

```
quest                              Display cost menu
quest create <type> [subtype]      Create new object
quest <item> <property> <value>    Modify existing item
```

## Creating Objects

Create objects from protoplasm (VNUM 30037). Requires `EXTRA_TRUSTED` flag.

```
quest create <type> [subtype]
```

### Object Types

| Type | Cost | Default Properties |
|------|------|-------------------|
| Light | 10 QP | Infinite duration (`value[2] = -1`) |
| Weapon | 50 QP | 20-30 damage, level 50, requires damage type |
| Armor | 20 QP | 15 AC (`value[0] = 15`) |
| Container | 10 QP | 999 capacity (`value[0] = 999`) |
| Boat | 10 QP | Standard boat |
| Fountain | 10 QP | 1000 capacity, requires liquid type |
| Stake | 10 QP | Vampire-slaying stake |

### Weapon Damage Types

Required when creating weapons:

| Type | Value | Cost |
|------|-------|------|
| Slice | 1 | Standard |
| Stab | 2 | Standard |
| Slash | 3 | Standard |
| Whip | 4 | Standard |
| Claw | 5 | Standard |
| Blast | 6 | Standard |
| Pound | 7 | Standard |
| Crush | 8 | Standard |
| Pierce | 11 | Standard |
| Suck | 12 | Standard |
| Grep | 9 | 1000 QP |
| Bite | 10 | 1000 QP |

### Fountain Liquid Types

| Liquid | Value |
|--------|-------|
| Water | 0 |
| Beer | 1 |
| Wine | 2 |
| Ale | 3 |
| Darkale | 4 |
| Whisky | 5 |
| Firebreather | 7 |
| Specialty | 8 |
| Slime | 9 |
| Milk | 10 |
| Tea | 11 |
| Coffee | 12 |
| Blood | 13 |
| Saltwater | 14 |

## Stat Modifications

Add permanent stat bonuses to items via `oset_affect()`.

```
quest <item> str|dex|int|wis|con <value>
quest <item> hp|mana|move <value>
quest <item> hitroll|damroll <value>
quest <item> ac <value>
```

### Base Limits

| Stat | Max | Cost |
|------|-----|------|
| Str | +3 | 20 QP per point |
| Dex | +3 | 20 QP per point |
| Int | +3 | 20 QP per point |
| Wis | +3 | 20 QP per point |
| Con | +3 | 20 QP per point |
| HP | +25 | 5 QP per point |
| Mana | +25 | 5 QP per point |
| Move | +25 | 5 QP per point |
| Hitroll | +5 | 30 QP per point |
| Damroll | +5 | 30 QP per point |
| AC | -25 | 10 QP per point |

### Limit Multipliers

Limits vary by item origin and type:

| Item Type | Multiplier |
|-----------|------------|
| Standard item | 1x base limits |
| Created item (protoplasm) | 2x base limits |
| Standard weapon | 2x base limits |
| Created weapon | 4x base limits |
| QUEST_IMPROVED item | Max 1500 total |

## Weapon Properties

### Min/Max Damage
```
quest <weapon> min <value>    Max: 20
quest <weapon> max <value>    Max: 30
```
Cost: 1 QP per point increase.

### Protection (Armor)
```
quest <armor> protection <value>    Max: 15
```
Cost: 1 QP per point.

### Weapon Type Change
```
quest <weapon> weapon <type>
```
Cost: 10 QP (1000 QP for grep/bite). Requires `EXTRA_TRUSTED`.

## Spell Effects

### Spell Weapons
Add elemental damage to weapons. Cost: 50 QP.

```
quest <weapon> spell <type>
quest <weapon> replacespell <type>    Replace existing spell
```

| Spell | Value | Effect |
|-------|-------|--------|
| Acid | 1 | Acid damage |
| Dark | 4 | Dark damage |
| Holy | 30 | Holy damage |
| Vampiric | 34 | Life drain |
| Flaming | 37 | Fire damage |
| Electrified | 48 | Lightning damage |
| Poisonous | 53 | Poison damage |

### Spell Affects
Add passive effects to weapons or armor. Cost: 50 QP.

```
quest <item> spell <affect>
quest <item> replacespell <affect>
```

| Affect | Value | Effect |
|--------|-------|--------|
| Infravision | 1 | See in dark |
| Seeinvis | 2 | Detect invisible |
| Fly | 3 | Flight |
| Blind | 4 | Blindness |
| Invis | 5 | Invisibility |
| Passdoor | 6 | Pass through doors |
| Protection_vs_evil | 7 | Protection vs evil |
| Protection_vs_good | 139 | Protection vs good |
| Sanct | 8 | Sanctuary |
| Sneak | 9 | Move silently |
| Shockshield | 10 | Lightning shield |
| Fireshield | 11 | Fire shield |
| Iceshield | 12 | Ice shield |
| Acidshield | 13 | Acid shield |

### Spell Power
```
quest <weapon> power <value>    Max: 50
```
Cost: 1 QP per power point. Stored in `obj->level`.

## Extra Flags

Toggle item flags. Cost varies by add/remove.

```
quest <item> extra <flag>
```

| Flag | Add Cost | Remove Cost |
|------|----------|-------------|
| Glow | 1 QP | 1 QP |
| Hum | 1 QP | 1 QP |
| Invis | 1 QP | 1 QP |
| Anti-good | 1 QP | 10 QP |
| Anti-evil | 1 QP | 10 QP |
| Anti-neutral | 1 QP | 10 QP |
| Loyal | 10 QP | 1 QP |
| Silver | 100 QP | Cannot remove |

## Wear Location

Change where item is worn. Cost: 20 QP. Requires `EXTRA_TRUSTED`.

```
quest <item> wear <location>
```

Locations: Finger, Neck, Body, Head, Legs, Feet, Hands, Arms, About, Waist, Wrist, Hold, Face

Note: Weapons can only use "Hold" location.

## Special Properties

### Transporter
Create a teleporter to current room. Cost: 50 QP.

```
quest <item> transporter
quest <item> retransporter    Update destination
```

Sets default messages:
- `chpoweron`: "You transform into a fine mist and seep into the ground."
- `victpoweron`: "$n transforms into a fine mist and seeps into the ground."
- `chpoweroff`: "You seep up from the ground and reform your body."
- `victpoweroff`: "A fine mist seeps up from the ground and reforms into $n."

### Weight
Remove item weight. Cost: 10 QP.

```
quest <item> weight
```
Sets `obj->weight = 1`.

### Naming
Rename item properties. Cost: 1 QP for all three.

```
quest <item> name <keywords>
quest <item> short <short description>
quest <item> long <room description>
```

### Custom Messages
Set custom activation messages (no QP cost after transporter):

```
quest <item> you-in <message>       Transporter arrival (to self)
quest <item> you-out <message>      Transporter departure (to self)
quest <item> other-in <message>     Transporter arrival (to room)
quest <item> other-out <message>    Transporter departure (to room)
quest <item> you-wear <message>     Wear message (to self)
quest <item> you-remove <message>   Remove message (to self)
quest <item> you-use <message>      Use message (to self)
quest <item> other-wear <message>   Wear message (to room)
quest <item> other-remove <message> Remove message (to room)
quest <item> other-use <message>    Use message (to room)
```

## Restrictions

### Items That Cannot Be Modified
- `ITEM_QUEST` (type 33)
- `ITEM_AMMO`
- `ITEM_EGG`
- `ITEM_VOODOO`
- `ITEM_MONEY`
- `ITEM_TREASURE`
- `ITEM_TOOL`
- `ITEM_SYMBOL`
- `ITEM_PAGE`
- Items with `QUEST_ARTIFACT` flag
- Items with `QUEST_PRIZE` flag

### Relic Restrictions
Items with `QUEST_RELIC` flag cannot have modified:
- Weapon type
- Extra flags
- Wear location
- Spell effects

### Ownership
- Players can only modify items they own (`obj->questowner`)
- Immortals (`IS_IMMORTAL`) bypass ownership check
- `questmaker` field tracks who last modified item

## Data Structures

### Object Fields
```c
char *questmaker;    // Player who created/modified item
char *questowner;    // Player who owns item
```

### Quest Flags (merc.h:1500-1527)
```c
#define QUEST_STR           1       // Has str bonus
#define QUEST_DEX           2       // Has dex bonus
#define QUEST_INT           4       // Has int bonus
#define QUEST_WIS           8       // Has wis bonus
#define QUEST_CON          16       // Has con bonus
#define QUEST_HITROLL      32       // Has hitroll bonus
#define QUEST_DAMROLL      64       // Has damroll bonus
#define QUEST_HIT         128       // Has hp bonus
#define QUEST_MANA        256       // Has mana bonus
#define QUEST_MOVE        512       // Has move bonus
#define QUEST_AC         1024       // Has AC bonus
#define QUEST_NAME       2048       // Name modified
#define QUEST_SHORT      4096       // Short desc modified
#define QUEST_LONG       8192       // Long desc modified
#define QUEST_FREENAME  16384       // Can rename freely
#define QUEST_ENCHANTED 32768       // Enchanted
#define QUEST_SPELLPROOF 65536      // Spell proof
#define QUEST_ARTIFACT  131072      // Artifact (cannot modify)
#define QUEST_IMPROVED  262144      // Improved limits (1500 max)
#define QUEST_PRIZE     524288      // Prize (cannot modify)
#define QUEST_RELIC    1048576      // Relic (restricted mods)
#define QUEST_BLOODA   2097152      // Blood artifact
#define QUEST_CLONED   4194304      // Cloned item
#define QUEST_ZOMBIE   8388608      // Zombie-related
#define QUEST_FORGE   16777216      // Forged item
```

## Source Files

| File | Lines | Function |
|------|-------|----------|
| [kav_wiz.c](../../src/commands/kav_wiz.c) | 1351-3674 | `do_quest()` |
| [kav_wiz.c](../../src/commands/kav_wiz.c) | 3675-3774 | `oset_affect()` |
| [merc.h](../../src/core/merc.h) | 1500-1527 | Quest flags |
| [merc.h](../../src/core/merc.h) | 1275 | OBJ_VNUM_PROTOPLASM (30037) |
