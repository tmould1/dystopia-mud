# OLC (Online Creation) System

The OLC system allows authorized builders to create and edit game content (areas, rooms, objects, mobiles, help files) while the MUD is running.

Based on **ILAB Online Creation [Beta 1.1]** by Jason Dinkel (jdinkel@mines.colorado.edu), May 15, 1995. Original by Surreality and Locke.

## Quick Reference

| Editor | Command | Trust | Description |
|--------|---------|-------|-------------|
| Area | `aedit <vnum>` | 8 | Edit area properties |
| Room | `redit [vnum]` | 8 | Edit room (default: current room) |
| Object | `oedit <vnum>` | 8 | Edit object prototype |
| Mobile | `medit <vnum>` | 10 | Edit mobile prototype |
| Help | `hedit <keyword>` | 10 | Edit help entries |

Use `done` to exit any editor. Use `show` to display current values.

## Editor Modes

Defined in [olc.h:44-48](../../src/world/olc.h#L44-L48):

| Constant | Value | Editor |
|----------|-------|--------|
| ED_AREA | 1 | Area Editor |
| ED_ROOM | 2 | Room Editor |
| ED_OBJECT | 3 | Object Editor |
| ED_MOBILE | 4 | Mobile Editor |
| ED_HELP | 6 | Help Editor |

## Security Model

Access controlled by the `IS_BUILDER` macro in [olc.h:225-230](../../src/world/olc.h#L225-L230):

```c
#define IS_BUILDER(ch, Area)    ( ( ch->pcdata->security >= Area->security  \
                                || strstr( Area->builders, ch->name )       \
                                || strstr( Area->builders, "All" ) )        \
                                && !IS_EXTRA( ch, EXTRA_OSWITCH )           \
                                && !IS_HEAD(ch, LOST_HEAD)                  \
                                && !IS_EXTRA(ch, EXTRA_SWITCH) )
```

Requirements:
- Player security level >= area security level, OR
- Player name in area's builders list, OR
- "All" in area's builders list
- AND not switched/polymorphed

## Related Systems

- **[Special Functions](special-functions.md)** - Hardcoded mob behaviors (spec_fun)
- **[RoomText](roomtext.md)** - Text-triggered room events

## Documentation

- [Editors](editors.md) - Detailed editor documentation
- [Commands](commands.md) - Complete command reference
- [Special Functions](special-functions.md) - Mob special procedures
- [RoomText](roomtext.md) - Room trigger system

## Source Files

| File | Purpose |
|------|---------|
| [olc.h](../../src/world/olc.h) | Constants, macros, prototypes |
| [olc.c](../../src/world/olc.c) | Entry points, interpreters, command tables |
| [olc_act.c](../../src/world/olc_act.c) | Editor action functions |
| [olc_save.c](../../src/world/olc_save.c) | Save/load area files |
| [build.c](../../src/world/build.c) | Extra description utilities |
