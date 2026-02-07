/***************************************************************************
 *  db_class.c - SQLite persistence for class configuration data
 *
 *  Manages class.db stored in gamedata/db/game/ containing:
 *    - class_registry, class_brackets, class_generations, class_auras
 *    - class_armor_config (includes mastery_vnum), class_armor_pieces
 *    - class_starting, class_score_stats
 *
 *  Data is pre-populated in the shipped database; this file only loads
 *  and provides access to that data.
 ***************************************************************************/

#include "db_util.h"
#include "db_class.h"
#include "../classes/dragonkin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External: game database directory set by db_game_init() */
extern char mud_db_game_dir[];

/* Database connection */
static sqlite3 *class_db = NULL;

/* Cache sizes */
#define MAX_CACHED_BRACKETS       32
#define MAX_CACHED_GENERATIONS   256
#define MAX_CACHED_AURAS          32
#define MAX_CACHED_ARMOR_CONFIGS  32
#define MAX_CACHED_ARMOR_PIECES  512
#define MAX_CACHED_STARTING       32
#define MAX_CACHED_SCORE_STATS   128
#define MAX_CACHED_REGISTRY       32

/* In-memory caches */
static CLASS_BRACKET bracket_cache[MAX_CACHED_BRACKETS];
static int bracket_count = 0;

static CLASS_GENERATION gen_cache[MAX_CACHED_GENERATIONS];
static int gen_count = 0;

static CLASS_AURA aura_cache[MAX_CACHED_AURAS];
static int aura_count = 0;

static CLASS_ARMOR_CONFIG armor_config_cache[MAX_CACHED_ARMOR_CONFIGS];
static int armor_config_count = 0;

static CLASS_ARMOR_PIECE armor_piece_cache[MAX_CACHED_ARMOR_PIECES];
static int armor_piece_count = 0;

static CLASS_STARTING starting_cache[MAX_CACHED_STARTING];
static int starting_count = 0;

static CLASS_SCORE_STAT score_stats_cache[MAX_CACHED_SCORE_STATS];
static int score_stats_count = 0;

static CLASS_REGISTRY_ENTRY registry_cache[MAX_CACHED_REGISTRY];
static int registry_count = 0;


/*
 * Initialize class database. Called after db_game_init() in boot_db().
 */
void db_class_init( void ) {
	char path[MUD_PATH_MAX];

	if ( snprintf( path, sizeof( path ), "%s%sclass.db",
			mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
		bug( "db_class_init: class.db path truncated.", 0 );
	}
	if ( sqlite3_open_v2( path, &class_db, SQLITE_OPEN_READONLY, NULL ) != SQLITE_OK ) {
		bug( "db_class_init: cannot open class.db", 0 );
		class_db = NULL;
	}
}

/*
 * Close class database connection after boot loading is complete.
 */
void db_class_close( void ) {
	if ( class_db ) {
		sqlite3_close( class_db );
		class_db = NULL;
	}
}


/***************************************************************************
 * Class Display Config (class_brackets, class_generations)
 ***************************************************************************/

void db_class_load_display( void ) {
	sqlite3_stmt *stmt;
	char buf[256];
	int i;

	if ( !class_db )
		return;

	/* Clear existing caches */
	for ( i = 0; i < bracket_count; i++ ) {
		if ( bracket_cache[i].open_bracket ) free_string( bracket_cache[i].open_bracket );
		if ( bracket_cache[i].close_bracket ) free_string( bracket_cache[i].close_bracket );
		if ( bracket_cache[i].accent_color ) free_string( bracket_cache[i].accent_color );
		if ( bracket_cache[i].primary_color ) free_string( bracket_cache[i].primary_color );
	}
	bracket_count = 0;

	for ( i = 0; i < gen_count; i++ ) {
		if ( gen_cache[i].title ) free_string( gen_cache[i].title );
		if ( gen_cache[i].title_color ) free_string( gen_cache[i].title_color );
	}
	gen_count = 0;

	/* Load brackets */
	if ( sqlite3_prepare_v2( class_db,
			"SELECT class_id, open_bracket, close_bracket, accent_color, primary_color "
			"FROM class_brackets ORDER BY class_id",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && bracket_count < MAX_CACHED_BRACKETS ) {
			const char *text;
			bracket_cache[bracket_count].class_id      = sqlite3_column_int( stmt, 0 );
			bracket_cache[bracket_count].open_bracket  = str_dup( col_text( stmt, 1 ) );
			bracket_cache[bracket_count].close_bracket = str_dup( col_text( stmt, 2 ) );
			text = (const char *)sqlite3_column_text( stmt, 3 );
			bracket_cache[bracket_count].accent_color  = text ? str_dup( text ) : NULL;
			text = (const char *)sqlite3_column_text( stmt, 4 );
			bracket_cache[bracket_count].primary_color = text ? str_dup( text ) : NULL;
			bracket_count++;
		}
		sqlite3_finalize( stmt );
	}

	/* Load generations - join with brackets to get title_color from primary_color */
	if ( sqlite3_prepare_v2( class_db,
			"SELECT g.class_id, g.generation, g.title, b.primary_color "
			"FROM class_generations g "
			"LEFT JOIN class_brackets b ON g.class_id = b.class_id "
			"ORDER BY g.class_id, g.generation DESC",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && gen_count < MAX_CACHED_GENERATIONS ) {
			const char *text;
			gen_cache[gen_count].class_id   = sqlite3_column_int( stmt, 0 );
			gen_cache[gen_count].generation = sqlite3_column_int( stmt, 1 );
			gen_cache[gen_count].title      = str_dup( col_text( stmt, 2 ) );
			text = (const char *)sqlite3_column_text( stmt, 3 );
			gen_cache[gen_count].title_color = text ? str_dup( text ) : NULL;
			gen_count++;
		}
		sqlite3_finalize( stmt );
	}

	snprintf( buf, sizeof( buf ),
		"  Loaded %d class brackets, %d generation titles.", bracket_count, gen_count );
	log_string( buf );
}

const CLASS_BRACKET *db_class_get_bracket( int class_id ) {
	int i;
	for ( i = 0; i < bracket_count; i++ ) {
		if ( bracket_cache[i].class_id == class_id )
			return &bracket_cache[i];
	}
	return NULL;
}

const char *db_class_get_generation_title( int class_id, int generation ) {
	const CLASS_GENERATION *gen = db_class_get_generation( class_id, generation );
	return gen ? gen->title : NULL;
}

const CLASS_GENERATION *db_class_get_generation( int class_id, int generation ) {
	int i;
	const CLASS_GENERATION *fallback = NULL;

	for ( i = 0; i < gen_count; i++ ) {
		if ( gen_cache[i].class_id == class_id ) {
			if ( gen_cache[i].generation == generation )
				return &gen_cache[i];
			if ( gen_cache[i].generation == 0 )
				fallback = &gen_cache[i];
		}
	}
	return fallback;
}


/***************************************************************************
 * Class Aura Config (class_auras)
 ***************************************************************************/

void db_class_load_aura( void ) {
	sqlite3_stmt *stmt;
	char buf[256];
	int i;

	if ( !class_db )
		return;

	for ( i = 0; i < aura_count; i++ ) {
		if ( aura_cache[i].aura_text )   free_string( aura_cache[i].aura_text );
		if ( aura_cache[i].mxp_tooltip ) free_string( aura_cache[i].mxp_tooltip );
	}
	aura_count = 0;

	/* Join with class_registry to get class_name for tooltip */
	if ( sqlite3_prepare_v2( class_db,
			"SELECT a.class_id, a.aura_text, r.class_name, a.display_order "
			"FROM class_auras a "
			"JOIN class_registry r ON a.class_id = r.class_id "
			"ORDER BY a.display_order, a.class_id",
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

const CLASS_AURA *db_class_get_aura( int class_id ) {
	int i;
	for ( i = 0; i < aura_count; i++ ) {
		if ( aura_cache[i].class_id == class_id )
			return &aura_cache[i];
	}
	return NULL;
}

int db_class_get_aura_count( void ) {
	return aura_count;
}

const CLASS_AURA *db_class_get_aura_by_index( int index ) {
	if ( index < 0 || index >= aura_count )
		return NULL;
	return &aura_cache[index];
}


/***************************************************************************
 * Class Armor Config (class_armor_config, class_armor_pieces)
 ***************************************************************************/

void db_class_load_armor( void ) {
	sqlite3_stmt *stmt;
	char buf[256];
	int i;

	if ( !class_db )
		return;

	for ( i = 0; i < armor_config_count; i++ ) {
		if ( armor_config_cache[i].acfg_cost_key ) free_string( armor_config_cache[i].acfg_cost_key );
		if ( armor_config_cache[i].usage_message ) free_string( armor_config_cache[i].usage_message );
		if ( armor_config_cache[i].act_to_char )   free_string( armor_config_cache[i].act_to_char );
		if ( armor_config_cache[i].act_to_room )   free_string( armor_config_cache[i].act_to_room );
	}
	armor_config_count = 0;

	for ( i = 0; i < armor_piece_count; i++ ) {
		if ( armor_piece_cache[i].keyword ) free_string( armor_piece_cache[i].keyword );
	}
	armor_piece_count = 0;

	/* Load armor configs */
	if ( sqlite3_prepare_v2( class_db,
			"SELECT class_id, acfg_cost_key, usage_message, act_to_char, act_to_room, mastery_vnum "
			"FROM class_armor_config ORDER BY class_id",
			-1, &stmt, NULL ) == SQLITE_OK ) {
		while ( sqlite3_step( stmt ) == SQLITE_ROW && armor_config_count < MAX_CACHED_ARMOR_CONFIGS ) {
			armor_config_cache[armor_config_count].class_id      = sqlite3_column_int( stmt, 0 );
			armor_config_cache[armor_config_count].acfg_cost_key = str_dup( col_text( stmt, 1 ) );
			armor_config_cache[armor_config_count].usage_message = str_dup( col_text( stmt, 2 ) );
			armor_config_cache[armor_config_count].act_to_char   = str_dup( col_text( stmt, 3 ) );
			armor_config_cache[armor_config_count].act_to_room   = str_dup( col_text( stmt, 4 ) );
			armor_config_cache[armor_config_count].mastery_vnum  = sqlite3_column_type( stmt, 5 ) == SQLITE_NULL ? 0 : sqlite3_column_int( stmt, 5 );
			armor_config_count++;
		}
		sqlite3_finalize( stmt );
	}

	/* Load armor pieces */
	if ( sqlite3_prepare_v2( class_db,
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

const CLASS_ARMOR_CONFIG *db_class_get_armor_config( int class_id ) {
	int i;
	for ( i = 0; i < armor_config_count; i++ ) {
		if ( armor_config_cache[i].class_id == class_id )
			return &armor_config_cache[i];
	}
	return NULL;
}

int db_class_get_armor_vnum( int class_id, const char *keyword ) {
	int i;
	for ( i = 0; i < armor_piece_count; i++ ) {
		if ( armor_piece_cache[i].class_id == class_id &&
		     !str_cmp( armor_piece_cache[i].keyword, keyword ) )
			return armor_piece_cache[i].vnum;
	}
	return 0;
}


/***************************************************************************
 * Class Starting Config (class_starting)
 ***************************************************************************/

void db_class_load_starting( void ) {
	sqlite3_stmt *stmt;
	char buf[256];

	if ( !class_db )
		return;

	starting_count = 0;

	if ( sqlite3_prepare_v2( class_db,
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

const CLASS_STARTING *db_class_get_starting( int class_id ) {
	int i;
	for ( i = 0; i < starting_count; i++ ) {
		if ( starting_cache[i].class_id == class_id )
			return &starting_cache[i];
	}
	return NULL;
}


/***************************************************************************
 * Class Score Stats (class_score_stats)
 ***************************************************************************/

void db_class_load_score( void ) {
	sqlite3_stmt *stmt;
	char buf[256];

	if ( !class_db )
		return;

	score_stats_count = 0;

	if ( sqlite3_prepare_v2( class_db,
			"SELECT class_id, stat_source, stat_source_max, stat_label, format_string, display_order "
			"FROM class_score_stats ORDER BY class_id, display_order",
			-1, &stmt, NULL ) == SQLITE_OK ) {
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
	}

	snprintf( buf, sizeof( buf ), "  Loaded %d class score stats.", score_stats_count );
	log_string( buf );
}

int db_class_get_score_stat_count( int class_id ) {
	int i, count = 0;
	for ( i = 0; i < score_stats_count; i++ ) {
		if ( score_stats_cache[i].class_id == class_id )
			count++;
	}
	return count;
}

const CLASS_SCORE_STAT *db_class_get_score_stats( int class_id ) {
	int i;
	for ( i = 0; i < score_stats_count; i++ ) {
		if ( score_stats_cache[i].class_id == class_id )
			return &score_stats_cache[i];
	}
	return NULL;
}

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
		case STAT_DRAGON_ATTUNEMENT:  return ch->pcdata->powers[DRAGON_ATTUNEMENT];
		case STAT_DRAGON_ESSENCE_PEAK: return ch->pcdata->stats[DRAGON_ESSENCE_PEAK];
		default:                  return 0;
	}
}


/***************************************************************************
 * Class Registry (class_registry)
 ***************************************************************************/

void db_class_load_registry( void ) {
	sqlite3_stmt *stmt;
	char buf[256];
	const char *text;

	if ( !class_db )
		return;

	registry_count = 0;

	if ( sqlite3_prepare_v2( class_db,
			"SELECT class_id, class_name, keyword, keyword_alt, mudstat_label, "
			"selfclass_message, display_order, upgrade_class, requirements "
			"FROM class_registry ORDER BY display_order, class_id",
			-1, &stmt, NULL ) == SQLITE_OK ) {
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
	}

	snprintf( buf, sizeof( buf ), "  Loaded %d class registry entries.", registry_count );
	log_string( buf );
}

int db_class_get_registry_count( void ) {
	return registry_count;
}

const CLASS_REGISTRY_ENTRY *db_class_get_registry_by_id( int class_id ) {
	int i;
	for ( i = 0; i < registry_count; i++ ) {
		if ( registry_cache[i].class_id == class_id )
			return &registry_cache[i];
	}
	return NULL;
}

const CLASS_REGISTRY_ENTRY *db_class_get_registry_by_keyword( const char *keyword ) {
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

const CLASS_REGISTRY_ENTRY *db_class_get_registry_by_index( int index ) {
	if ( index < 0 || index >= registry_count )
		return NULL;
	return &registry_cache[index];
}

const char *db_class_get_name( int class_id ) {
	const CLASS_REGISTRY_ENTRY *reg = db_class_get_registry_by_id( class_id );
	return reg ? reg->class_name : "Hero";
}


/***************************************************************************
 * Class Equipment Restrictions (derived from armor pieces)
 ***************************************************************************/

#define MAX_VNUM_RANGES 32

static CLASS_VNUM_RANGE vnum_range_cache[MAX_VNUM_RANGES];
static int vnum_range_count = 0;

/*
 * Build vnum ranges from loaded armor pieces.
 * Call this AFTER db_class_load_armor() during boot.
 */
void db_class_build_vnum_ranges( void ) {
	char buf[256];
	int i;
	int current_class = 0;
	int vnum_min = 0, vnum_max = 0;

	vnum_range_count = 0;

	/* armor_piece_cache is already sorted by class_id from SQL ORDER BY */
	for ( i = 0; i < armor_piece_count; i++ ) {
		if ( armor_piece_cache[i].class_id != current_class ) {
			/* Save previous range if we had one */
			if ( current_class != 0 && vnum_range_count < MAX_VNUM_RANGES ) {
				vnum_range_cache[vnum_range_count].class_id   = current_class;
				vnum_range_cache[vnum_range_count].vnum_start = vnum_min;
				vnum_range_cache[vnum_range_count].vnum_end   = vnum_max;
				vnum_range_count++;
			}

			/* Start new class */
			current_class = armor_piece_cache[i].class_id;
			vnum_min = armor_piece_cache[i].vnum;
			vnum_max = armor_piece_cache[i].vnum;
		} else {
			/* Update range */
			if ( armor_piece_cache[i].vnum < vnum_min )
				vnum_min = armor_piece_cache[i].vnum;
			if ( armor_piece_cache[i].vnum > vnum_max )
				vnum_max = armor_piece_cache[i].vnum;
		}
	}

	/* Don't forget the last class */
	if ( current_class != 0 && vnum_range_count < MAX_VNUM_RANGES ) {
		vnum_range_cache[vnum_range_count].class_id   = current_class;
		vnum_range_cache[vnum_range_count].vnum_start = vnum_min;
		vnum_range_cache[vnum_range_count].vnum_end   = vnum_max;
		vnum_range_count++;
	}

	snprintf( buf, sizeof( buf ), "  Built %d class equipment vnum ranges.", vnum_range_count );
	log_string( buf );
}

/*
 * Check if an object is class-restricted equipment that the character cannot use.
 * Returns TRUE if the character should be zapped (restricted), FALSE if allowed.
 */
bool db_class_is_equipment_restricted( CHAR_DATA *ch, OBJ_DATA *obj ) {
	int i;
	int vnum;

	if ( ch == NULL || obj == NULL || obj->pIndexData == NULL )
		return FALSE;

	/* NPCs are always restricted from class equipment */
	if ( IS_NPC( ch ) ) {
		vnum = obj->pIndexData->vnum;
		for ( i = 0; i < vnum_range_count; i++ ) {
			if ( vnum >= vnum_range_cache[i].vnum_start &&
			     vnum <= vnum_range_cache[i].vnum_end )
				return TRUE;
		}
		return FALSE;
	}

	vnum = obj->pIndexData->vnum;

	/* Check each class vnum range */
	for ( i = 0; i < vnum_range_count; i++ ) {
		if ( vnum >= vnum_range_cache[i].vnum_start &&
		     vnum <= vnum_range_cache[i].vnum_end ) {
			/* Object is in this class's range - check if character is that class */
			if ( !IS_CLASS( ch, vnum_range_cache[i].class_id ) )
				return TRUE;  /* Not the right class - restricted */
			else
				return FALSE; /* Is the right class - allowed */
		}
	}

	/* Not class equipment */
	return FALSE;
}

