/***************************************************************************
 *  ability_config.c - Per-ability balance configuration
 *
 *  Stores per-ability tuning constants (cooldowns, costs, damage, etc.)
 *  in an ability_config table in game.db.  Values are loaded at boot and
 *  can be viewed/modified at runtime via the 'ability' admin command.
 *
 *  Type-safe implementation using X-macros for compile-time safety.
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "ability_config.h"
#include "../db/db_game.h"


/* ================================================================
 * Table generated from X-macro - ORDER MATCHES ENUM
 * ================================================================ */

static acfg_entry_t acfg_table[] = {
#define ACFG_X(name, key, def) { key, def, def },
	ACFG_ENTRIES
#undef ACFG_X
	{ NULL, 0, 0 }  /* Sentinel */
};


/* ================================================================
 * Type-safe lookup functions (O(1) array index)
 * ================================================================ */

int acfg( acfg_key_t key ) {
	if ( key < 0 || key >= ACFG_COUNT ) {
		log_string( "acfg: invalid key index" );
		return 0;
	}
	return acfg_table[key].value;
}

int acfg_default( acfg_key_t key ) {
	if ( key < 0 || key >= ACFG_COUNT )
		return 0;
	return acfg_table[key].default_value;
}

void acfg_set( acfg_key_t key, int value ) {
	if ( key < 0 || key >= ACFG_COUNT )
		return;
	acfg_table[key].value = value;
}

void acfg_reset( acfg_key_t key ) {
	if ( key < 0 || key >= ACFG_COUNT )
		return;
	acfg_table[key].value = acfg_table[key].default_value;
}

acfg_entry_t *acfg_entry( acfg_key_t key ) {
	if ( key < 0 || key >= ACFG_COUNT )
		return NULL;
	return &acfg_table[key];
}


/* ================================================================
 * String-to-Enum conversion (for admin commands & database)
 * Uses linear search - only called interactively, not performance critical.
 * ================================================================ */

acfg_key_t acfg_key_from_string( const char *key ) {
	int i;

	if ( !key || key[0] == '\0' )
		return ACFG_COUNT;

	for ( i = 0; i < ACFG_COUNT; i++ ) {
		if ( !str_cmp( key, acfg_table[i].key ) )
			return (acfg_key_t) i;
	}

	return ACFG_COUNT;
}

const char *acfg_key_to_string( acfg_key_t key ) {
	if ( key < 0 || key >= ACFG_COUNT )
		return NULL;
	return acfg_table[key].key;
}


/* ================================================================
 * Iteration API
 * ================================================================ */

int acfg_count( void ) {
	return ACFG_COUNT;
}

acfg_entry_t *acfg_entry_by_index( int index ) {
	if ( index < 0 || index >= ACFG_COUNT )
		return NULL;
	return &acfg_table[index];
}


/* ================================================================
 * Load / Save wrappers
 * ================================================================ */

void load_ability_config( void ) {
	char buf[MAX_STRING_LENGTH];

	/* Defaults are already baked into the table initializer.
	 * Just load overrides from the DB. */
	db_game_load_ability_config();

	snprintf( buf, sizeof( buf ),
		"  Ability config: %d entries registered", ACFG_COUNT );
	log_string( buf );
}

void save_ability_config( void ) {
	db_game_save_ability_config();
}


/* ================================================================
 * Admin command: do_ability
 * ================================================================ */

/*
 * Extract the class prefix from a dotted key (everything before the
 * first dot).  Returns a pointer to a static buffer.
 */
static const char *acfg_class_prefix( const char *key ) {
	static char buf[64];
	const char *dot = strchr( key, '.' );

	if ( !dot ) {
		snprintf( buf, sizeof( buf ), "%s", key );
		return buf;
	}

	{
		size_t len = (size_t)( dot - key );
		if ( len >= sizeof( buf ) )
			len = sizeof( buf ) - 1;
		memcpy( buf, key, len );
		buf[len] = '\0';
	}
	return buf;
}


void do_ability( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char output[MAX_STRING_LENGTH * 4];
	int i, count;
	acfg_key_t key;
	acfg_entry_t *e;

	argument = one_argument( argument, arg );
	argument = one_argument( argument, arg2 );

	/* No args: show summary by class */
	if ( arg[0] == '\0' ) {
		char last_class[64];
		int class_count = 0;
		int total_classes = 0;

		last_class[0] = '\0';
		output[0] = '\0';
		snprintf( buf, sizeof( buf ),
			"#yAbility Config#n  (%d total entries)\r\n"
			"#y---------------------------------------------#n\r\n",
			ACFG_COUNT );
		strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

		for ( i = 0; i < ACFG_COUNT; i++ ) {
			const char *cls = acfg_class_prefix( acfg_table[i].key );
			if ( str_cmp( cls, last_class ) ) {
				if ( last_class[0] != '\0' ) {
					snprintf( buf, sizeof( buf ),
						"  #g%-20s#n  %d entries\r\n",
						last_class, class_count );
					strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
					total_classes++;
				}
				snprintf( last_class, sizeof( last_class ), "%s", cls );
				class_count = 1;
			} else {
				class_count++;
			}
		}
		if ( last_class[0] != '\0' ) {
			snprintf( buf, sizeof( buf ),
				"  #g%-20s#n  %d entries\r\n",
				last_class, class_count );
			strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
			total_classes++;
		}

		snprintf( buf, sizeof( buf ),
			"#y---------------------------------------------#n\r\n"
			"%d classes.  Use #yability <class>#n for details.\r\n"
			"#yability <key> <value>#n to modify.  "
			"#yability reload#n to reload from DB.\r\n",
			total_classes );
		strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

		send_to_char( output, ch );
		return;
	}

	/* "ability reload" - reload from DB */
	if ( !str_cmp( arg, "reload" ) ) {
		/* Reset all to defaults first */
		for ( i = 0; i < ACFG_COUNT; i++ )
			acfg_table[i].value = acfg_table[i].default_value;
		db_game_load_ability_config();
		send_to_char( "Ability config reloaded from database.\r\n", ch );
		return;
	}

	/* "ability defaults" - reset all to defaults */
	if ( !str_cmp( arg, "defaults" ) ) {
		for ( i = 0; i < ACFG_COUNT; i++ )
			acfg_table[i].value = acfg_table[i].default_value;
		db_game_save_ability_config();
		send_to_char( "All ability config values reset to defaults.\r\n", ch );
		return;
	}

	/* "ability reset <key>" - reset single key */
	if ( !str_cmp( arg, "reset" ) ) {
		if ( arg2[0] == '\0' ) {
			send_to_char( "Reset which key?  Usage: ability reset <key>\r\n", ch );
			return;
		}

		key = acfg_key_from_string( arg2 );
		if ( key == ACFG_COUNT ) {
			send_to_char( "Unknown ability config key.\r\n", ch );
			return;
		}

		acfg_reset( key );
		db_game_save_ability_config();
		e = acfg_entry( key );
		snprintf( buf, sizeof( buf ),
			"Reset #g%s#n to default: #y%d#n\r\n",
			e->key, e->default_value );
		send_to_char( buf, ch );
		return;
	}

	/* "ability <key> <value>" - set a value */
	if ( arg2[0] != '\0' ) {
		int val;

		key = acfg_key_from_string( arg );
		if ( key == ACFG_COUNT ) {
			send_to_char( "Unknown ability config key.\r\n", ch );
			return;
		}

		val = atoi( arg2 );
		acfg_set( key, val );
		db_game_save_ability_config();
		e = acfg_entry( key );
		snprintf( buf, sizeof( buf ),
			"Set #g%s#n = #y%d#n (default: %d)\r\n",
			e->key, e->value, e->default_value );
		send_to_char( buf, ch );
		return;
	}

	/* "ability <prefix>" - show matching entries */
	output[0] = '\0';
	count = 0;

	for ( i = 0; i < ACFG_COUNT; i++ ) {
		if ( !str_prefix( arg, acfg_table[i].key ) ) {
			const char *modified =
				( acfg_table[i].value != acfg_table[i].default_value )
				? " #r*#n" : "";

			snprintf( buf, sizeof( buf ),
				"  #g%-50s#n  #y%d#n  (default: %d)%s\r\n",
				acfg_table[i].key,
				acfg_table[i].value,
				acfg_table[i].default_value,
				modified );
			strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
			count++;
		}
	}

	if ( count == 0 ) {
		send_to_char( "No matching ability config keys found.\r\n", ch );
		return;
	}

	snprintf( buf, sizeof( buf ),
		"#y--- %d matching entries ---#n\r\n", count );
	strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

	send_to_char( output, ch );
}
