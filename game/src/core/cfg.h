/***************************************************************************
 *  cfg.h - Unified game configuration system
 *
 *  This is the single configuration system for all tunable game values.
 *  It replaces both the 'balance' and 'acfg' (ability config) systems.
 *
 *  Categories:
 *    combat.*      - Damage caps, multipliers, weapon formulas
 *    progression.* - Upgrade bonuses, generation bonuses
 *    world.*       - Time scale, weather, regeneration
 *    ability.*     - Per-class ability parameters (cooldowns, costs, etc.)
 *
 *  Type-safe API: All lookups use cfg_key_t enum for compile-time safety.
 ***************************************************************************/

#ifndef CFG_H
#define CFG_H

#include "cfg_keys.h"  /* For cfg_key_t enum and CFG_ENTRIES */

typedef struct cfg_entry {
	const char *key;           /* Dotted string key */
	int         value;         /* Current runtime value */
	int         default_value; /* Compile-time default */
} cfg_entry_t;


/* ================================================================
 * Type-safe lookup API (use these in code)
 * All lookups are O(1) array indexing.
 * ================================================================ */

/* Get config value by enum key. */
int cfg( cfg_key_t key );

/* Get default value for a key. */
int cfg_default( cfg_key_t key );

/* Set a value by enum key. */
void cfg_set( cfg_key_t key, int value );

/* Reset a key to its default value. */
void cfg_reset( cfg_key_t key );

/* Reset all keys to their default values. */
void cfg_reset_all( void );


/* ================================================================
 * String-to-Enum API (for admin commands & database layer)
 * ================================================================ */

/* Convert string key to enum. Returns CFG_COUNT if not found. */
cfg_key_t cfg_key_from_string( const char *key );

/* Get string key for an enum (for display/database). */
const char *cfg_key_to_string( cfg_key_t key );

/* Get a pointer to the table entry for an enum key (NULL if invalid). */
cfg_entry_t *cfg_entry( cfg_key_t key );


/* ================================================================
 * Iteration API (for admin commands)
 * ================================================================ */

/* Count of entries (equals CFG_COUNT). */
int cfg_count( void );

/* Get entry by index (NULL if out of range). For iteration only. */
cfg_entry_t *cfg_entry_by_index( int index );


/* ================================================================
 * Load / Save
 * ================================================================ */

/* Initialize defaults then load overrides from game.db */
void load_cfg( void );

/* Save all non-default values to game.db */
void save_cfg( void );


/* ================================================================
 * Indexed lookup helpers (for array-style config values)
 * ================================================================ */

/* Get upgrade damage multiplier for level 1-10. Returns 100 if out of range. */
int cfg_upgrade_dmg( int level );

/* Get ninja belt damage multiplier for tier 1-10. Returns 100 if out of range. */
int cfg_ninja_belt_mult( int tier );

/* Get generation damcap bonus for gen 1-5. Returns 0 if out of range. */
int cfg_gen_damcap( int gen );

/* Get superstance damcap bonus for tier 1-3. Returns 0 if out of range. */
int cfg_superstance_cap( int tier );

/* Get reverse superstance damcap penalty for tier 1-3. Returns 0 if out of range. */
int cfg_rev_superstance_cap( int tier );


#endif /* CFG_H */
