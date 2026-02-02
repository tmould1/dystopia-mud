# MCMP — MUD Client Media Protocol

MCMP delivers sounds, music, and ambient audio to clients that support the `Client.Media` GMCP package. It is defined by the [MUD Client Media Protocol specification](https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol).

**Location:** [mcmp.h](../../src/systems/mcmp.h), [mcmp.c](../../src/systems/mcmp.c)

## Protocol Overview

MCMP rides on GMCP — no separate telnet option is needed. The client advertises support via:

```
Core.Supports.Set ["Client.Media 1"]
```

The server then sets the `GMCP_PACKAGE_CLIENT_MEDIA` bit on the descriptor and sends `Client.Media.Default` with the base URL for all sound files:

```json
Client.Media.Default {"url":"https://raw.githubusercontent.com/tmould1/dystopia-mud/main/audio/"}
```

All subsequent `Client.Media.Play` messages reference files relative to this base URL.

### GMCP Messages Used

| Message | Direction | Purpose |
|---------|-----------|---------|
| `Client.Media.Default` | Server -> Client | Set base URL for media files |
| `Client.Media.Load` | Server -> Client | Preload a media file |
| `Client.Media.Play` | Server -> Client | Play a sound or music file |
| `Client.Media.Stop` | Server -> Client | Stop playing media |

### Client Support

MCMP is supported by:

- **Mudlet** 4.4+ (native support)
- Any GMCP-capable client with a `Client.Media` plugin

## Configuration

### Player Toggle

Players toggle MCMP via:

```
config mcmp
```

The preference is saved as `PLR_PREFER_MCMP` (bit 22) on the player's `act` flags and restored on reconnect.

New players who connect with a client that advertises `Client.Media` support automatically get `PLR_PREFER_MCMP` set.

### Audio Base URL

The base URL for all sound files is a runtime gameconfig setting:

```
gameconfig audio_url <url>
```

**Default:** `https://raw.githubusercontent.com/tmould1/dystopia-mud/main/audio/`

The URL must end with a trailing slash. If the gameconfig value is empty, the compile-time default (`MCMP_DEFAULT_URL` in [mcmp.h](../../src/systems/mcmp.h)) is used as a fallback.

**Location:** [config.c](../../src/core/config.c), [db_game.c](../../src/db/db_game.c)

## Implementation

### Core Functions

**Location:** [mcmp.c](../../src/systems/mcmp.c)

| Function | Purpose |
|----------|---------|
| `mcmp_enabled(d)` | Check if descriptor supports Client.Media |
| `mcmp_set_default(d)` | Send `Client.Media.Default` with base URL |
| `mcmp_load(d, name)` | Preload a media file on the client |
| `mcmp_play(d, name, type, tag, volume, loops, priority, key, cont, caption)` | Play a media file |
| `mcmp_stop(d, name, type, tag, key, fadeaway, fadeout)` | Stop media playback |

### Game Event Functions

All game event functions check `mcmp_enabled()` internally and are safe to call unconditionally.

| Function | Trigger Location | Purpose |
|----------|-----------------|---------|
| `mcmp_combat_round(ch, victim, hits, misses, total_dam)` | [fight.c — violence_update()](../../src/combat/fight.c) | Per-round combat sound |
| `mcmp_combat_death(ch, victim)` | [fight.c — hurt_person()](../../src/combat/fight.c) | Death sound (killer + victim) |
| `mcmp_combat_start(ch, victim)` | Available, not yet hooked | Engagement sound |
| `mcmp_combat_end(ch)` | [fight.c — hurt_person()](../../src/combat/fight.c) | Victory sound |
| `mcmp_spell_cast(ch, sn)` | Available for magic.c | Spell casting sound |
| `mcmp_movement(ch, sector_to)` | [act_move.c — move_char()](../../src/commands/act_move.c) | Footstep sound |
| `mcmp_room_ambient(ch, sector_type)` | [act_move.c — move_char()](../../src/commands/act_move.c) | Ambient loop change |
| `mcmp_weather_change(ch, sky_state)` | [update.c — weather_update()](../../src/systems/update.c) | Weather overlay sound |
| `mcmp_channel_notify(ch, channel_type)` | [act_comm.c — talk_channel(), do_tell()](../../src/commands/act_comm.c) | Channel notification |
| `mcmp_ui_event(ch, event)` | [nanny.c](../../src/core/nanny.c) | Login, levelup, death, achievement |

### Audio Layering Strategy

MCMP uses **keys** to manage concurrent audio streams. Sounds with the same key replace each other; sounds with different keys overlay.

| Key | Type | Loops | Continue | Purpose |
|-----|------|-------|----------|---------|
| `"ambient"` | music | -1 (infinite) | true | Sector-based ambient loop |
| `"weather"` | music | -1 (infinite) | false | Weather overlay |
| *(none)* | sound | 1 | false | One-shot sounds (combat, channels, UI) |

- **Ambient** sounds use `continue: true` so walking between rooms of the same sector type doesn't restart the loop.
- **Weather** sounds overlay on top of ambient. When skies clear, a `Client.Media.Stop` with `fadeaway: true` and `fadeout: 3000` fades them out.
- **One-shot** sounds (combat, channels, UI) have no key and play independently.

### Combat Sound Pacing

Combat in Dystopia does 2-30+ individual attacks per 0.75-second round via `multi_hit()`. Sending a sound per attack would flood the client.

The solution: `violence_update()` captures the victim's HP before calling `multi_hit()`, then calculates the total damage delta afterward and calls `mcmp_combat_round()` exactly once with a summary:

- `total_dam > 2000` -> `combat/heavy_hit.ogg`
- `total_dam > 0` -> `combat/light_hit.ogg`
- `total_dam == 0` (all misses) -> `combat/miss.ogg`

## MSSP Advertisement

**Location:** [mssp.c](../../src/systems/mssp.c)

The MSSP response includes `MCMP = 1` so MUD listing sites (like The MUD Connector, Grapevine, etc.) can advertise MCMP support.
