/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/*
 * mssp.c - MUD Server Status Protocol support
 *
 * MSSP allows MUD listing sites to automatically query game statistics
 * via telnet option negotiation (RFC 2217 extended).
 *
 * Protocol flow:
 *   Server sends: IAC WILL MSSP
 *   Client sends: IAC DO MSSP
 *   Server sends: IAC SB MSSP <variables> IAC SE
 */

#include "merc.h"
#include "telnet.h"
#include <stdio.h>
#include <string.h>

/* External references */
extern DESCRIPTOR_DATA *descriptor_list;
extern GAMECONFIG_DATA game_config;
extern time_t current_time;
extern time_t boot_time;
extern int port;
extern int top_area;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_room;

/* MSSP telnet negotiation sequences */
#if defined( WIN32 )
const char mssp_will[] = { (char) IAC, (char) WILL, (char) TELOPT_MSSP, '\0' };
const char mssp_wont[] = { (char) IAC, (char) WONT, (char) TELOPT_MSSP, '\0' };
const char mssp_do[] = { (char) IAC, (char) DO, (char) TELOPT_MSSP, '\0' };
#else
const char mssp_will[] = { IAC, WILL, TELOPT_MSSP, '\0' };
const char mssp_wont[] = { IAC, WONT, TELOPT_MSSP, '\0' };
const char mssp_do[] = { IAC, DO, TELOPT_MSSP, '\0' };
#endif

/* Forward declarations */
bool write_to_descriptor( DESCRIPTOR_DATA *d, char *txt, int length );

/*
 * Count players currently connected and playing
 */
static int mssp_count_players( void ) {
	DESCRIPTOR_DATA *d;
	int count = 0;

	for ( d = descriptor_list; d != NULL; d = d->next ) {
		if ( d->connected == CON_PLAYING && d->character != NULL )
			count++;
	}

	return count;
}

/*
 * Add an MSSP variable to the buffer
 * Format: MSSP_VAR <name> MSSP_VAL <value>
 */
static int mssp_add_var( unsigned char *buf, int pos, const char *var, const char *val ) {
	buf[pos++] = MSSP_VAR;
	while ( *var )
		buf[pos++] = *var++;
	buf[pos++] = MSSP_VAL;
	while ( *val )
		buf[pos++] = *val++;
	return pos;
}

/*
 * Add an additional value to the current MSSP variable
 * Used for multi-value variables (e.g., GENRE with multiple entries)
 */
static int mssp_add_val( unsigned char *buf, int pos, const char *val ) {
	buf[pos++] = MSSP_VAL;
	while ( *val )
		buf[pos++] = *val++;
	return pos;
}

/*
 * Send MSSP status data to the client
 * Called when client responds with IAC DO MSSP
 */
void mssp_send( DESCRIPTOR_DATA *d ) {
	unsigned char buf[4096];
	char numbuf[32];
	int pos = 0;

	/* Start subnegotiation: IAC SB MSSP */
	buf[pos++] = IAC;
	buf[pos++] = SB;
	buf[pos++] = TELOPT_MSSP;

	/* --- Required --- */
	pos = mssp_add_var( buf, pos, "NAME", game_config.game_name );

	sprintf( numbuf, "%d", mssp_count_players() );
	pos = mssp_add_var( buf, pos, "PLAYERS", numbuf );

	sprintf( numbuf, "%ld", (long) boot_time );
	pos = mssp_add_var( buf, pos, "UPTIME", numbuf );

	/* --- Generic --- */
	sprintf( numbuf, "%d", port );
	pos = mssp_add_var( buf, pos, "PORT", numbuf );

	pos = mssp_add_var( buf, pos, "CODEBASE", "Diku/Merc/GodWars/Dystopia" );
	pos = mssp_add_var( buf, pos, "FAMILY", "DikuMUD" );
	pos = mssp_add_var( buf, pos, "LANGUAGE", "English" );
	pos = mssp_add_var( buf, pos, "CREATED", "2026" );

	/* --- Categorization --- */
	pos = mssp_add_var( buf, pos, "GENRE", "Fantasy" );
	pos = mssp_add_val( buf, pos, "Science Fiction" );
	pos = mssp_add_val( buf, pos, "Horror" );

	pos = mssp_add_var( buf, pos, "SUBGENRE", "Cyberpunk" );
	pos = mssp_add_val( buf, pos, "Medieval Fantasy" );
	pos = mssp_add_val( buf, pos, "Multiverse" );

	pos = mssp_add_var( buf, pos, "GAMEPLAY", "Hack and Slash" );
	pos = mssp_add_var( buf, pos, "GAMESYSTEM", "Custom" );
	pos = mssp_add_var( buf, pos, "STATUS", "Live" );

	/* --- World --- */
	sprintf( numbuf, "%d", top_area );
	pos = mssp_add_var( buf, pos, "AREAS", numbuf );

	sprintf( numbuf, "%d", top_help );
	pos = mssp_add_var( buf, pos, "HELPFILES", numbuf );

	sprintf( numbuf, "%d", top_mob_index );
	pos = mssp_add_var( buf, pos, "MOBILES", numbuf );

	sprintf( numbuf, "%d", top_obj_index );
	pos = mssp_add_var( buf, pos, "OBJECTS", numbuf );

	sprintf( numbuf, "%d", top_room );
	pos = mssp_add_var( buf, pos, "ROOMS", numbuf );

	pos = mssp_add_var( buf, pos, "CLASSES", "22" );
	pos = mssp_add_var( buf, pos, "LEVELS", "0" );
	pos = mssp_add_var( buf, pos, "RACES", "0" );

	/* --- Protocol support --- */
	pos = mssp_add_var( buf, pos, "ANSI", "1" );
	pos = mssp_add_var( buf, pos, "MCCP", "1" );
	pos = mssp_add_var( buf, pos, "GMCP", "1" );
	pos = mssp_add_var( buf, pos, "MCMP", "1" );
	pos = mssp_add_var( buf, pos, "MXP", "1" );
	pos = mssp_add_var( buf, pos, "UTF-8", "0" );
	pos = mssp_add_var( buf, pos, "VT100", "1" );
	pos = mssp_add_var( buf, pos, "XTERM 256 COLORS", "1" );
	pos = mssp_add_var( buf, pos, "XTERM TRUE COLORS", "1" );

	/* --- Commercial --- */
	pos = mssp_add_var( buf, pos, "PAY TO PLAY", "0" );
	pos = mssp_add_var( buf, pos, "PAY FOR PERKS", "0" );

	/* End subnegotiation: IAC SE */
	buf[pos++] = IAC;
	buf[pos++] = SE;

	/* Send directly to descriptor */
	write_to_descriptor( d, (char *) buf, pos );
}
