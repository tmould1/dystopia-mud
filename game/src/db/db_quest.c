/***************************************************************************
 *  db_quest.c - SQLite persistence for the quest system
 *
 *  Loads quest definitions from quest.db at boot, caches in memory.
 *  Provides load/save for per-player quest progress in player.db.
 *
 *  Pattern follows db_tables.c: open db read-only at boot, populate
 *  static arrays, close.  Player progress uses the player's own .db.
 ***************************************************************************/

#include "db_util.h"
#include "db_quest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External: game database directory set by db_game_init() */
extern char mud_db_game_dir[];

/*--------------------------------------------------------------------------
 * Static storage for quest definitions
 *--------------------------------------------------------------------------*/

static QUEST_DEF quest_defs[MAX_QUEST_DEFS];
static int       quest_count = 0;

/*--------------------------------------------------------------------------
 * Static storage for story clues (nodes 1-16, stages 0-1)
 *--------------------------------------------------------------------------*/

#define STORY_NODE_MAX    16
#define STORY_STAGE_COUNT  2
#define STORY_CLUE_MAX   MAX_STRING_LENGTH

static char story_clue_table[STORY_NODE_MAX + 1][STORY_STAGE_COUNT][STORY_CLUE_MAX];
static int  story_clue_count = 0;

/*--------------------------------------------------------------------------
 * Player progress table schema (added to each player.db)
 *--------------------------------------------------------------------------*/

static const char *QUEST_PROGRESS_SCHEMA =
    "CREATE TABLE IF NOT EXISTS quest_progress ("
    "  quest_id     TEXT PRIMARY KEY,"
    "  status       INTEGER NOT NULL DEFAULT 0,"
    "  started_at   INTEGER NOT NULL DEFAULT 0,"
    "  completed_at INTEGER NOT NULL DEFAULT 0"
    ");"
    "CREATE TABLE IF NOT EXISTS quest_obj_progress ("
    "  quest_id  TEXT NOT NULL,"
    "  obj_index INTEGER NOT NULL,"
    "  current   INTEGER NOT NULL DEFAULT 0,"
    "  PRIMARY KEY (quest_id, obj_index)"
    ");";

/*--------------------------------------------------------------------------
 * Helper: safe string copy into fixed buffer
 *--------------------------------------------------------------------------*/

static void safe_copy( char *dst, int dst_size, const char *src ) {
    if ( !src ) src = "";
    snprintf( dst, dst_size, "%s", src );
}

/*--------------------------------------------------------------------------
 * Lifecycle: Load quest definitions from quest.db
 *--------------------------------------------------------------------------*/

void db_quest_init( void ) {
    sqlite3      *db = NULL;
    sqlite3_stmt *stmt = NULL;
    char          path[MUD_PATH_MAX];

    if ( snprintf( path, sizeof( path ), "%s%squest.db",
            mud_db_game_dir, PATH_SEPARATOR ) >= (int)sizeof( path ) ) {
        bug( "db_quest_init: quest.db path truncated.", 0 );
    }

    if ( sqlite3_open_v2( path, &db, SQLITE_OPEN_READONLY, NULL ) != SQLITE_OK ) {
        char errbuf[MUD_PATH_MAX + 64];
        snprintf( errbuf, sizeof( errbuf ),
            "db_quest_init: quest.db not found at %s", path );
        bug( errbuf, 0 );
        bug( "  Run: python game/tools/seeders/seed_quest_db.py to generate it.", 0 );
        return;
    }

    /*
     * Load quest definitions
     */
    if ( sqlite3_prepare_v2( db,
            "SELECT id, name, description, category, tier, flags,"
            " min_explevel, max_explevel, qp_reward, exp_reward,"
            " primal_reward, item_reward_vnum, required_class"
            " FROM quest_defs ORDER BY sort_order",
            -1, &stmt, NULL ) != SQLITE_OK ) {
        bug( "db_quest_init: failed to prepare quest_defs query.", 0 );
        sqlite3_close( db );
        return;
    }

    quest_count = 0;
    while ( sqlite3_step( stmt ) == SQLITE_ROW && quest_count < MAX_QUEST_DEFS ) {
        QUEST_DEF *q = &quest_defs[quest_count];
        memset( q, 0, sizeof( *q ) );

        safe_copy( q->id,          sizeof( q->id ),          col_text( stmt, 0 ) );
        safe_copy( q->name,        sizeof( q->name ),        col_text( stmt, 1 ) );
        safe_copy( q->description, sizeof( q->description ), col_text( stmt, 2 ) );
        safe_copy( q->category,    sizeof( q->category ),    col_text( stmt, 3 ) );
        q->tier             = sqlite3_column_int( stmt, 4 );
        q->flags            = sqlite3_column_int( stmt, 5 );
        q->min_explevel     = sqlite3_column_int( stmt, 6 );
        q->max_explevel     = sqlite3_column_int( stmt, 7 );
        q->qp_reward        = sqlite3_column_int( stmt, 8 );
        q->exp_reward       = sqlite3_column_int( stmt, 9 );
        q->primal_reward    = sqlite3_column_int( stmt, 10 );
        q->item_reward_vnum = sqlite3_column_int( stmt, 11 );
        q->required_class   = sqlite3_column_int( stmt, 12 );
        q->obj_count        = 0;
        q->prereq_count     = 0;

        quest_count++;
    }
    sqlite3_finalize( stmt );

    /*
     * Load objectives for each quest
     */
    if ( sqlite3_prepare_v2( db,
            "SELECT quest_id, type, target, threshold, description"
            " FROM quest_objectives ORDER BY quest_id, obj_index",
            -1, &stmt, NULL ) == SQLITE_OK ) {
        while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
            const char *qid = col_text( stmt, 0 );
            int idx = quest_def_index_by_id( qid );
            if ( idx < 0 ) continue;

            QUEST_DEF *q = &quest_defs[idx];
            if ( q->obj_count >= MAX_QUEST_OBJECTIVES ) continue;

            QUEST_OBJ_DEF *obj = &q->objectives[q->obj_count];
            safe_copy( obj->type,        sizeof( obj->type ),        col_text( stmt, 1 ) );
            safe_copy( obj->target,      sizeof( obj->target ),      col_text( stmt, 2 ) );
            obj->threshold = sqlite3_column_int( stmt, 3 );
            safe_copy( obj->description, sizeof( obj->description ), col_text( stmt, 4 ) );
            q->obj_count++;
        }
        sqlite3_finalize( stmt );
    }

    /*
     * Load prerequisites and resolve to indices
     */
    if ( sqlite3_prepare_v2( db,
            "SELECT quest_id, requires_id FROM quest_prerequisites",
            -1, &stmt, NULL ) == SQLITE_OK ) {
        while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
            const char *qid = col_text( stmt, 0 );
            const char *rid = col_text( stmt, 1 );
            int qi = quest_def_index_by_id( qid );
            int ri = quest_def_index_by_id( rid );
            if ( qi < 0 || ri < 0 ) continue;

            QUEST_DEF *q = &quest_defs[qi];
            if ( q->prereq_count >= MAX_QUEST_PREREQS ) continue;
            q->prereq_indices[q->prereq_count++] = ri;
        }
        sqlite3_finalize( stmt );
    }

    /*
     * Load story clues (gracefully skip if table doesn't exist)
     */
    memset( story_clue_table, 0, sizeof( story_clue_table ) );
    story_clue_count = 0;

    if ( sqlite3_prepare_v2( db,
            "SELECT node, stage, clue FROM story_clues",
            -1, &stmt, NULL ) == SQLITE_OK ) {
        while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
            int node  = sqlite3_column_int( stmt, 0 );
            int stage = sqlite3_column_int( stmt, 1 );

            if ( node < 1 || node > STORY_NODE_MAX ) continue;
            if ( stage < 0 || stage >= STORY_STAGE_COUNT ) continue;

            safe_copy( story_clue_table[node][stage],
                       STORY_CLUE_MAX,
                       col_text( stmt, 2 ) );
            story_clue_count++;
        }
        sqlite3_finalize( stmt );
    }

    sqlite3_close( db );

    {
        char logbuf[MAX_STRING_LENGTH];
        snprintf( logbuf, sizeof( logbuf ),
            "  db_quest_init: loaded %d quest definitions, %d story clues.",
            quest_count, story_clue_count );
        log_string( logbuf );
    }
}

/*--------------------------------------------------------------------------
 * Accessors: Quest Definitions
 *--------------------------------------------------------------------------*/

int quest_def_count( void ) {
    return quest_count;
}

const QUEST_DEF *quest_def_by_index( int index ) {
    if ( index < 0 || index >= quest_count )
        return NULL;
    return &quest_defs[index];
}

const QUEST_DEF *quest_def_by_id( const char *id ) {
    int idx = quest_def_index_by_id( id );
    return ( idx >= 0 ) ? &quest_defs[idx] : NULL;
}

int quest_def_index_by_id( const char *id ) {
    int i;
    if ( !id || !id[0] )
        return -1;
    for ( i = 0; i < quest_count; i++ ) {
        if ( !str_cmp( quest_defs[i].id, id ) )
            return i;
    }
    return -1;
}

/*--------------------------------------------------------------------------
 * Accessors: Story Clues
 *--------------------------------------------------------------------------*/

const char *story_clue_lookup( int node, int stage ) {
    if ( node < 1 || node > STORY_NODE_MAX ) return "";
    if ( stage < 0 || stage >= STORY_STAGE_COUNT ) return "";
    return story_clue_table[node][stage];
}

/*--------------------------------------------------------------------------
 * Quest Tracker: Memory Management
 *--------------------------------------------------------------------------*/

QUEST_TRACKER *quest_tracker_new( void ) {
    QUEST_TRACKER *t = calloc( 1, sizeof( QUEST_TRACKER ) );
    t->entries  = NULL;
    t->count    = 0;
    t->capacity = 0;
    return t;
}

void quest_tracker_free( QUEST_TRACKER *tracker ) {
    if ( !tracker ) return;
    free( tracker->entries );
    free( tracker );
}

static void tracker_grow( QUEST_TRACKER *t ) {
    int new_cap = ( t->capacity == 0 ) ? 16 : t->capacity * 2;
    QUEST_PROGRESS *new_entries = realloc( t->entries,
        new_cap * sizeof( QUEST_PROGRESS ) );
    if ( !new_entries ) return;
    t->entries  = new_entries;
    t->capacity = new_cap;
}

QUEST_PROGRESS *quest_tracker_find( QUEST_TRACKER *tracker, int quest_index ) {
    int i;
    if ( !tracker ) return NULL;
    for ( i = 0; i < tracker->count; i++ ) {
        if ( tracker->entries[i].quest_index == quest_index )
            return &tracker->entries[i];
    }
    return NULL;
}

QUEST_PROGRESS *quest_tracker_get( QUEST_TRACKER *tracker, int quest_index ) {
    QUEST_PROGRESS *p;
    if ( !tracker ) return NULL;

    p = quest_tracker_find( tracker, quest_index );
    if ( p ) return p;

    if ( tracker->count >= tracker->capacity )
        tracker_grow( tracker );
    if ( tracker->count >= tracker->capacity )
        return NULL;  /* allocation failure */

    p = &tracker->entries[tracker->count];
    memset( p, 0, sizeof( *p ) );
    p->quest_index = quest_index;
    p->status      = QSTATUS_LOCKED;
    tracker->count++;
    return p;
}

/*--------------------------------------------------------------------------
 * Player Progress: Table Creation
 *--------------------------------------------------------------------------*/

void quest_progress_ensure_tables( sqlite3 *player_db ) {
    char *errmsg = NULL;
    if ( !player_db ) return;
    if ( sqlite3_exec( player_db, QUEST_PROGRESS_SCHEMA,
            NULL, NULL, &errmsg ) != SQLITE_OK ) {
        bug( "quest_progress_ensure_tables: schema error", 0 );
        if ( errmsg ) sqlite3_free( errmsg );
    }
}

/*--------------------------------------------------------------------------
 * Player Progress: Load
 *--------------------------------------------------------------------------*/

void quest_progress_load( QUEST_TRACKER *tracker, sqlite3 *player_db ) {
    sqlite3_stmt *stmt;

    if ( !tracker || !player_db ) return;

    quest_progress_ensure_tables( player_db );

    /*
     * Load quest status
     */
    if ( sqlite3_prepare_v2( player_db,
            "SELECT quest_id, status, started_at, completed_at"
            " FROM quest_progress",
            -1, &stmt, NULL ) != SQLITE_OK )
        return;

    while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
        const char *qid = col_text( stmt, 0 );
        int qi = quest_def_index_by_id( qid );
        if ( qi < 0 ) continue;

        QUEST_PROGRESS *p = quest_tracker_get( tracker, qi );
        if ( !p ) continue;
        p->status       = sqlite3_column_int( stmt, 1 );
        p->started_at   = sqlite3_column_int( stmt, 2 );
        p->completed_at = sqlite3_column_int( stmt, 3 );
    }
    sqlite3_finalize( stmt );

    /*
     * Load objective progress
     */
    if ( sqlite3_prepare_v2( player_db,
            "SELECT quest_id, obj_index, current"
            " FROM quest_obj_progress",
            -1, &stmt, NULL ) != SQLITE_OK )
        return;

    while ( sqlite3_step( stmt ) == SQLITE_ROW ) {
        const char *qid = col_text( stmt, 0 );
        int qi = quest_def_index_by_id( qid );
        if ( qi < 0 ) continue;

        QUEST_PROGRESS *p = quest_tracker_find( tracker, qi );
        if ( !p ) continue;

        int obj_idx = sqlite3_column_int( stmt, 1 );
        if ( obj_idx < 0 || obj_idx >= MAX_QUEST_OBJECTIVES ) continue;
        p->obj_progress[obj_idx].current = sqlite3_column_int( stmt, 2 );
    }
    sqlite3_finalize( stmt );
}

/*--------------------------------------------------------------------------
 * Player Progress: Save
 *--------------------------------------------------------------------------*/

void quest_progress_save( const QUEST_TRACKER *tracker, sqlite3 *player_db ) {
    sqlite3_stmt *stmt_prog = NULL;
    sqlite3_stmt *stmt_obj  = NULL;
    int i, j;

    if ( !tracker || !player_db ) return;

    quest_progress_ensure_tables( player_db );
    db_begin( player_db );

    /* Clear existing progress */
    sqlite3_exec( player_db, "DELETE FROM quest_progress", NULL, NULL, NULL );
    sqlite3_exec( player_db, "DELETE FROM quest_obj_progress", NULL, NULL, NULL );

    /* Insert current progress */
    if ( sqlite3_prepare_v2( player_db,
            "INSERT INTO quest_progress (quest_id, status, started_at, completed_at)"
            " VALUES (?, ?, ?, ?)",
            -1, &stmt_prog, NULL ) != SQLITE_OK )
        goto done;

    if ( sqlite3_prepare_v2( player_db,
            "INSERT INTO quest_obj_progress (quest_id, obj_index, current)"
            " VALUES (?, ?, ?)",
            -1, &stmt_obj, NULL ) != SQLITE_OK )
        goto done;

    for ( i = 0; i < tracker->count; i++ ) {
        const QUEST_PROGRESS *p = &tracker->entries[i];
        const QUEST_DEF *q = quest_def_by_index( p->quest_index );
        if ( !q ) continue;

        /* Only save quests that have been touched */
        if ( p->status == QSTATUS_LOCKED ) continue;

        sqlite3_reset( stmt_prog );
        sqlite3_bind_text( stmt_prog, 1, q->id, -1, SQLITE_STATIC );
        sqlite3_bind_int(  stmt_prog, 2, p->status );
        sqlite3_bind_int(  stmt_prog, 3, p->started_at );
        sqlite3_bind_int(  stmt_prog, 4, p->completed_at );
        sqlite3_step( stmt_prog );

        /* Save objective progress */
        for ( j = 0; j < q->obj_count; j++ ) {
            if ( p->obj_progress[j].current <= 0 ) continue;
            sqlite3_reset( stmt_obj );
            sqlite3_bind_text( stmt_obj, 1, q->id, -1, SQLITE_STATIC );
            sqlite3_bind_int(  stmt_obj, 2, j );
            sqlite3_bind_int(  stmt_obj, 3, p->obj_progress[j].current );
            sqlite3_step( stmt_obj );
        }
    }

done:
    if ( stmt_prog ) sqlite3_finalize( stmt_prog );
    if ( stmt_obj )  sqlite3_finalize( stmt_obj );
    db_commit( player_db );
}
