/*
 * Combat engagement tests for Dystopia MUD
 *
 * Tests set_fighting, stop_fighting, update_pos from fight.c.
 * Uses NPC-only tests to avoid act() descriptor issues.
 * Requires boot_headless() for g_characters list.
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"

extern list_head_t g_characters;

/* --- set_fighting tests --- */

void test_set_fighting_basic( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	CHAR_DATA *victim = make_full_test_npc();
	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );

	char_to_room( ch, room );
	char_to_room( victim, room );

	set_fighting( ch, victim );
	TEST_ASSERT_TRUE( ch->fighting == victim );
	TEST_ASSERT_EQ( ch->position, POS_FIGHTING );

	/* Clean up */
	ch->fighting = NULL;
	ch->position = POS_STANDING;
	char_from_room( ch );
	char_from_room( victim );
	free_char( ch );
	free_char( victim );
}

void test_set_fighting_already_noop( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	CHAR_DATA *v1 = make_full_test_npc();
	CHAR_DATA *v2 = make_full_test_npc();
	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );

	char_to_room( ch, room );
	char_to_room( v1, room );
	char_to_room( v2, room );

	set_fighting( ch, v1 );
	set_fighting( ch, v2 ); /* Should be ignored */
	TEST_ASSERT_TRUE( ch->fighting == v1 ); /* Still fighting v1 */

	ch->fighting = NULL;
	ch->position = POS_STANDING;
	char_from_room( ch );
	char_from_room( v1 );
	char_from_room( v2 );
	free_char( ch );
	free_char( v1 );
	free_char( v2 );
}

void test_set_fighting_damcap_change( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	CHAR_DATA *victim = make_full_test_npc();
	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );

	char_to_room( ch, room );
	char_to_room( victim, room );

	set_fighting( ch, victim );
	TEST_ASSERT_EQ( ch->damcap[DAM_CHANGE], 1 );

	ch->fighting = NULL;
	ch->position = POS_STANDING;
	char_from_room( ch );
	char_from_room( victim );
	free_char( ch );
	free_char( victim );
}

/* --- stop_fighting tests --- */

void test_stop_fighting_self( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	CHAR_DATA *victim = make_full_test_npc();
	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );

	char_to_room( ch, room );
	char_to_room( victim, room );
	list_push_back( &g_characters, &ch->char_node );
	list_push_back( &g_characters, &victim->char_node );

	set_fighting( ch, victim );
	stop_fighting( ch, FALSE );
	TEST_ASSERT_TRUE( ch->fighting == NULL );
	TEST_ASSERT_EQ( ch->position, POS_STANDING );

	list_remove( &g_characters, &ch->char_node );
	list_remove( &g_characters, &victim->char_node );
	char_from_room( ch );
	char_from_room( victim );
	free_char( ch );
	free_char( victim );
}

void test_stop_fighting_both( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	CHAR_DATA *victim = make_full_test_npc();
	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );

	char_to_room( ch, room );
	char_to_room( victim, room );
	list_push_back( &g_characters, &ch->char_node );
	list_push_back( &g_characters, &victim->char_node );

	set_fighting( ch, victim );
	set_fighting( victim, ch );
	stop_fighting( ch, TRUE );

	TEST_ASSERT_TRUE( ch->fighting == NULL );
	TEST_ASSERT_TRUE( victim->fighting == NULL );

	list_remove( &g_characters, &ch->char_node );
	list_remove( &g_characters, &victim->char_node );
	char_from_room( ch );
	char_from_room( victim );
	free_char( ch );
	free_char( victim );
}

void test_stop_fighting_all_opponents( void ) {
	ensure_booted();
	CHAR_DATA *a = make_full_test_npc();
	CHAR_DATA *b = make_full_test_npc();
	CHAR_DATA *c = make_full_test_npc();
	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );

	char_to_room( a, room );
	char_to_room( b, room );
	char_to_room( c, room );
	list_push_back( &g_characters, &a->char_node );
	list_push_back( &g_characters, &b->char_node );
	list_push_back( &g_characters, &c->char_node );

	set_fighting( a, b );
	set_fighting( c, b );
	stop_fighting( b, TRUE ); /* Should stop a and c too (they fight b) */

	TEST_ASSERT_TRUE( a->fighting == NULL );
	TEST_ASSERT_TRUE( b->fighting == NULL );
	TEST_ASSERT_TRUE( c->fighting == NULL );

	list_remove( &g_characters, &a->char_node );
	list_remove( &g_characters, &b->char_node );
	list_remove( &g_characters, &c->char_node );
	char_from_room( a );
	char_from_room( b );
	char_from_room( c );
	free_char( a );
	free_char( b );
	free_char( c );
}

/* --- update_pos tests --- */

void test_update_pos_standing( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	ch->hit = 100;
	ch->position = POS_STUNNED;

	update_pos( ch );
	TEST_ASSERT_EQ( ch->position, POS_STANDING );
	free_char( ch );
}

void test_update_pos_stunned( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	ch->hit = -1;
	ch->mount = NULL;

	update_pos( ch );
	TEST_ASSERT_EQ( ch->position, POS_STUNNED );
	free_char( ch );
}

void test_update_pos_incap( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	ch->hit = -4;
	ch->mount = NULL;

	update_pos( ch );
	TEST_ASSERT_EQ( ch->position, POS_INCAP );
	free_char( ch );
}

void test_update_pos_mortal( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	ch->hit = -6;
	ch->mount = NULL;

	update_pos( ch );
	/* NPCs with hit < -6 go to POS_DEAD, hit == -6 goes to POS_MORTAL */
	TEST_ASSERT_EQ( ch->position, POS_MORTAL );
	free_char( ch );
}

void test_update_pos_npc_dead( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	ch->hit = -7;
	ch->mount = NULL;

	update_pos( ch );
	TEST_ASSERT_EQ( ch->position, POS_DEAD );
	free_char( ch );
}

void test_update_pos_alive_no_change( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	ch->hit = 100;
	ch->position = POS_STANDING;

	update_pos( ch );
	TEST_ASSERT_EQ( ch->position, POS_STANDING );
	free_char( ch );
}

void test_update_pos_zero_hp( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	ch->hit = 0;
	ch->mount = NULL;

	update_pos( ch );
	TEST_ASSERT_EQ( ch->position, POS_STUNNED );
	free_char( ch );
}

/* --- Suite registration --- */

void suite_combat( void ) {
	RUN_TEST( test_set_fighting_basic );
	RUN_TEST( test_set_fighting_already_noop );
	RUN_TEST( test_set_fighting_damcap_change );
	RUN_TEST( test_stop_fighting_self );
	RUN_TEST( test_stop_fighting_both );
	RUN_TEST( test_stop_fighting_all_opponents );
	RUN_TEST( test_update_pos_standing );
	RUN_TEST( test_update_pos_stunned );
	RUN_TEST( test_update_pos_incap );
	RUN_TEST( test_update_pos_mortal );
	RUN_TEST( test_update_pos_npc_dead );
	RUN_TEST( test_update_pos_alive_no_change );
	RUN_TEST( test_update_pos_zero_hp );
}
