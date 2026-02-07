/***************************************************************************
 *  cfg.c - Unified game configuration system
 *
 *  This is the single configuration system for all tunable game values.
 *  It replaces both the 'balance' and 'acfg' (ability config) systems.
 *
 *  Type-safe implementation using X-macros for compile-time safety.
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "cfg.h"
#include "../db/db_game.h"


/* ================================================================
 * Table generated from X-macro - ORDER MATCHES ENUM
 * ================================================================ */

static cfg_entry_t cfg_table[] = {
#define CFG_X(name, key, def) { key, def, def },
	CFG_ENTRIES
#undef CFG_X
	{ NULL, 0, 0 }  /* Sentinel */
};


/* ================================================================
 * Type-safe lookup functions (O(1) array index)
 * ================================================================ */

int cfg( cfg_key_t key ) {
	if ( key < 0 || key >= CFG_COUNT ) {
		log_string( "cfg: invalid key index" );
		return 0;
	}
	return cfg_table[key].value;
}

int cfg_default( cfg_key_t key ) {
	if ( key < 0 || key >= CFG_COUNT )
		return 0;
	return cfg_table[key].default_value;
}

void cfg_set( cfg_key_t key, int value ) {
	if ( key < 0 || key >= CFG_COUNT )
		return;
	cfg_table[key].value = value;
}

void cfg_reset( cfg_key_t key ) {
	if ( key < 0 || key >= CFG_COUNT )
		return;
	cfg_table[key].value = cfg_table[key].default_value;
}

void cfg_reset_all( void ) {
	int i;
	for ( i = 0; i < CFG_COUNT; i++ )
		cfg_table[i].value = cfg_table[i].default_value;
}

cfg_entry_t *cfg_entry( cfg_key_t key ) {
	if ( key < 0 || key >= CFG_COUNT )
		return NULL;
	return &cfg_table[key];
}


/* ================================================================
 * String-to-Enum conversion (for admin commands & database)
 * Uses linear search - only called interactively, not performance critical.
 * ================================================================ */

cfg_key_t cfg_key_from_string( const char *key ) {
	int i;

	if ( !key || key[0] == '\0' )
		return CFG_COUNT;

	for ( i = 0; i < CFG_COUNT; i++ ) {
		if ( !str_cmp( key, cfg_table[i].key ) )
			return (cfg_key_t) i;
	}

	return CFG_COUNT;
}

const char *cfg_key_to_string( cfg_key_t key ) {
	if ( key < 0 || key >= CFG_COUNT )
		return NULL;
	return cfg_table[key].key;
}


/* ================================================================
 * Iteration API
 * ================================================================ */

int cfg_count( void ) {
	return CFG_COUNT;
}

cfg_entry_t *cfg_entry_by_index( int index ) {
	if ( index < 0 || index >= CFG_COUNT )
		return NULL;
	return &cfg_table[index];
}


/* ================================================================
 * Load / Save
 * ================================================================ */

void load_cfg( void ) {
	char buf[MAX_STRING_LENGTH];

	/* Defaults are already baked into the table initializer.
	 * Just load overrides from the DB. */
	db_game_load_cfg();

	snprintf( buf, sizeof( buf ),
		"  Config: %d entries registered", CFG_COUNT );
	log_string( buf );
}

void save_cfg( void ) {
	db_game_save_cfg();
}


/* ================================================================
 * Indexed lookup helpers
 *
 * These provide convenient access to array-indexed config values.
 * ================================================================ */

int cfg_upgrade_dmg( int level ) {
	static const cfg_key_t keys[] = {
		CFG_PROGRESSION_UPGRADE_DMG_1,
		CFG_PROGRESSION_UPGRADE_DMG_2,
		CFG_PROGRESSION_UPGRADE_DMG_3,
		CFG_PROGRESSION_UPGRADE_DMG_4,
		CFG_PROGRESSION_UPGRADE_DMG_5
	};

	if ( level < 1 || level > 5 )
		return 100;  /* No multiplier */

	return cfg( keys[level - 1] );
}

int cfg_ninja_belt_mult( int tier ) {
	static const cfg_key_t keys[] = {
		CFG_COMBAT_DMG_MULT_NINJA_BELT_1,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_2,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_3,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_4,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_5,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_6,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_7,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_8,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_9,
		CFG_COMBAT_DMG_MULT_NINJA_BELT_10
	};

	if ( tier < 1 || tier > 10 )
		return 100;  /* No multiplier */

	return cfg( keys[tier - 1] );
}

int cfg_gen_damcap( int gen ) {
	static const cfg_key_t keys[] = {
		CFG_PROGRESSION_GEN_DAMCAP_0,
		CFG_PROGRESSION_GEN_DAMCAP_1,
		CFG_PROGRESSION_GEN_DAMCAP_2,
		CFG_PROGRESSION_GEN_DAMCAP_3,
		CFG_PROGRESSION_GEN_DAMCAP_4
	};

	if ( gen < 1 || gen > 5 )
		return 0;  /* No bonus */

	return cfg( keys[gen - 1] );
}

int cfg_superstance_cap( int tier ) {
	/* Note: Original balance array was backwards - index 0 = tier 3 */
	static const cfg_key_t keys[] = {
		CFG_COMBAT_DAMCAP_SUPERSTANCE_1,
		CFG_COMBAT_DAMCAP_SUPERSTANCE_2,
		CFG_COMBAT_DAMCAP_SUPERSTANCE_3
	};

	if ( tier < 1 || tier > 3 )
		return 0;  /* No bonus */

	return cfg( keys[tier - 1] );
}

int cfg_rev_superstance_cap( int tier ) {
	/* Note: Original balance array was backwards - index 0 = tier 3 */
	static const cfg_key_t keys[] = {
		CFG_COMBAT_DAMCAP_REV_SUPERSTANCE_1,
		CFG_COMBAT_DAMCAP_REV_SUPERSTANCE_2,
		CFG_COMBAT_DAMCAP_REV_SUPERSTANCE_3
	};

	if ( tier < 1 || tier > 3 )
		return 0;  /* No penalty */

	return cfg( keys[tier - 1] );
}


/* ================================================================
 * Helper function for do_cfg
 * Extract the top-level category prefix from a dotted key.
 * ================================================================ */

static const char *cfg_category_prefix( const char *key ) {
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


/* ================================================================
 * Admin command: do_cfg
 * ================================================================ */

void do_cfg( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char output[MAX_STRING_LENGTH * 4];
	int i, count;
	cfg_key_t key;
	cfg_entry_t *e;

	argument = one_argument( argument, arg );
	argument = one_argument( argument, arg2 );

	/* No args: show summary by category */
	if ( arg[0] == '\0' ) {
		char last_cat[64];
		int cat_count = 0;
		int total_cats = 0;

		last_cat[0] = '\0';
		output[0] = '\0';
		snprintf( buf, sizeof( buf ),
			"#yGame Configuration#n  (%d total entries)\r\n"
			"#y---------------------------------------------#n\r\n",
			CFG_COUNT );
		strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

		for ( i = 0; i < CFG_COUNT; i++ ) {
			const char *cat = cfg_category_prefix( cfg_table[i].key );
			if ( str_cmp( cat, last_cat ) ) {
				if ( last_cat[0] != '\0' ) {
					snprintf( buf, sizeof( buf ),
						"  #g%-20s#n  %d entries\r\n",
						last_cat, cat_count );
					strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
					total_cats++;
				}
				snprintf( last_cat, sizeof( last_cat ), "%s", cat );
				cat_count = 1;
			} else {
				cat_count++;
			}
		}
		if ( last_cat[0] != '\0' ) {
			snprintf( buf, sizeof( buf ),
				"  #g%-20s#n  %d entries\r\n",
				last_cat, cat_count );
			strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
			total_cats++;
		}

		snprintf( buf, sizeof( buf ),
			"#y---------------------------------------------#n\r\n"
			"%d categories.  Use #ycfg <prefix>#n for details.\r\n"
			"#ycfg <key> <value>#n to modify.  "
			"#ycfg reload#n to reload from DB.\r\n",
			total_cats );
		strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

		send_to_char( output, ch );
		return;
	}

	/* "cfg reload" - reload from DB */
	if ( !str_cmp( arg, "reload" ) ) {
		cfg_reset_all();
		db_game_load_cfg();
		send_to_char( "Configuration reloaded from database.\r\n", ch );
		return;
	}

	/* "cfg defaults" - reset all to defaults */
	if ( !str_cmp( arg, "defaults" ) ) {
		cfg_reset_all();
		db_game_save_cfg();
		send_to_char( "All config values reset to defaults.\r\n", ch );
		return;
	}

	/* "cfg reset <key>" - reset single key */
	if ( !str_cmp( arg, "reset" ) ) {
		if ( arg2[0] == '\0' ) {
			send_to_char( "Reset which key?  Usage: cfg reset <key>\r\n", ch );
			return;
		}

		key = cfg_key_from_string( arg2 );
		if ( key == CFG_COUNT ) {
			send_to_char( "Unknown config key.\r\n", ch );
			return;
		}

		cfg_reset( key );
		db_game_save_cfg();
		e = cfg_entry( key );
		snprintf( buf, sizeof( buf ),
			"Reset #g%s#n to default: #y%d#n\r\n",
			e->key, e->default_value );
		send_to_char( buf, ch );
		return;
	}

	/* "cfg <key> <value>" - set a value */
	if ( arg2[0] != '\0' ) {
		int val;

		key = cfg_key_from_string( arg );
		if ( key == CFG_COUNT ) {
			send_to_char( "Unknown config key.\r\n", ch );
			return;
		}

		val = atoi( arg2 );
		cfg_set( key, val );
		db_game_save_cfg();
		e = cfg_entry( key );
		snprintf( buf, sizeof( buf ),
			"Set #g%s#n = #y%d#n (default: %d)\r\n",
			e->key, e->value, e->default_value );
		send_to_char( buf, ch );
		return;
	}

	/* "cfg <prefix>" - show matching entries or subcategories */
	{
		int prefix_len = strlen( arg );
		char subcats[64][64];
		int subcat_counts[64];
		int num_subcats = 0;
		int total_matches = 0;

		/* First pass: count matches and collect unique subcategories */
		for ( i = 0; i < CFG_COUNT; i++ ) {
			if ( !str_prefix( arg, cfg_table[i].key ) ) {
				const char *rest;
				char subcat[64];
				const char *dot;
				int j, found;

				total_matches++;

				/* Extract the next subcategory after the prefix */
				rest = cfg_table[i].key + prefix_len;
				if ( *rest == '.' ) rest++;  /* Skip leading dot */

				dot = strchr( rest, '.' );
				if ( dot ) {
					size_t len = (size_t)( dot - rest );
					if ( len >= sizeof( subcat ) ) len = sizeof( subcat ) - 1;
					memcpy( subcat, rest, len );
					subcat[len] = '\0';
				} else {
					snprintf( subcat, sizeof( subcat ), "%s", rest );
				}

				/* Check if this subcategory is already tracked */
				found = 0;
				for ( j = 0; j < num_subcats; j++ ) {
					if ( !str_cmp( subcats[j], subcat ) ) {
						subcat_counts[j]++;
						found = 1;
						break;
					}
				}
				if ( !found && num_subcats < 64 ) {
					snprintf( subcats[num_subcats], sizeof( subcats[0] ), "%s", subcat );
					subcat_counts[num_subcats] = 1;
					num_subcats++;
				}
			}
		}

		if ( total_matches == 0 ) {
			send_to_char( "No matching config keys found.\r\n", ch );
			return;
		}

		output[0] = '\0';

		/* Check if any subcategory has multiple entries (meaning it's a real category) */
		{
			int has_real_subcats = 0;
			for ( i = 0; i < num_subcats; i++ ) {
				if ( subcat_counts[i] > 1 ) {
					has_real_subcats = 1;
					break;
				}
			}

			/* If too many matches and real subcategories exist, show subcategories */
			if ( total_matches > 30 && has_real_subcats ) {
			snprintf( buf, sizeof( buf ),
				"#ycfg %s#n  (%d entries)\r\n"
				"#y---------------------------------------------#n\r\n",
				arg, total_matches );
			strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );

			for ( i = 0; i < num_subcats; i++ ) {
				snprintf( buf, sizeof( buf ),
					"  #g%s.%-20s#n  %d entries\r\n",
					arg, subcats[i], subcat_counts[i] );
				strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
			}

			snprintf( buf, sizeof( buf ),
				"#y---------------------------------------------#n\r\n"
				"%d subcategories.  Use #ycfg %s.<subcat>#n for details.\r\n",
				num_subcats, arg );
			strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
		} else {
			/* Show actual entries */
			count = 0;
			for ( i = 0; i < CFG_COUNT; i++ ) {
				if ( !str_prefix( arg, cfg_table[i].key ) ) {
					const char *modified =
						( cfg_table[i].value != cfg_table[i].default_value )
						? " #r*#n" : "";

					snprintf( buf, sizeof( buf ),
						"  #g%-55s#n  #y%d#n  (default: %d)%s\r\n",
						cfg_table[i].key,
						cfg_table[i].value,
						cfg_table[i].default_value,
						modified );
					strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
					count++;
				}
			}

			snprintf( buf, sizeof( buf ),
				"#y--- %d matching entries ---#n\r\n", count );
			strncat( output, buf, sizeof( output ) - strlen( output ) - 1 );
		}
		}

		send_to_char( output, ch );
	}
}
