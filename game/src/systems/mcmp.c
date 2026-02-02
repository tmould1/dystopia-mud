/*
 * mcmp.c - MUD Client Media Protocol (MCMP) support
 *
 * Delivers sounds and music to clients via GMCP Client.Media.* packages.
 * Spec: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol
 *
 * MCMP piggybacks on GMCP — uses gmcp_send() for all messages.
 * Client advertises support via: Core.Supports.Set ["Client.Media 1"]
 */

#include "merc.h"
#include <stdio.h>
#include <string.h>
#include "gmcp.h"
#include "mcmp.h"

extern GAMECONFIG_DATA game_config;

/*
 * Check if a descriptor supports MCMP (Client.Media)
 */
bool mcmp_enabled( DESCRIPTOR_DATA *d ) {
	if ( d == NULL || !d->gmcp_enabled )
		return FALSE;
	if ( !( d->gmcp_packages & GMCP_PACKAGE_CLIENT_MEDIA ) )
		return FALSE;
	return TRUE;
}

/*
 * Send Client.Media.Default — set the base URL for media files.
 *
 * Spec: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol#Client.Media.Default
 *
 * Called once when the client first advertises Client.Media support
 * (in gmcp_handle_subnegotiation). Sets the base directory URL that all
 * subsequent Client.Media.Load and Client.Media.Play filenames are
 * relative to. The URL must end with a trailing slash.
 *
 * Uses the runtime gameconfig audio_url if set, otherwise falls back
 * to MCMP_DEFAULT_URL.
 */
void mcmp_set_default( DESCRIPTOR_DATA *d ) {
	char buf[MAX_STRING_LENGTH];
	const char *url;

	if ( !mcmp_enabled( d ) )
		return;

	url = ( game_config.audio_url && game_config.audio_url[0] != '\0' )
		? game_config.audio_url : MCMP_DEFAULT_URL;

	snprintf( buf, sizeof( buf ), "{\"url\":\"%s\"}", url );
	gmcp_send( d, "Client.Media.Default", buf );
}

/*
 * Send Client.Media.Load — preload a media file for client-side caching.
 *
 * Spec: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol#Client.Media.Load
 *
 * The client downloads the file in advance so that later Play messages
 * start instantly without a download delay. The "name" field is a path
 * relative to the base URL set by Client.Media.Default.
 *
 * We omit the optional "url" parameter because our base URL is already
 * established via Client.Media.Default at connection time.
 */
void mcmp_load( DESCRIPTOR_DATA *d, const char *name ) {
	char buf[MAX_STRING_LENGTH];

	if ( !mcmp_enabled( d ) || name == NULL )
		return;

	snprintf( buf, sizeof( buf ), "{\"name\":\"%s\"}", name );
	gmcp_send( d, "Client.Media.Load", buf );
}

/*
 * Send Client.Media.Play — play a media file.
 *
 * Spec: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol#Client.Media.Play
 *
 * Parameters we emit (mapped to spec):
 *   name     (required) Filename relative to base URL, e.g. "combat/engage.mp3"
 *   type     "sound" or "music" — controls client mixing behavior
 *   tag      Category label for bulk stop (e.g. "combat", "weather")
 *   volume   1-100, relative to client master volume (spec default: 50)
 *   loops    Repetitions: 1 = once, -1 = infinite (spec default: 1)
 *   priority 1-100, higher priority halts lower-priority sounds (spec default: none)
 *   key      Unique binding key — sounds with same key replace each other
 *   continue TRUE = don't restart if same file already playing (spec default: true)
 *   caption  Accessibility text for hearing-impaired players (spec v1.0.3+)
 *            Should use the "Captioning Key" style: onomatopoeia for
 *            environmental/ambient, short descriptive phrases for events.
 *
 * Parameters intentionally omitted (no current use case):
 *   url      Not needed — base URL set once via Client.Media.Default
 *   fadein   Linear volume ramp at start (milliseconds)
 *   fadeout  Linear volume ramp at end (milliseconds)
 *   start    Playback start offset (milliseconds)
 *   finish   Playback end offset (milliseconds)
 */
void mcmp_play( DESCRIPTOR_DATA *d, const char *name, const char *type,
                const char *tag, int volume, int loops, int priority,
                const char *key, bool cont, const char *caption ) {
	char buf[MAX_STRING_LENGTH];
	int len;

	if ( !mcmp_enabled( d ) || name == NULL )
		return;

	len = snprintf( buf, sizeof( buf ), "{\"name\":\"%s\"", name );

	if ( type != NULL )
		len += snprintf( buf + len, sizeof( buf ) - len, ",\"type\":\"%s\"", type );
	if ( tag != NULL )
		len += snprintf( buf + len, sizeof( buf ) - len, ",\"tag\":\"%s\"", tag );
	if ( volume > 0 )
		len += snprintf( buf + len, sizeof( buf ) - len, ",\"volume\":%d", volume );
	if ( loops != 0 )
		len += snprintf( buf + len, sizeof( buf ) - len, ",\"loops\":%d", loops );
	if ( priority > 0 )
		len += snprintf( buf + len, sizeof( buf ) - len, ",\"priority\":%d", priority );
	if ( key != NULL )
		len += snprintf( buf + len, sizeof( buf ) - len, ",\"key\":\"%s\"", key );
	if ( cont )
		len += snprintf( buf + len, sizeof( buf ) - len, ",\"continue\":true" );
	if ( caption != NULL )
		len += snprintf( buf + len, sizeof( buf ) - len, ",\"caption\":\"%s\"", caption );
	snprintf( buf + len, sizeof( buf ) - len, "}" );

	gmcp_send( d, "Client.Media.Play", buf );
}

/*
 * Send Client.Media.Stop — stop playing media.
 *
 * Spec: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol#Client.Media.Stop
 *
 * All filter parameters are optional. An empty body {} stops all media.
 * Multiple filters combine as AND (e.g. name + type stops only that
 * specific file of that type).
 *
 * Parameters we emit (mapped to spec):
 *   name     Stop by filename match
 *   type     Stop by media type ("sound" or "music")
 *   tag      Stop by tag category match
 *   key      Stop by binding key match
 *   fadeaway TRUE = apply a volume decrease before stopping
 *   fadeout  Fade duration in ms (spec default: 5000 if fadeaway but no fadeout)
 *
 * Parameters intentionally omitted:
 *   priority  Stop sounds at or below a priority level — not currently needed
 */
void mcmp_stop( DESCRIPTOR_DATA *d, const char *name, const char *type,
                const char *tag, const char *key, bool fadeaway, int fadeout ) {
	char buf[MAX_STRING_LENGTH];
	int len;

	if ( !mcmp_enabled( d ) )
		return;

	len = snprintf( buf, sizeof( buf ), "{" );

	/* Build filter fields — first field has no leading comma */
	if ( name != NULL )
		len += snprintf( buf + len, sizeof( buf ) - len,
			"%s\"name\":\"%s\"", len > 1 ? "," : "", name );
	if ( type != NULL )
		len += snprintf( buf + len, sizeof( buf ) - len,
			"%s\"type\":\"%s\"", len > 1 ? "," : "", type );
	if ( tag != NULL )
		len += snprintf( buf + len, sizeof( buf ) - len,
			"%s\"tag\":\"%s\"", len > 1 ? "," : "", tag );
	if ( key != NULL )
		len += snprintf( buf + len, sizeof( buf ) - len,
			"%s\"key\":\"%s\"", len > 1 ? "," : "", key );
	if ( fadeaway )
		len += snprintf( buf + len, sizeof( buf ) - len,
			"%s\"fadeaway\":true", len > 1 ? "," : "" );
	if ( fadeout > 0 )
		len += snprintf( buf + len, sizeof( buf ) - len,
			"%s\"fadeout\":%d", len > 1 ? "," : "", fadeout );

	snprintf( buf + len, sizeof( buf ) - len, "}" );

	gmcp_send( d, "Client.Media.Stop", buf );
}

/*
 * ============================================================
 * High-level game event functions
 * ============================================================
 */

/*
 * Sector type to ambient sound file mapping
 */
static const char *sector_ambient[] = {
	"ambient/indoor.mp3",   /* SECT_INSIDE     0 */
	"ambient/city.mp3",     /* SECT_CITY       1 */
	"ambient/field.mp3",    /* SECT_FIELD      2 */
	"ambient/forest.mp3",   /* SECT_FOREST     3 */
	"ambient/hills.mp3",    /* SECT_HILLS      4 */
	"ambient/mountain.mp3", /* SECT_MOUNTAIN   5 */
	"ambient/water.mp3",    /* SECT_WATER_SWIM 6 */
	"ambient/water.mp3",    /* SECT_WATER_NOSW 7 */
	"ambient/indoor.mp3",   /* SECT_UNUSED     8 */
	"ambient/air.mp3",      /* SECT_AIR        9 */
	"ambient/desert.mp3",   /* SECT_DESERT    10 */
};

static const char *sector_ambient_caption[] = {
	"Indoor ambience",
	"City sounds",
	"Open field, wind blowing",
	"Forest, birds and rustling",
	"Hilly terrain, wind",
	"Mountain wind, distant echoes",
	"Water flowing",
	"Water flowing",
	"Indoor ambience",
	"High altitude wind",
	"Desert wind",
};

/*
 * Sector type to footstep sound mapping
 */
static const char *sector_footstep[] = {
	"movement/footstep_stone.mp3",  /* SECT_INSIDE     */
	"movement/footstep_stone.mp3",  /* SECT_CITY       */
	"movement/footstep_grass.mp3",  /* SECT_FIELD      */
	"movement/footstep_forest.mp3", /* SECT_FOREST     */
	"movement/footstep_gravel.mp3", /* SECT_HILLS      */
	"movement/footstep_gravel.mp3", /* SECT_MOUNTAIN   */
	"movement/splash.mp3",          /* SECT_WATER_SWIM */
	"movement/splash.mp3",          /* SECT_WATER_NOSW */
	"movement/footstep_stone.mp3",  /* SECT_UNUSED     */
	"movement/whoosh.mp3",          /* SECT_AIR        */
	"movement/footstep_sand.mp3",   /* SECT_DESERT     */
};

static const char *sector_footstep_caption[] = {
	"Footsteps on stone",
	"Footsteps on stone",
	"Footsteps on grass",
	"Footsteps through undergrowth",
	"Footsteps on gravel",
	"Footsteps on gravel",
	"Splashing through water",
	"Splashing through water",
	"Footsteps on stone",
	"Wind rushing past",
	"Footsteps on sand",
};

/*
 * Combat round summary — called once per round from violence_update().
 *
 * Combat in Dystopia does 2-30+ individual attacks per 0.75s round via
 * multi_hit(). Sending a sound per attack would flood the client. Instead,
 * violence_update() captures the victim's HP before multi_hit(), computes
 * the total damage delta afterward, and calls this function exactly once
 * with the aggregated result.
 *
 * Damage thresholds:
 *   total_dam > 2000  -> heavy_hit.mp3  (vol 50, priority 40)
 *   total_dam > 0     -> light_hit.mp3  (vol 40, priority 30)
 *   total_dam == 0    -> miss.mp3       (vol 30, priority 30)
 *
 * All combat sounds are one-shot (loops=1), no key (independent of
 * ambient/weather layers), continue=false.
 *
 * Caption uses descriptive phrases: "Heavy blows land", "Light blows
 * exchanged", "Attacks miss".
 */
void mcmp_combat_round( CHAR_DATA *ch, CHAR_DATA *victim, int hits, int misses, int total_dam ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	if ( hits == 0 ) {
		mcmp_play( ch->desc, "combat/miss.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			30, 1, 30, NULL, FALSE, "Attacks miss" );
	} else if ( total_dam > 2000 ) {
		mcmp_play( ch->desc, "combat/heavy_hit.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 40, NULL, FALSE, "Heavy blows land" );
	} else {
		mcmp_play( ch->desc, "combat/light_hit.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			40, 1, 30, NULL, FALSE, "Light blows exchanged" );
	}
}

/*
 * Death event — sends separate sounds to killer and victim.
 *
 * The killer hears combat/death.mp3 ("A death cry rings out"), tagged as
 * combat. The victim hears ui/death.mp3 ("You have died"), tagged as UI
 * with higher priority (90) to cut through any ongoing combat sounds.
 */
void mcmp_combat_death( CHAR_DATA *ch, CHAR_DATA *victim ) {
	if ( ch != NULL && ch->desc != NULL && mcmp_enabled( ch->desc ) ) {
		mcmp_play( ch->desc, "combat/death.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			60, 1, 80, NULL, FALSE, "A death cry rings out" );
	}
	if ( victim != NULL && victim->desc != NULL && mcmp_enabled( victim->desc ) ) {
		mcmp_play( victim->desc, "ui/death.mp3", MCMP_SOUND, MCMP_TAG_UI,
			60, 1, 90, NULL, FALSE, "You have died" );
	}
}

/*
 * Combat starts — first engagement. One-shot sound at medium priority
 * so it doesn't override higher-priority death/victory sounds.
 */
void mcmp_combat_start( CHAR_DATA *ch, CHAR_DATA *victim ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	mcmp_play( ch->desc, "combat/engage.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
		50, 1, 50, NULL, FALSE, "Combat begins" );
}

/*
 * Combat ends — last enemy killed
 */
void mcmp_combat_end( CHAR_DATA *ch ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	mcmp_play( ch->desc, "combat/victory.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
		50, 1, 60, NULL, FALSE, "Victory" );
}

/*
 * Spell cast — generic spell sound
 */
void mcmp_spell_cast( CHAR_DATA *ch, int sn ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	mcmp_play( ch->desc, "specials/spell_cast.mp3", MCMP_SOUND, MCMP_TAG_SPELL,
		40, 1, 40, NULL, FALSE, "A spell is cast" );
}

/*
 * Movement — one-shot footstep sound based on destination sector type.
 *
 * Low volume (25) and low priority (10) so footsteps provide subtle
 * spatial awareness without dominating. Captions describe the surface:
 * "Footsteps on stone", "Splashing through water", etc.
 */
void mcmp_movement( CHAR_DATA *ch, int sector_to ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	if ( sector_to < 0 || sector_to >= SECT_MAX )
		return;

	mcmp_play( ch->desc, sector_footstep[sector_to], MCMP_SOUND, MCMP_TAG_MOVEMENT,
		25, 1, 10, NULL, FALSE, sector_footstep_caption[sector_to] );
}

/*
 * Room ambient — start or change the background ambient loop.
 *
 * Audio layering strategy (uses MCMP key + continue interaction):
 *   key="ambient", type="music", loops=-1, continue=true
 *
 * Per the spec, "continue: true" means if a file with the same name is
 * already playing, the client keeps it running rather than restarting.
 * This prevents the ambient loop from cutting out and restarting when
 * a player walks between rooms of the same sector type.
 *
 * When the sector type changes, the new file has a different name, so
 * the client starts the new ambient and the old one (same key) is
 * implicitly replaced.
 *
 * Volume is kept low (20) so ambient doesn't overpower one-shot sounds.
 * Priority is low (10) so combat/UI sounds take precedence.
 *
 * Captions describe the ambient environment in present tense:
 * "Forest, birds and rustling", "City sounds", etc.
 */
void mcmp_room_ambient( CHAR_DATA *ch, int sector_type ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	if ( sector_type < 0 || sector_type >= SECT_MAX )
		return;

	mcmp_play( ch->desc, sector_ambient[sector_type], MCMP_MUSIC, MCMP_TAG_ENVIRONMENT,
		20, -1, 10, "ambient", TRUE, sector_ambient_caption[sector_type] );
}

/*
 * Weather change — ambient weather overlay that layers on top of the
 * sector ambient loop.
 *
 * Audio layering strategy:
 *   key="weather", separate from key="ambient" — both play concurrently.
 *   Weather loops use continue=false so changing weather replaces the
 *   previous weather sound (e.g., wind -> rain replaces, not overlaps).
 *
 * Thunder is a special case: it's a one-shot sound with no key, so it
 * plays independently on top of any existing weather loop.
 *
 * When skies clear (SKY_CLOUDLESS), we send Client.Media.Stop with
 * key="weather", fadeaway=true, fadeout=3000 to fade out the weather
 * overlay over 3 seconds rather than cutting abruptly.
 *
 * Captions use descriptive present tense: "Wind picks up", "Heavy rain",
 * "Thunder crashes".
 */
void mcmp_weather_change( CHAR_DATA *ch, int sky_state ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	switch ( sky_state ) {
	case SKY_CLOUDLESS:
		/* Clear skies — stop weather sounds */
		mcmp_stop( ch->desc, NULL, NULL, NULL, "weather", TRUE, 3000 );
		break;
	case SKY_CLOUDY:
		mcmp_play( ch->desc, "weather/wind.mp3", MCMP_MUSIC, MCMP_TAG_WEATHER,
			15, -1, 20, "weather", FALSE, "Wind picks up" );
		break;
	case SKY_RAINING:
		mcmp_play( ch->desc, "weather/rain_heavy.mp3", MCMP_MUSIC, MCMP_TAG_WEATHER,
			25, -1, 20, "weather", FALSE, "Heavy rain" );
		break;
	case SKY_LIGHTNING:
		mcmp_play( ch->desc, "weather/thunder.mp3", MCMP_SOUND, MCMP_TAG_WEATHER,
			50, 1, 60, NULL, FALSE, "Thunder crashes" );
		break;
	}
}

/*
 * Channel notification — short one-shot sound when receiving a message.
 *
 * Each channel type maps to a distinct sound file so players can
 * distinguish tells from chats by audio alone. Captions use the
 * channel context: "Private message", "Chat message", etc.
 *
 * Low priority (20) ensures these don't interrupt combat or UI sounds.
 */
void mcmp_channel_notify( CHAR_DATA *ch, int channel_type ) {
	const char *sound;
	const char *caption;

	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	switch ( channel_type ) {
	case CHANNEL_TELL:
		sound = "channels/tell.mp3";
		caption = "Private message";
		break;
	case CHANNEL_YELL:
		sound = "channels/yell.mp3";
		caption = "Someone yells";
		break;
	case CHANNEL_IMMTALK:
		sound = "channels/immtalk.mp3";
		caption = "Immortal message";
		break;
	case CHANNEL_CHAT:
	default:
		sound = "channels/chat.mp3";
		caption = "Chat message";
		break;
	}

	mcmp_play( ch->desc, sound, MCMP_SOUND, MCMP_TAG_CHANNEL,
		30, 1, 20, NULL, FALSE, caption );
}

/*
 * UI event — significant player milestones: login, levelup, death, achievement.
 *
 * High priority (70) so these are heard over ambient and combat sounds.
 * Captions are short imperative labels: "Welcome", "Level up", etc.
 */
void mcmp_ui_event( CHAR_DATA *ch, const char *event ) {
	const char *sound;
	const char *caption;

	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	if ( event == NULL )
		return;

	if ( !strcmp( event, "login" ) ) {
		sound = "ui/login.mp3";
		caption = "Welcome";
	} else if ( !strcmp( event, "levelup" ) ) {
		sound = "ui/levelup.mp3";
		caption = "Level up";
	} else if ( !strcmp( event, "death" ) ) {
		sound = "ui/death.mp3";
		caption = "You have died";
	} else {
		sound = "ui/achievement.mp3";
		caption = "Achievement earned";
	}

	mcmp_play( ch->desc, sound, MCMP_SOUND, MCMP_TAG_UI,
		50, 1, 70, NULL, FALSE, caption );
}

/*
 * Immortal test command — play a specific MCMP sound by category
 */
void do_mcmptest( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];

	if ( ch->desc == NULL ) {
		send_to_char( "No descriptor.\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	/* Force-enable Client.Media for testing */
	if ( !str_cmp( arg, "force" ) ) {
		ch->desc->gmcp_packages |= GMCP_PACKAGE_CLIENT_MEDIA;
		mcmp_set_default( ch->desc );
		send_to_char( "Forced Client.Media enabled on your descriptor.\n\r", ch );
		return;
	}

	/* Debug: show URL and resend Default */
	if ( !str_cmp( arg, "debug" ) ) {
		char dbg[MAX_STRING_LENGTH];
		const char *url;
		url = ( game_config.audio_url && game_config.audio_url[0] != '\0' )
			? game_config.audio_url : MCMP_DEFAULT_URL;
		snprintf( dbg, sizeof( dbg ),
			"MCMP Debug:\n\r"
			"  gmcp_enabled: %d\n\r"
			"  Client.Media: %s\n\r"
			"  audio_url:    \"%s\"\n\r"
			"  test URL:     %sweather/thunder.mp3\n\r",
			ch->desc->gmcp_enabled,
			( ch->desc->gmcp_packages & GMCP_PACKAGE_CLIENT_MEDIA ) ? "Yes" : "No",
			url, url );
		send_to_char( dbg, ch );
		if ( mcmp_enabled( ch->desc ) ) {
			mcmp_set_default( ch->desc );
			send_to_char( "  -> Resent Client.Media.Default\n\r", ch );
		}
		return;
	}

	if ( !mcmp_enabled( ch->desc ) ) {
		send_to_char( "MCMP is not active on your connection.\n\r", ch );
		send_to_char( "In Mudlet: Settings > Game protocols > enable 'Allow server to download and play media'\n\r", ch );
		send_to_char( "Or use 'mcmptest force' to enable manually.\n\r", ch );
		return;
	}

	if ( arg[0] == '\0' ) {
		send_to_char( "mcmptest <category> -- play a test sound\n\r", ch );
		send_to_char( "Categories: combat miss engage death victory\n\r", ch );
		send_to_char( "            footstep splash whoosh ambient\n\r", ch );
		send_to_char( "            weather thunder rain wind\n\r", ch );
		send_to_char( "            chat tell yell spell\n\r", ch );
		send_to_char( "            login levelup die achievement\n\r", ch );
		send_to_char( "            stop  -- stop all sounds\n\r", ch );
		return;
	}

	/* Combat */
	if ( !str_cmp( arg, "combat" ) )
		mcmp_play( ch->desc, "combat/light_hit.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: light hit" );
	else if ( !str_cmp( arg, "miss" ) )
		mcmp_play( ch->desc, "combat/miss.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: miss" );
	else if ( !str_cmp( arg, "engage" ) )
		mcmp_play( ch->desc, "combat/engage.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: engage" );
	else if ( !str_cmp( arg, "death" ) )
		mcmp_play( ch->desc, "combat/death.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: death" );
	else if ( !str_cmp( arg, "victory" ) )
		mcmp_play( ch->desc, "combat/victory.mp3", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: victory" );

	/* Movement */
	else if ( !str_cmp( arg, "footstep" ) )
		mcmp_play( ch->desc, "movement/footstep_stone.mp3", MCMP_SOUND, MCMP_TAG_MOVEMENT,
			50, 1, 50, NULL, FALSE, "Test: footstep" );
	else if ( !str_cmp( arg, "splash" ) )
		mcmp_play( ch->desc, "movement/splash.mp3", MCMP_SOUND, MCMP_TAG_MOVEMENT,
			50, 1, 50, NULL, FALSE, "Test: splash" );
	else if ( !str_cmp( arg, "whoosh" ) )
		mcmp_play( ch->desc, "movement/whoosh.mp3", MCMP_SOUND, MCMP_TAG_MOVEMENT,
			50, 1, 50, NULL, FALSE, "Test: whoosh" );

	/* Ambient */
	else if ( !str_cmp( arg, "ambient" ) )
		mcmp_play( ch->desc, "ambient/forest.mp3", MCMP_MUSIC, MCMP_TAG_ENVIRONMENT,
			30, -1, 10, "ambient", TRUE, "Test: forest ambient" );

	/* Weather */
	else if ( !str_cmp( arg, "weather" ) )
		mcmp_play( ch->desc, "weather/wind.mp3", MCMP_MUSIC, MCMP_TAG_WEATHER,
			30, -1, 20, "weather", FALSE, "Test: wind" );
	else if ( !str_cmp( arg, "thunder" ) )
		mcmp_play( ch->desc, "weather/thunder.mp3", MCMP_SOUND, MCMP_TAG_WEATHER,
			50, 1, 60, NULL, FALSE, "Test: thunder" );
	else if ( !str_cmp( arg, "rain" ) )
		mcmp_play( ch->desc, "weather/rain_heavy.mp3", MCMP_MUSIC, MCMP_TAG_WEATHER,
			30, -1, 20, "weather", FALSE, "Test: rain" );
	else if ( !str_cmp( arg, "wind" ) )
		mcmp_play( ch->desc, "weather/wind.mp3", MCMP_MUSIC, MCMP_TAG_WEATHER,
			30, -1, 20, "weather", FALSE, "Test: wind" );

	/* Channels */
	else if ( !str_cmp( arg, "chat" ) )
		mcmp_play( ch->desc, "channels/chat.mp3", MCMP_SOUND, MCMP_TAG_CHANNEL,
			50, 1, 20, NULL, FALSE, "Test: chat" );
	else if ( !str_cmp( arg, "tell" ) )
		mcmp_play( ch->desc, "channels/tell.mp3", MCMP_SOUND, MCMP_TAG_CHANNEL,
			50, 1, 20, NULL, FALSE, "Test: tell" );
	else if ( !str_cmp( arg, "yell" ) )
		mcmp_play( ch->desc, "channels/yell.mp3", MCMP_SOUND, MCMP_TAG_CHANNEL,
			50, 1, 20, NULL, FALSE, "Test: yell" );

	/* Spell */
	else if ( !str_cmp( arg, "spell" ) )
		mcmp_play( ch->desc, "specials/spell_cast.mp3", MCMP_SOUND, MCMP_TAG_SPELL,
			50, 1, 40, NULL, FALSE, "Test: spell cast" );

	/* UI */
	else if ( !str_cmp( arg, "login" ) )
		mcmp_play( ch->desc, "ui/login.mp3", MCMP_SOUND, MCMP_TAG_UI,
			50, 1, 70, NULL, FALSE, "Test: login" );
	else if ( !str_cmp( arg, "levelup" ) )
		mcmp_play( ch->desc, "ui/levelup.mp3", MCMP_SOUND, MCMP_TAG_UI,
			50, 1, 70, NULL, FALSE, "Test: level up" );
	else if ( !str_cmp( arg, "die" ) )
		mcmp_play( ch->desc, "ui/death.mp3", MCMP_SOUND, MCMP_TAG_UI,
			50, 1, 70, NULL, FALSE, "Test: death" );
	else if ( !str_cmp( arg, "achievement" ) )
		mcmp_play( ch->desc, "ui/achievement.mp3", MCMP_SOUND, MCMP_TAG_UI,
			50, 1, 70, NULL, FALSE, "Test: achievement" );

	/* Stop all */
	else if ( !str_cmp( arg, "stop" ) ) {
		mcmp_stop( ch->desc, NULL, NULL, NULL, NULL, TRUE, 1000 );
		send_to_char( "Stopping all MCMP sounds.\n\r", ch );
		return;
	}

	else {
		send_to_char( "Unknown category. Type 'mcmptest' for a list.\n\r", ch );
		return;
	}

	send_to_char( "MCMP test sound sent.\n\r", ch );
}
