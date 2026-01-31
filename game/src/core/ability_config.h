/***************************************************************************
 *  ability_config.h - Per-ability balance configuration
 *
 *  Stores per-ability tuning constants (cooldowns, costs, damage, etc.)
 *  in an ability_config table in game.db.  Values are loaded at boot and
 *  can be viewed/modified at runtime via the 'ability' admin command.
 ***************************************************************************/

#ifndef ABILITY_CONFIG_H
#define ABILITY_CONFIG_H

#include "merc.h"

typedef struct acfg_entry {
	const char *key;
	int         value;
	int         default_value;
} acfg_entry_t;

/* Lookup a value by dotted key (e.g. "vampire.spiritgate.cooldown").
 * Returns 0 if key not found. */
int  acfg( const char *key );

/* Set a value by key. Returns TRUE if key exists, FALSE otherwise. */
bool acfg_set( const char *key, int value );

/* Get a pointer to the table entry for a key (NULL if not found). */
acfg_entry_t *acfg_find( const char *key );

/* Count of entries in acfg_table (excluding NULL sentinel). */
int acfg_count( void );

/* Get a pointer to a table entry by index (NULL if out of range). */
acfg_entry_t *acfg_find_by_index( int index );

/* Initialize defaults then load overrides from game.db */
void load_ability_config( void );

/* Save all values to game.db */
void save_ability_config( void );

#endif /* ABILITY_CONFIG_H */
