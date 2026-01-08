# OLC Editors

Each editor provides commands to modify game content. Enter an editor with its entry command, make changes, then type `done` to exit.

## Area Editor (aedit)

Entry: `aedit <vnum>` or `aedit create`
Source: [olc.c:704-757](../../src/world/olc.c#L704-L757)

Areas define zones containing rooms, objects, and mobiles within a VNUM range.

### Commands

| Command | Function | Description |
|---------|----------|-------------|
| show | aedit_show | Display area properties |
| create | aedit_create | Create new area |
| name | aedit_name | Set area name |
| filename | aedit_file | Set .are filename |
| age | aedit_age | Set repop timer |
| recall | aedit_recall | Set recall room VNUM |
| security | aedit_security | Set builder security level |
| builders | aedit_builder | Add/remove builder names |
| vnum | aedit_vnum | Set VNUM range (low high) |
| lvnum | aedit_lvnum | Set lower VNUM only |
| uvnum | aedit_uvnum | Set upper VNUM only |
| hidden | aedit_hidden | Toggle hidden from area list |
| reset | aedit_reset | Reset entire area |

Command table: [olc.c:228-251](../../src/world/olc.c#L228-L251)

### Data Structure

`AREA_DATA` in [merc.h](../../src/core/merc.h) contains:
- `name` - Area display name
- `filename` - File to save/load (.are)
- `age` / `nplayer` - Repop tracking
- `lvnum` / `uvnum` - VNUM range
- `area_flags` - Status flags (AREA_CHANGED, etc.)
- `security` - Required builder level
- `builders` - Space-separated builder names
- `recall` - Default recall room VNUM

---

## Room Editor (redit)

Entry: `redit [vnum]` (defaults to current room)
Source: [olc.c:762-800](../../src/world/olc.c#L762-L800)

Rooms are locations players can occupy, with exits, descriptions, and resets.

### Commands

| Command | Function | Description |
|---------|----------|-------------|
| show | redit_show | Display room properties |
| create | redit_create | Create new room |
| name | redit_name | Set room title |
| desc | redit_desc | Enter string editor for description |
| format | redit_format | Auto-format description text |
| ed | redit_ed | Edit extra descriptions |
| north | redit_north | Edit north exit |
| south | redit_south | Edit south exit |
| east | redit_east | Edit east exit |
| west | redit_west | Edit west exit |
| up | redit_up | Edit up exit |
| down | redit_down | Edit down exit |
| walk | redit_move | Move through exit while editing |
| mreset | redit_mreset | Add mobile reset (spawn) |
| oreset | redit_oreset | Add object reset (placement) |
| mlist | redit_mlist | List mobiles in area |
| olist | redit_olist | List objects in area |
| mshow | redit_mshow | Preview mobile details |
| oshow | redit_oshow | Preview object details |

Command table: [olc.c:255-287](../../src/world/olc.c#L255-L287)

### Exit Editing

Exit commands accept subcommands:
- `<dir> dig <vnum>` - Create exit to room (creates room if needed)
- `<dir> link <vnum>` - Create two-way exit
- `<dir> room <vnum>` - Change destination
- `<dir> key <vnum>` - Set key object VNUM
- `<dir> name <text>` - Set door keyword
- `<dir> desc` - Edit exit description
- `<dir> delete` - Remove exit
- `<dir> <flag>` - Toggle door flag (door, closed, locked, etc.)

### Data Structure

`ROOM_INDEX_DATA` in [merc.h](../../src/core/merc.h) contains:
- `vnum` - Virtual number (unique ID)
- `name` - Room title
- `description` - Long description
- `exit[6]` - Exits (N/E/S/W/U/D)
- `extra_descr` - Examinable items
- `room_flags` - Room attributes
- `sector_type` - Terrain type
- `roomtext` - RoomText triggers (see [roomtext.md](roomtext.md))

---

## Object Editor (oedit)

Entry: `oedit <vnum>` or `oedit create <vnum>`
Source: [olc.c:805-863](../../src/world/olc.c#L805-L863)

Objects are items players can interact with (weapons, armor, containers, etc.).

### Commands

| Command | Function | Description |
|---------|----------|-------------|
| show | oedit_show | Display object properties |
| create | oedit_create | Create new object |
| name | oedit_name | Set keywords (for get/drop) |
| short | oedit_short | Set short desc (in inventory) |
| long | oedit_long | Set long desc (on ground) |
| ed | oedit_ed | Edit extra descriptions |
| v0 | oedit_value0 | Set value[0] |
| v1 | oedit_value1 | Set value[1] |
| v2 | oedit_value2 | Set value[2] |
| v3 | oedit_value3 | Set value[3] |
| weight | oedit_weight | Set weight |
| cost | oedit_cost | Set gold value |
| addaffect | oedit_addaffect | Add stat modifier |
| delaffect | oedit_delaffect | Remove stat modifier |

Command table: [olc.c:291-315](../../src/world/olc.c#L291-L315)

### Value Fields by Type

Values (v0-v3) have different meanings per item type:

| Type | v0 | v1 | v2 | v3 |
|------|----|----|----|----|
| WEAPON | - | dice count | dice size | weapon type |
| ARMOR | AC pierce | AC bash | AC slash | AC exotic |
| CONTAINER | capacity | flags | key vnum | - |
| DRINK_CON | max amount | current | liquid type | poisoned |
| FOOD | hours filled | - | - | poisoned |
| MONEY | gold amount | - | - | - |

### Flag Editing

Type flags directly to toggle:
- Item types: `type <name>` (weapon, armor, container, etc.)
- Extra flags: toggle by name (glow, hum, nodrop, etc.)
- Wear flags: toggle by name (take, finger, neck, etc.)

### Data Structure

`OBJ_INDEX_DATA` in [merc.h](../../src/core/merc.h) contains:
- `vnum` - Virtual number
- `name` - Keywords
- `short_descr` - Inventory description
- `description` - Ground description
- `item_type` - Object type
- `extra_flags` - Item flags
- `wear_flags` - Equip locations
- `value[4]` - Type-specific values
- `weight` / `cost` - Physical properties
- `affected` - Stat modifiers

---

## Mobile Editor (medit)

Entry: `medit <vnum>` or `medit create <vnum>`
Source: [olc.c:868-922](../../src/world/olc.c#L868-L922)

Mobiles are NPCs that populate the world.

### Commands

| Command | Function | Description |
|---------|----------|-------------|
| show | medit_show | Display mobile properties |
| create | medit_create | Create new mobile |
| name | medit_name | Set keywords |
| short | medit_short | Set short desc (in room) |
| long | medit_long | Set long desc (when looking) |
| desc | medit_desc | Enter string editor for examine text |
| level | medit_level | Set level |
| alignment | medit_align | Set alignment (-1000 to +1000) |
| spec | medit_spec | Set special function (see [special-functions.md](special-functions.md)) |
| shop | medit_shop | Configure as shopkeeper |
| v0 | medit_value0 | Set max HP |
| v1 | medit_value1 | Set max mana |
| v2 | medit_value2 | Set practice count |

Command table: [olc.c:319-342](../../src/world/olc.c#L319-L342)

### Flag Editing

Type flags directly to toggle:
- Sex: `sex <name>` (male, female, neutral)
- Act flags: toggle by name (sentinel, scavenger, aggressive, etc.)
- Affect flags: toggle by name (blind, invisible, flying, etc.)

### Shop Configuration

`shop` command with no args shows shop syntax:
- `shop <hours> <profit_buy> <profit_sell> <type1> [type2...]`

### Data Structure

`MOB_INDEX_DATA` in [merc.h](../../src/core/merc.h) contains:
- `vnum` - Virtual number
- `player_name` - Keywords
- `short_descr` - Room description
- `long_descr` - Look description
- `description` - Examine text
- `level` / `alignment` - Stats
- `act` - Behavior flags
- `affected_by` - Affect flags
- `spec_fun` - Special procedure pointer
- `pShop` - Shop data (if merchant)

---

## Help Editor (hedit)

Entry: `hedit <keyword>` or `hedit create <keyword>`
Source: [olc.c:1450-1489](../../src/world/olc.c#L1450-L1489)

Help entries are in-game documentation accessed via the `help` command.

### Commands

| Command | Function | Description |
|---------|----------|-------------|
| create | hedit_create | Create new help entry |
| change | hedit_change | Edit existing entry |
| keyword | hedit_keyword | Set help keywords |
| text | hedit_text | Enter string editor for help text |
| level | hedit_level | Set minimum level to view |
| index | hedit_index | List all help entries |
| delete | hedit_delete | Remove help entry |

Command table: [olc.c:213-225](../../src/world/olc.c#L213-L225)

### Data Structure

`HELP_DATA` in [merc.h](../../src/core/merc.h) contains:
- `level` - Minimum level to see help
- `keyword` - Search keywords
- `text` - Help content

---

## Common Features

### String Editor

Commands like `desc` and `text` enter a line-based string editor:
- Type text to add lines
- `.c` - Clear all text
- `.s` - Show current text
- `.f` - Format text (word wrap)
- `@` or `.q` - Exit string editor

### Saving Changes

Changes are held in memory until saved:
- Areas marked with `AREA_CHANGED` flag when modified
- Use `asave` command to write all changed areas to disk
- Areas auto-save on shutdown

Save functions in [olc_save.c](../../src/world/olc_save.c):
- `save_area()` - Write single area file
- `save_area_list()` - Write area.lst
- `save_mobiles()` / `save_objects()` / `save_rooms()` - Section writers
