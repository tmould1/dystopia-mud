/*
 * ttype.c - Terminal Type / MTTS (MUD Terminal Type Standard) support
 *
 * Implements TTYPE negotiation (RFC 1091) with MTTS extensions.
 * Uses telnet option 24 (TELOPT_TTYPE) with subnegotiation.
 *
 * The MTTS standard uses three rounds of TTYPE negotiation:
 *   Round 1: Client sends terminal/client name (e.g., "MUDLET", "TINTIN++")
 *   Round 2: Client sends terminal name again (or extended version)
 *   Round 3: Client sends "MTTS <flags>" with a capability bitfield
 *
 * This allows automatic detection of client capabilities including
 * ANSI color, 256-color, true color, screen reader, and UTF-8 support.
 */

#include "merc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "telnet.h"
#include "ttype.h"

/*
 * Telnet negotiation strings for TTYPE
 *
 * TTYPE is client-to-server (client sends terminal type), so server uses
 * DO to request. After client replies WILL, server sends SB TTYPE SEND
 * subnegotiations to solicit each round of information.
 */
#if defined( WIN32 )
const char ttype_do[]   = { (char) IAC, (char) DO,   (char) TELOPT_TTYPE, '\0' };
const char ttype_dont[] = { (char) IAC, (char) DONT, (char) TELOPT_TTYPE, '\0' };
const char ttype_will[] = { (char) IAC, (char) WILL, (char) TELOPT_TTYPE, '\0' };
const char ttype_wont[] = { (char) IAC, (char) WONT, (char) TELOPT_TTYPE, '\0' };
#else
const char ttype_do[]   = { IAC, DO,   TELOPT_TTYPE, '\0' };
const char ttype_dont[] = { IAC, DONT, TELOPT_TTYPE, '\0' };
const char ttype_will[] = { IAC, WILL, TELOPT_TTYPE, '\0' };
const char ttype_wont[] = { IAC, WONT, TELOPT_TTYPE, '\0' };
#endif

/* TTYPE SEND subnegotiation: IAC SB TTYPE SEND IAC SE */
#if defined( WIN32 )
static const char ttype_send[] = {
	(char) IAC, (char) SB, (char) TELOPT_TTYPE, (char) TELQUAL_SEND,
	(char) IAC, (char) SE, '\0'
};
#else
static const char ttype_send[] = {
	IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE, '\0'
};
#endif

#define TTYPE_MAX_ROUNDS 3

/*
 * Initialize TTYPE/MTTS defaults on a descriptor
 */
void ttype_init( DESCRIPTOR_DATA *d ) {
	if ( d == NULL )
		return;

	d->ttype_enabled     = FALSE;
	d->ttype_round       = 0;
	d->mtts_flags        = 0;
	d->client_name[0]    = '\0';
	d->terminal_type[0]  = '\0';
}

/*
 * Send TTYPE SEND subnegotiation to request terminal info
 */
void ttype_request( DESCRIPTOR_DATA *d ) {
	if ( d == NULL )
		return;

	write_to_descriptor( d, (char *) ttype_send, 6 );
}

/*
 * Handle incoming TTYPE subnegotiation response from client
 *
 * Data format: TELQUAL_IS <terminal-type-string>
 * (The IAC SB TTYPE prefix and IAC SE suffix are already stripped by caller)
 *
 * Round 1: Client sends terminal name (e.g., "MUDLET", "XTERM-256COLOR")
 * Round 2: Client sends terminal name again (may differ)
 * Round 3: Client sends "MTTS <flags>" bitfield
 */
void ttype_handle_subnegotiation( DESCRIPTOR_DATA *d, unsigned char *data, int len ) {
	char term_type[128];
	int i;

	if ( d == NULL || data == NULL || len < 1 )
		return;

	/* Data must start with TELQUAL_IS (0) */
	if ( data[0] != TELQUAL_IS )
		return;

	/* Extract terminal type string (skip the IS byte) */
	for ( i = 0; i < len - 1 && i < (int) sizeof( term_type ) - 1; i++ )
		term_type[i] = data[i + 1];
	term_type[i] = '\0';

	d->ttype_round++;
	d->ttype_enabled = TRUE;

	if ( d->ttype_round == 1 ) {
		/* Round 1: Store as terminal type (Mudlet sends terminal type here) */
		strncpy( d->terminal_type, term_type, sizeof( d->terminal_type ) - 1 );
		d->terminal_type[sizeof( d->terminal_type ) - 1] = '\0';

		/* Only use as client name if GMCP Core.Hello hasn't already set it */
		if ( d->client_name[0] == '\0' ) {
			strncpy( d->client_name, term_type, sizeof( d->client_name ) - 1 );
			d->client_name[sizeof( d->client_name ) - 1] = '\0';
		}

		/* Request round 2 */
		ttype_request( d );
	} else if ( d->ttype_round == 2 ) {
		/* Round 2: May be a terminal name or MTTS flags string.
		 * Only overwrite terminal_type if not an MTTS flags string. */
		if ( strncmp( term_type, "MTTS ", 5 ) != 0 ) {
			strncpy( d->terminal_type, term_type, sizeof( d->terminal_type ) - 1 );
			d->terminal_type[sizeof( d->terminal_type ) - 1] = '\0';
		}

		/* Request round 3 for MTTS */
		ttype_request( d );
	} else if ( d->ttype_round == 3 ) {
		/* Round 3: MTTS capability bitfield */
		if ( !strncmp( term_type, "MTTS ", 5 ) ) {
			d->mtts_flags = atoi( term_type + 5 );
		}
		/* Do not request further rounds */
	}
}

/*
 * Apply MTTS-detected capabilities to a player character.
 *
 * Upgrade-only: sets capability flags when MTTS reports higher capabilities
 * than the player currently has enabled. Never removes flags, so manual
 * player settings are preserved.
 *
 * Called on new character creation, returning player login, reconnect,
 * and kickoff to ensure all login paths benefit from MTTS detection.
 */
void ttype_apply_mtts( DESCRIPTOR_DATA *d, CHAR_DATA *ch ) {
	if ( d == NULL || ch == NULL || IS_NPC( ch ) )
		return;
	if ( !d->ttype_enabled )
		return;

	/* MTTS round 3 bitfield-based upgrades */
	if ( d->mtts_flags > 0 ) {
		/* Color capability upgrades (highest first) */
		if ( ( d->mtts_flags & MTTS_TRUECOLOR ) && !IS_EXTRA( ch, EXTRA_TRUECOLOR ) ) {
			SET_BIT( ch->act, PLR_ANSI );
			SET_BIT( ch->act, PLR_XTERM );
			SET_BIT( ch->extra, EXTRA_TRUECOLOR );
		} else if ( ( d->mtts_flags & MTTS_256_COLORS ) && !IS_SET( ch->act, PLR_XTERM ) ) {
			SET_BIT( ch->act, PLR_ANSI );
			SET_BIT( ch->act, PLR_XTERM );
		} else if ( ( d->mtts_flags & MTTS_ANSI ) && !IS_SET( ch->act, PLR_ANSI ) ) {
			SET_BIT( ch->act, PLR_ANSI );
		}

		/* Screen reader mode */
		if ( ( d->mtts_flags & MTTS_SCREEN_READER ) && !IS_SET( ch->act, PLR_SCREENREADER ) ) {
			SET_BIT( ch->act, PLR_SCREENREADER );
			SET_BIT( ch->act, PLR_BLANK );
			SET_BIT( ch->act, PLR_AUTOEXIT );
			d->mxp_enabled = FALSE;
		}
		return;
	}

	/*
	 * Round 2 fallback: for non-MTTS clients (mtts_flags == 0) that still
	 * provided a terminal type string in round 2 (e.g., raw telnet sends
	 * "VT100"), infer basic ANSI capability from the terminal type name.
	 */
	if ( d->terminal_type[0] != '\0' && !IS_SET( ch->act, PLR_ANSI ) ) {
		if ( !str_prefix( "XTERM", d->terminal_type )
		||   !str_prefix( "VT100", d->terminal_type )
		||   !str_prefix( "ANSI", d->terminal_type ) ) {
			SET_BIT( ch->act, PLR_ANSI );
		}
	}
}
