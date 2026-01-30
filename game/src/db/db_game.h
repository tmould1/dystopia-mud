/***************************************************************************
 *  db_game.h - SQLite persistence for global game data
 *
 *  Manages help.db (help entries) and game.db (config, leaderboards,
 *  kingdoms) stored in gamedata/db/game/.
 ***************************************************************************/

#ifndef DB_GAME_H
#define DB_GAME_H

#include "../core/merc.h"

/* Initialize game databases (creates game/ subdir, opens connections) */
void db_game_init( void );

/* Close game database connections */
void db_game_close( void );

/* Help entries (help.db) */
void db_game_load_helps( void );
void db_game_save_helps( void );
bool db_game_reload_help( const char *keyword );

/* Game config (game.db) */
void db_game_load_gameconfig( void );
void db_game_save_gameconfig( void );

/* Topboard (game.db) */
void db_game_load_topboard( void );
void db_game_save_topboard( void );

/* Leaderboard (game.db) */
void db_game_load_leaderboard( void );
void db_game_save_leaderboard( void );

/* Kingdoms (game.db) */
void db_game_load_kingdoms( void );
void db_game_save_kingdoms( void );

#endif /* DB_GAME_H */
