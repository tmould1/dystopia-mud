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
 * Send Client.Media.Default — set the base URL for media files
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
 * Send Client.Media.Load — preload a media file
 */
void mcmp_load( DESCRIPTOR_DATA *d, const char *name ) {
	char buf[MAX_STRING_LENGTH];

	if ( !mcmp_enabled( d ) || name == NULL )
		return;

	snprintf( buf, sizeof( buf ), "{\"name\":\"%s\"}", name );
	gmcp_send( d, "Client.Media.Load", buf );
}

/*
 * Send Client.Media.Play — play a media file
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
 * Send Client.Media.Stop — stop playing media
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
	"ambient/indoor.ogg",   /* SECT_INSIDE     0 */
	"ambient/city.ogg",     /* SECT_CITY       1 */
	"ambient/field.ogg",    /* SECT_FIELD      2 */
	"ambient/forest.ogg",   /* SECT_FOREST     3 */
	"ambient/hills.ogg",    /* SECT_HILLS      4 */
	"ambient/mountain.ogg", /* SECT_MOUNTAIN   5 */
	"ambient/water.ogg",    /* SECT_WATER_SWIM 6 */
	"ambient/water.ogg",    /* SECT_WATER_NOSW 7 */
	"ambient/indoor.ogg",   /* SECT_UNUSED     8 */
	"ambient/air.ogg",      /* SECT_AIR        9 */
	"ambient/desert.ogg",   /* SECT_DESERT    10 */
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
	"movement/footstep_stone.ogg",  /* SECT_INSIDE     */
	"movement/footstep_stone.ogg",  /* SECT_CITY       */
	"movement/footstep_grass.ogg",  /* SECT_FIELD      */
	"movement/footstep_forest.ogg", /* SECT_FOREST     */
	"movement/footstep_gravel.ogg", /* SECT_HILLS      */
	"movement/footstep_gravel.ogg", /* SECT_MOUNTAIN   */
	"movement/splash.ogg",          /* SECT_WATER_SWIM */
	"movement/splash.ogg",          /* SECT_WATER_NOSW */
	"movement/footstep_stone.ogg",  /* SECT_UNUSED     */
	"movement/whoosh.ogg",          /* SECT_AIR        */
	"movement/footstep_sand.ogg",   /* SECT_DESERT     */
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
 * Combat round summary — called once at end of multi_hit()
 */
void mcmp_combat_round( CHAR_DATA *ch, CHAR_DATA *victim, int hits, int misses, int total_dam ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	if ( hits == 0 ) {
		mcmp_play( ch->desc, "combat/miss.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			30, 1, 30, NULL, FALSE, "Attacks miss" );
	} else if ( total_dam > 2000 ) {
		mcmp_play( ch->desc, "combat/heavy_hit.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 40, NULL, FALSE, "Heavy blows land" );
	} else {
		mcmp_play( ch->desc, "combat/light_hit.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			40, 1, 30, NULL, FALSE, "Light blows exchanged" );
	}
}

/*
 * Death event — target killed
 */
void mcmp_combat_death( CHAR_DATA *ch, CHAR_DATA *victim ) {
	if ( ch != NULL && ch->desc != NULL && mcmp_enabled( ch->desc ) ) {
		mcmp_play( ch->desc, "combat/death.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			60, 1, 80, NULL, FALSE, "A death cry rings out" );
	}
	if ( victim != NULL && victim->desc != NULL && mcmp_enabled( victim->desc ) ) {
		mcmp_play( victim->desc, "ui/death.ogg", MCMP_SOUND, MCMP_TAG_UI,
			60, 1, 90, NULL, FALSE, "You have died" );
	}
}

/*
 * Combat starts — first engagement
 */
void mcmp_combat_start( CHAR_DATA *ch, CHAR_DATA *victim ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	mcmp_play( ch->desc, "combat/engage.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
		50, 1, 50, NULL, FALSE, "Combat begins" );
}

/*
 * Combat ends — last enemy killed
 */
void mcmp_combat_end( CHAR_DATA *ch ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	mcmp_play( ch->desc, "combat/victory.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
		50, 1, 60, NULL, FALSE, "Victory" );
}

/*
 * Spell cast — generic spell sound
 */
void mcmp_spell_cast( CHAR_DATA *ch, int sn ) {
	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	mcmp_play( ch->desc, "specials/spell_cast.ogg", MCMP_SOUND, MCMP_TAG_SPELL,
		40, 1, 40, NULL, FALSE, "A spell is cast" );
}

/*
 * Movement — footstep sound based on sector type
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
 * Room ambient — start/change ambient loop based on sector type
 * Uses key "ambient" and continue=true so same-sector moves don't restart.
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
 * Weather change — ambient weather overlay
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
		mcmp_play( ch->desc, "weather/wind.ogg", MCMP_MUSIC, MCMP_TAG_WEATHER,
			15, -1, 20, "weather", FALSE, "Wind picks up" );
		break;
	case SKY_RAINING:
		mcmp_play( ch->desc, "weather/rain_heavy.ogg", MCMP_MUSIC, MCMP_TAG_WEATHER,
			25, -1, 20, "weather", FALSE, "Heavy rain" );
		break;
	case SKY_LIGHTNING:
		mcmp_play( ch->desc, "weather/thunder.ogg", MCMP_SOUND, MCMP_TAG_WEATHER,
			50, 1, 60, NULL, FALSE, "Thunder crashes" );
		break;
	}
}

/*
 * Channel notification — sound when receiving a chat/tell message
 */
void mcmp_channel_notify( CHAR_DATA *ch, int channel_type ) {
	const char *sound;
	const char *caption;

	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	switch ( channel_type ) {
	case CHANNEL_TELL:
		sound = "channels/tell.ogg";
		caption = "Private message";
		break;
	case CHANNEL_YELL:
		sound = "channels/yell.ogg";
		caption = "Someone yells";
		break;
	case CHANNEL_IMMTALK:
		sound = "channels/immtalk.ogg";
		caption = "Immortal message";
		break;
	case CHANNEL_CHAT:
	default:
		sound = "channels/chat.ogg";
		caption = "Chat message";
		break;
	}

	mcmp_play( ch->desc, sound, MCMP_SOUND, MCMP_TAG_CHANNEL,
		30, 1, 20, NULL, FALSE, caption );
}

/*
 * UI event — login, levelup, death, achievement
 */
void mcmp_ui_event( CHAR_DATA *ch, const char *event ) {
	const char *sound;
	const char *caption;

	if ( ch == NULL || ch->desc == NULL || !mcmp_enabled( ch->desc ) )
		return;

	if ( event == NULL )
		return;

	if ( !strcmp( event, "login" ) ) {
		sound = "ui/login.ogg";
		caption = "Welcome";
	} else if ( !strcmp( event, "levelup" ) ) {
		sound = "ui/levelup.ogg";
		caption = "Level up";
	} else if ( !strcmp( event, "death" ) ) {
		sound = "ui/death.ogg";
		caption = "You have died";
	} else {
		sound = "ui/achievement.ogg";
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

	if ( !mcmp_enabled( ch->desc ) ) {
		send_to_char( "MCMP is not active on your connection.\n\r", ch );
		send_to_char( "Use 'mcmptest force' to enable Client.Media manually.\n\r", ch );
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
		mcmp_play( ch->desc, "combat/light_hit.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: light hit" );
	else if ( !str_cmp( arg, "miss" ) )
		mcmp_play( ch->desc, "combat/miss.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: miss" );
	else if ( !str_cmp( arg, "engage" ) )
		mcmp_play( ch->desc, "combat/engage.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: engage" );
	else if ( !str_cmp( arg, "death" ) )
		mcmp_play( ch->desc, "combat/death.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: death" );
	else if ( !str_cmp( arg, "victory" ) )
		mcmp_play( ch->desc, "combat/victory.ogg", MCMP_SOUND, MCMP_TAG_COMBAT,
			50, 1, 50, NULL, FALSE, "Test: victory" );

	/* Movement */
	else if ( !str_cmp( arg, "footstep" ) )
		mcmp_play( ch->desc, "movement/footstep_stone.ogg", MCMP_SOUND, MCMP_TAG_MOVEMENT,
			50, 1, 50, NULL, FALSE, "Test: footstep" );
	else if ( !str_cmp( arg, "splash" ) )
		mcmp_play( ch->desc, "movement/splash.ogg", MCMP_SOUND, MCMP_TAG_MOVEMENT,
			50, 1, 50, NULL, FALSE, "Test: splash" );
	else if ( !str_cmp( arg, "whoosh" ) )
		mcmp_play( ch->desc, "movement/whoosh.ogg", MCMP_SOUND, MCMP_TAG_MOVEMENT,
			50, 1, 50, NULL, FALSE, "Test: whoosh" );

	/* Ambient */
	else if ( !str_cmp( arg, "ambient" ) )
		mcmp_play( ch->desc, "ambient/forest.ogg", MCMP_MUSIC, MCMP_TAG_ENVIRONMENT,
			30, -1, 10, "ambient", TRUE, "Test: forest ambient" );

	/* Weather */
	else if ( !str_cmp( arg, "weather" ) )
		mcmp_play( ch->desc, "weather/wind.ogg", MCMP_MUSIC, MCMP_TAG_WEATHER,
			30, -1, 20, "weather", FALSE, "Test: wind" );
	else if ( !str_cmp( arg, "thunder" ) )
		mcmp_play( ch->desc, "weather/thunder.ogg", MCMP_SOUND, MCMP_TAG_WEATHER,
			50, 1, 60, NULL, FALSE, "Test: thunder" );
	else if ( !str_cmp( arg, "rain" ) )
		mcmp_play( ch->desc, "weather/rain_heavy.ogg", MCMP_MUSIC, MCMP_TAG_WEATHER,
			30, -1, 20, "weather", FALSE, "Test: rain" );
	else if ( !str_cmp( arg, "wind" ) )
		mcmp_play( ch->desc, "weather/wind.ogg", MCMP_MUSIC, MCMP_TAG_WEATHER,
			30, -1, 20, "weather", FALSE, "Test: wind" );

	/* Channels */
	else if ( !str_cmp( arg, "chat" ) )
		mcmp_play( ch->desc, "channels/chat.ogg", MCMP_SOUND, MCMP_TAG_CHANNEL,
			50, 1, 20, NULL, FALSE, "Test: chat" );
	else if ( !str_cmp( arg, "tell" ) )
		mcmp_play( ch->desc, "channels/tell.ogg", MCMP_SOUND, MCMP_TAG_CHANNEL,
			50, 1, 20, NULL, FALSE, "Test: tell" );
	else if ( !str_cmp( arg, "yell" ) )
		mcmp_play( ch->desc, "channels/yell.ogg", MCMP_SOUND, MCMP_TAG_CHANNEL,
			50, 1, 20, NULL, FALSE, "Test: yell" );

	/* Spell */
	else if ( !str_cmp( arg, "spell" ) )
		mcmp_play( ch->desc, "specials/spell_cast.ogg", MCMP_SOUND, MCMP_TAG_SPELL,
			50, 1, 40, NULL, FALSE, "Test: spell cast" );

	/* UI */
	else if ( !str_cmp( arg, "login" ) )
		mcmp_play( ch->desc, "ui/login.ogg", MCMP_SOUND, MCMP_TAG_UI,
			50, 1, 70, NULL, FALSE, "Test: login" );
	else if ( !str_cmp( arg, "levelup" ) )
		mcmp_play( ch->desc, "ui/levelup.ogg", MCMP_SOUND, MCMP_TAG_UI,
			50, 1, 70, NULL, FALSE, "Test: level up" );
	else if ( !str_cmp( arg, "die" ) )
		mcmp_play( ch->desc, "ui/death.ogg", MCMP_SOUND, MCMP_TAG_UI,
			50, 1, 70, NULL, FALSE, "Test: death" );
	else if ( !str_cmp( arg, "achievement" ) )
		mcmp_play( ch->desc, "ui/achievement.ogg", MCMP_SOUND, MCMP_TAG_UI,
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
