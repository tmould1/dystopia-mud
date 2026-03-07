/*
 * Test suite for the quest system (db_quest + quest_new)
 *
 * Tests quest tracker lifecycle, progress tracking, event hooks,
 * milestone evaluation, prerequisite checking, auto-complete, and
 * FTUE-based auto-grant logic.
 *
 * Tier 2: Requires boot (quest definitions loaded from quest.db).
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"
#include "../db/db_quest.h"
#include "../systems/quest_new.h"

/*--------------------------------------------------------------------------
 * Tier 1: Quest Tracker (no boot required)
 *--------------------------------------------------------------------------*/

static void test_tracker_new_free( void ) {
	QUEST_TRACKER *t = quest_tracker_new();
	TEST_ASSERT( t != NULL );
	TEST_ASSERT_EQ( t->count, 0 );
	TEST_ASSERT_EQ( t->capacity, 0 );
	TEST_ASSERT( t->entries == NULL );
	quest_tracker_free( t );
}

static void test_tracker_free_null( void ) {
	/* Should not crash */
	quest_tracker_free( NULL );
	TEST_ASSERT( 1 );
}

static void test_tracker_get_creates_entry( void ) {
	QUEST_TRACKER *t = quest_tracker_new();
	QUEST_PROGRESS *p = quest_tracker_get( t, 5 );
	TEST_ASSERT( p != NULL );
	TEST_ASSERT_EQ( p->quest_index, 5 );
	TEST_ASSERT_EQ( p->status, QSTATUS_LOCKED );
	TEST_ASSERT_EQ( t->count, 1 );
	quest_tracker_free( t );
}

static void test_tracker_get_returns_existing( void ) {
	QUEST_TRACKER *t = quest_tracker_new();
	QUEST_PROGRESS *p1 = quest_tracker_get( t, 3 );
	QUEST_PROGRESS *p2 = quest_tracker_get( t, 3 );
	TEST_ASSERT( p1 == p2 );
	TEST_ASSERT_EQ( t->count, 1 );
	quest_tracker_free( t );
}

static void test_tracker_find_missing( void ) {
	QUEST_TRACKER *t = quest_tracker_new();
	QUEST_PROGRESS *p = quest_tracker_find( t, 99 );
	TEST_ASSERT( p == NULL );
	quest_tracker_free( t );
}

static void test_tracker_find_null( void ) {
	QUEST_PROGRESS *p = quest_tracker_find( NULL, 0 );
	TEST_ASSERT( p == NULL );
}

static void test_tracker_get_null( void ) {
	QUEST_PROGRESS *p = quest_tracker_get( NULL, 0 );
	TEST_ASSERT( p == NULL );
}

static void test_tracker_multiple_entries( void ) {
	QUEST_TRACKER *t = quest_tracker_new();
	QUEST_PROGRESS *p0, *p1, *p2;

	p0 = quest_tracker_get( t, 0 );
	p1 = quest_tracker_get( t, 7 );
	p2 = quest_tracker_get( t, 15 );

	TEST_ASSERT_EQ( t->count, 3 );
	TEST_ASSERT_EQ( p0->quest_index, 0 );
	TEST_ASSERT_EQ( p1->quest_index, 7 );
	TEST_ASSERT_EQ( p2->quest_index, 15 );

	/* Find should return the same pointers */
	TEST_ASSERT( quest_tracker_find( t, 7 ) == p1 );
	TEST_ASSERT( quest_tracker_find( t, 0 ) == p0 );
	TEST_ASSERT( quest_tracker_find( t, 15 ) == p2 );

	quest_tracker_free( t );
}

static void test_tracker_grows_capacity( void ) {
	QUEST_TRACKER *t = quest_tracker_new();
	int i;

	/* Add more entries than initial capacity (16) */
	for ( i = 0; i < 20; i++ ) {
		QUEST_PROGRESS *p = quest_tracker_get( t, i );
		TEST_ASSERT( p != NULL );
	}

	TEST_ASSERT_EQ( t->count, 20 );
	TEST_ASSERT( t->capacity >= 20 );

	/* Verify all entries are still accessible */
	for ( i = 0; i < 20; i++ ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, i );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT_EQ( p->quest_index, i );
	}

	quest_tracker_free( t );
}

static void test_progress_initial_state( void ) {
	QUEST_TRACKER *t = quest_tracker_new();
	QUEST_PROGRESS *p = quest_tracker_get( t, 0 );
	int i;

	TEST_ASSERT_EQ( p->status, QSTATUS_LOCKED );
	TEST_ASSERT_EQ( p->started_at, 0 );
	TEST_ASSERT_EQ( p->completed_at, 0 );

	for ( i = 0; i < MAX_QUEST_OBJECTIVES; i++ ) {
		TEST_ASSERT_EQ( p->obj_progress[i].current, 0 );
	}

	quest_tracker_free( t );
}

/*--------------------------------------------------------------------------
 * Tier 2: Quest Definitions (requires boot for quest.db)
 *--------------------------------------------------------------------------*/

static void test_quest_defs_loaded( void ) {
	TEST_ASSERT( quest_def_count() > 0 );
}

static void test_quest_def_by_index_valid( void ) {
	const QUEST_DEF *q = quest_def_by_index( 0 );
	TEST_ASSERT( q != NULL );
	TEST_ASSERT( q->id[0] != '\0' );
	TEST_ASSERT( q->name[0] != '\0' );
}

static void test_quest_def_by_index_invalid( void ) {
	TEST_ASSERT( quest_def_by_index( -1 ) == NULL );
	TEST_ASSERT( quest_def_by_index( 9999 ) == NULL );
}

static void test_quest_def_by_id_found( void ) {
	const QUEST_DEF *q = quest_def_by_id( "M01" );
	TEST_ASSERT( q != NULL );
	TEST_ASSERT_STR_EQ( q->id, "M01" );
}

static void test_quest_def_by_id_not_found( void ) {
	TEST_ASSERT( quest_def_by_id( "NONEXISTENT" ) == NULL );
	TEST_ASSERT( quest_def_by_id( "" ) == NULL );
	TEST_ASSERT( quest_def_by_id( NULL ) == NULL );
}

static void test_quest_def_index_by_id( void ) {
	int idx = quest_def_index_by_id( "M01" );
	TEST_ASSERT( idx >= 0 );

	const QUEST_DEF *q = quest_def_by_index( idx );
	TEST_ASSERT( q != NULL );
	TEST_ASSERT_STR_EQ( q->id, "M01" );
}

static void test_quest_def_has_objectives( void ) {
	const QUEST_DEF *q = quest_def_by_id( "M01" );
	TEST_ASSERT( q != NULL );
	TEST_ASSERT( q->obj_count > 0 );
	TEST_ASSERT( q->obj_count <= MAX_QUEST_OBJECTIVES );
	TEST_ASSERT( q->objectives[0].type[0] != '\0' );
	TEST_ASSERT( q->objectives[0].threshold > 0 );
}

static void test_quest_tutorial_chain( void ) {
	/* T01 should exist with no prerequisites */
	const QUEST_DEF *t01 = quest_def_by_id( "T01" );
	TEST_ASSERT( t01 != NULL );
	TEST_ASSERT_EQ( t01->prereq_count, 0 );
	TEST_ASSERT_STR_EQ( t01->category, "T" );
	TEST_ASSERT_EQ( t01->min_explevel, 0 );

	/* T02 should require T01 */
	const QUEST_DEF *t02 = quest_def_by_id( "T02" );
	TEST_ASSERT( t02 != NULL );
	TEST_ASSERT( t02->prereq_count > 0 );
}

static void test_quest_m01_objectives( void ) {
	/* M01 should have REACH_STAT hp, USE_COMMAND train, USE_COMMAND selfclass */
	const QUEST_DEF *q = quest_def_by_id( "M01" );
	TEST_ASSERT( q != NULL );
	TEST_ASSERT_EQ( q->obj_count, 3 );
	TEST_ASSERT_STR_EQ( q->objectives[0].type, "REACH_STAT" );
	TEST_ASSERT_STR_EQ( q->objectives[0].target, "hp" );
	TEST_ASSERT_STR_EQ( q->objectives[1].type, "USE_COMMAND" );
	TEST_ASSERT_STR_EQ( q->objectives[1].target, "train" );
	TEST_ASSERT_STR_EQ( q->objectives[2].type, "USE_COMMAND" );
	TEST_ASSERT_STR_EQ( q->objectives[2].target, "selfclass" );
}

/*--------------------------------------------------------------------------
 * Tier 2: Quest Progress Tracking (requires boot)
 *--------------------------------------------------------------------------*/

static void test_check_progress_use_command( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;

	/* Find a quest with USE_COMMAND objective */
	int qi = quest_def_index_by_id( "T01" );
	if ( qi >= 0 ) {
		const QUEST_DEF *q = quest_def_by_index( qi );
		QUEST_PROGRESS *p = quest_tracker_get( t, qi );
		p->status = QSTATUS_ACTIVE;

		/* Simulate using the "look" command */
		quest_check_progress( ch, QOBJ_USE_COMMAND, "look", 1 );

		/* Find the "look" objective and check progress */
		int j;
		for ( j = 0; j < q->obj_count; j++ ) {
			if ( !strcmp( q->objectives[j].type, QOBJ_USE_COMMAND )
					&& !strcmp( q->objectives[j].target, "look" ) ) {
				TEST_ASSERT_EQ( p->obj_progress[j].current, 1 );
				break;
			}
		}
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_check_progress_ignores_inactive( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;

	int qi = quest_def_index_by_id( "T01" );
	if ( qi >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_get( t, qi );
		/* Leave as LOCKED - should NOT track progress */
		p->status = QSTATUS_LOCKED;

		quest_check_progress( ch, QOBJ_USE_COMMAND, "look", 1 );

		int j;
		for ( j = 0; j < MAX_QUEST_OBJECTIVES; j++ ) {
			TEST_ASSERT_EQ( p->obj_progress[j].current, 0 );
		}
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_check_progress_npc_noop( void ) {
	CHAR_DATA *npc = make_test_npc();
	/* Should not crash on NPC */
	quest_check_progress( npc, QOBJ_USE_COMMAND, "look", 1 );
	TEST_ASSERT( 1 );
	free_test_char( npc );
}

static void test_check_progress_no_tracker_noop( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->pcdata->quest_tracker = NULL;
	/* Should not crash */
	quest_check_progress( ch, QOBJ_USE_COMMAND, "look", 1 );
	TEST_ASSERT( 1 );
	free_test_char( ch );
}

static void test_progress_capped_at_threshold( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;

	int qi = quest_def_index_by_id( "T01" );
	if ( qi >= 0 ) {
		const QUEST_DEF *q = quest_def_by_index( qi );
		QUEST_PROGRESS *p = quest_tracker_get( t, qi );
		p->status = QSTATUS_ACTIVE;

		/* Find first USE_COMMAND objective */
		int j;
		for ( j = 0; j < q->obj_count; j++ ) {
			if ( !strcmp( q->objectives[j].type, QOBJ_USE_COMMAND ) ) {
				/* Set near threshold then exceed it */
				p->obj_progress[j].current = q->objectives[j].threshold - 1;
				quest_check_progress( ch, QOBJ_USE_COMMAND,
					q->objectives[j].target, 100 );

				/* Should be capped at threshold */
				TEST_ASSERT_EQ( p->obj_progress[j].current,
					q->objectives[j].threshold );
				break;
			}
		}
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_target_matching_any( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;

	/* Find a quest with "any" target, like T05 KILL_MOB any */
	int qi = quest_def_index_by_id( "T05" );
	if ( qi >= 0 ) {
		const QUEST_DEF *q = quest_def_by_index( qi );
		QUEST_PROGRESS *p = quest_tracker_get( t, qi );
		p->status = QSTATUS_ACTIVE;

		int j;
		for ( j = 0; j < q->obj_count; j++ ) {
			if ( !strcmp( q->objectives[j].type, QOBJ_KILL_MOB )
					&& !strcmp( q->objectives[j].target, "any" ) ) {
				/* "any" target should match any mob name */
				quest_check_progress( ch, QOBJ_KILL_MOB, "random_mob_12345", 1 );
				TEST_ASSERT_EQ( p->obj_progress[j].current, 1 );
				break;
			}
		}
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_target_matching_specific( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;

	/* T01 has USE_COMMAND look 1 */
	int qi = quest_def_index_by_id( "T01" );
	if ( qi >= 0 ) {
		const QUEST_DEF *q = quest_def_by_index( qi );
		QUEST_PROGRESS *p = quest_tracker_get( t, qi );
		p->status = QSTATUS_ACTIVE;

		/* "kill" should NOT match "look" objective */
		quest_check_progress( ch, QOBJ_USE_COMMAND, "kill", 1 );

		int j;
		for ( j = 0; j < q->obj_count; j++ ) {
			if ( !strcmp( q->objectives[j].type, QOBJ_USE_COMMAND )
					&& !strcmp( q->objectives[j].target, "look" ) ) {
				TEST_ASSERT_EQ( p->obj_progress[j].current, 0 );
				break;
			}
		}
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

/*--------------------------------------------------------------------------
 * Tier 2: Milestone Evaluation
 *--------------------------------------------------------------------------*/

static void test_milestone_reach_stat_hp( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;

	/* M01 has REACH_STAT hp 2000 */
	int qi = quest_def_index_by_id( "M01" );
	if ( qi >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_get( t, qi );
		p->status = QSTATUS_ACTIVE;

		/* HP below threshold — progress shows actual value */
		ch->max_hit = 1500;
		quest_check_milestones( ch );
		TEST_ASSERT_EQ( p->obj_progress[0].current, 1500 );

		/* HP at threshold */
		ch->max_hit = 2000;
		quest_check_milestones( ch );
		TEST_ASSERT_EQ( p->obj_progress[0].current, 2000 );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_milestone_reach_stat_above_threshold( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;

	/* M01 has REACH_STAT hp 2000 */
	int qi = quest_def_index_by_id( "M01" );
	if ( qi >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_get( t, qi );
		p->status = QSTATUS_ACTIVE;

		/* HP above threshold — stores actual value */
		ch->max_hit = 50000;
		quest_check_milestones( ch );
		TEST_ASSERT_EQ( p->obj_progress[0].current, 50000 );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_milestone_npc_noop( void ) {
	CHAR_DATA *npc = make_test_npc();
	quest_check_milestones( npc );
	TEST_ASSERT( 1 );
	free_test_char( npc );
}

static void test_milestone_no_tracker_noop( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->pcdata->quest_tracker = NULL;
	quest_check_milestones( ch );
	TEST_ASSERT( 1 );
	free_test_char( ch );
}

static void test_milestone_skips_already_met( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;

	int qi = quest_def_index_by_id( "M01" );
	if ( qi >= 0 ) {
		const QUEST_DEF *q = quest_def_by_index( qi );
		QUEST_PROGRESS *p = quest_tracker_get( t, qi );
		p->status = QSTATUS_ACTIVE;

		/* Pre-set objective as already met */
		p->obj_progress[0].current = q->objectives[0].threshold;
		int saved = p->obj_progress[0].current;

		ch->max_hit = 999999;
		quest_check_milestones( ch );

		/* Should remain unchanged */
		TEST_ASSERT_EQ( p->obj_progress[0].current, saved );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

/*--------------------------------------------------------------------------
 * Tier 2: Quest Availability & Prerequisites
 *--------------------------------------------------------------------------*/

static void test_evaluate_availability_unlocks( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 0;

	/* T01 has no prereqs - should be unlocked for explevel 0 */
	quest_evaluate_availability( ch );

	int qi = quest_def_index_by_id( "T01" );
	if ( qi >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi );
		TEST_ASSERT( p != NULL );
		/* T01 is auto-complete, so should go straight to ACTIVE */
		const QUEST_DEF *q = quest_def_by_index( qi );
		if ( q && IS_SET( q->flags, QFLAG_AUTO_COMPLETE ) ) {
			TEST_ASSERT_EQ( p->status, QSTATUS_ACTIVE );
		} else {
			TEST_ASSERT_EQ( p->status, QSTATUS_AVAILABLE );
		}
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_evaluate_availability_npc_noop( void ) {
	CHAR_DATA *npc = make_test_npc();
	/* Should not crash */
	quest_evaluate_availability( npc );
	TEST_ASSERT( 1 );
	free_test_char( npc );
}

static void test_ftue_visibility_explevel0( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 0;

	quest_evaluate_availability( ch );

	/* T01 (min=0, max=0) should be visible */
	int qi_t01 = quest_def_index_by_id( "T01" );
	if ( qi_t01 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t01 );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT( p->status > QSTATUS_LOCKED );
	}

	/* T06 (min=1, max=1) should NOT be visible to explevel 0 */
	int qi_t06 = quest_def_index_by_id( "T06" );
	if ( qi_t06 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t06 );
		/* Should either not exist or be locked */
		TEST_ASSERT( p == NULL || p->status == QSTATUS_LOCKED );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_ftue_visibility_explevel1( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 1;

	quest_evaluate_availability( ch );

	/* T06 (min=1, max=1) should be visible */
	int qi_t06 = quest_def_index_by_id( "T06" );
	if ( qi_t06 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t06 );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT( p->status > QSTATUS_LOCKED );
	}

	/* T01 (min=0, max=0) should NOT be visible to explevel 1 */
	int qi_t01 = quest_def_index_by_id( "T01" );
	if ( qi_t01 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t01 );
		TEST_ASSERT( p == NULL || p->status == QSTATUS_LOCKED );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_ftue_visibility_veteran( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 2;

	quest_evaluate_availability( ch );

	/* T01 and T06 should NOT be visible to veterans */
	int qi_t01 = quest_def_index_by_id( "T01" );
	if ( qi_t01 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t01 );
		TEST_ASSERT( p == NULL || p->status == QSTATUS_LOCKED );
	}

	int qi_t06 = quest_def_index_by_id( "T06" );
	if ( qi_t06 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t06 );
		TEST_ASSERT( p == NULL || p->status == QSTATUS_LOCKED );
	}

	/* M01 should be visible (min=0, max=3) */
	int qi_m01 = quest_def_index_by_id( "M01" );
	if ( qi_m01 >= 0 ) {
		const QUEST_DEF *q = quest_def_by_index( qi_m01 );
		/* For veterans, M01 prereqs are tutorial quests which are FTUE-filtered
		 * so they should be considered met */
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_m01 );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT( p->status > QSTATUS_LOCKED );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_prereqs_block_locked_quest( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 0;

	quest_evaluate_availability( ch );

	/* T02 requires T01 - should still be locked if T01 not turned in */
	int qi_t02 = quest_def_index_by_id( "T02" );
	if ( qi_t02 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t02 );
		TEST_ASSERT( p == NULL || p->status == QSTATUS_LOCKED );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_prereqs_unlock_after_completion( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 0;

	/* Complete T01 */
	int qi_t01 = quest_def_index_by_id( "T01" );
	if ( qi_t01 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_get( t, qi_t01 );
		p->status = QSTATUS_TURNED_IN;
	}

	/* Re-evaluate - T02 should now unlock */
	quest_evaluate_availability( ch );

	int qi_t02 = quest_def_index_by_id( "T02" );
	if ( qi_t02 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t02 );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT( p->status > QSTATUS_LOCKED );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

/*--------------------------------------------------------------------------
 * Tier 2: Auto-Grant First Quest
 *--------------------------------------------------------------------------*/

static void test_init_player_auto_grant_explevel0( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 0;

	quest_init_player( ch );

	int qi = quest_def_index_by_id( "T01" );
	if ( qi >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT_EQ( p->status, QSTATUS_ACTIVE );
		TEST_ASSERT( p->started_at > 0 );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_init_player_auto_grant_explevel1( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 1;

	quest_init_player( ch );

	int qi = quest_def_index_by_id( "T06" );
	if ( qi >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT_EQ( p->status, QSTATUS_ACTIVE );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_init_player_auto_grant_veteran( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 2;

	quest_init_player( ch );

	int qi = quest_def_index_by_id( "M01" );
	if ( qi >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT_EQ( p->status, QSTATUS_ACTIVE );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_init_player_no_double_grant( void ) {
	CHAR_DATA *ch = make_test_player();
	QUEST_TRACKER *t = quest_tracker_new();
	ch->pcdata->quest_tracker = t;
	ch->explevel = 0;

	/* Pre-existing progress (not a new player) */
	int qi_t01 = quest_def_index_by_id( "T01" );
	if ( qi_t01 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_get( t, qi_t01 );
		p->status = QSTATUS_TURNED_IN;
	}

	quest_init_player( ch );

	/* T01 should remain TURNED_IN, not reset to ACTIVE */
	if ( qi_t01 >= 0 ) {
		QUEST_PROGRESS *p = quest_tracker_find( t, qi_t01 );
		TEST_ASSERT( p != NULL );
		TEST_ASSERT_EQ( p->status, QSTATUS_TURNED_IN );
	}

	quest_tracker_free( t );
	ch->pcdata->quest_tracker = NULL;
	free_test_char( ch );
}

static void test_init_player_npc_noop( void ) {
	CHAR_DATA *npc = make_test_npc();
	quest_init_player( npc );
	TEST_ASSERT( 1 );
	free_test_char( npc );
}

static void test_init_player_no_tracker_noop( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->pcdata->quest_tracker = NULL;
	quest_init_player( ch );
	TEST_ASSERT( 1 );
	free_test_char( ch );
}

/*--------------------------------------------------------------------------
 * Tier 2: Status Constants
 *--------------------------------------------------------------------------*/

static void test_status_constants_ordered( void ) {
	TEST_ASSERT( QSTATUS_LOCKED < QSTATUS_AVAILABLE );
	TEST_ASSERT( QSTATUS_AVAILABLE < QSTATUS_ACTIVE );
	TEST_ASSERT( QSTATUS_ACTIVE < QSTATUS_COMPLETE );
	TEST_ASSERT( QSTATUS_COMPLETE < QSTATUS_TURNED_IN );
}

static void test_status_constant_values( void ) {
	TEST_ASSERT_EQ( QSTATUS_LOCKED, 0 );
	TEST_ASSERT_EQ( QSTATUS_AVAILABLE, 1 );
	TEST_ASSERT_EQ( QSTATUS_ACTIVE, 2 );
	TEST_ASSERT_EQ( QSTATUS_COMPLETE, 3 );
	TEST_ASSERT_EQ( QSTATUS_TURNED_IN, 4 );
}

static void test_flag_constants( void ) {
	/* Flags should be distinct powers of 2 */
	TEST_ASSERT_EQ( QFLAG_AUTO_COMPLETE, 1 );
	TEST_ASSERT_EQ( QFLAG_REPEATABLE, 2 );
	TEST_ASSERT_EQ( QFLAG_FTUE_SKIP, 4 );

	/* No overlap */
	TEST_ASSERT_EQ( QFLAG_AUTO_COMPLETE & QFLAG_REPEATABLE, 0 );
	TEST_ASSERT_EQ( QFLAG_AUTO_COMPLETE & QFLAG_FTUE_SKIP, 0 );
	TEST_ASSERT_EQ( QFLAG_REPEATABLE & QFLAG_FTUE_SKIP, 0 );
}

/*--------------------------------------------------------------------------
 * Suite entry point
 *--------------------------------------------------------------------------*/

void suite_quest( void ) {
	/* Tier 1: No boot required - tracker operations */
	RUN_TEST( test_tracker_new_free );
	RUN_TEST( test_tracker_free_null );
	RUN_TEST( test_tracker_get_creates_entry );
	RUN_TEST( test_tracker_get_returns_existing );
	RUN_TEST( test_tracker_find_missing );
	RUN_TEST( test_tracker_find_null );
	RUN_TEST( test_tracker_get_null );
	RUN_TEST( test_tracker_multiple_entries );
	RUN_TEST( test_tracker_grows_capacity );
	RUN_TEST( test_progress_initial_state );
	RUN_TEST( test_status_constants_ordered );
	RUN_TEST( test_status_constant_values );
	RUN_TEST( test_flag_constants );

	/* Tier 2: Requires boot for quest.db */
	if ( !ensure_booted() ) return;

	RUN_TEST( test_quest_defs_loaded );
	RUN_TEST( test_quest_def_by_index_valid );
	RUN_TEST( test_quest_def_by_index_invalid );
	RUN_TEST( test_quest_def_by_id_found );
	RUN_TEST( test_quest_def_by_id_not_found );
	RUN_TEST( test_quest_def_index_by_id );
	RUN_TEST( test_quest_def_has_objectives );
	RUN_TEST( test_quest_tutorial_chain );
	RUN_TEST( test_quest_m01_objectives );

	RUN_TEST( test_check_progress_use_command );
	RUN_TEST( test_check_progress_ignores_inactive );
	RUN_TEST( test_check_progress_npc_noop );
	RUN_TEST( test_check_progress_no_tracker_noop );
	RUN_TEST( test_progress_capped_at_threshold );
	RUN_TEST( test_target_matching_any );
	RUN_TEST( test_target_matching_specific );

	RUN_TEST( test_milestone_reach_stat_hp );
	RUN_TEST( test_milestone_reach_stat_above_threshold );
	RUN_TEST( test_milestone_npc_noop );
	RUN_TEST( test_milestone_no_tracker_noop );
	RUN_TEST( test_milestone_skips_already_met );

	RUN_TEST( test_evaluate_availability_unlocks );
	RUN_TEST( test_evaluate_availability_npc_noop );
	RUN_TEST( test_ftue_visibility_explevel0 );
	RUN_TEST( test_ftue_visibility_explevel1 );
	RUN_TEST( test_ftue_visibility_veteran );
	RUN_TEST( test_prereqs_block_locked_quest );
	RUN_TEST( test_prereqs_unlock_after_completion );

	RUN_TEST( test_init_player_auto_grant_explevel0 );
	RUN_TEST( test_init_player_auto_grant_explevel1 );
	RUN_TEST( test_init_player_auto_grant_veteran );
	RUN_TEST( test_init_player_no_double_grant );
	RUN_TEST( test_init_player_npc_noop );
	RUN_TEST( test_init_player_no_tracker_noop );
}
