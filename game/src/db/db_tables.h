/***************************************************************************
 *  db_tables.h - SQLite persistence for game reference data tables
 *
 *  Manages tables.db stored in gamedata/db/game/:
 *    - socials: Social command message strings
 *    - slays: Immortal slay option messages
 *    - liquids: Liquid properties (name, color, effects)
 *    - wear_locations: Equipment slot display strings
 *    - calendar: In-game day and month names
 *
 *  Data is pre-populated in the shipped database via seed_tables_db.py;
 *  this file only loads and provides access to that data.  Follows the
 *  same read-only-at-boot, cache-in-memory, close pattern as db_class.
 ***************************************************************************/

#ifndef DB_TABLES_H
#define DB_TABLES_H

#include "../core/merc.h"

/*--------------------------------------------------------------------------
 * Script Library
 *--------------------------------------------------------------------------*/

#define MAX_SCRIPT_LIBRARY  64

typedef struct script_library_entry {
	char     *name;       /* Unique identifier (e.g. 'tick_cast_mage') */
	uint32_t  trigger;    /* Trigger type bitmask */
	char     *code;       /* Lua source code */
	char     *pattern;    /* SPEECH pattern (NULL = any) */
	int       chance;     /* Percent chance (0 = always) */
} SCRIPT_LIBRARY_ENTRY;

/*--------------------------------------------------------------------------
 * Cache Limits
 *--------------------------------------------------------------------------*/

#define MAX_CACHED_SOCIALS     128
#define MAX_CACHED_SLAYS        16
#define MAX_CACHED_WEAR_LOCS    MAX_WEAR
#define MAX_CACHED_DAYS         35
#define MAX_CACHED_MONTHS       35

/*--------------------------------------------------------------------------
 * Lifecycle
 *--------------------------------------------------------------------------*/

/* Open tables.db read-only.  Called from boot_db() after db_class_init(). */
void db_tables_init( void );

/* Close tables.db after all load functions have run. */
void db_tables_close( void );

/*--------------------------------------------------------------------------
 * Loaders - call once during boot, after db_tables_init()
 *--------------------------------------------------------------------------*/

void db_tables_load_script_library( void );
void db_tables_load_socials( void );
void db_tables_load_slays( void );
void db_tables_load_liquids( void );
void db_tables_load_wear_locations( void );
void db_tables_load_calendar( void );

/*--------------------------------------------------------------------------
 * Accessors
 *--------------------------------------------------------------------------*/

/* Script library - lookup shared Lua script templates by name */
const SCRIPT_LIBRARY_ENTRY *db_tables_get_script_library( const char *name );
int db_tables_get_script_library_count( void );
const SCRIPT_LIBRARY_ENTRY *db_tables_get_script_library_by_index( int idx );

/* Socials - returns count loaded; social_table[] is populated directly */
int db_tables_get_social_count( void );

/* Slays - returns count loaded; slay_table[] is populated directly */
int db_tables_get_slay_count( void );

/* Wear locations - returns display string for a WEAR_* slot */
const char *db_tables_get_wear_display( int slot );
const char *db_tables_get_wear_sr( int slot );

/* Calendar */
const char *db_tables_get_day_name( int day );
const char *db_tables_get_month_name( int month );
int db_tables_get_day_count( void );
int db_tables_get_month_count( void );

#endif /* DB_TABLES_H */
