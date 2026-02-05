/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "../systems/mcmp.h"
#include "../systems/profile.h"
#include "../db/db_sql.h"
#include "../db/db_game.h"
#include "../db/db_player.h"

/*
 * Path management - allows executable to run from any directory
 */
char mud_base_dir[MUD_PATH_MAX] = "";
char mud_run_dir[MUD_PATH_MAX] = "";
char mud_log_dir[MUD_PATH_MAX] = "";

/* Build a path from directory and filename - uses rotating static buffers */
char *mud_path( const char *dir, const char *filename ) {
	static char buffers[4][MUD_PATH_MAX];
	static int idx = 0;
	char *buf;

	buf = buffers[idx];
	idx = ( idx + 1 ) % 4;

	if ( dir && dir[0] ) {
		snprintf( buf, MUD_PATH_MAX, "%s%s%s", dir, PATH_SEPARATOR, filename );
	} else {
		snprintf( buf, MUD_PATH_MAX, "%s", filename );
	}
	return buf;
}

/* Initialize all paths based on executable location */
void mud_init_paths( const char *exe_path ) {
	char *last_sep;
	char exe_dir[MUD_PATH_MAX];
	char gamedata_dir[MUD_PATH_MAX];

	/* Get directory containing executable */
	strncpy( exe_dir, exe_path, MUD_PATH_MAX - 1 );
	exe_dir[MUD_PATH_MAX - 1] = '\0';

#if defined( WIN32 )
	last_sep = strrchr( exe_dir, '\\' );
	if ( !last_sep ) last_sep = strrchr( exe_dir, '/' );
#else
	last_sep = strrchr( exe_dir, '/' );
#endif

	if ( last_sep ) {
		*last_sep = '\0'; /* Remove executable name, keep directory */
	} else {
		strcpy( exe_dir, "." ); /* No path separator, use current dir */
	}

	/* Convert to absolute path to ensure consistency across copyover */
#if defined( WIN32 )
	{
		char abs_path[MUD_PATH_MAX];
		if ( _fullpath( abs_path, exe_dir, MUD_PATH_MAX ) != NULL ) {
			strncpy( exe_dir, abs_path, MUD_PATH_MAX - 1 );
			exe_dir[MUD_PATH_MAX - 1] = '\0';
		}
	}
#else
	{
		/* realpath() on POSIX requires a PATH_MAX buffer (typically 4096 bytes).
		 * Pass NULL to let it allocate memory, then copy the result. */
		char *abs_path = realpath( exe_dir, NULL );
		if ( abs_path != NULL ) {
			strncpy( exe_dir, abs_path, MUD_PATH_MAX - 1 );
			exe_dir[MUD_PATH_MAX - 1] = '\0';
			free( abs_path );
		}
	}
#endif

	/* Base dir is where the executable lives (gamedata/)
	 * Limit to 450 chars to guarantee room for subdirs + filenames
	 * The executable is now placed directly in gamedata/, so all
	 * data directories are siblings of the executable.
	 */
	{
		size_t max_base = 450;
		size_t len = strlen( exe_dir );
		if ( len > max_base ) len = max_base;
		memcpy( mud_base_dir, exe_dir, len );
		mud_base_dir[len] = '\0';
	}

	/* gamedata_dir is the same as mud_base_dir since exe is in gamedata/ */
	strncpy( gamedata_dir, mud_base_dir, MUD_PATH_MAX - 1 );
	gamedata_dir[MUD_PATH_MAX - 1] = '\0';

	/* Set up all subdirectories relative to gamedata (exe location) */
	snprintf( mud_run_dir, sizeof( mud_run_dir ), "%.450s%srun", gamedata_dir, PATH_SEPARATOR );
	snprintf( mud_log_dir, sizeof( mud_log_dir ), "%.450s%slog", gamedata_dir, PATH_SEPARATOR );

	/* Ensure runtime directories exist */
	ensure_directory( mud_run_dir );
	ensure_directory( mud_log_dir );

	/* Initialize SQLite database directories */
	db_sql_init();
	db_player_init();

	/* Log path initialization - uses log_string() to write to both stderr and log file */
	log_string( "MUD paths initialized:" );
	{
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ), "  Base:     %s", mud_base_dir );
		log_string( buf );
		snprintf( buf, sizeof( buf ), "  Run:      %s", mud_run_dir );
		log_string( buf );
		snprintf( buf, sizeof( buf ), "  Log:      %s", mud_log_dir );
		log_string( buf );
	}
}

/*
 * Globals.
 */
SHOP_DATA *shop_first;
SHOP_DATA *shop_last;
DUMMY_ARG *dummy_free;
DUMMY_ARG *dummy_list;
CHAR_DATA *char_free;
EXTRA_DESCR_DATA *extra_descr_free;
NOTE_DATA *note_free;
OBJ_DATA *obj_free;
PC_DATA *pcdata_free;
BAN_DATA *ban_free;

char bug_buf[MAX_STRING_LENGTH];
CHAR_DATA *char_list;
char *help_greeting;
char log_buf[MAX_STRING_LENGTH];
KILL_DATA kill_table[MAX_LEVEL];
OBJ_DATA *object_list;
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;
sh_int gsn_stuntubes;
sh_int gsn_thrownpie;
sh_int gsn_bash;
sh_int gsn_smack;
sh_int gsn_thwack;
sh_int gsn_mocha;
sh_int gsn_plasma;
sh_int gsn_telekinetic;
sh_int gsn_potato;
sh_int gsn_laser;
sh_int gsn_backstab;
sh_int gsn_shred;
sh_int gsn_quills;
sh_int gsn_stinger;
sh_int gsn_bladespin;
sh_int gsn_fiery;
sh_int gsn_hooves;
sh_int gsn_fireball;
sh_int gsn_tentacle;
sh_int gsn_lightning;
sh_int gsn_supreme;
sh_int gsn_deathaura;
sh_int gsn_wrathofgod;
sh_int gsn_claws;
sh_int gsn_heavenlyaura;
sh_int gsn_mageshield;
sh_int gsn_breath;
sh_int gsn_darktendrils;
sh_int gsn_cheapshot;
sh_int gsn_spit;
sh_int gsn_venomtong;
sh_int gsn_spiketail;
sh_int gsn_badbreath;
sh_int gsn_magma;
sh_int gsn_hellfire;
sh_int gsn_shards;
sh_int gsn_hide;
sh_int gsn_peek;
sh_int gsn_pick_lock;
sh_int gsn_sneak;
sh_int gsn_steal;
sh_int gsn_spiderform;
sh_int gsn_garotte;
sh_int gsn_disarm;
sh_int gsn_tendrils;
sh_int gsn_berserk;
sh_int gsn_punch;
sh_int gsn_headbutt;
sh_int gsn_spiket;
sh_int gsn_venomt;
sh_int gsn_shards;
sh_int gsn_magma;
sh_int gsn_shiroken;
sh_int gsn_inferno;
sh_int gsn_blinky;
sh_int gsn_fangs;
sh_int gsn_buffet;
sh_int gsn_sweep;
sh_int gsn_knee;
sh_int gsn_lightningslash;
sh_int gsn_rfangs;
sh_int gsn_thrustkick;
sh_int gsn_spinkick;
sh_int gsn_backfist;
sh_int gsn_elbow;
sh_int gsn_shinkick;
sh_int gsn_palmstrike;
sh_int gsn_lightningkick;
sh_int gsn_tornadokick;
sh_int gsn_jumpkick;
sh_int gsn_spinkick;
sh_int gsn_monksweep;
sh_int gsn_circle;
sh_int gsn_booming;
sh_int gsn_chillhand;
sh_int gsn_kick;
sh_int gsn_hurl;
sh_int gsn_rescue;
sh_int gsn_track;
sh_int gsn_polymorph;
sh_int gsn_web;
sh_int gsn_drowfire;
sh_int gsn_infirmity;
sh_int gsn_spew;
sh_int gsn_blindness;
sh_int gsn_charm_person;
sh_int gsn_curse;
sh_int gsn_invis;
sh_int gsn_mass_invis;
sh_int gsn_poison;
sh_int gsn_sleep;
sh_int gsn_multiplearms;

sh_int gsn_darkness;
sh_int gsn_paradox;

/*
 * Locals.
 */
MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
char *string_hash[MAX_KEY_HASH];

AREA_DATA *area_first;
AREA_DATA *area_last;

ROOM_INDEX_DATA *room_list;

HELP_DATA *first_help;
HELP_DATA *last_help;

char *string_space;
char *top_string;
char str_empty[1];

int top_affect;
int top_area;
int top_rt;
int top_ed;
int top_exit;
int top_help;
int top_mob_index;
int top_obj_index;
int top_reset;
int top_room;
int top_shop;
int top_vnum_room; /* OLC */
int top_vnum_mob;  /* OLC */
int top_vnum_obj;  /* OLC */

bool CHAOS = FALSE;
bool VISOR = FALSE;
bool DARKNESS = FALSE;
bool SPEED = FALSE;
bool BRACELET = FALSE;
bool TORC = FALSE;
bool ARMOUR = FALSE;
bool CLAWS = FALSE;
bool ITEMAFFMANTIS = FALSE;
bool ITEMAFFENTROPY = FALSE;
/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.

#define			MAX_STRING	1048576

 */

#define MAX_STRING	   2072864
#define MAX_PERM_BLOCK 131072
#define MAX_MEM_LIST   11

/* MEM_DEBUG: heap corruption detection via canary bytes, free list pointer
 * validation, and freed memory poisoning.
 * Auto-enabled in Visual Studio Debug builds (_DEBUG).
 * For Linux/Makefile: make MEM_DEBUG=1
 * To force on everywhere: uncomment the #define below. */
/* #define MEM_DEBUG */
#if defined( _DEBUG ) && !defined( MEM_DEBUG )
#define MEM_DEBUG
#endif

void *rgFreeList[MAX_MEM_LIST];
const int rgSizeList[MAX_MEM_LIST] =
	{
		16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768 - 64 };

int nAllocString;
int sAllocString;
int nAllocPerm;
int sAllocPerm;

/*
 * Semi-locals.
 */
bool fBootDb;

/*
 * Local booting procedures.
 */
void init_mm args( (void) );


void fix_exits args( (void) );

void reset_area args( ( AREA_DATA * pArea ) );

/*
 * Big mama top level function.
 */
void boot_db( bool fCopyOver ) {

	/*
	 * Init some data space stuff.
	 */
	{
		if ( ( string_space = calloc( 1, MAX_STRING ) ) == NULL ) {
			bug( "Boot_db: can't alloc %d string space.", MAX_STRING );
			exit( 1 );
		}
		top_string = string_space;
		fBootDb = TRUE;
	}

	/*
	 * Init random number generator.
	 */
	{
		init_mm();
	}

	/*
	 * Init profiling system.
	 */
	{
		profile_init();
	}

	/*
	 * Set time and weather.
	 */
	{
		long lhour, lday, lmonth;

		lhour = (long) ( ( current_time - 650336715 ) / ( PULSE_TICK / PULSE_PER_SECOND ) );
		time_info.hour = lhour % 24;
		lday = lhour / 24;
		time_info.day = lday % 35;
		lmonth = lday / 35;
		time_info.month = lmonth % 17;
		time_info.year = lmonth / 17;
		time_info.tick = 0;

		if ( time_info.hour < 5 )
			weather_info.sunlight = SUN_DARK;
		else if ( time_info.hour < 6 )
			weather_info.sunlight = SUN_RISE;
		else if ( time_info.hour < 19 )
			weather_info.sunlight = SUN_LIGHT;
		else if ( time_info.hour < 20 )
			weather_info.sunlight = SUN_SET;
		else
			weather_info.sunlight = SUN_DARK;

		weather_info.change = 0;
		weather_info.mmhg = 960;
		if ( time_info.month >= 7 && time_info.month <= 12 )
			weather_info.mmhg += number_range( 1, 50 );
		else
			weather_info.mmhg += number_range( 1, 80 );

		if ( weather_info.mmhg <= 980 )
			weather_info.sky = SKY_LIGHTNING;
		else if ( weather_info.mmhg <= 1000 )
			weather_info.sky = SKY_RAINING;
		else if ( weather_info.mmhg <= 1020 )
			weather_info.sky = SKY_CLOUDY;
		else
			weather_info.sky = SKY_CLOUDLESS;
	}

	/*
	 * Assign gsn's for skills which have them.
	 */
	{
		int sn;

		for ( sn = 0; sn < MAX_SKILL; sn++ ) {
			if ( skill_table[sn].pgsn != NULL )
				*skill_table[sn].pgsn = sn;
		}
	}

	/*
	 * Initialize game databases and load help entries.
	 * Must happen before area loading since help_greeting is needed early.
	 */
	db_game_init();
	db_class_init();
	db_game_load_helps();

	/*
	 * Load all areas from SQLite .db files in gamedata/db/areas/.
	 * Phase 1: Load area definitions (mobiles, objects, rooms).
	 * Phase 2: Link resets, shops, specials (may reference cross-area vnums).
	 */
	{
		char **area_files;
		int area_count, i;

		area_count = db_sql_scan_areas( &area_files );
		if ( area_count <= 0 ) {
			bug( "Boot_db: no area .db files found.", 0 );
			exit( 1 );
		}

		for ( i = 0; i < area_count; i++ ) {
			db_sql_load_area( area_files[i] );
		}

		for ( i = 0; i < area_count; i++ ) {
			db_sql_link_area( area_files[i] );
		}

		db_sql_free_scan( area_files, area_count );
	}

	/*
	 * Fix up exits.
	 * Declare db booting over.
	 * Reset all areas once.
	 * Load up the notes file.
	 */
	{
		fix_exits();
		fBootDb = FALSE;
		area_update();

		/* Initial population: Reset all areas once to spawn mobiles before players connect.
		 * This bypasses the deferred reset logic which would skip empty areas. */
		{
			AREA_DATA *pArea;
			for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
				reset_area( pArea );
				pArea->needs_reset = FALSE;
			}
		}

		calculate_all_area_difficulties();
		load_bans();
		load_topboard();
		load_leaderboard();
		load_kingdoms();
		load_boards();
		save_notes();
		load_disabled();
		load_gameconfig();
		load_balance();
		load_ability_config();
		db_class_load_registry();  /* Must load first - other class tables reference it */
		db_class_load_display();
		db_class_load_aura();
		db_class_load_armor();
		db_class_load_starting();
		db_class_load_score();
		db_class_load_vnum_ranges();
		db_game_close_boot_connections();
	}

	if ( fCopyOver )
		copyover_recover();

	return;
}


/*
 * Sets vnum range for area using OLC protection features.
 */
void assign_area_vnum( int vnum ) {
	if ( area_last->lvnum == 0 || area_last->uvnum == 0 )
		area_last->lvnum = area_last->uvnum = vnum;
	if ( vnum != URANGE( area_last->lvnum, vnum, area_last->uvnum ) ) {
		if ( vnum < area_last->lvnum ) {
			area_last->lvnum = vnum;
		} else {
			area_last->uvnum = vnum;
		}
	}
	return;
}

/*
 * Returns an uppercase string.
 */
char *strupper( const char *str ) {
	static char strup[MAX_STRING_LENGTH];
	int i;

	for ( i = 0; str[i] != '\0'; i++ )
		strup[i] = UPPER( str[i] );
	strup[i] = '\0';
	return strup;
}

/*
 * Adds a help page to the list if it is not a duplicate of an existing page.
 * Page is insert-sorted by keyword.			-Thoric
 * (The reason for sorting is to keep do_hlist looking nice)
 */
void add_help( HELP_DATA *pHelp ) {
	HELP_DATA *tHelp;
	/* char buf[MAX_STRING_LENGTH];*/
	int match;

	for ( tHelp = first_help; tHelp; tHelp = tHelp->next )
		/*
			if ( pHelp->level == tHelp->level
			&&  !strcmp(pHelp->keyword, tHelp->keyword) )
			{
				sprintf(buf, "Duplicate %s. Deleting Help.\n\r",pHelp->keyword);
				bug(buf,0);
				STRFREE( pHelp->text );
				STRFREE( pHelp->keyword );
				DISPOSE( pHelp );
				return;
			}
			else
		*/
		if ( ( match = strcmp( pHelp->keyword[0] == '\'' ? pHelp->keyword + 1 : pHelp->keyword,
				   tHelp->keyword[0] == '\'' ? tHelp->keyword + 1 : tHelp->keyword ) ) < 0 ||
			( match == 0 && pHelp->level > tHelp->level ) ) {
			if ( !tHelp->prev )
				first_help = pHelp;
			else
				tHelp->prev->next = pHelp;
			pHelp->prev = tHelp->prev;
			pHelp->next = tHelp;
			tHelp->prev = pHelp;
			break;
		}

	if ( !tHelp )
		LINK( pHelp, first_help, last_help, next, prev );

	top_help++;
}



/*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
void new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset ) {
	RESET_DATA *pr;

	if ( !pR )
		return;

	pr = pR->reset_last;

	if ( !pr ) {
		pR->reset_first = pReset;
		pR->reset_last = pReset;
	} else {
		pR->reset_last->next = pReset;
		pR->reset_last = pReset;
		pR->reset_last->next = NULL;
	}

	top_reset++;
	return;
}






/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void ) {
	extern const sh_int rev_dir[];
	char buf[MAX_STRING_LENGTH];
	ROOM_INDEX_DATA *pRoomIndex;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;
	int iHash;
	int door;

	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
		for ( pRoomIndex = room_index_hash[iHash];
			pRoomIndex != NULL;
			pRoomIndex = pRoomIndex->next ) {
			bool fexit;

			fexit = FALSE;
			for ( door = 0; door <= 5; door++ ) {
				if ( ( pexit = pRoomIndex->exit[door] ) != NULL ) {
					fexit = TRUE;
					if ( pexit->vnum <= 0 )
						pexit->to_room = NULL;
					else
						pexit->to_room = get_room_index( pexit->vnum );
				}
			}

			if ( !fexit )
				SET_BIT( pRoomIndex->room_flags, ROOM_NO_MOB );
		}
	}

	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
		for ( pRoomIndex = room_index_hash[iHash];
			pRoomIndex != NULL;
			pRoomIndex = pRoomIndex->next ) {
			for ( door = 0; door <= 5; door++ ) {
				if ( ( pexit = pRoomIndex->exit[door] ) != NULL && ( to_room = pexit->to_room ) != NULL && ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL && pexit_rev->to_room != pRoomIndex ) {
					sprintf( buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
						pRoomIndex->vnum, door,
						to_room->vnum, rev_dir[door],
						( pexit_rev->to_room == NULL )
							? 0
							: pexit_rev->to_room->vnum );
					/*		    bug( buf, 0 ); */
				}
			}
		}
	}

	return;
}

/*
 * Repopulate areas periodically.
 */
void area_update( void ) {
	AREA_DATA *pArea;

	PROFILE_START( "area_update" );

	for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
		CHAR_DATA *pch;

		if ( ++pArea->age < 3 )
			continue;

		/*
		 * Check for PC's.
		 */
		if ( pArea->nplayer > 0 && pArea->age == 15 - 1 && profile_stats.tick_multiplier <= 1 ) {
			for ( pch = char_list; pch != NULL; pch = pch->next ) {
				if ( !IS_NPC( pch ) && IS_AWAKE( pch ) && pch->in_room != NULL && pch->in_room->area == pArea ) {
					send_to_char( "You hear the sound of a bell in the distance.\n\r", pch );
					if ( pch->desc != NULL )
						mcmp_play( pch->desc, "environment/bell_distant.mp3", MCMP_SOUND, MCMP_TAG_ENVIRONMENT,
							25, 1, 20, NULL, FALSE, "Distant bell tolls" );
				}
			}
		}

		/*
		 * Check age and reset.
		 * Note: Mud School resets every 3 minutes (not 15).
		 * Deferred reset: If no players, mark for reset on entry instead.
		 */
		if ( pArea->age >= 15 ) {
			ROOM_INDEX_DATA *pRoomIndex;

			if ( pArea->nplayer > 0 ) {
				/* Players present - reset immediately */
				PROFILE_START( "area_reset" );
				reset_area( pArea );
				PROFILE_END( "area_reset" );
				pArea->needs_reset = FALSE;
			} else {
				/* No players - defer reset until someone enters */
				pArea->needs_reset = TRUE;
			}
			pArea->age = number_range( 0, 3 );
			pRoomIndex = get_room_index( ROOM_VNUM_SCHOOL );
			if ( pRoomIndex != NULL && pArea == pRoomIndex->area )
				pArea->age = 15 - 3;
		}
	}

	PROFILE_END( "area_update" );
	return;
}

/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room( ROOM_INDEX_DATA *pRoom ) {
	RESET_DATA *pReset;
	CHAR_DATA *pMob;
	OBJ_DATA *pObj;
	CHAR_DATA *LastMob = NULL;
	OBJ_DATA *LastObj = NULL;
	int iExit;
	int level = 0;
	bool last;

	if ( !pRoom )
		return;

	pMob = NULL;
	last = FALSE;

	for ( iExit = 0; iExit < MAX_DIR; iExit++ ) {
		EXIT_DATA *pExit;
		if ( ( pExit = pRoom->exit[iExit] ) ) {
			pExit->exit_info = pExit->rs_flags;
			if ( ( pExit->to_room != NULL ) && ( ( pExit = pExit->to_room->exit[rev_dir[iExit]] ) ) ) {
				/* nail the other side */
				pExit->exit_info = pExit->rs_flags;
			}
		}
	}

	for ( pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next ) {
		MOB_INDEX_DATA *pMobIndex;
		OBJ_INDEX_DATA *pObjIndex;
		OBJ_INDEX_DATA *pObjToIndex;
		ROOM_INDEX_DATA *pRoomIndex;

		switch ( pReset->command ) {
		default:
			bug( "Reset_room: bad command %c.", pReset->command );
			break;

		case 'M':
			if ( !( pMobIndex = get_mob_index( pReset->arg1 ) ) ) {
				bug( "Reset_room: 'M': bad vnum %d.", pReset->arg1 );
				continue;
			}

			/*
			 * Some hard coding.
			 */
			/*            if ( ( pMobIndex->spec_fun == spec_lookup( "spec_cast_ghost" ) &&
							 ( weather_info.sunlight != SUN_DARK ) ) ) continue;  */

			if ( pMobIndex->count >= pReset->arg2 ) {
				last = FALSE;
				break;
			}

			pMob = create_mobile( pMobIndex );

			/*
			 * Some more hard coding.
			 */
			if ( room_is_dark( pRoom ) )
				SET_BIT( pMob->affected_by, AFF_INFRARED );

			/*
			 * Pet shop mobiles get ACT_PET set.
			 */
			{
				ROOM_INDEX_DATA *pRoomIndexPrev;

				pRoomIndexPrev = get_room_index( pRoom->vnum - 1 );
				if ( pRoomIndexPrev && IS_SET( pRoomIndexPrev->room_flags, ROOM_PET_SHOP ) )
					SET_BIT( pMob->act, ACT_PET );
			}

			char_to_room( pMob, pRoom );

			LastMob = pMob;
			level = URANGE( 0, pMob->level - 2, LEVEL_HERO );
			last = TRUE;
			break;

		case 'O':
			if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) ) {
				bug( "Reset_room: 'O': bad vnum %d.", pReset->arg1 );
				continue;
			}

			if ( !( pRoomIndex = get_room_index( pReset->arg3 ) ) ) {
				bug( "Reset_room: 'O': bad vnum %d.", pReset->arg3 );
				continue;
			}

			if ( pRoom->area->nplayer > 0 || count_obj_list( pObjIndex, pRoom->contents ) > 0 )
				break;

			pObj = create_object( pObjIndex, number_fuzzy( level ) );
			pObj->cost = 0;
			obj_to_room( pObj, pRoom );
			break;

		case 'P':
			if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) ) {
				bug( "Reset_room: 'P': bad vnum %d.", pReset->arg1 );
				continue;
			}
			if ( !( pObjToIndex = get_obj_index( pReset->arg3 ) ) ) {
				bug( "Reset_room: 'P': bad vnum %d.", pReset->arg3 );
				continue;
			}

			if ( pRoom->area->nplayer > 0 || !( LastObj = get_obj_type( pObjToIndex ) ) || count_obj_list( pObjIndex, LastObj->contains ) > 0 )
				break;

			pObj = create_object( pObjIndex, number_fuzzy( level ) );
			obj_to_obj( pObj, LastObj );

			/*
			 * Ensure that the container gets reset.    OLC 1.1b
			 */
			if ( LastObj->item_type == ITEM_CONTAINER ) {
				LastObj->value[1] = LastObj->pIndexData->value[1];
			} else {
				/* THIS SPACE INTENTIONALLY LEFT BLANK */
			}
			break;

		case 'G':
		case 'E':
			if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) ) {
				bug( "Reset_room: 'E' or 'G': bad vnum %d.", pReset->arg1 );
				continue;
			}

			if ( !last )
				break;

			if ( !LastMob ) {
				bug( "Reset_room: 'E' or 'G': null mob for vnum %d.",
					pReset->arg1 );
				last = FALSE;
				break;
			}

			if ( LastMob->pIndexData->pShop ) /* Shop-keeper? */
			{
				int olevel;

				switch ( pObjIndex->item_type ) {
				default:
					olevel = 0;
					break;
				case ITEM_PILL:
					olevel = number_range( 0, 10 );
					break;
				case ITEM_POTION:
					olevel = number_range( 0, 10 );
					break;
				case ITEM_SCROLL:
					olevel = number_range( 5, 15 );
					break;
				case ITEM_WAND:
					olevel = number_range( 10, 20 );
					break;
				case ITEM_STAFF:
					olevel = number_range( 15, 25 );
					break;
				case ITEM_ARMOR:
					olevel = number_range( 5, 15 );
					break;
				case ITEM_WEAPON:
					if ( pReset->command == 'G' )
						olevel = number_range( 5, 15 );
					else
						olevel = number_fuzzy( level );
					break;
				}

				if ( pObjIndex->vnum == 29603 && CHAOS ) return;
				if ( pObjIndex->vnum == 29603 ) CHAOS = TRUE;

				pObj = create_object( pObjIndex, olevel );
				if ( pReset->command == 'G' )
					SET_BIT( pObj->extra_flags, ITEM_INVENTORY );
			} else {
				pObj = create_object( pObjIndex, number_fuzzy( level ) );
			}
			obj_to_char( pObj, LastMob );
			if ( pReset->command == 'E' )
				equip_char( LastMob, pObj, pReset->arg3 );
			last = TRUE;
			break;

		case 'D':
			break;

		case 'R':
			/* OLC 1.1b
						if ( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
						{
							bug( "Reset_room: 'R': bad vnum %d.", pReset->arg1 );
							continue;
						}

						{
							EXIT_DATA *pExit;
							int d0;
							int d1;

							for ( d0 = 0; d0 < pReset->arg2 - 1; d0++ )
							{
								d1                   = number_range( d0, pReset->arg2-1 );
								pExit                = pRoomIndex->exit[d0];
								pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
								pRoomIndex->exit[d1] = pExit;
							}
						}
			*/
			break;
		}
	}

	return;
}

/* OLC
 * Reset one area.
 */
void reset_area( AREA_DATA *pArea ) {
	ROOM_INDEX_DATA *pRoom;
	int room_count = 0;
	struct timeval start_time, end_time;

	/* Track per-area reset timing when profiling is enabled */
	if ( profile_stats.enabled )
		gettimeofday( &start_time, NULL );

	/*
	 * Use the area's room list for efficient iteration.
	 * This avoids iterating sparse vnum ranges (e.g., 2-30073 with only 3324 rooms).
	 */
	for ( pRoom = pArea->room_first; pRoom != NULL; pRoom = pRoom->next_in_area ) {
		room_count++;
		PROFILE_START( "reset_room" );
		reset_room( pRoom );
		PROFILE_END( "reset_room" );
	}

	/* Update per-area profiling stats */
	if ( profile_stats.enabled ) {
		long elapsed_us;
		gettimeofday( &end_time, NULL );
		elapsed_us = ( end_time.tv_sec - start_time.tv_sec ) * 1000000
		           + ( end_time.tv_usec - start_time.tv_usec );
		pArea->profile_reset_count++;
		pArea->profile_reset_time_us += elapsed_us;
	}

	/* Debug: Log areas with many rooms */
	if ( profile_stats.verbose && room_count > 500 ) {
		char debug_buf[256];
		int vnum_range = pArea->uvnum - pArea->lvnum + 1;
		snprintf( debug_buf, sizeof( debug_buf ),
			"AREA RESET: %s - vnum range %d-%d (%d span), %d rooms reset (list)",
			pArea->name ? pArea->name : "Unknown",
			pArea->lvnum, pArea->uvnum, vnum_range, room_count );
		log_string( debug_buf );
	}

	return;
}

/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile( MOB_INDEX_DATA *pMobIndex ) {
	CHAR_DATA *mob;
	int tempvalue;

	if ( pMobIndex == NULL ) {
		bug( "Create_mobile: NULL pMobIndex.", 0 );
		exit( 1 );
	}

	if ( char_free == NULL ) {
		mob = alloc_perm( sizeof( *mob ) );
	} else {
		mob = char_free;
		char_free = char_free->next;
	}

	clear_char( mob );
	mob->pIndexData = pMobIndex;

	mob->hunting = str_dup( "" );
	mob->lord = str_dup( "" );
	mob->morph = str_dup( "" );
	mob->createtime = str_dup( "" );
	mob->lasttime = str_dup( "" );
	mob->lasthost = str_dup( "" );
	mob->powertype = str_dup( "" );
	mob->poweraction = str_dup( "" );
	mob->pload = str_dup( "" );
	mob->prompt = str_dup( "" );
	mob->cprompt = str_dup( "" );

	mob->name = str_dup( pMobIndex->player_name );		  /* OLC */
	mob->short_descr = str_dup( pMobIndex->short_descr ); /* OLC */
	mob->long_descr = str_dup( pMobIndex->long_descr );	  /* OLC */
	mob->description = str_dup( pMobIndex->description ); /* OLC */

	mob->spec_fun = pMobIndex->spec_fun;

	mob->home = 3001;
	mob->form = 32767;
	mob->level = number_fuzzy( pMobIndex->level );
	mob->act = pMobIndex->act;
	mob->affected_by = pMobIndex->affected_by;
	mob->alignment = pMobIndex->alignment;
	mob->sex = pMobIndex->sex;

	mob->armor = interpolate( mob->level, 100, -100 );

	tempvalue = mob->level * 12 + number_range( mob->level * mob->level / 4, mob->level * mob->level );

	if ( tempvalue > 300000 || tempvalue < 0 )
		mob->max_hit = 300000;
	else
		mob->max_hit = tempvalue;

	mob->hit = mob->max_hit;

	mob->hitroll = mob->level;
	mob->damroll = mob->level;
	mob->practice = mob->level * ( number_range( 10, 20 ) / 10 );

	/* Random Forge Objects */
	{
		OBJ_DATA *obj = NULL;
		int roll = number_percent();

		/* Metals have tiered drop rates: copper 4%, iron 2%, steel 1%, adamantite 0.4% */
		if ( roll <= 4 )
			obj = create_object( get_obj_index( 30049 ), 0 ); /* copper */
		else if ( roll <= 6 )
			obj = create_object( get_obj_index( 30050 ), 0 ); /* iron */
		else if ( roll <= 7 )
			obj = create_object( get_obj_index( 30051 ), 0 ); /* steel */
		else if ( roll == 8 && number_range( 1, 5 ) == 1 )
			obj = create_object( get_obj_index( 30052 ), 0 ); /* adamantite (0.4%) */
		/* Gems and hilts: 2% chance each, randomly selected */
		else if ( roll <= 10 )
			obj = create_object( get_obj_index( number_range( 30053, 30062 ) ), 0 ); /* gems */
		else if ( roll <= 12 )
			obj = create_object( get_obj_index( number_range( 30063, 30071 ) ), 0 ); /* hilts */

		if ( obj != NULL )
			obj_to_char( obj, mob );
	}
	/*
	 * Insert in list.
	 */
	mob->next = char_list;
	char_list = mob;
	pMobIndex->count++;
	return mob;
}

/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object( OBJ_INDEX_DATA *pObjIndex, int level ) {
	static OBJ_DATA obj_zero;
	OBJ_DATA *obj;

	if ( pObjIndex == NULL ) {
		bug( "Create_object: NULL pObjIndex.", 0 );
		exit( 1 );
	}

	if ( obj_free == NULL ) {
		obj = alloc_perm( sizeof( *obj ) );
	} else {
		obj = obj_free;
		obj_free = obj_free->next;
	}

	*obj = obj_zero;
	obj->pIndexData = pObjIndex;
	obj->in_room = NULL;
	obj->level = level;
	obj->wear_loc = -1;

	obj->name = str_dup( pObjIndex->name );				  /* OLC */
	obj->short_descr = str_dup( pObjIndex->short_descr ); /* OLC */
	obj->description = str_dup( pObjIndex->description ); /* OLC */

	if ( pObjIndex->chpoweron != NULL ) {
		obj->chpoweron = str_dup( pObjIndex->chpoweron );
		obj->chpoweroff = str_dup( pObjIndex->chpoweroff );
		obj->chpoweruse = str_dup( pObjIndex->chpoweruse );
		obj->victpoweron = str_dup( pObjIndex->victpoweron );
		obj->victpoweroff = str_dup( pObjIndex->victpoweroff );
		obj->victpoweruse = str_dup( pObjIndex->victpoweruse );
		obj->spectype = pObjIndex->spectype;
		obj->specpower = pObjIndex->specpower;
	} else {
		obj->chpoweron = str_dup( "(null)" );
		obj->chpoweroff = str_dup( "(null)" );
		obj->chpoweruse = str_dup( "(null)" );
		obj->victpoweron = str_dup( "(null)" );
		obj->victpoweroff = str_dup( "(null)" );
		obj->victpoweruse = str_dup( "(null)" );
		obj->spectype = 0;
		obj->specpower = 0;
	}
	obj->questmaker = str_dup( "" );
	obj->questowner = str_dup( "" );

	obj->chobj = NULL;

	obj->quest = 0;
	obj->points = 0;

	obj->item_type = pObjIndex->item_type;
	obj->extra_flags = pObjIndex->extra_flags;
	obj->wear_flags = pObjIndex->wear_flags;
	obj->value[0] = pObjIndex->value[0];
	obj->value[1] = pObjIndex->value[1];
	obj->value[2] = pObjIndex->value[2];
	obj->value[3] = pObjIndex->value[3];
	obj->weight = pObjIndex->weight;
	obj->cost = number_fuzzy( 10 ) * number_fuzzy( level ) * number_fuzzy( level );

	if ( obj->pIndexData->vnum == 30039 ) /* Questcards */
	{
		obj->condition = 100;
		obj->toughness = 100;
		obj->resistance = 1;
		SET_BIT( obj->quest, QUEST_RELIC );
	} else if ( obj->pIndexData->vnum >= 33960 && obj->pIndexData->vnum <= 34000 ) /* artifacts */
	{
		SET_BIT( obj->quest, QUEST_ARTIFACT );
		obj->condition = 100;
		obj->toughness = 100;
		obj->resistance = 1;
		obj->level = 60;
		obj->cost = 1000000;
	} else if ( obj->pIndexData->vnum >= 33800 && obj->pIndexData->vnum <= 33899 ) /* Prizes */
	{
		SET_BIT( obj->quest, QUEST_PRIZE );
		obj->condition = 100;
		obj->toughness = 100;
		obj->resistance = 1;
		obj->level = 60;
		obj->cost = 1000000;
	} else if ( obj->pIndexData->vnum >= 33000 && obj->pIndexData->vnum <= 33360 ) /* normal class eq */
	{
		obj->condition = 100;
		obj->toughness = 100;
		obj->resistance = 1;
		SET_BIT( obj->quest, QUEST_RELIC );
	} else if ( obj->pIndexData->vnum >= 29775 && obj->pIndexData->vnum <= 29990 ) /* Undead Knight eq */
	{
		obj->condition = 100;
		obj->toughness = 100;
		obj->resistance = 1;
		SET_BIT( obj->quest, QUEST_RELIC );
	} else if ( obj->pIndexData->vnum >= 800 && obj->pIndexData->vnum <= 825 ) /* rune EQ */
	{
		SET_BIT( obj->quest, QUEST_RELIC );
		obj->condition = 100;
		obj->toughness = 100;
		obj->resistance = 1;
	} else {
		obj->condition = 100;
		obj->toughness = 5;
		obj->resistance = 25;
	}

	/*
	 * Special bits for class eq
	 */
	if ( obj->pIndexData->vnum >= 33060 && obj->pIndexData->vnum <= 33079 )
		SET_BIT( obj->spectype, SITEM_DROW );
	if ( obj->pIndexData->vnum >= 33120 && obj->pIndexData->vnum <= 33139 )
		SET_BIT( obj->spectype, SITEM_DEMONIC );

	/*
	 * Mess with object properties.
	 */
	switch ( obj->item_type ) {
	default:
		bug( "Read_object: vnum %d bad type.", pObjIndex->vnum );
		break;

	case ITEM_LIGHT:
	case ITEM_TREASURE:
	case ITEM_INSTRUMENT:
	case ITEM_FURNITURE:
	case ITEM_TRASH:
	case ITEM_CONTAINER:
	case ITEM_DRINK_CON:
	case ITEM_KEY:
	case ITEM_FOOD:
	case ITEM_BOAT:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	case ITEM_FOUNTAIN:
	case ITEM_PORTAL:
	case ITEM_EGG:
	case ITEM_VOODOO:
	case ITEM_STAKE:
	case ITEM_MISSILE:
	case ITEM_AMMO:
	case ITEM_QUEST:
	case ITEM_QUESTCARD:
	case ITEM_QUESTMACHINE:
	case ITEM_SYMBOL:
	case ITEM_BOOK:
	case ITEM_PAGE:
	case ITEM_TOOL:
	case ITEM_COPPER:
	case ITEM_IRON:
	case ITEM_STEEL:
	case ITEM_ADAMANTITE:
	case ITEM_COOKINGPOT:
	case ITEM_GEMSTONE:
	case ITEM_HILT:
	case ITEM_SCROLL:
	case ITEM_WAND:
	case ITEM_STAFF:
		break;

	case ITEM_WEAPON:
		if ( !IS_SET( obj->quest, QUEST_ARTIFACT ) && !IS_SET( obj->quest, QUEST_RELIC ) && !IS_SET( obj->quest, QUEST_PRIZE ) ) {
			obj->value[1] = number_range( obj->value[1], obj->value[2] );
			obj->value[2] = number_range( ( obj->value[1] + 1 ), ( obj->value[1] * 2 ) );
		}
		break;

	case ITEM_ARMOR:
		if ( !IS_SET( obj->quest, QUEST_ARTIFACT ) && !IS_SET( obj->quest, QUEST_RELIC ) && !IS_SET( obj->quest, QUEST_PRIZE ) )
			obj->value[0] = number_range( 10, obj->value[0] );
		break;

	case ITEM_POTION:
	case ITEM_PILL:
		obj->value[0] = number_fuzzy( number_fuzzy( obj->value[0] ) );
		break;

	case ITEM_MONEY:
		obj->value[0] = obj->cost;
		break;
	}

	obj->next = object_list;
	object_list = obj;
	pObjIndex->count++;

	return obj;
}

/*
 * Clear a new character.
 */
void clear_char( CHAR_DATA *ch ) {
	static CHAR_DATA ch_zero;

	*ch = ch_zero;
	ch->name = &str_empty[0];
	ch->short_descr = &str_empty[0];
	ch->long_descr = &str_empty[0];
	ch->description = &str_empty[0];
	ch->lord = &str_empty[0];
	ch->morph = &str_empty[0];
	ch->createtime = &str_empty[0];
	ch->lasthost = &str_empty[0];
	ch->lasttime = &str_empty[0];
	ch->powertype = &str_empty[0];
	ch->poweraction = &str_empty[0];
	ch->pload = &str_empty[0];
	ch->prompt = &str_empty[0];
	ch->cprompt = &str_empty[0];
	ch->hunting = &str_empty[0];

	ch->logon = current_time;
	ch->armor = 100;
	ch->position = POS_STANDING;
	ch->practice = 0;
	ch->hit = 1000;
	ch->max_hit = 1000;
	ch->mana = 1500;
	ch->max_mana = 1500;
	ch->move = 1500;
	ch->max_move = 1500;
	ch->master = NULL;
	ch->leader = NULL;
	ch->fighting = NULL;
	ch->mount = NULL;
	ch->wizard = NULL;
	ch->paradox[0] = 0;
	ch->paradox[1] = 0;
	ch->paradox[2] = 0;
	ch->damcap[0] = 1000;
	ch->damcap[1] = 0;
	return;
}

/*
 * Free a character.
 */
void free_char( CHAR_DATA *ch ) {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;
	ALIAS_DATA *ali;
	ALIAS_DATA *ali_next;

	for ( obj = ch->carrying; obj != NULL; obj = obj_next ) {
		obj_next = obj->next_content;
		extract_obj( obj );
	}

	for ( paf = ch->affected; paf != NULL; paf = paf_next ) {
		paf_next = paf->next;
		affect_remove( ch, paf );
	}

	free_string( ch->name );
	free_string( ch->short_descr );
	free_string( ch->long_descr );
	free_string( ch->description );
	free_string( ch->lord );
	free_string( ch->morph );
	free_string( ch->createtime );
	free_string( ch->lasttime );
	free_string( ch->lasthost );
	free_string( ch->powertype );
	free_string( ch->poweraction );
	free_string( ch->pload );
	free_string( ch->prompt );
	free_string( ch->cprompt );
	free_string( ch->hunting );

	if ( ch->pcdata != NULL ) {
		for ( ali = ch->pcdata->alias; ali; ali = ali_next ) {
			ali_next = ali->next;
			alias_remove( ch, ali );
		}

		free_string( ch->pcdata->switchname );
		free_string( ch->pcdata->logoutmessage );
		free_string( ch->pcdata->avatarmessage );
		free_string( ch->pcdata->loginmessage );
		free_string( ch->pcdata->decapmessage );
		free_string( ch->pcdata->tiemessage );
		free_string( ch->pcdata->pwd );
		free_string( ch->pcdata->bamfin );
		free_string( ch->pcdata->bamfout );
		free_string( ch->pcdata->title );
		free_string( ch->pcdata->conception );
		free_string( ch->pcdata->parents );
		free_string( ch->pcdata->cparents );
		free_string( ch->pcdata->marriage );
		ch->pcdata->next = pcdata_free;
		pcdata_free = ch->pcdata;
	}

	ch->next = char_free;
	char_free = ch;
	return;
}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr( char *name, EXTRA_DESCR_DATA *ed ) {
	for ( ; ed != NULL; ed = ed->next ) {
		if ( is_name( name, ed->keyword ) )
			return ed->description;
	}
	return NULL;
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index( int vnum ) {
	MOB_INDEX_DATA *pMobIndex;

	for ( pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH];
		pMobIndex != NULL;
		pMobIndex = pMobIndex->next ) {
		if ( pMobIndex->vnum == vnum )
			return pMobIndex;
	}

	if ( fBootDb ) {
		bug( "Get_mob_index: bad vnum %d.", vnum );
		exit( 1 );
	}

	return NULL;
}

/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index( int vnum ) {
	OBJ_INDEX_DATA *pObjIndex;

	for ( pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH];
		pObjIndex != NULL;
		pObjIndex = pObjIndex->next ) {
		if ( pObjIndex->vnum == vnum )
			return pObjIndex;
	}

	if ( fBootDb ) {
		bug( "Get_obj_index: bad vnum %d.", vnum );
		exit( 1 );
	}

	return NULL;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index( int vnum ) {
	ROOM_INDEX_DATA *pRoomIndex;

	for ( pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH];
		pRoomIndex != NULL;
		pRoomIndex = pRoomIndex->next ) {
		if ( pRoomIndex->vnum == vnum )
			return pRoomIndex;
	}

	if ( fBootDb ) {
		bug( "Get_room_index: bad vnum %d.", vnum );
		exit( 1 );
	}

	return NULL;
}

#ifdef MEM_DEBUG
#define MEM_CANARY_BYTE  0xFD  /* Written in slack space after requested size */
#define MEM_FREE_BYTE    0xDD  /* Written over freed blocks (after free list ptr) */

static void mem_debug_dump( const char *label, void *ptr, int size ) {
	unsigned char *p = (unsigned char *) ptr;
	char buf[MAX_STRING_LENGTH];
	int dump_len = ( size < 128 ) ? size : 128;
	int off = 0;

	off += snprintf( buf + off, sizeof( buf ) - off,
		"MEM_DEBUG %s ptr=%p size=%d hex: ", label, ptr, size );
	for ( int i = 0; i < dump_len && off < (int) sizeof( buf ) - 4; i++ )
		off += snprintf( buf + off, sizeof( buf ) - off, "%02X ", p[i] );
	log_string( buf );

	off = 0;
	off += snprintf( buf + off, sizeof( buf ) - off, "MEM_DEBUG %s ascii: ", label );
	for ( int i = 0; i < dump_len && off < (int) sizeof( buf ) - 4; i++ )
		off += snprintf( buf + off, sizeof( buf ) - off, "%c",
			( p[i] >= 32 && p[i] < 127 ) ? (char) p[i] : '.' );
	log_string( buf );
}

static bool mem_debug_valid_ptr( void *ptr ) {
	uintptr_t addr;
	if ( ptr == NULL ) return TRUE;
	addr = (uintptr_t) ptr;
	if ( addr % sizeof( void * ) != 0 ) return FALSE;
	if ( addr < 0x10000 ) return FALSE;
	return TRUE;
}

/* Scan all free lists for corruption. Call periodically from game loop. */
void mem_debug_check_freelists( void ) {
	int iList;

	pthread_mutex_lock( &memory_mutex );
	for ( iList = 0; iList < MAX_MEM_LIST; iList++ ) {
		void *cur = rgFreeList[iList];
		int count = 0;
		while ( cur != NULL && count < 100000 ) {
			void *next = *( (void **) cur );
			if ( !mem_debug_valid_ptr( next ) ) {
				char dbuf[256];
				snprintf( dbuf, sizeof( dbuf ),
					"MEM_DEBUG CORRUPTION: free_list[%d] (bucket %d) node #%d at %p has invalid next=%p",
					iList, rgSizeList[iList], count, cur, next );
				log_string( dbuf );
				mem_debug_dump( "Corrupted free node", cur, rgSizeList[iList] );
				/* Truncate the chain here to prevent crashes */
				*( (void **) cur ) = NULL;
				break;
			}
			cur = next;
			count++;
		}
	}
	pthread_mutex_unlock( &memory_mutex );
}
#endif /* MEM_DEBUG */

/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void *alloc_mem( int sMem ) {
	void *pMem;
	int iList;

	pthread_mutex_lock( &memory_mutex );

	for ( iList = 0; iList < MAX_MEM_LIST; iList++ ) {
		if ( sMem <= rgSizeList[iList] )
			break;
	}

	if ( iList == MAX_MEM_LIST ) {
		bug( "Alloc_mem: size %d too large.", sMem );
		exit( 1 );
	}

	if ( rgFreeList[iList] == NULL ) {
		pthread_mutex_unlock( &memory_mutex );
		pMem = alloc_perm( rgSizeList[iList] );
		pthread_mutex_lock( &memory_mutex );
	} else {
#ifdef MEM_DEBUG
		/* Validate free list head's next pointer before following it */
		void *next = *( (void **) rgFreeList[iList] );
		if ( !mem_debug_valid_ptr( next ) ) {
			char dbuf[256];
			snprintf( dbuf, sizeof( dbuf ),
				"MEM_DEBUG CORRUPTION in alloc_mem: free_list[%d] (bucket %d) head=%p has invalid next=%p",
				iList, rgSizeList[iList], rgFreeList[iList], next );
			log_string( dbuf );
			mem_debug_dump( "Corrupted free list head", rgFreeList[iList], rgSizeList[iList] );
			dump_last_command();
			/* Discard corrupted chain, allocate fresh */
			rgFreeList[iList] = NULL;
			pthread_mutex_unlock( &memory_mutex );
			pMem = alloc_perm( rgSizeList[iList] );
			pthread_mutex_lock( &memory_mutex );
		} else {
			pMem = rgFreeList[iList];
			rgFreeList[iList] = next;
		}
#else
		pMem = rgFreeList[iList];
		rgFreeList[iList] = *( (void **) rgFreeList[iList] );
#endif
	}

#ifdef MEM_DEBUG
	/* Write canary bytes in the slack space between requested size and bucket size */
	{
		int slack = rgSizeList[iList] - sMem;
		if ( slack > 0 )
			memset( (unsigned char *) pMem + sMem, MEM_CANARY_BYTE, slack );
	}
#endif

	pthread_mutex_unlock( &memory_mutex );
	return pMem;
}

/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void free_mem( void *pMem, int sMem ) {
	int iList;

	pthread_mutex_lock( &memory_mutex );

	for ( iList = 0; iList < MAX_MEM_LIST; iList++ ) {
		if ( sMem <= rgSizeList[iList] )
			break;
	}

	if ( iList == MAX_MEM_LIST ) {
		bug( "Free_mem: size %d too large.", sMem );
		exit( 1 );
	}

#ifdef MEM_DEBUG
	/* Check for double-free: if bytes after the free list pointer are all 0xDD,
	 * this block was already freed and is being freed again. */
	if ( rgSizeList[iList] > (int) sizeof( void * ) ) {
		unsigned char *post_ptr = (unsigned char *) pMem + sizeof( void * );
		int check_len = rgSizeList[iList] - (int) sizeof( void * );
		int all_poison = 1;
		int j;
		for ( j = 0; j < check_len; j++ ) {
			if ( post_ptr[j] != MEM_FREE_BYTE ) {
				all_poison = 0;
				break;
			}
		}
		if ( all_poison ) {
			char dbuf[512];
			snprintf( dbuf, sizeof( dbuf ),
				"MEM_DEBUG DOUBLE-FREE: ptr=%p req_size=%d bucket=%d - block already has free poison!",
				pMem, sMem, rgSizeList[iList] );
			log_string( dbuf );
			/* Show the free list pointer stored in this already-freed block */
			{
				void *stale_next = *( (void **) pMem );
				snprintf( dbuf, sizeof( dbuf ),
					"MEM_DEBUG DOUBLE-FREE: stale free-list next=%p (block is already on free list)",
					stale_next );
				log_string( dbuf );
			}
			mem_debug_dump( "Double-freed block", pMem, rgSizeList[iList] );
			dump_last_command();
			/* Skip the free to avoid corrupting the free list further */
			pthread_mutex_unlock( &memory_mutex );
			return;
		}
	}

	/* Check canary bytes in slack space - detects buffer overflows */
	{
		int slack = rgSizeList[iList] - sMem;
		if ( slack > 0 ) {
			unsigned char *canary = (unsigned char *) pMem + sMem;
			int i;
			for ( i = 0; i < slack; i++ ) {
				if ( canary[i] != MEM_CANARY_BYTE )
					break;
			}
			if ( i < slack ) {
				char dbuf[256];
				snprintf( dbuf, sizeof( dbuf ),
					"MEM_DEBUG OVERFLOW in free_mem: ptr=%p req_size=%d bucket=%d canary corrupted at offset %d",
					pMem, sMem, rgSizeList[iList], sMem + i );
				log_string( dbuf );
				mem_debug_dump( "Overflowed block", pMem, rgSizeList[iList] );
				dump_last_command();
			}
		}
	}

	/* Poison freed memory (after free list pointer) to detect use-after-free */
	if ( rgSizeList[iList] > (int) sizeof( void * ) )
		memset( (unsigned char *) pMem + sizeof( void * ), MEM_FREE_BYTE,
			rgSizeList[iList] - (int) sizeof( void * ) );
#endif

	*( (void **) pMem ) = rgFreeList[iList];
	rgFreeList[iList] = pMem;

	pthread_mutex_unlock( &memory_mutex );
	return;
}

#ifndef MEM_DEBUG
void mem_debug_check_freelists( void ) {
	/* No-op when MEM_DEBUG is not enabled */
}
#endif

/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm( int sMem ) {
	static char *pMemPerm;
	static int iMemPerm;
	void *pMem;

	pthread_mutex_lock( &memory_mutex );

	while ( sMem % sizeof( long ) != 0 )
		sMem++;
	if ( sMem > MAX_PERM_BLOCK ) {
		bug( "Alloc_perm: %d too large.", sMem );
		exit( 1 );
	}

	if ( pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK ) {
		iMemPerm = 0;
		if ( ( pMemPerm = calloc( 1, MAX_PERM_BLOCK ) ) == NULL ) {
			dump_last_command();
			perror( "Alloc_perm" );
			exit( 1 );
		}
	}

	pMem = pMemPerm + iMemPerm;
	iMemPerm += sMem;
	nAllocPerm += 1;
	sAllocPerm += sMem;

	pthread_mutex_unlock( &memory_mutex );
	return pMem;
}

/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup( const char *str ) {
	char *str_new;
	size_t len;

	if ( str[0] == '\0' ) {
		return &str_empty[0];
	}
	if ( str >= string_space && str < top_string ) {
		return (char *) str;
	}

	len = strlen( str );
	/* Check if string is too large for alloc_mem (max is 32704 bytes) */
	if ( len >= 32704 ) {
		bug( "str_dup: string too large (%d bytes), truncating to 32700", (int)len );
		len = 32700; /* Leave room for null terminator */
	}

	str_new = alloc_mem( (int) len + 1 );
	strncpy( str_new, str, len );
	str_new[len] = '\0';
	return str_new;
}

/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string( char *pstr ) {

	if ( pstr == NULL || pstr == &str_empty[0] || ( pstr >= string_space && pstr < top_string ) ) {
		return;
	}

	free_mem( pstr, (int) strlen( pstr ) + 1 );
	return;
}

/*
 * Calculate difficulty score for a single mob.
 * Based on HP, damage output, AC, hitroll, and special flags.
 */
int calculate_mob_difficulty( MOB_INDEX_DATA *pMob ) {
	int avg_hp, avg_damage, num_attacks;
	int hp_score, damage_score, ac_score, hitroll_score, disarm_score;
	int difficulty;

	if ( pMob == NULL )
		return 0;

	/* HP from dice: N*(S+1)/2 + P */
	avg_hp = ( pMob->hitnodice * ( pMob->hitsizedice + 1 ) / 2 ) + pMob->hitplus;

	/* Attack count: level thresholds + hitsizedice bonus */
	num_attacks = 1;
	if ( pMob->level >= 50 ) num_attacks++;
	if ( pMob->level >= 100 ) num_attacks++;
	if ( pMob->level >= 500 ) num_attacks++;
	if ( pMob->level >= 1000 ) num_attacks++;
	if ( pMob->level >= 1500 ) num_attacks++;
	if ( pMob->level >= 2000 ) num_attacks += 2;
	num_attacks += UMIN( pMob->hitsizedice, 20 );

	/* NPC damage = level per hit, total = attacks * level */
	avg_damage = pMob->level * num_attacks;

	/* Component scores */
	hp_score = UMIN( avg_hp / 20, 500 );
	damage_score = avg_damage / 5;
	ac_score = ( pMob->ac < 0 ) ? UMIN( -pMob->ac / 10, 100 ) : -pMob->ac / 20;
	hitroll_score = UMIN( pMob->hitroll, 100 );
	disarm_score = UMIN( pMob->level / 10, 50 );

	difficulty = hp_score + damage_score + ac_score + hitroll_score + disarm_score;

	/* Flag multipliers */
	if ( IS_SET( pMob->affected_by, AFF_SANCTUARY ) )
		difficulty = difficulty * 3 / 2;
	if ( IS_SET( pMob->act, ACT_AGGRESSIVE ) )
		difficulty = difficulty * 11 / 10;
	if ( IS_SET( pMob->act, ACT_MOUNT ) )
		difficulty = difficulty * 3 / 10;
	if ( IS_SET( pMob->act, ACT_NOEXP ) || IS_SET( pMob->act, ACT_NOEXP2 ) )
		difficulty = difficulty / 2;

	return difficulty;
}

/*
 * Calculate difficulty stats for an area based on its mobs.
 */
void calculate_area_difficulty( AREA_DATA *pArea ) {
	MOB_INDEX_DATA *pMob;
	int vnum, total_level = 0, total_diff = 0, count = 0;
	int min_lvl = 32767, max_lvl = 0;

	if ( pArea == NULL )
		return;

	for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ ) {
		if ( ( pMob = get_mob_index( vnum ) ) != NULL && pMob->area == pArea ) {
			int diff = calculate_mob_difficulty( pMob );
			count++;
			total_level += pMob->level;
			total_diff += diff;
			if ( pMob->level < min_lvl ) min_lvl = pMob->level;
			if ( pMob->level > max_lvl ) max_lvl = pMob->level;
		}
	}

	pArea->mob_count = count;
	if ( count > 0 ) {
		pArea->avg_mob_level = total_level / count;
		pArea->avg_difficulty = total_diff / count;
		pArea->min_mob_level = min_lvl;
		pArea->max_mob_level = max_lvl;
	} else {
		pArea->avg_mob_level = 0;
		pArea->avg_difficulty = 0;
		pArea->min_mob_level = 0;
		pArea->max_mob_level = 0;
	}

	/* Assign tier */
	if ( pArea->avg_difficulty < 30 )
		pArea->difficulty_tier = 0; /* trivial */
	else if ( pArea->avg_difficulty < 100 )
		pArea->difficulty_tier = 1; /* easy */
	else if ( pArea->avg_difficulty < 300 )
		pArea->difficulty_tier = 2; /* normal */
	else if ( pArea->avg_difficulty < 600 )
		pArea->difficulty_tier = 3; /* hard */
	else
		pArea->difficulty_tier = 4; /* deadly */
}

/*
 * Calculate difficulty for all areas in the world.
 */
void calculate_all_area_difficulties( void ) {
	AREA_DATA *pArea;

	for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
		calculate_area_difficulty( pArea );
}

static const char *tier_names[] = { "Trivial", "Easy", "Normal", "Hard", "Deadly" };

void do_areas( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char level_buf[20];
	AREA_DATA *pArea;
	AREA_DATA **arr;
	int count = 0, i, j;

	WAIT_STATE( ch, 10 );

	/* Count visible areas (skip hidden) */
	for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
		if ( !pArea->is_hidden )
			count++;
	}

	if ( count == 0 ) {
		send_to_char( "No areas found.\n\r", ch );
		return;
	}

	/* Allocate array for sorting */
	arr = alloc_mem( sizeof( AREA_DATA * ) * count );

	/* Fill array (skip hidden) */
	i = 0;
	for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
		if ( !pArea->is_hidden )
			arr[i++] = pArea;
	}

	/* Sort by difficulty_tier, then avg_mob_level within tier (insertion sort) */
	for ( i = 1; i < count; i++ ) {
		AREA_DATA *tmp = arr[i];
		for ( j = i - 1; j >= 0; j-- ) {
			if ( arr[j]->difficulty_tier > tmp->difficulty_tier )
				arr[j + 1] = arr[j];
			else if ( arr[j]->difficulty_tier == tmp->difficulty_tier && arr[j]->avg_mob_level > tmp->avg_mob_level )
				arr[j + 1] = arr[j];
			else
				break;
		}
		arr[j + 1] = tmp;
	}

	/* Header */
	send_to_char(
		"#w------------------------------------------------------------------------------\n\r"
		"Area Name                        Tier      Level      Builders\n\r"
		"------------------------------------------------------------------------------#n\n\r",
		ch );

	/* Display */
	for ( i = 0; i < count; i++ ) {
		pArea = arr[i];
		if ( pArea->mob_count > 0 ) {
			sprintf( level_buf, "%d-%d", pArea->min_mob_level, pArea->max_mob_level );
			sprintf( buf, "%-32.32s %-9s %-10s %s\n\r",
				pArea->name, tier_names[pArea->difficulty_tier],
				level_buf, pArea->builders );
		} else {
			sprintf( buf, "%-32.32s %-9s %-10s %s\n\r",
				pArea->name, "N/A", "No mobs", pArea->builders );
		}
		send_to_char( buf, ch );
	}

	/* Footer */
	sprintf( buf,
		"#w------------------------------------------------------------------------------#n\n\r"
		"Total: %d areas\n\r",
		count );
	send_to_char( buf, ch );

	free_mem( arr, sizeof( AREA_DATA * ) * count );
}

void do_memory( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	sprintf( buf, "Affects %5d\n\r", top_affect );
	send_to_char( buf, ch );
	sprintf( buf, "Areas   %5d\n\r", top_area );
	send_to_char( buf, ch );
	sprintf( buf, "RmTxt   %5d\n\r", top_rt );
	send_to_char( buf, ch );
	sprintf( buf, "ExDes   %5d\n\r", top_ed );
	send_to_char( buf, ch );
	sprintf( buf, "Exits   %5d\n\r", top_exit );
	send_to_char( buf, ch );
	sprintf( buf, "Helps   %5d\n\r", top_help );
	send_to_char( buf, ch );
	sprintf( buf, "Mobs    %5d\n\r", top_mob_index );
	send_to_char( buf, ch );
	sprintf( buf, "Objs    %5d\n\r", top_obj_index );
	send_to_char( buf, ch );
	sprintf( buf, "Resets  %5d\n\r", top_reset );
	send_to_char( buf, ch );
	sprintf( buf, "Rooms   %5d\n\r", top_room );
	send_to_char( buf, ch );
	sprintf( buf, "Shops   %5d\n\r", top_shop );
	send_to_char( buf, ch );

	sprintf( buf, "Strings %5d strings of %7d bytes (max %d).\n\r",
		nAllocString, sAllocString, MAX_STRING );
	send_to_char( buf, ch );

	sprintf( buf, "Perms   %5d blocks  of %7d bytes.\n\r",
		nAllocPerm, sAllocPerm );
	send_to_char( buf, ch );

	return;
}

/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy( int number ) {
	switch ( number_bits( 2 ) ) {
	case 0:
		number -= 1;
		break;
	case 3:
		number += 1;
		break;
	}

	return UMAX( 1, number );
}

/*
 * Generate a random number.
 */
int number_range( int from, int to ) {
	int power;
	int number;

	if ( ( to = to - from + 1 ) <= 1 )
		return from;

	for ( power = 2; power < to; power <<= 1 );

	while ( ( number = number_mm() & ( power - 1 ) ) >= to );

	return from + number;
}

/*
 * Generate a percentile roll.
 */
int number_percent( void ) {
	int percent;

	while ( ( percent = number_mm() & ( 128 - 1 ) ) > 99 );

	return 1 + percent;
}

/*
 * Generate a random door.
 */
int number_door( void ) {
	int door;

	while ( ( door = number_mm() & ( 8 - 1 ) ) > 5 );

	return door;
}

int number_bits( int width ) {
	return number_mm() & ( ( 1 << width ) - 1 );
}

/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */
static int rgiState[2 + 55];

void init_mm() {
	int *piState;
	int iState;

	piState = &rgiState[2];

	piState[-2] = 55 - 55;
	piState[-1] = 55 - 24;

	piState[0] = ( (int) current_time ) & ( ( 1 << 30 ) - 1 );
	piState[1] = 1;
	for ( iState = 2; iState < 55; iState++ ) {
		piState[iState] = ( piState[iState - 1] + piState[iState - 2] ) & ( ( 1 << 30 ) - 1 );
	}
	return;
}

int number_mm( void ) {
	int *piState;
	int iState1;
	int iState2;
	int iRand;

	piState = &rgiState[2];
	iState1 = piState[-2];
	iState2 = piState[-1];
	iRand = ( piState[iState1] + piState[iState2] ) & ( ( 1 << 30 ) - 1 );
	piState[iState1] = iRand;
	if ( ++iState1 == 55 )
		iState1 = 0;
	if ( ++iState2 == 55 )
		iState2 = 0;
	piState[-2] = iState1;
	piState[-1] = iState2;
	return iRand >> 6;
}

/*
 * Roll some dice.
 */
int dice( int number, int size ) {
	int idice;
	int sum;

	switch ( size ) {
	case 0:
		return 0;
	case 1:
		return number;
	}

	for ( idice = 0, sum = 0; idice < number; idice++ )
		sum += number_range( 1, size );

	return sum;
}

/*
 * Simple linear interpolation.
 */
int interpolate( int level, int value_00, int value_32 ) {
	return value_00 + level * ( value_32 - value_00 ) / 32;
}

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str ) {
	for ( ; *str != '\0'; str++ ) {
		if ( *str == '~' )
			*str = '-';
	}

	return;
}

/*
 * Compare strings, case insensitive.
 * Return TRUE if different
 *   (compatibility with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr ) {
	if ( astr == NULL ) {
		bug( "Str_cmp: null astr.", 0 );
		return TRUE;
	}

	if ( bstr == NULL ) {
		bug( "Str_cmp: null bstr.", 0 );
		return TRUE;
	}

	for ( ; *astr || *bstr; astr++, bstr++ ) {
		if ( LOWER( *astr ) != LOWER( *bstr ) )
			return TRUE;
	}

	return FALSE;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix( const char *astr, const char *bstr ) {
	if ( astr == NULL ) {
		bug( "Strn_cmp: null astr.", 0 );
		return TRUE;
	}

	if ( bstr == NULL ) {
		bug( "Strn_cmp: null bstr.", 0 );
		return TRUE;
	}

	for ( ; *astr; astr++, bstr++ ) {
		if ( LOWER( *astr ) != LOWER( *bstr ) )
			return TRUE;
	}

	return FALSE;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr ) {
	int sstr1;
	int sstr2;
	int ichar;
	char c0;

	if ( ( c0 = LOWER( astr[0] ) ) == '\0' )
		return FALSE;

	sstr1 = (int) strlen( astr );
	sstr2 = (int) strlen( bstr );

	for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ ) {
		if ( c0 == LOWER( bstr[ichar] ) && !str_prefix( astr, bstr + ichar ) )
			return FALSE;
	}

	return TRUE;
}

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr ) {
	int sstr1;
	int sstr2;

	sstr1 = (int) strlen( astr );
	sstr2 = (int) strlen( bstr );
	if ( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
		return FALSE;
	else
		return TRUE;
}

/*
 * Returns an initial-capped string.
 */
char *capitalize( const char *str ) {
	static char strcap[MAX_STRING_LENGTH];
	int i;
	bool found_alpha = FALSE;

	for ( i = 0; str[i] != '\0'; i++ ) {
		strcap[i] = str[i];
		if ( !found_alpha && str[i] == '#' && str[i + 1] != '\0' ) {
			/* Skip color code: copy the '#' and the next char as-is */
			i++;
			strcap[i] = str[i];
			continue;
		}
		if ( !found_alpha && isalpha( (unsigned char) str[i] ) ) {
			strcap[i] = UPPER( str[i] );
			found_alpha = TRUE;
		}
	}
	strcap[i] = '\0';
	return strcap;
}

/*
 * Append a string to a file.
 */
void append_file( CHAR_DATA *ch, char *file, char *str ) {
	FILE *fp;

	if ( IS_NPC( ch ) || str[0] == '\0' )
		return;

	fclose( fpReserve );
	if ( ( fp = fopen( file, "a" ) ) == NULL ) {
		perror( file );
		send_to_char( "Could not open the file!\n\r", ch );
	} else {
		fprintf( fp, "[%5d] %s: %s\n",
			ch->in_room ? ch->in_room->vnum : 0, ch->name, str );
		fclose( fp );
	}

	fpReserve = fopen( NULL_FILE, "r" );
	return;
}

/*
 * Reports a bug.
 */
void bug( const char *str, int param ) {
	char buf[MAX_STRING_LENGTH];

	strcpy( buf, "[*****] BUG: " );
	sprintf( buf + strlen( buf ), str, param );
	log_string( buf );

	db_game_append_bug( 0, "SYSTEM", buf );
}

/*
 * Writes a string to the log (stderr and log file).
 * Log file is named by startup timestamp: YYYYMMDD-HHMMSS.log
 */
static FILE *log_fp = NULL;

void log_string( const char *str ) {
	char *strtime;
	char logout[MAX_STRING_LENGTH];
	struct tm *tm_info;
	char log_filename[MUD_PATH_MAX + 32]; /* Extra room for date suffix */

	strtime = ctime( &current_time );
	strtime[strlen( strtime ) - 1] = '\0';

	/* Always write to stderr */
	fprintf( stderr, "%s :: %s\n", strtime, str );

	/* Write to log file if log directory is set up */
	if ( mud_log_dir[0] != '\0' ) {
		/* Open log file on first call, named by startup time */
		if ( log_fp == NULL ) {
			tm_info = localtime( &current_time );
			snprintf( log_filename, sizeof( log_filename ), "%.480s%s%04d%02d%02d-%02d%02d%02d.log",
				mud_log_dir, PATH_SEPARATOR,
				tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
				tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec );
			log_fp = fopen( log_filename, "a" );
		}

		if ( log_fp != NULL ) {
			fprintf( log_fp, "%s :: %s\n", strtime, str );
			fflush( log_fp );
		}
	}

	strcpy( logout, str );
	logchan( logout );
	return;
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void ) {
	return;
}
