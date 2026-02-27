/*
 * Test helpers for Dystopia MUD unit tests
 *
 * Provides factory functions for creating minimal game objects
 * suitable for testing without booting the full game.
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "merc.h"

/*
 * Create a minimal player character for stat/attribute testing.
 * Allocated with calloc so all fields default to 0/NULL.
 * Must be freed with free_test_char().
 */
CHAR_DATA *make_test_player( void );

/*
 * Create a minimal NPC character for testing NPC-specific paths.
 * Has ACT_IS_NPC set, no pcdata.
 */
CHAR_DATA *make_test_npc( void );

/*
 * Free a character created by make_test_player() or make_test_npc().
 */
void free_test_char( CHAR_DATA *ch );

/*
 * Seed the RNG for deterministic testing.
 * Sets current_time to the given value and calls init_mm().
 */
void seed_rng( int seed );

#endif /* TEST_HELPERS_H */
