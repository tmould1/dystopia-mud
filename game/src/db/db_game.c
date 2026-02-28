/***************************************************************************
 *  db_game.c - SQLite persistence for global game data
 *
 *  Manages base_help.db + live_help.db (help entries) and game.db
 *  (config, leaderboards, kingdoms) stored in gamedata/db/game/.
 ***************************************************************************/

#include "db_util.h"
#include "db_game.h"
#include "../core/cfg.h"
#include "../core/utf8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External globals */
extern char mud_db_dir[MUD_PATH_MAX];
extern list_head_t g_helps;
extern char *help_greeting;
extern int top_help;
extern TOP_BOARD top_board[MAX_TOP_PLAYERS + 1];
extern LEADER_BOARD leader_board;
extern KINGDOM_DATA kingdom_table[MAX_KINGDOM + 1];
extern GAMECONFIG_DATA game_config;
extern BOARD_DATA boards[];
extern BAN_DATA *ban_list;
extern DISABLED_DATA *disabled_first;
extern const struct cmd_type cmd_table[];
extern time_t current_time;

/* Cached database connections */
static sqlite3 *base_help_db = NULL;
static sqlite3 *live_help_db = NULL;
static sqlite3 *game_db = NULL;

/* Directory for game databases (non-static for db_class.c access) */
char mud_db_game_dir[MUD_PATH_MAX] = "";

/* In-memory cache for immortal pretitles */
typedef struct pretitle_entry {
	char   *immortal_name;
	char   *pretitle;
	char   *set_by;
	time_t  set_date;
} PRETITLE_ENTRY;

static PRETITLE_ENTRY *pretitle_entries = NULL;
static int pretitle_count = 0;

/* Forward declarations */
static void db_game_migrate_schema( void );
static void db_game_seed_name_rules( void );

/* In-memory name validation lists */
FORBIDDEN_NAME  *forbidden_name_list  = NULL;
PROFANITY_FILTER *profanity_filter_list = NULL;

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

	"CREATE TABLE IF NOT EXISTS config ("
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
	");"

	"CREATE TABLE IF NOT EXISTS forbidden_names ("
	"  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  name     TEXT NOT NULL COLLATE NOCASE,"
	"  type     INTEGER NOT NULL DEFAULT 0,"
	"  added_by TEXT NOT NULL DEFAULT 'system'"
	");"

	"CREATE TABLE IF NOT EXISTS profanity_filters ("
	"  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  pattern  TEXT NOT NULL COLLATE NOCASE,"
	"  added_by TEXT NOT NULL DEFAULT 'system'"
	");"

	"CREATE TABLE IF NOT EXISTS confusable_chars ("
	"  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  codepoint INTEGER NOT NULL UNIQUE,"
	"  canonical TEXT NOT NULL"
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
		/* WAL mode for faster writes */
		sqlite3_exec( live_help_db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL );
		sqlite3_exec( live_help_db, "PRAGMA synchronous=NORMAL", NULL, NULL, NULL );
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
		/* Run schema migrations for existing databases */
		db_game_migrate_schema();

		/* Seed name validation tables if empty (first boot) */
		db_game_seed_name_rules();

		/* WAL mode: eliminates journal file create/delete per transaction.
		 * synchronous=NORMAL: fsync only at WAL checkpoints, not every commit.
		 * Together these reduce write latency ~5-10x vs defaults. */
		sqlite3_exec( game_db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL );
		sqlite3_exec( game_db, "PRAGMA synchronous=NORMAL", NULL, NULL, NULL );
	}
}

/*
 * Migrate existing game.db schemas by adding new columns.
 * Uses ALTER TABLE which is safe on existing data.
 * Called during db_game_init() after schema creation.
 */
static void db_game_migrate_schema( void ) {
	/* Currently no game.db-specific migrations needed */
	(void)game_db;  /* Suppress unused warning */
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
	db_class_close();
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

	/* WAL mode for faster writes (matches db_game_init settings) */
	sqlite3_exec( db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL );
	sqlite3_exec( db, "PRAGMA synchronous=NORMAL", NULL, NULL, NULL );

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

	/* WAL mode for faster writes */
	sqlite3_exec( db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL );
	sqlite3_exec( db, "PRAGMA synchronous=NORMAL", NULL, NULL, NULL );

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
		pHelp->level   = (int)sqlite3_column_int( stmt, 0 );
		pHelp->keyword = str_dup( col_text( stmt, 1 ) );
		pHelp->text    = str_dup( col_text( stmt, 2 ) );
		pHelp->area    = NULL;

		if ( pHelp->keyword[0] == '\0' ) {
			free( pHelp->text );
			free( pHelp->keyword );
			free( pHelp );
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

	LIST_FOR_EACH( pHelp, &g_helps, HELP_DATA, node ) {
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
			pHelp->level   = (int)sqlite3_column_int( stmt, 0 );
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
			free(game_config.game_name);
			game_config.game_name = str_dup( val );
		} else if ( !str_cmp( key, "gui_url" ) ) {
			free(game_config.gui_url);
			game_config.gui_url = str_dup( val );
		} else if ( !str_cmp( key, "gui_version" ) ) {
			free(game_config.gui_version);
			game_config.gui_version = str_dup( val );
		} else if ( !str_cmp( key, "banner_left" ) ) {
			free(game_config.banner_left);
			game_config.banner_left = str_dup( val );
		} else if ( !str_cmp( key, "banner_right" ) ) {
			free(game_config.banner_right);
			game_config.banner_right = str_dup( val );
		} else if ( !str_cmp( key, "banner_fill" ) ) {
			free(game_config.banner_fill);
			game_config.banner_fill = str_dup( val );
		} else if ( !str_cmp( key, "audio_url" ) ) {
			free(game_config.audio_url);
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
		free(top_board[rank].name);
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
			free(leader_board.bestpk_name);
			leader_board.bestpk_name   = str_dup( name );
			leader_board.bestpk_number = value;
		} else if ( !str_cmp( cat, "pk" ) ) {
			free(leader_board.pk_name);
			leader_board.pk_name   = str_dup( name );
			leader_board.pk_number = value;
		} else if ( !str_cmp( cat, "pd" ) ) {
			free(leader_board.pd_name);
			leader_board.pd_name   = str_dup( name );
			leader_board.pd_number = value;
		} else if ( !str_cmp( cat, "mk" ) ) {
			free(leader_board.mk_name);
			leader_board.mk_name   = str_dup( name );
			leader_board.mk_number = value;
		} else if ( !str_cmp( cat, "md" ) ) {
			free(leader_board.md_name);
			leader_board.md_name   = str_dup( name );
			leader_board.md_number = value;
		} else if ( !str_cmp( cat, "tt" ) ) {
			free(leader_board.tt_name);
			leader_board.tt_name   = str_dup( name );
			leader_board.tt_number = value;
		} else if ( !str_cmp( cat, "qc" ) ) {
			free(leader_board.qc_name);
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
		free(kingdom_table[id].name);
		free(kingdom_table[id].whoname);
		free(kingdom_table[id].leader);
		free(kingdom_table[id].general);
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
		NOTE_DATA *pnote = calloc( 1, sizeof( NOTE_DATA ) );
		if ( !pnote ) {
			bug( "load_board_notes: calloc failed", 0 );
			exit( 1 );
		}
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
		BAN_DATA *p = calloc( 1, sizeof( BAN_DATA ) );
		if ( !p ) { bug( "db_game_load_bans: calloc failed", 0 ); exit( 1 ); }
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

		p = calloc( 1, sizeof( DISABLED_DATA ) );
		if ( !p ) { bug( "db_game_load_disabled: calloc failed", 0 ); exit( 1 ); }
		p->command     = &cmd_table[i];
		p->level       = (int)sqlite3_column_int( stmt, 1 );
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
 * Unified config (game.db)
 */
void db_game_load_cfg( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT key, value FROM config";
	char buf[MAX_STRING_LENGTH];
	int count = 0;

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *key_str = col_text( stmt, 0 );
		int val             = sqlite3_column_int( stmt, 1 );
		cfg_key_t key       = cfg_key_from_string( key_str );

		if ( key != CFG_COUNT ) {
			cfg_set( key, val );
			count++;
		}
	}

	sqlite3_finalize( stmt );

	if ( count > 0 ) {
		snprintf( buf, sizeof( buf ),
			"  Loaded %d config overrides from game.db", count );
		log_string( buf );
	}
}

void db_game_save_cfg( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO config (key, value) VALUES (?,?)";
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
	total = cfg_count();
	for ( i = 0; i < total; i++ ) {
		cfg_entry_t *e = cfg_entry_by_index( i );
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

	/* Free existing entries */
	if ( audio_entries != NULL ) {
		for ( i = 0; i < audio_entry_count; i++ ) {
			if ( audio_entries[i].category )   free(audio_entries[i].category);
			if ( audio_entries[i].trigger_key ) free(audio_entries[i].trigger_key);
			if ( audio_entries[i].filename )   free(audio_entries[i].filename);
			if ( audio_entries[i].media_type ) free(audio_entries[i].media_type);
			if ( audio_entries[i].tag )        free(audio_entries[i].tag);
			if ( audio_entries[i].caption )    free(audio_entries[i].caption);
			if ( audio_entries[i].use_key )    free(audio_entries[i].use_key);
		}
		free( audio_entries );
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
	audio_entries = calloc( 1, audio_entry_alloc * sizeof( AUDIO_ENTRY ) );
	if ( !audio_entries ) { bug( "db_game_load_audio: calloc failed", 0 ); exit( 1 ); }

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
		free(ae->filename);
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
		free(ae->media_type);
		ae->media_type = str_dup( arg4 );
	}
	else if ( !str_cmp( arg3, "tag" ) ) {
		free(ae->tag);
		ae->tag = str_dup( arg4 );
	}
	else if ( !str_cmp( arg3, "caption" ) ) {
		free(ae->caption);
		ae->caption = str_dup( arg4 );
	}
	else if ( !str_cmp( arg3, "use_key" ) ) {
		if ( ae->use_key )
			free(ae->use_key);
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
				free(pretitle_entries[i].immortal_name);
			if ( pretitle_entries[i].pretitle )
				free(pretitle_entries[i].pretitle);
			if ( pretitle_entries[i].set_by )
				free(pretitle_entries[i].set_by);
		}
		free( pretitle_entries );
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
	pretitle_entries = calloc( 1, count * sizeof( PRETITLE_ENTRY ) );
	if ( !pretitle_entries ) { bug( "db_game_load_pretitles: calloc failed", 0 ); exit( 1 ); }

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


/*
 * =======================================================================
 * Name validation tables (game.db)
 *
 * Three tables provide database-driven name validation:
 *   forbidden_names  - reserved words, protected prefixes, exact blocks
 *   profanity_filters - substring patterns checked against skeletonized names
 *   confusable_chars  - Unicode -> ASCII mappings for skeleton normalization
 *
 * These replace the hardcoded checks in check_parse_name() and prevent
 * UTF-8 confusable-character bypass of profanity filters.
 * =======================================================================
 */

/* Seed data for confusable characters (sorted by codepoint for binary search).
 * Covers: accented Latin, Greek lookalikes, Cyrillic lookalikes. */
static const struct {
	unsigned int codepoint;
	const char  *canonical;
} confusable_seed[] = {
	/* Accented Latin uppercase */
	{ 0x00C0, "a" }, { 0x00C1, "a" }, { 0x00C2, "a" }, { 0x00C3, "a" },
	{ 0x00C4, "a" }, { 0x00C5, "a" }, { 0x00C8, "e" }, { 0x00C9, "e" },
	{ 0x00CA, "e" }, { 0x00CB, "e" }, { 0x00CC, "i" }, { 0x00CD, "i" },
	{ 0x00CE, "i" }, { 0x00CF, "i" }, { 0x00D2, "o" }, { 0x00D3, "o" },
	{ 0x00D4, "o" }, { 0x00D5, "o" }, { 0x00D6, "o" }, { 0x00D9, "u" },
	{ 0x00DA, "u" }, { 0x00DB, "u" }, { 0x00DC, "u" },
	/* Accented Latin lowercase */
	{ 0x00E0, "a" }, { 0x00E1, "a" }, { 0x00E2, "a" }, { 0x00E3, "a" },
	{ 0x00E4, "a" }, { 0x00E5, "a" }, { 0x00E8, "e" }, { 0x00E9, "e" },
	{ 0x00EA, "e" }, { 0x00EB, "e" }, { 0x00EC, "i" }, { 0x00ED, "i" },
	{ 0x00EE, "i" }, { 0x00EF, "i" }, { 0x00F2, "o" }, { 0x00F3, "o" },
	{ 0x00F4, "o" }, { 0x00F5, "o" }, { 0x00F6, "o" }, { 0x00F9, "u" },
	{ 0x00FA, "u" }, { 0x00FB, "u" }, { 0x00FC, "u" },
	/* Greek uppercase lookalikes */
	{ 0x0391, "a" }, { 0x0392, "b" }, { 0x0395, "e" }, { 0x0396, "z" },
	{ 0x0397, "h" }, { 0x0399, "i" }, { 0x039A, "k" }, { 0x039C, "m" },
	{ 0x039D, "n" }, { 0x039F, "o" }, { 0x03A1, "p" }, { 0x03A4, "t" },
	{ 0x03A5, "y" },
	/* Greek lowercase lookalikes */
	{ 0x03B1, "a" }, { 0x03B5, "e" }, { 0x03B9, "i" }, { 0x03BA, "k" },
	{ 0x03BD, "v" }, { 0x03BF, "o" }, { 0x03C4, "t" },
	/* Cyrillic uppercase lookalikes */
	{ 0x0410, "a" }, { 0x0412, "b" }, { 0x0415, "e" }, { 0x041A, "k" },
	{ 0x041C, "m" }, { 0x041D, "h" }, { 0x041E, "o" }, { 0x0420, "p" },
	{ 0x0421, "c" }, { 0x0422, "t" }, { 0x0423, "y" }, { 0x0425, "x" },
	/* Cyrillic lowercase lookalikes */
	{ 0x0430, "a" }, { 0x0435, "e" }, { 0x043E, "o" }, { 0x0440, "p" },
	{ 0x0441, "c" }, { 0x0443, "y" }, { 0x0445, "x" },
	/* Cyrillic extended lookalikes */
	{ 0x0455, "s" }, { 0x0456, "i" }, { 0x0458, "j" },
	{ 0x0501, "d" }, { 0x051B, "q" }, { 0x051D, "w" },
};
#define CONFUSABLE_SEED_COUNT \
	( (int)( sizeof( confusable_seed ) / sizeof( confusable_seed[0] ) ) )

/*
 * Seed name validation tables if they are empty (first boot).
 * Called during db_game_init() after schema creation.
 */
static void db_game_seed_name_rules( void ) {
	sqlite3_stmt *stmt;
	int count;

	if ( !game_db )
		return;

	/* Check if forbidden_names is empty */
	count = 0;
	if ( sqlite3_prepare_v2( game_db,
			"SELECT COUNT(*) FROM forbidden_names", -1, &stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( stmt, 0 );
		sqlite3_finalize( stmt );
	}

	if ( count == 0 ) {
		static const struct { const char *name; int type; } forbidden_seed[] = {
			/* Reserved words (type 0) */
			{ "all",       0 }, { "wtf",       0 }, { "auto",      0 },
			{ "immortal",  0 }, { "self",      0 }, { "someone",   0 },
			{ "gaia",      0 }, { "none",      0 }, { "save",      0 },
			{ "quit",      0 }, { "why",       0 }, { "who",       0 },
			{ "noone",     0 },
			/* Protected prefixes (type 1) */
			{ "jobo",      1 }, { "dracknuur", 1 }, { "vladd",     1 },
			{ "tarasque",  1 },
		};
		int i, seed_count = (int)( sizeof( forbidden_seed ) / sizeof( forbidden_seed[0] ) );

		if ( sqlite3_prepare_v2( game_db,
				"INSERT INTO forbidden_names (name, type, added_by) VALUES (?,?,'system')",
				-1, &stmt, NULL ) == SQLITE_OK ) {
			for ( i = 0; i < seed_count; i++ ) {
				sqlite3_reset( stmt );
				sqlite3_bind_text( stmt, 1, forbidden_seed[i].name, -1, SQLITE_STATIC );
				sqlite3_bind_int( stmt, 2, forbidden_seed[i].type );
				sqlite3_step( stmt );
			}
			sqlite3_finalize( stmt );
		}
	}

	/* Check if profanity_filters is empty */
	count = 0;
	if ( sqlite3_prepare_v2( game_db,
			"SELECT COUNT(*) FROM profanity_filters", -1, &stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( stmt, 0 );
		sqlite3_finalize( stmt );
	}

	if ( count == 0 ) {
		static const char *profanity_seed[] = { "fuck", "bitch", "whore" };
		int i, seed_count = (int)( sizeof( profanity_seed ) / sizeof( profanity_seed[0] ) );

		if ( sqlite3_prepare_v2( game_db,
				"INSERT INTO profanity_filters (pattern, added_by) VALUES (?,'system')",
				-1, &stmt, NULL ) == SQLITE_OK ) {
			for ( i = 0; i < seed_count; i++ ) {
				sqlite3_reset( stmt );
				sqlite3_bind_text( stmt, 1, profanity_seed[i], -1, SQLITE_STATIC );
				sqlite3_step( stmt );
			}
			sqlite3_finalize( stmt );
		}
	}

	/* Check if confusable_chars is empty */
	count = 0;
	if ( sqlite3_prepare_v2( game_db,
			"SELECT COUNT(*) FROM confusable_chars", -1, &stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( stmt, 0 );
		sqlite3_finalize( stmt );
	}

	if ( count == 0 ) {
		int i;

		if ( sqlite3_prepare_v2( game_db,
				"INSERT INTO confusable_chars (codepoint, canonical) VALUES (?,?)",
				-1, &stmt, NULL ) == SQLITE_OK ) {
			for ( i = 0; i < CONFUSABLE_SEED_COUNT; i++ ) {
				sqlite3_reset( stmt );
				sqlite3_bind_int( stmt, 1, (int) confusable_seed[i].codepoint );
				sqlite3_bind_text( stmt, 2, confusable_seed[i].canonical, -1, SQLITE_STATIC );
				sqlite3_step( stmt );
			}
			sqlite3_finalize( stmt );
		}
	}
}


/*
 * Load forbidden names from game.db into a linked list.
 */
void db_game_load_forbidden_names( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT name, type, added_by FROM forbidden_names ORDER BY id";
	FORBIDDEN_NAME *tail = NULL;
	char buf[256];
	int count = 0;

	if ( !game_db )
		return;

	/* Free existing list */
	while ( forbidden_name_list ) {
		FORBIDDEN_NAME *next = forbidden_name_list->next;
		free(forbidden_name_list->name);
		free(forbidden_name_list->added_by);
		free( forbidden_name_list );
		forbidden_name_list = next;
	}

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		FORBIDDEN_NAME *p = calloc( 1, sizeof( FORBIDDEN_NAME ) );
		if ( !p ) { bug( "db_game_load_forbidden_names: calloc failed", 0 ); exit( 1 ); }
		p->name     = str_dup( col_text( stmt, 0 ) );
		p->type     = sqlite3_column_int( stmt, 1 );
		p->added_by = str_dup( col_text( stmt, 2 ) );
		p->next     = NULL;

		if ( !forbidden_name_list )
			forbidden_name_list = p;
		else
			tail->next = p;
		tail = p;
		count++;
	}

	sqlite3_finalize( stmt );

	snprintf( buf, sizeof( buf ), "  Loaded %d forbidden name rules.", count );
	log_string( buf );
}

/*
 * Save forbidden names from memory to game.db.
 */
void db_game_save_forbidden_names( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	FORBIDDEN_NAME *p;

	db = db_game_open_for_write();
	if ( !db )
		return;

	db_begin( db );
	sqlite3_exec( db, "DELETE FROM forbidden_names", NULL, NULL, NULL );

	if ( sqlite3_prepare_v2( db,
			"INSERT INTO forbidden_names (name, type, added_by) VALUES (?,?,?)",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		db_rollback( db );
		sqlite3_close( db );
		return;
	}

	for ( p = forbidden_name_list; p; p = p->next ) {
		sqlite3_reset( stmt );
		sqlite3_bind_text( stmt, 1, p->name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( stmt, 2, p->type );
		sqlite3_bind_text( stmt, 3, p->added_by, -1, SQLITE_TRANSIENT );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Load profanity filters from game.db into a linked list.
 */
void db_game_load_profanity_filters( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT pattern, added_by FROM profanity_filters ORDER BY id";
	PROFANITY_FILTER *tail = NULL;
	char buf[256];
	int count = 0;

	if ( !game_db )
		return;

	/* Free existing list */
	while ( profanity_filter_list ) {
		PROFANITY_FILTER *next = profanity_filter_list->next;
		free(profanity_filter_list->pattern);
		free(profanity_filter_list->added_by);
		free( profanity_filter_list );
		profanity_filter_list = next;
	}

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		PROFANITY_FILTER *p = calloc( 1, sizeof( PROFANITY_FILTER ) );
		if ( !p ) { bug( "db_game_load_profanity_filters: calloc failed", 0 ); exit( 1 ); }
		p->pattern  = str_dup( col_text( stmt, 0 ) );
		p->added_by = str_dup( col_text( stmt, 1 ) );
		p->next     = NULL;

		if ( !profanity_filter_list )
			profanity_filter_list = p;
		else
			tail->next = p;
		tail = p;
		count++;
	}

	sqlite3_finalize( stmt );

	snprintf( buf, sizeof( buf ), "  Loaded %d profanity filter patterns.", count );
	log_string( buf );
}

/*
 * Save profanity filters from memory to game.db.
 */
void db_game_save_profanity_filters( void ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	PROFANITY_FILTER *p;

	db = db_game_open_for_write();
	if ( !db )
		return;

	db_begin( db );
	sqlite3_exec( db, "DELETE FROM profanity_filters", NULL, NULL, NULL );

	if ( sqlite3_prepare_v2( db,
			"INSERT INTO profanity_filters (pattern, added_by) VALUES (?,?)",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		db_rollback( db );
		sqlite3_close( db );
		return;
	}

	for ( p = profanity_filter_list; p; p = p->next ) {
		sqlite3_reset( stmt );
		sqlite3_bind_text( stmt, 1, p->pattern, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 2, p->added_by, -1, SQLITE_TRANSIENT );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( db );
	sqlite3_close( db );
}


/*
 * Load confusable character mappings from game.db into a sorted array.
 * The array is stored in confusable_table/confusable_count (declared in utf8.h,
 * defined in utf8.c) and used by utf8_skeletonize() for binary search lookups.
 */
void db_game_load_confusables( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT codepoint, canonical FROM confusable_chars "
		"ORDER BY codepoint ASC";
	char buf[256];
	int count = 0, alloc = 0;

	if ( !game_db )
		return;

	/* Free existing table */
	if ( confusable_table != NULL ) {
		free( confusable_table );
		confusable_table = NULL;
		confusable_count = 0;
	}

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	/* Count rows first */
	while ( sqlite3_step( stmt ) == SQLITE_ROW )
		alloc++;
	sqlite3_reset( stmt );

	if ( alloc == 0 ) {
		sqlite3_finalize( stmt );
		return;
	}

	/* Allocate sorted array */
	confusable_table = calloc( 1, alloc * sizeof( CONFUSABLE_ENTRY ) );
	if ( !confusable_table ) { bug( "db_game_load_confusables: calloc failed", 0 ); exit( 1 ); }

	/* Load entries (already sorted by ORDER BY codepoint ASC) */
	while ( sqlite3_step( stmt ) == SQLITE_ROW && count < alloc ) {
		CONFUSABLE_ENTRY *ce = &confusable_table[count];
		const char *can;
		int len;

		ce->codepoint = (unsigned int) sqlite3_column_int( stmt, 0 );
		can = col_text( stmt, 1 );
		len = (int) strlen( can );
		if ( len >= (int) sizeof( ce->canonical ) )
			len = (int) sizeof( ce->canonical ) - 1;
		memcpy( ce->canonical, can, len );
		ce->canonical[len] = '\0';

		count++;
	}

	confusable_count = count;
	sqlite3_finalize( stmt );

	snprintf( buf, sizeof( buf ), "  Loaded %d confusable character mappings.", count );
	log_string( buf );
}
