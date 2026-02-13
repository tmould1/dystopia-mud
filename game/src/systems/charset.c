/*
 * charset.c - Character Set negotiation (RFC 2066) support
 *
 * Detects whether a client supports UTF-8 via TELOPT_CHARSET.
 * For clients that don't support UTF-8, provides transliteration
 * of multi-byte characters to '?' for clean ASCII output.
 */

#include "merc.h"
#include <stdio.h>
#include <string.h>
#include "telnet.h"
#include "charset.h"
#include "ttype.h"
#include "../core/utf8.h"

/*
 * Telnet negotiation strings for CHARSET
 */
#if defined( WIN32 )
const char charset_will[] = { (char) IAC, (char) WILL, (char) TELOPT_CHARSET, '\0' };
const char charset_wont[] = { (char) IAC, (char) WONT, (char) TELOPT_CHARSET, '\0' };
const char charset_do[]   = { (char) IAC, (char) DO,   (char) TELOPT_CHARSET, '\0' };
const char charset_dont[] = { (char) IAC, (char) DONT, (char) TELOPT_CHARSET, '\0' };
#else
const char charset_will[] = { IAC, WILL, TELOPT_CHARSET, '\0' };
const char charset_wont[] = { IAC, WONT, TELOPT_CHARSET, '\0' };
const char charset_do[]   = { IAC, DO,   TELOPT_CHARSET, '\0' };
const char charset_dont[] = { IAC, DONT, TELOPT_CHARSET, '\0' };
#endif

/*
 * CHARSET REQUEST subnegotiation:
 * IAC SB CHARSET REQUEST <space> U T F - 8 IAC SE
 */
#if defined( WIN32 )
static const char charset_request_utf8[] = {
	(char) IAC, (char) SB, (char) TELOPT_CHARSET,
	(char) CHARSET_REQUEST, ' ',
	'U', 'T', 'F', '-', '8',
	(char) IAC, (char) SE, '\0'
};
#else
static const char charset_request_utf8[] = {
	IAC, SB, TELOPT_CHARSET,
	CHARSET_REQUEST, ' ',
	'U', 'T', 'F', '-', '8',
	IAC, SE, '\0'
};
#endif

/*
 * Initialize charset defaults on a descriptor
 */
void charset_init( DESCRIPTOR_DATA *d ) {
	if ( d == NULL )
		return;

	d->client_charset = CHARSET_UNKNOWN;
	d->charset_negotiated = FALSE;
}

/*
 * Send CHARSET REQUEST subnegotiation to the client.
 * Called when the client responds with IAC DO CHARSET.
 */
void charset_send_request( DESCRIPTOR_DATA *d ) {
	if ( d == NULL )
		return;

	write_to_descriptor( d, (char *) charset_request_utf8,
		(int) strlen( charset_request_utf8 ) );
}

/*
 * Handle incoming CHARSET subnegotiation from client.
 * Data starts AFTER "IAC SB CHARSET" and ends BEFORE "IAC SE".
 *
 * Expected formats:
 *   ACCEPTED UTF-8     (opcode 2, then "UTF-8")
 *   REJECTED           (opcode 3)
 */
void charset_handle_subnegotiation( DESCRIPTOR_DATA *d, unsigned char *data, int len ) {
	if ( d == NULL || data == NULL || len < 1 )
		return;

	switch ( data[0] ) {
	case CHARSET_ACCEPTED:
		/* Client accepted - check if it's UTF-8 */
		if ( len >= 6
			&& data[1] == ' '
			&& ( data[2] == 'U' || data[2] == 'u' )
			&& ( data[3] == 'T' || data[3] == 't' )
			&& ( data[4] == 'F' || data[4] == 'f' )
			&& data[5] == '-'
			&& data[6] == '8' ) {
			d->client_charset = CHARSET_UTF8;
			d->charset_negotiated = TRUE;
		}
		break;

	case CHARSET_REJECTED:
		/* Client rejected our charset offer */
		d->client_charset = CHARSET_ASCII;
		d->charset_negotiated = TRUE;
		break;

	default:
		/* Unknown opcode - ignore */
		break;
	}
}

/*
 * Finalize charset if still unknown.
 * Called when the player sends their first actual command.
 *
 * Priority:
 *   1. Explicit CHARSET negotiation result (already set, nothing to do)
 *   2. MTTS UTF-8 flag from TTYPE round 3
 *   3. Default to UTF-8 (virtually all modern clients support it)
 *
 * Only clients that explicitly REJECTED via CHARSET get ASCII.
 */
void charset_finalize( DESCRIPTOR_DATA *d ) {
	if ( d == NULL )
		return;

	if ( !d->charset_negotiated ) {
		/* Check MTTS flags (from TTYPE round 3) for UTF-8 support */
		if ( d->mtts_flags & MTTS_UTF8 ) {
			d->client_charset = CHARSET_UTF8;
		} else {
			/* Default to UTF-8: in 2026, virtually all MUD clients
			 * handle UTF-8. Only CHARSET REJECTED sets ASCII. */
			d->client_charset = CHARSET_UTF8;
		}
		d->charset_negotiated = TRUE;
	}
}

/*
 * Check if descriptor supports UTF-8
 */
bool charset_is_utf8( DESCRIPTOR_DATA *d ) {
	if ( d == NULL )
		return FALSE;

	return ( d->client_charset == CHARSET_UTF8 );
}

/*
 * Transliterate UTF-8 multi-byte sequences to '?' in-place.
 * Used for output to clients that don't support UTF-8.
 * Returns the new length of the buffer.
 *
 * Each multi-byte sequence (2-4 bytes) is replaced with a single '?',
 * and the rest of the buffer is shifted left to close the gap.
 * Telnet IAC sequences (0xFF + command) are preserved unchanged.
 */
int charset_transliterate( char *buf, int len ) {
	int src, dst;

	if ( buf == NULL || len <= 0 )
		return 0;

	for ( src = 0, dst = 0; src < len; ) {
		unsigned char b = (unsigned char) buf[src];

		if ( b < 0x80 ) {
			/* ASCII byte - copy as-is */
			buf[dst++] = buf[src++];
		} else if ( b == 0xFF ) {
			/* Telnet IAC - preserve IAC + following command byte */
			buf[dst++] = buf[src++];
			if ( src < len )
				buf[dst++] = buf[src++];
		} else if ( utf8_is_cont( b ) ) {
			/* Stray continuation byte - skip */
			src++;
		} else {
			/* UTF-8 lead byte - skip the whole sequence, emit '?' */
			int seq = utf8_seq_len( b );
			if ( seq == 0 ) seq = 1; /* invalid byte, skip one */
			buf[dst++] = '?';
			src += seq;
			if ( src > len ) src = len; /* bounds safety */
		}
	}
	buf[dst] = '\0';
	return dst;
}
