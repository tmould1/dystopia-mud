/*
 * mcmp.h - MUD Client Media Protocol (MCMP) support
 *
 * MCMP delivers sounds, music, and media via GMCP's Client.Media.* packages.
 * Protocol spec: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol
 * Version: 1.0.3 (caption support added for accessibility)
 *
 * MCMP rides on GMCP — no separate telnet option needed.
 * Client advertises support via: Core.Supports.Set ["Client.Media 1"]
 *
 * Supported clients: Mudlet 4.4+, BeipMU 4.00.298+, LociTerm 2.5.1+
 *
 * Protocol flow:
 *   1. Client connects, GMCP negotiated (telopt 201)
 *   2. Client sends Core.Supports.Set ["Client.Media 1"]
 *   3. Server sets GMCP_PACKAGE_CLIENT_MEDIA flag
 *   4. Server sends Client.Media.Default with base URL (once)
 *   5. Server sends Client.Media.Play/Stop/Load as game events occur
 *   6. All messages are server->client only; no client->server media msgs
 */

#ifndef MCMP_H
#define MCMP_H

/*
 * Media type constants
 */
#define MCMP_SOUND "sound"
#define MCMP_MUSIC "music"

/*
 * Tag constants for categorizing media events
 */
#define MCMP_TAG_COMBAT      "combat"
#define MCMP_TAG_ENVIRONMENT "environment"
#define MCMP_TAG_WEATHER     "weather"
#define MCMP_TAG_CHANNEL     "channel"
#define MCMP_TAG_UI          "ui"
#define MCMP_TAG_MOVEMENT    "movement"
#define MCMP_TAG_SPELL       "spell"

/*
 * Default media URL — set this to your sound file hosting location.
 * Must end with a trailing slash.
 */
#define MCMP_DEFAULT_URL "https://raw.githubusercontent.com/tmould1/dystopia-mud/main/audio/"

/*
 * Core protocol functions
 */

/* Send Client.Media.Default to set the base URL for media files */
void mcmp_set_default( DESCRIPTOR_DATA *d );

/* Send Client.Media.Load to preload a media file */
void mcmp_load( DESCRIPTOR_DATA *d, const char *name );

/*
 * Send Client.Media.Play
 *
 * Spec: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol#Client.Media.Play
 *
 * All 14 spec parameters and our support status:
 *
 *   SUPPORTED:
 *   name     (required) Filename relative to base URL, e.g. "combat/engage.mp3"
 *   type     MCMP_SOUND or MCMP_MUSIC (spec default: "sound")
 *   tag      Category tag for bulk stop, e.g. MCMP_TAG_COMBAT
 *   volume   1-100, relative to client master (0 = omit, spec default: 50)
 *   loops    -1 = infinite, 1+ = finite count (0 = omit, spec default: 1)
 *   priority 1-100, halts lower-priority media (0 = omit)
 *   key      Binding key — same-key sounds replace each other (NULL = omit)
 *   cont     TRUE = don't restart matching file (spec default: true)
 *   caption  Accessibility text for hearing-impaired players (NULL = omit)
 *            Use "Captioning Key" conventions: onomatopoeia for ambient,
 *            short descriptive phrases for events, imperative labels for UI.
 *
 *   INTENTIONALLY OMITTED (no current use case):
 *   url      Not needed — set once via Client.Media.Default
 *   fadein   Linear volume ramp at start (ms)
 *   fadeout  Linear volume ramp at end (ms)
 *   start    Playback start offset (ms)
 *   finish   Playback end offset (ms)
 */
void mcmp_play( DESCRIPTOR_DATA *d, const char *name, const char *type,
                const char *tag, int volume, int loops, int priority,
                const char *key, bool cont, const char *caption );

/*
 * Send Client.Media.Stop
 *
 * Spec: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol#Client.Media.Stop
 *
 * All parameters are optional — an empty body {} stops all media.
 * Pass NULL/0/FALSE for any field to omit it from the stop filter.
 *
 *   SUPPORTED: name, type, tag, key, fadeaway, fadeout
 *   OMITTED:   priority (stop by priority threshold — no current use)
 *
 *   fadeaway: TRUE to apply a volume decrease before stopping
 *   fadeout:  fade duration in ms (spec default: 5000 if fadeaway but no fadeout)
 */
void mcmp_stop( DESCRIPTOR_DATA *d, const char *name, const char *type,
                const char *tag, const char *key, bool fadeaway, int fadeout );

/* Check if a descriptor has MCMP (Client.Media) support enabled */
bool mcmp_enabled( DESCRIPTOR_DATA *d );

/*
 * High-level game event functions
 * Each checks mcmp_enabled() internally — safe to call unconditionally.
 */

/* Combat — one sound per round, not per-hit */
void mcmp_combat_round( CHAR_DATA *ch, CHAR_DATA *victim, int hits, int misses, int total_dam );
void mcmp_combat_death( CHAR_DATA *ch, CHAR_DATA *victim );
void mcmp_combat_start( CHAR_DATA *ch, CHAR_DATA *victim );
void mcmp_combat_end( CHAR_DATA *ch );

/* Special abilities — per use */
void mcmp_spell_cast( CHAR_DATA *ch, int sn );

/* Movement and ambient */
void mcmp_movement( CHAR_DATA *ch, int sector_to );
void mcmp_room_ambient( CHAR_DATA *ch, int sector_type );

/* Weather */
void mcmp_weather_change( CHAR_DATA *ch, int sky_state );

/* Communication channels */
void mcmp_channel_notify( CHAR_DATA *ch, int channel_type );

/* UI events */
void mcmp_ui_event( CHAR_DATA *ch, const char *event );

#endif /* MCMP_H */
