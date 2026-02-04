/***************************************************************************
 *  db_game.c - SQLite persistence for global game data
 *
 *  Manages base_help.db + live_help.db (help entries) and game.db
 *  (config, leaderboards, kingdoms) stored in gamedata/db/game/.
 ***************************************************************************/

#include "db_util.h"
#include "db_game.h"
#include "../core/ability_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External globals */
extern char mud_db_dir[MUD_PATH_MAX];
extern HELP_DATA *first_help;
extern HELP_DATA *last_help;
extern char *help_greeting;
extern int top_help;
extern TOP_BOARD top_board[MAX_TOP_PLAYERS + 1];
extern LEADER_BOARD leader_board;
extern KINGDOM_DATA kingdom_table[MAX_KINGDOM + 1];
extern GAMECONFIG_DATA game_config;
extern BALANCE_DATA balance;
extern BOARD_DATA boards[];
extern BAN_DATA *ban_list;
extern DISABLED_DATA *disabled_first;
extern const struct cmd_type cmd_table[];
extern time_t current_time;

/* Cached database connections */
static sqlite3 *base_help_db = NULL;
static sqlite3 *live_help_db = NULL;
static sqlite3 *game_db = NULL;

/* Directory for game databases */
static char mud_db_game_dir[MUD_PATH_MAX] = "";

/* In-memory cache for immortal pretitles */
typedef struct pretitle_entry {
	char   *immortal_name;
	char   *pretitle;
	char   *set_by;
	time_t  set_date;
} PRETITLE_ENTRY;

static PRETITLE_ENTRY *pretitle_entries = NULL;
static int pretitle_count = 0;

/* Schema for help.db */
static const char *HELP_SCHEMA_SQL =
	"CREATE TABLE IF NOT EXISTS helps ("
	"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  level      INTEGER NOT NULL DEFAULT 0,"
	"  keyword    TEXT NOT NULL,"
	"  text       TEXT NOT NULL DEFAULT ''"
	");";

/* Schema for game.db */
static const char *GAME_SCHEMA_SQL =
	"CREATE TABLE IF NOT EXISTS gameconfig ("
	"  key        TEXT PRIMARY KEY,"
	"  value      TEXT NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS topboard ("
	"  rank       INTEGER PRIMARY KEY,"
	"  name       TEXT NOT NULL DEFAULT 'Empty',"
	"  pkscore    INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS leaderboard ("
	"  category   TEXT PRIMARY KEY,"
	"  name       TEXT NOT NULL DEFAULT 'Nobody',"
	"  value      INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS kingdoms ("
	"  id         INTEGER PRIMARY KEY,"
	"  name       TEXT NOT NULL DEFAULT 'None',"
	"  whoname    TEXT NOT NULL DEFAULT 'None',"
	"  leader     TEXT NOT NULL DEFAULT 'None',"
	"  general    TEXT NOT NULL DEFAULT 'None',"
	"  kills      INTEGER NOT NULL DEFAULT 0,"
	"  deaths     INTEGER NOT NULL DEFAULT 0,"
	"  qps        INTEGER NOT NULL DEFAULT 0,"
	"  req_hit    INTEGER NOT NULL DEFAULT 0,"
	"  req_move   INTEGER NOT NULL DEFAULT 0,"
	"  req_mana   INTEGER NOT NULL DEFAULT 0,"
	"  req_qps    INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS notes ("
	"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  board_idx  INTEGER NOT NULL,"
	"  sender     TEXT NOT NULL,"
	"  date       TEXT NOT NULL,"
	"  date_stamp INTEGER NOT NULL,"
	"  expire     INTEGER NOT NULL,"
	"  to_list    TEXT NOT NULL,"
	"  subject    TEXT NOT NULL,"
	"  text       TEXT NOT NULL DEFAULT ''"
	");"

	"CREATE TABLE IF NOT EXISTS bugs ("
	"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  room_vnum  INTEGER NOT NULL DEFAULT 0,"
	"  player     TEXT NOT NULL DEFAULT '',"
	"  message    TEXT NOT NULL,"
	"  timestamp  INTEGER NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS bans ("
	"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  name       TEXT NOT NULL,"
	"  reason     TEXT NOT NULL DEFAULT ''"
	");"

	"CREATE TABLE IF NOT EXISTS disabled_commands ("
	"  command_name TEXT PRIMARY KEY,"
	"  level        INTEGER NOT NULL,"
	"  disabled_by  TEXT NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS balance_config ("
	"  key   TEXT PRIMARY KEY,"
	"  value TEXT NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS ability_config ("
	"  key   TEXT PRIMARY KEY,"
	"  value INTEGER NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS super_admins ("
	"  name  TEXT PRIMARY KEY COLLATE NOCASE"
	");"

	"CREATE TABLE IF NOT EXISTS audio_config ("
	"  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  category     TEXT NOT NULL,"
	"  trigger_key  TEXT NOT NULL,"
	"  filename     TEXT NOT NULL,"
	"  volume       INTEGER NOT NULL DEFAULT 50,"
	"  priority     INTEGER NOT NULL DEFAULT 50,"
	"  loops        INTEGER NOT NULL DEFAULT 1,"
	"  media_type   TEXT NOT NULL DEFAULT 'sound',"
	"  tag          TEXT NOT NULL DEFAULT '',"
	"  caption      TEXT NOT NULL DEFAULT '',"
	"  use_key      TEXT DEFAULT NULL,"
	"  use_continue INTEGER NOT NULL DEFAULT 0,"
	"  UNIQUE(category, trigger_key)"
	");"
	"CREATE INDEX IF NOT EXISTS idx_audio_category ON audio_config(category);"

	"CREATE TABLE IF NOT EXISTS immortal_pretitles ("
	"  immortal_name TEXT PRIMARY KEY COLLATE NOCASE,"
	"  pretitle      TEXT NOT NULL,"
	"  set_by        TEXT NOT NULL,"
	"  set_date      INTEGER NOT NULL"
	");";


/*
 * Initialize game databases. Called early in boot_db().
 */
void db_game_init( void ) {
	char path[MUD_PATH_MAX];
	char *errmsg = NULL;

	/* Create game/ subdirectory */
	if ( snprintf( mud_db_game_dir, sizeof( mud_db_game_dir ), "%s%sgame",
			mud_db_dir, PATH_SEPARATOR ) >= (int)sizeof( mud_db_game_dir ) ) {
		bug( "db_game_init: path truncated.", 0 );
	}
	ensure_directory( mud_db_game_dir );

	/* Open base_help.db (read-only, deployed with each release) */
	if ( snprintf( path, sizeof( path ), "%s%sbase_help.db",
			mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
		bug( "db_game_init: base_help.db path truncated.", 0 );
	}
	if ( sqlite3_open_v2( path, &base_help_db, SQLITE_OPEN_READONLY, NULL ) != SQLITE_OK ) {
		bug( "db_game_init: cannot open base_help.db", 0 );
		base_help_db = NULL;
	}

	/* Open live_help.db (read-write, preserved on server for runtime changes) */
	if ( snprintf( path, sizeof( path ), "%s%slive_help.db",
			mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
		bug( "db_game_init: live_help.db path truncated.", 0 );
	}
	if ( sqlite3_open( path, &live_help_db ) != SQLITE_OK ) {
		bug( "db_game_init: cannot open live_help.db", 0 );
		live_help_db = NULL;
	} else {
		if ( sqlite3_exec( live_help_db, HELP_SCHEMA_SQL, NULL, NULL, &errmsg ) != SQLITE_OK ) {
			bug( "db_game_init: live_help schema error", 0 );
			if ( errmsg ) sqlite3_free( errmsg );
			errmsg = NULL;
		}
	}

	/* Open game.db */
	if ( snprintf( path, sizeof( path ), "%s%sgame.db",
			mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
		bug( "db_game_init: game.db path truncated.", 0 );
	}
	if ( sqlite3_open( path, &game_db ) != SQLITE_OK ) {
		bug( "db_game_init: cannot open game.db", 0 );
		game_db = NULL;
	} else {
		if ( sqlite3_exec( game_db, GAME_SCHEMA_SQL, NULL, NULL, &errmsg ) != SQLITE_OK ) {
			bug( "db_game_init: game schema error", 0 );
			if ( errmsg ) sqlite3_free( errmsg );
			errmsg = NULL;
		}
	}
}


/*
 * Close database connections after boot loading is complete.
 * All data has been cached in memory; connections are no longer needed.
 * Called from db.c after all db_game_load_* functions have run.
 */
void db_game_close_boot_connections( void ) {
	if ( base_help_db ) {
		sqlite3_close( base_help_db );
		base_help_db = NULL;
	}
	if ( live_help_db ) {
		sqlite3_close( live_help_db );
		live_help_db = NULL;
	}
	if ( game_db ) {
		sqlite3_close( game_db );
		game_db = NULL;
	}
	log_string( "  Game database connections closed (data cached in memory)." );
}


/*
 * Helper: Open game.db for a write or query operation.
 * Returns NULL on failure.
 */
static sqlite3 *db_game_open_for_write( void ) {
	char path[MUD_PATH_MAX];
	sqlite3 *db = NULL;

	if ( mud_db_game_dir[0] == '\0' )
		return NULL;

	if ( snprintf( path, sizeof( path ), "%s%sgame.db",
			mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
		bug( "db_game_open_for_write: path truncated.", 0 );
		return NULL;
	}

	if ( sqlite3_open( path, &db ) != SQLITE_OK ) {
		bug( "db_game_open_for_write: cannot open game.db", 0 );
		if ( db ) sqlite3_close( db );
		return NULL;
	}

	return db;
}

/*
 * Helper: Open live_help.db for a write operation.
 * Returns NULL on failure.
 */
static sqlite3 *db_game_open_live_help_for_write( void ) {
	char path[MUD_PATH_MAX];
	sqlite3 *db = NULL;

	if ( mud_db_game_dir[0] == '\0' )
		return NULL;

	if ( snprintf( path, sizeof( path ), "%s%slive_help.db",
			mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
		bug( "db_game_open_live_help_for_write: path truncated.", 0 );
		return NULL;
	}

	if ( sqlite3_open( path, &db ) != SQLITE_OK ) {
		bug( "db_game_open_live_help_for_write: cannot open live_help.db", 0 );
		if ( db ) sqlite3_close( db );
		return NULL;
	}

	return db;
}

/*
 * Helper: Open help databases for a read operation (used by db_game_reload_help).
 * Opens both base_help.db (read-only) and live_help.db (read-write).
 */
static void db_game_open_help_dbs_for_read( sqlite3 **base_out, sqlite3 **live_out ) {
	char path[MUD_PATH_MAX];

	*base_out = NULL;
	*live_out = NULL;

	if ( mud_db_game_dir[0] == '\0' )
		return;

	/* Open base_help.db read-only */
	if ( snprintf( path, sizeof( path ), "%s%sbase_help.db",
			mud_db_game_dir, PATH_SEPARATOR ) < (int)sizeof( path ) ) {
		sqlite3_open_v2( path, base_out, SQLITE_OPEN_READONLY, NULL );
	}

	/* Open live_help.db */
	if ( snprintf( path, sizeof( path ), "%s%slive_help.db",
			mud_db_game_dir, PATH_SEPARATOR ) < (int)sizeof( path ) ) {
		sqlite3_open( path, live_out );
	}
}


/*
 * Load help entries from a single SQLite database into in-memory lists.
 * Returns the number of entries loaded.
 */
static int load_helps_from_db( sqlite3 *db ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT level, keyword, text FROM helps ORDER BY id";
	int count = 0;

	if ( !db )
		return 0;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return 0;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		HELP_DATA *pHelp;

		CREATE( pHelp, HELP_DATA, 1 );
		pHelp->level   = (sh_int)sqlite3_column_int( stmt, 0 );
		pHelp->keyword = str_dup( col_text( stmt, 1 ) );
		pHelp->text    = str_dup( col_text( stmt, 2 ) );
		pHelp->area    = NULL;

		if ( pHelp->keyword[0] == '\0' ) {
			STRFREE( pHelp->text );
			STRFREE( pHelp->keyword );
			DISPOSE( pHelp );
			continue;
		}

		if ( !str_cmp( pHelp->keyword, "greeting" ) )
			help_greeting = pHelp->text;

		add_help( pHelp );
		count++;
	}

	sqlite3_finalize( stmt );
	return count;
}

/*
 * Load all help entries from base_help.db and live_help.db.
 * Base helps are loaded first, then live helps override (via add_help).
 */
void db_game_load_helps( void ) {
	char buf[256];
	int base_count, live_count;

	log_string( "  Loading base help entries..." );
	base_count = load_helps_from_db( base_help_db );
	log_string( "  Loading live help entries..." );
	live_count = load_helps_from_db( live_help_db );

	snprintf( buf, sizeof( buf ),
		"  Loaded %d base + %d live help entries", base_count, live_count );
	log_string( buf );
}


/*
 * Save all help entries to live_help.db.
 * Runtime changes (hedit, hset) are persisted to the live database.
 * base_help.db is never modified at runtime.
 */
void db_game_save_helps( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT INTO helps (level, keyword, text) VALUES (?,?,?)";
	HELP_DATA *pHelp;

	db = db_game_open_live_help_for_write();
	if ( !db )
		return;

	db_begin( db );
	sqlite3_exec( db, "DELETE FROM helps", NULL, NULL, NULL );

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		db_rollback( db );
		sqlite3_close( db );
		return;
	}

	for ( pHelp = first_help; pHelp; pHelp = pHelp->next ) {
		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, pHelp->level );
		sqlite3_bind_text( stmt, 2, pHelp->keyword, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 3, pHelp->text, -1, SQLITE_TRANSIENT );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Reload a single help entry by keyword.
 * Checks live_help.db first, then falls back to base_help.db.
 * Returns TRUE if the entry was found and loaded.
 */
bool db_game_reload_help( const char *keyword ) {
	sqlite3 *base_db, *live_db;
	sqlite3_stmt *stmt;
	const char *sql = "SELECT level, keyword, text FROM helps WHERE keyword=?";
	sqlite3 *dbs[2];
	int i;
	bool found = FALSE;

	db_game_open_help_dbs_for_read( &base_db, &live_db );
	dbs[0] = live_db;   /* Check live first */
	dbs[1] = base_db;

	for ( i = 0; i < 2 && !found; i++ ) {
		if ( !dbs[i] )
			continue;

		if ( sqlite3_prepare_v2( dbs[i], sql, -1, &stmt, NULL ) != SQLITE_OK )
			continue;

		sqlite3_bind_text( stmt, 1, keyword, -1, SQLITE_TRANSIENT );

		if ( sqlite3_step( stmt ) == SQLITE_ROW ) {
			HELP_DATA *pHelp;

			CREATE( pHelp, HELP_DATA, 1 );
			pHelp->level   = (sh_int)sqlite3_column_int( stmt, 0 );
			pHelp->keyword = str_dup( col_text( stmt, 1 ) );
			pHelp->text    = str_dup( col_text( stmt, 2 ) );
			pHelp->area    = NULL;

			if ( !str_cmp( pHelp->keyword, "greeting" ) )
				help_greeting = pHelp->text;

			add_help( pHelp );
			found = TRUE;
		}

		sqlite3_finalize( stmt );
	}

	if ( base_db ) sqlite3_close( base_db );
	if ( live_db ) sqlite3_close( live_db );

	return found;
}


/*
 * Game config (game.db)
 * Key-value pairs in the gameconfig table.
 */
void db_game_load_gameconfig( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT key, value FROM gameconfig";

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *key = col_text( stmt, 0 );
		const char *val = col_text( stmt, 1 );

		if ( !str_cmp( key, "base_xp" ) ) {
			game_config.base_xp = atoi( val );
		} else if ( !str_cmp( key, "max_xp_per_kill" ) ) {
			game_config.max_xp_per_kill = atoi( val );
		} else if ( !str_cmp( key, "game_name" ) ) {
			free_string( game_config.game_name );
			game_config.game_name = str_dup( val );
		} else if ( !str_cmp( key, "gui_url" ) ) {
			free_string( game_config.gui_url );
			game_config.gui_url = str_dup( val );
		} else if ( !str_cmp( key, "gui_version" ) ) {
			free_string( game_config.gui_version );
			game_config.gui_version = str_dup( val );
		} else if ( !str_cmp( key, "banner_left" ) ) {
			free_string( game_config.banner_left );
			game_config.banner_left = str_dup( val );
		} else if ( !str_cmp( key, "banner_right" ) ) {
			free_string( game_config.banner_right );
			game_config.banner_right = str_dup( val );
		} else if ( !str_cmp( key, "banner_fill" ) ) {
			free_string( game_config.banner_fill );
			game_config.banner_fill = str_dup( val );
		} else if ( !str_cmp( key, "audio_url" ) ) {
			free_string( game_config.audio_url );
			game_config.audio_url = str_dup( val );
		}
	}

	sqlite3_finalize( stmt );
}

void db_game_save_gameconfig( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO gameconfig (key, value) VALUES (?,?)";
	char buf[64];

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	db_begin( db );

#define SAVE_INT( k, v ) \
	sqlite3_reset( stmt ); \
	snprintf( buf, sizeof( buf ), "%d", v ); \
	sqlite3_bind_text( stmt, 1, k, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( stmt, 2, buf, -1, SQLITE_TRANSIENT ); \
	sqlite3_step( stmt )

#define SAVE_STR( k, v ) \
	sqlite3_reset( stmt ); \
	sqlite3_bind_text( stmt, 1, k, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( stmt, 2, v ? v : "", -1, SQLITE_TRANSIENT ); \
	sqlite3_step( stmt )

	SAVE_INT( "base_xp", game_config.base_xp );
	SAVE_INT( "max_xp_per_kill", game_config.max_xp_per_kill );
	SAVE_STR( "game_name", game_config.game_name );
	SAVE_STR( "gui_url", game_config.gui_url );
	SAVE_STR( "gui_version", game_config.gui_version );
	SAVE_STR( "banner_left", game_config.banner_left );
	SAVE_STR( "banner_right", game_config.banner_right );
	SAVE_STR( "banner_fill", game_config.banner_fill );
	SAVE_STR( "audio_url", game_config.audio_url );

#undef SAVE_INT
#undef SAVE_STR

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Topboard (game.db)
 * Ranked list of top PK players (1..MAX_TOP_PLAYERS).
 */
void db_game_load_topboard( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT rank, name, pkscore FROM topboard ORDER BY rank";

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		int rank = sqlite3_column_int( stmt, 0 );
		if ( rank < 1 || rank > MAX_TOP_PLAYERS )
			continue;
		free_string( top_board[rank].name );
		top_board[rank].name    = str_dup( col_text( stmt, 1 ) );
		top_board[rank].pkscore = sqlite3_column_int( stmt, 2 );
	}

	sqlite3_finalize( stmt );
}

void db_game_save_topboard( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO topboard (rank, name, pkscore) VALUES (?,?,?)";
	int i;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	db_begin( db );

	for ( i = 1; i <= MAX_TOP_PLAYERS; i++ ) {
		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, i );
		sqlite3_bind_text( stmt, 2, top_board[i].name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( stmt, 3, top_board[i].pkscore );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Leaderboard (game.db)
 * Each category is a single row: (category, name, value).
 */
void db_game_load_leaderboard( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT category, name, value FROM leaderboard";

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *cat  = col_text( stmt, 0 );
		const char *name = col_text( stmt, 1 );
		int value        = sqlite3_column_int( stmt, 2 );

		if ( !str_cmp( cat, "bestpk" ) ) {
			free_string( leader_board.bestpk_name );
			leader_board.bestpk_name   = str_dup( name );
			leader_board.bestpk_number = value;
		} else if ( !str_cmp( cat, "pk" ) ) {
			free_string( leader_board.pk_name );
			leader_board.pk_name   = str_dup( name );
			leader_board.pk_number = value;
		} else if ( !str_cmp( cat, "pd" ) ) {
			free_string( leader_board.pd_name );
			leader_board.pd_name   = str_dup( name );
			leader_board.pd_number = value;
		} else if ( !str_cmp( cat, "mk" ) ) {
			free_string( leader_board.mk_name );
			leader_board.mk_name   = str_dup( name );
			leader_board.mk_number = value;
		} else if ( !str_cmp( cat, "md" ) ) {
			free_string( leader_board.md_name );
			leader_board.md_name   = str_dup( name );
			leader_board.md_number = value;
		} else if ( !str_cmp( cat, "tt" ) ) {
			free_string( leader_board.tt_name );
			leader_board.tt_name   = str_dup( name );
			leader_board.tt_number = value;
		} else if ( !str_cmp( cat, "qc" ) ) {
			free_string( leader_board.qc_name );
			leader_board.qc_name   = str_dup( name );
			leader_board.qc_number = value;
		}
	}

	sqlite3_finalize( stmt );
}

void db_game_save_leaderboard( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO leaderboard (category, name, value) VALUES (?,?,?)";

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	db_begin( db );

#define SAVE_LB( cat, n, v ) \
	sqlite3_reset( stmt ); \
	sqlite3_bind_text( stmt, 1, cat, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( stmt, 2, n ? n : "Nobody", -1, SQLITE_TRANSIENT ); \
	sqlite3_bind_int( stmt, 3, v ); \
	sqlite3_step( stmt )

	SAVE_LB( "bestpk", leader_board.bestpk_name, leader_board.bestpk_number );
	SAVE_LB( "pk", leader_board.pk_name, leader_board.pk_number );
	SAVE_LB( "pd", leader_board.pd_name, leader_board.pd_number );
	SAVE_LB( "mk", leader_board.mk_name, leader_board.mk_number );
	SAVE_LB( "md", leader_board.md_name, leader_board.md_number );
	SAVE_LB( "tt", leader_board.tt_name, leader_board.tt_number );
	SAVE_LB( "qc", leader_board.qc_name, leader_board.qc_number );

#undef SAVE_LB

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Kingdoms (game.db)
 * One row per kingdom (1..MAX_KINGDOM).
 */
void db_game_load_kingdoms( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT id, name, whoname, leader, general, "
		"kills, deaths, qps, req_hit, req_move, req_mana, req_qps "
		"FROM kingdoms ORDER BY id";

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		int id = sqlite3_column_int( stmt, 0 );
		if ( id < 1 || id > MAX_KINGDOM )
			continue;
		free_string( kingdom_table[id].name );
		free_string( kingdom_table[id].whoname );
		free_string( kingdom_table[id].leader );
		free_string( kingdom_table[id].general );
		kingdom_table[id].name     = str_dup( col_text( stmt, 1 ) );
		kingdom_table[id].whoname  = str_dup( col_text( stmt, 2 ) );
		kingdom_table[id].leader   = str_dup( col_text( stmt, 3 ) );
		kingdom_table[id].general  = str_dup( col_text( stmt, 4 ) );
		kingdom_table[id].kills    = sqlite3_column_int( stmt, 5 );
		kingdom_table[id].deaths   = sqlite3_column_int( stmt, 6 );
		kingdom_table[id].qps      = sqlite3_column_int( stmt, 7 );
		kingdom_table[id].req_hit  = sqlite3_column_int( stmt, 8 );
		kingdom_table[id].req_move = sqlite3_column_int( stmt, 9 );
		kingdom_table[id].req_mana = sqlite3_column_int( stmt, 10 );
		kingdom_table[id].req_qps  = sqlite3_column_int( stmt, 11 );
	}

	sqlite3_finalize( stmt );
}

void db_game_save_kingdoms( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO kingdoms "
		"(id, name, whoname, leader, general, kills, deaths, qps, "
		"req_hit, req_move, req_mana, req_qps) "
		"VALUES (?,?,?,?,?,?,?,?,?,?,?,?)";
	int i;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	db_begin( db );

	for ( i = 1; i <= MAX_KINGDOM; i++ ) {
		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, i );
		sqlite3_bind_text( stmt, 2, kingdom_table[i].name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 3, kingdom_table[i].whoname, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 4, kingdom_table[i].leader, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 5, kingdom_table[i].general, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( stmt, 6, kingdom_table[i].kills );
		sqlite3_bind_int( stmt, 7, kingdom_table[i].deaths );
		sqlite3_bind_int( stmt, 8, kingdom_table[i].qps );
		sqlite3_bind_int( stmt, 9, kingdom_table[i].req_hit );
		sqlite3_bind_int( stmt, 10, kingdom_table[i].req_move );
		sqlite3_bind_int( stmt, 11, kingdom_table[i].req_mana );
		sqlite3_bind_int( stmt, 12, kingdom_table[i].req_qps );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Board notes (game.db)
 * One row per note, keyed by board_idx + date_stamp.
 */
void db_game_load_notes( int board_idx ) {
	sqlite3_stmt *del_stmt, *stmt;
	NOTE_DATA *last = NULL;

	if ( !game_db )
		return;

	/* Delete expired notes from DB */
	if ( sqlite3_prepare_v2( game_db,
			"DELETE FROM notes WHERE board_idx=? AND expire<?",
			-1, &del_stmt, NULL ) == SQLITE_OK ) {
		sqlite3_bind_int( del_stmt, 1, board_idx );
		sqlite3_bind_int64( del_stmt, 2, (sqlite3_int64)current_time );
		sqlite3_step( del_stmt );
		sqlite3_finalize( del_stmt );
	}

	/* Load remaining notes */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT sender, date, date_stamp, expire, to_list, subject, text "
			"FROM notes WHERE board_idx=? ORDER BY date_stamp ASC",
			-1, &stmt, NULL ) != SQLITE_OK )
		return;

	sqlite3_bind_int( stmt, 1, board_idx );
	boards[board_idx].note_first = NULL;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		NOTE_DATA *pnote = alloc_perm( sizeof( NOTE_DATA ) );
		pnote->sender     = str_dup( col_text( stmt, 0 ) );
		pnote->date       = str_dup( col_text( stmt, 1 ) );
		pnote->date_stamp = (time_t)sqlite3_column_int64( stmt, 2 );
		pnote->expire     = (time_t)sqlite3_column_int64( stmt, 3 );
		pnote->to_list    = str_dup( col_text( stmt, 4 ) );
		pnote->subject    = str_dup( col_text( stmt, 5 ) );
		pnote->text       = str_dup( col_text( stmt, 6 ) );
		pnote->next       = NULL;

		if ( !boards[board_idx].note_first )
			boards[board_idx].note_first = pnote;
		else
			last->next = pnote;
		last = pnote;
	}

	sqlite3_finalize( stmt );
}

void db_game_save_board_notes( int board_idx ) {
	sqlite3 *db;
	sqlite3_stmt *del_stmt, *ins_stmt;
	NOTE_DATA *note;

	db = db_game_open_for_write();
	if ( !db )
		return;

	db_begin( db );

	/* Delete all notes for this board */
	if ( sqlite3_prepare_v2( db,
			"DELETE FROM notes WHERE board_idx=?",
			-1, &del_stmt, NULL ) == SQLITE_OK ) {
		sqlite3_bind_int( del_stmt, 1, board_idx );
		sqlite3_step( del_stmt );
		sqlite3_finalize( del_stmt );
	}

	/* Re-insert all current notes */
	if ( sqlite3_prepare_v2( db,
			"INSERT INTO notes (board_idx, sender, date, date_stamp, expire, "
			"to_list, subject, text) VALUES (?,?,?,?,?,?,?,?)",
			-1, &ins_stmt, NULL ) != SQLITE_OK ) {
		db_rollback( db );
		sqlite3_close( db );
		return;
	}

	for ( note = boards[board_idx].note_first; note; note = note->next ) {
		sqlite3_reset( ins_stmt );
		sqlite3_bind_int( ins_stmt, 1, board_idx );
		sqlite3_bind_text( ins_stmt, 2, note->sender, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( ins_stmt, 3, note->date, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int64( ins_stmt, 4, (sqlite3_int64)note->date_stamp );
		sqlite3_bind_int64( ins_stmt, 5, (sqlite3_int64)note->expire );
		sqlite3_bind_text( ins_stmt, 6, note->to_list, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( ins_stmt, 7, note->subject, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( ins_stmt, 8, note->text, -1, SQLITE_TRANSIENT );
		sqlite3_step( ins_stmt );
	}

	sqlite3_finalize( ins_stmt );
	db_commit( db );
	sqlite3_close( db );
}

void db_game_append_note( int board_idx, NOTE_DATA *note ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO notes (board_idx, sender, date, date_stamp, expire, "
		"to_list, subject, text) VALUES (?,?,?,?,?,?,?,?)";

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	sqlite3_bind_int( stmt, 1, board_idx );
	sqlite3_bind_text( stmt, 2, note->sender, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 3, note->date, -1, SQLITE_TRANSIENT );
	sqlite3_bind_int64( stmt, 4, (sqlite3_int64)note->date_stamp );
	sqlite3_bind_int64( stmt, 5, (sqlite3_int64)note->expire );
	sqlite3_bind_text( stmt, 6, note->to_list, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 7, note->subject, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 8, note->text, -1, SQLITE_TRANSIENT );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
	sqlite3_close( db );
}


/*
 * Bug reports (game.db)
 * Append-only log of player bug reports and system bugs.
 */
void db_game_append_bug( int room_vnum, const char *player, const char *message ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO bugs (room_vnum, player, message, timestamp) VALUES (?,?,?,?)";

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	sqlite3_bind_int( stmt, 1, room_vnum );
	sqlite3_bind_text( stmt, 2, player, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 3, message, -1, SQLITE_TRANSIENT );
	sqlite3_bind_int64( stmt, 4, (sqlite3_int64)current_time );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
	sqlite3_close( db );
}


/*
 * Bans (game.db)
 * Site bans with optional reason.
 */
void db_game_load_bans( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT name, reason FROM bans ORDER BY id";

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	ban_list = NULL;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		BAN_DATA *p = alloc_mem( sizeof( BAN_DATA ) );
		p->name   = str_dup( col_text( stmt, 0 ) );
		p->reason = str_dup( col_text( stmt, 1 ) );
		p->next   = ban_list;
		ban_list  = p;
	}

	sqlite3_finalize( stmt );
}

void db_game_save_bans( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	BAN_DATA *p;

	db = db_game_open_for_write();
	if ( !db )
		return;

	db_begin( db );
	sqlite3_exec( db, "DELETE FROM bans", NULL, NULL, NULL );

	if ( sqlite3_prepare_v2( db,
			"INSERT INTO bans (name, reason) VALUES (?,?)",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		db_rollback( db );
		sqlite3_close( db );
		return;
	}

	for ( p = ban_list; p; p = p->next ) {
		sqlite3_reset( stmt );
		sqlite3_bind_text( stmt, 1, p->name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 2, p->reason, -1, SQLITE_TRANSIENT );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Disabled commands (game.db)
 * Commands that have been disabled by admins.
 */
void db_game_load_disabled( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT command_name, level, disabled_by FROM disabled_commands";
	int i;

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	disabled_first = NULL;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *name = col_text( stmt, 0 );
		DISABLED_DATA *p;

		for ( i = 0; cmd_table[i].name[0]; i++ )
			if ( !str_cmp( cmd_table[i].name, name ) )
				break;

		if ( !cmd_table[i].name[0] )
			continue;

		p = alloc_mem( sizeof( DISABLED_DATA ) );
		p->command     = &cmd_table[i];
		p->level       = (sh_int)sqlite3_column_int( stmt, 1 );
		p->disabled_by = str_dup( col_text( stmt, 2 ) );
		p->next        = disabled_first;
		disabled_first = p;
	}

	sqlite3_finalize( stmt );
}

void db_game_save_disabled( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	DISABLED_DATA *p;

	db = db_game_open_for_write();
	if ( !db )
		return;

	db_begin( db );
	sqlite3_exec( db, "DELETE FROM disabled_commands", NULL, NULL, NULL );

	if ( disabled_first ) {
		if ( sqlite3_prepare_v2( db,
				"INSERT INTO disabled_commands (command_name, level, disabled_by) "
				"VALUES (?,?,?)",
				-1, &stmt, NULL ) != SQLITE_OK ) {
			db_rollback( db );
			sqlite3_close( db );
			return;
		}

		for ( p = disabled_first; p; p = p->next ) {
			sqlite3_reset( stmt );
			sqlite3_bind_text( stmt, 1, p->command->name, -1, SQLITE_TRANSIENT );
			sqlite3_bind_int( stmt, 2, p->level );
			sqlite3_bind_text( stmt, 3, p->disabled_by, -1, SQLITE_TRANSIENT );
			sqlite3_step( stmt );
		}

		sqlite3_finalize( stmt );
	}

	db_commit( db );
	sqlite3_close( db );
}


/*
 * Balance config (game.db)
 * Key-value pairs for combat balance tuning constants.
 *
 * Uses the balance_map defined in balance.c to iterate all fields.
 */
typedef struct balance_entry {
	const char *key;
	int        *field;
} balance_entry_t;

extern balance_entry_t balance_map[];

void db_game_load_balance( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT key, value FROM balance_config";
	int i;

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *key = col_text( stmt, 0 );
		const char *val = col_text( stmt, 1 );

		for ( i = 0; balance_map[i].key != NULL; i++ ) {
			if ( !str_cmp( key, balance_map[i].key ) ) {
				*balance_map[i].field = atoi( val );
				break;
			}
		}
	}

	sqlite3_finalize( stmt );
}

void db_game_save_balance( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO balance_config (key, value) VALUES (?,?)";
	char buf[64];
	int i;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	db_begin( db );

	for ( i = 0; balance_map[i].key != NULL; i++ ) {
		sqlite3_reset( stmt );
		snprintf( buf, sizeof( buf ), "%d", *balance_map[i].field );
		sqlite3_bind_text( stmt, 1, balance_map[i].key, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 2, buf, -1, SQLITE_TRANSIENT );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Ability config (game.db)
 * Per-ability balance constants.  Uses the acfg_table defined in
 * ability_config.c to iterate all entries.
 */
void db_game_load_ability_config( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT key, value FROM ability_config";
	char buf[MAX_STRING_LENGTH];
	int count = 0;

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *key = col_text( stmt, 0 );
		int val         = sqlite3_column_int( stmt, 1 );

		if ( acfg_set( key, val ) )
			count++;
	}

	sqlite3_finalize( stmt );

	if ( count > 0 ) {
		snprintf( buf, sizeof( buf ),
			"  Loaded %d ability config overrides from game.db", count );
		log_string( buf );
	}
}

void db_game_save_ability_config( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO ability_config (key, value) VALUES (?,?)";
	int i, total;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	db_begin( db );

	/* Only save values that differ from defaults */
	total = acfg_count();
	for ( i = 0; i < total; i++ ) {
		acfg_entry_t *e = acfg_find_by_index( i );
		if ( !e || e->value == e->default_value )
			continue;
		sqlite3_reset( stmt );
		sqlite3_bind_text( stmt, 1, e->key, -1, SQLITE_STATIC );
		sqlite3_bind_int( stmt, 2, e->value );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Super admins (game.db)
 * Character names that can use the relevel command for instant admin access.
 * This allows forks to configure their own admins without modifying code.
 */
bool db_game_is_super_admin( const char *name ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "SELECT 1 FROM super_admins WHERE name = ? LIMIT 1";
	bool found = FALSE;

	if ( !name || !name[0] )
		return FALSE;

	db = db_game_open_for_write();
	if ( !db )
		return FALSE;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return FALSE;
	}

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );

	if ( sqlite3_step( stmt ) == SQLITE_ROW )
		found = TRUE;

	sqlite3_finalize( stmt );
	sqlite3_close( db );
	return found;
}

void db_game_add_super_admin( const char *name ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR IGNORE INTO super_admins (name) VALUES (?)";

	if ( !name || !name[0] )
		return;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
	sqlite3_close( db );
}

void db_game_remove_super_admin( const char *name ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "DELETE FROM super_admins WHERE name = ?";

	if ( !name || !name[0] )
		return;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
	sqlite3_close( db );
}

void db_game_list_super_admins( CHAR_DATA *ch ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "SELECT name FROM super_admins ORDER BY name";
	int count = 0;

	db = db_game_open_for_write();
	if ( !db ) {
		send_to_char( "Database not available.\n\r", ch );
		return;
	}

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		send_to_char( "Database error.\n\r", ch );
		sqlite3_close( db );
		return;
	}

	send_to_char( "#GSuperadmin list:#n\n\r", ch );

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ), "  %s\n\r", col_text( stmt, 0 ) );
		send_to_char( buf, ch );
		count++;
	}

	sqlite3_finalize( stmt );
	sqlite3_close( db );

	if ( count == 0 )
		send_to_char( "  (none)\n\r", ch );
	else {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ), "#wTotal: %d#n\n\r", count );
		send_to_char( buf, ch );
	}
}


/*
 * Audio config (game.db)
 * MCMP audio file mappings loaded into memory for fast lookup.
 */
static AUDIO_ENTRY *audio_entries = NULL;
static int audio_entry_count = 0;
static int audio_entry_alloc = 0;

/* Sector-indexed lookup arrays */
AUDIO_ENTRY *audio_ambient[SECT_MAX];
AUDIO_ENTRY *audio_footstep[SECT_MAX];

/* Parse sector key string to integer */
static int parse_sector_key( const char *key ) {
	if ( !str_cmp( key, "SECT_INSIDE" ) )      return SECT_INSIDE;
	if ( !str_cmp( key, "SECT_CITY" ) )        return SECT_CITY;
	if ( !str_cmp( key, "SECT_FIELD" ) )       return SECT_FIELD;
	if ( !str_cmp( key, "SECT_FOREST" ) )      return SECT_FOREST;
	if ( !str_cmp( key, "SECT_HILLS" ) )       return SECT_HILLS;
	if ( !str_cmp( key, "SECT_MOUNTAIN" ) )    return SECT_MOUNTAIN;
	if ( !str_cmp( key, "SECT_WATER_SWIM" ) )  return SECT_WATER_SWIM;
	if ( !str_cmp( key, "SECT_WATER_NOSWIM" )) return SECT_WATER_NOSWIM;
	if ( !str_cmp( key, "SECT_UNUSED" ) )      return SECT_UNUSED;
	if ( !str_cmp( key, "SECT_AIR" ) )         return SECT_AIR;
	if ( !str_cmp( key, "SECT_DESERT" ) )      return SECT_DESERT;
	return -1;
}

/* Insert default audio entries if table is empty */
static void audio_config_insert_defaults( void ) {
	sqlite3_stmt *stmt, *count_stmt;
	const char *sql = "INSERT OR IGNORE INTO audio_config "
		"(category, trigger_key, filename, volume, priority, loops, media_type, tag, caption, use_key, use_continue) "
		"VALUES (?,?,?,?,?,?,?,?,?,?,?)";
	int count = 0;

	if ( !game_db )
		return;

	/* Check if table already has entries */
	if ( sqlite3_prepare_v2( game_db, "SELECT COUNT(*) FROM audio_config", -1, &count_stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( count_stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( count_stmt, 0 );
		sqlite3_finalize( count_stmt );
	}

	if ( count > 0 )
		return;  /* Already has data */

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

#define INSERT_AUDIO( cat, key, file, vol, pri, lps, mtype, tg, cap, ukey, ucont ) \
	sqlite3_reset( stmt ); \
	sqlite3_bind_text( stmt, 1, cat, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( stmt, 2, key, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( stmt, 3, file, -1, SQLITE_STATIC ); \
	sqlite3_bind_int( stmt, 4, vol ); \
	sqlite3_bind_int( stmt, 5, pri ); \
	sqlite3_bind_int( stmt, 6, lps ); \
	sqlite3_bind_text( stmt, 7, mtype, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( stmt, 8, tg, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( stmt, 9, cap, -1, SQLITE_STATIC ); \
	if ( ukey ) sqlite3_bind_text( stmt, 10, ukey, -1, SQLITE_STATIC ); \
	else sqlite3_bind_null( stmt, 10 ); \
	sqlite3_bind_int( stmt, 11, ucont ); \
	sqlite3_step( stmt )

	/* Ambient sounds by sector type */
	INSERT_AUDIO( "ambient", "SECT_INSIDE",      "ambient/indoor.mp3",   20, 10, -1, "music", "environment", "Indoor ambience", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_CITY",        "ambient/city.mp3",     20, 10, -1, "music", "environment", "City sounds", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_FIELD",       "ambient/field.mp3",    20, 10, -1, "music", "environment", "Open field, wind blowing", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_FOREST",      "ambient/forest.mp3",   20, 10, -1, "music", "environment", "Forest, birds and rustling", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_HILLS",       "ambient/hills.mp3",    20, 10, -1, "music", "environment", "Hilly terrain, wind", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_MOUNTAIN",    "ambient/mountain.mp3", 20, 10, -1, "music", "environment", "Mountain wind, distant echoes", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_WATER_SWIM",  "ambient/water.mp3",    20, 10, -1, "music", "environment", "Water flowing", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_WATER_NOSWIM","ambient/water.mp3",    20, 10, -1, "music", "environment", "Water flowing", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_UNUSED",      "ambient/indoor.mp3",   20, 10, -1, "music", "environment", "Indoor ambience", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_AIR",         "ambient/air.mp3",      20, 10, -1, "music", "environment", "High altitude wind", "ambient", 1 );
	INSERT_AUDIO( "ambient", "SECT_DESERT",      "ambient/desert.mp3",   20, 10, -1, "music", "environment", "Desert wind", "ambient", 1 );

	/* Footstep sounds by sector type */
	INSERT_AUDIO( "footstep", "SECT_INSIDE",      "movement/footstep_stone.mp3",  25, 10, 1, "sound", "movement", "Footsteps on stone", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_CITY",        "movement/footstep_stone.mp3",  25, 10, 1, "sound", "movement", "Footsteps on stone", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_FIELD",       "movement/footstep_grass.mp3",  25, 10, 1, "sound", "movement", "Footsteps on grass", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_FOREST",      "movement/footstep_forest.mp3", 25, 10, 1, "sound", "movement", "Footsteps through undergrowth", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_HILLS",       "movement/footstep_gravel.mp3", 25, 10, 1, "sound", "movement", "Footsteps on gravel", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_MOUNTAIN",    "movement/footstep_gravel.mp3", 25, 10, 1, "sound", "movement", "Footsteps on gravel", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_WATER_SWIM",  "movement/splash.mp3",          25, 10, 1, "sound", "movement", "Splashing through water", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_WATER_NOSWIM","movement/splash.mp3",          25, 10, 1, "sound", "movement", "Splashing through water", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_UNUSED",      "movement/footstep_stone.mp3",  25, 10, 1, "sound", "movement", "Footsteps on stone", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_AIR",         "movement/whoosh.mp3",          25, 10, 1, "sound", "movement", "Wind rushing past", NULL, 0 );
	INSERT_AUDIO( "footstep", "SECT_DESERT",      "movement/footstep_sand.mp3",   25, 10, 1, "sound", "movement", "Footsteps on sand", NULL, 0 );

	/* Combat sounds */
	INSERT_AUDIO( "combat", "combat_miss",      "combat/miss.mp3",      30, 30, 1, "sound", "combat", "Attacks miss", NULL, 0 );
	INSERT_AUDIO( "combat", "combat_light_hit", "combat/light_hit.mp3", 40, 30, 1, "sound", "combat", "Light blows exchanged", NULL, 0 );
	INSERT_AUDIO( "combat", "combat_heavy_hit", "combat/heavy_hit.mp3", 50, 40, 1, "sound", "combat", "Heavy blows land", NULL, 0 );
	INSERT_AUDIO( "combat", "combat_death",     "combat/death.mp3",     60, 80, 1, "sound", "combat", "A death cry rings out", NULL, 0 );
	INSERT_AUDIO( "combat", "combat_engage",    "combat/engage.mp3",    50, 50, 1, "sound", "combat", "Combat begins", NULL, 0 );
	INSERT_AUDIO( "combat", "combat_victory",   "combat/victory.mp3",   50, 60, 1, "sound", "combat", "Victory", NULL, 0 );

	/* Weather sounds */
	INSERT_AUDIO( "weather", "SKY_CLOUDY",    "weather/wind.mp3",       15, 20, -1, "music", "weather", "Wind picks up", "weather", 0 );
	INSERT_AUDIO( "weather", "SKY_RAINING",   "weather/rain_heavy.mp3", 25, 20, -1, "music", "weather", "Heavy rain", "weather", 0 );
	INSERT_AUDIO( "weather", "SKY_LIGHTNING", "weather/thunder.mp3",    50, 60, 1,  "sound", "weather", "Thunder crashes", NULL, 0 );

	/* Channel notification sounds */
	INSERT_AUDIO( "channel", "CHANNEL_TELL",    "channels/tell.mp3",    30, 20, 1, "sound", "channel", "Private message", NULL, 0 );
	INSERT_AUDIO( "channel", "CHANNEL_YELL",    "channels/yell.mp3",    30, 20, 1, "sound", "channel", "Someone yells", NULL, 0 );
	INSERT_AUDIO( "channel", "CHANNEL_IMMTALK", "channels/immtalk.mp3", 30, 20, 1, "sound", "channel", "Immortal message", NULL, 0 );
	INSERT_AUDIO( "channel", "CHANNEL_CHAT",    "channels/chat.mp3",    30, 20, 1, "sound", "channel", "Chat message", NULL, 0 );

	/* Time of day sounds */
	INSERT_AUDIO( "time", "hour_5",  "environment/dawn.mp3",           30, 30, 1, "sound", "environment", "Dawn breaks", NULL, 0 );
	INSERT_AUDIO( "time", "hour_6",  "environment/sunrise.mp3",        30, 30, 1, "sound", "environment", "Birds sing at sunrise", NULL, 0 );
	INSERT_AUDIO( "time", "hour_19", "environment/sunset.mp3",         30, 30, 1, "sound", "environment", "Evening crickets", NULL, 0 );
	INSERT_AUDIO( "time", "hour_20", "environment/nightfall.mp3",      30, 30, 1, "sound", "environment", "Night creatures stir", NULL, 0 );
	INSERT_AUDIO( "time", "hour_0",  "environment/clock_midnight.mp3", 35, 40, 1, "sound", "environment", "Clock tolls midnight", NULL, 0 );

	/* UI event sounds */
	INSERT_AUDIO( "ui", "ui_login",       "ui/login.mp3",       50, 70, 1, "sound", "ui", "Welcome", NULL, 0 );
	INSERT_AUDIO( "ui", "ui_levelup",     "ui/levelup.mp3",     50, 70, 1, "sound", "ui", "Level up", NULL, 0 );
	INSERT_AUDIO( "ui", "ui_death",       "ui/death.mp3",       60, 90, 1, "sound", "ui", "You have died", NULL, 0 );
	INSERT_AUDIO( "ui", "ui_achievement", "ui/achievement.mp3", 50, 70, 1, "sound", "ui", "Achievement earned", NULL, 0 );

	/* Spell sounds */
	INSERT_AUDIO( "spell", "spell_generic", "specials/spell_cast.mp3", 40, 40, 1, "sound", "spell", "A spell is cast", NULL, 0 );

	/* Environment misc sounds */
	INSERT_AUDIO( "environment", "env_bell",       "environment/bell_distant.mp3", 25, 20, 1, "sound", "environment", "Distant bell", NULL, 0 );
	INSERT_AUDIO( "environment", "env_door_open",  "environment/door_open.mp3",    35, 20, 1, "sound", "environment", "Door opens", NULL, 0 );
	INSERT_AUDIO( "environment", "env_door_close", "environment/door_close.mp3",   35, 20, 1, "sound", "environment", "Door closes", NULL, 0 );
	INSERT_AUDIO( "environment", "env_howl",       "environment/howl.mp3",         50, 30, 1, "sound", "environment", "Wolf howl", NULL, 0 );

#undef INSERT_AUDIO

	db_commit( game_db );
	sqlite3_finalize( stmt );

	log_string( "  Inserted default audio config entries." );
}

/*
 * Load audio config from database into memory.
 * Call this during boot after db_game_init().
 */
void db_game_load_audio_config( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT category, trigger_key, filename, volume, priority, "
		"loops, media_type, tag, caption, use_key, use_continue "
		"FROM audio_config ORDER BY category, trigger_key";
	char buf[256];
	int i;

	if ( !game_db )
		return;

	/* Insert defaults if table is empty */
	audio_config_insert_defaults();

	/* Free existing entries */
	if ( audio_entries != NULL ) {
		for ( i = 0; i < audio_entry_count; i++ ) {
			if ( audio_entries[i].category )   free_string( audio_entries[i].category );
			if ( audio_entries[i].trigger_key ) free_string( audio_entries[i].trigger_key );
			if ( audio_entries[i].filename )   free_string( audio_entries[i].filename );
			if ( audio_entries[i].media_type ) free_string( audio_entries[i].media_type );
			if ( audio_entries[i].tag )        free_string( audio_entries[i].tag );
			if ( audio_entries[i].caption )    free_string( audio_entries[i].caption );
			if ( audio_entries[i].use_key )    free_string( audio_entries[i].use_key );
		}
		free_mem( audio_entries, audio_entry_alloc * sizeof( AUDIO_ENTRY ) );
		audio_entries = NULL;
		audio_entry_count = 0;
		audio_entry_alloc = 0;
	}

	/* Clear sector lookup arrays */
	for ( i = 0; i < SECT_MAX; i++ ) {
		audio_ambient[i] = NULL;
		audio_footstep[i] = NULL;
	}

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	/* Count rows first */
	while ( sqlite3_step( stmt ) == SQLITE_ROW )
		audio_entry_alloc++;
	sqlite3_reset( stmt );

	if ( audio_entry_alloc == 0 ) {
		sqlite3_finalize( stmt );
		return;
	}

	/* Allocate array */
	audio_entries = alloc_mem( audio_entry_alloc * sizeof( AUDIO_ENTRY ) );

	/* Load entries */
	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		AUDIO_ENTRY *ae = &audio_entries[audio_entry_count];
		const char *ukey;

		ae->category    = str_dup( col_text( stmt, 0 ) );
		ae->trigger_key = str_dup( col_text( stmt, 1 ) );
		ae->filename    = str_dup( col_text( stmt, 2 ) );
		ae->volume      = sqlite3_column_int( stmt, 3 );
		ae->priority    = sqlite3_column_int( stmt, 4 );
		ae->loops       = sqlite3_column_int( stmt, 5 );
		ae->media_type  = str_dup( col_text( stmt, 6 ) );
		ae->tag         = str_dup( col_text( stmt, 7 ) );
		ae->caption     = str_dup( col_text( stmt, 8 ) );
		ukey = (const char *)sqlite3_column_text( stmt, 9 );
		ae->use_key     = ( ukey && ukey[0] ) ? str_dup( ukey ) : NULL;
		ae->use_continue = sqlite3_column_int( stmt, 10 ) != 0;

		audio_entry_count++;
	}

	sqlite3_finalize( stmt );

	/* Build sector-indexed lookup arrays */
	for ( i = 0; i < audio_entry_count; i++ ) {
		AUDIO_ENTRY *ae = &audio_entries[i];
		int sect;

		if ( !str_cmp( ae->category, "ambient" ) ) {
			sect = parse_sector_key( ae->trigger_key );
			if ( sect >= 0 && sect < SECT_MAX )
				audio_ambient[sect] = ae;
		}
		else if ( !str_cmp( ae->category, "footstep" ) ) {
			sect = parse_sector_key( ae->trigger_key );
			if ( sect >= 0 && sect < SECT_MAX )
				audio_footstep[sect] = ae;
		}
	}

	snprintf( buf, sizeof( buf ), "  Loaded %d audio config entries.", audio_entry_count );
	log_string( buf );
}

/*
 * Find an audio config entry by category and trigger key.
 * Returns NULL if not found.
 */
AUDIO_ENTRY *audio_config_find( const char *category, const char *trigger_key ) {
	int i;

	if ( category == NULL || trigger_key == NULL )
		return NULL;

	for ( i = 0; i < audio_entry_count; i++ ) {
		if ( !str_cmp( audio_entries[i].category, category ) &&
		     !str_cmp( audio_entries[i].trigger_key, trigger_key ) )
			return &audio_entries[i];
	}

	return NULL;
}

/*
 * Save a single audio config entry to the database.
 */
static void audio_config_save_entry( AUDIO_ENTRY *ae ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "UPDATE audio_config SET filename=?, volume=?, priority=?, "
		"loops=?, media_type=?, tag=?, caption=?, use_key=?, use_continue=? "
		"WHERE category=? AND trigger_key=?";

	if ( !ae )
		return;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	sqlite3_bind_text( stmt, 1, ae->filename, -1, SQLITE_TRANSIENT );
	sqlite3_bind_int( stmt, 2, ae->volume );
	sqlite3_bind_int( stmt, 3, ae->priority );
	sqlite3_bind_int( stmt, 4, ae->loops );
	sqlite3_bind_text( stmt, 5, ae->media_type, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 6, ae->tag, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 7, ae->caption, -1, SQLITE_TRANSIENT );
	if ( ae->use_key )
		sqlite3_bind_text( stmt, 8, ae->use_key, -1, SQLITE_TRANSIENT );
	else
		sqlite3_bind_null( stmt, 8 );
	sqlite3_bind_int( stmt, 9, ae->use_continue ? 1 : 0 );
	sqlite3_bind_text( stmt, 10, ae->category, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 11, ae->trigger_key, -1, SQLITE_TRANSIENT );

	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
	sqlite3_close( db );
}

/*
 * In-game command to view and edit audio configuration.
 *
 * Syntax:
 *   audioconfig                      - list categories
 *   audioconfig <category>           - list entries in category
 *   audioconfig <cat> <trigger>      - show entry details
 *   audioconfig <cat> <trigger> <field> <value> - edit a field
 */
void do_audioconfig( CHAR_DATA *ch, char *argument ) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char arg4[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	AUDIO_ENTRY *ae;
	int i, count;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	argument = one_argument( argument, arg3 );
	strcpy( arg4, argument );  /* Value may have spaces */

	/* No arguments - list categories */
	if ( arg1[0] == '\0' ) {
		const char *categories[] = {
			"ambient", "footstep", "combat", "weather",
			"channel", "time", "ui", "spell", "environment", NULL
		};

		send_to_char( "#GAudio Config Categories:#n\n\r\n\r", ch );

		for ( i = 0; categories[i] != NULL; i++ ) {
			count = 0;
			for ( int j = 0; j < audio_entry_count; j++ ) {
				if ( !str_cmp( audio_entries[j].category, categories[i] ) )
					count++;
			}
			snprintf( buf, sizeof( buf ), "  %-12s  %d entries\n\r",
				categories[i], count );
			send_to_char( buf, ch );
		}

		send_to_char( "\n\r#wSyntax:#n audioconfig <category> [trigger] [field] [value]\n\r", ch );
		send_to_char( "#wFields:#n filename volume priority loops media_type tag caption use_key use_continue\n\r", ch );
		return;
	}

	/* One argument - list entries in category */
	if ( arg2[0] == '\0' ) {
		send_to_char( "#GAudio entries in category:#n ", ch );
		send_to_char( arg1, ch );
		send_to_char( "\n\r\n\r", ch );

		count = 0;
		for ( i = 0; i < audio_entry_count; i++ ) {
			if ( !str_cmp( audio_entries[i].category, arg1 ) ) {
				snprintf( buf, sizeof( buf ),
					"  %-20s  %-30s  vol=%d pri=%d\n\r",
					audio_entries[i].trigger_key,
					audio_entries[i].filename,
					audio_entries[i].volume,
					audio_entries[i].priority );
				send_to_char( buf, ch );
				count++;
			}
		}

		if ( count == 0 )
			send_to_char( "  (no entries found)\n\r", ch );
		else {
			snprintf( buf, sizeof( buf ), "\n\r#wTotal:#n %d entries\n\r", count );
			send_to_char( buf, ch );
		}
		return;
	}

	/* Two arguments - show entry details */
	ae = audio_config_find( arg1, arg2 );
	if ( ae == NULL ) {
		send_to_char( "No audio entry found for that category/trigger.\n\r", ch );
		return;
	}

	if ( arg3[0] == '\0' ) {
		snprintf( buf, sizeof( buf ),
			"#GAudio Entry:#n %s / %s\n\r\n\r"
			"  filename:     %s\n\r"
			"  volume:       %d\n\r"
			"  priority:     %d\n\r"
			"  loops:        %d\n\r"
			"  media_type:   %s\n\r"
			"  tag:          %s\n\r"
			"  caption:      %s\n\r"
			"  use_key:      %s\n\r"
			"  use_continue: %s\n\r",
			ae->category, ae->trigger_key,
			ae->filename,
			ae->volume,
			ae->priority,
			ae->loops,
			ae->media_type,
			ae->tag[0] ? ae->tag : "(none)",
			ae->caption[0] ? ae->caption : "(none)",
			ae->use_key ? ae->use_key : "(none)",
			ae->use_continue ? "true" : "false" );
		send_to_char( buf, ch );
		return;
	}

	/* Four arguments - edit a field */
	if ( arg4[0] == '\0' ) {
		send_to_char( "Syntax: audioconfig <category> <trigger> <field> <value>\n\r", ch );
		return;
	}

	/* Edit the field */
	if ( !str_cmp( arg3, "filename" ) ) {
		free_string( ae->filename );
		ae->filename = str_dup( arg4 );
	}
	else if ( !str_cmp( arg3, "volume" ) ) {
		int val = atoi( arg4 );
		if ( val < 0 || val > 100 ) {
			send_to_char( "Volume must be 0-100.\n\r", ch );
			return;
		}
		ae->volume = val;
	}
	else if ( !str_cmp( arg3, "priority" ) ) {
		int val = atoi( arg4 );
		if ( val < 0 || val > 100 ) {
			send_to_char( "Priority must be 0-100.\n\r", ch );
			return;
		}
		ae->priority = val;
	}
	else if ( !str_cmp( arg3, "loops" ) ) {
		ae->loops = atoi( arg4 );
	}
	else if ( !str_cmp( arg3, "media_type" ) ) {
		if ( str_cmp( arg4, "sound" ) && str_cmp( arg4, "music" ) ) {
			send_to_char( "Media type must be 'sound' or 'music'.\n\r", ch );
			return;
		}
		free_string( ae->media_type );
		ae->media_type = str_dup( arg4 );
	}
	else if ( !str_cmp( arg3, "tag" ) ) {
		free_string( ae->tag );
		ae->tag = str_dup( arg4 );
	}
	else if ( !str_cmp( arg3, "caption" ) ) {
		free_string( ae->caption );
		ae->caption = str_dup( arg4 );
	}
	else if ( !str_cmp( arg3, "use_key" ) ) {
		if ( ae->use_key )
			free_string( ae->use_key );
		if ( !str_cmp( arg4, "none" ) || !str_cmp( arg4, "null" ) )
			ae->use_key = NULL;
		else
			ae->use_key = str_dup( arg4 );
	}
	else if ( !str_cmp( arg3, "use_continue" ) ) {
		if ( !str_cmp( arg4, "true" ) || !str_cmp( arg4, "1" ) || !str_cmp( arg4, "yes" ) )
			ae->use_continue = TRUE;
		else
			ae->use_continue = FALSE;
	}
	else {
		send_to_char( "Unknown field. Valid fields:\n\r", ch );
		send_to_char( "  filename volume priority loops media_type tag caption use_key use_continue\n\r", ch );
		return;
	}

	/* Save to database */
	audio_config_save_entry( ae );

	snprintf( buf, sizeof( buf ), "Audio config %s/%s %s set to: %s\n\r",
		ae->category, ae->trigger_key, arg3, arg4 );
	send_to_char( buf, ch );
}


/*
 * Immortal pretitles (game.db)
 * Custom pre-titles for immortals displayed in the who list.
 */

void db_game_load_pretitles( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "SELECT immortal_name, pretitle, set_by, set_date "
		"FROM immortal_pretitles ORDER BY immortal_name";
	int i, count = 0;

	/* Free existing entries */
	if ( pretitle_entries != NULL ) {
		for ( i = 0; i < pretitle_count; i++ ) {
			if ( pretitle_entries[i].immortal_name )
				free_string( pretitle_entries[i].immortal_name );
			if ( pretitle_entries[i].pretitle )
				free_string( pretitle_entries[i].pretitle );
			if ( pretitle_entries[i].set_by )
				free_string( pretitle_entries[i].set_by );
		}
		free_mem( pretitle_entries, pretitle_count * sizeof( PRETITLE_ENTRY ) );
		pretitle_entries = NULL;
		pretitle_count = 0;
	}

	/* Use global game_db if available (during boot), otherwise open fresh */
	db = game_db ? game_db : db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		if ( !game_db ) sqlite3_close( db );
		return;
	}

	/* Count rows first */
	while ( sqlite3_step( stmt ) == SQLITE_ROW )
		count++;
	sqlite3_reset( stmt );

	if ( count == 0 ) {
		sqlite3_finalize( stmt );
		if ( !game_db ) sqlite3_close( db );
		return;
	}

	/* Allocate array */
	pretitle_entries = alloc_mem( count * sizeof( PRETITLE_ENTRY ) );

	/* Load entries */
	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		PRETITLE_ENTRY *pe = &pretitle_entries[pretitle_count];

		pe->immortal_name = str_dup( col_text( stmt, 0 ) );
		pe->pretitle      = str_dup( col_text( stmt, 1 ) );
		pe->set_by        = str_dup( col_text( stmt, 2 ) );
		pe->set_date      = (time_t)sqlite3_column_int64( stmt, 3 );

		pretitle_count++;
	}

	sqlite3_finalize( stmt );
	if ( !game_db ) sqlite3_close( db );
}

const char *db_game_get_pretitle( const char *name ) {
	int i;

	if ( !name || !name[0] )
		return NULL;

	for ( i = 0; i < pretitle_count; i++ ) {
		if ( !str_cmp( pretitle_entries[i].immortal_name, name ) )
			return pretitle_entries[i].pretitle;
	}

	return NULL;
}

void db_game_set_pretitle( const char *name, const char *pretitle, const char *set_by ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO immortal_pretitles "
		"(immortal_name, pretitle, set_by, set_date) VALUES (?,?,?,?)";

	if ( !name || !name[0] || !pretitle || !pretitle[0] )
		return;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 2, pretitle, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 3, set_by ? set_by : "system", -1, SQLITE_TRANSIENT );
	sqlite3_bind_int64( stmt, 4, (sqlite3_int64)current_time );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
	sqlite3_close( db );

	/* Reload the cache */
	db_game_load_pretitles();
}

void db_game_delete_pretitle( const char *name ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "DELETE FROM immortal_pretitles WHERE immortal_name = ?";

	if ( !name || !name[0] )
		return;

	db = db_game_open_for_write();
	if ( !db )
		return;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		return;
	}

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
	sqlite3_close( db );

	/* Reload the cache */
	db_game_load_pretitles();
}

void db_game_list_pretitles( CHAR_DATA *ch ) {
	char buf[MAX_STRING_LENGTH];
	int i;

	/* Uses in-memory cache, no DB connection needed */
	send_to_char( "#GImmortal pretitles:#n\n\r", ch );

	if ( pretitle_count == 0 ) {
		send_to_char( "  (none)\n\r", ch );
		return;
	}

	for ( i = 0; i < pretitle_count; i++ ) {
		snprintf( buf, sizeof( buf ), "  #w%-15s#n : %s#n\n\r",
			pretitle_entries[i].immortal_name,
			pretitle_entries[i].pretitle );
		send_to_char( buf, ch );
	}

	snprintf( buf, sizeof( buf ), "#wTotal: %d#n\n\r", pretitle_count );
	send_to_char( buf, ch );
}
