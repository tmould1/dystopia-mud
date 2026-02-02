# Accessibility System Overview

The accessibility suite provides two complementary features: a **screen reader mode** for players using assistive technology, and **MCMP (MUD Client Media Protocol)** for delivering audio cues to supported clients. Both are opt-in, toggled independently, and have zero impact on players who don't enable them.

## Architecture

```
Player Connection
  -> GMCP negotiation (comm.c)
  -> Client sends Core.Supports.Set ["Client.Media 1"]
  -> Server sets GMCP_PACKAGE_CLIENT_MEDIA flag
  -> Server sends Client.Media.Default with base URL
  -> Game events call mcmp_*() functions
  -> mcmp_play()/mcmp_stop() build JSON, call gmcp_send()
  -> Client receives Client.Media.Play/Stop via GMCP subneg

Player types "screenreader"
  -> PLR_SCREENREADER flag toggled on ch->act
  -> ANSI off, BRIEF on, BLANK on, AUTOEXIT on (auto-set)
  -> Output functions check IS_SCREENREADER(ch) for clean branches
  -> write_to_buffer() collapses excess whitespace
```

## New Player Creation

Screen reader mode is offered as a display mode option (`[S]creen reader`) alongside None/ANSI/Xterm during character creation. Choosing it enables screen reader mode and skips the need for a color choice.

**Location:** [nanny.c](../../src/core/nanny.c)

## Player Commands

| Command | Effect |
|---------|--------|
| `screenreader` | Toggle screen reader mode on/off |
| `config screenrd` | Toggle screen reader mode (alias) |
| `config mcmp` | Toggle MCMP preference on/off |
| `gmcp` | Show GMCP status including Client.Media |

## PLR Flags

**Location:** [merc.h](../../src/core/merc.h)

| Flag | Bit | Value | Purpose |
|------|-----|-------|---------|
| `PLR_SCREENREADER` | 8 | 256 | Master screen reader toggle |
| `PLR_PREFER_MCMP` | 22 | 4194304 | Persist MCMP preference across sessions |

Both flags are saved in the player's `act` bitmask via the standard save system.

## Macro

```c
#define IS_SCREENREADER(ch) (!IS_NPC(ch) && IS_SET((ch)->act, PLR_SCREENREADER))
```

## Files Modified

| File | Changes |
|------|---------|
| [merc.h](../../src/core/merc.h) | PLR_SCREENREADER, PLR_PREFER_MCMP, IS_SCREENREADER macro |
| [interp.c](../../src/core/interp.c) | `screenreader` command in cmd_table |
| [comm.c](../../src/core/comm.c) | Space-collapsing filter in write_to_buffer() |
| [nanny.c](../../src/core/nanny.c) | MCMP auto-detect for new players, login UI sound |
| [act_info.c](../../src/commands/act_info.c) | do_screenreader, SR branches in score/who/equipment/exits/weaplist/config |
| [fight.c](../../src/combat/fight.c) | SR-clean damage messages, per-round MCMP combat hooks |
| [act_comm.c](../../src/commands/act_comm.c) | MCMP channel notification hooks |
| [act_move.c](../../src/commands/act_move.c) | MCMP movement + ambient hooks |
| [update.c](../../src/systems/update.c) | MCMP weather sound hooks |
| [gmcp.c](../../src/systems/gmcp.c) | Client.Media package handling |
| [gmcp.h](../../src/systems/gmcp.h) | GMCP_PACKAGE_CLIENT_MEDIA flag |
| [mssp.c](../../src/systems/mssp.c) | MCMP=1 in MSSP advertisement |

## New Files

| File | Purpose |
|------|---------|
| [mcmp.h](../../src/systems/mcmp.h) | MCMP protocol header — constants and prototypes |
| [mcmp.c](../../src/systems/mcmp.c) | MCMP protocol implementation — core + game event functions |

## Design Principles

1. **Opt-in only** — No accessibility feature activates unless the player explicitly enables it or the client advertises support.
2. **Zero impact** — All code paths check flags before branching. Players without screen reader mode or MCMP see no difference.
3. **Safe to call unconditionally** — All `mcmp_*()` game event functions check `mcmp_enabled()` internally, so callers don't need guard checks.
4. **Per-round combat sounds** — Combat does 2-30+ individual attacks per 0.75s round. MCMP sends exactly one combat sound per round (in `violence_update()`), not per-hit.
5. **Layered audio** — Ambient and weather sounds use separate MCMP keys (`"ambient"` and `"weather"`) so they overlay without replacing each other.
6. **Caption accessibility** — Every MCMP sound includes a `caption` field for clients that display text descriptions of audio events.
