# PK Flags and States

This document details the data fields used to track PvP status and statistics.

## Character Data Fields

Located in `CHAR_DATA` structure in [merc.h](../../src/core/merc.h):

| Field | Type | Line | Purpose |
|-------|------|------|---------|
| `pkill` | int | [2328](../../src/core/merc.h#L2328) | Total player kills by this character |
| `pdeath` | int | [2329](../../src/core/merc.h#L2329) | Total player deaths of this character |
| `bounty` | int | [2468](../../src/core/merc.h#L2468) | Current QP bounty on this character |
| `safe_counter` | sh_int | [2415](../../src/core/merc.h#L2415) | Post-avatar safety timer (ticks) |
| `generation` | sh_int | [2299](../../src/core/merc.h#L2299) | Character generation level |

## Room Flags

Located in [merc.h](../../src/core/merc.h):

| Flag | Value | Line | Purpose |
|------|-------|------|---------|
| `ROOM_SAFE` | 1024 | [1611](../../src/core/merc.h#L1611) | Prevents all combat in room |
| `ROOM_ARENA` | 131072 | [1618](../../src/core/merc.h#L1618) | Marks room as arena zone |

## Affect Flags

Located in [merc.h](../../src/core/merc.h):

| Flag | Line | Purpose |
|------|------|---------|
| `AFF_PEACE` | [957](../../src/core/merc.h#L957) | Spell effect preventing PK initiation |
| `ITEMA_PEACE` | [985](../../src/core/merc.h#L985) | Item effect preventing PK initiation |

## Kill/Death Tracking

When a player kills another player, the following updates occur in [fight.c:4981-4999](../../src/combat/fight.c#L4981-L4999):

```
Attacker: ch->pkill++
Victim: victim->pdeath++
```

### Kingdom Tracking

If both players belong to kingdoms, kingdom statistics are also updated:
- [fight.c:4996](../../src/combat/fight.c#L4996): `kingdom_table[ch->pcdata->kingdom].kills++`
- [fight.c:4998](../../src/combat/fight.c#L4998): `kingdom_table[victim->pcdata->kingdom].deaths++`

Kingdom PK data is persisted in `kingdoms.txt` and displayed with a ratio: `(100 * kills) / (kills + deaths)`

## Player Data Fields

Located in `PC_DATA` structure in [merc.h](../../src/core/merc.h):

| Field | Purpose |
|-------|---------|
| `kingdom` | Kingdom membership for PK tracking |
| `upgrade_level` | Affects ragnarok matchmaking restrictions |
| `awins` | Arena wins |
| `alosses` | Arena losses |

## CAN_PK Macro

**Location:** [merc.h:3104](../../src/core/merc.h#L3104)

```c
#define CAN_PK(ch)   (get_trust(ch)>= 3 && get_trust(ch)<= 12)
```

This macro determines if a character can participate in PvP:
- Trust level 3 = Avatar (minimum for PK)
- Trust level 12 = Maximum mortal level for PK
- Trust > 12 = Immortal (cannot PK)
