/***************************************************************************
 *  db_player.c - SQLite persistence for player character data
 *
 *  Per-player .db files stored in gamedata/db/players/.
 *  Each player gets their own SQLite database containing tables for
 *  character data, skills, aliases, affects, boards, and inventory.
 ***************************************************************************/

#include "db_util.h"
#include "db_player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../systems/profile.h"
#include "../core/compat.h"

/* External globals */
extern char mud_db_dir[MUD_PATH_MAX];
extern const struct skill_type skill_table[MAX_SKILL];
extern time_t current_time;

/* Directory for player databases */
static char mud_db_players_dir[MUD_PATH_MAX] = "";

/*
 * Background save infrastructure.
 * Uses sqlite3_serialize to capture database state in main thread,
 * then writes to disk in background thread.
 */
typedef struct {
	char            path[MUD_PATH_MAX];     /* Full path to player .db file */
	unsigned char  *data;                   /* Serialized database */
	sqlite3_int64   size;                   /* Size of serialized data */
} PLAYER_SAVE_TASK;

/* Pending save tracking for graceful shutdown */
static int              pending_saves = 0;
static pthread_mutex_t  save_mutex;
static pthread_cond_t   save_cond;


/*
 * Schema for player .db files.
 * Uses multiple statements separated by semicolons.
 */
static const char *PLAYER_SCHEMA_SQL =
	"CREATE TABLE IF NOT EXISTS meta ("
	"  key   TEXT PRIMARY KEY,"
	"  value TEXT NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS player ("
	/* Identity strings */
	"  name           TEXT NOT NULL,"
	"  switchname     TEXT NOT NULL DEFAULT '',"
	"  short_descr    TEXT NOT NULL DEFAULT '',"
	"  long_descr     TEXT NOT NULL DEFAULT '',"
	"  objdesc        TEXT NOT NULL DEFAULT '',"
	"  description    TEXT NOT NULL DEFAULT '',"
	"  lord           TEXT NOT NULL DEFAULT '',"
	"  clan           TEXT NOT NULL DEFAULT '',"
	"  morph          TEXT NOT NULL DEFAULT '',"
	"  createtime     TEXT NOT NULL DEFAULT '',"
	"  lasttime       TEXT NOT NULL DEFAULT '',"
	"  lasthost       TEXT NOT NULL DEFAULT '',"
	"  poweraction    TEXT NOT NULL DEFAULT '',"
	"  powertype      TEXT NOT NULL DEFAULT '',"
	"  prompt         TEXT NOT NULL DEFAULT '',"
	"  cprompt        TEXT NOT NULL DEFAULT '',"
	/* PC-only strings */
	"  password       TEXT NOT NULL DEFAULT '',"
	"  bamfin         TEXT NOT NULL DEFAULT '',"
	"  bamfout        TEXT NOT NULL DEFAULT '',"
	"  title          TEXT NOT NULL DEFAULT '',"
	"  conception     TEXT NOT NULL DEFAULT '',"
	"  parents        TEXT NOT NULL DEFAULT '',"
	"  cparents       TEXT NOT NULL DEFAULT '',"
	"  marriage       TEXT NOT NULL DEFAULT '',"
	"  decapmessage   TEXT NOT NULL DEFAULT '',"
	"  loginmessage   TEXT NOT NULL DEFAULT '',"
	"  logoutmessage  TEXT NOT NULL DEFAULT '',"
	"  avatarmessage  TEXT NOT NULL DEFAULT '',"
	"  tiemessage     TEXT NOT NULL DEFAULT '',"
	"  last_decap_0   TEXT NOT NULL DEFAULT '',"
	"  last_decap_1   TEXT NOT NULL DEFAULT '',"
	/* Core ints */
	"  sex            INTEGER NOT NULL DEFAULT 0,"
	"  class          INTEGER NOT NULL DEFAULT 0,"
	"  level          INTEGER NOT NULL DEFAULT 0,"
	"  trust          INTEGER NOT NULL DEFAULT 0,"
	"  played         INTEGER NOT NULL DEFAULT 0,"
	"  room_vnum      INTEGER NOT NULL DEFAULT 3001,"
	"  gold           INTEGER NOT NULL DEFAULT 0,"
	"  exp            INTEGER NOT NULL DEFAULT 0,"
	"  expgained      INTEGER NOT NULL DEFAULT 0,"
	"  act            INTEGER NOT NULL DEFAULT 0,"
	"  extra          INTEGER NOT NULL DEFAULT 0,"
	"  newbits        INTEGER NOT NULL DEFAULT 0,"
	"  special        INTEGER NOT NULL DEFAULT 0,"
	"  affected_by    INTEGER NOT NULL DEFAULT 0,"
	"  immune         INTEGER NOT NULL DEFAULT 0,"
	"  polyaff        INTEGER NOT NULL DEFAULT 0,"
	"  itemaffect     INTEGER NOT NULL DEFAULT 0,"
	"  form           INTEGER NOT NULL DEFAULT 1048575,"
	"  position       INTEGER NOT NULL DEFAULT 0,"
	"  practice       INTEGER NOT NULL DEFAULT 0,"
	"  saving_throw   INTEGER NOT NULL DEFAULT 0,"
	"  alignment      INTEGER NOT NULL DEFAULT 0,"
	"  xhitroll       INTEGER NOT NULL DEFAULT 0,"
	"  xdamroll       INTEGER NOT NULL DEFAULT 0,"
	"  hitroll        INTEGER NOT NULL DEFAULT 0,"
	"  damroll        INTEGER NOT NULL DEFAULT 0,"
	"  armor          INTEGER NOT NULL DEFAULT 0,"
	"  wimpy          INTEGER NOT NULL DEFAULT 0,"
	"  deaf           INTEGER NOT NULL DEFAULT 0,"
	"  beast          INTEGER NOT NULL DEFAULT 15,"
	"  home           INTEGER NOT NULL DEFAULT 3001,"
	"  spectype       INTEGER NOT NULL DEFAULT 0,"
	"  specpower      INTEGER NOT NULL DEFAULT 0,"
	/* HP/Mana/Move */
	"  hit            INTEGER NOT NULL DEFAULT 0,"
	"  max_hit        INTEGER NOT NULL DEFAULT 0,"
	"  mana           INTEGER NOT NULL DEFAULT 0,"
	"  max_mana       INTEGER NOT NULL DEFAULT 0,"
	"  move           INTEGER NOT NULL DEFAULT 0,"
	"  max_move       INTEGER NOT NULL DEFAULT 0,"
	/* PK/PD/MK/MD */
	"  pkill          INTEGER NOT NULL DEFAULT 0,"
	"  pdeath         INTEGER NOT NULL DEFAULT 0,"
	"  mkill          INTEGER NOT NULL DEFAULT 0,"
	"  mdeath         INTEGER NOT NULL DEFAULT 0,"
	"  awins          INTEGER NOT NULL DEFAULT 0,"
	"  alosses        INTEGER NOT NULL DEFAULT 0,"
	/* Class-specific */
	"  warp           INTEGER NOT NULL DEFAULT 0,"
	"  warpcount      INTEGER NOT NULL DEFAULT 0,"
	"  monkstuff      INTEGER NOT NULL DEFAULT 0,"
	"  monkcrap       INTEGER NOT NULL DEFAULT 0,"
	"  garou1         INTEGER NOT NULL DEFAULT 0,"
	"  garou2         INTEGER NOT NULL DEFAULT 0,"
	"  rage           INTEGER NOT NULL DEFAULT 0,"
	"  generation     INTEGER NOT NULL DEFAULT 0,"
	"  cur_form       INTEGER NOT NULL DEFAULT 0,"
	"  flag2          INTEGER NOT NULL DEFAULT 0,"
	"  flag3          INTEGER NOT NULL DEFAULT 0,"
	"  flag4          INTEGER NOT NULL DEFAULT 0,"
	"  siltol         INTEGER NOT NULL DEFAULT 0,"
	"  gnosis_max     INTEGER NOT NULL DEFAULT 0,"
	/* PC_DATA ints */
	"  kingdom        INTEGER NOT NULL DEFAULT 0,"
	"  quest          INTEGER NOT NULL DEFAULT 0,"
	"  rank           INTEGER NOT NULL DEFAULT 0,"
	"  bounty         INTEGER NOT NULL DEFAULT 0,"
	"  security       INTEGER NOT NULL DEFAULT 0,"
	"  jflags         INTEGER NOT NULL DEFAULT 0,"
	"  souls          INTEGER NOT NULL DEFAULT 0,"
	"  upgrade_level  INTEGER NOT NULL DEFAULT 0,"
	"  mean_paradox   INTEGER NOT NULL DEFAULT 0,"
	"  relrank        INTEGER NOT NULL DEFAULT 0,"
	"  rune_count     INTEGER NOT NULL DEFAULT 0,"
	"  revision       INTEGER NOT NULL DEFAULT 0,"
	"  disc_research  INTEGER NOT NULL DEFAULT -1,"
	"  disc_points    INTEGER NOT NULL DEFAULT 0,"
	"  obj_vnum       INTEGER NOT NULL DEFAULT 0,"
	"  exhaustion     INTEGER NOT NULL DEFAULT 0,"
	"  questsrun      INTEGER NOT NULL DEFAULT 0,"
	"  questtotal     INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS player_arrays ("
	"  name TEXT PRIMARY KEY,"
	"  data TEXT NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS skills ("
	"  skill_name TEXT PRIMARY KEY,"
	"  value      INTEGER NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS aliases ("
	"  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  short_n TEXT NOT NULL,"
	"  long_n  TEXT NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS affects ("
	"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  skill_name TEXT NOT NULL,"
	"  duration   INTEGER NOT NULL,"
	"  modifier   INTEGER NOT NULL,"
	"  location   INTEGER NOT NULL,"
	"  bitvector  INTEGER NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS boards ("
	"  board_name TEXT PRIMARY KEY,"
	"  last_note  INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS objects ("
	"  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  nest         INTEGER NOT NULL DEFAULT 0,"
	"  vnum         INTEGER NOT NULL,"
	"  name         TEXT NOT NULL DEFAULT '',"
	"  short_descr  TEXT NOT NULL DEFAULT '',"
	"  description  TEXT NOT NULL DEFAULT '',"
	"  chpoweron    TEXT,"
	"  chpoweroff   TEXT,"
	"  chpoweruse   TEXT,"
	"  victpoweron  TEXT,"
	"  victpoweroff TEXT,"
	"  victpoweruse TEXT,"
	"  questmaker   TEXT,"
	"  questowner   TEXT,"
	"  extra_flags  INTEGER NOT NULL DEFAULT 0,"
	"  extra_flags2 INTEGER NOT NULL DEFAULT 0,"
	"  weapflags    INTEGER NOT NULL DEFAULT 0,"
	"  wear_flags   INTEGER NOT NULL DEFAULT 0,"
	"  wear_loc     INTEGER NOT NULL DEFAULT 0,"
	"  item_type    INTEGER NOT NULL DEFAULT 0,"
	"  weight       INTEGER NOT NULL DEFAULT 0,"
	"  spectype     INTEGER NOT NULL DEFAULT 0,"
	"  specpower    INTEGER NOT NULL DEFAULT 0,"
	"  condition    INTEGER NOT NULL DEFAULT 100,"
	"  toughness    INTEGER NOT NULL DEFAULT 0,"
	"  resistance   INTEGER NOT NULL DEFAULT 100,"
	"  quest        INTEGER NOT NULL DEFAULT 0,"
	"  points       INTEGER NOT NULL DEFAULT 0,"
	"  level        INTEGER NOT NULL DEFAULT 0,"
	"  timer        INTEGER NOT NULL DEFAULT 0,"
	"  cost         INTEGER NOT NULL DEFAULT 0,"
	"  value_0      INTEGER NOT NULL DEFAULT 0,"
	"  value_1      INTEGER NOT NULL DEFAULT 0,"
	"  value_2      INTEGER NOT NULL DEFAULT 0,"
	"  value_3      INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS obj_affects ("
	"  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  obj_id   INTEGER NOT NULL,"
	"  duration INTEGER NOT NULL,"
	"  modifier INTEGER NOT NULL,"
	"  location INTEGER NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS obj_extra_descr ("
	"  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  obj_id      INTEGER NOT NULL,"
	"  keyword     TEXT NOT NULL,"
	"  description TEXT NOT NULL"
	");"
;


/*
 * Background thread function for async player saves.
 * Writes pre-serialized database data to disk.
 */
static void *player_save_thread( void *arg ) {
	PLAYER_SAVE_TASK *task = (PLAYER_SAVE_TASK *)arg;
	FILE *fp;

	/* Write serialized data directly to file */
	fp = fopen( task->path, "wb" );
	if ( fp != NULL ) {
		fwrite( task->data, 1, task->size, fp );
		fclose( fp );
	}

	/* Free resources */
	sqlite3_free( task->data );
	free( task );

	/* Decrement pending saves counter and signal if all saves complete */
	pthread_mutex_lock( &save_mutex );
	pending_saves--;
	if ( pending_saves == 0 ) {
		pthread_cond_broadcast( &save_cond );
	}
	pthread_mutex_unlock( &save_mutex );

	return NULL;
}

/*
 * Wait for all pending background saves to complete.
 * Called during quit/shutdown to ensure data is written.
 * Uses condition variable for efficient blocking.
 */
void db_player_wait_pending( void ) {
	pthread_mutex_lock( &save_mutex );
	while ( pending_saves > 0 ) {
		pthread_cond_wait( &save_cond, &save_mutex );
	}
	pthread_mutex_unlock( &save_mutex );
}

/*
 * Get count of pending background saves.
 */
int db_player_pending_count( void ) {
	return pending_saves;
}


/*
 * Build path to a player's .db file.
 * Returns 0 on success, -1 on truncation.
 */
static int db_player_path( const char *name, char *buf, int bufsize ) {
	if ( snprintf( buf, bufsize, "%s%s%s.db",
			mud_db_players_dir, PATH_SEPARATOR, capitalize( (char *)name ) )
			>= bufsize ) {
		bug( "db_player_path: path truncated.", 0 );
		return -1;
	}
	return 0;
}


/*
 * Open (or create) a player's .db file and ensure schema exists.
 * Returns sqlite3* on success, NULL on failure.
 * Caller must close the connection when done.
 */
static sqlite3 *db_player_open( const char *name ) {
	char path[MUD_PATH_MAX];
	sqlite3 *db = NULL;
	char *errmsg = NULL;

	if ( db_player_path( name, path, sizeof( path ) ) < 0 )
		return NULL;

	if ( sqlite3_open( path, &db ) != SQLITE_OK ) {
		bug( "db_player_open: cannot open db", 0 );
		if ( db ) sqlite3_close( db );
		return NULL;
	}

	if ( sqlite3_exec( db, PLAYER_SCHEMA_SQL, NULL, NULL, &errmsg ) != SQLITE_OK ) {
		bug( "db_player_open: schema error", 0 );
		if ( errmsg ) sqlite3_free( errmsg );
		sqlite3_close( db );
		return NULL;
	}

	/* WAL mode: eliminates journal file create/delete per transaction.
	 * synchronous=NORMAL: fsync only at WAL checkpoints, not every commit.
	 * Together these reduce write latency ~5-10x vs defaults. */
	sqlite3_exec( db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL );
	sqlite3_exec( db, "PRAGMA synchronous=NORMAL", NULL, NULL, NULL );

	return db;
}


/*
 * Format an integer array as a space-separated string into buffer.
 * Returns number of chars written (excluding null terminator).
 */
static int format_int_array( char *buf, size_t bufsize, const int *arr, int count ) {
	int pos = 0;
	int i;
	for ( i = 0; i < count && pos < (int)bufsize - 12; i++ ) {
		if ( i > 0 )
			buf[pos++] = ' ';
		pos += snprintf( buf + pos, bufsize - pos, "%d", arr[i] );
	}
	buf[pos] = '\0';
	return pos;
}

/*
 * Format a short int array as a space-separated string into buffer.
 */
static int format_short_array( char *buf, size_t bufsize, const int *arr, int count ) {
	int temp[64];
	int i;
	for ( i = 0; i < count && i < 64; i++ )
		temp[i] = arr[i];
	return format_int_array( buf, bufsize, temp, count );
}

/*
 * Batch save all character arrays in a single multi-row INSERT.
 * Much faster than 26 individual INSERT statements.
 */
static void save_all_arrays( sqlite3 *db, CHAR_DATA *ch ) {
	PROFILE_START( "save_arrays" );
	/* Pre-allocated buffers for each array's data string */
	char power[512], stance[256], gifts[256], paradox[64], monkab[64], damcap[64];
	char wpn[128], spl[64], cmbt[128], loc_hp[64], chi_buf[64], focus_buf[64];
	char attr_perm[64], attr_mod[64], cond[64], fake[128];
	char language[64], stage[64], wolfform[64], score[128], genes[128];
	char powers[256], stats[128], disc_a[128];
	char stat_ab[64], stat_am[64], stat_dur[64];
	sqlite3_stmt *stmt;
	int i;

	/* Build temporary arrays for PC_DATA fields */
	int t_attr_perm[5], t_attr_mod[5], t_cond[3], t_fake[8];
	int t_stat_ab[4], t_stat_am[4], t_stat_dur[4];

	t_attr_perm[0] = ch->pcdata->perm_str;
	t_attr_perm[1] = ch->pcdata->perm_int;
	t_attr_perm[2] = ch->pcdata->perm_wis;
	t_attr_perm[3] = ch->pcdata->perm_dex;
	t_attr_perm[4] = ch->pcdata->perm_con;

	t_attr_mod[0] = ch->pcdata->mod_str;
	t_attr_mod[1] = ch->pcdata->mod_int;
	t_attr_mod[2] = ch->pcdata->mod_wis;
	t_attr_mod[3] = ch->pcdata->mod_dex;
	t_attr_mod[4] = ch->pcdata->mod_con;

	t_cond[0] = ch->pcdata->condition[0];
	t_cond[1] = ch->pcdata->condition[1];
	t_cond[2] = ch->pcdata->condition[2];

	t_fake[0] = ch->pcdata->fake_skill;
	t_fake[1] = ch->pcdata->fake_stance;
	t_fake[2] = ch->pcdata->fake_hit;
	t_fake[3] = ch->pcdata->fake_dam;
	t_fake[4] = ch->pcdata->fake_ac;
	t_fake[5] = ch->pcdata->fake_hp;
	t_fake[6] = ch->pcdata->fake_mana;
	t_fake[7] = ch->pcdata->fake_move;

	for ( i = 0; i < 4; i++ ) {
		t_stat_ab[i] = ch->pcdata->stat_ability[i];
		t_stat_am[i] = ch->pcdata->stat_amount[i];
		t_stat_dur[i] = ch->pcdata->stat_duration[i];
	}

	/* Format all arrays to strings */
	format_int_array( power, sizeof(power), ch->power, MAX_DISCIPLINES );
	format_int_array( stance, sizeof(stance), ch->stance, 24 );
	format_int_array( gifts, sizeof(gifts), ch->gifts, 21 );
	format_int_array( paradox, sizeof(paradox), ch->paradox, 3 );
	format_int_array( monkab, sizeof(monkab), ch->monkab, 4 );
	format_int_array( damcap, sizeof(damcap), ch->damcap, 2 );

	format_short_array( wpn, sizeof(wpn), ch->wpn, 13 );
	format_short_array( spl, sizeof(spl), ch->spl, 5 );
	format_short_array( cmbt, sizeof(cmbt), ch->cmbt, 8 );
	format_short_array( loc_hp, sizeof(loc_hp), ch->loc_hp, 7 );
	format_short_array( chi_buf, sizeof(chi_buf), ch->chi, 2 );
	format_short_array( focus_buf, sizeof(focus_buf), ch->focus, 2 );

	format_int_array( attr_perm, sizeof(attr_perm), t_attr_perm, 5 );
	format_int_array( attr_mod, sizeof(attr_mod), t_attr_mod, 5 );
	format_int_array( cond, sizeof(cond), t_cond, 3 );
	format_int_array( fake, sizeof(fake), t_fake, 8 );

	format_int_array( language, sizeof(language), ch->pcdata->language, 2 );
	format_short_array( stage, sizeof(stage), ch->pcdata->stage, 3 );
	format_short_array( wolfform, sizeof(wolfform), ch->pcdata->wolfform, 2 );
	format_int_array( score, sizeof(score), ch->pcdata->score, 6 );
	format_int_array( genes, sizeof(genes), ch->pcdata->genes, 10 );
	format_int_array( powers, sizeof(powers), ch->pcdata->powers, 20 );
	format_int_array( stats, sizeof(stats), ch->pcdata->stats, 12 );
	format_short_array( disc_a, sizeof(disc_a), ch->pcdata->disc_a, 11 );

	format_int_array( stat_ab, sizeof(stat_ab), t_stat_ab, 4 );
	format_int_array( stat_am, sizeof(stat_am), t_stat_am, 4 );
	format_int_array( stat_dur, sizeof(stat_dur), t_stat_dur, 4 );

	/* Single multi-row INSERT for all 26 arrays */
	if ( sqlite3_prepare_v2( db,
		"INSERT INTO player_arrays (name, data) VALUES "
		"('power',?),('stance',?),('gifts',?),('paradox',?),('monkab',?),('damcap',?),"
		"('wpn',?),('spl',?),('cmbt',?),('loc_hp',?),('chi',?),('focus',?),"
		"('attr_perm',?),('attr_mod',?),('condition',?),('fake_con',?),"
		"('language',?),('stage',?),('wolfform',?),('score',?),('genes',?),"
		"('powers',?),('stats',?),('disc_a',?),"
		"('stat_ability',?),('stat_amount',?),('stat_duration',?)",
		-1, &stmt, NULL ) == SQLITE_OK ) {

		sqlite3_bind_text( stmt, 1, power, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 2, stance, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 3, gifts, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 4, paradox, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 5, monkab, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 6, damcap, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 7, wpn, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 8, spl, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 9, cmbt, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 10, loc_hp, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 11, chi_buf, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 12, focus_buf, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 13, attr_perm, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 14, attr_mod, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 15, cond, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 16, fake, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 17, language, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 18, stage, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 19, wolfform, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 20, score, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 21, genes, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 22, powers, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 23, stats, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 24, disc_a, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 25, stat_ab, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 26, stat_am, -1, SQLITE_STATIC );
		sqlite3_bind_text( stmt, 27, stat_dur, -1, SQLITE_STATIC );

		sqlite3_step( stmt );
		sqlite3_finalize( stmt );
	}
	PROFILE_END( "save_arrays" );
}


/*
 * Load an integer array from a space-separated string.
 */
static void load_int_array( const char *data, int *arr, int count ) {
	const char *p = data;
	char *end;
	int i;

	for ( i = 0; i < count; i++ ) {
		arr[i] = (int)strtol( p, &end, 10 );
		if ( end == p )
			break;
		p = end;
	}
}


/*
 * Load a short int array from a space-separated string.
 */
static void load_short_array( const char *data, int *arr, int count ) {
	int temp[64];
	int i;
	load_int_array( data, temp, count < 64 ? count : 64 );
	for ( i = 0; i < count && i < 64; i++ )
		arr[i] = (int)temp[i];
}



/*
 * Initialize player database directory. Called from boot_db().
 */
void db_player_init( void ) {
	char backup_dir[MUD_PATH_MAX];

	/* Initialize mutex and condition variable for background save tracking */
	pthread_mutex_init( &save_mutex, NULL );
	pthread_cond_init( &save_cond, NULL );

	if ( snprintf( mud_db_players_dir, sizeof( mud_db_players_dir ), "%s%splayers",
			mud_db_dir, PATH_SEPARATOR ) >= (int)sizeof( mud_db_players_dir ) ) {
		bug( "db_player_init: path truncated.", 0 );
	}
	ensure_directory( mud_db_players_dir );

	/* Ensure backup subdirectory exists */
	if ( snprintf( backup_dir, sizeof( backup_dir ), "%s%sbackup",
		mud_db_players_dir, PATH_SEPARATOR ) >= (int)sizeof( backup_dir ) ) {
		bug( "db_player_init: backup path truncated.", 0 );
		return;
	}
	ensure_directory( backup_dir );
}


/*
 * Check if a player .db file exists.
 */
bool db_player_exists( const char *name ) {
	char path[MUD_PATH_MAX];
	FILE *fp;

	if ( db_player_path( name, path, sizeof( path ) ) < 0 )
		return FALSE;

	fp = fopen( path, "r" );
	if ( fp ) {
		fclose( fp );
		return TRUE;
	}
	return FALSE;
}

/*
 * Delete a player's database file permanently.
 * Returns TRUE on success, FALSE if file doesn't exist or deletion failed.
 */
bool db_player_delete( const char *name ) {
	char path[MUD_PATH_MAX];
	char log_buf[MAX_STRING_LENGTH];

	if ( db_player_path( name, path, sizeof( path ) ) < 0 )
		return FALSE;

	if ( !db_player_exists( name ) )
		return FALSE;

	if ( remove( path ) == 0 ) {
		snprintf( log_buf, MAX_STRING_LENGTH, "Player file deleted: %s", name );
		log_string( log_buf );
		return TRUE;
	}

	snprintf( log_buf, MAX_STRING_LENGTH, "Failed to delete player file: %s", name );
	log_string( log_buf );
	return FALSE;
}


/*
 * Write all inventory objects for a character to the objects table.
 * Iterates the carrying list iteratively to avoid deep recursion.
 *
 * The old fwrite_obj wrote siblings via recursion on next_content first,
 * then the object, then contains. For SQLite we write in forward order
 * with explicit nest tracking. Objects are inserted in ORDER BY id and
 * reconstructed using nest levels on load.
 */
static void save_objects( sqlite3 *db, CHAR_DATA *ch ) {
	PROFILE_START( "save_objects" );
	sqlite3_stmt *obj_stmt = NULL;
	sqlite3_stmt *aff_stmt = NULL;
	sqlite3_stmt *ed_stmt = NULL;
	const char *obj_sql =
		"INSERT INTO objects (nest, vnum, name, short_descr, description,"
		" chpoweron, chpoweroff, chpoweruse,"
		" victpoweron, victpoweroff, victpoweruse,"
		" questmaker, questowner,"
		" extra_flags, extra_flags2, weapflags, wear_flags, wear_loc,"
		" item_type, weight, spectype, specpower,"
		" condition, toughness, resistance, quest, points,"
		" level, timer, cost, value_0, value_1, value_2, value_3)"
		" VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	const char *aff_sql =
		"INSERT INTO obj_affects (obj_id, duration, modifier, location)"
		" VALUES (?,?,?,?)";
	const char *ed_sql =
		"INSERT INTO obj_extra_descr (obj_id, keyword, description)"
		" VALUES (?,?,?)";

	/*
	 * We use a stack to traverse the object tree iteratively.
	 * Stack holds (OBJ_DATA*, nest_level) pairs.
	 */
	typedef struct {
		OBJ_DATA *obj;
		int nest;
	} obj_entry;

	obj_entry stack[512];
	int top = -1;
	OBJ_DATA *obj;

	if ( sqlite3_prepare_v2( db, obj_sql, -1, &obj_stmt, NULL ) != SQLITE_OK )
		return;
	if ( sqlite3_prepare_v2( db, aff_sql, -1, &aff_stmt, NULL ) != SQLITE_OK ) {
		sqlite3_finalize( obj_stmt );
		return;
	}
	if ( sqlite3_prepare_v2( db, ed_sql, -1, &ed_stmt, NULL ) != SQLITE_OK ) {
		sqlite3_finalize( obj_stmt );
		sqlite3_finalize( aff_stmt );
		return;
	}

	/* Push top-level objects in reverse order so first item pops first */
	{
		OBJ_DATA *list[512];
		int count = 0;
		int i;
		LIST_FOR_EACH( obj, &ch->carrying, OBJ_DATA, content_node ) {
			if ( count < 512 )
				list[count++] = obj;
		}
		for ( i = count - 1; i >= 0; i-- ) {
			top++;
			stack[top].obj = list[i];
			stack[top].nest = 0;
		}
	}

	while ( top >= 0 ) {
		AFFECT_DATA *paf;
		EXTRA_DESCR_DATA *ed;
		sqlite3_int64 obj_id;
		int nest;

		obj = stack[top].obj;
		nest = stack[top].nest;
		top--;

		/* Skip key items and player-object transformations */
		if ( ( obj->chobj != NULL && ( !IS_NPC( obj->chobj )
				&& obj->chobj->pcdata->obj_vnum != 0 ) )
				|| obj->item_type == ITEM_KEY )
			continue;

		/* Insert the object */
		sqlite3_reset( obj_stmt );
		sqlite3_bind_int( obj_stmt, 1, nest );
		sqlite3_bind_int( obj_stmt, 2, obj->pIndexData->vnum );
		sqlite3_bind_text( obj_stmt, 3, safe_str( obj->name ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( obj_stmt, 4, safe_str( obj->short_descr ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( obj_stmt, 5, safe_str( obj->description ), -1, SQLITE_TRANSIENT );

		/* Power strings - save NULL for default/empty
		 * Use str[0] && str[1] instead of strlen() > 1 for O(1) check */
#define BIND_POWER_STR( col, s ) \
		if ( (s) && (s)[0] && (s)[1] && str_cmp( (s), "(null)" ) ) \
			sqlite3_bind_text( obj_stmt, col, (s), -1, SQLITE_TRANSIENT ); \
		else \
			sqlite3_bind_null( obj_stmt, col )

		BIND_POWER_STR( 6, obj->chpoweron );
		BIND_POWER_STR( 7, obj->chpoweroff );
		BIND_POWER_STR( 8, obj->chpoweruse );
		BIND_POWER_STR( 9, obj->victpoweron );
		BIND_POWER_STR( 10, obj->victpoweroff );
		BIND_POWER_STR( 11, obj->victpoweruse );

#undef BIND_POWER_STR

		/* Quest strings - no "(null)" check needed */
		if ( obj->questmaker && obj->questmaker[0] && obj->questmaker[1] )
			sqlite3_bind_text( obj_stmt, 12, obj->questmaker, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 12 );

		if ( obj->questowner && obj->questowner[0] && obj->questowner[1] )
			sqlite3_bind_text( obj_stmt, 13, obj->questowner, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 13 );

		sqlite3_bind_int( obj_stmt, 14, obj->extra_flags );
		sqlite3_bind_int( obj_stmt, 15, obj->extra_flags2 );
		sqlite3_bind_int( obj_stmt, 16, obj->weapflags );
		sqlite3_bind_int( obj_stmt, 17, obj->wear_flags );
		sqlite3_bind_int( obj_stmt, 18, obj->wear_loc );
		sqlite3_bind_int( obj_stmt, 19, obj->item_type );
		sqlite3_bind_int( obj_stmt, 20, obj->weight );
		sqlite3_bind_int( obj_stmt, 21, obj->spectype );
		sqlite3_bind_int( obj_stmt, 22, obj->specpower );
		sqlite3_bind_int( obj_stmt, 23, obj->condition );
		sqlite3_bind_int( obj_stmt, 24, obj->toughness );
		sqlite3_bind_int( obj_stmt, 25, obj->resistance );
		sqlite3_bind_int( obj_stmt, 26, obj->quest );
		sqlite3_bind_int( obj_stmt, 27, obj->points );
		sqlite3_bind_int( obj_stmt, 28, obj->level );
		sqlite3_bind_int( obj_stmt, 29, obj->timer );
		sqlite3_bind_int( obj_stmt, 30, obj->cost );
		sqlite3_bind_int( obj_stmt, 31, obj->value[0] );
		sqlite3_bind_int( obj_stmt, 32, obj->value[1] );
		sqlite3_bind_int( obj_stmt, 33, obj->value[2] );
		sqlite3_bind_int( obj_stmt, 34, obj->value[3] );
		sqlite3_step( obj_stmt );

		obj_id = sqlite3_last_insert_rowid( db );

		/* Object affects */
		LIST_FOR_EACH(paf, &obj->affects, AFFECT_DATA, node) {
			sqlite3_reset( aff_stmt );
			sqlite3_bind_int64( aff_stmt, 1, obj_id );
			sqlite3_bind_int( aff_stmt, 2, paf->duration );
			sqlite3_bind_int( aff_stmt, 3, paf->modifier );
			sqlite3_bind_int( aff_stmt, 4, paf->location );
			sqlite3_step( aff_stmt );
		}

		/* Extra descriptions */
		LIST_FOR_EACH( ed, &obj->extra_descr, EXTRA_DESCR_DATA, node ) {
			sqlite3_reset( ed_stmt );
			sqlite3_bind_int64( ed_stmt, 1, obj_id );
			sqlite3_bind_text( ed_stmt, 2, ed->keyword, -1, SQLITE_TRANSIENT );
			sqlite3_bind_text( ed_stmt, 3, ed->description, -1, SQLITE_TRANSIENT );
			sqlite3_step( ed_stmt );
		}

		/* Push contained objects (reverse order so first pops first) */
		if ( !list_empty( &obj->contents ) ) {
			OBJ_DATA *list[512];
			int count = 0;
			int i;
			OBJ_DATA *c;
			LIST_FOR_EACH( c, &obj->contents, OBJ_DATA, content_node ) {
				if ( count < 512 )
					list[count++] = c;
			}
			for ( i = count - 1; i >= 0; i-- ) {
				if ( top < 511 ) {
					top++;
					stack[top].obj = list[i];
					stack[top].nest = nest + 1;
				}
			}
		}
	}

	sqlite3_finalize( obj_stmt );
	sqlite3_finalize( aff_stmt );
	sqlite3_finalize( ed_stmt );
	PROFILE_END( "save_objects" );
}


/*
 * Internal: Save player data to an already-open database handle.
 * Used by both sync and async save paths.
 */
static void db_player_save_to_db( sqlite3 *db, CHAR_DATA *ch ) {
	sqlite3_stmt *stmt;
	int sn, i;
	AFFECT_DATA *paf;
	ALIAS_DATA *ali;

	PROFILE_START( "save_delete_all" );
	/* Clear all tables for full rewrite (batched for efficiency) */
	sqlite3_exec( db,
		"DELETE FROM player;"
		"DELETE FROM player_arrays;"
		"DELETE FROM skills;"
		"DELETE FROM aliases;"
		"DELETE FROM affects;"
		"DELETE FROM boards;"
		"DELETE FROM objects;"
		"DELETE FROM obj_affects;"
		"DELETE FROM obj_extra_descr",
		NULL, NULL, NULL );
	PROFILE_END( "save_delete_all" );
	/* ================================================================
	 * Player table - single row with all scalar fields
	 * ================================================================ */
	{
		const char *sql =
			"INSERT INTO player ("
			"name, switchname, short_descr, long_descr, objdesc, description,"
			"lord, clan, morph, createtime, lasttime, lasthost,"
			"poweraction, powertype, prompt, cprompt,"
			"password, bamfin, bamfout, title, conception, parents, cparents,"
			"marriage, decapmessage, loginmessage, logoutmessage, avatarmessage,"
			"tiemessage, last_decap_0, last_decap_1,"
			"sex, class, level, trust, played, room_vnum, gold, exp, expgained,"
			"act, extra, newbits, special, affected_by, immune, polyaff, itemaffect,"
			"form, position, practice, saving_throw, alignment,"
			"xhitroll, xdamroll, hitroll, damroll, armor, wimpy, deaf,"
			"beast, home, spectype, specpower,"
			"hit, max_hit, mana, max_mana, move, max_move,"
			"pkill, pdeath, mkill, mdeath, awins, alosses,"
			"warp, warpcount, monkstuff, monkcrap, garou1, garou2,"
			"rage, generation, cur_form, flag2, flag3, flag4, siltol, gnosis_max,"
			"kingdom, quest, rank, bounty, security, jflags, souls,"
			"upgrade_level, mean_paradox, relrank, rune_count, revision,"
			"disc_research, disc_points, obj_vnum, exhaustion, questsrun, questtotal"
			") VALUES ("
			"?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,"  /* 16: strings 1 */
			"?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,"    /* 15: strings 2 */
			"?,?,?,?,?,?,?,?,?,"                /* 9: core ints 1 */
			"?,?,?,?,?,?,?,?,"                  /* 8: flags */
			"?,?,?,?,?,"                        /* 5: form..alignment */
			"?,?,?,?,?,?,?,"                    /* 7: rolls/armor/wimpy/deaf */
			"?,?,?,?,"                          /* 4: beast/home/spec */
			"?,?,?,?,?,?,"                      /* 6: hp/mana/move */
			"?,?,?,?,?,?,"                      /* 6: pk/pd/mk/md/arena */
			"?,?,?,?,?,?,"                      /* 6: warp/monk/garou */
			"?,?,?,?,?,?,?,?,"                  /* 8: rage..gnosis */
			"?,?,?,?,?,?,?,"                    /* 7: kingdom..souls */
			"?,?,?,?,?,"                        /* 5: upgrade..revision */
			"?,?,?,?,?,?"                       /* 6: disc..questtotal */
			")";
		int col = 1;
		int room_vnum;

		if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK ) {
			db_rollback( db );
			sqlite3_close( db );
			return;
		}

		/* Identity strings */
		sqlite3_bind_text( stmt, col++, safe_str( ch->name ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->switchname ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->short_descr ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->long_descr ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->objdesc ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->description ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->lord ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->clan ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->morph ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->createtime ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->lasttime ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->lasthost ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->poweraction ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->powertype ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->prompt ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->cprompt ), -1, SQLITE_TRANSIENT );
		/* PC-only strings */
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->pwd ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->bamfin ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->bamfout ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->title ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->conception ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->parents ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->cparents ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->marriage ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->decapmessage ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->loginmessage ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->logoutmessage ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->avatarmessage ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->tiemessage ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->last_decap[0] ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, col++, safe_str( ch->pcdata->last_decap[1] ), -1, SQLITE_TRANSIENT );

		/* Core ints */
		sqlite3_bind_int( stmt, col++, ch->sex );
		sqlite3_bind_int( stmt, col++, ch->class );
		sqlite3_bind_int( stmt, col++, ch->level );
		sqlite3_bind_int( stmt, col++, ch->trust );
		sqlite3_bind_int( stmt, col++, ch->played + (int)( current_time - ch->logon ) );

		/* Room vnum - same logic as fwrite_char */
		room_vnum = ( ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
				&& ch->was_in_room != NULL )
			? ch->was_in_room->vnum
			: ch->in_room->vnum;
		sqlite3_bind_int( stmt, col++, room_vnum );

		sqlite3_bind_int( stmt, col++, ch->gold );
		sqlite3_bind_int( stmt, col++, ch->exp );
		sqlite3_bind_int( stmt, col++, ch->expgained );
		/* Flags */
		sqlite3_bind_int( stmt, col++, ch->act );
		sqlite3_bind_int( stmt, col++, ch->extra );
		sqlite3_bind_int( stmt, col++, ch->newbits );
		sqlite3_bind_int( stmt, col++, ch->special );
		sqlite3_bind_int( stmt, col++, ch->affected_by );
		sqlite3_bind_int( stmt, col++, ch->immune );
		sqlite3_bind_int( stmt, col++, ch->polyaff );
		sqlite3_bind_int( stmt, col++, ch->itemaffect );
		/* Form through alignment */
		sqlite3_bind_int( stmt, col++, ch->form );
		sqlite3_bind_int( stmt, col++,
			ch->position == POS_FIGHTING ? POS_STANDING : ch->position );
		sqlite3_bind_int( stmt, col++, ch->practice );
		sqlite3_bind_int( stmt, col++, ch->saving_throw );
		sqlite3_bind_int( stmt, col++, ch->alignment );
		/* Rolls/armor/wimpy/deaf */
		sqlite3_bind_int( stmt, col++, ch->xhitroll );
		sqlite3_bind_int( stmt, col++, ch->xdamroll );
		sqlite3_bind_int( stmt, col++, ch->hitroll );
		sqlite3_bind_int( stmt, col++, ch->damroll );
		sqlite3_bind_int( stmt, col++, ch->armor );
		sqlite3_bind_int( stmt, col++, ch->wimpy );
		sqlite3_bind_int( stmt, col++, ch->deaf );
		/* Beast/home/spec */
		sqlite3_bind_int( stmt, col++, ch->beast );
		sqlite3_bind_int( stmt, col++, ch->home );
		sqlite3_bind_int( stmt, col++, ch->spectype );
		sqlite3_bind_int( stmt, col++, ch->specpower );
		/* HP/Mana/Move */
		sqlite3_bind_int( stmt, col++, ch->hit );
		sqlite3_bind_int( stmt, col++, ch->max_hit );
		sqlite3_bind_int( stmt, col++, ch->mana );
		sqlite3_bind_int( stmt, col++, ch->max_mana );
		sqlite3_bind_int( stmt, col++, ch->move );
		sqlite3_bind_int( stmt, col++, ch->max_move );
		/* PK/PD/MK/MD/Arena */
		sqlite3_bind_int( stmt, col++, ch->pkill );
		sqlite3_bind_int( stmt, col++, ch->pdeath );
		sqlite3_bind_int( stmt, col++, ch->mkill );
		sqlite3_bind_int( stmt, col++, ch->mdeath );
		sqlite3_bind_int( stmt, col++, ch->pcdata->awins );
		sqlite3_bind_int( stmt, col++, ch->pcdata->alosses );
		/* Class-specific */
		sqlite3_bind_int( stmt, col++, ch->warp );
		sqlite3_bind_int( stmt, col++, ch->warpcount );
		sqlite3_bind_int( stmt, col++, ch->monkstuff );
		sqlite3_bind_int( stmt, col++, ch->monkcrap );
		sqlite3_bind_int( stmt, col++, ch->garou1 );
		sqlite3_bind_int( stmt, col++, ch->garou2 );
		sqlite3_bind_int( stmt, col++, ch->rage );
		sqlite3_bind_int( stmt, col++, ch->generation );
		sqlite3_bind_int( stmt, col++, ch->cur_form );
		sqlite3_bind_int( stmt, col++, ch->flag2 );
		sqlite3_bind_int( stmt, col++, ch->flag3 );
		sqlite3_bind_int( stmt, col++, ch->flag4 );
		sqlite3_bind_int( stmt, col++, ch->siltol );
		sqlite3_bind_int( stmt, col++, ch->gnosis[GMAXIMUM] );
		/* PC_DATA ints */
		sqlite3_bind_int( stmt, col++, ch->pcdata->kingdom );
		sqlite3_bind_int( stmt, col++, ch->pcdata->quest );
		sqlite3_bind_int( stmt, col++, ch->pcdata->rank );
		sqlite3_bind_int( stmt, col++, ch->pcdata->bounty );
		sqlite3_bind_int( stmt, col++, ch->pcdata->security );
		sqlite3_bind_int( stmt, col++, ch->pcdata->jflags );
		sqlite3_bind_int( stmt, col++, ch->pcdata->souls );
		sqlite3_bind_int( stmt, col++, ch->pcdata->upgrade_level );
		sqlite3_bind_int( stmt, col++, ch->pcdata->mean_paradox_counter );
		sqlite3_bind_int( stmt, col++, ch->pcdata->relrank );
		sqlite3_bind_int( stmt, col++, ch->pcdata->rune_count );
		sqlite3_bind_int( stmt, col++, ch->pcdata->revision );
		sqlite3_bind_int( stmt, col++, ch->pcdata->disc_research );
		sqlite3_bind_int( stmt, col++, ch->pcdata->disc_points );
		sqlite3_bind_int( stmt, col++, ch->pcdata->obj_vnum );
		sqlite3_bind_int( stmt, col++, ch->pcdata->exhaustion );
		sqlite3_bind_int( stmt, col++, ch->pcdata->questsrun );
		sqlite3_bind_int( stmt, col++, ch->pcdata->questtotal );

		sqlite3_step( stmt );
		sqlite3_finalize( stmt );
	}
	PROFILE_END( "save_player_row" );

	/* ================================================================
	 * Integer arrays (batched for efficiency - 27 arrays in 1 INSERT)
	 * ================================================================ */
	save_all_arrays( db, ch );

	PROFILE_START( "save_skills" );
	/* ================================================================
	 * Skills (only non-zero)
	 * ================================================================ */
	{
		const char *sk_sql = "INSERT INTO skills (skill_name, value) VALUES (?,?)";
		if ( sqlite3_prepare_v2( db, sk_sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			for ( sn = 0; sn < MAX_SKILL; sn++ ) {
				if ( skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0 ) {
					sqlite3_reset( stmt );
					sqlite3_bind_text( stmt, 1, skill_table[sn].name, -1, SQLITE_STATIC );
					sqlite3_bind_int( stmt, 2, ch->pcdata->learned[sn] );
					sqlite3_step( stmt );
				}
			}
			sqlite3_finalize( stmt );
		}
	}
	PROFILE_END( "save_skills" );

	PROFILE_START( "save_aliases" );
	/* ================================================================
	 * Aliases
	 * ================================================================ */
	{
		const char *al_sql = "INSERT INTO aliases (short_n, long_n) VALUES (?,?)";
		if ( sqlite3_prepare_v2( db, al_sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			LIST_FOR_EACH( ali, &ch->pcdata->aliases, ALIAS_DATA, node ) {
				sqlite3_reset( stmt );
				sqlite3_bind_text( stmt, 1, ali->short_n, -1, SQLITE_TRANSIENT );
				sqlite3_bind_text( stmt, 2, ali->long_n, -1, SQLITE_TRANSIENT );
				sqlite3_step( stmt );
			}
			sqlite3_finalize( stmt );
		}
	}
	PROFILE_END( "save_aliases" );

	PROFILE_START( "save_affects" );
	/* ================================================================
	 * Character affects
	 * ================================================================ */
	{
		const char *af_sql =
			"INSERT INTO affects (skill_name, duration, modifier, location, bitvector)"
			" VALUES (?,?,?,?,?)";
		if ( sqlite3_prepare_v2( db, af_sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			LIST_FOR_EACH(paf, &ch->affects, AFFECT_DATA, node) {
				if ( paf->type < 0 || paf->type >= MAX_SKILL )
					continue;
				sqlite3_reset( stmt );
				sqlite3_bind_text( stmt, 1, skill_table[paf->type].name, -1, SQLITE_STATIC );
				sqlite3_bind_int( stmt, 2, paf->duration );
				sqlite3_bind_int( stmt, 3, paf->modifier );
				sqlite3_bind_int( stmt, 4, paf->location );
				sqlite3_bind_int( stmt, 5, paf->bitvector );
				sqlite3_step( stmt );
			}
			sqlite3_finalize( stmt );
		}
	}
	PROFILE_END( "save_affects" );

	PROFILE_START( "save_boards" );
	/* ================================================================
	 * Board timestamps (only save non-zero timestamps)
	 * ================================================================ */
	{
		const char *bd_sql = "INSERT INTO boards (board_name, last_note) VALUES (?,?)";
		if ( sqlite3_prepare_v2( db, bd_sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			for ( i = 0; i < MAX_BOARD; i++ ) {
				if ( ch->pcdata->last_note[i] == 0 )
					continue;
				sqlite3_reset( stmt );
				sqlite3_bind_text( stmt, 1, boards[i].short_name, -1, SQLITE_STATIC );
				sqlite3_bind_int64( stmt, 2, (sqlite3_int64)ch->pcdata->last_note[i] );
				sqlite3_step( stmt );
			}
			sqlite3_finalize( stmt );
		}
	}
	PROFILE_END( "save_boards" );

	/* ================================================================
	 * Objects (inventory + equipment)
	 * ================================================================ */
	if ( !list_empty( &ch->carrying ) )
		save_objects( db, ch );

	/* Schema version */
	sqlite3_exec( db, "INSERT OR REPLACE INTO meta (key, value) VALUES ('schema_version', '1')",
		NULL, NULL, NULL );
}


/*
 * Save full character + inventory to SQLite database.
 * Uses background thread for disk I/O to avoid blocking game loop.
 */
void db_player_save( CHAR_DATA *ch ) {
	sqlite3 *db = NULL;
	PLAYER_SAVE_TASK *task = NULL;
	unsigned char *serialized = NULL;
	sqlite3_int64 size = 0;
	char path[MUD_PATH_MAX];
	pthread_t thread;
	pthread_attr_t attr;

	if ( IS_NPC( ch ) || ch->level < 2 )
		return;

	PROFILE_START( "db_player_save" );

	/* Build path for background thread */
	if ( db_player_path( ch->pcdata->switchname, path, sizeof( path ) ) < 0 ) {
		PROFILE_END( "db_player_save" );
		return;
	}

	/* Open in-memory database with schema */
	if ( sqlite3_open( ":memory:", &db ) != SQLITE_OK ) {
		PROFILE_END( "db_player_save" );
		return;
	}

	if ( sqlite3_exec( db, PLAYER_SCHEMA_SQL, NULL, NULL, NULL ) != SQLITE_OK ) {
		sqlite3_close( db );
		PROFILE_END( "db_player_save" );
		return;
	}

	/* Serialize player data to in-memory database */
	db_begin( db );
	db_player_save_to_db( db, ch );
	db_commit( db );

	/* Serialize the in-memory database to a byte buffer */
	serialized = sqlite3_serialize( db, "main", &size, 0 );
	sqlite3_close( db );

	if ( serialized == NULL || size == 0 ) {
		if ( serialized ) sqlite3_free( serialized );
		PROFILE_END( "db_player_save" );
		return;
	}

	/* Create task for background thread */
	task = (PLAYER_SAVE_TASK *)malloc( sizeof( PLAYER_SAVE_TASK ) );
	if ( task == NULL ) {
		sqlite3_free( serialized );
		PROFILE_END( "db_player_save" );
		return;
	}

	strncpy( task->path, path, sizeof( task->path ) - 1 );
	task->path[sizeof( task->path ) - 1] = '\0';
	task->data = serialized;
	task->size = size;

	/* Increment pending saves counter */
	pthread_mutex_lock( &save_mutex );
	pending_saves++;
	pthread_mutex_unlock( &save_mutex );

	/* Launch background thread to write file */
	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

	if ( pthread_create( &thread, &attr, player_save_thread, task ) != 0 ) {
		/* Thread creation failed - do synchronous write as fallback */
		FILE *fp = fopen( path, "wb" );
		if ( fp != NULL ) {
			fwrite( serialized, 1, size, fp );
			fclose( fp );
		}
		sqlite3_free( serialized );
		free( task );

		pthread_mutex_lock( &save_mutex );
		pending_saves--;
		pthread_mutex_unlock( &save_mutex );
	}

	PROFILE_END( "db_player_save" );
}


/*
 * Initialize a fresh CHAR_DATA + PC_DATA for loading.
 * This mirrors the init block in load_char_obj() from save.c.
 * Returns the allocated CHAR_DATA.
 */
CHAR_DATA *init_char_for_load( DESCRIPTOR_DATA *d, char *name ) {
	CHAR_DATA *ch;
	char *strtime;
	int sn;

	ch = calloc( 1, sizeof( *ch ) );
	if ( !ch ) {
		bug( "load_char_obj: calloc failed for ch", 0 );
		exit( 1 );
	}
	clear_char( ch );

	ch->pcdata = calloc( 1, sizeof( *ch->pcdata ) );
	if ( !ch->pcdata ) {
		bug( "load_char_obj: calloc failed for pcdata", 0 );
		exit( 1 );
	}
	/* calloc already zeroes memory, no need for pcdata_zero */
	list_init( &ch->pcdata->aliases );

	d->character = ch;
	ch->desc = d;
	ch->name = str_dup( name );
	ch->pcdata->switchname = str_dup( name );
	ch->act = PLR_BLANK | PLR_COMBINE | PLR_PROMPT;
	ch->pcdata->board = &boards[DEFAULT_BOARD];

	ch->extra = 0;
	ch->deaf = 0;
	ch->special = 0;
	ch->newbits = 0;
	ch->class = 0;
	ch->pcdata->familiar = NULL;
	ch->pcdata->partner = NULL;
	ch->pcdata->propose = NULL;
	ch->pcdata->chobj = NULL;
	ch->pcdata->memorised = NULL;
	ch->pcdata->upgrade_level = 0;
	ch->pcdata->safe_counter = 0;
	ch->pcdata->mean_paradox_counter = 0;
	ch->pcdata->relrank = 0;
	ch->pcdata->rune_count = 0;
	ch->pcdata->revision = 0;
	ch->pcdata->jflags = 0;
	ch->pcdata->tiemessage = str_dup( "" );
	ch->pcdata->decapmessage = str_dup( "" );
	ch->pcdata->avatarmessage = str_dup( "" );
	ch->pcdata->logoutmessage = str_dup( "" );
	ch->pcdata->loginmessage = str_dup( "" );
	ch->pcdata->pwd = str_dup( "" );
	ch->pcdata->bamfin = str_dup( "" );
	ch->pcdata->bamfout = str_dup( "" );
	ch->pcdata->last_decap[0] = str_dup( "" );
	ch->pcdata->last_decap[1] = str_dup( "" );
	ch->pcdata->title = str_dup( "" );
	ch->pcdata->bounty = 0;
	ch->pcdata->stats_dirty = FALSE;
	ch->pcdata->conception = str_dup( "" );
	ch->pcdata->parents = str_dup( "" );
	ch->pcdata->cparents = str_dup( "" );
	ch->pcdata->marriage = str_dup( "" );
	ch->pcdata->disc_research = -1;
	ch->lord = str_dup( "" );
	ch->morph = str_dup( "" );
	ch->pload = str_dup( "" );
	ch->prompt = str_dup( "" );
	ch->cprompt = str_dup( "" );
	strtime = ctime( &current_time );
	strtime[strlen( strtime ) - 1] = '\0';
	free(ch->lasttime);
	ch->createtime = str_dup( strtime );
	ch->lasttime = str_dup( "" );
	ch->lasthost = str_dup( "" );
	ch->poweraction = str_dup( "" );
	ch->powertype = str_dup( "" );
	ch->hunting = str_dup( "" );
	ch->pcdata->followers = 0;
	ch->spectype = 0;
	ch->specpower = 0;
	ch->mounted = 0;
	ch->home = ROOM_VNUM_SCHOOL;
	ch->vampgen_a = 0;
	ch->paradox[0] = 0;
	ch->paradox[1] = 0;
	ch->paradox[2] = 0;
	ch->damcap[0] = 1000;
	ch->damcap[1] = 0;
	ch->vampaff_a = 0;
	ch->itemaffect = 0;
	ch->polyaff = 0;
	ch->immune = 0;
	ch->form = 1048575;
	ch->beast = 15;
	for ( sn = 0; sn < 7; sn++ )
		ch->loc_hp[sn] = 0;
	for ( sn = 0; sn < 13; sn++ )
		ch->wpn[sn] = 0;
	for ( sn = 0; sn < 5; sn++ )
		ch->spl[sn] = 0;
	for ( sn = 0; sn < 8; sn++ )
		ch->cmbt[sn] = 0;
	ch->pkill = 0;
	ch->pdeath = 0;
	ch->mkill = 0;
	ch->mdeath = 0;
	ch->pcdata->followers = 0;
	ch->pcdata->perm_str = 13;
	ch->pcdata->perm_int = 13;
	ch->pcdata->perm_wis = 13;
	ch->pcdata->perm_dex = 13;
	ch->pcdata->perm_con = 13;
	ch->pcdata->quest = 0;
	ch->pcdata->kingdom = 0;
	ch->pcdata->wolf = 0;
	ch->pcdata->rank = 0;
	ch->pcdata->language[0] = 0;
	ch->pcdata->language[1] = 0;
	ch->pcdata->stage[0] = 0;
	ch->pcdata->stage[1] = 0;
	ch->pcdata->stage[2] = 0;
	ch->pcdata->wolfform[0] = 1;
	ch->pcdata->wolfform[1] = 1;
	ch->pcdata->score[0] = 0;
	ch->pcdata->score[1] = 0;
	ch->pcdata->score[2] = 0;
	ch->pcdata->score[3] = 0;
	ch->pcdata->score[4] = 0;
	ch->pcdata->score[5] = 0;
	for ( sn = 0; sn < 11; sn++ )
		ch->pcdata->disc_a[sn] = 0;
	for ( sn = 0; sn < 10; sn++ )
		ch->pcdata->genes[sn] = 0;
	for ( sn = 0; sn < 20; sn++ )
		ch->pcdata->powers[sn] = 0;
	for ( sn = 0; sn < 12; sn++ )
		ch->pcdata->stats[sn] = 0;
	ch->pcdata->security = 0;
	ch->pcdata->fake_skill = 0;
	ch->pcdata->fake_stance = 0;
	ch->pcdata->fake_hit = 0;
	ch->pcdata->fake_dam = 0;
	ch->pcdata->fake_ac = 0;
	ch->pcdata->fake_hp = 0;
	ch->pcdata->fake_mana = 0;
	ch->pcdata->fake_move = 0;
	ch->pcdata->obj_vnum = 0;
	ch->pcdata->condition[COND_THIRST] = 48;
	ch->pcdata->condition[COND_FULL] = 48;
	ch->pcdata->stat_ability[STAT_STR] = 0;
	ch->pcdata->stat_ability[STAT_END] = 0;
	ch->pcdata->stat_ability[STAT_REF] = 0;
	ch->pcdata->stat_ability[STAT_FLE] = 0;
	ch->pcdata->stat_amount[STAT_STR] = 0;
	ch->pcdata->stat_amount[STAT_END] = 0;
	ch->pcdata->stat_amount[STAT_REF] = 0;
	ch->pcdata->stat_amount[STAT_FLE] = 0;
	ch->pcdata->stat_duration[STAT_STR] = 0;
	ch->pcdata->stat_duration[STAT_END] = 0;
	ch->pcdata->stat_duration[STAT_REF] = 0;
	ch->pcdata->stat_duration[STAT_FLE] = 0;
	ch->pcdata->exhaustion = 0;

	return ch;
}


/*
 * Load player scalar fields from the player table.
 */
static void load_player_row( sqlite3 *db, CHAR_DATA *ch ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT "
		"name, switchname, short_descr, long_descr, objdesc, description,"
		"lord, clan, morph, createtime, lasttime, lasthost,"
		"poweraction, powertype, prompt, cprompt,"
		"password, bamfin, bamfout, title, conception, parents, cparents,"
		"marriage, decapmessage, loginmessage, logoutmessage, avatarmessage,"
		"tiemessage, last_decap_0, last_decap_1,"
		"sex, class, level, trust, played, room_vnum, gold, exp, expgained,"
		"act, extra, newbits, special, affected_by, immune, polyaff, itemaffect,"
		"form, position, practice, saving_throw, alignment,"
		"xhitroll, xdamroll, hitroll, damroll, armor, wimpy, deaf,"
		"beast, home, spectype, specpower,"
		"hit, max_hit, mana, max_mana, move, max_move,"
		"pkill, pdeath, mkill, mdeath, awins, alosses,"
		"warp, warpcount, monkstuff, monkcrap, garou1, garou2,"
		"rage, generation, cur_form, flag2, flag3, flag4, siltol, gnosis_max,"
		"kingdom, quest, rank, bounty, security, jflags, souls,"
		"upgrade_level, mean_paradox, relrank, rune_count, revision,"
		"disc_research, disc_points, obj_vnum, exhaustion, questsrun, questtotal"
		" FROM player LIMIT 1";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	if ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		int col = 0;
		int room_vnum;

		/* Identity strings - must free the defaults first */
		free(ch->name);
		ch->name = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->switchname);
		ch->pcdata->switchname = str_dup( col_text( stmt, col++ ) );
		free(ch->short_descr);
		ch->short_descr = str_dup( col_text( stmt, col++ ) );
		free(ch->long_descr);
		ch->long_descr = str_dup( col_text( stmt, col++ ) );
		free(ch->objdesc);
		ch->objdesc = str_dup( col_text( stmt, col++ ) );
		free(ch->description);
		ch->description = str_dup( col_text( stmt, col++ ) );
		free(ch->lord);
		ch->lord = str_dup( col_text( stmt, col++ ) );
		free(ch->clan);
		ch->clan = str_dup( col_text( stmt, col++ ) );
		free(ch->morph);
		ch->morph = str_dup( col_text( stmt, col++ ) );
		free(ch->createtime);
		ch->createtime = str_dup( col_text( stmt, col++ ) );
		free(ch->lasttime);
		ch->lasttime = str_dup( col_text( stmt, col++ ) );
		free(ch->lasthost);
		ch->lasthost = str_dup( col_text( stmt, col++ ) );
		free(ch->poweraction);
		ch->poweraction = str_dup( col_text( stmt, col++ ) );
		free(ch->powertype);
		ch->powertype = str_dup( col_text( stmt, col++ ) );
		free(ch->prompt);
		ch->prompt = str_dup( col_text( stmt, col++ ) );
		free(ch->cprompt);
		ch->cprompt = str_dup( col_text( stmt, col++ ) );
		/* PC-only strings */
		free(ch->pcdata->pwd);
		ch->pcdata->pwd = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->bamfin);
		ch->pcdata->bamfin = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->bamfout);
		ch->pcdata->bamfout = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->title);
		ch->pcdata->title = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->conception);
		ch->pcdata->conception = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->parents);
		ch->pcdata->parents = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->cparents);
		ch->pcdata->cparents = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->marriage);
		ch->pcdata->marriage = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->decapmessage);
		ch->pcdata->decapmessage = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->loginmessage);
		ch->pcdata->loginmessage = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->logoutmessage);
		ch->pcdata->logoutmessage = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->avatarmessage);
		ch->pcdata->avatarmessage = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->tiemessage);
		ch->pcdata->tiemessage = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->last_decap[0]);
		ch->pcdata->last_decap[0] = str_dup( col_text( stmt, col++ ) );
		free(ch->pcdata->last_decap[1]);
		ch->pcdata->last_decap[1] = str_dup( col_text( stmt, col++ ) );

		/* Core ints */
		ch->sex = sqlite3_column_int( stmt, col++ );
		ch->class = sqlite3_column_int( stmt, col++ );
		ch->level = sqlite3_column_int( stmt, col++ );
		ch->trust = sqlite3_column_int( stmt, col++ );
		ch->played = sqlite3_column_int( stmt, col++ );

		room_vnum = sqlite3_column_int( stmt, col++ );
		ch->in_room = get_room_index( room_vnum );
		if ( ch->in_room == NULL )
			ch->in_room = get_room_index( ROOM_VNUM_LIMBO );

		ch->gold = sqlite3_column_int( stmt, col++ );
		ch->exp = sqlite3_column_int( stmt, col++ );
		ch->expgained = sqlite3_column_int( stmt, col++ );
		/* Flags */
		ch->act = sqlite3_column_int( stmt, col++ );
		ch->extra = sqlite3_column_int( stmt, col++ );
		ch->newbits = sqlite3_column_int( stmt, col++ );
		ch->special = sqlite3_column_int( stmt, col++ );
		ch->affected_by = sqlite3_column_int( stmt, col++ );
		ch->immune = sqlite3_column_int( stmt, col++ );
		ch->polyaff = sqlite3_column_int( stmt, col++ );
		ch->itemaffect = sqlite3_column_int( stmt, col++ );
		/* Form through alignment */
		ch->form = sqlite3_column_int( stmt, col++ );
		ch->position = sqlite3_column_int( stmt, col++ );
		ch->practice = sqlite3_column_int( stmt, col++ );
		ch->saving_throw = sqlite3_column_int( stmt, col++ );
		ch->alignment = sqlite3_column_int( stmt, col++ );
		/* Rolls/armor/wimpy/deaf */
		ch->xhitroll = sqlite3_column_int( stmt, col++ );
		ch->xdamroll = sqlite3_column_int( stmt, col++ );
		ch->hitroll = sqlite3_column_int( stmt, col++ );
		ch->damroll = sqlite3_column_int( stmt, col++ );
		ch->armor = sqlite3_column_int( stmt, col++ );
		ch->wimpy = sqlite3_column_int( stmt, col++ );
		ch->deaf = sqlite3_column_int( stmt, col++ );
		/* Beast/home/spec */
		ch->beast = sqlite3_column_int( stmt, col++ );
		ch->home = sqlite3_column_int( stmt, col++ );
		ch->spectype = sqlite3_column_int( stmt, col++ );
		ch->specpower = sqlite3_column_int( stmt, col++ );
		/* HP/Mana/Move */
		ch->hit = sqlite3_column_int( stmt, col++ );
		ch->max_hit = sqlite3_column_int( stmt, col++ );
		ch->mana = sqlite3_column_int( stmt, col++ );
		ch->max_mana = sqlite3_column_int( stmt, col++ );
		ch->move = sqlite3_column_int( stmt, col++ );
		ch->max_move = sqlite3_column_int( stmt, col++ );
		/* PK/PD/MK/MD/Arena */
		ch->pkill = sqlite3_column_int( stmt, col++ );
		ch->pdeath = sqlite3_column_int( stmt, col++ );
		ch->mkill = sqlite3_column_int( stmt, col++ );
		ch->mdeath = sqlite3_column_int( stmt, col++ );
		ch->pcdata->awins = sqlite3_column_int( stmt, col++ );
		ch->pcdata->alosses = sqlite3_column_int( stmt, col++ );
		/* Class-specific */
		ch->warp = sqlite3_column_int( stmt, col++ );
		ch->warpcount = sqlite3_column_int( stmt, col++ );
		ch->monkstuff = sqlite3_column_int( stmt, col++ );
		ch->monkcrap = sqlite3_column_int( stmt, col++ );
		ch->garou1 = sqlite3_column_int( stmt, col++ );
		ch->garou2 = sqlite3_column_int( stmt, col++ );
		ch->rage = sqlite3_column_int( stmt, col++ );
		ch->generation = sqlite3_column_int( stmt, col++ );
		ch->cur_form = sqlite3_column_int( stmt, col++ );
		ch->flag2 = sqlite3_column_int( stmt, col++ );
		ch->flag3 = sqlite3_column_int( stmt, col++ );
		ch->flag4 = sqlite3_column_int( stmt, col++ );
		ch->siltol = sqlite3_column_int( stmt, col++ );
		ch->gnosis[GMAXIMUM] = sqlite3_column_int( stmt, col++ );
		/* PC_DATA ints */
		ch->pcdata->kingdom = sqlite3_column_int( stmt, col++ );
		ch->pcdata->quest = sqlite3_column_int( stmt, col++ );
		ch->pcdata->rank = sqlite3_column_int( stmt, col++ );
		ch->pcdata->bounty = sqlite3_column_int( stmt, col++ );
		ch->pcdata->security = sqlite3_column_int( stmt, col++ );
		ch->pcdata->jflags = sqlite3_column_int( stmt, col++ );
		ch->pcdata->souls = sqlite3_column_int( stmt, col++ );
		ch->pcdata->upgrade_level = sqlite3_column_int( stmt, col++ );
		ch->pcdata->mean_paradox_counter = sqlite3_column_int( stmt, col++ );
		ch->pcdata->relrank = sqlite3_column_int( stmt, col++ );
		ch->pcdata->rune_count = sqlite3_column_int( stmt, col++ );
		ch->pcdata->revision = sqlite3_column_int( stmt, col++ );
		ch->pcdata->disc_research = sqlite3_column_int( stmt, col++ );
		ch->pcdata->disc_points = sqlite3_column_int( stmt, col++ );
		ch->pcdata->obj_vnum = sqlite3_column_int( stmt, col++ );
		ch->pcdata->exhaustion = sqlite3_column_int( stmt, col++ );
		ch->pcdata->questsrun = sqlite3_column_int( stmt, col++ );
		ch->pcdata->questtotal = sqlite3_column_int( stmt, col++ );
	}

	sqlite3_finalize( stmt );
}


/*
 * Load integer arrays from player_arrays table.
 */
static void load_player_arrays( sqlite3 *db, CHAR_DATA *ch ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT name, data FROM player_arrays";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *name = col_text( stmt, 0 );
		const char *data = col_text( stmt, 1 );

		if ( !str_cmp( name, "power" ) )
			load_int_array( data, ch->power, MAX_DISCIPLINES );
		else if ( !str_cmp( name, "stance" ) )
			load_int_array( data, ch->stance, 24 );
		else if ( !str_cmp( name, "gifts" ) )
			load_int_array( data, ch->gifts, 21 );
		else if ( !str_cmp( name, "paradox" ) )
			load_int_array( data, ch->paradox, 3 );
		else if ( !str_cmp( name, "monkab" ) )
			load_int_array( data, ch->monkab, 4 );
		else if ( !str_cmp( name, "damcap" ) )
			load_int_array( data, ch->damcap, 2 );
		else if ( !str_cmp( name, "wpn" ) )
			load_short_array( data, ch->wpn, 13 );
		else if ( !str_cmp( name, "spl" ) )
			load_short_array( data, ch->spl, 5 );
		else if ( !str_cmp( name, "cmbt" ) )
			load_short_array( data, ch->cmbt, 8 );
		else if ( !str_cmp( name, "loc_hp" ) )
			load_short_array( data, ch->loc_hp, 7 );
		else if ( !str_cmp( name, "chi" ) )
			load_short_array( data, ch->chi, 2 );
		else if ( !str_cmp( name, "focus" ) )
			load_short_array( data, ch->focus, 2 );
		else if ( !str_cmp( name, "attr_perm" ) ) {
			int v[5];
			load_int_array( data, v, 5 );
			ch->pcdata->perm_str = v[0];
			ch->pcdata->perm_int = v[1];
			ch->pcdata->perm_wis = v[2];
			ch->pcdata->perm_dex = v[3];
			ch->pcdata->perm_con = v[4];
		}
		else if ( !str_cmp( name, "attr_mod" ) ) {
			int v[5];
			load_int_array( data, v, 5 );
			ch->pcdata->mod_str = v[0];
			ch->pcdata->mod_int = v[1];
			ch->pcdata->mod_wis = v[2];
			ch->pcdata->mod_dex = v[3];
			ch->pcdata->mod_con = v[4];
		}
		else if ( !str_cmp( name, "condition" ) ) {
			int v[3];
			load_int_array( data, v, 3 );
			ch->pcdata->condition[0] = v[0];
			ch->pcdata->condition[1] = v[1];
			ch->pcdata->condition[2] = v[2];
		}
		else if ( !str_cmp( name, "fake_con" ) ) {
			int v[8];
			load_int_array( data, v, 8 );
			ch->pcdata->fake_skill = v[0];
			ch->pcdata->fake_stance = v[1];
			ch->pcdata->fake_hit = v[2];
			ch->pcdata->fake_dam = v[3];
			ch->pcdata->fake_ac = v[4];
			ch->pcdata->fake_hp = v[5];
			ch->pcdata->fake_mana = v[6];
			ch->pcdata->fake_move = v[7];
		}
		else if ( !str_cmp( name, "language" ) )
			load_int_array( data, ch->pcdata->language, 2 );
		else if ( !str_cmp( name, "stage" ) )
			load_short_array( data, ch->pcdata->stage, 3 );
		else if ( !str_cmp( name, "wolfform" ) )
			load_short_array( data, ch->pcdata->wolfform, 2 );
		else if ( !str_cmp( name, "score" ) )
			load_int_array( data, ch->pcdata->score, 6 );
		else if ( !str_cmp( name, "genes" ) )
			load_int_array( data, ch->pcdata->genes, 10 );
		else if ( !str_cmp( name, "powers" ) )
			load_int_array( data, ch->pcdata->powers, 20 );
		else if ( !str_cmp( name, "stats" ) )
			load_int_array( data, ch->pcdata->stats, 12 );
		else if ( !str_cmp( name, "disc_a" ) )
			load_short_array( data, ch->pcdata->disc_a, 11 );
		else if ( !str_cmp( name, "stat_ability" ) ) {
			int v[4];
			load_int_array( data, v, 4 );
			ch->pcdata->stat_ability[0] = v[0];
			ch->pcdata->stat_ability[1] = v[1];
			ch->pcdata->stat_ability[2] = v[2];
			ch->pcdata->stat_ability[3] = v[3];
		}
		else if ( !str_cmp( name, "stat_amount" ) ) {
			int v[4];
			load_int_array( data, v, 4 );
			ch->pcdata->stat_amount[0] = v[0];
			ch->pcdata->stat_amount[1] = v[1];
			ch->pcdata->stat_amount[2] = v[2];
			ch->pcdata->stat_amount[3] = v[3];
		}
		else if ( !str_cmp( name, "stat_duration" ) ) {
			int v[4];
			load_int_array( data, v, 4 );
			ch->pcdata->stat_duration[0] = v[0];
			ch->pcdata->stat_duration[1] = v[1];
			ch->pcdata->stat_duration[2] = v[2];
			ch->pcdata->stat_duration[3] = v[3];
		}
	}

	sqlite3_finalize( stmt );
}


/*
 * Load skills from skills table.
 */
static void load_player_skills( sqlite3 *db, CHAR_DATA *ch ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT skill_name, value FROM skills";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *name = col_text( stmt, 0 );
		int value = sqlite3_column_int( stmt, 1 );
		int sn = skill_lookup( name );
		if ( sn >= 0 )
			ch->pcdata->learned[sn] = value;
	}

	sqlite3_finalize( stmt );
}


/*
 * Load aliases from aliases table.
 */
static void load_player_aliases( sqlite3 *db, CHAR_DATA *ch ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT short_n, long_n FROM aliases ORDER BY id";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		ALIAS_DATA *ali;

		ali = calloc( 1, sizeof( *ali ) );
		if ( !ali ) {
			bug( "load_aliases: calloc failed", 0 );
			exit( 1 );
		}
		ali->short_n = str_dup( col_text( stmt, 0 ) );
		ali->long_n = str_dup( col_text( stmt, 1 ) );
		list_push_front( &ch->pcdata->aliases, &ali->node );
	}

	sqlite3_finalize( stmt );
}


/*
 * Load character affects from affects table.
 */
static void load_player_affects( sqlite3 *db, CHAR_DATA *ch ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT skill_name, duration, modifier, location, bitvector"
		" FROM affects ORDER BY id";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		AFFECT_DATA *paf;
		int sn;

		sn = skill_lookup( col_text( stmt, 0 ) );
		if ( sn < 0 )
			continue;

		paf = calloc( 1, sizeof( *paf ) );
		if ( !paf ) {
			bug( "load_char_affects: calloc failed", 0 );
			exit( 1 );
		}

		paf->type = sn;
		paf->duration = sqlite3_column_int( stmt, 1 );
		paf->modifier = sqlite3_column_int( stmt, 2 );
		paf->location = sqlite3_column_int( stmt, 3 );
		paf->bitvector = sqlite3_column_int( stmt, 4 );
		list_push_front(&ch->affects, &paf->node);
	}

	sqlite3_finalize( stmt );
}


/*
 * Load board timestamps from boards table.
 */
static void load_player_boards( sqlite3 *db, CHAR_DATA *ch ) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT board_name, last_note FROM boards";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *bname = col_text( stmt, 0 );
		int i = board_lookup( bname );
		if ( i != BOARD_NOTFOUND )
			ch->pcdata->last_note[i] = (time_t)sqlite3_column_int64( stmt, 1 );
	}

	sqlite3_finalize( stmt );
}


/*
 * Load inventory objects from objects/obj_affects/obj_extra_descr tables.
 */
static void load_player_objects( sqlite3 *db, CHAR_DATA *ch ) {
	sqlite3_stmt *obj_stmt = NULL;
	sqlite3_stmt *aff_stmt = NULL;
	sqlite3_stmt *ed_stmt = NULL;
	const char *obj_sql =
		"SELECT id, nest, vnum, name, short_descr, description,"
		" chpoweron, chpoweroff, chpoweruse,"
		" victpoweron, victpoweroff, victpoweruse,"
		" questmaker, questowner,"
		" extra_flags, extra_flags2, weapflags, wear_flags, wear_loc,"
		" item_type, weight, spectype, specpower,"
		" condition, toughness, resistance, quest, points,"
		" level, timer, cost, value_0, value_1, value_2, value_3"
		" FROM objects ORDER BY id";
	const char *aff_sql =
		"SELECT duration, modifier, location FROM obj_affects WHERE obj_id=? ORDER BY id";
	const char *ed_sql =
		"SELECT keyword, description FROM obj_extra_descr WHERE obj_id=? ORDER BY id";

	OBJ_DATA *rgObjNest[100];
	int iNest;

	for ( iNest = 0; iNest < 100; iNest++ )
		rgObjNest[iNest] = NULL;

	if ( sqlite3_prepare_v2( db, obj_sql, -1, &obj_stmt, NULL ) != SQLITE_OK )
		return;
	if ( sqlite3_prepare_v2( db, aff_sql, -1, &aff_stmt, NULL ) != SQLITE_OK ) {
		sqlite3_finalize( obj_stmt );
		return;
	}
	if ( sqlite3_prepare_v2( db, ed_sql, -1, &ed_stmt, NULL ) != SQLITE_OK ) {
		sqlite3_finalize( obj_stmt );
		sqlite3_finalize( aff_stmt );
		return;
	}

	while ( sqlite3_step( obj_stmt ) == SQLITE_ROW ) {
		OBJ_DATA *obj;
		sqlite3_int64 obj_id;
		int nest, vnum, col;
		const char *s;

		obj = calloc( 1, sizeof( *obj ) );
		if ( !obj ) {
			bug( "load_char_objects: calloc failed", 0 );
			exit( 1 );
		}
		list_node_init( &obj->obj_node );
		list_node_init( &obj->room_node );
		list_node_init( &obj->content_node );
		list_init( &obj->affects );
		list_init( &obj->contents );

		col = 0;
		obj_id = sqlite3_column_int64( obj_stmt, col++ );
		nest = sqlite3_column_int( obj_stmt, col++ );
		vnum = sqlite3_column_int( obj_stmt, col++ );

		obj->name = str_dup( col_text( obj_stmt, col++ ) );
		obj->short_descr = str_dup( col_text( obj_stmt, col++ ) );
		obj->description = str_dup( col_text( obj_stmt, col++ ) );

		/* Power strings - NULL means use default "(null)" */
		s = (const char *)sqlite3_column_text( obj_stmt, col++ );
		obj->chpoweron = str_dup( s ? s : "(null)" );
		s = (const char *)sqlite3_column_text( obj_stmt, col++ );
		obj->chpoweroff = str_dup( s ? s : "(null)" );
		s = (const char *)sqlite3_column_text( obj_stmt, col++ );
		obj->chpoweruse = str_dup( s ? s : "(null)" );
		s = (const char *)sqlite3_column_text( obj_stmt, col++ );
		obj->victpoweron = str_dup( s ? s : "(null)" );
		s = (const char *)sqlite3_column_text( obj_stmt, col++ );
		obj->victpoweroff = str_dup( s ? s : "(null)" );
		s = (const char *)sqlite3_column_text( obj_stmt, col++ );
		obj->victpoweruse = str_dup( s ? s : "(null)" );

		s = (const char *)sqlite3_column_text( obj_stmt, col++ );
		obj->questmaker = str_dup( s ? s : "" );
		s = (const char *)sqlite3_column_text( obj_stmt, col++ );
		obj->questowner = str_dup( s ? s : "" );

		obj->extra_flags = sqlite3_column_int( obj_stmt, col++ );
		obj->extra_flags2 = sqlite3_column_int( obj_stmt, col++ );
		obj->weapflags = sqlite3_column_int( obj_stmt, col++ );
		obj->wear_flags = sqlite3_column_int( obj_stmt, col++ );
		obj->wear_loc = sqlite3_column_int( obj_stmt, col++ );
		obj->item_type = sqlite3_column_int( obj_stmt, col++ );
		obj->weight = sqlite3_column_int( obj_stmt, col++ );
		obj->spectype = sqlite3_column_int( obj_stmt, col++ );
		obj->specpower = sqlite3_column_int( obj_stmt, col++ );
		obj->condition = sqlite3_column_int( obj_stmt, col++ );
		obj->toughness = sqlite3_column_int( obj_stmt, col++ );
		obj->resistance = sqlite3_column_int( obj_stmt, col++ );
		obj->quest = sqlite3_column_int( obj_stmt, col++ );
		obj->points = sqlite3_column_int( obj_stmt, col++ );
		obj->level = sqlite3_column_int( obj_stmt, col++ );
		obj->timer = sqlite3_column_int( obj_stmt, col++ );
		obj->cost = sqlite3_column_int( obj_stmt, col++ );
		obj->value[0] = sqlite3_column_int( obj_stmt, col++ );
		obj->value[1] = sqlite3_column_int( obj_stmt, col++ );
		obj->value[2] = sqlite3_column_int( obj_stmt, col++ );
		obj->value[3] = sqlite3_column_int( obj_stmt, col++ );

		/* Clamp liquid index for drink containers and fountains */
		if ( ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOUNTAIN )
				&& ( obj->value[2] < 0 || obj->value[2] >= LIQ_MAX ) ) {
			bug( "load_player_objects: bad liquid %d, clamping to water",
				obj->value[2] );
			obj->value[2] = 0;
		}

		/* Look up the object's prototype */
		obj->pIndexData = get_obj_index( vnum );
		if ( !obj->pIndexData )
			obj->pIndexData = get_obj_index( OBJ_VNUM_DUMMY );

		/* Load object affects */
		sqlite3_reset( aff_stmt );
		sqlite3_bind_int64( aff_stmt, 1, obj_id );
		while ( sqlite3_step( aff_stmt ) == SQLITE_ROW ) {
			AFFECT_DATA *paf;
			paf = calloc( 1, sizeof( *paf ) );
			if ( !paf ) {
				bug( "load_char_objects: calloc failed for affect", 0 );
				exit( 1 );
			}
			paf->type = 0;
			paf->duration = sqlite3_column_int( aff_stmt, 0 );
			paf->modifier = sqlite3_column_int( aff_stmt, 1 );
			paf->location = sqlite3_column_int( aff_stmt, 2 );
			paf->bitvector = 0;
			list_push_front(&obj->affects, &paf->node);
		}

		/* Load extra descriptions */
		sqlite3_reset( ed_stmt );
		sqlite3_bind_int64( ed_stmt, 1, obj_id );
		while ( sqlite3_step( ed_stmt ) == SQLITE_ROW ) {
			EXTRA_DESCR_DATA *ed;
			ed = calloc( 1, sizeof( *ed ) );
			if ( !ed ) {
				bug( "load_char_objects: calloc failed for extra_descr", 0 );
				exit( 1 );
			}
			ed->keyword = str_dup( col_text( ed_stmt, 0 ) );
			ed->description = str_dup( col_text( ed_stmt, 1 ) );
			list_push_front( &obj->extra_descr, &ed->node );
		}

		/* Link into global object list */
		list_push_back( &g_objects, &obj->obj_node );
		obj->pIndexData->count++;

		/* Nest into inventory or container */
		if ( nest >= 0 && nest < 100 )
			rgObjNest[nest] = obj;

		if ( nest == 0 || rgObjNest[nest - 1] == NULL )
			obj_to_char( obj, ch );
		else
			obj_to_obj( obj, rgObjNest[nest - 1] );
	}

	sqlite3_finalize( obj_stmt );
	sqlite3_finalize( aff_stmt );
	sqlite3_finalize( ed_stmt );
}


/*
 * Internal load implementation shared by load and load_short.
 * When load_objects is FALSE, skips inventory (for finger lookups).
 */
static bool db_player_load_internal( DESCRIPTOR_DATA *d, char *name,
	bool load_objects )
{
	sqlite3 *db;
	CHAR_DATA *ch;

	if ( !db_player_exists( name ) )
		return FALSE;

	ch = init_char_for_load( d, name );

	db = db_player_open( name );
	if ( !db )
		return FALSE;

	load_player_row( db, ch );
	load_player_arrays( db, ch );
	load_player_skills( db, ch );
	load_player_aliases( db, ch );
	load_player_affects( db, ch );
	load_player_boards( db, ch );

	if ( load_objects )
		load_player_objects( db, ch );

	sqlite3_close( db );
	return TRUE;
}


/*
 * Load full character + inventory from .db file.
 */
bool db_player_load( DESCRIPTOR_DATA *d, char *name ) {
	return db_player_load_internal( d, name, TRUE );
}


/*
 * Load character only (no objects) for finger/short lookups.
 */
bool db_player_load_short( DESCRIPTOR_DATA *d, char *name ) {
	return db_player_load_internal( d, name, FALSE );
}
