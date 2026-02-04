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

/* In-memory cache for class display config */
#define MAX_CACHED_BRACKETS     32
#define MAX_CACHED_GENERATIONS  256

static CLASS_BRACKET bracket_cache[MAX_CACHED_BRACKETS];
static int bracket_count = 0;

static CLASS_GENERATION gen_cache[MAX_CACHED_GENERATIONS];
static int gen_count = 0;

/* In-memory cache for class aura config */
#define MAX_CACHED_AURAS        32

static CLASS_AURA aura_cache[MAX_CACHED_AURAS];
static int aura_count = 0;

/* In-memory cache for class armor config */
#define MAX_CACHED_ARMOR_CONFIGS  32
#define MAX_CACHED_ARMOR_PIECES   512

static CLASS_ARMOR_CONFIG armor_config_cache[MAX_CACHED_ARMOR_CONFIGS];
static int armor_config_count = 0;

static CLASS_ARMOR_PIECE armor_piece_cache[MAX_CACHED_ARMOR_PIECES];
static int armor_piece_count = 0;

/* In-memory cache for class starting config */
#define MAX_CACHED_STARTING  32

static CLASS_STARTING starting_cache[MAX_CACHED_STARTING];
static int starting_count = 0;

/* In-memory cache for class score stats */
#define MAX_CACHED_SCORE_STATS  128

static CLASS_SCORE_STAT score_stats_cache[MAX_CACHED_SCORE_STATS];
static int score_stats_count = 0;

/* In-memory cache for class registry */
#define MAX_CACHED_REGISTRY  32

static CLASS_REGISTRY_ENTRY registry_cache[MAX_CACHED_REGISTRY];
static int registry_count = 0;

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
	");"

	/* Class registry must be created first - other class tables reference it */
	"CREATE TABLE IF NOT EXISTS class_registry ("
	"  class_id          INTEGER PRIMARY KEY,"
	"  class_name        TEXT NOT NULL,"
	"  keyword           TEXT NOT NULL UNIQUE,"
	"  keyword_alt       TEXT,"
	"  mudstat_label     TEXT NOT NULL,"
	"  selfclass_message TEXT NOT NULL,"
	"  display_order     INTEGER NOT NULL DEFAULT 0,"
	"  upgrade_class     INTEGER REFERENCES class_registry(class_id),"
	"  requirements      TEXT"
	");"
	"CREATE INDEX IF NOT EXISTS idx_class_keyword ON class_registry(keyword);"
	"CREATE INDEX IF NOT EXISTS idx_class_keyword_alt ON class_registry(keyword_alt);"

	"CREATE TABLE IF NOT EXISTS class_brackets ("
	"  class_id      INTEGER PRIMARY KEY REFERENCES class_registry(class_id),"
	"  class_name    TEXT NOT NULL,"
	"  open_bracket  TEXT NOT NULL,"
	"  close_bracket TEXT NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS class_generations ("
	"  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  class_id      INTEGER NOT NULL REFERENCES class_registry(class_id),"
	"  generation    INTEGER NOT NULL,"
	"  title         TEXT NOT NULL,"
	"  UNIQUE(class_id, generation)"
	");"
	"CREATE INDEX IF NOT EXISTS idx_class_gen ON class_generations(class_id, generation);"

	"CREATE TABLE IF NOT EXISTS class_auras ("
	"  class_id      INTEGER PRIMARY KEY REFERENCES class_registry(class_id),"
	"  aura_text     TEXT NOT NULL,"
	"  mxp_tooltip   TEXT NOT NULL,"
	"  display_order INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS class_armor_config ("
	"  class_id      INTEGER PRIMARY KEY REFERENCES class_registry(class_id),"
	"  acfg_cost_key TEXT NOT NULL,"
	"  usage_message TEXT NOT NULL,"
	"  act_to_char   TEXT NOT NULL DEFAULT '$p appears in your hands.',"
	"  act_to_room   TEXT NOT NULL DEFAULT '$p appears in $n''s hands.'"
	");"

	"CREATE TABLE IF NOT EXISTS class_armor_pieces ("
	"  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  class_id      INTEGER NOT NULL REFERENCES class_registry(class_id),"
	"  keyword       TEXT NOT NULL,"
	"  vnum          INTEGER NOT NULL,"
	"  UNIQUE(class_id, keyword)"
	");"

	"CREATE TABLE IF NOT EXISTS class_starting ("
	"  class_id         INTEGER PRIMARY KEY REFERENCES class_registry(class_id),"
	"  starting_beast   INTEGER NOT NULL DEFAULT 15,"
	"  starting_level   INTEGER NOT NULL DEFAULT 1,"
	"  has_disciplines  INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS class_score_stats ("
	"  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  class_id        INTEGER NOT NULL REFERENCES class_registry(class_id),"
	"  stat_source     INTEGER NOT NULL,"
	"  stat_source_max INTEGER NOT NULL DEFAULT 0,"
	"  stat_label      TEXT NOT NULL,"
	"  format_string   TEXT NOT NULL DEFAULT '#R[#n%s: #C%d#R]\\n\\r',"
	"  display_order   INTEGER NOT NULL DEFAULT 0"
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


/*
 * Class display config (game.db)
 * Brackets and generation names for the who list.
 */

/* Insert default class display entries if tables are empty */
static void class_display_insert_defaults( void ) {
	sqlite3_stmt *count_stmt, *br_stmt, *gen_stmt;
	int count = 0;

	if ( !game_db )
		return;

	/* Check if brackets table already has entries */
	if ( sqlite3_prepare_v2( game_db, "SELECT COUNT(*) FROM class_brackets", -1, &count_stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( count_stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( count_stmt, 0 );
		sqlite3_finalize( count_stmt );
	}

	if ( count > 0 )
		return;  /* Already has data */

	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO class_brackets (class_id, class_name, open_bracket, close_bracket) VALUES (?,?,?,?)",
			-1, &br_stmt, NULL ) != SQLITE_OK )
		return;

	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO class_generations (class_id, generation, title) VALUES (?,?,?)",
			-1, &gen_stmt, NULL ) != SQLITE_OK ) {
		sqlite3_finalize( br_stmt );
		return;
	}

	db_begin( game_db );

#define INSERT_BRACKET( cid, cname, openb, closeb ) \
	sqlite3_reset( br_stmt ); \
	sqlite3_bind_int( br_stmt, 1, cid ); \
	sqlite3_bind_text( br_stmt, 2, cname, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( br_stmt, 3, openb, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( br_stmt, 4, closeb, -1, SQLITE_STATIC ); \
	sqlite3_step( br_stmt )

#define INSERT_GEN( cid, gen, title ) \
	sqlite3_reset( gen_stmt ); \
	sqlite3_bind_int( gen_stmt, 1, cid ); \
	sqlite3_bind_int( gen_stmt, 2, gen ); \
	sqlite3_bind_text( gen_stmt, 3, title, -1, SQLITE_STATIC ); \
	sqlite3_step( gen_stmt )

	/* Brackets - CLASS_DEMON=1, CLASS_MAGE=2, CLASS_WEREWOLF=4, CLASS_VAMPIRE=8, etc. */
	INSERT_BRACKET( 1,      "Demon",         "#0[#n",            "#0]#n" );
	INSERT_BRACKET( 2,      "Mage",          "{{",               "}}" );
	INSERT_BRACKET( 4,      "Werewolf",      "#y((#n",           "#y))#n" );
	INSERT_BRACKET( 8,      "Vampire",       "#R<<#n",           "#R>>#n" );
	INSERT_BRACKET( 16,     "Samurai",       "#C-=#n",           "#C=-#n" );
	INSERT_BRACKET( 32,     "Drow",          "#P.o0",            "#P0o.#n" );
	INSERT_BRACKET( 64,     "Monk",          "#0.x[#n",          "#0]x.#n" );
	INSERT_BRACKET( 128,    "Ninja",         "#C***#n",          "#C***#n" );
	INSERT_BRACKET( 256,    "Lich",          "#G>*<#n",          "#G>*<#n" );
	INSERT_BRACKET( 512,    "Shapeshifter",  "#0[#P*#0]#n",      "#0[#P*#0]#n" );
	INSERT_BRACKET( 1024,   "Tanarri",       "#y{#n",            "#y}#n" );
	INSERT_BRACKET( 2048,   "Angel",         "#y.x#0[#n",        "#0]#yx.#n" );
	INSERT_BRACKET( 4096,   "Undead Knight", "#0|[#n",           "#0]|#n" );
	INSERT_BRACKET( 8192,   "Spider Droid",  "#p{#0-#p}#n",      "#p{#0-#p}#n" );
	INSERT_BRACKET( 16384,  "Dirgesinger",   "#x136~#x178[#n",   "#x178]#x136~#n" );
	INSERT_BRACKET( 32768,  "Siren",         "#x255)#x147(#n",   "#x147)#x255(#n" );
	INSERT_BRACKET( 65536,  "Psion",         "#x255<#x033|#n",   "#x033|#x255>#n" );
	INSERT_BRACKET( 131072, "Mindflayer",    "#x135}#x035{#n",   "#x035}#x135{#n" );

	/* Generation titles - Demon (CLASS_DEMON=1) */
	INSERT_GEN( 1, 1, "#RPit Lord#n" );
	INSERT_GEN( 1, 2, "#RPit Fiend#n" );
	INSERT_GEN( 1, 3, "#RGateKeeper#n" );
	INSERT_GEN( 1, 4, "#RImp Lord#n" );
	INSERT_GEN( 1, 5, "#RHell Spawn#n" );
	INSERT_GEN( 1, 6, "#RDemon#n" );
	INSERT_GEN( 1, 7, "#RSpawnling#n" );
	INSERT_GEN( 1, 0, "#RImp#n" );  /* Default/fallback */

	/* Generation titles - Mage (CLASS_MAGE=2) */
	INSERT_GEN( 2, 1, "#CHigh Archmage#n" );
	INSERT_GEN( 2, 2, "#CArchmage#n" );
	INSERT_GEN( 2, 3, "#CLord of Spells#n" );
	INSERT_GEN( 2, 4, "#CHigh Invoker#n" );
	INSERT_GEN( 2, 5, "#CWizard of War#n" );
	INSERT_GEN( 2, 6, "#CBattlemage#n" );
	INSERT_GEN( 2, 7, "#CApprentice#n" );
	INSERT_GEN( 2, 0, "#CMagician#n" );

	/* Generation titles - Werewolf (CLASS_WEREWOLF=4) */
	INSERT_GEN( 4, 1, "#LChieftain#n" );
	INSERT_GEN( 4, 2, "#LTribe Shaman#n" );
	INSERT_GEN( 4, 3, "#LPack Leader#n" );
	INSERT_GEN( 4, 4, "#LFenris Wolf#n" );
	INSERT_GEN( 4, 5, "#LStalker#n" );
	INSERT_GEN( 4, 6, "#LHunter#n" );
	INSERT_GEN( 4, 7, "#LPup of Gaia#n" );
	INSERT_GEN( 4, 0, "#LHalfbreed Bastard#n" );

	/* Generation titles - Vampire (CLASS_VAMPIRE=8) */
	INSERT_GEN( 8, 1, "#RI#0nner #RC#0ircle#n" );
	INSERT_GEN( 8, 2, "#RV#0ampire #RJ#0usticar#n" );
	INSERT_GEN( 8, 3, "#RV#0ampire #RP#0rince#n" );
	INSERT_GEN( 8, 4, "#RV#0ampire #RA#0ncilla#n" );
	INSERT_GEN( 8, 5, "#RV#0ampire #RA#0rchon#n" );
	INSERT_GEN( 8, 6, "#RV#0ampire#n" );
	INSERT_GEN( 8, 7, "#RV#0ampire #RA#0narch#n" );
	INSERT_GEN( 8, 0, "#RB#0loodsucker#n" );

	/* Generation titles - Samurai (CLASS_SAMURAI=16) */
	INSERT_GEN( 16, 1, "#RSho#ygun#n" );
	INSERT_GEN( 16, 2, "#RKata#yna Mas#Rter#n" );
	INSERT_GEN( 16, 3, "#RSamu#yrai Mas#Rter#n" );
	INSERT_GEN( 16, 4, "#RSamu#yrai El#Rite#n" );
	INSERT_GEN( 16, 5, "#RSamu#yrai#n" );
	INSERT_GEN( 16, 0, "#RTrai#ynee#n" );

	/* Generation titles - Drow (CLASS_DROW=32) */
	INSERT_GEN( 32, 1, "#0Matron Mother#n" );
	INSERT_GEN( 32, 2, "#0Weaponmaster#n" );
	INSERT_GEN( 32, 3, "#0High Priestess#n" );
	INSERT_GEN( 32, 4, "#0Favored of Lloth#n" );
	INSERT_GEN( 32, 5, "#0Black Widow#n" );
	INSERT_GEN( 32, 6, "#0Drow#n" );
	INSERT_GEN( 32, 7, "#0Drow Male#n" );
	INSERT_GEN( 32, 0, "#0Drider#n" );

	/* Generation titles - Monk (CLASS_MONK=64) */
	INSERT_GEN( 64, 1, "#cAbbot#n" );
	INSERT_GEN( 64, 2, "#cHigh Priest#n" );
	INSERT_GEN( 64, 3, "#cPriest#n" );
	INSERT_GEN( 64, 4, "#cFather#n" );
	INSERT_GEN( 64, 5, "#cMonk#n" );
	INSERT_GEN( 64, 6, "#cBrother#n" );
	INSERT_GEN( 64, 7, "#cAcolyte#n" );
	INSERT_GEN( 64, 0, "#cFanatic#n" );

	/* Generation titles - Ninja (CLASS_NINJA=128) */
	INSERT_GEN( 128, 1, "#yMaster Assassin#n" );
	INSERT_GEN( 128, 2, "#yExpert Assassin#n" );
	INSERT_GEN( 128, 3, "#yShadowlord#n" );
	INSERT_GEN( 128, 4, "#yShadow Warrior#n" );
	INSERT_GEN( 128, 5, "#yShadow#n" );
	INSERT_GEN( 128, 6, "#yNinja#n" );
	INSERT_GEN( 128, 7, "#yNinja Apprentice#n" );
	INSERT_GEN( 128, 0, "#yThug#n" );

	/* Generation titles - Lich (CLASS_LICH=256) */
	INSERT_GEN( 256, 1, "#7Demilich#n" );
	INSERT_GEN( 256, 2, "#7Lich Lord#n" );
	INSERT_GEN( 256, 3, "#7Power Lich#n" );
	INSERT_GEN( 256, 4, "#7Ancient Lich#n" );
	INSERT_GEN( 256, 0, "#7Lich#n" );

	/* Generation titles - Shapeshifter (CLASS_SHAPESHIFTER=512) */
	INSERT_GEN( 512, 1, "#RSpawn of Malaug#n" );
	INSERT_GEN( 512, 2, "#RShadowmaster#n" );
	INSERT_GEN( 512, 3, "#RMalaugrym Elder#n" );
	INSERT_GEN( 512, 4, "#RMalaugrym#n" );
	INSERT_GEN( 512, 5, "#RShapeshifter#n" );
	INSERT_GEN( 512, 6, "#RDoppleganger#n" );
	INSERT_GEN( 512, 7, "#RMass of tentacles#n" );
	INSERT_GEN( 512, 0, "#RPlay-Doh#n" );

	/* Generation titles - Tanarri (CLASS_TANARRI=1024) */
	INSERT_GEN( 1024, 1, "#RTanar'ri Balor#n" );
	INSERT_GEN( 1024, 2, "#RTanar'ri Marilith#n" );
	INSERT_GEN( 1024, 3, "#RGreater Tanar'ri#n" );
	INSERT_GEN( 1024, 4, "#RTrue Tanar'ri#n" );
	INSERT_GEN( 1024, 5, "#RTanar'ri#n" );
	INSERT_GEN( 1024, 0, "#RCambion#n" );

	/* Generation titles - Angel (CLASS_ANGEL=2048) */
	INSERT_GEN( 2048, 1, "#7Arch Angel#n" );
	INSERT_GEN( 2048, 2, "#7Cherubim#n" );
	INSERT_GEN( 2048, 3, "#7Seraphim#n" );
	INSERT_GEN( 2048, 4, "#7Guardian Angel#n" );
	INSERT_GEN( 2048, 5, "#7Angel#n" );
	INSERT_GEN( 2048, 0, "#7Nephalim#n" );

	/* Generation titles - Undead Knight (CLASS_UNDEAD_KNIGHT=4096) */
	INSERT_GEN( 4096, 1, "#LFallen Paladin#n" );
	INSERT_GEN( 4096, 2, "#LUndead Lord#n" );
	INSERT_GEN( 4096, 3, "#LUndead Knight#n" );
	INSERT_GEN( 4096, 4, "#LUndead Knight#n" );
	INSERT_GEN( 4096, 5, "#LUndead Knight#n" );
	INSERT_GEN( 4096, 0, "#LSkeleton Knight#n" );

	/* Generation titles - Spider Droid (CLASS_DROID=8192) */
	INSERT_GEN( 8192, 1, "#0Drider Lord#n" );
	INSERT_GEN( 8192, 2, "#0Master of the Web#n" );
	INSERT_GEN( 8192, 0, "#0Spider Droid#n" );

	/* Generation titles - Dirgesinger (CLASS_DIRGESINGER=16384) */
	INSERT_GEN( 16384, 1, "#x178War Cantor#n" );
	INSERT_GEN( 16384, 2, "#x178Battle Bard#n" );
	INSERT_GEN( 16384, 3, "#x178Dirgesinger#n" );
	INSERT_GEN( 16384, 4, "#x178War Chanter#n" );
	INSERT_GEN( 16384, 5, "#x178Chanter#n" );
	INSERT_GEN( 16384, 0, "#x178Hummer#n" );

	/* Generation titles - Siren (CLASS_SIREN=32768) */
	INSERT_GEN( 32768, 1, "#x255Archsiren#n" );
	INSERT_GEN( 32768, 2, "#x255Diva of Doom#n" );
	INSERT_GEN( 32768, 3, "#x255Songweaver#n" );
	INSERT_GEN( 32768, 4, "#x255Voice of Ruin#n" );
	INSERT_GEN( 32768, 0, "#x255Siren#n" );

	/* Generation titles - Psion (CLASS_PSION=65536) */
	INSERT_GEN( 65536, 1, "#CMind Lord#n" );
	INSERT_GEN( 65536, 2, "#CPsychic Master#n" );
	INSERT_GEN( 65536, 3, "#CPsion#n" );
	INSERT_GEN( 65536, 4, "#CMentalist#n" );
	INSERT_GEN( 65536, 5, "#CAdept#n" );
	INSERT_GEN( 65536, 0, "#CAwakened#n" );

	/* Generation titles - Mindflayer (CLASS_MINDFLAYER=131072) */
	INSERT_GEN( 131072, 1, "#GElder Brain#n" );
	INSERT_GEN( 131072, 2, "#GMind Tyrant#n" );
	INSERT_GEN( 131072, 3, "#GIllithid#n" );
	INSERT_GEN( 131072, 4, "#GBrain Eater#n" );
	INSERT_GEN( 131072, 0, "#GMindflayer#n" );

#undef INSERT_BRACKET
#undef INSERT_GEN

	db_commit( game_db );
	sqlite3_finalize( br_stmt );
	sqlite3_finalize( gen_stmt );

	log_string( "  Inserted default class display configuration." );
}

/*
 * Load class display config from database into memory.
 * Call this during boot after db_game_init().
 */
void db_game_load_class_display( void ) {
	sqlite3_stmt *stmt;
	char buf[256];
	int i;

	if ( !game_db )
		return;

	/* Clear existing bracket cache */
	for ( i = 0; i < bracket_count; i++ ) {
		if ( bracket_cache[i].class_name )   free_string( bracket_cache[i].class_name );
		if ( bracket_cache[i].open_bracket ) free_string( bracket_cache[i].open_bracket );
		if ( bracket_cache[i].close_bracket ) free_string( bracket_cache[i].close_bracket );
	}
	bracket_count = 0;

	/* Clear existing generation cache */
	for ( i = 0; i < gen_count; i++ ) {
		if ( gen_cache[i].title ) free_string( gen_cache[i].title );
	}
	gen_count = 0;

	/* Insert defaults if tables are empty */
	class_display_insert_defaults();

	/* Load brackets */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, class_name, open_bracket, close_bracket "
			"FROM class_brackets ORDER BY class_id",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && bracket_count < MAX_CACHED_BRACKETS ) {
			bracket_cache[bracket_count].class_id      = sqlite3_column_int( stmt, 0 );
			bracket_cache[bracket_count].class_name    = str_dup( col_text( stmt, 1 ) );
			bracket_cache[bracket_count].open_bracket  = str_dup( col_text( stmt, 2 ) );
			bracket_cache[bracket_count].close_bracket = str_dup( col_text( stmt, 3 ) );
			bracket_count++;
		}
		sqlite3_finalize( stmt );
	}

	/* Load generations */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, generation, title "
			"FROM class_generations ORDER BY class_id, generation DESC",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && gen_count < MAX_CACHED_GENERATIONS ) {
			gen_cache[gen_count].class_id   = sqlite3_column_int( stmt, 0 );
			gen_cache[gen_count].generation = sqlite3_column_int( stmt, 1 );
			gen_cache[gen_count].title      = str_dup( col_text( stmt, 2 ) );
			gen_count++;
		}
		sqlite3_finalize( stmt );
	}

	snprintf( buf, sizeof( buf ),
		"  Loaded %d class brackets, %d generation titles.", bracket_count, gen_count );
	log_string( buf );
}

/*
 * Get bracket config for a class.
 * Returns NULL if not found (caller should use default brackets).
 */
const CLASS_BRACKET *db_game_get_bracket( int class_id ) {
	int i;

	for ( i = 0; i < bracket_count; i++ ) {
		if ( bracket_cache[i].class_id == class_id )
			return &bracket_cache[i];
	}

	return NULL;
}

/*
 * Get generation title for a class and generation.
 * Returns NULL if not found (caller should use default title).
 *
 * Lookup priority:
 *   1. Exact match for (class_id, generation)
 *   2. Fallback to (class_id, 0) if no exact match
 *   3. NULL if neither found
 */
const char *db_game_get_generation_title( int class_id, int generation ) {
	int i;
	const char *fallback = NULL;

	for ( i = 0; i < gen_count; i++ ) {
		if ( gen_cache[i].class_id == class_id ) {
			if ( gen_cache[i].generation == generation )
				return gen_cache[i].title;
			if ( gen_cache[i].generation == 0 )
				fallback = gen_cache[i].title;
		}
	}

	return fallback;
}


/*
 * Class aura config (game.db)
 * Room aura text displayed when looking at characters.
 */

/* Insert default class aura entries if table is empty */
static void class_aura_insert_defaults( void ) {
	sqlite3_stmt *count_stmt, *aura_stmt;
	int count = 0;

	if ( !game_db )
		return;

	/* Check if auras table already has entries */
	if ( sqlite3_prepare_v2( game_db, "SELECT COUNT(*) FROM class_auras", -1, &count_stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( count_stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( count_stmt, 0 );
		sqlite3_finalize( count_stmt );
	}

	if ( count > 0 )
		return;  /* Already has data */

	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO class_auras (class_id, aura_text, mxp_tooltip, display_order) VALUES (?,?,?,?)",
			-1, &aura_stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

#define INSERT_AURA( cid, text, tooltip, order ) \
	sqlite3_reset( aura_stmt ); \
	sqlite3_bind_int( aura_stmt, 1, cid ); \
	sqlite3_bind_text( aura_stmt, 2, text, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( aura_stmt, 3, tooltip, -1, SQLITE_STATIC ); \
	sqlite3_bind_int( aura_stmt, 4, order ); \
	sqlite3_step( aura_stmt )

	/* Default auras - matching existing hardcoded values from act_info.c */
	INSERT_AURA( 4,      "#y(#LWerewolf#y)#n ",                     "Werewolf",     1 );   /* CLASS_WEREWOLF */
	INSERT_AURA( 1,      "#0(#RDemon#0)#n ",                        "Demon",        2 );   /* CLASS_DEMON */
	INSERT_AURA( 128,    "#R(#yNinja#R)#n ",                        "Ninja",        3 );   /* CLASS_NINJA */
	INSERT_AURA( 64,     "#C(#nMonk#C)#n ",                         "Monk",         4 );   /* CLASS_MONK */
	INSERT_AURA( 8192,   "#p(#PDrider#p)#n ",                       "Spider Droid", 5 );   /* CLASS_DROID */
	INSERT_AURA( 2048,   "#0(#7Angel#0)#n ",                        "Angel",        6 );   /* CLASS_ANGEL */
	INSERT_AURA( 1024,   "#y(#RTanar'ri#y)#n ",                     "Tanar'ri",     7 );   /* CLASS_TANARRI */
	INSERT_AURA( 256,    "#0(#GLich#0)#n ",                         "Lich",         8 );   /* CLASS_LICH */
	INSERT_AURA( 4096,   "#y(#0Death Knight#y)#n ",                 "Death Knight", 9 );   /* CLASS_UNDEAD_KNIGHT */
	INSERT_AURA( 16,     "#C(#ySamu#Rrai#C)#n ",                    "Samurai",      10 );  /* CLASS_SAMURAI */
	INSERT_AURA( 2,      "{{#CBattlemage#n}} ",                     "Battlemage",   11 );  /* CLASS_MAGE */
	INSERT_AURA( 32,     "#P(#0Drow#P)#n ",                         "Drow",         12 );  /* CLASS_DROW */
	INSERT_AURA( 8,      "#R(V#0ampire#R)#n ",                      "Vampire",      13 );  /* CLASS_VAMPIRE */
	INSERT_AURA( 16384,  "#x178(#nDirgesinger#x178)#n ",            "Dirgesinger",  14 );  /* CLASS_DIRGESINGER */
	INSERT_AURA( 32768,  "#x255)#x147(#nSiren#x147)#x255(#n ",      "Siren",        15 );  /* CLASS_SIREN */
	INSERT_AURA( 65536,  "#x255<#x033|#nPsion#x033|#x255>#n ",      "Psion",        16 );  /* CLASS_PSION */
	INSERT_AURA( 131072, "#x135}#x035{#nMindflayer#x035}#x135{#n ", "Mindflayer",   17 );  /* CLASS_MINDFLAYER */

#undef INSERT_AURA

	db_commit( game_db );
	sqlite3_finalize( aura_stmt );

	log_string( "  Inserted default class aura configuration." );
}

/*
 * Load class aura config from database into memory.
 * Call this during boot after db_game_init().
 */
void db_game_load_class_aura( void ) {
	sqlite3_stmt *stmt;
	char buf[256];
	int i;

	if ( !game_db )
		return;

	/* Clear existing aura cache */
	for ( i = 0; i < aura_count; i++ ) {
		if ( aura_cache[i].aura_text )   free_string( aura_cache[i].aura_text );
		if ( aura_cache[i].mxp_tooltip ) free_string( aura_cache[i].mxp_tooltip );
	}
	aura_count = 0;

	/* Insert defaults if table is empty */
	class_aura_insert_defaults();

	/* Load auras ordered by display_order */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, aura_text, mxp_tooltip, display_order "
			"FROM class_auras ORDER BY display_order, class_id",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && aura_count < MAX_CACHED_AURAS ) {
			aura_cache[aura_count].class_id      = sqlite3_column_int( stmt, 0 );
			aura_cache[aura_count].aura_text     = str_dup( col_text( stmt, 1 ) );
			aura_cache[aura_count].mxp_tooltip   = str_dup( col_text( stmt, 2 ) );
			aura_cache[aura_count].display_order = sqlite3_column_int( stmt, 3 );
			aura_count++;
		}
		sqlite3_finalize( stmt );
	}

	snprintf( buf, sizeof( buf ), "  Loaded %d class auras.", aura_count );
	log_string( buf );
}

/*
 * Get aura config for a class.
 * Returns NULL if not found.
 */
const CLASS_AURA *db_game_get_aura( int class_id ) {
	int i;

	for ( i = 0; i < aura_count; i++ ) {
		if ( aura_cache[i].class_id == class_id )
			return &aura_cache[i];
	}

	return NULL;
}

/*
 * Get total number of loaded auras (for iteration).
 */
int db_game_get_aura_count( void ) {
	return aura_count;
}

/*
 * Get aura by index (for iteration in display order).
 * Returns NULL if index out of range.
 */
const CLASS_AURA *db_game_get_aura_by_index( int index ) {
	if ( index < 0 || index >= aura_count )
		return NULL;

	return &aura_cache[index];
}


/*
 * Class armor config (game.db)
 * Armor piece definitions for class equipment creation commands.
 */

/* Insert default class armor entries if tables are empty */
static void class_armor_insert_defaults( void ) {
	sqlite3_stmt *count_stmt, *cfg_stmt, *piece_stmt;
	int count = 0;

	if ( !game_db )
		return;

	/* Check if config table already has entries */
	if ( sqlite3_prepare_v2( game_db, "SELECT COUNT(*) FROM class_armor_config", -1, &count_stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( count_stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( count_stmt, 0 );
		sqlite3_finalize( count_stmt );
	}

	if ( count > 0 )
		return;  /* Already has data */

	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO class_armor_config (class_id, acfg_cost_key, usage_message, act_to_char, act_to_room) VALUES (?,?,?,?,?)",
			-1, &cfg_stmt, NULL ) != SQLITE_OK )
		return;

	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO class_armor_pieces (class_id, keyword, vnum) VALUES (?,?,?)",
			-1, &piece_stmt, NULL ) != SQLITE_OK ) {
		sqlite3_finalize( cfg_stmt );
		return;
	}

	db_begin( game_db );

#define INSERT_CONFIG( cid, cost_key, usage, to_char, to_room ) \
	sqlite3_reset( cfg_stmt ); \
	sqlite3_bind_int( cfg_stmt, 1, cid ); \
	sqlite3_bind_text( cfg_stmt, 2, cost_key, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( cfg_stmt, 3, usage, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( cfg_stmt, 4, to_char, -1, SQLITE_STATIC ); \
	sqlite3_bind_text( cfg_stmt, 5, to_room, -1, SQLITE_STATIC ); \
	sqlite3_step( cfg_stmt )

#define INSERT_PIECE( cid, kw, v ) \
	sqlite3_reset( piece_stmt ); \
	sqlite3_bind_int( piece_stmt, 1, cid ); \
	sqlite3_bind_text( piece_stmt, 2, kw, -1, SQLITE_STATIC ); \
	sqlite3_bind_int( piece_stmt, 3, v ); \
	sqlite3_step( piece_stmt )

	/* Mage armor (CLASS_MAGE=2) */
	INSERT_CONFIG( 2, "mage.magearmor.practice_cost",
		"Please specify which piece of mage armor you wish to make: Dagger Staff Ring Collar Robe Cap Leggings Boots Gloves Sleeves Cape Belt Bracer Mask.",
		"$p appears in your hands.", "$p appears in $n's hands." );
	INSERT_PIECE( 2, "staff",     33000 );
	INSERT_PIECE( 2, "dagger",    33001 );
	INSERT_PIECE( 2, "ring",      33002 );
	INSERT_PIECE( 2, "collar",    33003 );
	INSERT_PIECE( 2, "robe",      33004 );
	INSERT_PIECE( 2, "cap",       33005 );
	INSERT_PIECE( 2, "leggings",  33006 );
	INSERT_PIECE( 2, "boots",     33007 );
	INSERT_PIECE( 2, "gloves",    33008 );
	INSERT_PIECE( 2, "sleeves",   33009 );
	INSERT_PIECE( 2, "cape",      33010 );
	INSERT_PIECE( 2, "belt",      33011 );
	INSERT_PIECE( 2, "bracer",    33012 );
	INSERT_PIECE( 2, "mask",      33013 );

	/* Vampire armor (CLASS_VAMPIRE=8) */
	INSERT_CONFIG( 8, "vampire.vampirearmor.practice_cost",
		"Please specify which piece of vampire armor you wish to make: Ring Collar Plate Helmet Leggings Boots Gloves Sleeves Cape Belt Bracer Visor Dagger Longsword.",
		"$p appears in your hands.", "$p appears in $n's hands." );
	INSERT_PIECE( 8, "longsword", 33040 );
	INSERT_PIECE( 8, "dagger",    33041 );
	INSERT_PIECE( 8, "ring",      33042 );
	INSERT_PIECE( 8, "collar",    33043 );
	INSERT_PIECE( 8, "plate",     33044 );
	INSERT_PIECE( 8, "helmet",    33045 );
	INSERT_PIECE( 8, "leggings",  33046 );
	INSERT_PIECE( 8, "boots",     33047 );
	INSERT_PIECE( 8, "gloves",    33048 );
	INSERT_PIECE( 8, "sleeves",   33049 );
	INSERT_PIECE( 8, "cape",      33050 );
	INSERT_PIECE( 8, "belt",      33051 );
	INSERT_PIECE( 8, "bracer",    33052 );
	INSERT_PIECE( 8, "visor",     33053 );

	/* Monk armor (CLASS_MONK=64) */
	INSERT_CONFIG( 64, "monk.monkarmor.primal_cost",
		"Please specify which piece of monk armor you wish to make: Ring Collar Robe Helmet Shorts Boots Gloves Sleeves Cloak Belt Bracer Mask.",
		"$p appears in your hands.", "$p appears in $n's hands." );
	INSERT_PIECE( 64, "ring",     33020 );
	INSERT_PIECE( 64, "collar",   33021 );
	INSERT_PIECE( 64, "robe",     33022 );
	INSERT_PIECE( 64, "shorts",   33023 );
	INSERT_PIECE( 64, "helmet",   33024 );
	INSERT_PIECE( 64, "gloves",   33025 );
	INSERT_PIECE( 64, "sleeves",  33026 );
	INSERT_PIECE( 64, "cloak",    33027 );
	INSERT_PIECE( 64, "belt",     33028 );
	INSERT_PIECE( 64, "bracer",   33029 );
	INSERT_PIECE( 64, "mask",     33030 );
	INSERT_PIECE( 64, "boots",    33031 );

	/* Ninja armor (CLASS_NINJA=128) */
	INSERT_CONFIG( 128, "ninja.ninjaarmor.primal_cost",
		"Please specify which piece of ninja eq you wish to make: Ring Collar Robe Cap Leggings Boots Gloves Sleeves Cloak Belt Bracer Mask Sword Dagger.",
		"You make $p from the shadows.", "$n forms $p from the shadows." );
	INSERT_PIECE( 128, "dagger",   33080 );
	INSERT_PIECE( 128, "sword",    33081 );
	INSERT_PIECE( 128, "ring",     33082 );
	INSERT_PIECE( 128, "collar",   33083 );
	INSERT_PIECE( 128, "bracer",   33084 );
	INSERT_PIECE( 128, "robe",     33085 );
	INSERT_PIECE( 128, "cap",      33086 );
	INSERT_PIECE( 128, "leggings", 33087 );
	INSERT_PIECE( 128, "boots",    33088 );
	INSERT_PIECE( 128, "sleeves",  33089 );
	INSERT_PIECE( 128, "cloak",    33090 );
	INSERT_PIECE( 128, "gloves",   33091 );
	INSERT_PIECE( 128, "belt",     33092 );
	INSERT_PIECE( 128, "mask",     33093 );

	/* Lich armor (CLASS_LICH=256) */
	INSERT_CONFIG( 256, "lich.licharmor.practice_cost",
		"Please specify which piece of angel armor you wish to make: Scythe Bracer Amulet Ring Plate Helmet Leggings Boots Gauntlets Sleeves Cloak Belt Mask.",
		"$p appears in your hands.", "$p appears in $n's hands." );
	INSERT_PIECE( 256, "scythe",    33220 );
	INSERT_PIECE( 256, "ring",      33221 );
	INSERT_PIECE( 256, "bracer",    33222 );
	INSERT_PIECE( 256, "amulet",    33223 );
	INSERT_PIECE( 256, "plate",     33224 );
	INSERT_PIECE( 256, "helmet",    33225 );
	INSERT_PIECE( 256, "belt",      33226 );
	INSERT_PIECE( 256, "mask",      33227 );
	INSERT_PIECE( 256, "gauntlets", 33228 );
	INSERT_PIECE( 256, "sleeves",   33229 );
	INSERT_PIECE( 256, "boots",     33230 );
	INSERT_PIECE( 256, "leggings",  33231 );
	INSERT_PIECE( 256, "cloak",     33232 );

	/* Shapeshifter armor (CLASS_SHAPESHIFTER=512) */
	INSERT_CONFIG( 512, "shapeshifter.shapearmor.primal_cost",
		"Please specify which piece of shapeshifter armor you wish to make: Knife Kane Bands Necklace Ring Jacket Helmet Pants Boots Gloves Shirt Cloak Belt Visor.",
		"$p appears in your hands.", "$p appears in $n's hands." );
	INSERT_PIECE( 512, "knife",    33160 );
	INSERT_PIECE( 512, "kane",     33161 );
	INSERT_PIECE( 512, "bands",    33162 );
	INSERT_PIECE( 512, "necklace", 33163 );
	INSERT_PIECE( 512, "ring",     33164 );
	INSERT_PIECE( 512, "jacket",   33165 );
	INSERT_PIECE( 512, "helmet",   33166 );
	INSERT_PIECE( 512, "pants",    33167 );
	INSERT_PIECE( 512, "boots",    33168 );
	INSERT_PIECE( 512, "gloves",   33169 );
	INSERT_PIECE( 512, "shirt",    33170 );
	INSERT_PIECE( 512, "cloak",    33171 );
	INSERT_PIECE( 512, "belt",     33172 );
	INSERT_PIECE( 512, "visor",    33173 );

	/* Angel armor (CLASS_ANGEL=2048) - uses hardcoded cost 150 */
	INSERT_CONFIG( 2048, "angel.angelicarmor.primal_cost",
		"Please specify which piece of angel armor you wish to make: Sword Bracer Necklace Ring Plate Helmet Leggings Boots Gauntlets Sleeves Cloak Belt Visor.",
		"$p appears in your hands.", "$p appears in $n's hands." );
	INSERT_PIECE( 2048, "ring",      33180 );
	INSERT_PIECE( 2048, "bracer",    33181 );
	INSERT_PIECE( 2048, "necklace",  33182 );
	INSERT_PIECE( 2048, "belt",      33183 );
	INSERT_PIECE( 2048, "helmet",    33184 );
	INSERT_PIECE( 2048, "cloak",     33185 );
	INSERT_PIECE( 2048, "visor",     33186 );
	INSERT_PIECE( 2048, "plate",     33187 );
	INSERT_PIECE( 2048, "leggings",  33188 );
	INSERT_PIECE( 2048, "boots",     33189 );
	INSERT_PIECE( 2048, "gauntlets", 33190 );
	INSERT_PIECE( 2048, "sleeves",   33191 );
	INSERT_PIECE( 2048, "sword",     33192 );

	/* Undead Knight armor (CLASS_UNDEAD_KNIGHT=4096) */
	INSERT_CONFIG( 4096, "undead_knight.knightarmor.primal_cost",
		"Please specify which piece of unholy armor you wish to make: plate ring bracer collar helmet leggings boots gauntlets chains cloak belt visor longsword shortsword.",
		"$p appears in your hands.", "$p appears in $n's hands." );
	INSERT_PIECE( 4096, "plate",      29975 );
	INSERT_PIECE( 4096, "longsword",  29976 );
	INSERT_PIECE( 4096, "shortsword", 29977 );
	INSERT_PIECE( 4096, "ring",       29978 );
	INSERT_PIECE( 4096, "bracer",     29979 );
	INSERT_PIECE( 4096, "collar",     29980 );
	INSERT_PIECE( 4096, "helmet",     29981 );
	INSERT_PIECE( 4096, "leggings",   29982 );
	INSERT_PIECE( 4096, "boots",      29983 );
	INSERT_PIECE( 4096, "gauntlets",  29984 );
	INSERT_PIECE( 4096, "chains",     29985 );
	INSERT_PIECE( 4096, "cloak",      29986 );
	INSERT_PIECE( 4096, "belt",       29987 );
	INSERT_PIECE( 4096, "visor",      29988 );

	/* Dirgesinger armor (CLASS_DIRGESINGER=16384) */
	INSERT_CONFIG( 16384, "dirgesinger.armor.primal_cost",
		"Please specify which piece of armor to create.\nOptions: warhorn ring collar battleplate warhelm greaves warboots gauntlets vambraces warcape belt bracer warmask",
		"You shape sonic energy into $p!", "$n shapes sonic energy into $p!" );
	INSERT_PIECE( 16384, "warhorn",     33320 );
	INSERT_PIECE( 16384, "ring",        33321 );
	INSERT_PIECE( 16384, "collar",      33322 );
	INSERT_PIECE( 16384, "battleplate", 33323 );
	INSERT_PIECE( 16384, "warhelm",     33324 );
	INSERT_PIECE( 16384, "greaves",     33325 );
	INSERT_PIECE( 16384, "warboots",    33326 );
	INSERT_PIECE( 16384, "gauntlets",   33327 );
	INSERT_PIECE( 16384, "vambraces",   33328 );
	INSERT_PIECE( 16384, "warcape",     33329 );
	INSERT_PIECE( 16384, "belt",        33330 );
	INSERT_PIECE( 16384, "bracer",      33331 );
	INSERT_PIECE( 16384, "warmask",     33332 );

	/* Siren armor (CLASS_SIREN=32768) - uses hardcoded cost 150 */
	INSERT_CONFIG( 32768, "siren.sirenarmor.primal_cost",
		"Please specify which piece of siren armor to create.\nOptions: scepter ring choker gown diadem greaves slippers gloves armlets mantle sash bangle veil",
		"You weave a melody that materializes into $p!", "$n weaves a melody that materializes into $p!" );
	INSERT_PIECE( 32768, "scepter",  33340 );
	INSERT_PIECE( 32768, "ring",     33341 );
	INSERT_PIECE( 32768, "choker",   33342 );
	INSERT_PIECE( 32768, "gown",     33343 );
	INSERT_PIECE( 32768, "diadem",   33344 );
	INSERT_PIECE( 32768, "greaves",  33345 );
	INSERT_PIECE( 32768, "slippers", 33346 );
	INSERT_PIECE( 32768, "gloves",   33347 );
	INSERT_PIECE( 32768, "armlets",  33348 );
	INSERT_PIECE( 32768, "mantle",   33349 );
	INSERT_PIECE( 32768, "sash",     33350 );
	INSERT_PIECE( 32768, "bangle",   33351 );
	INSERT_PIECE( 32768, "veil",     33352 );

	/* Psion armor (CLASS_PSION=65536) - uses hardcoded cost 60 */
	INSERT_CONFIG( 65536, "psion.psionarmor.primal_cost",
		"#x033~#x039[#n Psion Equipment #x039]#x033~#n\nUsage: psionarmor <piece>\nAvailable: focus ring amulet robe circlet leggings sandals gloves bracers cloak sash wristband gem\nCost: 60 primal per piece",
		"You create $p with your psychic power.", "$n creates $p with psychic power." );
	INSERT_PIECE( 65536, "focus",     33360 );
	INSERT_PIECE( 65536, "ring",      33361 );
	INSERT_PIECE( 65536, "amulet",    33362 );
	INSERT_PIECE( 65536, "robe",      33363 );
	INSERT_PIECE( 65536, "circlet",   33364 );
	INSERT_PIECE( 65536, "leggings",  33365 );
	INSERT_PIECE( 65536, "sandals",   33366 );
	INSERT_PIECE( 65536, "gloves",    33367 );
	INSERT_PIECE( 65536, "bracers",   33368 );
	INSERT_PIECE( 65536, "cloak",     33369 );
	INSERT_PIECE( 65536, "sash",      33370 );
	INSERT_PIECE( 65536, "wristband", 33371 );
	INSERT_PIECE( 65536, "gem",       33372 );

	/* Mindflayer armor (CLASS_MINDFLAYER=131072) - uses hardcoded cost 150 */
	INSERT_CONFIG( 131072, "mindflayer.mindflayerarmor.primal_cost",
		"#x029~#x035[#n Mindflayer Equipment #x035]#x029~#n\nUsage: mindflayerarmor <piece>\nAvailable: scepter ring collar robes crown leggings sandals gloves vambraces shroud sash bangle lens\nCost: 150 primal per piece",
		"You create $p with your psychic power.", "$n creates $p with psychic power." );
	INSERT_PIECE( 131072, "scepter",   33380 );
	INSERT_PIECE( 131072, "ring",      33381 );
	INSERT_PIECE( 131072, "collar",    33382 );
	INSERT_PIECE( 131072, "robes",     33383 );
	INSERT_PIECE( 131072, "crown",     33384 );
	INSERT_PIECE( 131072, "leggings",  33385 );
	INSERT_PIECE( 131072, "sandals",   33386 );
	INSERT_PIECE( 131072, "gloves",    33387 );
	INSERT_PIECE( 131072, "vambraces", 33388 );
	INSERT_PIECE( 131072, "shroud",    33389 );
	INSERT_PIECE( 131072, "sash",      33390 );
	INSERT_PIECE( 131072, "bangle",    33391 );
	INSERT_PIECE( 131072, "lens",      33392 );

#undef INSERT_CONFIG
#undef INSERT_PIECE

	db_commit( game_db );
	sqlite3_finalize( cfg_stmt );
	sqlite3_finalize( piece_stmt );

	log_string( "  Inserted default class armor configuration." );
}

/*
 * Load class armor config from database into memory.
 * Call this during boot after db_game_init().
 */
void db_game_load_class_armor( void ) {
	sqlite3_stmt *stmt;
	char buf[256];
	int i;

	if ( !game_db )
		return;

	/* Clear existing config cache */
	for ( i = 0; i < armor_config_count; i++ ) {
		if ( armor_config_cache[i].acfg_cost_key ) free_string( armor_config_cache[i].acfg_cost_key );
		if ( armor_config_cache[i].usage_message ) free_string( armor_config_cache[i].usage_message );
		if ( armor_config_cache[i].act_to_char )   free_string( armor_config_cache[i].act_to_char );
		if ( armor_config_cache[i].act_to_room )   free_string( armor_config_cache[i].act_to_room );
	}
	armor_config_count = 0;

	/* Clear existing piece cache */
	for ( i = 0; i < armor_piece_count; i++ ) {
		if ( armor_piece_cache[i].keyword ) free_string( armor_piece_cache[i].keyword );
	}
	armor_piece_count = 0;

	/* Insert defaults if tables are empty */
	class_armor_insert_defaults();

	/* Load armor configs */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, acfg_cost_key, usage_message, act_to_char, act_to_room "
			"FROM class_armor_config ORDER BY class_id",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && armor_config_count < MAX_CACHED_ARMOR_CONFIGS ) {
			armor_config_cache[armor_config_count].class_id      = sqlite3_column_int( stmt, 0 );
			armor_config_cache[armor_config_count].acfg_cost_key = str_dup( col_text( stmt, 1 ) );
			armor_config_cache[armor_config_count].usage_message = str_dup( col_text( stmt, 2 ) );
			armor_config_cache[armor_config_count].act_to_char   = str_dup( col_text( stmt, 3 ) );
			armor_config_cache[armor_config_count].act_to_room   = str_dup( col_text( stmt, 4 ) );
			armor_config_count++;
		}
		sqlite3_finalize( stmt );
	}

	/* Load armor pieces */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, keyword, vnum "
			"FROM class_armor_pieces ORDER BY class_id, keyword",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && armor_piece_count < MAX_CACHED_ARMOR_PIECES ) {
			armor_piece_cache[armor_piece_count].class_id = sqlite3_column_int( stmt, 0 );
			armor_piece_cache[armor_piece_count].keyword  = str_dup( col_text( stmt, 1 ) );
			armor_piece_cache[armor_piece_count].vnum     = sqlite3_column_int( stmt, 2 );
			armor_piece_count++;
		}
		sqlite3_finalize( stmt );
	}

	snprintf( buf, sizeof( buf ),
		"  Loaded %d armor configs, %d armor pieces.", armor_config_count, armor_piece_count );
	log_string( buf );
}

/*
 * Get armor config for a class.
 * Returns NULL if not found.
 */
const CLASS_ARMOR_CONFIG *db_game_get_armor_config( int class_id ) {
	int i;

	for ( i = 0; i < armor_config_count; i++ ) {
		if ( armor_config_cache[i].class_id == class_id )
			return &armor_config_cache[i];
	}

	return NULL;
}

/*
 * Get armor piece vnum for a class and keyword.
 * Returns 0 if not found.
 */
int db_game_get_armor_vnum( int class_id, const char *keyword ) {
	int i;

	for ( i = 0; i < armor_piece_count; i++ ) {
		if ( armor_piece_cache[i].class_id == class_id &&
		     !str_cmp( armor_piece_cache[i].keyword, keyword ) )
			return armor_piece_cache[i].vnum;
	}

	return 0;
}


/***************************************************************************
 * Class starting config (game.db)
 * Starting values for class selection (beast, level, disciplines).
 ***************************************************************************/

/* Insert default class starting entries if table is empty */
static void class_starting_insert_defaults( void ) {
	sqlite3_stmt *count_stmt, *stmt;
	int count = 0;

	if ( !game_db )
		return;

	/* Check if table already has entries */
	if ( sqlite3_prepare_v2( game_db, "SELECT COUNT(*) FROM class_starting", -1, &count_stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( count_stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( count_stmt, 0 );
		sqlite3_finalize( count_stmt );
	}

	if ( count > 0 )
		return;  /* Already has data */

	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO class_starting (class_id, starting_beast, starting_level, has_disciplines) VALUES (?,?,?,?)",
			-1, &stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

#define INSERT_STARTING( cid, beast, level, disc ) \
	sqlite3_reset( stmt ); \
	sqlite3_bind_int( stmt, 1, cid ); \
	sqlite3_bind_int( stmt, 2, beast ); \
	sqlite3_bind_int( stmt, 3, level ); \
	sqlite3_bind_int( stmt, 4, disc ); \
	sqlite3_step( stmt )

	/* Vampire: beast=30, has disciplines */
	INSERT_STARTING( 8, 30, 1, 1 );     /* CLASS_VAMPIRE=8 */

	/* Werewolf: has disciplines */
	INSERT_STARTING( 4, 15, 1, 1 );     /* CLASS_WEREWOLF=4 */

	/* Demon: has disciplines */
	INSERT_STARTING( 1, 15, 1, 1 );     /* CLASS_DEMON=1 */

	/* Monk: starts at level 3 */
	INSERT_STARTING( 64, 15, 3, 0 );    /* CLASS_MONK=64 */

	/* Mage: starts at level 3 */
	INSERT_STARTING( 2, 15, 3, 0 );     /* CLASS_MAGE=2 */

	/* Standard classes (no special values) */
	INSERT_STARTING( 16, 15, 1, 0 );    /* CLASS_SAMURAI=16 */
	INSERT_STARTING( 32, 15, 1, 0 );    /* CLASS_DROW=32 */
	INSERT_STARTING( 128, 15, 1, 0 );   /* CLASS_NINJA=128 */
	INSERT_STARTING( 256, 15, 1, 0 );   /* CLASS_LICH=256 */
	INSERT_STARTING( 512, 15, 1, 0 );   /* CLASS_SHAPESHIFTER=512 */
	INSERT_STARTING( 1024, 15, 1, 0 );  /* CLASS_TANARRI=1024 */
	INSERT_STARTING( 2048, 15, 1, 0 );  /* CLASS_ANGEL=2048 */
	INSERT_STARTING( 4096, 15, 1, 0 );  /* CLASS_UNDEAD_KNIGHT=4096 */
	INSERT_STARTING( 8192, 15, 1, 0 );  /* CLASS_DROID=8192 */
	INSERT_STARTING( 16384, 15, 1, 0 ); /* CLASS_DIRGESINGER=16384 */
	INSERT_STARTING( 32768, 15, 1, 0 ); /* CLASS_SIREN=32768 */
	INSERT_STARTING( 65536, 15, 1, 0 ); /* CLASS_PSION=65536 */
	INSERT_STARTING( 131072, 15, 1, 0 ); /* CLASS_MINDFLAYER=131072 */

#undef INSERT_STARTING

	db_commit( game_db );
	sqlite3_finalize( stmt );

	log_string( "  Inserted default class starting configuration." );
}

/*
 * Load class starting config from database into memory.
 * Call this during boot after db_game_init().
 */
void db_game_load_class_starting( void ) {
	sqlite3_stmt *stmt;
	char buf[256];

	if ( !game_db )
		return;

	/* Clear existing cache */
	starting_count = 0;

	/* Insert defaults if table is empty */
	class_starting_insert_defaults();

	/* Load starting configs */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, starting_beast, starting_level, has_disciplines "
			"FROM class_starting ORDER BY class_id",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && starting_count < MAX_CACHED_STARTING ) {
			starting_cache[starting_count].class_id        = sqlite3_column_int( stmt, 0 );
			starting_cache[starting_count].starting_beast  = sqlite3_column_int( stmt, 1 );
			starting_cache[starting_count].starting_level  = sqlite3_column_int( stmt, 2 );
			starting_cache[starting_count].has_disciplines = sqlite3_column_int( stmt, 3 ) != 0;
			starting_count++;
		}
		sqlite3_finalize( stmt );
	}

	snprintf( buf, sizeof( buf ), "  Loaded %d class starting configs.", starting_count );
	log_string( buf );
}

/*
 * Get starting config for a class.
 * Returns NULL if not found.
 */
const CLASS_STARTING *db_game_get_starting( int class_id ) {
	int i;

	for ( i = 0; i < starting_count; i++ ) {
		if ( starting_cache[i].class_id == class_id )
			return &starting_cache[i];
	}

	return NULL;
}

/* --------------------------------------------------------------------------
 * Class Score Stats (class_score_stats table)
 * -------------------------------------------------------------------------- */

/*
 * Insert default score stats if table is empty.
 */
static void class_score_stats_insert_defaults( void ) {
	sqlite3_stmt *count_stmt = NULL;
	sqlite3_stmt *insert_stmt = NULL;
	int count = 0;

	/* Check if table is empty */
	if ( sqlite3_prepare_v2( game_db, "SELECT COUNT(*) FROM class_score_stats", -1, &count_stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( count_stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( count_stmt, 0 );
		sqlite3_finalize( count_stmt );
	}

	if ( count > 0 )
		return;  /* Table has data, don't insert defaults */

	/* Insert default score stats for each class */
	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO class_score_stats (class_id, stat_source, stat_source_max, stat_label, format_string, display_order) "
			"VALUES (?,?,?,?,?,?)",
			-1, &insert_stmt, NULL ) != SQLITE_OK )
		return;

	/* Vampire: beast */
	sqlite3_bind_int( insert_stmt, 1, CLASS_VAMPIRE );
	sqlite3_bind_int( insert_stmt, 2, STAT_BEAST );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Your current beast is", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s: #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 10 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Monk: block counter */
	sqlite3_bind_int( insert_stmt, 1, CLASS_MONK );
	sqlite3_bind_int( insert_stmt, 2, STAT_MONKBLOCK );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Your block counter is currently", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s: #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 10 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Monk: chi current */
	sqlite3_bind_int( insert_stmt, 1, CLASS_MONK );
	sqlite3_bind_int( insert_stmt, 2, STAT_CHI_CURRENT );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Your current level of chi", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s:       #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 20 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Monk: chi maximum */
	sqlite3_bind_int( insert_stmt, 1, CLASS_MONK );
	sqlite3_bind_int( insert_stmt, 2, STAT_CHI_MAXIMUM );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Your maximum level of chi", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s:       #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 30 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Werewolf: gnosis current */
	sqlite3_bind_int( insert_stmt, 1, CLASS_WEREWOLF );
	sqlite3_bind_int( insert_stmt, 2, STAT_GNOSIS_CURRENT );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Current Gnosis", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s:            #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 10 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Werewolf: gnosis maximum */
	sqlite3_bind_int( insert_stmt, 1, CLASS_WEREWOLF );
	sqlite3_bind_int( insert_stmt, 2, STAT_GNOSIS_MAXIMUM );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Maximum gnosis", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s:            #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 20 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Werewolf: silver tolerance */
	sqlite3_bind_int( insert_stmt, 1, CLASS_WEREWOLF );
	sqlite3_bind_int( insert_stmt, 2, STAT_SILTOL );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "You have attained", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s #C%d #npoints of silver tolerance#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 30 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Droid: class points */
	sqlite3_bind_int( insert_stmt, 1, CLASS_DROID );
	sqlite3_bind_int( insert_stmt, 2, STAT_DROID_POWER );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "You have", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s #C%d #nclass points stored#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 10 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Tanarri: class points */
	sqlite3_bind_int( insert_stmt, 1, CLASS_TANARRI );
	sqlite3_bind_int( insert_stmt, 2, STAT_TPOINTS );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "You have", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s #C%d #nclass points stored#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 10 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* NOTE: Demon souls is NOT included - it has generation condition in code */

	/* Drow: class points */
	sqlite3_bind_int( insert_stmt, 1, CLASS_DROW );
	sqlite3_bind_int( insert_stmt, 2, STAT_DROW_POWER );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "You have", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s #C%d#n class points stored#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 10 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Drow: magic resistance */
	sqlite3_bind_int( insert_stmt, 1, CLASS_DROW );
	sqlite3_bind_int( insert_stmt, 2, STAT_DROW_MAGIC );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "You have", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s #C%d #npoints of magic resistance#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 20 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Shapeshifter: counter */
	sqlite3_bind_int( insert_stmt, 1, CLASS_SHAPESHIFTER );
	sqlite3_bind_int( insert_stmt, 2, STAT_SHAPE_COUNTER );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Your shapeshifter counter is", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s : #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 10 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Shapeshifter: phase counter */
	sqlite3_bind_int( insert_stmt, 1, CLASS_SHAPESHIFTER );
	sqlite3_bind_int( insert_stmt, 2, STAT_PHASE_COUNTER );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Your phase counter is", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s        : #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 20 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Angel: justice */
	sqlite3_bind_int( insert_stmt, 1, CLASS_ANGEL );
	sqlite3_bind_int( insert_stmt, 2, STAT_ANGEL_JUSTICE );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Angelic Justice", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s   : #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 10 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Angel: love */
	sqlite3_bind_int( insert_stmt, 1, CLASS_ANGEL );
	sqlite3_bind_int( insert_stmt, 2, STAT_ANGEL_LOVE );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Angelic Love", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s      : #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 20 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Angel: harmony */
	sqlite3_bind_int( insert_stmt, 1, CLASS_ANGEL );
	sqlite3_bind_int( insert_stmt, 2, STAT_ANGEL_HARMONY );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Angelic Harmony", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s   : #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 30 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	/* Angel: peace */
	sqlite3_bind_int( insert_stmt, 1, CLASS_ANGEL );
	sqlite3_bind_int( insert_stmt, 2, STAT_ANGEL_PEACE );
	sqlite3_bind_int( insert_stmt, 3, 0 );
	sqlite3_bind_text( insert_stmt, 4, "Angelic Peace", -1, SQLITE_STATIC );
	sqlite3_bind_text( insert_stmt, 5, "#R[#n%s     : #C%d#R]\\n\\r", -1, SQLITE_STATIC );
	sqlite3_bind_int( insert_stmt, 6, 40 );
	sqlite3_step( insert_stmt );
	sqlite3_reset( insert_stmt );

	sqlite3_finalize( insert_stmt );
}

/*
 * Load class score stats from database into memory cache.
 * Called during boot after tables are created.
 */
void db_game_load_class_score( void ) {
	sqlite3_stmt *stmt = NULL;
	char buf[MAX_STRING_LENGTH];

	if ( game_db == NULL ) {
		log_string( "db_game_load_class_score: game_db not open" );
		return;
	}

	class_score_stats_insert_defaults();

	score_stats_count = 0;

	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, stat_source, stat_source_max, stat_label, format_string, display_order "
			"FROM class_score_stats ORDER BY class_id, display_order",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		log_string( "db_game_load_class_score: failed to prepare select" );
		return;
	}

	while ( sqlite3_step( stmt ) == SQLITE_ROW && score_stats_count < MAX_CACHED_SCORE_STATS ) {
		CLASS_SCORE_STAT *entry = &score_stats_cache[score_stats_count];

		entry->class_id        = sqlite3_column_int( stmt, 0 );
		entry->stat_source     = sqlite3_column_int( stmt, 1 );
		entry->stat_source_max = sqlite3_column_int( stmt, 2 );
		entry->stat_label      = str_dup( (const char *)sqlite3_column_text( stmt, 3 ) );
		entry->format_string   = str_dup( (const char *)sqlite3_column_text( stmt, 4 ) );
		entry->display_order   = sqlite3_column_int( stmt, 5 );

		score_stats_count++;
	}

	sqlite3_finalize( stmt );

	sprintf( buf, "Loaded %d class score stats from database.", score_stats_count );
	log_string( buf );
}

/*
 * Get count of score stats for a specific class.
 */
int db_game_get_score_stat_count( int class_id ) {
	int i;
	int count = 0;

	for ( i = 0; i < score_stats_count; i++ ) {
		if ( score_stats_cache[i].class_id == class_id )
			count++;
	}

	return count;
}

/*
 * Get array of score stats for a class.
 * Returns pointer to first entry for this class in the cache.
 * Caller should use db_game_get_score_stat_count to know how many entries.
 * Note: Cache is sorted by class_id, display_order, so entries are contiguous.
 */
const CLASS_SCORE_STAT *db_game_get_score_stats( int class_id ) {
	int i;

	for ( i = 0; i < score_stats_count; i++ ) {
		if ( score_stats_cache[i].class_id == class_id )
			return &score_stats_cache[i];
	}

	return NULL;
}

/*
 * Get the value of a stat from a character based on stat_source enum.
 */
int get_stat_value( CHAR_DATA *ch, int stat_source ) {
	if ( ch == NULL || IS_NPC( ch ) )
		return 0;

	switch ( stat_source ) {
		case STAT_BEAST:          return ch->beast;
		case STAT_RAGE:           return ch->rage;
		case STAT_CHI_CURRENT:    return ch->chi[CURRENT];
		case STAT_CHI_MAXIMUM:    return ch->chi[MAXIMUM];
		case STAT_GNOSIS_CURRENT: return ch->gnosis[GCURRENT];
		case STAT_GNOSIS_MAXIMUM: return ch->gnosis[GMAXIMUM];
		case STAT_MONKBLOCK:      return ch->monkblock;
		case STAT_SILTOL:         return ch->siltol;
		case STAT_SOULS:          return ch->pcdata->souls;
		case STAT_DEMON_POWER:    return ch->pcdata->stats[DEMON_CURRENT];
		case STAT_DEMON_TOTAL:    return ch->pcdata->stats[DEMON_TOTAL];
		case STAT_DROID_POWER:    return ch->pcdata->stats[DROID_POWER];
		case STAT_DROW_POWER:     return ch->pcdata->stats[DROW_POWER];
		case STAT_DROW_MAGIC:     return ch->pcdata->stats[DROW_MAGIC];
		case STAT_TPOINTS:        return ch->pcdata->stats[TPOINTS];
		case STAT_ANGEL_JUSTICE:  return ch->pcdata->powers[ANGEL_JUSTICE];
		case STAT_ANGEL_LOVE:     return ch->pcdata->powers[ANGEL_LOVE];
		case STAT_ANGEL_HARMONY:  return ch->pcdata->powers[ANGEL_HARMONY];
		case STAT_ANGEL_PEACE:    return ch->pcdata->powers[ANGEL_PEACE];
		case STAT_SHAPE_COUNTER:  return ch->pcdata->powers[SHAPE_COUNTER];
		case STAT_PHASE_COUNTER:  return ch->pcdata->powers[PHASE_COUNTER];
		case STAT_HARA_KIRI:      return ch->pcdata->powers[HARA_KIRI];
		default:                  return 0;
	}
}

/*
 * ==========================================================================
 *                         CLASS REGISTRY FUNCTIONS
 * ==========================================================================
 */

/*
 * Insert default class registry entries if table is empty.
 */
static void class_registry_insert_defaults( void ) {
	sqlite3_stmt *count_stmt = NULL;
	sqlite3_stmt *insert_stmt = NULL;
	int count = 0;

	/* Check if table is empty */
	if ( sqlite3_prepare_v2( game_db, "SELECT COUNT(*) FROM class_registry", -1, &count_stmt, NULL ) == SQLITE_OK ) {
		if ( sqlite3_step( count_stmt ) == SQLITE_ROW )
			count = sqlite3_column_int( count_stmt, 0 );
		sqlite3_finalize( count_stmt );
	}

	if ( count > 0 )
		return;  /* Table has data, don't insert defaults */

	/* Prepare insert statement */
	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO class_registry (class_id, class_name, keyword, keyword_alt, mudstat_label, "
			"selfclass_message, display_order, upgrade_class, requirements) "
			"VALUES (?,?,?,?,?,?,?,?,?)",
			-1, &insert_stmt, NULL ) != SQLITE_OK )
		return;

	/* Helper macro for inserting registry entries */
	#define INSERT_REGISTRY( cid, name, kw, kw_alt, label, msg, order, upg, req ) \
		sqlite3_bind_int( insert_stmt, 1, cid ); \
		sqlite3_bind_text( insert_stmt, 2, name, -1, SQLITE_STATIC ); \
		sqlite3_bind_text( insert_stmt, 3, kw, -1, SQLITE_STATIC ); \
		if ( kw_alt ) sqlite3_bind_text( insert_stmt, 4, kw_alt, -1, SQLITE_STATIC ); else sqlite3_bind_null( insert_stmt, 4 ); \
		sqlite3_bind_text( insert_stmt, 5, label, -1, SQLITE_STATIC ); \
		sqlite3_bind_text( insert_stmt, 6, msg, -1, SQLITE_STATIC ); \
		sqlite3_bind_int( insert_stmt, 7, order ); \
		if ( upg > 0 ) sqlite3_bind_int( insert_stmt, 8, upg ); else sqlite3_bind_null( insert_stmt, 8 ); \
		if ( req ) sqlite3_bind_text( insert_stmt, 9, req, -1, SQLITE_STATIC ); else sqlite3_bind_null( insert_stmt, 9 ); \
		sqlite3_step( insert_stmt ); \
		sqlite3_reset( insert_stmt );

	/* Base Classes (9) - upgrade_class = 0 (NULL) */
	INSERT_REGISTRY( CLASS_DEMON, "Demon", "demon", NULL, "Demons",
		"You have chosen the #RDemonic#n path, may god have mercy on yer soul.", 0, 0, NULL );

	INSERT_REGISTRY( CLASS_MAGE, "Mage", "mage", "battlemage", "Mages",
		"You start down the path of power, the #Rarcane#n is your weapon.", 0, 0,
		"Requires 5K mana and 100 in all spell colors" );

	INSERT_REGISTRY( CLASS_WEREWOLF, "Werewolf", "werewolf", NULL, "Werewolfs",
		"You have chosen the path of the #0Garou#n, may gaia guide you.", 1, 0, NULL );

	INSERT_REGISTRY( CLASS_VAMPIRE, "Vampire", "vampire", NULL, "Vampires",
		"Fear the #ySun#n nosferatu, God's curse lives in you.", 1, 0, NULL );

	INSERT_REGISTRY( CLASS_DROW, "Drow", "drow", NULL, "Drows",
		"Choose your profession, and #PLloth#n will guide you.", 2, 0, NULL );

	INSERT_REGISTRY( CLASS_MONK, "Monk", "monk", NULL, "Monks",
		"Your faith in God will guide you, destroy #7EVIL#n.", 3, 0, NULL );

	INSERT_REGISTRY( CLASS_NINJA, "Ninja", "ninja", NULL, "Ninjas",
		"You have chosen a life in the #0shadows#n, assassinate at will.", 2, 0, NULL );

	INSERT_REGISTRY( CLASS_DIRGESINGER, "Dirgesinger", "dirgesinger", NULL, "Dirges",
		"Your voice becomes your weapon. Let the #Gbattle hymns#n begin.", 5, 0, NULL );

	INSERT_REGISTRY( CLASS_PSION, "Psion", "psion", NULL, "Psions",
		"Your mind awakens to #x141psionic power#n. Focus your thoughts.", 6, 0, NULL );

	/* Upgrade Classes (9) - upgrade_class = base class_id */
	INSERT_REGISTRY( CLASS_SAMURAI, "Samurai", "samurai", NULL, "Samurais",
		"You walk the path of the #Rwarrior#n. Honor guides your blade.", 2, CLASS_NINJA, NULL );

	INSERT_REGISTRY( CLASS_LICH, "Lich", "lich", NULL, "Lichs",
		"Undeath calls to you. Embrace the #0cold#n eternity.", 3, CLASS_MAGE, NULL );

	INSERT_REGISTRY( CLASS_SHAPESHIFTER, "Shapeshifter", "shapeshifter", NULL, "Shapies",
		"Your form is malleable. Become #Ranything#n.", 3, CLASS_WEREWOLF, NULL );

	INSERT_REGISTRY( CLASS_TANARRI, "Tanarri", "tanarri", NULL, "Tanar'ris",
		"Chaos incarnate. The #Rabyss#n welcomes you.", 4, CLASS_DEMON, NULL );

	INSERT_REGISTRY( CLASS_ANGEL, "Angel", "angel", NULL, "Angels",
		"Heaven's light shines upon you. Protect the #7innocent#n.", 4, CLASS_MONK, NULL );

	INSERT_REGISTRY( CLASS_UNDEAD_KNIGHT, "Undead Knight", "knight", "undeadknight", "Knights",
		"Death is not the end. Rise as an #LUndead Knight#n.", 4, CLASS_VAMPIRE, NULL );

	INSERT_REGISTRY( CLASS_DROID, "Spider Droid", "droid", "spiderdroid", "Driders",
		"Mechanical precision. #0Calculate#n and destroy.", 5, CLASS_DROW, NULL );

	INSERT_REGISTRY( CLASS_SIREN, "Siren", "siren", NULL, "Sirens",
		"Your song enchants all who hear. #x255Sing#n of their doom.", 5, CLASS_DIRGESINGER, NULL );

	INSERT_REGISTRY( CLASS_MINDFLAYER, "Mindflayer", "mindflayer", NULL, "Mindflayers",
		"The hunger for #Gminds#n consumes you. Feed.", 6, CLASS_PSION, NULL );

	#undef INSERT_REGISTRY

	sqlite3_finalize( insert_stmt );
}

/*
 * Load class registry from database into memory cache.
 * Called during boot after tables are created.
 */
void db_game_load_class_registry( void ) {
	sqlite3_stmt *stmt = NULL;
	char buf[MAX_STRING_LENGTH];
	const char *text;

	if ( game_db == NULL ) {
		log_string( "db_game_load_class_registry: game_db not open" );
		return;
	}

	class_registry_insert_defaults();

	registry_count = 0;

	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, class_name, keyword, keyword_alt, mudstat_label, "
			"selfclass_message, display_order, upgrade_class, requirements "
			"FROM class_registry ORDER BY display_order, class_id",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		log_string( "db_game_load_class_registry: failed to prepare select" );
		return;
	}

	while ( sqlite3_step( stmt ) == SQLITE_ROW && registry_count < MAX_CACHED_REGISTRY ) {
		CLASS_REGISTRY_ENTRY *entry = &registry_cache[registry_count];

		entry->class_id        = sqlite3_column_int( stmt, 0 );
		entry->class_name      = str_dup( (const char *)sqlite3_column_text( stmt, 1 ) );
		entry->keyword         = str_dup( (const char *)sqlite3_column_text( stmt, 2 ) );

		text = (const char *)sqlite3_column_text( stmt, 3 );
		entry->keyword_alt     = text ? str_dup( text ) : NULL;

		entry->mudstat_label   = str_dup( (const char *)sqlite3_column_text( stmt, 4 ) );
		entry->selfclass_message = str_dup( (const char *)sqlite3_column_text( stmt, 5 ) );
		entry->display_order   = sqlite3_column_int( stmt, 6 );

		if ( sqlite3_column_type( stmt, 7 ) == SQLITE_NULL )
			entry->upgrade_class = 0;
		else
			entry->upgrade_class = sqlite3_column_int( stmt, 7 );

		text = (const char *)sqlite3_column_text( stmt, 8 );
		entry->requirements    = text ? str_dup( text ) : NULL;

		registry_count++;
	}

	sqlite3_finalize( stmt );

	sprintf( buf, "Loaded %d class registry entries from database.", registry_count );
	log_string( buf );
}

/*
 * Get count of registry entries.
 */
int db_game_get_registry_count( void ) {
	return registry_count;
}

/*
 * Get registry entry by class_id.
 */
const CLASS_REGISTRY_ENTRY *db_game_get_registry_by_id( int class_id ) {
	int i;

	for ( i = 0; i < registry_count; i++ ) {
		if ( registry_cache[i].class_id == class_id )
			return &registry_cache[i];
	}

	return NULL;
}

/*
 * Get registry entry by keyword or alternate keyword.
 * Case-insensitive match.
 */
const CLASS_REGISTRY_ENTRY *db_game_get_registry_by_keyword( const char *keyword ) {
	int i;

	if ( keyword == NULL || keyword[0] == '\0' )
		return NULL;

	for ( i = 0; i < registry_count; i++ ) {
		if ( !str_cmp( registry_cache[i].keyword, keyword ) )
			return &registry_cache[i];

		if ( registry_cache[i].keyword_alt != NULL &&
		     !str_cmp( registry_cache[i].keyword_alt, keyword ) )
			return &registry_cache[i];
	}

	return NULL;
}

/*
 * Get registry entry by index (for iteration).
 */
const CLASS_REGISTRY_ENTRY *db_game_get_registry_by_index( int index ) {
	if ( index < 0 || index >= registry_count )
		return NULL;

	return &registry_cache[index];
}
