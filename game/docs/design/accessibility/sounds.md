# Sound File Inventory

Sound files live in the `audio/` directory at the repository root and are served via `raw.githubusercontent.com`. The base URL is configured in [mcmp.h](../../src/systems/mcmp.h):

```c
#define MCMP_DEFAULT_URL "https://raw.githubusercontent.com/tmould1/dystopia-mud/main/audio/"
```

All files should be in **OGG Vorbis** format for broad client support.

## Setup

1. Create the `audio/` directory at the repository root with the subdirectory structure shown below.

2. Add OGG Vorbis sound files matching the filenames listed in the tables.

3. Commit and push to `main`. Files are immediately available via `raw.githubusercontent.com`.

### Directory Structure

```
audio/
  ambient/
    indoor.ogg
    city.ogg
    field.ogg
    forest.ogg
    hills.ogg
    mountain.ogg
    water.ogg
    desert.ogg
    air.ogg
  channels/
    chat.ogg
    tell.ogg
    yell.ogg
    class.ogg
    immtalk.ogg
  combat/
    engage.ogg
    light_hit.ogg
    heavy_hit.ogg
    miss.ogg
    death.ogg
    victory.ogg
  movement/
    footstep_stone.ogg
    footstep_grass.ogg
    footstep_forest.ogg
    footstep_gravel.ogg
    footstep_sand.ogg
    splash.ogg
    whoosh.ogg
  specials/
    spell_cast.ogg
    spell_heal.ogg
    spell_damage.ogg
    vampire_drain.ogg
    werewolf_howl.ogg
    demon_fire.ogg
    angel_smite.ogg
  ui/
    login.ogg
    levelup.ogg
    death.ogg
    achievement.ogg
  weather/
    rain_light.ogg
    rain_heavy.ogg
    thunder.ogg
    wind.ogg
```

**Total: ~37 sound files** across 7 directories.

## Sound File Reference

### Combat Sounds (`combat/`)

One-shot sounds played once per combat round or event.

| File | Trigger | Tag | Volume | Caption |
|------|---------|-----|--------|---------|
| `engage.ogg` | Combat starts (`mcmp_combat_start`) | combat | 50 | "Combat begins" |
| `light_hit.ogg` | Round: damage dealt, <= 2000 total | combat | 40 | "Light blows exchanged" |
| `heavy_hit.ogg` | Round: damage dealt, > 2000 total | combat | 50 | "Heavy blows land" |
| `miss.ogg` | Round: zero damage dealt (all misses) | combat | 30 | "Attacks miss" |
| `death.ogg` | Target killed (heard by killer) | combat | 60 | "A death cry rings out" |
| `victory.ogg` | Last enemy killed, combat ends | combat | 50 | "Victory" |

### Special Ability Sounds (`specials/`)

Per-use sounds for spells and class abilities. Currently `spell_cast.ogg` is the only one wired in. The others are available for future hooks.

| File | Trigger | Tag | Caption |
|------|---------|-----|---------|
| `spell_cast.ogg` | Generic spell cast | spell | "A spell is cast" |
| `spell_heal.ogg` | Healing spell (future) | spell | "Healing energy flows" |
| `spell_damage.ogg` | Damage spell (future) | spell | "Destructive magic unleashed" |
| `vampire_drain.ogg` | Vampire blood drain (future) | spell | "Life force drained" |
| `werewolf_howl.ogg` | Werewolf howl (future) | spell | "A primal howl echoes" |
| `demon_fire.ogg` | Demon fire attack (future) | spell | "Hellfire erupts" |
| `angel_smite.ogg` | Angel smite (future) | spell | "Divine light strikes" |

### Movement Sounds (`movement/`)

One-shot footstep sounds played on each room transition, selected by destination sector type.

| File | Sector Types | Tag | Caption |
|------|-------------|-----|---------|
| `footstep_stone.ogg` | SECT_INSIDE (0), SECT_CITY (1) | movement | "Footsteps on stone" |
| `footstep_grass.ogg` | SECT_FIELD (2) | movement | "Footsteps on grass" |
| `footstep_forest.ogg` | SECT_FOREST (3) | movement | "Footsteps through undergrowth" |
| `footstep_gravel.ogg` | SECT_HILLS (4), SECT_MOUNTAIN (5) | movement | "Footsteps on gravel" |
| `splash.ogg` | SECT_WATER_SWIM (6), SECT_WATER_NOSWIM (7) | movement | "Splashing through water" |
| `whoosh.ogg` | SECT_AIR (9) | movement | "Wind rushing past" |
| `footstep_sand.ogg` | SECT_DESERT (10) | movement | "Footsteps on sand" |

Sector-to-file mapping is defined in [mcmp.c](../../src/systems/mcmp.c) in the `sector_footstep[]` and `sector_footstep_caption[]` arrays.

### Ambient Loops (`ambient/`)

Continuous background loops that play while the player is in a given sector type. Uses `type: "music"`, `loops: -1` (infinite), `key: "ambient"`, `continue: true`.

| File | Sector Type | Tag | Caption |
|------|------------|-----|---------|
| `indoor.ogg` | SECT_INSIDE (0) | environment | "Indoor ambience" |
| `city.ogg` | SECT_CITY (1) | environment | "City sounds" |
| `field.ogg` | SECT_FIELD (2) | environment | "Open field, wind blowing" |
| `forest.ogg` | SECT_FOREST (3) | environment | "Forest, birds and rustling" |
| `hills.ogg` | SECT_HILLS (4) | environment | "Hilly terrain, wind" |
| `mountain.ogg` | SECT_MOUNTAIN (5) | environment | "Mountain wind, distant echoes" |
| `water.ogg` | SECT_WATER_SWIM (6), SECT_WATER_NOSWIM (7) | environment | "Water flowing" |
| `air.ogg` | SECT_AIR (9) | environment | "High altitude wind" |
| `desert.ogg` | SECT_DESERT (10) | environment | "Desert wind" |

Ambient sounds only change when the player moves between rooms with different sector types. Moving within the same sector type does not restart the loop (due to `continue: true`).

Sector-to-file mapping is defined in [mcmp.c](../../src/systems/mcmp.c) in the `sector_ambient[]` and `sector_ambient_caption[]` arrays.

### Weather Sounds (`weather/`)

Overlay sounds that play on top of ambient when weather changes. Uses `key: "weather"` with `loops: -1` for persistent weather, or one-shot for thunder.

| File | Sky State | Type | Loops | Caption |
|------|-----------|------|-------|---------|
| `wind.ogg` | SKY_CLOUDY | music | -1 | "Wind picks up" |
| `rain_light.ogg` | *(available for future use)* | music | -1 | "Light rain begins" |
| `rain_heavy.ogg` | SKY_RAINING | music | -1 | "Heavy rain" |
| `thunder.ogg` | SKY_LIGHTNING | sound | 1 | "Thunder crashes" |

When skies clear (SKY_CLOUDLESS), the server sends `Client.Media.Stop` with `key: "weather"`, `fadeaway: true`, `fadeout: 3000` to fade out weather sounds over 3 seconds.

### Channel Notification Sounds (`channels/`)

Short notification sounds played when receiving a message.

| File | Channel | Tag | Caption |
|------|---------|-----|---------|
| `chat.ogg` | CHANNEL_CHAT and all other public channels | channel | "Chat message" |
| `tell.ogg` | CHANNEL_TELL (private messages) | channel | "Private message" |
| `yell.ogg` | CHANNEL_YELL (area-local) | channel | "Someone yells" |
| `class.ogg` | *(available for future class-specific routing)* | channel | "Class channel message" |
| `immtalk.ogg` | CHANNEL_IMMTALK (immortal) | channel | "Immortal message" |

Channel-to-file mapping is in `mcmp_channel_notify()` in [mcmp.c](../../src/systems/mcmp.c).

### UI Sounds (`ui/`)

Event sounds for significant player milestones.

| File | Event String | Trigger | Caption |
|------|-------------|---------|---------|
| `login.ogg` | `"login"` | Player enters the game (new or returning) | "Welcome" |
| `levelup.ogg` | `"levelup"` | Player gains a level (future hook) | "Level up" |
| `death.ogg` | `"death"` | Player dies (sent to victim via `mcmp_combat_death`) | "You have died" |
| `achievement.ogg` | *(default)* | Quest/achievement (future hook) | "Achievement earned" |

## Sound Design Guidelines

When creating or sourcing sound files:

- **Format:** OGG Vorbis (`.ogg`). Broadest client support across Mudlet and other MCMP implementations.
- **Sample rate:** 44100 Hz or 22050 Hz. Lower rate is fine for ambient loops to save bandwidth.
- **Channels:** Mono is preferred (smaller files, no spatial confusion). Stereo is acceptable for ambient.
- **Duration:**
  - One-shot sounds (combat, channels, UI): 0.5-3 seconds
  - Ambient loops: 15-60 seconds, seamlessly loopable
  - Weather loops: 10-30 seconds, seamlessly loopable
- **Volume normalization:** Normalize all files to approximately the same peak level. The server controls relative volume via the MCMP `volume` parameter (1-100).
- **Licensing:** Use royalty-free sounds or create originals. Good free sources:
  - [Freesound.org](https://freesound.org) (CC0 and CC-BY)
  - [OpenGameArt.org](https://opengameart.org) (various CC licenses)
  - [Kenney.nl](https://kenney.nl) (CC0)

## Changing the Sound Hosting Location

To host sounds somewhere other than the repository's `audio/` directory:

1. Edit `MCMP_DEFAULT_URL` in [mcmp.h](../../src/systems/mcmp.h):
   ```c
   #define MCMP_DEFAULT_URL "https://your-server.com/sounds/"
   ```
   The URL must end with a trailing slash.

2. Rebuild the server.

3. Ensure the hosting location serves files with correct MIME types and allows cross-origin access if needed by web-based clients.

All file paths in `mcmp.c` are relative to this base URL (e.g., `"combat/engage.ogg"` becomes `https://raw.githubusercontent.com/tmould1/dystopia-mud/main/audio/combat/engage.ogg`).
