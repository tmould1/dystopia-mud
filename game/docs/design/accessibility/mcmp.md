# MCMP — MUD Client Media Protocol

MCMP delivers sounds, music, and ambient audio to clients that support the `Client.Media` GMCP package. It is defined by the [MUD Client Media Protocol specification](https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol) (version 1.0.3).

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

All messages are server-to-client only. There are no client-to-server media messages.

### Client Support

MCMP is supported by:

- **Mudlet** 4.4+ (native support)
- **BeipMU** 4.00.298+
- **LociTerm** 2.5.1+
- Any GMCP-capable client with a `Client.Media` plugin

## Protocol Parameter Reference

This section maps every parameter from the MCMP specification to our implementation.

### Client.Media.Default Parameters

| Spec Parameter | Type | Required | Our Status | Notes |
|---------------|------|----------|------------|-------|
| `url` | string | Yes | **Supported** | Base directory URL, must end with `/` |

### Client.Media.Load Parameters

| Spec Parameter | Type | Required | Our Status | Notes |
|---------------|------|----------|------------|-------|
| `name` | string | Yes | **Supported** | Path relative to base URL |
| `url` | string | Conditional | **Omitted** | Not needed — base URL set via Default |

### Client.Media.Play Parameters

| Spec Parameter | Type | Default | Our Status | Notes |
|---------------|------|---------|------------|-------|
| `name` | string | *(required)* | **Supported** | Path relative to base URL, e.g. `"combat/engage.mp3"` |
| `url` | string | *(none)* | **Omitted** | Not needed — base URL set once via Default |
| `type` | string | `"sound"` | **Supported** | `"sound"` or `"music"` — controls client mixing |
| `tag` | string | *(none)* | **Supported** | Category for bulk stop, e.g. `"combat"`, `"weather"` |
| `volume` | int | 50 | **Supported** | 1-100, relative to client master volume |
| `fadein` | int (ms) | *(none)* | **Omitted** | Linear volume ramp at start — no current use case |
| `fadeout` | int (ms) | *(none)* | **Omitted** | Linear volume ramp at end — no current use case |
| `start` | int (ms) | 0 | **Omitted** | Playback start offset — no current use case |
| `finish` | int (ms) | 0 | **Omitted** | Playback end offset — no current use case |
| `loops` | int | 1 | **Supported** | `-1` = infinite, `1+` = finite count |
| `priority` | int | *(none)* | **Supported** | 1-100, halts lower-priority sounds during playback |
| `continue` | bool | true | **Supported** | `true` = don't restart if same file already playing |
| `key` | string | *(none)* | **Supported** | Binding key — same-key sounds replace each other |
| `caption` | string | *(none)* | **Supported** | Accessibility text for hearing-impaired players |

### Client.Media.Stop Parameters

| Spec Parameter | Type | Default | Our Status | Notes |
|---------------|------|---------|------------|-------|
| `name` | string | *(none)* | **Supported** | Stop by filename match |
| `type` | string | *(none)* | **Supported** | Stop by media type (`"sound"` or `"music"`) |
| `tag` | string | *(none)* | **Supported** | Stop by tag category |
| `priority` | int | *(none)* | **Omitted** | Stop sounds at or below threshold — no current use case |
| `key` | string | *(none)* | **Supported** | Stop by binding key |
| `fadeaway` | bool | *(none)* | **Supported** | Apply volume decrease before stopping |
| `fadeout` | int (ms) | 5000 | **Supported** | Fade duration (spec defaults to 5000 if fadeaway set without fadeout) |

An empty body `{}` stops all media.

### Omitted Parameters — Rationale

| Parameter | Message | Why Omitted |
|-----------|---------|-------------|
| `url` | Play, Load | We set the base URL once via `Client.Media.Default` at connection time. All filenames are relative paths. |
| `fadein` | Play | No game event currently needs a volume ramp-in. Can be added to `mcmp_play()` if needed. |
| `fadeout` | Play | No game event needs per-file fade-out. We use `Client.Media.Stop` with `fadeout` for weather transitions instead. |
| `start` / `finish` | Play | No partial-playback use case. All sounds play from beginning to end. |
| `priority` | Stop | No need to stop sounds by priority threshold. We stop by key or tag instead. |

## Caption Accessibility

Every `Client.Media.Play` message includes a `caption` parameter — a short text description of the sound for hearing-impaired players. Clients that support captions (spec v1.0.3+) can display these as subtitles or pass them to screen readers.

### Caption Writing Conventions

Our captions follow the [Captioning Key: Sound Effects and Music](https://www.captioningkey.org/) guidelines referenced by the MCMP spec:

| Sound Category | Convention | Examples |
|---------------|------------|----------|
| **Ambient/Environment** | Present-tense description of the soundscape | "Forest, birds and rustling", "City sounds", "Water flowing" |
| **Weather** | Present-tense description of the weather event | "Wind picks up", "Heavy rain", "Thunder crashes" |
| **Movement/Footsteps** | Surface-specific onomatopoeia | "Footsteps on stone", "Splashing through water", "Wind rushing past" |
| **Combat** | Short descriptive phrases | "Heavy blows land", "Light blows exchanged", "Attacks miss" |
| **Death/Victory** | Event announcement | "A death cry rings out", "Victory", "You have died" |
| **Channels** | Message type context | "Private message", "Chat message", "Someone yells" |
| **UI events** | Imperative labels | "Welcome", "Level up", "Achievement earned" |
| **Spells** | Action description | "A spell is cast" |

### Caption Guidelines for New Sounds

When adding new MCMP sounds, always include a caption. Follow these rules:

1. **Keep it short** — 2-5 words is ideal. Captions appear as subtitles alongside game text.
2. **Describe the sound, not the mechanic** — "Thunder crashes" not "Weather state changed to lightning".
3. **Use present tense** — "Wind picks up" not "Wind picked up".
4. **Avoid jargon** — "A death cry rings out" not "Target HP reached zero".
5. **Be distinct** — Each caption should be distinguishable from others in the same category. Don't reuse the same caption for different sounds.

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

| Key | Type | Loops | Continue | Priority | Purpose |
|-----|------|-------|----------|----------|---------|
| `"ambient"` | music | -1 (infinite) | true | 10 | Sector-based ambient loop |
| `"weather"` | music | -1 (infinite) | false | 20 | Weather overlay |
| *(none)* | sound | 1 | false | 20-90 | One-shot sounds (combat, channels, UI) |

- **Ambient** sounds use `continue: true` so walking between rooms of the same sector type doesn't restart the loop. When the sector changes, the new file has a different name, so the client starts the new ambient and the old one (same key) is implicitly replaced.
- **Weather** sounds overlay on top of ambient (different key). Weather uses `continue: false` so changing weather (e.g., wind -> rain) replaces the previous weather sound. When skies clear, a `Client.Media.Stop` with `fadeaway: true` and `fadeout: 3000` fades them out over 3 seconds.
- **One-shot** sounds (combat, channels, UI) have no key and play independently of all layers.

### Combat Sound Pacing

Combat in Dystopia does 2-30+ individual attacks per 0.75-second round via `multi_hit()`. Sending a sound per attack would flood the client.

The solution: `violence_update()` captures the victim's HP before calling `multi_hit()`, then calculates the total damage delta afterward and calls `mcmp_combat_round()` exactly once with a summary:

- `total_dam > 2000` -> `combat/heavy_hit.mp3` (volume 50, priority 40)
- `total_dam > 0` -> `combat/light_hit.mp3` (volume 40, priority 30)
- `total_dam == 0` (all misses) -> `combat/miss.mp3` (volume 30, priority 30)

### Priority Hierarchy

| Priority Range | Usage |
|---------------|-------|
| 80-90 | Death events (victim death UI, killer death sound) |
| 70 | UI milestones (login, levelup, achievement) |
| 50-60 | Combat engagement, victory, thunder |
| 30-40 | Per-round combat, spells |
| 20 | Channels, weather loops |
| 10 | Ambient loops, footsteps |

## MSSP Advertisement

**Location:** [mssp.c](../../src/systems/mssp.c)

The MSSP response includes `MCMP = 1` so MUD listing sites (like The MUD Connector, Grapevine, etc.) can advertise MCMP support.
