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

/* External globals */
extern char mud_db_dir[MUD_PATH_MAX];
extern const struct skill_type skill_table[MAX_SKILL];
extern time_t current_time;

/* Directory for player databases */
static char mud_db_players_dir[MUD_PATH_MAX] = "";


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

	return db;
}


/*
 * Save an integer array as a space-separated string to player_arrays.
 */
static void save_int_array( sqlite3 *db, sqlite3_stmt *stmt,
	const char *name, const int *arr, int count )
{
	char buf[2048];
	int pos = 0;
	int i;

	for ( i = 0; i < count; i++ ) {
		if ( i > 0 )
			buf[pos++] = ' ';
		pos += snprintf( buf + pos, sizeof( buf ) - pos, "%d", arr[i] );
		if ( pos >= (int)sizeof( buf ) - 1 )
			break;
	}
	buf[pos] = '\0';

	sqlite3_reset( stmt );
	sqlite3_bind_text( stmt, 1, name, -1, SQLITE_STATIC );
	sqlite3_bind_text( stmt, 2, buf, -1, SQLITE_TRANSIENT );
	sqlite3_step( stmt );
}


/*
 * Save a short int array as space-separated string.
 */
static void save_short_array( sqlite3 *db, sqlite3_stmt *stmt,
	const char *name, const sh_int *arr, int count )
{
	int temp[64];
	int i;
	for ( i = 0; i < count && i < 64; i++ )
		temp[i] = arr[i];
	save_int_array( db, stmt, name, temp, count );
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
static void load_short_array( const char *data, sh_int *arr, int count ) {
	int temp[64];
	int i;
	load_int_array( data, temp, count < 64 ? count : 64 );
	for ( i = 0; i < count && i < 64; i++ )
		arr[i] = (sh_int)temp[i];
}



/*
 * Initialize player database directory. Called from boot_db().
 */
void db_player_init( void ) {
	if ( snprintf( mud_db_players_dir, sizeof( mud_db_players_dir ), "%s%splayers",
			mud_db_dir, PATH_SEPARATOR ) >= (int)sizeof( mud_db_players_dir ) ) {
		bug( "db_player_init: path truncated.", 0 );
	}
	ensure_directory( mud_db_players_dir );
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
 * Write all inventory objects for a character to the objects table.
 * Iterates the carrying list iteratively to avoid deep recursion.
 *
 * The old fwrite_obj wrote siblings via recursion on next_content first,
 * then the object, then contains. For SQLite we write in forward order
 * with explicit nest tracking. Objects are inserted in ORDER BY id and
 * reconstructed using nest levels on load.
 */
static void save_objects( sqlite3 *db, CHAR_DATA *ch ) {
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
		for ( obj = ch->carrying; obj; obj = obj->next_content ) {
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

		/* Power strings - save NULL for default/empty */
		if ( obj->chpoweron && strlen( obj->chpoweron ) > 1
				&& str_cmp( obj->chpoweron, "(null)" ) )
			sqlite3_bind_text( obj_stmt, 6, obj->chpoweron, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 6 );

		if ( obj->chpoweroff && strlen( obj->chpoweroff ) > 1
				&& str_cmp( obj->chpoweroff, "(null)" ) )
			sqlite3_bind_text( obj_stmt, 7, obj->chpoweroff, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 7 );

		if ( obj->chpoweruse && strlen( obj->chpoweruse ) > 1
				&& str_cmp( obj->chpoweruse, "(null)" ) )
			sqlite3_bind_text( obj_stmt, 8, obj->chpoweruse, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 8 );

		if ( obj->victpoweron && strlen( obj->victpoweron ) > 1
				&& str_cmp( obj->victpoweron, "(null)" ) )
			sqlite3_bind_text( obj_stmt, 9, obj->victpoweron, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 9 );

		if ( obj->victpoweroff && strlen( obj->victpoweroff ) > 1
				&& str_cmp( obj->victpoweroff, "(null)" ) )
			sqlite3_bind_text( obj_stmt, 10, obj->victpoweroff, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 10 );

		if ( obj->victpoweruse && strlen( obj->victpoweruse ) > 1
				&& str_cmp( obj->victpoweruse, "(null)" ) )
			sqlite3_bind_text( obj_stmt, 11, obj->victpoweruse, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 11 );

		if ( obj->questmaker && strlen( obj->questmaker ) > 1 )
			sqlite3_bind_text( obj_stmt, 12, obj->questmaker, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( obj_stmt, 12 );

		if ( obj->questowner && strlen( obj->questowner ) > 1 )
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
		for ( paf = obj->affected; paf; paf = paf->next ) {
			sqlite3_reset( aff_stmt );
			sqlite3_bind_int64( aff_stmt, 1, obj_id );
			sqlite3_bind_int( aff_stmt, 2, paf->duration );
			sqlite3_bind_int( aff_stmt, 3, paf->modifier );
			sqlite3_bind_int( aff_stmt, 4, paf->location );
			sqlite3_step( aff_stmt );
		}

		/* Extra descriptions */
		for ( ed = obj->extra_descr; ed; ed = ed->next ) {
			sqlite3_reset( ed_stmt );
			sqlite3_bind_int64( ed_stmt, 1, obj_id );
			sqlite3_bind_text( ed_stmt, 2, ed->keyword, -1, SQLITE_TRANSIENT );
			sqlite3_bind_text( ed_stmt, 3, ed->description, -1, SQLITE_TRANSIENT );
			sqlite3_step( ed_stmt );
		}

		/* Push contained objects (reverse order so first pops first) */
		if ( obj->contains ) {
			OBJ_DATA *list[512];
			int count = 0;
			int i;
			OBJ_DATA *c;
			for ( c = obj->contains; c; c = c->next_content ) {
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
}


/*
 * Save full character + inventory to SQLite database.
 */
void db_player_save( CHAR_DATA *ch ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	int sn, i;
	AFFECT_DATA *paf;
	ALIAS_DATA *ali;

	if ( IS_NPC( ch ) || ch->level < 2 )
		return;

	db = db_player_open( ch->pcdata->switchname );
	if ( !db )
		return;

	db_begin( db );

	/* Clear all tables for full rewrite */
	sqlite3_exec( db, "DELETE FROM player", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM player_arrays", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM skills", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM aliases", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM affects", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM boards", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM objects", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM obj_affects", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM obj_extra_descr", NULL, NULL, NULL );

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

	/* ================================================================
	 * Integer arrays
	 * ================================================================ */
	{
		const char *arr_sql = "INSERT INTO player_arrays (name, data) VALUES (?,?)";

		if ( sqlite3_prepare_v2( db, arr_sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			/* CHAR_DATA int arrays */
			save_int_array( db, stmt, "power", ch->power, MAX_DISCIPLINES );
			save_int_array( db, stmt, "stance", ch->stance, 24 );
			save_int_array( db, stmt, "gifts", ch->gifts, 21 );
			save_int_array( db, stmt, "paradox", ch->paradox, 3 );
			save_int_array( db, stmt, "monkab", ch->monkab, 4 );
			save_int_array( db, stmt, "damcap", ch->damcap, 2 );

			/* CHAR_DATA sh_int arrays */
			save_short_array( db, stmt, "wpn", ch->wpn, 13 );
			save_short_array( db, stmt, "spl", ch->spl, 5 );
			save_short_array( db, stmt, "cmbt", ch->cmbt, 8 );
			save_short_array( db, stmt, "loc_hp", ch->loc_hp, 7 );
			save_short_array( db, stmt, "chi", ch->chi, 2 );
			save_short_array( db, stmt, "focus", ch->focus, 2 );

			/* PC_DATA arrays */
			{
				int attr_perm[5];
				int attr_mod[5];
				int cond[3];
				int fake[8];

				attr_perm[0] = ch->pcdata->perm_str;
				attr_perm[1] = ch->pcdata->perm_int;
				attr_perm[2] = ch->pcdata->perm_wis;
				attr_perm[3] = ch->pcdata->perm_dex;
				attr_perm[4] = ch->pcdata->perm_con;
				save_int_array( db, stmt, "attr_perm", attr_perm, 5 );

				attr_mod[0] = ch->pcdata->mod_str;
				attr_mod[1] = ch->pcdata->mod_int;
				attr_mod[2] = ch->pcdata->mod_wis;
				attr_mod[3] = ch->pcdata->mod_dex;
				attr_mod[4] = ch->pcdata->mod_con;
				save_int_array( db, stmt, "attr_mod", attr_mod, 5 );

				cond[0] = ch->pcdata->condition[0];
				cond[1] = ch->pcdata->condition[1];
				cond[2] = ch->pcdata->condition[2];
				save_int_array( db, stmt, "condition", cond, 3 );

				fake[0] = ch->pcdata->fake_skill;
				fake[1] = ch->pcdata->fake_stance;
				fake[2] = ch->pcdata->fake_hit;
				fake[3] = ch->pcdata->fake_dam;
				fake[4] = ch->pcdata->fake_ac;
				fake[5] = ch->pcdata->fake_hp;
				fake[6] = ch->pcdata->fake_mana;
				fake[7] = ch->pcdata->fake_move;
				save_int_array( db, stmt, "fake_con", fake, 8 );
			}

			save_int_array( db, stmt, "language", ch->pcdata->language, 2 );
			save_short_array( db, stmt, "stage", ch->pcdata->stage, 3 );
			save_short_array( db, stmt, "wolfform", ch->pcdata->wolfform, 2 );
			save_int_array( db, stmt, "score", ch->pcdata->score, 6 );
			save_int_array( db, stmt, "genes", ch->pcdata->genes, 10 );
			save_int_array( db, stmt, "powers", ch->pcdata->powers, 20 );
			save_int_array( db, stmt, "stats", ch->pcdata->stats, 12 );
			save_short_array( db, stmt, "disc_a", ch->pcdata->disc_a, 11 );

			{
				int stat_ab[4], stat_am[4], stat_dur[4];
				for ( i = 0; i < 4; i++ ) {
					stat_ab[i] = ch->pcdata->stat_ability[i];
					stat_am[i] = ch->pcdata->stat_amount[i];
					stat_dur[i] = ch->pcdata->stat_duration[i];
				}
				save_int_array( db, stmt, "stat_ability", stat_ab, 4 );
				save_int_array( db, stmt, "stat_amount", stat_am, 4 );
				save_int_array( db, stmt, "stat_duration", stat_dur, 4 );
			}

			sqlite3_finalize( stmt );
		}
	}

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

	/* ================================================================
	 * Aliases
	 * ================================================================ */
	{
		const char *al_sql = "INSERT INTO aliases (short_n, long_n) VALUES (?,?)";
		if ( sqlite3_prepare_v2( db, al_sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			for ( ali = ch->pcdata->alias; ali; ali = ali->next ) {
				sqlite3_reset( stmt );
				sqlite3_bind_text( stmt, 1, ali->short_n, -1, SQLITE_TRANSIENT );
				sqlite3_bind_text( stmt, 2, ali->long_n, -1, SQLITE_TRANSIENT );
				sqlite3_step( stmt );
			}
			sqlite3_finalize( stmt );
		}
	}

	/* ================================================================
	 * Character affects
	 * ================================================================ */
	{
		const char *af_sql =
			"INSERT INTO affects (skill_name, duration, modifier, location, bitvector)"
			" VALUES (?,?,?,?,?)";
		if ( sqlite3_prepare_v2( db, af_sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			for ( paf = ch->affected; paf; paf = paf->next ) {
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

	/* ================================================================
	 * Board timestamps
	 * ================================================================ */
	{
		const char *bd_sql = "INSERT INTO boards (board_name, last_note) VALUES (?,?)";
		if ( sqlite3_prepare_v2( db, bd_sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			for ( i = 0; i < MAX_BOARD; i++ ) {
				sqlite3_reset( stmt );
				sqlite3_bind_text( stmt, 1, boards[i].short_name, -1, SQLITE_STATIC );
				sqlite3_bind_int64( stmt, 2, (sqlite3_int64)ch->pcdata->last_note[i] );
				sqlite3_step( stmt );
			}
			sqlite3_finalize( stmt );
		}
	}

	/* ================================================================
	 * Objects (inventory + equipment)
	 * ================================================================ */
	if ( ch->carrying != NULL )
		save_objects( db, ch );

	/* Schema version */
	sqlite3_exec( db, "INSERT OR REPLACE INTO meta (key, value) VALUES ('schema_version', '1')",
		NULL, NULL, NULL );

	db_commit( db );
	sqlite3_close( db );
}


/*
 * Initialize a fresh CHAR_DATA + PC_DATA for loading.
 * This mirrors the init block in load_char_obj() from save.c.
 * Returns the allocated CHAR_DATA.
 */
static CHAR_DATA *init_char_for_load( DESCRIPTOR_DATA *d, char *name ) {
	static PC_DATA pcdata_zero;
	CHAR_DATA *ch;
	char *strtime;
	int sn;

	if ( char_free == NULL ) {
		ch = alloc_perm( sizeof( *ch ) );
	} else {
		ch = char_free;
		char_free = char_free->next;
	}
	clear_char( ch );

	if ( pcdata_free == NULL ) {
		ch->pcdata = alloc_perm( sizeof( *ch->pcdata ) );
	} else {
		ch->pcdata = pcdata_free;
		pcdata_free = pcdata_free->next;
	}
	*ch->pcdata = pcdata_zero;

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
	free_string( ch->lasttime );
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

		/* Identity strings - must free_string the defaults first */
		free_string( ch->name );
		ch->name = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->switchname );
		ch->pcdata->switchname = str_dup( col_text( stmt, col++ ) );
		free_string( ch->short_descr );
		ch->short_descr = str_dup( col_text( stmt, col++ ) );
		free_string( ch->long_descr );
		ch->long_descr = str_dup( col_text( stmt, col++ ) );
		free_string( ch->objdesc );
		ch->objdesc = str_dup( col_text( stmt, col++ ) );
		free_string( ch->description );
		ch->description = str_dup( col_text( stmt, col++ ) );
		free_string( ch->lord );
		ch->lord = str_dup( col_text( stmt, col++ ) );
		free_string( ch->clan );
		ch->clan = str_dup( col_text( stmt, col++ ) );
		free_string( ch->morph );
		ch->morph = str_dup( col_text( stmt, col++ ) );
		free_string( ch->createtime );
		ch->createtime = str_dup( col_text( stmt, col++ ) );
		free_string( ch->lasttime );
		ch->lasttime = str_dup( col_text( stmt, col++ ) );
		free_string( ch->lasthost );
		ch->lasthost = str_dup( col_text( stmt, col++ ) );
		free_string( ch->poweraction );
		ch->poweraction = str_dup( col_text( stmt, col++ ) );
		free_string( ch->powertype );
		ch->powertype = str_dup( col_text( stmt, col++ ) );
		free_string( ch->prompt );
		ch->prompt = str_dup( col_text( stmt, col++ ) );
		free_string( ch->cprompt );
		ch->cprompt = str_dup( col_text( stmt, col++ ) );
		/* PC-only strings */
		free_string( ch->pcdata->pwd );
		ch->pcdata->pwd = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->bamfin );
		ch->pcdata->bamfin = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->bamfout );
		ch->pcdata->bamfout = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->title );
		ch->pcdata->title = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->conception );
		ch->pcdata->conception = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->parents );
		ch->pcdata->parents = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->cparents );
		ch->pcdata->cparents = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->marriage );
		ch->pcdata->marriage = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->decapmessage );
		ch->pcdata->decapmessage = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->loginmessage );
		ch->pcdata->loginmessage = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->logoutmessage );
		ch->pcdata->logoutmessage = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->avatarmessage );
		ch->pcdata->avatarmessage = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->tiemessage );
		ch->pcdata->tiemessage = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->last_decap[0] );
		ch->pcdata->last_decap[0] = str_dup( col_text( stmt, col++ ) );
		free_string( ch->pcdata->last_decap[1] );
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

		if ( alias_free == NULL ) {
			ali = alloc_perm( sizeof( *ali ) );
		} else {
			ali = alias_free;
			alias_free = alias_free->next;
		}
		ali->short_n = str_dup( col_text( stmt, 0 ) );
		ali->long_n = str_dup( col_text( stmt, 1 ) );
		ali->next = ch->pcdata->alias;
		ch->pcdata->alias = ali;
		ch->pcdata->alias_count++;
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

		if ( affect_free == NULL ) {
			paf = alloc_perm( sizeof( *paf ) );
		} else {
			paf = affect_free;
			affect_free = affect_free->next;
		}

		paf->type = sn;
		paf->duration = sqlite3_column_int( stmt, 1 );
		paf->modifier = sqlite3_column_int( stmt, 2 );
		paf->location = sqlite3_column_int( stmt, 3 );
		paf->bitvector = sqlite3_column_int( stmt, 4 );
		paf->next = ch->affected;
		ch->affected = paf;
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
		static OBJ_DATA obj_zero;
		OBJ_DATA *obj;
		sqlite3_int64 obj_id;
		int nest, vnum, col;
		const char *s;

		if ( obj_free == NULL ) {
			obj = alloc_perm( sizeof( *obj ) );
		} else {
			obj = obj_free;
			obj_free = obj_free->next;
		}
		*obj = obj_zero;

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

		/* Look up the object's prototype */
		obj->pIndexData = get_obj_index( vnum );
		if ( !obj->pIndexData )
			obj->pIndexData = get_obj_index( OBJ_VNUM_DUMMY );

		/* Load object affects */
		sqlite3_reset( aff_stmt );
		sqlite3_bind_int64( aff_stmt, 1, obj_id );
		while ( sqlite3_step( aff_stmt ) == SQLITE_ROW ) {
			AFFECT_DATA *paf;
			if ( affect_free == NULL ) {
				paf = alloc_perm( sizeof( *paf ) );
			} else {
				paf = affect_free;
				affect_free = affect_free->next;
			}
			paf->type = 0;
			paf->duration = sqlite3_column_int( aff_stmt, 0 );
			paf->modifier = sqlite3_column_int( aff_stmt, 1 );
			paf->location = sqlite3_column_int( aff_stmt, 2 );
			paf->bitvector = 0;
			paf->next = obj->affected;
			obj->affected = paf;
		}

		/* Load extra descriptions */
		sqlite3_reset( ed_stmt );
		sqlite3_bind_int64( ed_stmt, 1, obj_id );
		while ( sqlite3_step( ed_stmt ) == SQLITE_ROW ) {
			EXTRA_DESCR_DATA *ed;
			if ( extra_descr_free == NULL ) {
				ed = alloc_perm( sizeof( *ed ) );
			} else {
				ed = extra_descr_free;
				extra_descr_free = extra_descr_free->next;
			}
			ed->keyword = str_dup( col_text( ed_stmt, 0 ) );
			ed->description = str_dup( col_text( ed_stmt, 1 ) );
			ed->next = obj->extra_descr;
			obj->extra_descr = ed;
		}

		/* Link into global object list */
		obj->next = object_list;
		object_list = obj;
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
