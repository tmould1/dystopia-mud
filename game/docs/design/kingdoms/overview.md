# Kingdom System Overview

Kingdoms are PvP factions that players can join. Each kingdom tracks member kills and deaths, maintains a QPS treasury, and provides a private chat channel.

## Core Concept

The server supports up to `MAX_KINGDOM` (5) kingdoms stored in a global `kingdom_table[]` array. Index 0 is a dummy entry; kingdoms 1-5 are usable. Each player stores their affiliation in `ch->pcdata->kingdom` (0 = no kingdom).

**Location:** [jobo_king.c](../../src/systems/jobo_king.c)

## Data Structure

**Location:** [merc.h:321-333](../../src/core/merc.h#L321-L333)

```c
typedef struct kingdom_data {
    char *whoname;   // display name in who list
    char *name;      // keyword name
    char *leader;    // character name of leader
    char *general;   // character name of second-in-command
    int kills;       // total PvP kills by members
    int deaths;      // total PvP deaths of members
    int qps;         // treasury (quest points)
    int req_hit;     // max HP required to join
    int req_move;    // max move required to join
    int req_mana;    // max mana required to join
    int req_qps;     // QPS cost to join (donated to treasury)
} KINGDOM_DATA;
```

Global table declared at [jobo_king.c:29](../../src/systems/jobo_king.c#L29):

```c
KINGDOM_DATA kingdom_table[MAX_KINGDOM + 1];
```

Player affiliation stored in `PC_DATA` at [merc.h:2349](../../src/core/merc.h#L2349):

```c
int kingdom;  // 0 = none, 1-5 = kingdom ID
```

## Ranks and Permissions

| Role | Induct | Outcast | Set Requirements | Set General | Configurable by |
|------|--------|---------|------------------|-------------|-----------------|
| Leader | Yes | Yes (cannot outcast self) | Yes | Yes | Immortals via `kset` |
| General | Yes | Yes (cannot outcast leader) | No | No | Leader via `kset general <name>` |
| Member | No | No | No | No | Inducted by leader/general |

## Commands

**Location:** [interp.c:1141-1148](../../src/core/interp.c#L1141-L1148)

| Command | Trust | Position | Function | Description |
|---------|-------|----------|----------|-------------|
| `kingdoms` | 1 | Any | [do_kingdoms](../../src/systems/jobo_king.c#L101) | List all kingdoms with stats and join requirements |
| `ktalk` | 2 | Any | [do_ktalk](../../src/commands/act_comm.c#L486) | Kingdom-only chat channel |
| `kstats` | 2 | Any | [do_kstats](../../src/systems/jobo_king.c#L330) | View your kingdom's leader, general, and treasury |
| `wantkingdom` | 2 | Any | [do_wantkingdom](../../src/systems/jobo_king.c#L182) | Toggle opt-in flag for recruitment |
| `kinduct` | 2 | Standing | [do_kinduct](../../src/systems/jobo_king.c#L135) | Leader/general inducts a player in the same room |
| `kset` | 2 | Standing | [do_kset](../../src/systems/jobo_king.c#L195) | Leader sets requirements and general; immortals set name/leader/kills/deaths |
| `koutcast` | 2 | Standing | [do_koutcast](../../src/systems/jobo_king.c#L288) | Leader/general removes a member |
| `kingset` | 7 | Any | [do_kingset](../../src/systems/jobo_king.c#L351) | Immortal assigns a player to a kingdom directly |

## Membership Flow

### Joining

1. Player runs `wantkingdom` to set `JFLAG_WANT_KINGDOM` ([merc.h:1912](../../src/core/merc.h#L1912))
2. Leader or general runs `kinduct <player>` (must be in same room)
3. System checks requirements against the kingdom's thresholds:

| Requirement | Player Field | Kingdom Field |
|-------------|-------------|---------------|
| HP | `victim->max_hit` | `req_hit` |
| Move | `victim->max_move` | `req_move` |
| Mana | `victim->max_mana` | `req_mana` |
| Quest Points | `victim->pcdata->quest` | `req_qps` |

4. If all requirements met: player's QPS are deducted by `req_qps`, that amount is added to the kingdom treasury, and `victim->pcdata->kingdom` is set

### Leaving

- Leader or general runs `koutcast <player>` (must be in same room)
- The leader cannot be outcasted
- Players cannot leave voluntarily — they must be outcasted or reassigned by an immortal via `kingset`

## Kingdom Chat

**Location:** [act_comm.c:486-493](../../src/commands/act_comm.c#L486-L493)

The `ktalk` command uses `CHANNEL_KINGDOM` ([merc.h:2028](../../src/core/merc.h#L2028)) to broadcast messages only to online members of the same kingdom (and immortals). Players can toggle the channel on/off via the `channels` command.

## PvP Integration

**Location:** [fight.c:4652-4655](../../src/combat/fight.c#L4652-L4655)

When a player kills another player, kingdom stats update automatically:

```c
if (ch->pcdata->kingdom != 0)
    kingdom_table[ch->pcdata->kingdom].kills++;
if (victim->pcdata->kingdom != 0)
    kingdom_table[victim->pcdata->kingdom].deaths++;
```

The `kingdoms` command displays a kill ratio calculated as `100 * kills / (kills + deaths)`, formatted as a decimal (e.g. `0.75`).

## Treasury

- Funded by induction costs: each new member pays `req_qps` into `kingdom_table[k].qps`
- Viewable via `kstats`
- No withdrawal or spending mechanism exists in the current implementation

## Leader Settings

Leaders can configure their kingdom via `kset <field> <value>`:

| Field | Effect |
|-------|--------|
| `req_hit` | Set minimum HP to join |
| `req_move` | Set minimum move to join |
| `req_mana` | Set minimum mana to join |
| `req_qps` | Set QPS cost to join |
| `general` | Appoint a general (second-in-command) |

Immortals (level 7+) use `kset <kingdom#> <field> <value>` with additional fields: `leader`, `name`, `whoname`, `kills`, `deaths`.

## Persistence

**Location:** [jobo_king.c:33-99](../../src/systems/jobo_king.c#L33-L99)

Kingdoms are saved to `kingdoms.txt` in the text data directory. The file format stores each kingdom sequentially:

```
<name>~
<whoname>~
<leader>~
<general>~
<kills> <deaths> <qps>
<req_hit> <req_move> <req_mana> <req_qps>
```

- `save_kingdoms()` is called after any `kset` change
- `load_kingdoms()` runs at boot; creates a default file with 5 empty kingdoms if none exists
- Player kingdom affiliation is stored in the player save file via `ch->pcdata->kingdom`

## Source Files

| File | Contents |
|------|----------|
| [jobo_king.c](../../src/systems/jobo_king.c) | All kingdom commands and persistence |
| [act_comm.c:486-493](../../src/commands/act_comm.c#L486-L493) | `ktalk` channel implementation |
| [act_comm.c:335](../../src/commands/act_comm.c#L335) | Kingdom channel filtering in `talk_channel()` |
| [merc.h:321-333](../../src/core/merc.h#L321-L333) | `KINGDOM_DATA` struct |
| [merc.h:2349](../../src/core/merc.h#L2349) | `kingdom` field in `PC_DATA` |
| [merc.h:1912](../../src/core/merc.h#L1912) | `JFLAG_WANT_KINGDOM` constant |
| [merc.h:2028](../../src/core/merc.h#L2028) | `CHANNEL_KINGDOM` constant |
| [interp.c:1141-1148](../../src/core/interp.c#L1141-L1148) | Command table entries |
| [fight.c:4652-4655](../../src/combat/fight.c#L4652-L4655) | PvP kill/death tracking |
