# Communication System Overview

Players communicate through channels, local speech, tells, socials, and group messaging. Each channel uses a bitmask constant for selective muting, and most class-specific channels are restricted to members of that class.

## Architecture

All channel-based communication routes through a central dispatcher:

```
Player Input
  -> Command Interpreter (interp.c)
  -> do_* handler (act_comm.c)
  -> talk_channel() broadcast loop
  -> Per-recipient filtering (deaf, class, area)
  -> act() formatted output
```

Local commands (`say`, `emote`, `whisper`) bypass `talk_channel()` and deliver directly to the room.

**Location:** [act_comm.c](../../src/commands/act_comm.c)

## Channel Constants

**Location:** [merc.h:2028-2052](../../src/core/merc.h#L2028-L2052)

| Constant | Value | Scope | Restriction |
|----------|-------|-------|-------------|
| `CHANNEL_KINGDOM` | 1 | Kingdom members | `ch->pcdata->kingdom` match |
| `CHANNEL_CHAT` | 2 | Global | None (age-gated for newbies) |
| `CHANNEL_ANGEL` | 4 | Global | `CLASS_ANGEL` |
| `CHANNEL_IMMTALK` | 8 | Global | Immortal (level 7+) |
| `CHANNEL_MUSIC` | 16 | Global | None (age-gated) |
| `CHANNEL_YELL` | 128 | Area only | None (age-gated) |
| `CHANNEL_VAMPTALK` | 256 | Global | `CLASS_VAMPIRE` |
| `CHANNEL_HOWL` | 512 | Distance-based | `CLASS_WEREWOLF` or `POLY_WOLF` |
| `CHANNEL_LOG` | 1024 | Internal | Server logging |
| `CHANNEL_PRAY` | 2048 | Global | `CLASS_DEMON` |
| `CHANNEL_INFO` | 4096 | Global | System broadcasts |
| `CHANNEL_FLAME` | 8192 | Global | None (age-gated) |
| `CHANNEL_TELL` | 16384 | Direct | None |
| `CHANNEL_MAGETALK` | 32768 | Global | `CLASS_MAGE` |
| `CHANNEL_HIGHTALK` | 65536 | Global | `CLASS_SAMURAI` |
| `CHANNEL_NEWBIE` | 131072 | Global | Newbies and helpers |
| `CHANNEL_SIGN` | 262144 | Global | `CLASS_DROW` |
| `CHANNEL_MONK` | 524288 | Global | `CLASS_MONK` |
| `CHANNEL_MIKTALK` | 1048576 | Global | `CLASS_NINJA` |
| `CHANNEL_TELEPATH` | 2097152 | Global | `CLASS_SHAPESHIFTER` |
| `CHANNEL_COMMUNICATE` | 4194304 | Global | `CLASS_DROID` |
| `CHANNEL_KNIGHTTALK` | 8388608 | Global | `CLASS_UNDEAD_KNIGHT` |
| `CHANNEL_LICHTALK` | 16777216 | Global | `CLASS_LICH` |
| `CHANNEL_TANTALK` | 33554432 | Global | `CLASS_TANARRI` |

## Deafness System

Players selectively mute channels via the `channels` command, which toggles bits in `ch->deaf`:

```c
ch->deaf  // bitmask of muted CHANNEL_* constants
```

- `SET_BIT(ch->deaf, CHANNEL_CHAT)` — mute chat
- `REMOVE_BIT(ch->deaf, CHANNEL_CHAT)` — unmute chat
- Speaking on a channel auto-unmutes the speaker for that channel
- Immortals bypass deaf checks on recipients
- Marriage exemption: spouses can always `tell` each other regardless of deaf state

**Location:** Channel toggle UI in [act_info.c](../../src/commands/act_info.c)

## talk_channel()

**Location:** [act_comm.c:95-386](../../src/commands/act_comm.c#L95-L386)

The central broadcast function used by all channel commands.

### Speaker Checks (in order)

1. **Age gate** — newbies (age < 19) restricted to `CHANNEL_NEWBIE` on chat/music/yell
2. **Room silence** — `RTIMER_SILENCE` ([merc.h:534](../../src/core/merc.h#L534)) blocks all communication from the room
3. **Empty message** — rejected
4. **Lost tongue** — cannot speak without tongue body part
5. **Gagged** — cannot speak with gag equipped
6. **Auto-undeafen** — speaker is undeafened from the channel they're using

### Recipient Filtering

For each connected descriptor:

1. Must be `CON_PLAYING` and not the speaker
2. Must not have `ch->deaf` bit set for this channel
3. **Class check** — class channels require matching class or immortal status
4. **Kingdom check** — `CHANNEL_KINGDOM` requires same `ch->pcdata->kingdom`
5. **Proximity check** — `CHANNEL_YELL` filters to same area; `CHANNEL_HOWL` graduates by distance
6. `PLR_SILENCE` — silenced players transmit normally from their perspective but no one receives

### Message Formatting

Each channel has a unique format string with color codes. Examples:

| Channel | Format |
|---------|--------|
| Chat | Class-dependent verb: barks (werewolf), chants (monk), snarls (demon), etc. |
| Vamptalk | `#R<<#0Name#R>>#C message#n` |
| Magetalk | `#n{{#0Name#n}}#C 'message'#n` |
| Howl | `#y((#LName#y))#C 'message'#n` |
| Kingdom | `#R{{{ #PName #R}}}#n '#7message#n'` |

## Commands

### Global Channels

| Command | Aliases | Function | Position | Trust | Channel |
|---------|---------|----------|----------|-------|---------|
| `chat` | `.` | [do_chat](../../src/commands/act_comm.c#L397) | Any | 0 | `CHANNEL_CHAT` |
| `flame` | | [do_flame](../../src/commands/act_comm.c#L402) | Any | 0 | `CHANNEL_FLAME` |
| `music` | | [do_music](../../src/commands/act_comm.c#L410) | Sleeping | 2 | `CHANNEL_MUSIC` |
| `yell` | | [do_yell](../../src/commands/act_comm.c#L416) | Sitting | 2 | `CHANNEL_YELL` |
| `newbie` | | [do_newbie](../../src/commands/act_comm.c#L476) | Any | 1 | `CHANNEL_NEWBIE` |
| `immtalk` | `:` | [do_immtalk](../../src/commands/act_comm.c#L423) | Any | 7 | `CHANNEL_IMMTALK` |

### Class Channels

Every class has a dedicated channel. Immortals can listen to all.

| Command | Aliases | Class | Channel |
|---------|---------|-------|---------|
| `vamptalk` | | Vampire | `CHANNEL_VAMPTALK` |
| `howl` | | Werewolf | `CHANNEL_HOWL` |
| `pray` | | Demon | `CHANNEL_PRAY` |
| `magetalk` | | Mage | `CHANNEL_MAGETALK` |
| `monktalk` | `hum` | Monk | `CHANNEL_MONK` |
| `miktalk` | | Ninja | `CHANNEL_MIKTALK` |
| `samtalk` | | Samurai | `CHANNEL_HIGHTALK` |
| `sign` | | Drow | `CHANNEL_SIGN` |
| `prayer` | | Angel | `CHANNEL_ANGEL` |
| `telepath` | | Shapeshifter | `CHANNEL_TELEPATH` |
| `preach` | | Spider Droid | `CHANNEL_COMMUNICATE` |
| `knighttalk` | | Undead Knight | `CHANNEL_KNIGHTTALK` |
| `lichtalk` | | Lich | `CHANNEL_LICHTALK` |
| `tantalk` | | Tanarri | `CHANNEL_TANTALK` |

### Faction Channel

| Command | Function | Restriction |
|---------|----------|-------------|
| `ktalk` | [do_ktalk](../../src/commands/act_comm.c#L486) | `ch->pcdata->kingdom != 0`; same kingdom only |

See [Kingdom System](../kingdoms/overview.md) for details.

### Local Communication

| Command | Aliases | Function | Scope | Notes |
|---------|---------|----------|-------|-------|
| `say` | `'` | [do_say](../../src/commands/act_comm.c#L619) | Room | Polymorph-aware voice changes; drunk slurring; triggers room text |
| `emote` | `,` | [do_emote](../../src/commands/act_comm.c#L1097) | Room | Requires tongue and head |
| `whisper` | | [do_whisper](../../src/commands/act_comm.c#L1018) | Room | Others see generic "whispers to X" |

### Direct Communication

| Command | Aliases | Function | Scope | Notes |
|---------|---------|----------|-------|-------|
| `tell` | | [do_tell](../../src/commands/act_comm.c#L958) | Global | Sets `ch->reply` for reply tracking |
| `reply` | | [do_reply](../../src/commands/act_comm.c#L1056) | Global | Replies to last person who sent a tell |
| `gtell` | `;` | [do_gtell](../../src/commands/act_comm.c#L1688) | Group | Group members only; works while sleeping |

### System Broadcasts

| Function | Location | Purpose |
|----------|----------|---------|
| `do_info` | [act_wiz.c:1279](../../src/commands/act_wiz.c#L1279) | `<- Info ->` broadcasts to all not deaf to `CHANNEL_INFO` |
| `do_watching` | [act_wiz.c:1297](../../src/commands/act_wiz.c#L1297) | Monitor broadcasts to players with `PLR_WATCHER` flag |

## Silence Mechanics

Two separate systems prevent communication:

### PLR_SILENCE (Immortal Punishment)

- Set by immortals on problem players
- Silenced players can still type messages — they appear to send normally
- Messages are silently dropped during `talk_channel()` broadcast (no error shown)
- Effective as an anti-grief tool because the player doesn't know they're silenced

### RTIMER_SILENCE (Room-Based)

- Per-room timer that blocks all communication from that room
- Checked at the start of `talk_channel()` before any broadcast
- Defined at [merc.h:534](../../src/core/merc.h#L534)

## Howl Distance System

The werewolf `howl` command is the only channel with proximity-based delivery. Since MUD rooms are organized into areas, "distance" here means organizational proximity — same room, same area, or a different area — not a physical coordinate system.

| Recipient Location | What They See |
|--------------------|---------------|
| Same room | Full message with speaker's name and action text |
| Same area, different room | Hears the howl (no speaker details) |
| Different area entirely | Hears a distant howl |

All other global channels are all-or-nothing: either you receive the full message or you don't.

**Location:** [act_comm.c:358-371](../../src/commands/act_comm.c#L358-L371)

## Say and Polymorph

`do_say` checks the speaker's current form and adjusts the verb:

| Form | Verb |
|------|------|
| `POLY_WOLF` | howls |
| `POLY_BAT` | screeches |
| `POLY_SERPENT` | hisses |
| `POLY_MIST` | whispers |
| Default | says |

Drunk characters also have their speech slurred.

**Location:** [act_comm.c:665-715](../../src/commands/act_comm.c#L665-L715)

## Socials System

**Location:** [socials.c](../../src/commands/socials.c)

Socials are predefined emotes with contextual message variants (50+ socials: bow, smile, hug, slap, etc.).

### Data Structure

**Location:** [merc.h:3039-3048](../../src/core/merc.h#L3039-L3048)

```c
struct social_type {
    char *name;            // e.g. "smile"
    char *char_no_arg;     // "You smile." (to actor, no target)
    char *others_no_arg;   // "Bob smiles." (to room, no target)
    char *char_found;      // "You smile at Alice." (to actor)
    char *others_found;    // "Bob smiles at Alice." (to room)
    char *vict_found;      // "Bob smiles at you." (to target)
    char *char_auto;       // "You smile at yourself." (self-target)
    char *others_auto;     // "Bob smiles at himself." (self-target, to room)
};
```

### Execution Flow

**Location:** [interp.c:1655-1750](../../src/core/interp.c#L1655-L1750) — `check_social()`

1. Command interpreter tries `check_social()` after failing to find a regular command
2. Searches `social_table[]` for a name match
3. Position checks — no socials while dead/incapacitated (exception: `snore` while sleeping)
4. Selects message variant based on target: no argument, target found, or self-target
5. NPC reaction — awake, uncharmed NPCs may counter-social (10/16 chance), slap, or attack

## GMCP Integration

**Location:** [gmcp.c](../../src/systems/gmcp.c), [gmcp.h](../../src/systems/gmcp.h)

Modern MUD clients receive structured data via GMCP alongside the text channel system:

| Message | Content | Trigger |
|---------|---------|---------|
| `Core.Hello` | Server identity | Connection |
| `Char.Vitals` | HP, mana, move | Game updates |
| `Char.Status` | Level, class, position, XP | State changes |
| `Char.Info` | Player name, guild | Login |

GMCP runs parallel to the text communication system — it sends structured game state data, not chat messages.

## Source Files

| File | Contents |
|------|----------|
| [act_comm.c](../../src/commands/act_comm.c) | All channel commands, `talk_channel()`, say, emote, tell, whisper |
| [act_info.c](../../src/commands/act_info.c) | `channels` toggle command, channel status display |
| [act_wiz.c:1279-1295](../../src/commands/act_wiz.c#L1279-L1295) | `do_info` system broadcast |
| [socials.c](../../src/commands/socials.c) | Social table and `do_socials` command |
| [interp.c:1655-1750](../../src/core/interp.c#L1655-L1750) | `check_social()` dispatcher |
| [merc.h:2028-2052](../../src/core/merc.h#L2028-L2052) | `CHANNEL_*` constants |
| [gmcp.c](../../src/systems/gmcp.c) | GMCP structured data protocol |
