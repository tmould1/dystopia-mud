/***************************************************************************
 *  db_sql.c - SQLite persistence layer for area data
 *
 *  Provides functions to load and save area data (mobiles, objects, rooms,
 *  resets, shops, specials) from/to per-area SQLite database files stored
 *  in gamedata/db/areas/.
 ***************************************************************************/

#include "db_util.h"
#include "db_sql.h"
#include "../script/script.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef WIN32
#include <dirent.h>
#endif

/* Functions defined in db.c but not declared in merc.h */
extern void assign_area_vnum( int vnum );
extern void new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset );

/* Directory for .db files */
char mud_db_dir[MUD_PATH_MAX] = "";

/* External globals from db.c that we need to update during loading */
extern bool fBootDb;
extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;
extern int top_vnum_mob;
extern int top_vnum_obj;
extern int top_vnum_room;

/* Special item existence flags from db.c */
extern bool CHAOS;
extern bool VISOR;
extern bool DARKNESS;
extern bool SPEED;
extern bool BRACELET;
extern bool TORC;
extern bool ARMOUR;
extern bool CLAWS;
extern bool ITEMAFFMANTIS;
extern bool ITEMAFFENTROPY;

/*
 * Schema SQL for creating tables in a new per-area database.
 */
static const char *SCHEMA_SQL =
	"CREATE TABLE IF NOT EXISTS area ("
	"  name        TEXT NOT NULL,"
	"  builders    TEXT DEFAULT '',"
	"  lvnum       INTEGER NOT NULL,"
	"  uvnum       INTEGER NOT NULL,"
	"  security    INTEGER DEFAULT 3,"
	"  recall      INTEGER DEFAULT 0,"
	"  area_flags  INTEGER DEFAULT 0,"
	"  is_hidden   INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS mobiles ("
	"  vnum        INTEGER PRIMARY KEY,"
	"  player_name TEXT, short_descr TEXT, long_descr TEXT, description TEXT,"
	"  act         INTEGER, affected_by INTEGER, alignment INTEGER,"
	"  level       INTEGER, hitroll INTEGER, ac INTEGER,"
	"  hitnodice   INTEGER, hitsizedice INTEGER, hitplus INTEGER,"
	"  damnodice   INTEGER, damsizedice INTEGER, damplus INTEGER,"
	"  gold        INTEGER, sex INTEGER,"
	"  death_teleport_vnum INTEGER DEFAULT -1,"
	"  death_teleport_msg TEXT DEFAULT NULL,"
	"  death_teleport_msg_room TEXT DEFAULT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS objects ("
	"  vnum        INTEGER PRIMARY KEY,"
	"  name TEXT, short_descr TEXT, description TEXT,"
	"  item_type INTEGER, extra_flags INTEGER, wear_flags INTEGER,"
	"  value0 INTEGER, value1 INTEGER, value2 INTEGER, value3 INTEGER,"
	"  weight INTEGER, cost INTEGER,"
	"  chpoweron TEXT, chpoweroff TEXT, chpoweruse TEXT,"
	"  victpoweron TEXT, victpoweroff TEXT, victpoweruse TEXT,"
	"  spectype INTEGER DEFAULT 0, specpower INTEGER DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS object_affects ("
	"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  obj_vnum   INTEGER NOT NULL REFERENCES objects(vnum),"
	"  location   INTEGER NOT NULL,"
	"  modifier   INTEGER NOT NULL,"
	"  sort_order INTEGER DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS extra_descriptions ("
	"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  owner_type TEXT NOT NULL,"
	"  owner_vnum INTEGER NOT NULL,"
	"  keyword    TEXT NOT NULL,"
	"  description TEXT NOT NULL,"
	"  sort_order INTEGER DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS rooms ("
	"  vnum        INTEGER PRIMARY KEY,"
	"  name TEXT, description TEXT,"
	"  room_flags INTEGER, sector_type INTEGER"
	");"

	"CREATE TABLE IF NOT EXISTS exits ("
	"  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  room_vnum   INTEGER NOT NULL REFERENCES rooms(vnum),"
	"  direction   INTEGER NOT NULL,"
	"  description TEXT DEFAULT '', keyword TEXT DEFAULT '',"
	"  exit_info   INTEGER DEFAULT 0,"
	"  key_vnum    INTEGER DEFAULT -1,"
	"  to_vnum     INTEGER DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS resets ("
	"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  command    TEXT NOT NULL,"
	"  arg1 INTEGER, arg2 INTEGER, arg3 INTEGER,"
	"  sort_order INTEGER DEFAULT 0"
	");"

	"CREATE TABLE IF NOT EXISTS shops ("
	"  keeper_vnum INTEGER PRIMARY KEY,"
	"  buy_type0 INTEGER, buy_type1 INTEGER, buy_type2 INTEGER,"
	"  buy_type3 INTEGER, buy_type4 INTEGER,"
	"  profit_buy INTEGER, profit_sell INTEGER,"
	"  open_hour INTEGER, close_hour INTEGER"
	");"

	"CREATE TABLE IF NOT EXISTS specials ("
	"  mob_vnum      INTEGER PRIMARY KEY,"
	"  spec_fun_name TEXT NOT NULL"
	");"

	"CREATE TABLE IF NOT EXISTS scripts ("
	"  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
	"  owner_type  TEXT NOT NULL,"
	"  owner_vnum  INTEGER NOT NULL,"
	"  trigger     INTEGER NOT NULL,"
	"  name        TEXT NOT NULL DEFAULT '',"
	"  code        TEXT NOT NULL,"
	"  pattern     TEXT DEFAULT NULL,"
	"  chance      INTEGER DEFAULT 0,"
	"  sort_order  INTEGER DEFAULT 0"
	");";


/* Subdirectory for per-area .db files */
static char mud_db_areas_dir[MUD_PATH_MAX] = "";

/*
 * Helper: derive the .db path from an area filename.
 * "midgaard.are" -> "gamedata/db/areas/midgaard.db"
 */
static void db_sql_path( const char *area_filename, char *buf, size_t bufsize ) {
	char basename[256];
	const char *dot;
	size_t len;

	/* Copy the filename and strip the extension */
	dot = strrchr( area_filename, '.' );
	if ( dot ) {
		len = (size_t)( dot - area_filename );
		if ( len >= sizeof( basename ) )
			len = sizeof( basename ) - 1;
		memcpy( basename, area_filename, len );
		basename[len] = '\0';
	} else {
		strncpy( basename, area_filename, sizeof( basename ) - 1 );
		basename[sizeof( basename ) - 1] = '\0';
	}

	if ( snprintf( buf, bufsize, "%s%s%s.db", mud_db_areas_dir, PATH_SEPARATOR, basename )
			>= (int)bufsize ) {
		bug( "db_sql_path: path truncated for '%s'.", 0 );
	}
}

/*
 * Initialize the db directory path. Called from mud_init_paths().
 */
void db_sql_init( void ) {
	if ( snprintf( mud_db_dir, sizeof( mud_db_dir ), "%s%sdb",
			mud_base_dir, PATH_SEPARATOR ) >= (int)sizeof( mud_db_dir ) ) {
		bug( "db_sql_init: mud_db_dir path truncated.", 0 );
	}
	ensure_directory( mud_db_dir );

	/* Create areas/ subdirectory for per-area .db files */
	if ( snprintf( mud_db_areas_dir, sizeof( mud_db_areas_dir ), "%s%sareas",
			mud_db_dir, PATH_SEPARATOR ) >= (int)sizeof( mud_db_areas_dir ) ) {
		bug( "db_sql_init: mud_db_areas_dir path truncated.", 0 );
	}
	ensure_directory( mud_db_areas_dir );
}


/*
 * Helper for qsort: compare two strings by pointer.
 */
static int cmp_strings( const void *a, const void *b ) {
	return strcmp( *(const char **)a, *(const char **)b );
}


/*
 * Scan the areas/ directory for .db files and return a sorted list
 * of filenames (e.g. "midgaard.are"). Caller must free each string
 * and the array with db_sql_free_scan().
 *
 * Returns the number of filenames found, or -1 on error.
 * The filenames are converted from "foo.db" to "foo.are" so they
 * match the area_filename convention used throughout the codebase.
 */
int db_sql_scan_areas( char ***filenames_out ) {
	char **list = NULL;
	int count = 0;
	int capacity = 0;

	*filenames_out = NULL;

	if ( mud_db_areas_dir[0] == '\0' )
		return -1;

#ifdef WIN32
	{
		WIN32_FIND_DATAA fd;
		HANDLE hFind;
		char pattern[MUD_PATH_MAX + 8];

		snprintf( pattern, sizeof( pattern ), "%s%s*.db",
			mud_db_areas_dir, PATH_SEPARATOR );
		hFind = FindFirstFileA( pattern, &fd );
		if ( hFind == INVALID_HANDLE_VALUE )
			return 0;

		do {
			char arebuf[256];
			const char *dot;
			size_t stemlen;

			if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				continue;

			/* Convert "foo.db" -> "foo.are" */
			dot = strrchr( fd.cFileName, '.' );
			stemlen = dot ? (size_t)( dot - fd.cFileName ) : strlen( fd.cFileName );
			if ( stemlen + 5 > sizeof( arebuf ) )
				continue;
			memcpy( arebuf, fd.cFileName, stemlen );
			strcpy( arebuf + stemlen, ".are" );

			if ( count >= capacity ) {
				capacity = capacity ? capacity * 2 : 32;
				list = realloc( list, sizeof( char * ) * capacity );
			}
			list[count++] = str_dup( arebuf );

		} while ( FindNextFileA( hFind, &fd ) );
		FindClose( hFind );
	}
#else
	{
		DIR *dp;
		struct dirent *ep;

		dp = opendir( mud_db_areas_dir );
		if ( !dp )
			return 0;

		while ( ( ep = readdir( dp ) ) != NULL ) {
			char arebuf[256];
			size_t namelen = strlen( ep->d_name );
			size_t stemlen;

			if ( namelen < 4 || strcmp( ep->d_name + namelen - 3, ".db" ) != 0 )
				continue;

			/* Convert "foo.db" -> "foo.are" */
			stemlen = namelen - 3;
			if ( stemlen + 5 > sizeof( arebuf ) )
				continue;
			memcpy( arebuf, ep->d_name, stemlen );
			strcpy( arebuf + stemlen, ".are" );

			if ( count >= capacity ) {
				capacity = capacity ? capacity * 2 : 32;
				list = realloc( list, sizeof( char * ) * capacity );
			}
			list[count++] = str_dup( arebuf );
		}
		closedir( dp );
	}
#endif

	/* Sort alphabetically for deterministic load order */
	if ( list && count > 1 )
		qsort( list, count, sizeof( char * ), cmp_strings );

	*filenames_out = list;
	return count;
}


/*
 * Free the filename list returned by db_sql_scan_areas().
 */
void db_sql_free_scan( char **filenames, int count ) {
	int i;
	if ( !filenames )
		return;
	for ( i = 0; i < count; i++ )
		free(filenames[i]);
	free( filenames );
}


/*
 * Check if a .db file exists for the given area filename.
 */
bool db_sql_area_exists( const char *area_filename ) {
	char path[MUD_PATH_MAX];
	struct stat st;

	if ( mud_db_dir[0] == '\0' )
		return FALSE;

	db_sql_path( area_filename, path, sizeof( path ) );
	return ( stat( path, &st ) == 0 );
}


/*
 * Open a database and ensure the schema exists.
 * Returns NULL on failure.
 */
static sqlite3 *db_sql_open_area( const char *area_filename ) {
	char path[MUD_PATH_MAX];
	sqlite3 *db = NULL;
	char *errmsg = NULL;

	db_sql_path( area_filename, path, sizeof( path ) );

	if ( sqlite3_open( path, &db ) != SQLITE_OK ) {
		bug( "db_sql_open_area: cannot open %s: %s", 0 );
		if ( db )
			sqlite3_close( db );
		return NULL;
	}

	/* Create schema if not present */
	if ( sqlite3_exec( db, SCHEMA_SQL, NULL, NULL, &errmsg ) != SQLITE_OK ) {
		bug( "db_sql_open_area: schema error: %s", 0 );
		if ( errmsg )
			sqlite3_free( errmsg );
		sqlite3_close( db );
		return NULL;
	}

	/* Migrations for existing databases - ignore errors if columns exist */
	sqlite3_exec( db,
		"ALTER TABLE mobiles ADD COLUMN death_teleport_vnum INTEGER DEFAULT -1;",
		NULL, NULL, NULL );
	sqlite3_exec( db,
		"ALTER TABLE mobiles ADD COLUMN death_teleport_msg TEXT DEFAULT NULL;",
		NULL, NULL, NULL );
	sqlite3_exec( db,
		"ALTER TABLE mobiles ADD COLUMN death_teleport_msg_room TEXT DEFAULT NULL;",
		NULL, NULL, NULL );

	return db;
}


/*
 * Internal loaders - each loads one entity type from an open database
 * into the in-memory hash tables, following the same pattern as the
 * text-based loaders in db.c.
 */

static void sql_load_mobiles( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"SELECT vnum, player_name, short_descr, long_descr, description,"
		"  act, affected_by, alignment, level, hitroll, ac,"
		"  hitnodice, hitsizedice, hitplus,"
		"  damnodice, damsizedice, damplus,"
		"  gold, sex, death_teleport_vnum,"
		"  death_teleport_msg, death_teleport_msg_room"
		" FROM mobiles ORDER BY vnum";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		MOB_INDEX_DATA *pMobIndex;
		int vnum = sqlite3_column_int( stmt, 0 );
		int iHash;

		fBootDb = FALSE;
		if ( get_mob_index( vnum ) != NULL ) {
			bug( "sql_load_mobiles: vnum %d duplicated.", vnum );
			fBootDb = TRUE;
			continue;
		}
		fBootDb = TRUE;

		pMobIndex = calloc( 1, sizeof( *pMobIndex ) );
		if ( !pMobIndex ) {
			bug( "load_mobiles: calloc failed", 0 );
			exit( 1 );
		}
		list_init( &pMobIndex->scripts );
		pMobIndex->vnum         = vnum;
		pMobIndex->area         = pArea;
		pMobIndex->player_name  = str_dup( col_text( stmt, 1 ) );
		pMobIndex->short_descr  = str_dup( col_text( stmt, 2 ) );
		pMobIndex->long_descr   = str_dup( col_text( stmt, 3 ) );
		pMobIndex->description  = str_dup( col_text( stmt, 4 ) );

		pMobIndex->long_descr[0]  = toupper( pMobIndex->long_descr[0] );
		pMobIndex->description[0] = toupper( pMobIndex->description[0] );

		pMobIndex->act          = sqlite3_column_int( stmt, 5 ) | ACT_IS_NPC;
		pMobIndex->affected_by  = sqlite3_column_int( stmt, 6 );
		pMobIndex->itemaffect   = 0;
		pMobIndex->pShop        = NULL;
		pMobIndex->alignment    = sqlite3_column_int( stmt, 7 );
		pMobIndex->level        = sqlite3_column_int( stmt, 8 );
		pMobIndex->hitroll      = sqlite3_column_int( stmt, 9 );
		pMobIndex->ac           = sqlite3_column_int( stmt, 10 );
		pMobIndex->hitnodice    = sqlite3_column_int( stmt, 11 );
		pMobIndex->hitsizedice  = sqlite3_column_int( stmt, 12 );
		pMobIndex->hitplus      = sqlite3_column_int( stmt, 13 );
		pMobIndex->damnodice    = sqlite3_column_int( stmt, 14 );
		pMobIndex->damsizedice  = sqlite3_column_int( stmt, 15 );
		pMobIndex->damplus      = sqlite3_column_int( stmt, 16 );
		pMobIndex->gold         = sqlite3_column_int( stmt, 17 );
		pMobIndex->sex          = sqlite3_column_int( stmt, 18 );
		pMobIndex->death_teleport_vnum = sqlite3_column_int( stmt, 19 );
		pMobIndex->death_teleport_msg      = sqlite3_column_type( stmt, 20 ) != SQLITE_NULL
			? str_dup( (const char *) sqlite3_column_text( stmt, 20 ) ) : NULL;
		pMobIndex->death_teleport_msg_room = sqlite3_column_type( stmt, 21 ) != SQLITE_NULL
			? str_dup( (const char *) sqlite3_column_text( stmt, 21 ) ) : NULL;

		iHash = vnum % MAX_KEY_HASH;
		pMobIndex->next = mob_index_hash[iHash];
		mob_index_hash[iHash] = pMobIndex;
		top_mob_index++;
		if ( top_vnum_mob < vnum )
			top_vnum_mob = vnum;
		assign_area_vnum( vnum );

		kill_table[URANGE( 0, pMobIndex->level, MAX_LEVEL - 1 )].number++;
	}

	sqlite3_finalize( stmt );
}


static void sql_load_objects( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	sqlite3_stmt *af_stmt;
	sqlite3_stmt *ed_stmt;
	const char *sql =
		"SELECT vnum, name, short_descr, description,"
		"  item_type, extra_flags, wear_flags,"
		"  value0, value1, value2, value3,"
		"  weight, cost,"
		"  chpoweron, chpoweroff, chpoweruse,"
		"  victpoweron, victpoweroff, victpoweruse,"
		"  spectype, specpower"
		" FROM objects ORDER BY vnum";
	const char *af_sql =
		"SELECT location, modifier FROM object_affects"
		" WHERE obj_vnum = ? ORDER BY sort_order";
	const char *ed_sql =
		"SELECT keyword, description FROM extra_descriptions"
		" WHERE owner_type = 'object' AND owner_vnum = ? ORDER BY sort_order";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;
	sqlite3_prepare_v2( db, af_sql, -1, &af_stmt, NULL );
	sqlite3_prepare_v2( db, ed_sql, -1, &ed_stmt, NULL );

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		OBJ_INDEX_DATA *pObjIndex;
		int vnum = sqlite3_column_int( stmt, 0 );
		int iHash;
		const char *s;

		fBootDb = FALSE;
		if ( get_obj_index( vnum ) != NULL ) {
			bug( "sql_load_objects: vnum %d duplicated.", vnum );
			fBootDb = TRUE;
			continue;
		}
		fBootDb = TRUE;

		pObjIndex = calloc( 1, sizeof( *pObjIndex ) );
		if ( !pObjIndex ) {
			bug( "load_objects: calloc failed", 0 );
			exit( 1 );
		}
		pObjIndex->vnum        = vnum;
		pObjIndex->area        = pArea;
		pObjIndex->name        = str_dup( col_text( stmt, 1 ) );
		pObjIndex->short_descr = str_dup( col_text( stmt, 2 ) );
		pObjIndex->description = str_dup( col_text( stmt, 3 ) );

		pObjIndex->short_descr[0] = tolower( pObjIndex->short_descr[0] );
		pObjIndex->description[0] = toupper( pObjIndex->description[0] );

		pObjIndex->item_type   = sqlite3_column_int( stmt, 4 );
		pObjIndex->extra_flags = sqlite3_column_int( stmt, 5 );
		pObjIndex->wear_flags  = sqlite3_column_int( stmt, 6 );
		pObjIndex->value[0]    = sqlite3_column_int( stmt, 7 );
		pObjIndex->value[1]    = sqlite3_column_int( stmt, 8 );
		pObjIndex->value[2]    = sqlite3_column_int( stmt, 9 );
		pObjIndex->value[3]    = sqlite3_column_int( stmt, 10 );
		pObjIndex->weight      = sqlite3_column_int( stmt, 11 );
		pObjIndex->cost        = sqlite3_column_int( stmt, 12 );

		/* Power strings */
		s = col_text( stmt, 13 );
		pObjIndex->chpoweron   = ( s[0] != '\0' ) ? str_dup( s ) : NULL;
		s = col_text( stmt, 14 );
		pObjIndex->chpoweroff  = ( s[0] != '\0' ) ? str_dup( s ) : NULL;
		s = col_text( stmt, 15 );
		pObjIndex->chpoweruse  = ( s[0] != '\0' ) ? str_dup( s ) : NULL;
		s = col_text( stmt, 16 );
		pObjIndex->victpoweron  = ( s[0] != '\0' ) ? str_dup( s ) : NULL;
		s = col_text( stmt, 17 );
		pObjIndex->victpoweroff = ( s[0] != '\0' ) ? str_dup( s ) : NULL;
		s = col_text( stmt, 18 );
		pObjIndex->victpoweruse = ( s[0] != '\0' ) ? str_dup( s ) : NULL;
		pObjIndex->spectype    = sqlite3_column_int( stmt, 19 );
		pObjIndex->specpower   = sqlite3_column_int( stmt, 20 );

		list_init( &pObjIndex->affects );
		list_init( &pObjIndex->extra_descr );
		list_init( &pObjIndex->scripts );

		/* Load affects */
		if ( af_stmt ) {
			sqlite3_reset( af_stmt );
			sqlite3_bind_int( af_stmt, 1, vnum );
			while ( sqlite3_step( af_stmt ) == SQLITE_ROW ) {
				AFFECT_DATA *paf;

				paf = calloc( 1, sizeof( *paf ) );
				if ( !paf ) {
					bug( "load_objects: calloc failed", 0 );
					exit( 1 );
				}
				paf->type      = -1;
				paf->duration  = -1;
				paf->location  = sqlite3_column_int( af_stmt, 0 );
				paf->modifier  = sqlite3_column_int( af_stmt, 1 );
				paf->bitvector = 0;

				list_push_back( &pObjIndex->affects, &paf->node );
				top_affect++;
			}
		}

		/* Load extra descriptions */
		if ( ed_stmt ) {
			sqlite3_reset( ed_stmt );
			sqlite3_bind_int( ed_stmt, 1, vnum );
			while ( sqlite3_step( ed_stmt ) == SQLITE_ROW ) {
				EXTRA_DESCR_DATA *ed;

				ed = calloc( 1, sizeof( *ed ) );
				if ( !ed ) {
					bug( "load_objects: calloc failed", 0 );
					exit( 1 );
				}
				ed->keyword     = str_dup( col_text( ed_stmt, 0 ) );
				ed->description = str_dup( col_text( ed_stmt, 1 ) );

				list_push_back( &pObjIndex->extra_descr, &ed->node );
				top_ed++;
			}
		}

		/* Special vnum flags */
		if ( vnum == 29503 ) CHAOS = TRUE;
		if ( vnum == 29515 ) VISOR = TRUE;
		if ( vnum == 29512 ) DARKNESS = TRUE;
		if ( vnum == 29505 ) SPEED = TRUE;
		if ( vnum == 29518 ) BRACELET = TRUE;
		if ( vnum == 29504 ) TORC = TRUE;
		if ( vnum == 29514 ) ARMOUR = TRUE;
		if ( vnum == 29516 ) CLAWS = TRUE;
		if ( vnum == 29555 ) ITEMAFFMANTIS = TRUE;
		if ( vnum == 2654 )  ITEMAFFENTROPY = TRUE;
		if ( vnum == 29598 ) ITEMAFFENTROPY = TRUE;

		iHash = vnum % MAX_KEY_HASH;
		pObjIndex->next = obj_index_hash[iHash];
		obj_index_hash[iHash] = pObjIndex;
		top_obj_index++;
		if ( top_vnum_obj < vnum )
			top_vnum_obj = vnum;
		assign_area_vnum( vnum );
	}

	if ( ed_stmt ) sqlite3_finalize( ed_stmt );
	if ( af_stmt ) sqlite3_finalize( af_stmt );
	sqlite3_finalize( stmt );
}


static void sql_load_rooms( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	sqlite3_stmt *exit_stmt;
	sqlite3_stmt *ed_stmt;
	const char *sql =
		"SELECT vnum, name, description, room_flags, sector_type"
		" FROM rooms ORDER BY vnum";
	const char *exit_sql =
		"SELECT direction, description, keyword, exit_info, key_vnum, to_vnum"
		" FROM exits WHERE room_vnum = ? ORDER BY direction";
	const char *ed_sql =
		"SELECT keyword, description FROM extra_descriptions"
		" WHERE owner_type = 'room' AND owner_vnum = ? ORDER BY sort_order";
	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;
	sqlite3_prepare_v2( db, exit_sql, -1, &exit_stmt, NULL );
	sqlite3_prepare_v2( db, ed_sql, -1, &ed_stmt, NULL );

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		ROOM_INDEX_DATA *pRoomIndex;
		int vnum = sqlite3_column_int( stmt, 0 );
		int iHash;
		int door;

		fBootDb = FALSE;
		if ( get_room_index( vnum ) ) {
			bug( "sql_load_rooms: vnum %d duplicated.", vnum );
			fBootDb = TRUE;
			continue;
		}
		fBootDb = TRUE;

		pRoomIndex = calloc( 1, sizeof( *pRoomIndex ) );
		if ( !pRoomIndex ) {
			bug( "load_rooms: calloc failed", 0 );
			exit( 1 );
		}
		list_init( &pRoomIndex->characters );
		list_init( &pRoomIndex->objects );
		list_init( &pRoomIndex->extra_descr );
		list_init( &pRoomIndex->resets );
		pRoomIndex->area        = pArea;
		pRoomIndex->vnum        = vnum;
		pRoomIndex->name        = str_dup( col_text( stmt, 1 ) );
		pRoomIndex->description = str_dup( col_text( stmt, 2 ) );
		pRoomIndex->room_flags  = sqlite3_column_int( stmt, 3 );
		pRoomIndex->sector_type = sqlite3_column_int( stmt, 4 );
		pRoomIndex->light       = 0;
		pRoomIndex->blood       = 0;
		list_init( &pRoomIndex->scripts );

		for ( door = 0; door <= 4; door++ ) {
			pRoomIndex->track[door]     = str_dup( "" );
			pRoomIndex->track_dir[door] = 0;
		}
		for ( door = 0; door <= 5; door++ )
			pRoomIndex->exit[door] = NULL;

		/* Load exits */
		if ( exit_stmt ) {
			sqlite3_reset( exit_stmt );
			sqlite3_bind_int( exit_stmt, 1, vnum );
			while ( sqlite3_step( exit_stmt ) == SQLITE_ROW ) {
				EXIT_DATA *pexit;

				door = sqlite3_column_int( exit_stmt, 0 );
				if ( door < 0 || door > 5 )
					continue;

				pexit = calloc( 1, sizeof( *pexit ) );
				if ( !pexit ) {
					bug( "load_rooms: calloc failed", 0 );
					exit( 1 );
				}
				pexit->description = str_dup( col_text( exit_stmt, 1 ) );
				pexit->keyword     = str_dup( col_text( exit_stmt, 2 ) );
				pexit->exit_info   = sqlite3_column_int( exit_stmt, 3 );
				pexit->rs_flags    = sqlite3_column_int( exit_stmt, 3 );
				pexit->key         = sqlite3_column_int( exit_stmt, 4 );
				pexit->vnum        = sqlite3_column_int( exit_stmt, 5 );

				pRoomIndex->exit[door] = pexit;
				top_exit++;
			}
		}

		/* Load extra descriptions */
		if ( ed_stmt ) {
			sqlite3_reset( ed_stmt );
			sqlite3_bind_int( ed_stmt, 1, vnum );
			while ( sqlite3_step( ed_stmt ) == SQLITE_ROW ) {
				EXTRA_DESCR_DATA *ed;

				ed = calloc( 1, sizeof( *ed ) );
				if ( !ed ) {
					bug( "load_rooms: calloc failed", 0 );
					exit( 1 );
				}
				ed->keyword     = str_dup( col_text( ed_stmt, 0 ) );
				ed->description = str_dup( col_text( ed_stmt, 1 ) );

				list_push_back( &pRoomIndex->extra_descr, &ed->node );
				top_ed++;
			}
		}

		iHash = vnum % MAX_KEY_HASH;
		pRoomIndex->next = room_index_hash[iHash];
		room_index_hash[iHash] = pRoomIndex;

		/* Link room into area's room list for efficient reset iteration */
		pRoomIndex->next_in_area = pArea->room_first;
		pArea->room_first = pRoomIndex;
		pArea->room_count++;

		top_room++;
		if ( top_vnum_room < vnum )
			top_vnum_room = vnum;
		assign_area_vnum( vnum );
	}

	if ( ed_stmt )   sqlite3_finalize( ed_stmt );
	if ( exit_stmt ) sqlite3_finalize( exit_stmt );
	sqlite3_finalize( stmt );
}


static void sql_load_resets( sqlite3 *db ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"SELECT command, arg1, arg2, arg3 FROM resets ORDER BY sort_order";
	int iLastRoom = 0;
	int iLastObj = 0;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		RESET_DATA *pReset;
		ROOM_INDEX_DATA *pRoomIndex;
		EXIT_DATA *pexit;
		const char *cmd_str = col_text( stmt, 0 );
		char letter;

		if ( cmd_str[0] == '\0' )
			continue;
		letter = cmd_str[0];

		pReset = calloc( 1, sizeof( *pReset ) );
		if ( !pReset ) {
			bug( "load_resets: calloc failed", 0 );
			exit( 1 );
		}
		pReset->command = letter;
		pReset->arg1    = sqlite3_column_int( stmt, 1 );
		pReset->arg2    = sqlite3_column_int( stmt, 2 );
		pReset->arg3    = sqlite3_column_int( stmt, 3 );

		switch ( letter ) {
		case 'M':
			get_mob_index( pReset->arg1 );
			if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) ) {
				new_reset( pRoomIndex, pReset );
				iLastRoom = pReset->arg3;
			}
			break;

		case 'O':
			get_obj_index( pReset->arg1 );
			if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) ) {
				new_reset( pRoomIndex, pReset );
				iLastObj = pReset->arg3;
			}
			break;

		case 'P':
			get_obj_index( pReset->arg1 );
			if ( ( pRoomIndex = get_room_index( iLastObj ) ) ) {
				new_reset( pRoomIndex, pReset );
			}
			break;

		case 'G':
		case 'E':
			get_obj_index( pReset->arg1 );
			if ( ( pRoomIndex = get_room_index( iLastRoom ) ) ) {
				new_reset( pRoomIndex, pReset );
				iLastObj = iLastRoom;
			}
			break;

		case 'D':
			pRoomIndex = get_room_index( pReset->arg1 );
			if ( pReset->arg2 >= 0 && pReset->arg2 <= 5
			  && pRoomIndex
			  && ( pexit = pRoomIndex->exit[pReset->arg2] )
			  && IS_SET( pexit->rs_flags, EX_ISDOOR ) ) {
				switch ( pReset->arg3 ) {
				default: break;
				case 1: SET_BIT( pexit->rs_flags, EX_CLOSED ); break;
				case 2: SET_BIT( pexit->rs_flags, EX_CLOSED | EX_LOCKED ); break;
				}
			}
			break;

		case 'R':
			if ( ( pRoomIndex = get_room_index( pReset->arg1 ) ) )
				new_reset( pRoomIndex, pReset );
			break;

		default:
			bug( "sql_load_resets: bad command '%c'.", letter );
			break;
		}
	}

	sqlite3_finalize( stmt );
}


static void sql_load_shops( sqlite3 *db ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"SELECT keeper_vnum, buy_type0, buy_type1, buy_type2, buy_type3,"
		"  buy_type4, profit_buy, profit_sell, open_hour, close_hour"
		" FROM shops ORDER BY keeper_vnum";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		SHOP_DATA *pShop;
		MOB_INDEX_DATA *pMobIndex;

		pShop = calloc( 1, sizeof( *pShop ) );
		if ( !pShop ) {
			bug( "load_shops: calloc failed", 0 );
			exit( 1 );
		}
		pShop->keeper      = sqlite3_column_int( stmt, 0 );
		pShop->buy_type[0] = sqlite3_column_int( stmt, 1 );
		pShop->buy_type[1] = sqlite3_column_int( stmt, 2 );
		pShop->buy_type[2] = sqlite3_column_int( stmt, 3 );
		pShop->buy_type[3] = sqlite3_column_int( stmt, 4 );
		pShop->buy_type[4] = sqlite3_column_int( stmt, 5 );
		pShop->profit_buy  = sqlite3_column_int( stmt, 6 );
		pShop->profit_sell = sqlite3_column_int( stmt, 7 );
		pShop->open_hour   = sqlite3_column_int( stmt, 8 );
		pShop->close_hour  = sqlite3_column_int( stmt, 9 );

		pMobIndex = get_mob_index( pShop->keeper );
		if ( pMobIndex )
			pMobIndex->pShop = pShop;

		top_shop++;
	}

	sqlite3_finalize( stmt );
}


static void sql_load_specials( sqlite3 *db ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"SELECT mob_vnum, spec_fun_name FROM specials ORDER BY mob_vnum";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		MOB_INDEX_DATA *pMobIndex;
		int vnum = sqlite3_column_int( stmt, 0 );
		const char *name = col_text( stmt, 1 );

		pMobIndex = get_mob_index( vnum );
		if ( pMobIndex ) {
			pMobIndex->spec_fun = spec_lookup( name );
			if ( pMobIndex->spec_fun == 0 ) {
				bug( "sql_load_specials: spec_fun for vnum %d not found.",
					vnum );
			}
		}
	}

	sqlite3_finalize( stmt );
}


static void sql_load_scripts( sqlite3 *db ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"SELECT owner_type, owner_vnum, trigger, name, code, pattern, chance"
		" FROM scripts ORDER BY owner_vnum, sort_order";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
		const char *owner_type = col_text( stmt, 0 );
		int vnum        = sqlite3_column_int( stmt, 1 );
		int trigger     = sqlite3_column_int( stmt, 2 );
		const char *name = col_text( stmt, 3 );
		const char *code = col_text( stmt, 4 );
		const char *pat  = ( sqlite3_column_type( stmt, 5 ) != SQLITE_NULL )
			? (const char *) sqlite3_column_text( stmt, 5 ) : NULL;
		int chance      = sqlite3_column_int( stmt, 6 );
		list_head_t *list = NULL;
		SCRIPT_DATA *script;

		if ( !strcmp( owner_type, "mob" ) ) {
			MOB_INDEX_DATA *pMob = get_mob_index( vnum );
			if ( pMob ) list = &pMob->scripts;
		} else if ( !strcmp( owner_type, "obj" ) ) {
			OBJ_INDEX_DATA *pObj = get_obj_index( vnum );
			if ( pObj ) list = &pObj->scripts;
		} else if ( !strcmp( owner_type, "room" ) ) {
			ROOM_INDEX_DATA *pRoom = get_room_index( vnum );
			if ( pRoom ) list = &pRoom->scripts;
		}

		if ( list == NULL ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "sql_load_scripts: cannot find %s vnum %d", owner_type, vnum );
			bug( buf, 0 );
			continue;
		}

		script = calloc( 1, sizeof( *script ) );
		if ( !script ) {
			bug( "sql_load_scripts: calloc failed", 0 );
			exit( 1 );
		}
		script->name    = str_dup( name );
		script->trigger = trigger;
		script->code    = str_dup( code );
		script->pattern = pat ? str_dup( pat ) : NULL;
		script->chance  = chance;

		list_push_back( list, &script->node );
	}

	sqlite3_finalize( stmt );
}


/*
 * Load one complete area from its SQLite database file.
 * This replaces the text-based loading sequence for a single area.
 */
void db_sql_load_area( const char *area_filename ) {
	sqlite3 *db;
	sqlite3_stmt *stmt;
	AREA_DATA *pArea;
	char buf[MAX_STRING_LENGTH];
	const char *sql;

	db = db_sql_open_area( area_filename );
	if ( !db ) {
		bug( "db_sql_load_area: cannot open database for %s", 0 );
		return;
	}

	/* Load area metadata */
	sql = "SELECT name, builders, lvnum, uvnum, security, recall, area_flags, is_hidden"
		  " FROM area LIMIT 1";

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK
	  || sqlite3_step( stmt ) != SQLITE_ROW ) {
		bug( "db_sql_load_area: no area row in %s", 0 );
		if ( stmt ) sqlite3_finalize( stmt );
		sqlite3_close( db );
		return;
	}

	pArea = calloc( 1, sizeof( *pArea ) );
	if ( !pArea ) {
		bug( "load_area_from_db: calloc failed", 0 );
		exit( 1 );
	}
	top_area++;
	pArea->name      = str_dup( col_text( stmt, 0 ) );
	pArea->builders  = str_dup( col_text( stmt, 1 ) );
	pArea->lvnum     = sqlite3_column_int( stmt, 2 );
	pArea->uvnum     = sqlite3_column_int( stmt, 3 );
	pArea->security  = sqlite3_column_int( stmt, 4 );
	pArea->recall    = sqlite3_column_int( stmt, 5 );
	pArea->area_flags = sqlite3_column_int( stmt, 6 );
	pArea->is_hidden = sqlite3_column_int( stmt, 7 ) ? TRUE : FALSE;
	pArea->filename  = str_dup( area_filename );
	pArea->age       = 15;
	pArea->nplayer   = 0;
	pArea->vnum      = 0;
	sqlite3_finalize( stmt );

	/* Link area into global area list */
	list_push_back( &g_areas, &pArea->node );
	area_last = pArea;

	snprintf( buf, sizeof( buf ), "  [%5d-%5d] %s (sqlite)",
		pArea->lvnum, pArea->uvnum, pArea->name );
	log_string( buf );

	/* Phase 1: Load entity definitions (mobiles, objects, rooms).
	 * Resets, shops, and specials are deferred to db_sql_link_area()
	 * so all vnums from all areas are available for cross-references. */
	sql_load_mobiles( db, pArea );
	sql_load_objects( db, pArea );
	sql_load_rooms( db, pArea );

	sqlite3_close( db );
}


/*
 * Link phase: load resets, shops, and specials for one area.
 * Called after ALL areas have been loaded so cross-area vnum
 * references (e.g. a reset spawning a mob from another area) resolve.
 */
void db_sql_link_area( const char *area_filename ) {
	sqlite3 *db;

	db = db_sql_open_area( area_filename );
	if ( !db ) {
		bug( "db_sql_link_area: cannot open database for %s", 0 );
		return;
	}

	sql_load_resets( db );
	sql_load_shops( db );
	sql_load_specials( db );
	sql_load_scripts( db );

	sqlite3_close( db );
}


/*
 * Save helpers - insert data from in-memory structures into SQLite.
 */

static void sql_save_mobiles( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO mobiles (vnum, player_name, short_descr, long_descr,"
		"  description, act, affected_by, alignment, level, hitroll, ac,"
		"  hitnodice, hitsizedice, hitplus,"
		"  damnodice, damsizedice, damplus, gold, sex, death_teleport_vnum,"
		"  death_teleport_msg, death_teleport_msg_room)"
		" VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	int vnum;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ ) {
		MOB_INDEX_DATA *pMobIndex = get_mob_index( vnum );
		if ( !pMobIndex || pMobIndex->area != pArea )
			continue;

		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, pMobIndex->vnum );
		sqlite3_bind_text( stmt, 2, pMobIndex->player_name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 3, pMobIndex->short_descr, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 4, pMobIndex->long_descr, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 5, pMobIndex->description, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( stmt, 6, pMobIndex->act );
		sqlite3_bind_int( stmt, 7, pMobIndex->affected_by );
		sqlite3_bind_int( stmt, 8, pMobIndex->alignment );
		sqlite3_bind_int( stmt, 9, pMobIndex->level );
		sqlite3_bind_int( stmt, 10, pMobIndex->hitroll );
		sqlite3_bind_int( stmt, 11, pMobIndex->ac );
		sqlite3_bind_int( stmt, 12, pMobIndex->hitnodice );
		sqlite3_bind_int( stmt, 13, pMobIndex->hitsizedice );
		sqlite3_bind_int( stmt, 14, pMobIndex->hitplus );
		sqlite3_bind_int( stmt, 15, pMobIndex->damnodice );
		sqlite3_bind_int( stmt, 16, pMobIndex->damsizedice );
		sqlite3_bind_int( stmt, 17, pMobIndex->damplus );
		sqlite3_bind_int( stmt, 18, pMobIndex->gold );
		sqlite3_bind_int( stmt, 19, pMobIndex->sex );
		sqlite3_bind_int( stmt, 20, pMobIndex->death_teleport_vnum );
		if ( pMobIndex->death_teleport_msg )
			sqlite3_bind_text( stmt, 21, pMobIndex->death_teleport_msg, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( stmt, 21 );
		if ( pMobIndex->death_teleport_msg_room )
			sqlite3_bind_text( stmt, 22, pMobIndex->death_teleport_msg_room, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( stmt, 22 );

		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
}


static void sql_save_objects( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	sqlite3_stmt *af_stmt;
	sqlite3_stmt *ed_stmt;
	const char *sql =
		"INSERT INTO objects (vnum, name, short_descr, description,"
		"  item_type, extra_flags, wear_flags,"
		"  value0, value1, value2, value3,"
		"  weight, cost,"
		"  chpoweron, chpoweroff, chpoweruse,"
		"  victpoweron, victpoweroff, victpoweruse,"
		"  spectype, specpower)"
		" VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	const char *af_sql =
		"INSERT INTO object_affects (obj_vnum, location, modifier, sort_order)"
		" VALUES (?,?,?,?)";
	const char *ed_sql =
		"INSERT INTO extra_descriptions (owner_type, owner_vnum, keyword,"
		"  description, sort_order)"
		" VALUES ('object',?,?,?,?)";
	int vnum;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;
	sqlite3_prepare_v2( db, af_sql, -1, &af_stmt, NULL );
	sqlite3_prepare_v2( db, ed_sql, -1, &ed_stmt, NULL );

	for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ ) {
		OBJ_INDEX_DATA *pObjIndex = get_obj_index( vnum );
		AFFECT_DATA *paf;
		EXTRA_DESCR_DATA *ed;
		int order;

		if ( !pObjIndex || pObjIndex->area != pArea )
			continue;

		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, pObjIndex->vnum );
		sqlite3_bind_text( stmt, 2, pObjIndex->name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 3, pObjIndex->short_descr, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 4, pObjIndex->description, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( stmt, 5, pObjIndex->item_type );
		sqlite3_bind_int( stmt, 6, pObjIndex->extra_flags );
		sqlite3_bind_int( stmt, 7, pObjIndex->wear_flags );
		sqlite3_bind_int( stmt, 8, pObjIndex->value[0] );
		sqlite3_bind_int( stmt, 9, pObjIndex->value[1] );
		sqlite3_bind_int( stmt, 10, pObjIndex->value[2] );
		sqlite3_bind_int( stmt, 11, pObjIndex->value[3] );
		sqlite3_bind_int( stmt, 12, pObjIndex->weight );
		sqlite3_bind_int( stmt, 13, pObjIndex->cost );
		db_bind_text_or_null( stmt, 14, pObjIndex->chpoweron );
		db_bind_text_or_null( stmt, 15, pObjIndex->chpoweroff );
		db_bind_text_or_null( stmt, 16, pObjIndex->chpoweruse );
		db_bind_text_or_null( stmt, 17, pObjIndex->victpoweron );
		db_bind_text_or_null( stmt, 18, pObjIndex->victpoweroff );
		db_bind_text_or_null( stmt, 19, pObjIndex->victpoweruse );
		sqlite3_bind_int( stmt, 20, pObjIndex->spectype );
		sqlite3_bind_int( stmt, 21, pObjIndex->specpower );
		sqlite3_step( stmt );

		/* Save affects */
		order = 0;
		LIST_FOR_EACH( paf, &pObjIndex->affects, AFFECT_DATA, node ) {
			sqlite3_reset( af_stmt );
			sqlite3_bind_int( af_stmt, 1, vnum );
			sqlite3_bind_int( af_stmt, 2, paf->location );
			sqlite3_bind_int( af_stmt, 3, paf->modifier );
			sqlite3_bind_int( af_stmt, 4, order++ );
			sqlite3_step( af_stmt );
		}

		/* Save extra descriptions */
		order = 0;
		LIST_FOR_EACH( ed, &pObjIndex->extra_descr, EXTRA_DESCR_DATA, node ) {
			sqlite3_reset( ed_stmt );
			sqlite3_bind_int( ed_stmt, 1, vnum );
			sqlite3_bind_text( ed_stmt, 2, ed->keyword, -1, SQLITE_TRANSIENT );
			sqlite3_bind_text( ed_stmt, 3, ed->description, -1, SQLITE_TRANSIENT );
			sqlite3_bind_int( ed_stmt, 4, order++ );
			sqlite3_step( ed_stmt );
		}
	}

	if ( ed_stmt ) sqlite3_finalize( ed_stmt );
	if ( af_stmt ) sqlite3_finalize( af_stmt );
	sqlite3_finalize( stmt );
}


static void sql_save_rooms( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	sqlite3_stmt *exit_stmt;
	sqlite3_stmt *ed_stmt;
	const char *sql =
		"INSERT INTO rooms (vnum, name, description, room_flags, sector_type)"
		" VALUES (?,?,?,?,?)";
	const char *exit_sql =
		"INSERT INTO exits (room_vnum, direction, description, keyword,"
		"  exit_info, key_vnum, to_vnum)"
		" VALUES (?,?,?,?,?,?,?)";
	const char *ed_sql =
		"INSERT INTO extra_descriptions (owner_type, owner_vnum, keyword,"
		"  description, sort_order)"
		" VALUES ('room',?,?,?,?)";
	int vnum;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;
	sqlite3_prepare_v2( db, exit_sql, -1, &exit_stmt, NULL );
	sqlite3_prepare_v2( db, ed_sql, -1, &ed_stmt, NULL );

	for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ ) {
		ROOM_INDEX_DATA *pRoomIndex = get_room_index( vnum );
		EXTRA_DESCR_DATA *ed;
		int door, order;

		if ( !pRoomIndex || pRoomIndex->area != pArea )
			continue;

		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, pRoomIndex->vnum );
		sqlite3_bind_text( stmt, 2, pRoomIndex->name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 3, pRoomIndex->description, -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( stmt, 4, pRoomIndex->room_flags );
		sqlite3_bind_int( stmt, 5, pRoomIndex->sector_type );
		sqlite3_step( stmt );

		/* Save exits */
		for ( door = 0; door <= 5; door++ ) {
			EXIT_DATA *pexit = pRoomIndex->exit[door];
			if ( !pexit )
				continue;

			sqlite3_reset( exit_stmt );
			sqlite3_bind_int( exit_stmt, 1, vnum );
			sqlite3_bind_int( exit_stmt, 2, door );
			sqlite3_bind_text( exit_stmt, 3, pexit->description, -1, SQLITE_TRANSIENT );
			sqlite3_bind_text( exit_stmt, 4, pexit->keyword, -1, SQLITE_TRANSIENT );
			sqlite3_bind_int( exit_stmt, 5, pexit->rs_flags );
			sqlite3_bind_int( exit_stmt, 6, pexit->key );
			sqlite3_bind_int( exit_stmt, 7, pexit->vnum );
			sqlite3_step( exit_stmt );
		}

		/* Save extra descriptions */
		order = 0;
		LIST_FOR_EACH( ed, &pRoomIndex->extra_descr, EXTRA_DESCR_DATA, node ) {
			sqlite3_reset( ed_stmt );
			sqlite3_bind_int( ed_stmt, 1, vnum );
			sqlite3_bind_text( ed_stmt, 2, ed->keyword, -1, SQLITE_TRANSIENT );
			sqlite3_bind_text( ed_stmt, 3, ed->description, -1, SQLITE_TRANSIENT );
			sqlite3_bind_int( ed_stmt, 4, order++ );
			sqlite3_step( ed_stmt );
		}

	}

	if ( ed_stmt )   sqlite3_finalize( ed_stmt );
	if ( exit_stmt ) sqlite3_finalize( exit_stmt );
	sqlite3_finalize( stmt );
}


static void sql_save_resets( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO resets (command, arg1, arg2, arg3, sort_order)"
		" VALUES (?,?,?,?,?)";
	int vnum, order = 0;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ ) {
		ROOM_INDEX_DATA *pRoomIndex = get_room_index( vnum );
		RESET_DATA *pReset;
		char cmd[2];

		if ( !pRoomIndex || pRoomIndex->area != pArea )
			continue;

		LIST_FOR_EACH( pReset, &pRoomIndex->resets, RESET_DATA, node ) {
			cmd[0] = pReset->command;
			cmd[1] = '\0';

			sqlite3_reset( stmt );
			sqlite3_bind_text( stmt, 1, cmd, 1, SQLITE_TRANSIENT );
			sqlite3_bind_int( stmt, 2, pReset->arg1 );
			sqlite3_bind_int( stmt, 3, pReset->arg2 );
			sqlite3_bind_int( stmt, 4, pReset->arg3 );
			sqlite3_bind_int( stmt, 5, order++ );
			sqlite3_step( stmt );
		}
	}

	sqlite3_finalize( stmt );
}


static void sql_save_shops( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO shops (keeper_vnum, buy_type0, buy_type1, buy_type2,"
		"  buy_type3, buy_type4, profit_buy, profit_sell, open_hour, close_hour)"
		" VALUES (?,?,?,?,?,?,?,?,?,?)";
	int vnum;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ ) {
		MOB_INDEX_DATA *pMobIndex = get_mob_index( vnum );
		SHOP_DATA *pShop;

		if ( !pMobIndex || pMobIndex->area != pArea || !pMobIndex->pShop )
			continue;

		pShop = pMobIndex->pShop;
		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, pShop->keeper );
		sqlite3_bind_int( stmt, 2, pShop->buy_type[0] );
		sqlite3_bind_int( stmt, 3, pShop->buy_type[1] );
		sqlite3_bind_int( stmt, 4, pShop->buy_type[2] );
		sqlite3_bind_int( stmt, 5, pShop->buy_type[3] );
		sqlite3_bind_int( stmt, 6, pShop->buy_type[4] );
		sqlite3_bind_int( stmt, 7, pShop->profit_buy );
		sqlite3_bind_int( stmt, 8, pShop->profit_sell );
		sqlite3_bind_int( stmt, 9, pShop->open_hour );
		sqlite3_bind_int( stmt, 10, pShop->close_hour );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
}


static void sql_save_specials( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO specials (mob_vnum, spec_fun_name) VALUES (?,?)";
	int vnum;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ ) {
		MOB_INDEX_DATA *pMobIndex = get_mob_index( vnum );

		if ( !pMobIndex || pMobIndex->area != pArea || !pMobIndex->spec_fun )
			continue;

		sqlite3_reset( stmt );
		sqlite3_bind_int( stmt, 1, vnum );
		sqlite3_bind_text( stmt, 2, spec_string( pMobIndex->spec_fun ), -1,
			SQLITE_TRANSIENT );
		sqlite3_step( stmt );
	}

	sqlite3_finalize( stmt );
}


static void sql_save_scripts_for_list( sqlite3_stmt *stmt,
	const char *owner_type, int vnum, list_head_t *scripts ) {
	SCRIPT_DATA *script;
	int order = 0;

	LIST_FOR_EACH( script, scripts, SCRIPT_DATA, node ) {
		sqlite3_reset( stmt );
		sqlite3_bind_text( stmt, 1, owner_type, -1, SQLITE_STATIC );
		sqlite3_bind_int( stmt, 2, vnum );
		sqlite3_bind_int( stmt, 3, script->trigger );
		sqlite3_bind_text( stmt, 4, script->name, -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( stmt, 5, script->code, -1, SQLITE_TRANSIENT );
		if ( script->pattern )
			sqlite3_bind_text( stmt, 6, script->pattern, -1, SQLITE_TRANSIENT );
		else
			sqlite3_bind_null( stmt, 6 );
		sqlite3_bind_int( stmt, 7, script->chance );
		sqlite3_bind_int( stmt, 8, order++ );
		sqlite3_step( stmt );
	}
}


static void sql_save_scripts( sqlite3 *db, AREA_DATA *pArea ) {
	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO scripts (owner_type, owner_vnum, trigger, name,"
		"  code, pattern, chance, sort_order)"
		" VALUES (?,?,?,?,?,?,?,?)";
	int vnum;

	if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) != SQLITE_OK )
		return;

	for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ ) {
		MOB_INDEX_DATA *pMob = get_mob_index( vnum );
		if ( pMob && pMob->area == pArea && !list_empty( &pMob->scripts ) )
			sql_save_scripts_for_list( stmt, "mob", vnum, &pMob->scripts );

		OBJ_INDEX_DATA *pObj = get_obj_index( vnum );
		if ( pObj && pObj->area == pArea && !list_empty( &pObj->scripts ) )
			sql_save_scripts_for_list( stmt, "obj", vnum, &pObj->scripts );

		ROOM_INDEX_DATA *pRoom = get_room_index( vnum );
		if ( pRoom && pRoom->area == pArea && !list_empty( &pRoom->scripts ) )
			sql_save_scripts_for_list( stmt, "room", vnum, &pRoom->scripts );
	}

	sqlite3_finalize( stmt );
}


/*
 * Save one area to its SQLite database file.
 * Deletes all existing data and re-inserts from in-memory structures.
 * Wrapped in a single transaction for atomicity.
 */
void db_sql_save_area( AREA_DATA *pArea ) {
	sqlite3 *db;

	if ( mud_db_dir[0] == '\0' )
		return;

	db = db_sql_open_area( pArea->filename );
	if ( !db )
		return;

	/* Begin transaction */
	db_begin( db );

	/* Delete all existing data */
	sqlite3_exec( db, "DELETE FROM scripts", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM specials", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM shops", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM resets", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM exits", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM extra_descriptions", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM object_affects", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM rooms", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM objects", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM mobiles", NULL, NULL, NULL );
	sqlite3_exec( db, "DELETE FROM area", NULL, NULL, NULL );

	/* Insert area metadata */
	{
		sqlite3_stmt *stmt;
		const char *sql =
			"INSERT INTO area (name, builders, lvnum, uvnum, security, recall, area_flags, is_hidden)"
			" VALUES (?,?,?,?,?,?,?,?)";

		if ( sqlite3_prepare_v2( db, sql, -1, &stmt, NULL ) == SQLITE_OK ) {
			sqlite3_bind_text( stmt, 1, pArea->name, -1, SQLITE_TRANSIENT );
			sqlite3_bind_text( stmt, 2, pArea->builders, -1, SQLITE_TRANSIENT );
			sqlite3_bind_int( stmt, 3, pArea->lvnum );
			sqlite3_bind_int( stmt, 4, pArea->uvnum );
			sqlite3_bind_int( stmt, 5, pArea->security );
			sqlite3_bind_int( stmt, 6, pArea->recall );
			sqlite3_bind_int( stmt, 7, pArea->area_flags );
			sqlite3_bind_int( stmt, 8, pArea->is_hidden ? 1 : 0 );
			sqlite3_step( stmt );
			sqlite3_finalize( stmt );
		}
	}

	/* Insert all entity data */
	sql_save_mobiles( db, pArea );
	sql_save_objects( db, pArea );
	sql_save_rooms( db, pArea );
	sql_save_resets( db, pArea );
	sql_save_shops( db, pArea );
	sql_save_specials( db, pArea );
	sql_save_scripts( db, pArea );

	/* Commit transaction */
	db_commit( db );

	sqlite3_close( db );
}
