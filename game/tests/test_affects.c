/*
 * Affect system tests for Dystopia MUD
 *
 * Tests affect_to_char, affect_remove, affect_strip, affect_join,
 * and is_affected from handler.c.
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"

/* --- NPC affect application --- */

void test_affect_to_npc_hitroll( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_HITROLL, 5, 0 );
	int orig = ch->hitroll;

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( ch->hitroll, orig + 5 );
	free_test_char( ch );
}

void test_affect_to_npc_damroll( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_DAMROLL, 3, 0 );
	int orig = ch->damroll;

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( ch->damroll, orig + 3 );
	free_test_char( ch );
}

void test_affect_to_npc_armor( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_AC, -20, 0 );
	int orig = ch->armor;

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( ch->armor, orig - 20 );
	free_test_char( ch );
}

void test_affect_to_npc_max_hit( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_HIT, 100, 0 );
	int orig = ch->max_hit;

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( ch->max_hit, orig + 100 );
	free_test_char( ch );
}

void test_affect_to_npc_max_mana( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_MANA, 50, 0 );
	int orig = ch->max_mana;

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( ch->max_mana, orig + 50 );
	free_test_char( ch );
}

void test_affect_to_npc_bitvector( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_NONE, 0, AFF_SANCTUARY );

	affect_to_char( ch, &af );
	TEST_ASSERT_TRUE( IS_SET( ch->affected_by, AFF_SANCTUARY ) );
	free_test_char( ch );
}

/* --- Player affect application --- */

void test_affect_to_player_str( void ) {
	CHAR_DATA *ch = make_test_player();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_STR, 3, 0 );
	int orig = ch->pcdata->mod_str;

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( ch->pcdata->mod_str, orig + 3 );
	free_test_char( ch );
}

void test_affect_to_player_hitroll( void ) {
	CHAR_DATA *ch = make_test_player();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_HITROLL, 7, 0 );
	int orig = ch->hitroll;

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( ch->hitroll, orig + 7 );
	free_test_char( ch );
}

/* --- Affect removal --- */

void test_affect_remove_reverses_npc( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_HITROLL, 5, 0 );
	int orig = ch->hitroll;

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( ch->hitroll, orig + 5 );

	/* Remove the affect (it was copied by affect_to_char) */
	AFFECT_DATA *paf;
	LIST_FOR_EACH( paf, &ch->affects, AFFECT_DATA, node ) {
		break; /* Get first affect */
	}
	TEST_ASSERT_TRUE( paf != NULL );
	affect_remove( ch, paf );
	TEST_ASSERT_EQ( ch->hitroll, orig );
	free_test_char( ch );
}

void test_affect_remove_clears_bitvector( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_NONE, 0, AFF_SANCTUARY );

	affect_to_char( ch, &af );
	TEST_ASSERT_TRUE( IS_SET( ch->affected_by, AFF_SANCTUARY ) );

	AFFECT_DATA *paf;
	LIST_FOR_EACH( paf, &ch->affects, AFFECT_DATA, node ) {
		break;
	}
	affect_remove( ch, paf );
	TEST_ASSERT_FALSE( IS_SET( ch->affected_by, AFF_SANCTUARY ) );
	free_test_char( ch );
}

void test_affect_remove_frees_affect( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 1, 10, APPLY_HITROLL, 5, 0 );

	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 1 );

	AFFECT_DATA *paf;
	LIST_FOR_EACH( paf, &ch->affects, AFFECT_DATA, node ) {
		break;
	}
	affect_remove( ch, paf );
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 0 );
	free_test_char( ch );
}

/* --- Affect stripping --- */

void test_affect_strip_all_of_type( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 42, 10, APPLY_HITROLL, 1, 0 );

	affect_to_char( ch, &af );
	affect_to_char( ch, &af );
	affect_to_char( ch, &af );
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 3 );

	affect_strip( ch, 42 );
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 0 );
	free_test_char( ch );
}

void test_affect_strip_preserves_others( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af42 = make_test_affect( 42, 10, APPLY_HITROLL, 1, 0 );
	AFFECT_DATA af99 = make_test_affect( 99, 10, APPLY_DAMROLL, 2, 0 );

	affect_to_char( ch, &af42 );
	affect_to_char( ch, &af99 );
	affect_to_char( ch, &af42 );
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 3 );

	affect_strip( ch, 42 );
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 1 );
	TEST_ASSERT_TRUE( is_affected( ch, 99 ) );
	free_test_char( ch );
}

void test_affect_strip_empty_safe( void ) {
	CHAR_DATA *ch = make_test_npc();
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 0 );

	/* Should not crash */
	affect_strip( ch, 42 );
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 0 );
	free_test_char( ch );
}

/* --- is_affected --- */

void test_is_affected_present( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 42, 10, APPLY_NONE, 0, 0 );

	affect_to_char( ch, &af );
	TEST_ASSERT_TRUE( is_affected( ch, 42 ) );
	free_test_char( ch );
}

void test_is_affected_absent( void ) {
	CHAR_DATA *ch = make_test_npc();
	TEST_ASSERT_FALSE( is_affected( ch, 99 ) );
	free_test_char( ch );
}

void test_is_affected_after_strip( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af = make_test_affect( 42, 10, APPLY_NONE, 0, 0 );

	affect_to_char( ch, &af );
	TEST_ASSERT_TRUE( is_affected( ch, 42 ) );

	affect_strip( ch, 42 );
	TEST_ASSERT_FALSE( is_affected( ch, 42 ) );
	free_test_char( ch );
}

/* --- affect_join --- */

void test_affect_join_merges( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af1 = make_test_affect( 42, 10, APPLY_HITROLL, 5, 0 );
	AFFECT_DATA af2 = make_test_affect( 42, 20, APPLY_HITROLL, 3, 0 );

	affect_to_char( ch, &af1 );
	affect_join( ch, &af2 );

	/* Should have exactly one affect with combined values */
	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 1 );

	AFFECT_DATA *paf;
	LIST_FOR_EACH( paf, &ch->affects, AFFECT_DATA, node ) {
		break;
	}
	TEST_ASSERT_TRUE( paf != NULL );
	TEST_ASSERT_EQ( paf->duration, 30 );
	TEST_ASSERT_EQ( paf->modifier, 8 );
	free_test_char( ch );
}

/* --- Multiple affect stacking --- */

void test_affect_multiple_stacking( void ) {
	CHAR_DATA *ch = make_test_npc();
	AFFECT_DATA af_hr = make_test_affect( 1, 10, APPLY_HITROLL, 5, 0 );
	AFFECT_DATA af_dr = make_test_affect( 2, 10, APPLY_DAMROLL, 3, 0 );
	AFFECT_DATA af_ac = make_test_affect( 3, 10, APPLY_AC, -10, 0 );
	int orig_hr = ch->hitroll;
	int orig_dr = ch->damroll;
	int orig_ac = ch->armor;

	affect_to_char( ch, &af_hr );
	affect_to_char( ch, &af_dr );
	affect_to_char( ch, &af_ac );

	TEST_ASSERT_EQ( (int) list_count( &ch->affects ), 3 );
	TEST_ASSERT_EQ( ch->hitroll, orig_hr + 5 );
	TEST_ASSERT_EQ( ch->damroll, orig_dr + 3 );
	TEST_ASSERT_EQ( ch->armor, orig_ac - 10 );
	free_test_char( ch );
}

/* --- Suite registration --- */

void suite_affects( void ) {
	RUN_TEST( test_affect_to_npc_hitroll );
	RUN_TEST( test_affect_to_npc_damroll );
	RUN_TEST( test_affect_to_npc_armor );
	RUN_TEST( test_affect_to_npc_max_hit );
	RUN_TEST( test_affect_to_npc_max_mana );
	RUN_TEST( test_affect_to_npc_bitvector );
	RUN_TEST( test_affect_to_player_str );
	RUN_TEST( test_affect_to_player_hitroll );
	RUN_TEST( test_affect_remove_reverses_npc );
	RUN_TEST( test_affect_remove_clears_bitvector );
	RUN_TEST( test_affect_remove_frees_affect );
	RUN_TEST( test_affect_strip_all_of_type );
	RUN_TEST( test_affect_strip_preserves_others );
	RUN_TEST( test_affect_strip_empty_safe );
	RUN_TEST( test_is_affected_present );
	RUN_TEST( test_is_affected_absent );
	RUN_TEST( test_is_affected_after_strip );
	RUN_TEST( test_affect_join_merges );
	RUN_TEST( test_affect_multiple_stacking );
}
