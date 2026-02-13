/*
 * gmcp.c - Generic MUD Communication Protocol support
 *
 * GMCP enables structured JSON data exchange with modern MUD clients.
 * Uses telnet option 201 (TELOPT_GMCP) with subnegotiation.
 *
 * Protocol: IAC SB GMCP <package.name> <JSON data> IAC SE
 *
 * Supported packages:
 *   Core.Hello, Core.Goodbye, Core.Supports.Set
 *   Char.Vitals, Char.Status, Char.Info
 */

#include "merc.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "telnet.h"
#include "gmcp.h"
#include "mcmp.h"
#include "class.h"
#include "../db/db_class.h"

/*
 * Telnet negotiation strings for GMCP
 */
#if defined( WIN32 )
const char gmcp_will[] = { (char) IAC, (char) WILL, (char) TELOPT_GMCP, '\0' };
const char gmcp_wont[] = { (char) IAC, (char) WONT, (char) TELOPT_GMCP, '\0' };
const char gmcp_do[] = { (char) IAC, (char) DO, (char) TELOPT_GMCP, '\0' };
const char gmcp_dont[] = { (char) IAC, (char) DONT, (char) TELOPT_GMCP, '\0' };
#else
const char gmcp_will[] = { IAC, WILL, TELOPT_GMCP, '\0' };
const char gmcp_wont[] = { IAC, WONT, TELOPT_GMCP, '\0' };
const char gmcp_do[] = { IAC, DO, TELOPT_GMCP, '\0' };
const char gmcp_dont[] = { IAC, DONT, TELOPT_GMCP, '\0' };
#endif

/* External functions */
bool write_to_descriptor args( ( DESCRIPTOR_DATA * d, char *txt, int length ) );

/* External data */
extern GAMECONFIG_DATA game_config;


/*
 * Get position name
 */
static const char *get_pos_name( int position ) {
	switch ( position ) {
	case POS_DEAD:
		return "dead";
	case POS_MORTAL:
		return "mortally wounded";
	case POS_INCAP:
		return "incapacitated";
	case POS_STUNNED:
		return "stunned";
	case POS_SLEEPING:
		return "sleeping";
	case POS_MEDITATING:
		return "meditating";
	case POS_SITTING:
		return "sitting";
	case POS_RESTING:
		return "resting";
	case POS_FIGHTING:
		return "fighting";
	case POS_STANDING:
		return "standing";
	default:
		return "unknown";
	}
}

/*
 * Escape a string for JSON output
 * Returns a static buffer - not reentrant
 */
static char *json_escape( const char *str ) {
	static char buf[MAX_STRING_LENGTH];
	char *ptr = buf;

	if ( str == NULL ) {
		buf[0] = '\0';
		return buf;
	}

	while ( *str && ptr < buf + sizeof( buf ) - 6 ) {
		switch ( *str ) {
		case '"':
			*ptr++ = '\\';
			*ptr++ = '"';
			break;
		case '\\':
			*ptr++ = '\\';
			*ptr++ = '\\';
			break;
		case '\n':
			*ptr++ = '\\';
			*ptr++ = 'n';
			break;
		case '\r':
			*ptr++ = '\\';
			*ptr++ = 'r';
			break;
		case '\t':
			*ptr++ = '\\';
			*ptr++ = 't';
			break;
		default:
			if ( (unsigned char) *str >= 32 )
				*ptr++ = *str;
			break;
		}
		str++;
	}
	*ptr = '\0';
	return buf;
}

/*
 * Initialize GMCP on a descriptor after client sends IAC DO GMCP
 */
void gmcp_init( DESCRIPTOR_DATA *d ) {
	if ( d == NULL )
		return;

	d->gmcp_enabled = TRUE;

	/* Enable Core and all Char packages by default */
	d->gmcp_packages = GMCP_PACKAGE_CORE | GMCP_PACKAGE_CHAR |
		GMCP_PACKAGE_CHAR_VITALS | GMCP_PACKAGE_CHAR_STATUS |
		GMCP_PACKAGE_CHAR_INFO;

	/* Send Core.Hello to identify the server */
	gmcp_send( d, "Core.Hello",
		"{\"name\":\"Dystopia MUD\",\"version\":\"1.0\"}" );
}

/*
 * Send a raw GMCP message: IAC SB GMCP <package> <data> IAC SE
 */
void gmcp_send( DESCRIPTOR_DATA *d, const char *package, const char *data ) {
	char buf[MAX_STRING_LENGTH * 2];
	int len;

	if ( d == NULL || !d->gmcp_enabled )
		return;

	if ( package == NULL )
		return;

	/* Flush any pending text output before sending out-of-band data.
	 * This ensures text (combat messages, etc.) arrives at the client
	 * before related GMCP messages (sounds, vitals updates). */
	if ( d->outtop > 0 )
		process_output( d, FALSE );

	/* Build the GMCP message */
	buf[0] = (char) IAC;
	buf[1] = (char) SB;
	buf[2] = (char) TELOPT_GMCP;
	len = 3;

	/* Copy package name */
	while ( *package && len < sizeof( buf ) - 10 )
		buf[len++] = *package++;

	/* Add space separator if we have data */
	if ( data != NULL && *data ) {
		buf[len++] = ' ';

		/* Copy JSON data */
		while ( *data && len < sizeof( buf ) - 5 )
			buf[len++] = *data++;
	}

	/* End subnegotiation */
	buf[len++] = (char) IAC;
	buf[len++] = (char) SE;
	buf[len] = '\0';

	write_to_descriptor( d, buf, len );
}

/*
 * Send Char.Vitals with current/max hp, mana, move
 */
void gmcp_send_vitals( CHAR_DATA *ch ) {
	char buf[MAX_STRING_LENGTH];

	if ( ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled )
		return;

	/* Check if client supports Char.Vitals */
	if ( !( ch->desc->gmcp_packages & ( GMCP_PACKAGE_CHAR | GMCP_PACKAGE_CHAR_VITALS ) ) )
		return;

	snprintf( buf, sizeof( buf ),
		"{\"hp\":%d,\"maxhp\":%d,\"mana\":%d,\"maxmana\":%d,\"move\":%d,\"maxmove\":%d}",
		ch->hit, ch->max_hit,
		ch->mana, ch->max_mana,
		ch->move, ch->max_move );

	gmcp_send( ch->desc, "Char.Vitals", buf );
}

/*
 * Send Char.Status with level, class, position, experience
 */
void gmcp_send_status( CHAR_DATA *ch ) {
	char buf[512];
	char class_buf[64];
	char pos_buf[64];

	if ( ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled )
		return;

	/* Check if client supports Char.Status */
	if ( !( ch->desc->gmcp_packages & ( GMCP_PACKAGE_CHAR | GMCP_PACKAGE_CHAR_STATUS ) ) )
		return;

	/* Copy escaped strings to avoid static buffer issues */
	strncpy( class_buf, json_escape( db_class_get_name( ch->class ) ), sizeof( class_buf ) - 1 );
	class_buf[sizeof( class_buf ) - 1] = '\0';
	strncpy( pos_buf, json_escape( get_pos_name( ch->position ) ), sizeof( pos_buf ) - 1 );
	pos_buf[sizeof( pos_buf ) - 1] = '\0';

	snprintf( buf, sizeof( buf ),
		"{\"level\":%d,\"class\":\"%s\",\"position\":\"%s\",\"exp\":%d}",
		ch->level, class_buf, pos_buf, ch->exp );

	gmcp_send( ch->desc, "Char.Status", buf );
}

/*
 * Send Char.Info with name, race, guild/clan
 */
void gmcp_send_info( CHAR_DATA *ch ) {
	char buf[512];
	char name_buf[128];
	char clan_buf[128];
	const char *clan_name;

	if ( ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled )
		return;

	/* Check if client supports Char.Info */
	if ( !( ch->desc->gmcp_packages & ( GMCP_PACKAGE_CHAR | GMCP_PACKAGE_CHAR_INFO ) ) )
		return;

	clan_name = ( ch->clan != NULL && ch->clan[0] != '\0' ) ? ch->clan : "None";

	/* Copy escaped strings to avoid static buffer issues */
	strncpy( name_buf, json_escape( ch->name ), sizeof( name_buf ) - 1 );
	name_buf[sizeof( name_buf ) - 1] = '\0';
	strncpy( clan_buf, json_escape( clan_name ), sizeof( clan_buf ) - 1 );
	clan_buf[sizeof( clan_buf ) - 1] = '\0';

	snprintf( buf, sizeof( buf ),
		"{\"name\":\"%s\",\"guild\":\"%s\"}",
		name_buf, clan_buf );

	gmcp_send( ch->desc, "Char.Info", buf );
}

/*
 * Send Client.GUI for Mudlet auto-install package
 * Sends the URL and version of a Mudlet mpackage that clients will
 * automatically download and install.
 */
void gmcp_send_gui( DESCRIPTOR_DATA *d ) {
	char buf[MAX_STRING_LENGTH];

	if ( d == NULL || !d->gmcp_enabled )
		return;

	/* Skip if no GUI URL configured */
	if ( game_config.gui_url == NULL || game_config.gui_url[0] == '\0' )
		return;

	snprintf( buf, sizeof( buf ),
		"{\"version\":\"%s\",\"url\":\"%s\"}",
		game_config.gui_version,
		game_config.gui_url );

	gmcp_send( d, "Client.GUI", buf );
}

/*
 * Send all character data (called on login)
 */
void gmcp_send_char_data( CHAR_DATA *ch ) {
	if ( ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled )
		return;

	gmcp_send_gui( ch->desc ); /* Send GUI package first */
	gmcp_send_info( ch );
	gmcp_send_status( ch );
	gmcp_send_vitals( ch );
}

/*
 * Get sector type name for Room.Info
 */
static const char *get_sector_name( int sector ) {
	switch ( sector ) {
	case SECT_INSIDE:
		return "inside";
	case SECT_CITY:
		return "city";
	case SECT_FIELD:
		return "field";
	case SECT_FOREST:
		return "forest";
	case SECT_HILLS:
		return "hills";
	case SECT_MOUNTAIN:
		return "mountain";
	case SECT_WATER_SWIM:
		return "water_swim";
	case SECT_WATER_NOSWIM:
		return "water_noswim";
	case SECT_AIR:
		return "air";
	case SECT_DESERT:
		return "desert";
	default:
		return "unknown";
	}
}

/*
 * Send Room.Info with current room details and exits
 * Format follows IRE/Aardwolf standard for maximum client compatibility:
 * {"num": 12345, "name": "Room Name", "area": "Area Name",
 *  "terrain": "city", "exits": {"n": 12344, "s": 12336}}
 */
void gmcp_send_room_info( CHAR_DATA *ch ) {
	char buf[MAX_STRING_LENGTH];
	char exits_buf[512];
	char name_buf[256];
	char area_buf[256];
	ROOM_INDEX_DATA *room;
	EXIT_DATA *pexit;
	int dir;
	bool first_exit = TRUE;
	const char *dir_names[] = { "n", "e", "s", "w", "u", "d" };

	if ( ch == NULL || ch->desc == NULL || !ch->desc->gmcp_enabled )
		return;

	if ( !( ch->desc->gmcp_packages & GMCP_PACKAGE_ROOM_INFO ) )
		return;

	room = ch->in_room;
	if ( room == NULL )
		return;

	/* Build exits JSON object */
	strcpy( exits_buf, "{" );
	for ( dir = 0; dir < 6; dir++ ) {
		pexit = room->exit[dir];
		if ( pexit != NULL && pexit->to_room != NULL ) {
			char exit_entry[64];
			snprintf( exit_entry, sizeof( exit_entry ), "%s\"%s\":%d",
				first_exit ? "" : ",",
				dir_names[dir],
				pexit->to_room->vnum );
			strcat( exits_buf, exit_entry );
			first_exit = FALSE;
		}
	}
	strcat( exits_buf, "}" );

	/* Copy escaped strings to avoid static buffer issues */
	strncpy( name_buf, json_escape( room->name ), sizeof( name_buf ) - 1 );
	name_buf[sizeof( name_buf ) - 1] = '\0';
	strncpy( area_buf, json_escape( room->area ? room->area->name : "Unknown" ),
		sizeof( area_buf ) - 1 );
	area_buf[sizeof( area_buf ) - 1] = '\0';

	snprintf( buf, sizeof( buf ),
		"{\"num\":%d,\"name\":\"%s\",\"area\":\"%s\",\"terrain\":\"%s\",\"exits\":%s}",
		room->vnum,
		name_buf,
		area_buf,
		get_sector_name( room->sector_type ),
		exits_buf );

	gmcp_send( ch->desc, "Room.Info", buf );
}

/*
 * Parse a package name and version from Core.Supports.Set
 * Format: "Package.Name version" or just "Package.Name"
 * Returns the package flag if recognized, 0 otherwise
 */
static int parse_package_support( const char *pkg ) {
	if ( pkg == NULL )
		return 0;

	/* Skip leading whitespace and quotes */
	while ( *pkg == ' ' || *pkg == '"' || *pkg == '[' )
		pkg++;

	/* Check for known packages */
	if ( !strncmp( pkg, "Char.Vitals", 11 ) )
		return GMCP_PACKAGE_CHAR_VITALS | GMCP_PACKAGE_CHAR;
	if ( !strncmp( pkg, "Char.Status", 11 ) )
		return GMCP_PACKAGE_CHAR_STATUS | GMCP_PACKAGE_CHAR;
	if ( !strncmp( pkg, "Char.Info", 9 ) )
		return GMCP_PACKAGE_CHAR_INFO | GMCP_PACKAGE_CHAR;
	if ( !strncmp( pkg, "Char", 4 ) )
		return GMCP_PACKAGE_CHAR | GMCP_PACKAGE_CHAR_VITALS |
			GMCP_PACKAGE_CHAR_STATUS | GMCP_PACKAGE_CHAR_INFO;
	if ( !strncmp( pkg, "Client.Media", 12 ) )
		return GMCP_PACKAGE_CLIENT_MEDIA;
	if ( !strncmp( pkg, "Room.Info", 9 ) )
		return GMCP_PACKAGE_ROOM_INFO;
	if ( !strncmp( pkg, "Room", 4 ) )
		return GMCP_PACKAGE_ROOM_INFO;
	if ( !strncmp( pkg, "Core", 4 ) )
		return GMCP_PACKAGE_CORE;

	return 0;
}

/*
 * Handle incoming GMCP subnegotiation from client
 * Data format: <package.name> <JSON data>
 */
void gmcp_handle_subnegotiation( DESCRIPTOR_DATA *d, unsigned char *data, int len ) {
	char package[256];
	char json_data[MAX_STRING_LENGTH];
	int i, j;

	if ( d == NULL || data == NULL || len <= 0 )
		return;

	/* Extract package name (up to first space or end) */
	for ( i = 0; i < len && i < (int) sizeof( package ) - 1 && data[i] != ' '; i++ )
		package[i] = data[i];
	package[i] = '\0';

	/* Skip the space */
	if ( i < len && data[i] == ' ' )
		i++;

	/* Extract JSON data (rest of the message) */
	for ( j = 0; i < len && j < (int) sizeof( json_data ) - 1; i++, j++ )
		json_data[j] = data[i];
	json_data[j] = '\0';

	/* Handle Core.Supports.Set - client tells us what packages it wants */
	if ( !strcmp( package, "Core.Supports.Set" ) ) {
		/* Parse the JSON array of supported packages */
		/* Format: ["Char 1", "Char.Vitals 1", ...] */
		char *ptr = json_data;
		char pkg_buf[128];
		int pkg_i;
		int old_packages = d->gmcp_packages;

		while ( *ptr ) {
			/* Find start of package name (after quote) */
			while ( *ptr && *ptr != '"' )
				ptr++;
			if ( *ptr == '"' )
				ptr++;

			/* Extract package name */
			pkg_i = 0;
			while ( *ptr && *ptr != '"' && pkg_i < (int) sizeof( pkg_buf ) - 1 )
				pkg_buf[pkg_i++] = *ptr++;
			pkg_buf[pkg_i] = '\0';

			/* Parse and add to supported packages */
			if ( pkg_i > 0 )
				d->gmcp_packages |= parse_package_support( pkg_buf );

			/* Skip to next */
			if ( *ptr == '"' )
				ptr++;
		}

		/* If Client.Media was just enabled, send the default media URL */
		if ( ( d->gmcp_packages & GMCP_PACKAGE_CLIENT_MEDIA ) &&
			!( old_packages & GMCP_PACKAGE_CLIENT_MEDIA ) ) {
			mcmp_set_default( d );
		}
	}
	/* Handle Core.Supports.Add - add a package */
	else if ( !strcmp( package, "Core.Supports.Add" ) ) {
		int old_packages = d->gmcp_packages;
		d->gmcp_packages |= parse_package_support( json_data );

		/* If Client.Media was just enabled, send the default media URL */
		if ( ( d->gmcp_packages & GMCP_PACKAGE_CLIENT_MEDIA ) &&
			!( old_packages & GMCP_PACKAGE_CLIENT_MEDIA ) ) {
			mcmp_set_default( d );
		}
	}
	/* Handle Core.Supports.Remove - remove a package */
	else if ( !strcmp( package, "Core.Supports.Remove" ) ) {
		d->gmcp_packages &= ~parse_package_support( json_data );
	}
	/* Core.Hello - client identification, e.g. {"client":"Mudlet","version":"4.19.1"} */
	else if ( !strcmp( package, "Core.Hello" ) ) {
		char *ptr = strstr( json_data, "\"client\"" );
		if ( ptr != NULL ) {
			ptr += 8; /* skip past "client" */
			while ( *ptr && *ptr != '"' ) ptr++;
			if ( *ptr == '"' ) {
				int ci = 0;
				ptr++;
				while ( *ptr && *ptr != '"' && ci < (int) sizeof( d->client_name ) - 1 )
					d->client_name[ci++] = *ptr++;
				d->client_name[ci] = '\0';
			}
		}
	}
}

/*
 * User command to show GMCP status
 */
void do_gmcp( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( ch->desc == NULL ) {
		send_to_char( "No descriptor.\n\r", ch );
		return;
	}

	if ( !ch->desc->gmcp_enabled ) {
		send_to_char( "GMCP is not enabled. Your client does not support it.\n\r", ch );
		return;
	}

	snprintf( buf, sizeof( buf ),
		"GMCP Status:\n\r"
		"  Enabled: Yes\n\r"
		"  Packages: %s%s%s%s%s%s%s\n\r",
		( ch->desc->gmcp_packages & GMCP_PACKAGE_CORE ) ? "Core " : "",
		( ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR ) ? "Char " : "",
		( ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_VITALS ) ? "Char.Vitals " : "",
		( ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_STATUS ) ? "Char.Status " : "",
		( ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_INFO ) ? "Char.Info " : "",
		( ch->desc->gmcp_packages & GMCP_PACKAGE_CLIENT_MEDIA ) ? "Client.Media " : "",
		( ch->desc->gmcp_packages & GMCP_PACKAGE_ROOM_INFO ) ? "Room.Info " : "" );

	send_to_char( buf, ch );
}
