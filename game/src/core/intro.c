/*
 * intro.c - Connection intro system
 *
 * When a player connects, the server sends telnet negotiation offers and
 * enters CON_DETECT_CAPS state. This module waits for client responses
 * (TTYPE/MTTS, NAWS, GMCP, etc.), classifies the client's capabilities,
 * and sends the appropriate tier of intro content from the help database.
 *
 * Tier content is stored as help entries:
 *   intro_rich          - Truecolor/256-color, wide terminal
 *   intro_standard      - ANSI 16-color, 80-column
 *   intro_basic         - Plain text, no color codes
 *   intro_screenreader  - Accessible prose for screen readers
 */

#include "merc.h"
#include "intro.h"
#include "../systems/ttype.h"
#include "../systems/charset.h"

/*
 * Cached pointers to intro help text, set by intro_load().
 * Falls through tiers: rich → standard → basic → help_greeting.
 */
static char *intro_text[INTRO_TIER_MAX];

/* Help keywords for each tier */
static const char *intro_keywords[INTRO_TIER_MAX] = {
	"intro_rich",
	"intro_standard",
	"intro_basic",
	"intro_screenreader"
};

/*
 * Classify a descriptor's detected capabilities into a tier.
 */
static intro_tier_t intro_classify_tier( DESCRIPTOR_DATA *d ) {
	if ( d == NULL )
		return INTRO_TIER_BASIC;

	/* Screen reader takes priority */
	if ( d->mtts_flags & MTTS_SCREEN_READER )
		return INTRO_TIER_SCREENREADER;

	/* Truecolor or 256-color = rich (if terminal is wide enough) */
	if ( ( d->mtts_flags & MTTS_TRUECOLOR ) || ( d->mtts_flags & MTTS_256_COLORS ) ) {
		if ( d->naws_enabled && d->client_width >= 100 )
			return INTRO_TIER_RICH;
		return INTRO_TIER_STANDARD;
	}

	/* ANSI color from MTTS or terminal type inference */
	if ( ( d->mtts_flags & MTTS_ANSI ) || d->ttype_enabled )
		return INTRO_TIER_STANDARD;

	/* GMCP-capable clients are typically modern (Mudlet, etc.) */
	if ( d->gmcp_enabled )
		return INTRO_TIER_STANDARD;

	return INTRO_TIER_BASIC;
}

/*
 * Get the intro text for a given tier, falling back to lower tiers.
 * Fallback order: requested tier → standard → basic → help_greeting.
 */
static const char *intro_get_text( intro_tier_t tier ) {
	/* Try the requested tier first */
	if ( intro_text[tier] != NULL )
		return intro_text[tier];

	/* Fall back through tiers */
	if ( tier == INTRO_TIER_RICH && intro_text[INTRO_TIER_STANDARD] != NULL )
		return intro_text[INTRO_TIER_STANDARD];

	if ( tier != INTRO_TIER_BASIC && intro_text[INTRO_TIER_BASIC] != NULL )
		return intro_text[INTRO_TIER_BASIC];

	/* Last resort: use the original help greeting */
	{
		extern char *help_greeting;
		if ( help_greeting != NULL )
			return help_greeting;
	}

	return "\n\rWelcome.\n\r\n\rWhat is your name? ";
}

/*
 * Send the intro sequence for a descriptor and transition to CON_GET_NAME.
 */
static void intro_send( DESCRIPTOR_DATA *d ) {
	intro_tier_t tier;
	const char *text;

	tier = intro_classify_tier( d );
	text = intro_get_text( tier );

	/* Skip leading '.' (same convention as help_greeting) */
	if ( text[0] == '.' )
		text++;

	write_to_buffer( d, text, 0 );

	/* Finalize charset if still pending */
	charset_finalize( d );

	d->connected = CON_GET_NAME;
}

/*
 * Load intro text from the help database.
 * Walks the help list looking for intro_* keywords and caches pointers.
 * Called once during boot_db() after all help entries are loaded.
 */
void intro_load( void ) {
	HELP_DATA *pHelp;
	int tier;
	int loaded = 0;

	/* Clear cached pointers */
	for ( tier = 0; tier < INTRO_TIER_MAX; tier++ )
		intro_text[tier] = NULL;

	/* Walk the help list looking for intro keywords */
	for ( pHelp = first_help; pHelp != NULL; pHelp = pHelp->next ) {
		for ( tier = 0; tier < INTRO_TIER_MAX; tier++ ) {
			if ( !str_cmp( pHelp->keyword, intro_keywords[tier] ) ) {
				intro_text[tier] = pHelp->text;
				loaded++;
				break;
			}
		}
	}

	{
		char buf[128];
		snprintf( buf, sizeof( buf ), "  Intro system: %d of %d tiers loaded.",
			loaded, INTRO_TIER_MAX );
		log_string( buf );
	}
}

/*
 * Called each pulse for descriptors in CON_DETECT_CAPS.
 * Checks whether capability detection is complete (or timed out)
 * and sends the intro when ready.
 */
void intro_check_ready( DESCRIPTOR_DATA *d ) {
	if ( d == NULL || d->connected != CON_DETECT_CAPS )
		return;

	d->intro_pulse++;

	/* Short-circuit: MTTS round 3 complete = full capability picture */
	if ( d->ttype_round >= 3 ) {
		intro_send( d );
		return;
	}

	/* Short-circuit: TTYPE explicitly refused and we've waited a bit */
	if ( !d->ttype_enabled && d->ttype_round == 0 && d->intro_pulse >= 2 ) {
		intro_send( d );
		return;
	}

	/* Timeout: wait at most INTRO_DETECT_TIMEOUT pulses.
	 * If TTYPE negotiation is actively in progress (rounds started but
	 * not yet complete), allow up to 3x the timeout so slow connections
	 * can finish all 3 rounds. This prevents the intro from firing
	 * mid-negotiation while still bounding the total wait. */
	if ( d->intro_pulse >= INTRO_DETECT_TIMEOUT ) {
		if ( d->ttype_enabled && d->ttype_round > 0 && d->ttype_round < 3
			&& d->intro_pulse < INTRO_DETECT_TIMEOUT * 3 )
			return;
		intro_send( d );
		return;
	}
}
