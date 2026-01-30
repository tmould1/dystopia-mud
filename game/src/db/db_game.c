/***************************************************************************
 *  db_game.c - SQLite persistence for global game data
 *
 *  Manages help.db (help entries) and game.db (config, leaderboards,
 *  kingdoms) stored in gamedata/db/game/.
 ***************************************************************************/

/*
 * sqlite3.h must be included BEFORE merc.h because merc.h defines
 * single-letter macros (N, Z) that collide with SQLite API parameter names.
 */
#include "sqlite3.h"
#include "../core/merc.h"
#undef N    /* merc.h: #define N 8192  - conflicts with sqlite3 API */
#undef Z    /* merc.h: #define Z 33554432 - conflicts with sqlite3 API */

#include "db_game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External globals */
extern char mud_db_dir[MUD_PATH_MAX];
extern HELP_DATA *first_help;
extern HELP_DATA *last_help;
extern HELP_DATA *help_first;
extern HELP_DATA *help_last;
extern char *help_greeting;
extern int top_help;
extern TOP_BOARD top_board[MAX_TOP_PLAYERS + 1];
extern LEADER_BOARD leader_board;
extern KINGDOM_DATA kingdom_table[MAX_KINGDOM + 1];
extern GAMECONFIG_DATA game_config;

/* Cached database connections */
static sqlite3 *help_db = NULL;
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
	");";


/*
 * Helper: safe string from sqlite column (returns "" for NULL).
 */
static const char *col_text( sqlite3_stmt *stmt, int col ) {
	const char *val = (const char *)sqlite3_column_text( stmt, col );
	return val ? val : "";
}


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

	/* Open help.db */
	if ( snprintf( path, sizeof( path ), "%s%shelp.db",
			mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
		bug( "db_game_init: help.db path truncated.", 0 );
	}
	if ( sqlite3_open( path, &help_db ) != SQLITE_OK ) {
		bug( "db_game_init: cannot open help.db", 0 );
		help_db = NULL;
	} else {
		if ( sqlite3_exec( help_db, HELP_SCHEMA_SQL, NULL, NULL, &errmsg ) != SQLITE_OK ) {
			bug( "db_game_init: help schema error", 0 );
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
	if ( help_db ) {
		sqlite3_close( help_db );
		help_db = NULL;
	}
	if ( game_db ) {
		sqlite3_close( game_db );
		game_db = NULL;
	}
}


/*
 * Load all help entries from help.db into the in-memory linked lists.
 */
void db_game_load_helps( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT level, keyword, text FROM helps ORDER BY id";
	char buf[MAX_STRING_LENGTH];
	int count = 0;

	if ( !help_db )
		return;

	if ( sqlite3_prepare_v2( help_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

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

	snprintf( buf, sizeof( buf ), "  Loaded %d help entries from help.db", count );
	log_string( buf );
}


/*
 * Save all help entries to help.db.
 * Deletes existing data and re-inserts from in-memory lists.
 */
void db_game_save_helps( void ) {
	sqlite3_stmt *stmt;
	const char *sql = "INSERT INTO helps (level, keyword, text) VALUES (?,?,?)";
	HELP_DATA *pHelp;

	if ( !help_db )
		return;

	sqlite3_exec( help_db, "BEGIN TRANSACTION", NULL, NULL, NULL );
	sqlite3_exec( help_db, "DELETE FROM helps", NULL, NULL, NULL );

	if ( sqlite3_prepare_v2( help_db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		sqlite3_exec( help_db, "ROLLBACK", NULL, NULL, NULL );
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
	sqlite3_exec( help_db, "COMMIT", NULL, NULL, NULL );
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

	sqlite3_exec( game_db, "BEGIN TRANSACTION", NULL, NULL, NULL );

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

#undef SAVE_INT
#undef SAVE_STR

	sqlite3_finalize( stmt );
	sqlite3_exec( game_db, "COMMIT", NULL, NULL, NULL );
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

	sqlite3_exec( game_db, "BEGIN TRANSACTION", NULL, NULL, NULL );

	for ( i = 1; i <= MAX_TOP_PLAYERS; i++ ) {
		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, i );
		sqlite3_bind_text( stmt, 2, top_board[i].name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( stmt, 3, top_board[i].pkscore );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
	sqlite3_exec( game_db, "COMMIT", NULL, NULL, NULL );
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

	sqlite3_exec( game_db, "BEGIN TRANSACTION", NULL, NULL, NULL );

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
	sqlite3_exec( game_db, "COMMIT", NULL, NULL, NULL );
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

	sqlite3_exec( game_db, "BEGIN TRANSACTION", NULL, NULL, NULL );

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
	sqlite3_exec( game_db, "COMMIT", NULL, NULL, NULL );
}
