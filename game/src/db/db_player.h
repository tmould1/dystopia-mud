/***************************************************************************
 *  db_player.h - SQLite persistence for player character data
 *
 *  Per-player .db files stored in gamedata/db/players/.
 ***************************************************************************/

#ifndef DB_PLAYER_H
#define DB_PLAYER_H

#include "../core/merc.h"

/* Initialize players/ subdirectory under gamedata/db/ */
void db_player_init( void );

/* Save full character + inventory to <Name>.db */
void db_player_save( CHAR_DATA *ch );

/* Load full character + inventory from <Name>.db (or migrate from text) */
bool db_player_load( DESCRIPTOR_DATA *d, char *name );

/* Load character only (no objects) for finger/short lookups */
bool db_player_load_short( DESCRIPTOR_DATA *d, char *name );

/* Check if a player .db file exists */
bool db_player_exists( const char *name );

#endif /* DB_PLAYER_H */
