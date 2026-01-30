/***************************************************************************
 *  db_sql.h - SQLite persistence layer for area data
 *
 *  Provides functions to load and save area data (mobiles, objects, rooms,
 *  resets, shops, specials) from/to per-area SQLite database files stored
 *  in gamedata/db/areas/.
 ***************************************************************************/

#ifndef DB_SQL_H
#define DB_SQL_H

#include "../core/merc.h"

/* Directory path for .db files (set during init) */
extern char mud_db_dir[MUD_PATH_MAX];

/* Initialize the db directory path */
void db_sql_init( void );

/* Scan gamedata/db/areas/ for .db files, returns sorted list of area filenames.
 * Filenames use .are extension (e.g. "midgaard.are") for compatibility.
 * Returns count of files found, or -1 on error.
 * Caller must free with db_sql_free_scan(). */
int db_sql_scan_areas( char ***filenames_out );
void db_sql_free_scan( char **filenames, int count );

/* Check if a .db file exists for the given area filename (e.g. "midgaard.are") */
bool db_sql_area_exists( const char *area_filename );

/* Load one area from its .db file into the in-memory hash tables.
 * Phase 1: loads area metadata, mobiles, objects, rooms.
 * Call db_sql_link_area() after ALL areas are loaded to resolve
 * cross-area vnum references in resets, shops, and specials. */
void db_sql_load_area( const char *area_filename );

/* Link phase: load resets, shops, and specials for one area.
 * Must be called after all areas have been loaded. */
void db_sql_link_area( const char *area_filename );

/* Save one area to its .db file (creates/overwrites) */
void db_sql_save_area( AREA_DATA *pArea );

#endif /* DB_SQL_H */
