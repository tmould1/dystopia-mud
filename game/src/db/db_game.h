/***************************************************************************
 *  db_game.h - SQLite persistence for global game data
 *
 *  Manages multiple databases stored in gamedata/db/game/:
 *    - base_help.db: read-only help entries (deployed with release)
 *    - live_help.db: runtime help additions/overrides (preserved on server)
 *    - game.db: config, leaderboards, kingdoms, notes, bugs, bans, etc.
 *
 *  Class configuration is in db_class.h/db_class.c (class.db).
 ***************************************************************************/

#ifndef DB_GAME_H
#define DB_GAME_H

#include "../core/merc.h"
#include "db_class.h"

/* Initialize game databases (creates game/ subdir, opens connections) */
void db_game_init( void );

/* Close game database connections after boot (data cached in memory) */
void db_game_close_boot_connections( void );

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

/* Unified config (game.db) */
void db_game_load_cfg( void );
void db_game_save_cfg( void );

/* Super admins (game.db) - character names with relevel access */
bool db_game_is_super_admin( const char *name );
void db_game_add_super_admin( const char *name );
void db_game_remove_super_admin( const char *name );
void db_game_list_super_admins( CHAR_DATA *ch );

/* Immortal pretitles (game.db) - custom who list pretitles */
void db_game_load_pretitles( void );
const char *db_game_get_pretitle( const char *name );
void db_game_set_pretitle( const char *name, const char *pretitle, const char *set_by );
void db_game_delete_pretitle( const char *name );
void db_game_list_pretitles( CHAR_DATA *ch );

/* Name validation tables (game.db) */
#define NAMETYPE_RESERVED   0  /* Reserved word (exact match block) */
#define NAMETYPE_PROTECTED  1  /* Protected prefix (contains-block, exact allowed) */
#define NAMETYPE_BLOCKED    2  /* Exact name block */

typedef struct forbidden_name {
	struct forbidden_name *next;
	char *name;
	int   type;      /* NAMETYPE_RESERVED, NAMETYPE_PROTECTED, NAMETYPE_BLOCKED */
	char *added_by;
} FORBIDDEN_NAME;

typedef struct profanity_filter {
	struct profanity_filter *next;
	char *pattern;
	char *added_by;
} PROFANITY_FILTER;

extern FORBIDDEN_NAME  *forbidden_name_list;
extern PROFANITY_FILTER *profanity_filter_list;

void db_game_load_forbidden_names( void );
void db_game_save_forbidden_names( void );
void db_game_load_profanity_filters( void );
void db_game_save_profanity_filters( void );
void db_game_load_confusables( void );

/* Audio config (game.db) - MCMP audio file mappings */
typedef struct audio_entry {
	char   *category;      /* "ambient", "footstep", "combat", etc. */
	char   *trigger_key;   /* "SECT_FOREST", "combat_miss", etc. */
	char   *filename;      /* "ambient/forest.mp3" */
	int    volume;         /* 1-100, default 50 */
	int    priority;       /* 1-100, default 50 */
	int    loops;          /* 1 = once, -1 = infinite */
	char   *media_type;    /* "sound" or "music" */
	char   *tag;           /* "combat", "environment", etc. */
	char   *caption;       /* accessibility caption */
	char   *use_key;       /* binding key (e.g., "ambient", "weather") */
	bool   use_continue;   /* TRUE = don't restart if same file playing */
} AUDIO_ENTRY;

void db_game_load_audio_config( void );
AUDIO_ENTRY *audio_config_find( const char *category, const char *trigger_key );

/* Sector-indexed audio lookup arrays (for fast ambient/footstep access) */
extern AUDIO_ENTRY *audio_ambient[SECT_MAX];
extern AUDIO_ENTRY *audio_footstep[SECT_MAX];

#endif /* DB_GAME_H */
