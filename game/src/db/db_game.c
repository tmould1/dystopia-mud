/***************************************************************************
 *  db_game.c - SQLite persistence for global game data
 *
 *  Manages help.db (help entries) and game.db (config, leaderboards,
 *  kingdoms) stored in gamedata/db/game/.
 ***************************************************************************/

#include "db_util.h"
#include "db_game.h"

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

	db_begin( help_db );
	sqlite3_exec( help_db, "DELETE FROM helps", NULL, NULL, NULL );

	if ( sqlite3_prepare_v2( help_db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
		db_rollback( help_db );
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
	db_commit( help_db );
}


/*
 * Reload a single help entry from help.db by keyword.
 * Returns TRUE if the entry was found and loaded.
 */
bool db_game_reload_help( const char *keyword ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT level, keyword, text FROM helps WHERE keyword=?";
	bool found = FALSE;

	if ( !help_db )
		return FALSE;

	if ( sqlite3_prepare_v2( help_db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return FALSE;

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
