/***************************************************************************
 *  ability_config.h - Per-ability balance configuration
 *
 *  Stores per-ability tuning constants (cooldowns, costs, damage, etc.)
 *  in an ability_config table in game.db.  Values are loaded at boot and
 *  can be viewed/modified at runtime via the 'ability' admin command.
 *
 *  Type-safe API: All lookups use acfg_key_t enum for compile-time safety.
 ***************************************************************************/

#ifndef ABILITY_CONFIG_H
#define ABILITY_CONFIG_H

#include "merc.h"
#include "acfg_keys.h"  /* For acfg_key_t enum */

typedef struct acfg_entry {
	const char *key;
	int         value;
	int         default_value;
} acfg_entry_t;


/* ================================================================
 * Type-safe lookup API (use these in code)
 * ================================================================ */

/* Lookup a value by enum key. O(1) array index. */
int acfg( acfg_key_t key );

/* Get default value for a key. */
int acfg_default( acfg_key_t key );

/* Set a value by enum key. */
void acfg_set( acfg_key_t key, int value );

/* Reset a key to its default value. */
void acfg_reset( acfg_key_t key );


/* ================================================================
 * String-to-Enum API (for admin commands & database layer)
 * ================================================================ */

/* Convert string key to enum. Returns ACFG_COUNT if not found. */
acfg_key_t acfg_key_from_string( const char *key );

/* Get string key for an enum (for display/database). */
const char *acfg_key_to_string( acfg_key_t key );

/* Get a pointer to the table entry for an enum key (NULL if invalid). */
acfg_entry_t *acfg_entry( acfg_key_t key );


/* ================================================================
 * Iteration API (for admin commands)
 * ================================================================ */

/* Count of entries (equals ACFG_COUNT). */
int acfg_count( void );

/* Get entry by index (NULL if out of range). For iteration only. */
acfg_entry_t *acfg_entry_by_index( int index );


/* ================================================================
 * Load / Save
 * ================================================================ */

/* Initialize defaults then load overrides from game.db */
void load_ability_config( void );

/* Save all values to game.db */
void save_ability_config( void );

#endif /* ABILITY_CONFIG_H */
