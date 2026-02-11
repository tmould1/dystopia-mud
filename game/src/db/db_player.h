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

/* Delete a player's database file permanently */
bool db_player_delete( const char *name );

/* Background save support */
void db_player_wait_pending( void );
int  db_player_pending_count( void );

/* Character initialization for loading */
CHAR_DATA *init_char_for_load( DESCRIPTOR_DATA *d, char *name );

#endif /* DB_PLAYER_H */
