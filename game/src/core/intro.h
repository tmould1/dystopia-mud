/*
 * intro.h - Connection intro system
 *
 * Manages the initial connection experience: waits for telnet capability
 * negotiation to complete, classifies the client into a capability tier,
 * then sends an appropriate intro sequence from the help database.
 *
 * Help keywords: intro_rich, intro_standard, intro_basic, intro_screenreader
 */

#ifndef INTRO_H
#define INTRO_H

/* Capability tiers for intro content selection */
typedef enum {
	INTRO_TIER_RICH,          /* Truecolor/256-color + wide terminal */
	INTRO_TIER_STANDARD,      /* ANSI 16-color */
	INTRO_TIER_BASIC,         /* No color detected */
	INTRO_TIER_SCREENREADER,  /* Screen reader active */
	INTRO_TIER_MAX
} intro_tier_t;

/* Maximum pulses to wait for capability detection (5 = 1.25 seconds) */
#define INTRO_DETECT_TIMEOUT 5

/*
 * Load intro text from help database.
 * Called during boot_db() after helps are loaded.
 * Caches pointers to help text for intro_rich, intro_standard,
 * intro_basic, and intro_screenreader keywords.
 */
void intro_load( void );

/*
 * Called each pulse for descriptors in CON_DETECT_CAPS state.
 * Increments the detection timer and checks whether enough capability
 * data has been gathered (or timeout reached). When ready, classifies
 * the client tier, sends the appropriate intro, and transitions to
 * CON_GET_NAME.
 */
void intro_check_ready( DESCRIPTOR_DATA *d );

#endif /* INTRO_H */
