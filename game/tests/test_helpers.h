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
 * Allocated with calloc, list heads initialized.
 * Must be freed with free_test_char().
 */
CHAR_DATA *make_test_player( void );

/*
 * Create a minimal NPC character for testing NPC-specific paths.
 * Has ACT_IS_NPC set, no pcdata. List heads initialized.
 */
CHAR_DATA *make_test_npc( void );

/*
 * Create a fully initialized NPC via clear_char().
 * All string fields allocated, all lists initialized.
 * Suitable for functions that call act(), char_to_room(), etc.
 * Must be freed with free_char() (not free_test_char).
 */
CHAR_DATA *make_full_test_npc( void );

/*
 * Free a character created by make_test_player() or make_test_npc().
 * Also strips any remaining affects.
 */
void free_test_char( CHAR_DATA *ch );

/*
 * Seed the RNG for deterministic testing.
 * Sets current_time to the given value and calls init_mm().
 */
void seed_rng( int seed );

/*
 * Boot the game engine once (via boot_headless). Subsequent calls are no-ops.
 * Returns TRUE on success.
 */
bool ensure_booted( void );

/*
 * Create a stack-allocated AFFECT_DATA for testing.
 */
AFFECT_DATA make_test_affect( int type, int duration, int location,
	int modifier, int bitvector );

/*
 * Find a command index in cmd_table by name. Returns -1 if not found.
 */
int find_cmd_index( const char *name );

#endif /* TEST_HELPERS_H */
