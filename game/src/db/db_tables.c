/***************************************************************************
 *  db_tables.c - SQLite persistence for game reference data tables
 *
 *  Loads tables.db from gamedata/db/game/ at boot time, populates
 *  in-memory arrays, then closes the connection.  Follows the same
 *  pattern as db_class.c.
 *
 *  Tables managed:
 *    - socials       -> social_table[] in socials.c
 *    - slays         -> slay_table[] in const.c
 *    - liquids       -> liq_table[] in const.c
 *    - wear_locations -> where_name[]/where_name_sr[] in act_info.c
 *    - calendar      -> day_name[]/month_name[] in act_info.c
 ***************************************************************************/

#include "db_util.h"
#include "db_tables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External: game database directory set by db_game_init() */
extern char mud_db_game_dir[];

/* Database connection (open only during boot) */
static sqlite3 *tables_db = NULL;

/*--------------------------------------------------------------------------
 * External arrays populated by loaders
 *--------------------------------------------------------------------------*/

extern struct social_type social_table[];
extern int social_count;

extern struct slay_type slay_table[];
extern int slay_count;

extern struct liq_type liq_table[];

extern char *where_name[];
extern char *where_name_sr[];

extern char *day_name[];
extern char *month_name[];

/*--------------------------------------------------------------------------
 * Internal tracking
 *--------------------------------------------------------------------------*/

static int cached_social_count = 0;
static int cached_slay_count = 0;
static int day_count = 0;
static int month_count = 0;


/*--------------------------------------------------------------------------
 * Lifecycle
 *--------------------------------------------------------------------------*/

void db_tables_init( void ) {
	char path[MUD_PATH_MAX];

	if ( snprintf( path, sizeof( path ), "%s%stables.db",
			mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
		bug( "db_tables_init: tables.db path truncated.", 0 );
	}
	if ( sqlite3_open_v2( path, &tables_db, SQLITE_OPEN_READONLY, NULL ) != SQLITE_OK ) {
		char errbuf[MUD_PATH_MAX + 64];
		snprintf( errbuf, sizeof( errbuf ),
			"db_tables_init: FATAL - tables.db not found at %s", path );
		bug( errbuf, 0 );
		bug( "  Run: python game/tools/seed_tables_db.py to generate it.", 0 );
		tables_db = NULL;
		return;
	}
	log_string( "  db_tables_init: tables.db opened for boot loading." );
}

void db_tables_close( void ) {
	if ( tables_db ) {
		sqlite3_close( tables_db );
		tables_db = NULL;
	}
}


/***************************************************************************
 * Socials
 ***************************************************************************/

void db_tables_load_socials( void ) {
	sqlite3_stmt *stmt;
	int i;

	if ( !tables_db )
		return;

	if ( sqlite3_prepare_v2( tables_db,
			"SELECT name, char_no_arg, others_no_arg, char_found, "
			"others_found, vict_found, char_auto, others_auto "
			"FROM socials ORDER BY name",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		char errbuf[256];
		snprintf( errbuf, sizeof( errbuf ),
			"db_tables_load_socials: prepare failed: %s",
			sqlite3_errmsg( tables_db ) );
		bug( errbuf, 0 );
		return;
	}

	i = 0;
	while ( sqlite3_step( stmt ) == SQLITE_ROW && i < MAX_CACHED_SOCIALS - 1 ) {
		const char *val;

		social_table[i].name        = str_dup( col_text( stmt, 0 ) );
		val = (const char *)sqlite3_column_text( stmt, 1 );
		social_table[i].char_no_arg = val ? str_dup( val ) : NULL;
		val = (const char *)sqlite3_column_text( stmt, 2 );
		social_table[i].others_no_arg = val ? str_dup( val ) : NULL;
		val = (const char *)sqlite3_column_text( stmt, 3 );
		social_table[i].char_found  = val ? str_dup( val ) : NULL;
		val = (const char *)sqlite3_column_text( stmt, 4 );
		social_table[i].others_found = val ? str_dup( val ) : NULL;
		val = (const char *)sqlite3_column_text( stmt, 5 );
		social_table[i].vict_found  = val ? str_dup( val ) : NULL;
		val = (const char *)sqlite3_column_text( stmt, 6 );
		social_table[i].char_auto   = val ? str_dup( val ) : NULL;
		val = (const char *)sqlite3_column_text( stmt, 7 );
		social_table[i].others_auto = val ? str_dup( val ) : NULL;
		i++;
	}

	/* Sentinel entry - check_social() relies on empty name to stop iteration */
	social_table[i].name        = str_dup( "" );
	social_table[i].char_no_arg = NULL;
	social_table[i].others_no_arg = NULL;
	social_table[i].char_found  = NULL;
	social_table[i].others_found = NULL;
	social_table[i].vict_found  = NULL;
	social_table[i].char_auto   = NULL;
	social_table[i].others_auto = NULL;

	cached_social_count = i;
	social_count = i;

	sqlite3_finalize( stmt );

	{
		char buf[80];
		snprintf( buf, sizeof( buf ), "  Loaded %d socials from tables.db.", i );
		log_string( buf );
	}
}

int db_tables_get_social_count( void ) {
	return cached_social_count;
}


/***************************************************************************
 * Slays
 ***************************************************************************/

void db_tables_load_slays( void ) {
	sqlite3_stmt *stmt;
	int i;

	if ( !tables_db )
		return;

	if ( sqlite3_prepare_v2( tables_db,
			"SELECT owner, title, char_msg, vict_msg, room_msg "
			"FROM slays ORDER BY id",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		char errbuf[256];
		snprintf( errbuf, sizeof( errbuf ),
			"db_tables_load_slays: prepare failed: %s",
			sqlite3_errmsg( tables_db ) );
		bug( errbuf, 0 );
		return;
	}

	i = 0;
	while ( sqlite3_step( stmt ) == SQLITE_ROW && i < MAX_SLAY_TYPES ) {
		const char *val;

		val = (const char *)sqlite3_column_text( stmt, 0 );
		slay_table[i].owner    = val ? str_dup( val ) : str_dup( "" );
		slay_table[i].title    = str_dup( col_text( stmt, 1 ) );
		slay_table[i].char_msg = str_dup( col_text( stmt, 2 ) );
		slay_table[i].vict_msg = str_dup( col_text( stmt, 3 ) );
		slay_table[i].room_msg = str_dup( col_text( stmt, 4 ) );
		i++;
	}

	cached_slay_count = i;
	slay_count = i;

	sqlite3_finalize( stmt );

	{
		char buf[80];
		snprintf( buf, sizeof( buf ), "  Loaded %d slay types from tables.db.", i );
		log_string( buf );
	}
}

int db_tables_get_slay_count( void ) {
	return cached_slay_count;
}


/***************************************************************************
 * Liquids
 ***************************************************************************/

void db_tables_load_liquids( void ) {
	sqlite3_stmt *stmt;
	int i;

	if ( !tables_db )
		return;

	if ( sqlite3_prepare_v2( tables_db,
			"SELECT id, name, color, proof, full_effect, thirst "
			"FROM liquids ORDER BY id",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		char errbuf[256];
		snprintf( errbuf, sizeof( errbuf ),
			"db_tables_load_liquids: prepare failed: %s",
			sqlite3_errmsg( tables_db ) );
		bug( errbuf, 0 );
		return;
	}

	i = 0;
	while ( sqlite3_step( stmt ) == SQLITE_ROW && i < LIQ_MAX ) {
		int id = sqlite3_column_int( stmt, 0 );
		if ( id < 0 || id >= LIQ_MAX )
			continue;

		liq_table[id].liq_name     = str_dup( col_text( stmt, 1 ) );
		liq_table[id].liq_color    = str_dup( col_text( stmt, 2 ) );
		liq_table[id].liq_affect[0] = (sh_int)sqlite3_column_int( stmt, 3 );
		liq_table[id].liq_affect[1] = (sh_int)sqlite3_column_int( stmt, 4 );
		liq_table[id].liq_affect[2] = (sh_int)sqlite3_column_int( stmt, 5 );
		i++;
	}

	sqlite3_finalize( stmt );

	{
		char buf[80];
		snprintf( buf, sizeof( buf ), "  Loaded %d liquids from tables.db.", i );
		log_string( buf );
	}
}


/***************************************************************************
 * Wear Locations - populates where_name[] and where_name_sr[] directly
 ***************************************************************************/

void db_tables_load_wear_locations( void ) {
	sqlite3_stmt *stmt;
	int loaded = 0;

	if ( !tables_db )
		return;

	if ( sqlite3_prepare_v2( tables_db,
			"SELECT slot_id, display_text, sr_text "
			"FROM wear_locations ORDER BY slot_id",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		char errbuf[256];
		snprintf( errbuf, sizeof( errbuf ),
			"db_tables_load_wear_locations: prepare failed: %s",
			sqlite3_errmsg( tables_db ) );
		bug( errbuf, 0 );
		return;
	}

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		int slot = sqlite3_column_int( stmt, 0 );
		if ( slot < 0 || slot >= MAX_WEAR )
			continue;

		where_name[slot]    = str_dup( col_text( stmt, 1 ) );
		where_name_sr[slot] = str_dup( col_text( stmt, 2 ) );
		loaded++;
	}

	sqlite3_finalize( stmt );

	{
		char buf[80];
		snprintf( buf, sizeof( buf ), "  Loaded %d wear locations from tables.db.", loaded );
		log_string( buf );
	}
}

const char *db_tables_get_wear_display( int slot ) {
	if ( slot >= 0 && slot < MAX_WEAR && where_name[slot] )
		return where_name[slot];
	return "";
}

const char *db_tables_get_wear_sr( int slot ) {
	if ( slot >= 0 && slot < MAX_WEAR && where_name_sr[slot] )
		return where_name_sr[slot];
	return "";
}


/***************************************************************************
 * Calendar - populates day_name[] and month_name[] directly
 ***************************************************************************/

void db_tables_load_calendar( void ) {
	sqlite3_stmt *stmt;

	if ( !tables_db )
		return;

	/* Load day names */
	if ( sqlite3_prepare_v2( tables_db,
			"SELECT idx, name FROM calendar WHERE type = 'day' ORDER BY idx",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		day_count = 0;
		while ( sqlite3_step( stmt ) == SQLITE_ROW && day_count < MAX_CACHED_DAYS ) {
			day_name[day_count] = str_dup( col_text( stmt, 1 ) );
			day_count++;
		}
		sqlite3_finalize( stmt );
	}

	/* Load month names */
	if ( sqlite3_prepare_v2( tables_db,
			"SELECT idx, name FROM calendar WHERE type = 'month' ORDER BY idx",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		month_count = 0;
		while ( sqlite3_step( stmt ) == SQLITE_ROW && month_count < MAX_CACHED_MONTHS ) {
			month_name[month_count] = str_dup( col_text( stmt, 1 ) );
			month_count++;
		}
		sqlite3_finalize( stmt );
	}

	{
		char buf[80];
		snprintf( buf, sizeof( buf ), "  Loaded %d days, %d months from tables.db.",
			day_count, month_count );
		log_string( buf );
	}
}

const char *db_tables_get_day_name( int day ) {
	if ( day >= 0 && day < day_count && day_name[day] )
		return day_name[day];
	return "Unknown";
}

const char *db_tables_get_month_name( int month ) {
	if ( month >= 0 && month < month_count && month_name[month] )
		return month_name[month];
	return "Unknown";
}

int db_tables_get_day_count( void ) {
	return day_count;
}

int db_tables_get_month_count( void ) {
	return month_count;
}
