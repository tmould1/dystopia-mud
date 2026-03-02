/*
 * Character/Object movement tests for Dystopia MUD
 *
 * Tests char_to_room, char_from_room, obj_to_char, obj_from_char,
 * obj_to_room, obj_from_room from handler.c.
 * Requires boot_headless() for real rooms and objects.
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"

extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];

static ROOM_INDEX_DATA *get_test_room( void ) {
	return get_room_index( ROOM_VNUM_LIMBO );
}

static ROOM_INDEX_DATA *get_second_room( void ) {
	return get_room_index( ROOM_VNUM_ALTAR );
}

/* Find any valid obj index from the hash table */
static OBJ_INDEX_DATA *get_any_obj_index( void ) {
	int i;
	for ( i = 0; i < MAX_KEY_HASH; i++ ) {
		if ( obj_index_hash[i] != NULL )
			return obj_index_hash[i];
	}
	return NULL;
}

/* --- char_to_room tests --- */

void test_char_to_room_sets_in_room( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	CHAR_DATA *ch = make_full_test_npc();

	char_to_room( ch, room );
	TEST_ASSERT_TRUE( ch->in_room == room );
	char_from_room( ch );
	free_char( ch );
}

void test_char_to_room_links_node( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	CHAR_DATA *ch = make_full_test_npc();

	char_to_room( ch, room );
	TEST_ASSERT_TRUE( list_node_is_linked( &ch->room_node ) );
	char_from_room( ch );
	free_char( ch );
}

void test_char_to_room_in_character_list( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	CHAR_DATA *ch = make_full_test_npc();
	size_t before = list_count( &room->characters );

	char_to_room( ch, room );
	TEST_ASSERT_EQ( (int) list_count( &room->characters ), (int) ( before + 1 ) );
	char_from_room( ch );
	free_char( ch );
}

void test_char_to_room_null_goes_limbo( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();

	char_to_room( ch, NULL );
	TEST_ASSERT_TRUE( ch->in_room != NULL );
	TEST_ASSERT_EQ( ch->in_room->vnum, ROOM_VNUM_LIMBO );
	char_from_room( ch );
	free_char( ch );
}

/* --- char_from_room tests --- */

void test_char_from_room_clears_in_room( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	CHAR_DATA *ch = make_full_test_npc();

	char_to_room( ch, room );
	char_from_room( ch );
	TEST_ASSERT_TRUE( ch->in_room == NULL );
	free_char( ch );
}

void test_char_from_room_unlinks_node( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	CHAR_DATA *ch = make_full_test_npc();

	char_to_room( ch, room );
	char_from_room( ch );
	TEST_ASSERT_FALSE( list_node_is_linked( &ch->room_node ) );
	free_char( ch );
}

void test_char_from_room_decrements_list( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	CHAR_DATA *ch = make_full_test_npc();
	size_t before = list_count( &room->characters );

	char_to_room( ch, room );
	char_from_room( ch );
	TEST_ASSERT_EQ( (int) list_count( &room->characters ), (int) before );
	free_char( ch );
}

/* --- Room-to-room movement --- */

void test_char_move_between_rooms( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room1 = get_test_room();
	ROOM_INDEX_DATA *room2 = get_second_room();
	CHAR_DATA *ch = make_full_test_npc();

	if ( room2 == NULL ) {
		free_char( ch );
		return; /* Skip if second room not available */
	}

	char_to_room( ch, room1 );
	TEST_ASSERT_TRUE( ch->in_room == room1 );

	char_from_room( ch );
	char_to_room( ch, room2 );
	TEST_ASSERT_TRUE( ch->in_room == room2 );

	char_from_room( ch );
	free_char( ch );
}

/* --- obj_to_char / obj_from_char tests --- */

void test_obj_to_char_sets_carried_by( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	ROOM_INDEX_DATA *room = get_test_room();
	char_to_room( ch, room );

	obj_to_char( obj, ch );
	TEST_ASSERT_TRUE( obj->carried_by == ch );

	obj_from_char( obj );
	extract_obj( obj );
	char_from_room( ch );
	free_char( ch );
}

void test_obj_to_char_increments_carry_number( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	int before = ch->carry_number;

	obj_to_char( obj, ch );
	TEST_ASSERT_EQ( ch->carry_number, before + 1 );

	obj_from_char( obj );
	extract_obj( obj );
	free_char( ch );
}

void test_obj_to_char_updates_carry_weight( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	int before = ch->carry_weight;
	int obj_weight = get_obj_weight( obj );

	obj_to_char( obj, ch );
	TEST_ASSERT_EQ( ch->carry_weight, before + obj_weight );

	obj_from_char( obj );
	extract_obj( obj );
	free_char( ch );
}

void test_obj_from_char_clears_carried_by( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	obj_to_char( obj, ch );
	obj_from_char( obj );
	TEST_ASSERT_TRUE( obj->carried_by == NULL );

	extract_obj( obj );
	free_char( ch );
}

void test_obj_from_char_decrements_carry( void ) {
	ensure_booted();
	CHAR_DATA *ch = make_full_test_npc();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	int before = ch->carry_number;

	obj_to_char( obj, ch );
	obj_from_char( obj );
	TEST_ASSERT_EQ( ch->carry_number, before );

	extract_obj( obj );
	free_char( ch );
}

/* --- obj_to_room / obj_from_room tests --- */

void test_obj_to_room_sets_in_room( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	obj_to_room( obj, room );
	TEST_ASSERT_TRUE( obj->in_room == room );

	obj_from_room( obj );
	extract_obj( obj );
}

void test_obj_to_room_links_node( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	obj_to_room( obj, room );
	TEST_ASSERT_TRUE( list_node_is_linked( &obj->room_node ) );

	obj_from_room( obj );
	extract_obj( obj );
}

void test_obj_from_room_clears_in_room( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	obj_to_room( obj, room );
	obj_from_room( obj );
	TEST_ASSERT_TRUE( obj->in_room == NULL );

	extract_obj( obj );
}

void test_obj_from_room_unlinks_node( void ) {
	ensure_booted();
	ROOM_INDEX_DATA *room = get_test_room();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 0 );
	obj_to_room( obj, room );
	obj_from_room( obj );
	TEST_ASSERT_FALSE( list_node_is_linked( &obj->room_node ) );

	extract_obj( obj );
}

/* --- Suite registration --- */

void suite_handler( void ) {
	RUN_TEST( test_char_to_room_sets_in_room );
	RUN_TEST( test_char_to_room_links_node );
	RUN_TEST( test_char_to_room_in_character_list );
	RUN_TEST( test_char_to_room_null_goes_limbo );
	RUN_TEST( test_char_from_room_clears_in_room );
	RUN_TEST( test_char_from_room_unlinks_node );
	RUN_TEST( test_char_from_room_decrements_list );
	RUN_TEST( test_char_move_between_rooms );
	RUN_TEST( test_obj_to_char_sets_carried_by );
	RUN_TEST( test_obj_to_char_increments_carry_number );
	RUN_TEST( test_obj_to_char_updates_carry_weight );
	RUN_TEST( test_obj_from_char_clears_carried_by );
	RUN_TEST( test_obj_from_char_decrements_carry );
	RUN_TEST( test_obj_to_room_sets_in_room );
	RUN_TEST( test_obj_to_room_links_node );
	RUN_TEST( test_obj_from_room_clears_in_room );
	RUN_TEST( test_obj_from_room_unlinks_node );
}
