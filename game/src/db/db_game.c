/***************************************************************************
 *  db_game.c - SQLite persistence for global game data
 *
 *  Manages base_help.db + live_help.db (help entries) and game.db
 *  (config, leaderboards, kingdoms) stored in gamedata/db/game/.
 ***************************************************************************/

#include "db_util.h"
#include "db_game.h"
#include "../core/ability_config.h"
#include "../classes/class_display.h"

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

	"CREATE TABLE IF NOT EXISTS class_display ("
	"  class_id      INTEGER PRIMARY KEY,"
	"  name          TEXT NOT NULL,"
	"  left_symbol   TEXT NOT NULL DEFAULT '',"
	"  right_symbol  TEXT NOT NULL DEFAULT '',"
	"  look_display  TEXT NOT NULL DEFAULT '',"
	"  title_color   TEXT NOT NULL DEFAULT ''"
	");"

	"CREATE TABLE IF NOT EXISTS generation_titles ("
	"  class_id    INTEGER NOT NULL,"
	"  generation  INTEGER NOT NULL,"
	"  title       TEXT NOT NULL,"
	"  PRIMARY KEY (class_id, generation)"
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
 * Close game database connections.
 */
void db_game_close( void ) {
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
	sqlite3_stmt *stmt;
	const char *sql = "INSERT INTO helps (level, keyword, text) VALUES (?,?,?)";
	HELP_DATA *pHelp;

	if ( !live_help_db )
		return;

	db_begin( live_help_db );
	sqlite3_exec( live_help_db, "DELETE FROM helps", NULL, NULL, NULL );

	if ( sqlite3_prepare_v2( live_help_db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		db_rollback( live_help_db );
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
	db_commit( live_help_db );
}


/*
 * Reload a single help entry by keyword.
 * Checks live_help.db first, then falls back to base_help.db.
 * Returns TRUE if the entry was found and loaded.
 */
bool db_game_reload_help( const char *keyword ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT level, keyword, text FROM helps WHERE keyword=?";
	sqlite3 *dbs[] = { live_help_db, base_help_db };
	int i;

	for ( i = 0; i < 2; i++ ) {
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
			sqlite3_finalize( stmt );
			return TRUE;
		}

		sqlite3_finalize( stmt );
	}

	return FALSE;
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
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO gameconfig (key, value) VALUES (?,?)";
	char buf[64];

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

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
	db_commit( game_db );
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
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO topboard (rank, name, pkscore) VALUES (?,?,?)";
	int i;

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

	for ( i = 1; i <= MAX_TOP_PLAYERS; i++ ) {
		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, i );
		sqlite3_bind_text( stmt, 2, top_board[i].name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( stmt, 3, top_board[i].pkscore );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( game_db );
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
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO leaderboard (category, name, value) VALUES (?,?,?)";

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

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
	db_commit( game_db );
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
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO kingdoms "
		"(id, name, whoname, leader, general, kills, deaths, qps, "
		"req_hit, req_move, req_mana, req_qps) "
		"VALUES (?,?,?,?,?,?,?,?,?,?,?,?)";
	int i;

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

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
	db_commit( game_db );
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
	sqlite3_stmt *del_stmt, *ins_stmt;
	NOTE_DATA *note;

	if ( !game_db )
		return;

	db_begin( game_db );

	/* Delete all notes for this board */
	if ( sqlite3_prepare_v2( game_db,
			"DELETE FROM notes WHERE board_idx=?",
			-1, &del_stmt, NULL ) == SQLITE_OK ) {
		sqlite3_bind_int( del_stmt, 1, board_idx );
		sqlite3_step( del_stmt );
		sqlite3_finalize( del_stmt );
	}

	/* Re-insert all current notes */
	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO notes (board_idx, sender, date, date_stamp, expire, "
			"to_list, subject, text) VALUES (?,?,?,?,?,?,?,?)",
			-1, &ins_stmt, NULL ) != SQLITE_OK ) {
		db_rollback( game_db );
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
	db_commit( game_db );
}

void db_game_append_note( int board_idx, NOTE_DATA *note ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO notes (board_idx, sender, date, date_stamp, expire, "
		"to_list, subject, text) VALUES (?,?,?,?,?,?,?,?)";

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

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
}


/*
 * Bug reports (game.db)
 * Append-only log of player bug reports and system bugs.
 */
void db_game_append_bug( int room_vnum, const char *player, const char *message ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO bugs (room_vnum, player, message, timestamp) VALUES (?,?,?,?)";

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	sqlite3_bind_int( stmt, 1, room_vnum );
	sqlite3_bind_text( stmt, 2, player, -1, SQLITE_TRANSIENT );
	sqlite3_bind_text( stmt, 3, message, -1, SQLITE_TRANSIENT );
	sqlite3_bind_int64( stmt, 4, (sqlite3_int64)current_time );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
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
	sqlite3_stmt *stmt;
	BAN_DATA *p;

	if ( !game_db )
		return;

	db_begin( game_db );
	sqlite3_exec( game_db, "DELETE FROM bans", NULL, NULL, NULL );

	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO bans (name, reason) VALUES (?,?)",
			-1, &stmt, NULL ) != SQLITE_OK ) {
		db_rollback( game_db );
		return;
	}

	for ( p = ban_list; p; p = p->next ) {
		sqlite3_reset( stmt );
		sqlite3_bind_text( stmt, 1, p->name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 2, p->reason, -1, SQLITE_TRANSIENT );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( game_db );
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
	sqlite3_stmt *stmt;
	DISABLED_DATA *p;

	if ( !game_db )
		return;

	db_begin( game_db );
	sqlite3_exec( game_db, "DELETE FROM disabled_commands", NULL, NULL, NULL );

	if ( disabled_first ) {
		if ( sqlite3_prepare_v2( game_db,
				"INSERT INTO disabled_commands (command_name, level, disabled_by) "
				"VALUES (?,?,?)",
				-1, &stmt, NULL ) != SQLITE_OK ) {
			db_rollback( game_db );
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

	db_commit( game_db );
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
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO balance_config (key, value) VALUES (?,?)";
	char buf[64];
	int i;

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

	for ( i = 0; balance_map[i].key != NULL; i++ ) {
		sqlite3_reset( stmt );
		snprintf( buf, sizeof( buf ), "%d", *balance_map[i].field );
		sqlite3_bind_text( stmt, 1, balance_map[i].key, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 2, buf, -1, SQLITE_TRANSIENT );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	db_commit( game_db );
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
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR REPLACE INTO ability_config (key, value) VALUES (?,?)";
	int i, total;

	if ( !game_db )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	db_begin( game_db );

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
	db_commit( game_db );
}


/*
 * Super admins (game.db)
 * Character names that can use the relevel command for instant admin access.
 * This allows forks to configure their own admins without modifying code.
 */
bool db_game_is_super_admin( const char *name ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT 1 FROM super_admins WHERE name = ? LIMIT 1";
	bool found = FALSE;

	if ( !game_db || !name || !name[0] )
		return FALSE;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return FALSE;

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );

	if ( sqlite3_step( stmt ) == SQLITE_ROW )
		found = TRUE;

	sqlite3_finalize( stmt );
	return found;
}

void db_game_add_super_admin( const char *name ) {
	sqlite3_stmt *stmt;
	const char *sql = "INSERT OR IGNORE INTO super_admins (name) VALUES (?)";

	if ( !game_db || !name || !name[0] )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
}

void db_game_remove_super_admin( const char *name ) {
	sqlite3_stmt *stmt;
	const char *sql = "DELETE FROM super_admins WHERE name = ?";

	if ( !game_db || !name || !name[0] )
		return;

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_TRANSIENT );
	sqlite3_step( stmt );
	sqlite3_finalize( stmt );
}

void db_game_list_super_admins( CHAR_DATA *ch ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT name FROM super_admins ORDER BY name";
	int count = 0;

	if ( !game_db ) {
		send_to_char( "Database not available.\n\r", ch );
		return;
	}

	if ( sqlite3_prepare_v2( game_db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		send_to_char( "Database error.\n\r", ch );
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

	if ( count == 0 )
		send_to_char( "  (none)\n\r", ch );
	else {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ), "#wTotal: %d#n\n\r", count );
		send_to_char( buf, ch );
	}
}


/*
 * Class display data (game.db)
 * Stores class symbols, titles, and look display strings.
 */
void db_class_display_load( void ) {
	sqlite3_stmt *stmt;
	char buf[MAX_STRING_LENGTH];
	int loaded = 0;

	if ( !game_db )
		return;

	/* Load class_display table */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, name, left_symbol, right_symbol, look_display, title_color "
			"FROM class_display ORDER BY class_id",
			-1, &stmt, NULL ) == SQLITE_OK ) {

		while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
			int class_id = sqlite3_column_int( stmt, 0 );
			CLASS_DISPLAY_DATA *cd = class_display_get( class_id );

			if ( cd ) {
				/* Update existing entry with DB values */
				strncpy( cd->name, col_text( stmt, 1 ), CLASS_NAME_LEN - 1 );
				cd->name[CLASS_NAME_LEN - 1] = '\0';
				strncpy( cd->left_symbol, col_text( stmt, 2 ), CLASS_SYMBOL_LEN - 1 );
				cd->left_symbol[CLASS_SYMBOL_LEN - 1] = '\0';
				strncpy( cd->right_symbol, col_text( stmt, 3 ), CLASS_SYMBOL_LEN - 1 );
				cd->right_symbol[CLASS_SYMBOL_LEN - 1] = '\0';
				strncpy( cd->look_display, col_text( stmt, 4 ), CLASS_LOOK_LEN - 1 );
				cd->look_display[CLASS_LOOK_LEN - 1] = '\0';
				strncpy( cd->title_color, col_text( stmt, 5 ), sizeof(cd->title_color) - 1 );
				cd->title_color[sizeof(cd->title_color) - 1] = '\0';
				loaded++;
			}
		}
		sqlite3_finalize( stmt );
	}

	/* Load generation_titles table */
	if ( sqlite3_prepare_v2( game_db,
			"SELECT class_id, generation, title FROM generation_titles",
			-1, &stmt, NULL ) == SQLITE_OK ) {

		while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
			int class_id   = sqlite3_column_int( stmt, 0 );
			int generation = sqlite3_column_int( stmt, 1 );
			CLASS_DISPLAY_DATA *cd = class_display_get( class_id );

			if ( cd && generation >= 0 && generation < MAX_GENERATIONS ) {
				strncpy( cd->titles[generation], col_text( stmt, 2 ), CLASS_TITLE_LEN - 1 );
				cd->titles[generation][CLASS_TITLE_LEN - 1] = '\0';
			}
		}
		sqlite3_finalize( stmt );
	}

	if ( loaded > 0 ) {
		snprintf( buf, sizeof(buf), "  Loaded %d class display overrides from game.db", loaded );
		log_string( buf );
	}
}

void db_class_display_save( void ) {
	int i;

	if ( !game_db )
		return;

	for ( i = 0; i < class_display_count; i++ ) {
		db_class_display_save_one( class_display_table[i].class_id );
	}
}

void db_class_display_save_one( int class_id ) {
	sqlite3_stmt *stmt;
	CLASS_DISPLAY_DATA *cd;
	int j;

	if ( !game_db )
		return;

	cd = class_display_get( class_id );
	if ( !cd )
		return;

	db_begin( game_db );

	/* Save/update class_display row */
	if ( sqlite3_prepare_v2( game_db,
			"INSERT OR REPLACE INTO class_display "
			"(class_id, name, left_symbol, right_symbol, look_display, title_color) "
			"VALUES (?,?,?,?,?,?)",
			-1, &stmt, NULL ) == SQLITE_OK ) {

		sqlite3_bind_int( stmt, 1, cd->class_id );
		sqlite3_bind_text( stmt, 2, cd->name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 3, cd->left_symbol, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 4, cd->right_symbol, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 5, cd->look_display, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 6, cd->title_color, -1, SQLITE_TRANSIENT );
		sqlite3_step( stmt );
		sqlite3_finalize( stmt );
	}

	/* Delete existing generation titles for this class */
	if ( sqlite3_prepare_v2( game_db,
			"DELETE FROM generation_titles WHERE class_id=?",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		sqlite3_bind_int( stmt, 1, cd->class_id );
		sqlite3_step( stmt );
		sqlite3_finalize( stmt );
	}

	/* Insert generation titles */
	if ( sqlite3_prepare_v2( game_db,
			"INSERT INTO generation_titles (class_id, generation, title) VALUES (?,?,?)",
			-1, &stmt, NULL ) == SQLITE_OK ) {

		for ( j = 0; j < MAX_GENERATIONS; j++ ) {
			if ( cd->titles[j][0] != '\0' ) {
				sqlite3_reset( stmt );
				sqlite3_bind_int( stmt, 1, cd->class_id );
				sqlite3_bind_int( stmt, 2, j );
				sqlite3_bind_text( stmt, 3, cd->titles[j], -1, SQLITE_TRANSIENT );
				sqlite3_step( stmt );
			}
		}
		sqlite3_finalize( stmt );
	}

	db_commit( game_db );
}
