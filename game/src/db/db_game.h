/***************************************************************************
 *  db_game.h - SQLite persistence for global game data
 *
 *  Manages base_help.db + live_help.db (help entries) and game.db
 *  (config, leaderboards, kingdoms, notes, bugs, bans, disabled commands)
 *  stored in gamedata/db/game/.
 *
 *  base_help.db is deployed read-only with each release.
 *  live_help.db holds runtime additions/overrides (preserved on server).
 ***************************************************************************/

#ifndef DB_GAME_H
#define DB_GAME_H

#include "../core/merc.h"

/* Initialize game databases (creates game/ subdir, opens connections) */
void db_game_init( void );

/* Close game database connections */
void db_game_close( void );

/* Help entries (base_help.db + live_help.db) */
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

/* Board notes (game.db) */
void db_game_load_notes( int board_idx );
void db_game_save_board_notes( int board_idx );
void db_game_append_note( int board_idx, NOTE_DATA *note );

/* Bug reports (game.db) */
void db_game_append_bug( int room_vnum, const char *player, const char *message );

/* Bans (game.db) */
void db_game_load_bans( void );
void db_game_save_bans( void );

/* Disabled commands (game.db) */
void db_game_load_disabled( void );
void db_game_save_disabled( void );

/* Balance config (game.db) */
void db_game_load_balance( void );
void db_game_save_balance( void );

/* Ability config (game.db) */
void db_game_load_ability_config( void );
void db_game_save_ability_config( void );

/* Super admins (game.db) - character names with relevel access */
bool db_game_is_super_admin( const char *name );
void db_game_add_super_admin( const char *name );
void db_game_remove_super_admin( const char *name );
void db_game_list_super_admins( CHAR_DATA *ch );

#endif /* DB_GAME_H */
