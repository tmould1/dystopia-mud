# RoomText System

RoomText is a text-triggered event system for rooms. When players say specific words or enter rooms, RoomText triggers can fire actions like creating objects, teleporting players, or displaying messages.

## Overview

RoomText provides room-level "programs" without scripting. Triggers are pattern-matched against player input and execute predefined actions.

**Key limitation**: Unlike traditional MobProgs, RoomText triggers are not editable via OLC. They are loaded directly from area files and cannot be modified online.

## Data Structure

Defined in [merc.h:2626-2636](../../src/core/merc.h#L2626-L2636):

```c
typedef struct roomtext_data
{
    int                 type;       // Trigger type flags (RT_*)
    int                 power;      // VNUM for objects/mobs/rooms, or spell number
    int                 mob;        // Required mob VNUM (0 = no requirement)
    char *              input;      // Input pattern to match
    char *              output;     // Message to room
    char *              choutput;   // Message to character
    char *              name;       // Player name filter ("all" = everyone)
    struct roomtext_data *next;     // Linked list
} ROOMTEXT_DATA;
```

Stored in `ROOM_INDEX_DATA->roomtext` as a linked list.

---

## Trigger Types

Flags defined in [merc.h:1633-1655](../../src/core/merc.h#L1633-L1655):

### Action Types

| Flag | Value | Description |
|------|-------|-------------|
| RT_LIGHTS | 1 | Toggle room lights on/off |
| RT_SAY | 2 | Display message only (default) |
| RT_ENTER | 4 | Trigger on room entry |
| RT_CAST | 8 | (unused in execution) |
| RT_THROWOUT | 16 | (unused) |
| RT_OBJECT | 32 | Create and give/place object |
| RT_MOBILE | 64 | Spawn mobile in room |
| RT_LIGHT | 128 | Turn lights ON only |
| RT_DARK | 256 | Turn lights OFF only |
| RT_OPEN_LIFT | 512 | Open elevator |
| RT_CLOSE_LIFT | 1024 | Close elevator |
| RT_MOVE_LIFT | 2048 | Move elevator to floor |
| RT_SPELL | 4096 | Cast spell on character |
| RT_PORTAL | 8192 | Create one-way portal |
| RT_TELEPORT | 16384 | Teleport character to room |
| RT_ACTION | 32768 | Force mob to execute command |

### Modifier Flags

| Flag | Value | Description |
|------|-------|-------------|
| RT_RETURN | 1048576 | Stop processing after this trigger |
| RT_PERSONAL | 2097152 | Show message only to character, not room |
| RT_TIMER | 4194304 | Set created object timer to 1 tick |

---

## Execution Flow

### Entry Points

RoomText triggers fire from:

1. **Room Entry** - [act_move.c:536](../../src/commands/act_move.c#L536), [comm.c:2937](../../src/core/comm.c#L2937):
   ```c
   room_text(ch, ">ENTER<");
   ```

2. **Player Say** - [act_comm.c:871](../../src/commands/act_comm.c#L871):
   ```c
   room_text( ch, strlower(argument) );
   ```

### Processing Logic

Main function: [act_comm.c:921-1058](../../src/commands/act_comm.c#L921-L1058)

```c
void room_text( CHAR_DATA *ch, char *argument)
```

1. Iterate through room's roomtext list
2. Check if input matches pattern:
   - Exact match: `!strcmp(argument, rt->input)`
   - Contains match: `is_in(argument, rt->input)`
   - All words match: `all_in(argument, rt->input)`
3. Check player name filter (if not "all")
4. Check required mob presence (if mob VNUM != 0)
5. Execute action based on type flags
6. Display output messages
7. If RT_RETURN set, stop processing more triggers

### Action Execution

From [act_comm.c:963-1045](../../src/commands/act_comm.c#L963-L1045):

| Type | Action |
|------|--------|
| RT_SAY | Display messages only |
| RT_LIGHTS | Toggle room ROOM_DARK flag |
| RT_LIGHT | Remove ROOM_DARK flag |
| RT_DARK | Set ROOM_DARK flag |
| RT_OBJECT | Create object, give to char or place in room |
| RT_MOBILE | Spawn mobile in room |
| RT_SPELL | Cast spell (by skill number) on character |
| RT_PORTAL | Create portal object to destination room |
| RT_TELEPORT | Move character to destination room |
| RT_ACTION | Parse argument for mob name, make mob interpret output |
| RT_OPEN_LIFT | Call `open_lift(ch)` |
| RT_CLOSE_LIFT | Call `close_lift(ch)` |
| RT_MOVE_LIFT | Call `move_lift(ch, rt->power)` |

---

## Area File Format

RoomText triggers use the `T` section marker within room definitions.

### Format

```
T
<input pattern>~
<room output message>~
<char output message>~
<name filter>~
<type> <power> <mob>
```

Fields:
- **input**: Text pattern to match (can use `*` wildcards, `&` prefix, `|` separators)
- **output**: Message shown to room (supports act() format codes)
- **choutput**: Message shown to triggering character (`copy` = use output)
- **name**: Player name filter (`all` = any player)
- **type**: Combination of RT_* flags
- **power**: Object/Mob/Room VNUM or spell number depending on type
- **mob**: Required mob VNUM (0 = no requirement)

### Example

From [quest.are:285-290](../../../gamedata/area/quest.are#L285):

```
#29500
The green chamber~
You are standing within a small chamber of glowing green stone.
~
0 25608 0
T
&restore*health*~
The room pulses with green energy.~
copy~
all~
4096 43 0
D4
...
S
```

This trigger:
- **Input**: `&restore*health*` - matches "restore health", "restore my health", etc.
- **Type**: 4096 (RT_SPELL)
- **Power**: 43 (spell number for restoration)
- **Mob**: 0 (no mob required)
- When player says text containing "restore" and "health", casts spell #43 on them

### Pattern Syntax

- `&word` - Input must contain this word
- `word*` - Word prefix match
- `*word` - Word suffix match
- `|word1|word2|` - Match any of these words
- Plain text - Exact match

---

## Loading

RoomText is loaded in `load_rooms()` and `new_load_rooms()` in [db.c](../../src/core/db.c).

When parser encounters `T` after room header:

From [db.c:1517-1527](../../src/core/db.c#L1517-L1527):

```c
rt = alloc_mem( sizeof(ROOMTEXT_DATA) );
rt->input       = fread_string( fp );
rt->output      = fread_string( fp );
rt->choutput    = fread_string( fp );
rt->name        = fread_string( fp );
rt->type        = fread_number( fp );
rt->power       = fread_number( fp );
rt->mob         = fread_number( fp );
rt->next        = NULL;
```

**Note**: RoomText is initialized to NULL for each room and not saved by OLC. Area files must be manually edited to add/modify RoomText.

---

## Saving

RoomText saving is handled in [olc_save.c](../../src/world/olc_save.c) when writing rooms, but requires the data to already exist on the room.

---

## Limitations

1. **No OLC editing** - RoomText cannot be created or modified via the room editor
2. **Manual file editing** - Must edit .are files directly to add triggers
3. **No conditions** - Cannot check character stats, items, etc.
4. **Fixed actions** - Limited to predefined action types
5. **No scripting** - Unlike MobProgs, cannot write custom logic

## Common Use Cases

1. **Puzzle rooms** - Say magic words to reveal exits or get items
2. **Healing chambers** - Enter to restore health/mana
3. **Teleport rooms** - Say destination name to teleport
4. **Quest triggers** - Speaking to NPCs triggers events
5. **Elevator systems** - Control lifts with spoken commands
