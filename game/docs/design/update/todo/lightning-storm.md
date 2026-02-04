# Irregular Lightning Flashes During Storms

## Problem
Currently, "Lightning flashes in the sky" only appears once when entering the `SKY_LIGHTNING` weather state. The storm then continues silently (no text, no audio) until it transitions to another weather state.

## Solution
Add random lightning flash messages AND audio that occur at irregular intervals while `weather_info.sky == SKY_LIGHTNING`.

## Implementation

### Files to Modify
- [update.c](../../../src/systems/update.c) - `weather_update()` function (lines 574-756)
- [mcmp.c](../../../src/systems/mcmp.c) - Add `mcmp_lightning_flash()` function
- [mcmp.h](../../../src/systems/mcmp.h) - Add function prototype

---

### Part 1: Text Messages (update.c)

**1a. Add lightning flash messages array (at file scope, near top of file)**

```c
static const char *lightning_msgs[] = {
    "Lightning flashes in the sky.\n\r",
    "A bolt of lightning streaks across the sky.\n\r",
    "Thunder rumbles overhead.\n\r",
    "The sky lights up with a brilliant flash.\n\r",
    "A deafening crack of thunder shakes the air.\n\r"
};
#define NUM_LIGHTNING_MSGS (sizeof(lightning_msgs) / sizeof(lightning_msgs[0]))
```

**1b. Add lightning flash check in `weather_update()` (around line 700, before the weather state switch)**

```c
/* Periodic lightning flashes during storms */
if ( weather_info.sky == SKY_LIGHTNING && number_bits( 2 ) == 0 )
{
    const char *flash_msg = lightning_msgs[number_range( 0, NUM_LIGHTNING_MSGS - 1 )];

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
            && IS_OUTSIDE( d->character )
            && IS_AWAKE( d->character ) )
        {
            send_to_char( flash_msg, d->character );
            mcmp_lightning_flash( d->character );
        }
    }
}
```

---

### Part 2: Audio Support (mcmp.c/mcmp.h)

**2a. Add to mcmp.h (after other function prototypes)**

```c
void mcmp_lightning_flash( CHAR_DATA *ch );
```

**2b. Add new function to mcmp.c (after mcmp_weather_change)**

```c
/*
 * Lightning flash during storm — random one-shot thunder sound.
 *
 * Selects randomly from thunder_1.mp3 through thunder_4.mp3 for variety.
 * Uses wildcard-style naming: "weather/thunder_?" expands to thunder_1, thunder_2, etc.
 * Low priority so it doesn't interrupt combat sounds.
 */
void mcmp_lightning_flash( CHAR_DATA *ch )
{
    static const char *thunder_sounds[] = {
        "weather/thunder_1.mp3",
        "weather/thunder_2.mp3",
        "weather/thunder_3.mp3",
        "weather/thunder_4.mp3"
    };
    static const char *thunder_captions[] = {
        "Thunder crashes",
        "Thunder rumbles",
        "Lightning cracks",
        "Thunder booms"
    };
    int idx;

    if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
        return;

    idx = number_range( 0, 3 );
    mcmp_play( ch->desc, thunder_sounds[idx], MCMP_SOUND, MCMP_TAG_WEATHER,
        50, 1, 40, NULL, FALSE, thunder_captions[idx] );
}
```

**Key audio details:**
- Uses numbered files (thunder_1.mp3 through thunder_4.mp3) for variety
- Volume 50, priority 40 (lower than combat sounds at 60)
- One-shot (loops=1), no key binding (allows overlap with rain ambient)
- Varied accessibility captions match the varied sounds

---

### Part 3: Audio Files Needed

The following audio files should exist (or be added) in the audio repository:
- `weather/thunder_1.mp3`
- `weather/thunder_2.mp3`
- `weather/thunder_3.mp3`
- `weather/thunder_4.mp3`

(If only `weather/thunder.mp3` exists currently, we can fall back to that single file or create variants)

---

## Timing Summary

- Weather updates: every 15-45 seconds (randomized `pulse_point`)
- Lightning flash chance: 25% per update (`number_bits(2) == 0`)
- **Result**: Lightning flashes roughly every 60-180 seconds during storms, at unpredictable intervals

---

## Verification
1. Build the project: `game\build\build.bat`
2. Start the server and connect with a client that supports GMCP/Client.Media (e.g., Mudlet)
3. Enable MCMP audio preference (`config +mcmp`)
4. Use immortal command to force weather: test weather changes or wait for natural storm
5. Verify:
   - Text: Varied lightning messages appear at irregular intervals
   - Audio: Thunder sounds play with each flash (requires thunder_?.mp3 files)
   - Only affects outdoor, awake players
6. Verify flashes stop when weather transitions away from SKY_LIGHTNING