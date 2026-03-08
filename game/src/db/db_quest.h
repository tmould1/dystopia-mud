/***************************************************************************
 *  db_quest.h - SQLite persistence for the quest system
 *
 *  Manages two databases:
 *    - quest.db (gamedata/db/game/) : Quest definitions, objectives,
 *      prerequisites.  Read-only at runtime, pre-populated by
 *      seed_quest_db.py.
 *    - player.db (per-character)    : Quest progress and objective
 *      progress.  Read/write at runtime via db_player integration.
 *
 *  Follows the same boot-load, cache-in-memory pattern as db_tables.
 ***************************************************************************/

#ifndef DB_QUEST_H
#define DB_QUEST_H

#include "../core/merc.h"

/*--------------------------------------------------------------------------
 * Quest flag constants
 *--------------------------------------------------------------------------*/

#define QFLAG_AUTO_COMPLETE  (1 << 0)  /* Completes automatically on trigger */
#define QFLAG_REPEATABLE     (1 << 1)  /* Can be completed multiple times   */
#define QFLAG_FTUE_SKIP      (1 << 2)  /* Auto-skip for experienced players */

/*--------------------------------------------------------------------------
 * Quest status values (stored in player.db quest_progress.status)
 *--------------------------------------------------------------------------*/

#define QSTATUS_LOCKED       0  /* Prerequisites not met               */
#define QSTATUS_AVAILABLE    1  /* Can be accepted                     */
#define QSTATUS_ACTIVE       2  /* Currently in progress               */
#define QSTATUS_COMPLETE     3  /* All objectives met, awaiting turn-in*/
#define QSTATUS_TURNED_IN    4  /* Rewards claimed, done               */

/*--------------------------------------------------------------------------
 * Maximum definitions
 *--------------------------------------------------------------------------*/

#define MAX_QUEST_DEFS       256  /* Max quest definitions loaded       */
#define MAX_QUEST_OBJECTIVES  8   /* Max objectives per quest           */
#define MAX_QUEST_PREREQS     8   /* Max prerequisites per quest        */
#define MAX_QUEST_ID_LEN     32   /* Max quest ID string length         */

/*--------------------------------------------------------------------------
 * Objective type strings (stored in quest.db quest_objectives.type)
 *--------------------------------------------------------------------------*/

#define QOBJ_USE_COMMAND       "USE_COMMAND"
#define QOBJ_KILL_MOB          "KILL_MOB"
#define QOBJ_KILL_PLAYER       "KILL_PLAYER"
#define QOBJ_VISIT_ROOM        "VISIT_ROOM"
#define QOBJ_VISIT_AREA        "VISIT_AREA"
#define QOBJ_REACH_STAT        "REACH_STAT"
#define QOBJ_REACH_GEN         "REACH_GEN"
#define QOBJ_REACH_PKSCORE     "REACH_PKSCORE"
#define QOBJ_REACH_UPGRADE     "REACH_UPGRADE"
#define QOBJ_EARN_QP           "EARN_QP"
#define QOBJ_LEARN_STANCE      "LEARN_STANCE"
#define QOBJ_LEARN_SUPERSTANCE "LEARN_SUPERSTANCE"
#define QOBJ_LEARN_DISCIPLINE  "LEARN_DISCIPLINE"
#define QOBJ_WEAPON_SKILL      "WEAPON_SKILL"
#define QOBJ_SPELL_SKILL       "SPELL_SKILL"
#define QOBJ_FORGE_ITEM        "FORGE_ITEM"
#define QOBJ_QUEST_CREATE      "QUEST_CREATE"
#define QOBJ_QUEST_MODIFY      "QUEST_MODIFY"
#define QOBJ_ARENA_WIN         "ARENA_WIN"
#define QOBJ_COLLECT_ITEM      "COLLECT_ITEM"
#define QOBJ_COMPLETE_QUEST    "COMPLETE_QUEST"
#define QOBJ_MASTERY           "MASTERY"
#define QOBJ_CLASS_TRAIN       "CLASS_TRAIN"
#define QOBJ_CLASS_POWER       "CLASS_POWER"

/*--------------------------------------------------------------------------
 * Data Structures
 *--------------------------------------------------------------------------*/

typedef struct quest_objective_def {
    char  type[32];           /* Objective type string                 */
    char  target[64];         /* Target (vnum, command name, stat)     */
    int   threshold;          /* Required count/level                  */
    char  description[256];   /* Human-readable description            */
} QUEST_OBJ_DEF;

typedef struct quest_def {
    char  id[MAX_QUEST_ID_LEN]; /* Unique quest ID (e.g. "M01")       */
    char  name[64];             /* Display name                        */
    char  description[512];     /* Quest description                   */
    char  category[4];          /* M/T/C/E/CL/CR/D/A                   */
    int   tier;                 /* Progression phase 0-5               */
    int   flags;                /* QFLAG_* bitmask                     */
    int   min_explevel;         /* Min FTUE level (0=all)              */
    int   max_explevel;         /* Max FTUE level (3=all)              */
    int   qp_reward;            /* Quest point reward                  */
    int   exp_reward;           /* Experience reward                   */
    int   primal_reward;        /* Primal energy reward                */
    int   item_reward_vnum;     /* Reward item vnum (0=none)           */
    int   required_class;       /* CLASS_* constant (0=any)            */

    /* Objectives */
    QUEST_OBJ_DEF objectives[MAX_QUEST_OBJECTIVES];
    int   obj_count;

    /* Prerequisites (stored as indices into the global quest_defs array) */
    int   prereq_indices[MAX_QUEST_PREREQS];
    int   prereq_count;
} QUEST_DEF;

/*--------------------------------------------------------------------------
 * Per-player quest progress (in-memory, saved to player.db)
 *--------------------------------------------------------------------------*/

typedef struct quest_obj_progress {
    int   current;            /* Current progress count               */
} QUEST_OBJ_PROGRESS;

typedef struct quest_progress {
    int   quest_index;        /* Index into global quest_defs array   */
    int   status;             /* QSTATUS_*                            */
    int   started_at;         /* Unix timestamp                       */
    int   completed_at;       /* Unix timestamp                       */
    QUEST_OBJ_PROGRESS obj_progress[MAX_QUEST_OBJECTIVES];
} QUEST_PROGRESS;

typedef struct quest_tracker {
    QUEST_PROGRESS *entries;  /* Dynamic array of tracked quests      */
    int   count;              /* Number of entries                    */
    int   capacity;           /* Allocated capacity                   */
} QUEST_TRACKER;

/*--------------------------------------------------------------------------
 * Lifecycle - Quest Definitions (quest.db)
 *--------------------------------------------------------------------------*/

/* Load quest definitions from quest.db at boot. Called from boot_db(). */
void db_quest_init( void );

/*--------------------------------------------------------------------------
 * Accessors - Quest Definitions
 *--------------------------------------------------------------------------*/

/* Get total number of quest definitions loaded. */
int quest_def_count( void );

/* Get quest definition by index (0-based). Returns NULL if out of range. */
const QUEST_DEF *quest_def_by_index( int index );

/* Find quest definition by ID string. Returns NULL if not found. */
const QUEST_DEF *quest_def_by_id( const char *id );

/* Find quest definition index by ID string. Returns -1 if not found. */
int quest_def_index_by_id( const char *id );

/*--------------------------------------------------------------------------
 * Story Clue Lookup (centralized in quest.db story_clues table)
 *--------------------------------------------------------------------------*/

/* Look up story clue text by node and stage (0=travel, 1=tasks).
 * Returns clue text or "" if not found. */
const char *story_clue_lookup( int node, int stage );

/*--------------------------------------------------------------------------
 * Per-Player Progress - Load/Save (player.db integration)
 *--------------------------------------------------------------------------*/

/* Create a new empty quest tracker for a character. */
QUEST_TRACKER *quest_tracker_new( void );

/* Free a quest tracker and all its entries. */
void quest_tracker_free( QUEST_TRACKER *tracker );

/* SQLite-dependent functions: only visible when sqlite3.h is included */
#ifdef SQLITE_VERSION
void quest_progress_load( QUEST_TRACKER *tracker, sqlite3 *player_db );
void quest_progress_save( const QUEST_TRACKER *tracker, sqlite3 *player_db );
void quest_progress_ensure_tables( sqlite3 *player_db );
#endif

/*--------------------------------------------------------------------------
 * Per-Player Progress - Accessors
 *--------------------------------------------------------------------------*/

/* Get progress entry for a quest, or NULL if not tracked. */
QUEST_PROGRESS *quest_tracker_find( QUEST_TRACKER *tracker, int quest_index );

/* Get or create a progress entry for a quest. */
QUEST_PROGRESS *quest_tracker_get( QUEST_TRACKER *tracker, int quest_index );

#endif /* DB_QUEST_H */
