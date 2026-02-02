# Sound File Inventory

Sound files live in the `audio/` directory at the repository root and are served via `raw.githubusercontent.com`. The base URL is configured in [mcmp.h](../../src/systems/mcmp.h):

```c
#define MCMP_DEFAULT_URL "https://raw.githubusercontent.com/tmould1/dystopia-mud/main/audio/"
```

All files should be in **MP3** format for broad client support (Mudlet uses Windows Media Foundation on Windows, which requires MP3).

## Setup

1. Create the `audio/` directory at the repository root with the subdirectory structure shown below.

2. Add MP3 sound files matching the filenames listed in the tables.

3. Commit and push to `main`. Files are immediately available via `raw.githubusercontent.com`.

### Directory Structure

```
audio/
  ambient/
    indoor.mp3
    city.mp3
    field.mp3
    forest.mp3
    hills.mp3
    mountain.mp3
    water.mp3
    desert.mp3
    air.mp3
  channels/
    chat.mp3
    tell.mp3
    yell.mp3
    class.mp3
    immtalk.mp3
  combat/
    engage.mp3
    light_hit.mp3
    heavy_hit.mp3
    miss.mp3
    death.mp3
    victory.mp3
  movement/
    footstep_stone.mp3
    footstep_grass.mp3
    footstep_forest.mp3
    footstep_gravel.mp3
    footstep_sand.mp3
    splash.mp3
    whoosh.mp3
  specials/
    spell_cast.mp3
    spell_heal.mp3
    spell_damage.mp3
    vampire_drain.mp3
    werewolf_howl.mp3
    demon_fire.mp3
    angel_smite.mp3
  environment/
    dawn.mp3
    sunrise.mp3
    sunset.mp3
    nightfall.mp3
    clock_midnight.mp3
    bell_distant.mp3
    door_open.mp3
    door_close.mp3
    howl.mp3
  ui/
    login.mp3
    levelup.mp3
    death.mp3
    achievement.mp3
  weather/
    rain_light.mp3
    rain_heavy.mp3
    thunder.mp3
    wind.mp3
```

**Total: ~46 sound files** across 8 directories.

## Sound File Reference

### Combat Sounds (`combat/`)

One-shot sounds played once per combat round or event.

| File | Trigger | Tag | Volume | Caption |
|------|---------|-----|--------|---------|
| `engage.mp3` | Combat starts (`mcmp_combat_start`) | combat | 50 | "Combat begins" |
| `light_hit.mp3` | Round: damage dealt, <= 2000 total | combat | 40 | "Light blows exchanged" |
| `heavy_hit.mp3` | Round: damage dealt, > 2000 total | combat | 50 | "Heavy blows land" |
| `miss.mp3` | Round: zero damage dealt (all misses) | combat | 30 | "Attacks miss" |
| `death.mp3` | Target killed (heard by killer) | combat | 60 | "A death cry rings out" |
| `victory.mp3` | Last enemy killed, combat ends | combat | 50 | "Victory" |

### Special Ability Sounds (`specials/`)

Per-use sounds for spells and class abilities. Currently `spell_cast.mp3` is the only one wired in. The others are available for future hooks.

| File | Trigger | Tag | Caption |
|------|---------|-----|---------|
| `spell_cast.mp3` | Generic spell cast | spell | "A spell is cast" |
| `spell_heal.mp3` | Healing spell (future) | spell | "Healing energy flows" |
| `spell_damage.mp3` | Damage spell (future) | spell | "Destructive magic unleashed" |
| `vampire_drain.mp3` | Vampire blood drain (future) | spell | "Life force drained" |
| `werewolf_howl.mp3` | Werewolf howl ability (future) | spell | "A primal howl echoes" |
| `demon_fire.mp3` | Demon fire attack (future) | spell | "Hellfire erupts" |
| `angel_smite.mp3` | Angel smite (future) | spell | "Divine light strikes" |

### Movement Sounds (`movement/`)

One-shot footstep sounds played on each room transition, selected by destination sector type.

| File | Sector Types | Tag | Caption |
|------|-------------|-----|---------|
| `footstep_stone.mp3` | SECT_INSIDE (0), SECT_CITY (1) | movement | "Footsteps on stone" |
| `footstep_grass.mp3` | SECT_FIELD (2) | movement | "Footsteps on grass" |
| `footstep_forest.mp3` | SECT_FOREST (3) | movement | "Footsteps through undergrowth" |
| `footstep_gravel.mp3` | SECT_HILLS (4), SECT_MOUNTAIN (5) | movement | "Footsteps on gravel" |
| `splash.mp3` | SECT_WATER_SWIM (6), SECT_WATER_NOSWIM (7) | movement | "Splashing through water" |
| `whoosh.mp3` | SECT_AIR (9) | movement | "Wind rushing past" |
| `footstep_sand.mp3` | SECT_DESERT (10) | movement | "Footsteps on sand" |

Sector-to-file mapping is defined in [mcmp.c](../../src/systems/mcmp.c) in the `sector_footstep[]` and `sector_footstep_caption[]` arrays.

### Ambient Loops (`ambient/`)

Continuous background loops that play while the player is in a given sector type. Uses `type: "music"`, `loops: -1` (infinite), `key: "ambient"`, `continue: true`.

| File | Sector Type | Tag | Caption |
|------|------------|-----|---------|
| `indoor.mp3` | SECT_INSIDE (0) | environment | "Indoor ambience" |
| `city.mp3` | SECT_CITY (1) | environment | "City sounds" |
| `field.mp3` | SECT_FIELD (2) | environment | "Open field, wind blowing" |
| `forest.mp3` | SECT_FOREST (3) | environment | "Forest, birds and rustling" |
| `hills.mp3` | SECT_HILLS (4) | environment | "Hilly terrain, wind" |
| `mountain.mp3` | SECT_MOUNTAIN (5) | environment | "Mountain wind, distant echoes" |
| `water.mp3` | SECT_WATER_SWIM (6), SECT_WATER_NOSWIM (7) | environment | "Water flowing" |
| `air.mp3` | SECT_AIR (9) | environment | "High altitude wind" |
| `desert.mp3` | SECT_DESERT (10) | environment | "Desert wind" |

Ambient sounds only change when the player moves between rooms with different sector types. Moving within the same sector type does not restart the loop (due to `continue: true`).

Sector-to-file mapping is defined in [mcmp.c](../../src/systems/mcmp.c) in the `sector_ambient[]` and `sector_ambient_caption[]` arrays.

### Weather Sounds (`weather/`)

Overlay sounds that play on top of ambient when weather changes. Uses `key: "weather"` with `loops: -1` for persistent weather, or one-shot for thunder.

| File | Sky State | Type | Loops | Caption |
|------|-----------|------|-------|---------|
| `wind.mp3` | SKY_CLOUDY | music | -1 | "Wind picks up" |
| `rain_light.mp3` | *(available for future use)* | music | -1 | "Light rain begins" |
| `rain_heavy.mp3` | SKY_RAINING | music | -1 | "Heavy rain" |
| `thunder.mp3` | SKY_LIGHTNING | sound | 1 | "Thunder crashes" |

When skies clear (SKY_CLOUDLESS), the server sends `Client.Media.Stop` with `key: "weather"`, `fadeaway: true`, `fadeout: 3000` to fade out weather sounds over 3 seconds.

### Environment Sounds (`environment/`)

One-shot environmental sounds triggered by game events — time-of-day transitions, area resets, door interactions, and werewolf howls. All use `tag: "environment"`.

#### Time of Day

Broadcast to all outdoor, awake players at each time-of-day transition. Triggered by `mcmp_time_of_day()` in [mcmp.c](../../src/systems/mcmp.c), called from `weather_update()` in [update.c](../../src/systems/update.c).

| File | Hour | Text Message | Volume | Priority | Caption |
|------|------|-------------|--------|----------|---------|
| `dawn.mp3` | 5 | "The day has begun." | 30 | 30 | "Dawn breaks" |
| `sunrise.mp3` | 6 | "The sun rises in the east." | 30 | 30 | "Birds sing at sunrise" |
| `sunset.mp3` | 19 | "The sun slowly disappears in the west." | 30 | 30 | "Evening crickets" |
| `nightfall.mp3` | 20 | "The night has begun." | 30 | 30 | "Night creatures stir" |
| `clock_midnight.mp3` | 0 (24) | "You hear a clock in the distance strike midnight." | 35 | 40 | "Clock tolls midnight" |

#### Area Reset Bell

Sent to all awake players in an area when it is about to reset (~1 tick before reset). Direct `mcmp_play()` call in `area_update()` in [db.c](../../src/core/db.c).

| File | Trigger | Volume | Priority | Caption |
|------|---------|--------|----------|---------|
| `bell_distant.mp3` | Area age reaches reset-1 | 25 | 20 | "Distant bell tolls" |

#### Doors

Played to the actor when opening or closing a door (not containers). Direct `mcmp_play()` calls in `do_open()` and `do_close()` in [act_move.c](../../src/commands/act_move.c).

| File | Trigger | Volume | Priority | Caption |
|------|---------|--------|----------|---------|
| `door_open.mp3` | Player opens a door | 35 | 20 | "Door creaks open" |
| `door_close.mp3` | Player closes a door | 35 | 20 | "Door thuds shut" |

#### Werewolf Howl

Played at three volume tiers based on distance from the howler. Non-werewolves hear a generic howl instead of the channel message. Direct `mcmp_play()` calls in `talk_channel()` in [act_comm.c](../../src/commands/act_comm.c).

| File | Distance | Volume | Priority | Caption |
|------|----------|--------|----------|---------|
| `howl.mp3` | Same room | 60 | 30 | "A loud howl" |
| `howl.mp3` | Same area | 35 | 30 | "A howl nearby" |
| `howl.mp3` | Different area | 15 | 30 | "A distant howl" |

The same sound file is used at all three distances — the volume parameter creates the perception of distance.

### Channel Notification Sounds (`channels/`)

Short notification sounds played when receiving a message.

| File | Channel | Tag | Caption |
|------|---------|-----|---------|
| `chat.mp3` | CHANNEL_CHAT and all other public channels | channel | "Chat message" |
| `tell.mp3` | CHANNEL_TELL (private messages) | channel | "Private message" |
| `yell.mp3` | CHANNEL_YELL (area-local) | channel | "Someone yells" |
| `class.mp3` | *(available for future class-specific routing)* | channel | "Class channel message" |
| `immtalk.mp3` | CHANNEL_IMMTALK (immortal) | channel | "Immortal message" |

Channel-to-file mapping is in `mcmp_channel_notify()` in [mcmp.c](../../src/systems/mcmp.c).

### UI Sounds (`ui/`)

Event sounds for significant player milestones.

| File | Event String | Trigger | Caption |
|------|-------------|---------|---------|
| `login.mp3` | `"login"` | Player enters the game (new or returning) | "Welcome" |
| `levelup.mp3` | `"levelup"` | Player gains a level (future hook) | "Level up" |
| `death.mp3` | `"death"` | Player dies (sent to victim via `mcmp_combat_death`) | "You have died" |
| `achievement.mp3` | *(default)* | Quest/achievement (future hook) | "Achievement earned" |

## Caption Writing Guidelines

Every sound file must have a corresponding `caption` string in its `mcmp_play()` call. Captions provide accessibility for hearing-impaired players — clients display them as subtitles or pass them to screen readers.

Follow the [Captioning Key: Sound Effects and Music](https://www.captioningkey.org/) standard referenced by the MCMP spec:

1. **Keep it short** — 2-5 words. Captions appear inline with game text.
2. **Describe the sound, not the mechanic** — "Thunder crashes" not "Weather state 3".
3. **Use present tense** — "Wind picks up" not "Wind picked up".
4. **Be distinct within each category** — Don't reuse the same caption for different sounds.
5. **Match the tables below** — Each sound file table includes the required caption text.

See [mcmp.md — Caption Accessibility](mcmp.md#caption-accessibility) for the full convention guide.

## Sound Design Guidelines

When creating or sourcing sound files:

- **Format:** MP3 (`.mp3`). Universally supported by Mudlet's Windows Media Foundation backend and all other MCMP clients.
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

All file paths in `mcmp.c` are relative to this base URL (e.g., `"combat/engage.mp3"` becomes `https://raw.githubusercontent.com/tmould1/dystopia-mud/main/audio/combat/engage.mp3`).
