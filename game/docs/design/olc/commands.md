# OLC Command Reference

Complete list of OLC commands with source locations.

## Entry Commands

Commands to enter editors. Defined in [interp.c](../../src/core/interp.c).

| Command | Function | Trust | Source |
|---------|----------|-------|--------|
| aedit | do_aedit | 8 | [interp.c:1146](../../src/core/interp.c#L1146) |
| redit | do_redit | 8 | [interp.c:1147](../../src/core/interp.c#L1147) |
| oedit | do_oedit | 8 | [interp.c:1148](../../src/core/interp.c#L1148) |
| medit | do_medit | 10 | [interp.c:278](../../src/core/interp.c#L278) |
| hedit | do_hedit | 10 | [interp.c:841](../../src/core/interp.c#L841) |

Entry function implementations in [olc.c](../../src/world/olc.c):
- `do_aedit()` - [olc.c:704-757](../../src/world/olc.c#L704-L757)
- `do_redit()` - [olc.c:762-800](../../src/world/olc.c#L762-L800)
- `do_oedit()` - [olc.c:805-863](../../src/world/olc.c#L805-L863)
- `do_medit()` - [olc.c:868-922](../../src/world/olc.c#L868-L922)
- `do_hedit()` - [olc.c:1450-1489](../../src/world/olc.c#L1450-L1489)

---

## Area Editor Commands

Command table: [olc.c:228-251](../../src/world/olc.c#L228-L251)

| Command | Function | Source | Description |
|---------|----------|--------|-------------|
| age | aedit_age | [olc_act.c](../../src/world/olc_act.c) | Set area repop timer |
| builders | aedit_builder | [olc_act.c](../../src/world/olc_act.c) | Manage builder list |
| commands | show_commands | [olc.c](../../src/world/olc.c) | List available commands |
| create | aedit_create | [olc_act.c](../../src/world/olc_act.c) | Create new area |
| filename | aedit_file | [olc_act.c](../../src/world/olc_act.c) | Set .are filename |
| hidden | aedit_hidden | [olc_act.c](../../src/world/olc_act.c) | Toggle area visibility |
| name | aedit_name | [olc_act.c](../../src/world/olc_act.c) | Set area name |
| recall | aedit_recall | [olc_act.c](../../src/world/olc_act.c) | Set recall room VNUM |
| reset | aedit_reset | [olc_act.c](../../src/world/olc_act.c) | Reset entire area |
| security | aedit_security | [olc_act.c](../../src/world/olc_act.c) | Set security level |
| show | aedit_show | [olc_act.c](../../src/world/olc_act.c) | Display area info |
| vnum | aedit_vnum | [olc_act.c](../../src/world/olc_act.c) | Set VNUM range |
| lvnum | aedit_lvnum | [olc_act.c](../../src/world/olc_act.c) | Set lower VNUM |
| uvnum | aedit_uvnum | [olc_act.c](../../src/world/olc_act.c) | Set upper VNUM |
| ? | show_help | [olc.c](../../src/world/olc.c) | Show help |
| version | show_version | [olc.c](../../src/world/olc.c) | Show OLC version |

---

## Room Editor Commands

Command table: [olc.c:255-287](../../src/world/olc.c#L255-L287)

| Command | Function | Source | Description |
|---------|----------|--------|-------------|
| commands | show_commands | [olc.c](../../src/world/olc.c) | List available commands |
| create | redit_create | [olc_act.c](../../src/world/olc_act.c) | Create new room |
| desc | redit_desc | [olc_act.c](../../src/world/olc_act.c) | Edit room description |
| ed | redit_ed | [olc_act.c](../../src/world/olc_act.c) | Edit extra descriptions |
| format | redit_format | [olc_act.c](../../src/world/olc_act.c) | Auto-format description |
| name | redit_name | [olc_act.c](../../src/world/olc_act.c) | Set room title |
| show | redit_show | [olc_act.c](../../src/world/olc_act.c) | Display room info |
| north | redit_north | [olc_act.c](../../src/world/olc_act.c) | Edit north exit |
| south | redit_south | [olc_act.c](../../src/world/olc_act.c) | Edit south exit |
| east | redit_east | [olc_act.c](../../src/world/olc_act.c) | Edit east exit |
| west | redit_west | [olc_act.c](../../src/world/olc_act.c) | Edit west exit |
| up | redit_up | [olc_act.c](../../src/world/olc_act.c) | Edit up exit |
| down | redit_down | [olc_act.c](../../src/world/olc_act.c) | Edit down exit |
| walk | redit_move | [olc_act.c](../../src/world/olc_act.c) | Move through exit |
| mreset | redit_mreset | [olc_act.c](../../src/world/olc_act.c) | Add mobile reset |
| oreset | redit_oreset | [olc_act.c](../../src/world/olc_act.c) | Add object reset |
| mlist | redit_mlist | [olc_act.c](../../src/world/olc_act.c) | List mobiles |
| olist | redit_olist | [olc_act.c](../../src/world/olc_act.c) | List objects |
| mshow | redit_mshow | [olc_act.c](../../src/world/olc_act.c) | Show mobile details |
| oshow | redit_oshow | [olc_act.c](../../src/world/olc_act.c) | Show object details |
| ? | show_help | [olc.c](../../src/world/olc.c) | Show help |
| version | show_version | [olc.c](../../src/world/olc.c) | Show OLC version |

---

## Object Editor Commands

Command table: [olc.c:291-315](../../src/world/olc.c#L291-L315)

| Command | Function | Source | Description |
|---------|----------|--------|-------------|
| addaffect | oedit_addaffect | [olc_act.c](../../src/world/olc_act.c) | Add stat modifier |
| commands | show_commands | [olc.c](../../src/world/olc.c) | List available commands |
| cost | oedit_cost | [olc_act.c](../../src/world/olc_act.c) | Set gold value |
| create | oedit_create | [olc_act.c](../../src/world/olc_act.c) | Create new object |
| delaffect | oedit_delaffect | [olc_act.c](../../src/world/olc_act.c) | Remove stat modifier |
| ed | oedit_ed | [olc_act.c](../../src/world/olc_act.c) | Edit extra descriptions |
| long | oedit_long | [olc_act.c](../../src/world/olc_act.c) | Set ground description |
| name | oedit_name | [olc_act.c](../../src/world/olc_act.c) | Set keywords |
| short | oedit_short | [olc_act.c](../../src/world/olc_act.c) | Set inventory description |
| show | oedit_show | [olc_act.c](../../src/world/olc_act.c) | Display object info |
| v0 | oedit_value0 | [olc_act.c](../../src/world/olc_act.c) | Set value[0] |
| v1 | oedit_value1 | [olc_act.c](../../src/world/olc_act.c) | Set value[1] |
| v2 | oedit_value2 | [olc_act.c](../../src/world/olc_act.c) | Set value[2] |
| v3 | oedit_value3 | [olc_act.c](../../src/world/olc_act.c) | Set value[3] |
| weight | oedit_weight | [olc_act.c](../../src/world/olc_act.c) | Set weight |
| ? | show_help | [olc.c](../../src/world/olc.c) | Show help |
| version | show_version | [olc.c](../../src/world/olc.c) | Show OLC version |

---

## Mobile Editor Commands

Command table: [olc.c:319-342](../../src/world/olc.c#L319-L342)

| Command | Function | Source | Description |
|---------|----------|--------|-------------|
| alignment | medit_align | [olc_act.c](../../src/world/olc_act.c) | Set alignment |
| commands | show_commands | [olc.c](../../src/world/olc.c) | List available commands |
| create | medit_create | [olc_act.c](../../src/world/olc_act.c) | Create new mobile |
| desc | medit_desc | [olc_act.c](../../src/world/olc_act.c) | Edit examine description |
| level | medit_level | [olc_act.c](../../src/world/olc_act.c) | Set level |
| long | medit_long | [olc_act.c](../../src/world/olc_act.c) | Set look description |
| name | medit_name | [olc_act.c](../../src/world/olc_act.c) | Set keywords |
| shop | medit_shop | [olc_act.c](../../src/world/olc_act.c) | Configure shopkeeper |
| short | medit_short | [olc_act.c](../../src/world/olc_act.c) | Set room description |
| show | medit_show | [olc_act.c](../../src/world/olc_act.c) | Display mobile info |
| spec | medit_spec | [olc_act.c](../../src/world/olc_act.c) | Set special function |
| v0 | medit_value0 | [olc_act.c](../../src/world/olc_act.c) | Set max HP |
| v1 | medit_value1 | [olc_act.c](../../src/world/olc_act.c) | Set max mana |
| v2 | medit_value2 | [olc_act.c](../../src/world/olc_act.c) | Set practice count |
| ? | show_help | [olc.c](../../src/world/olc.c) | Show help |
| version | show_version | [olc.c](../../src/world/olc.c) | Show OLC version |

---

## Help Editor Commands

Command table: [olc.c:213-225](../../src/world/olc.c#L213-L225)

| Command | Function | Source | Description |
|---------|----------|--------|-------------|
| commands | show_commands | [olc.c](../../src/world/olc.c) | List available commands |
| create | hedit_create | [olc_act.c](../../src/world/olc_act.c) | Create new help entry |
| ? | show_help | [olc.c](../../src/world/olc.c) | Show help |
| level | hedit_level | [olc_act.c](../../src/world/olc_act.c) | Set view level |
| text | hedit_text | [olc_act.c](../../src/world/olc_act.c) | Edit help text |
| index | hedit_index | [olc_act.c](../../src/world/olc_act.c) | List all entries |
| change | hedit_change | [olc_act.c](../../src/world/olc_act.c) | Modify existing entry |
| keyword | hedit_keyword | [olc_act.c](../../src/world/olc_act.c) | Set keywords |
| delete | hedit_delete | [olc_act.c](../../src/world/olc_act.c) | Remove entry |

---

## Global Commands

Available in all editors:

| Command | Function | Description |
|---------|----------|-------------|
| done | edit_done | Exit editor, return to normal mode |
| commands | show_commands | List commands for current editor |
| show | *_show | Display current values |
| ? | show_help | Show help text |
| version | show_version | Show OLC version info |

---

## Utility Commands

Additional OLC-related commands:

| Command | Function | Trust | Source | Description |
|---------|----------|-------|--------|-------------|
| asave | do_asave | 8 | [olc.c](../../src/world/olc.c) | Save changed areas |
| alist | do_alist | 2 | [olc.c:1383-1411](../../src/world/olc.c#L1383-L1411) | List all areas |
| reset | do_resets | varies | [olc.c:1182-1374](../../src/world/olc.c#L1182-L1374) | Manage room resets |

---

## Function Prototypes

All OLC functions declared in [olc.h](../../src/world/olc.h):

```c
typedef bool OLC_FUN args( ( CHAR_DATA *ch, char *argument ) );
#define DECLARE_OLC_FUN( fun )  OLC_FUN fun
```

Function returns `TRUE` if the area should be marked as changed, `FALSE` otherwise.
