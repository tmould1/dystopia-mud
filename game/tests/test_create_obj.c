/*
 * Object creation tests for Dystopia MUD
 *
 * Tests create_object() from db.c using loaded obj_index_hash.
 * Requires boot_headless().
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"

extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];

/* Find any valid obj index from the hash table */
static OBJ_INDEX_DATA *get_any_obj_index( void ) {
	int i;
	for ( i = 0; i < MAX_KEY_HASH; i++ ) {
		if ( obj_index_hash[i] != NULL )
			return obj_index_hash[i];
	}
	return NULL;
}

void test_create_obj_returns_nonnull( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_TRUE( obj != NULL );
	extract_obj( obj );
}

void test_create_obj_copies_name( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_STR_EQ( obj->name, pObjIndex->name );
	extract_obj( obj );
}

void test_create_obj_copies_short_descr( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_STR_EQ( obj->short_descr, pObjIndex->short_descr );
	extract_obj( obj );
}

void test_create_obj_copies_item_type( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_EQ( obj->item_type, pObjIndex->item_type );
	extract_obj( obj );
}

void test_create_obj_copies_extra_flags( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_EQ( (int) obj->extra_flags, (int) pObjIndex->extra_flags );
	extract_obj( obj );
}

void test_create_obj_copies_wear_flags( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_EQ( (int) obj->wear_flags, (int) pObjIndex->wear_flags );
	extract_obj( obj );
}

void test_create_obj_sets_level( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 7 );
	TEST_ASSERT_EQ( obj->level, 7 );
	extract_obj( obj );
}

void test_create_obj_wear_loc_none( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_EQ( obj->wear_loc, WEAR_NONE );
	extract_obj( obj );
}

void test_create_obj_in_global_list( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_TRUE( list_node_is_linked( &obj->obj_node ) );
	extract_obj( obj );
}

void test_create_obj_lists_initialized( void ) {
	ensure_booted();
	OBJ_INDEX_DATA *pObjIndex = get_any_obj_index();
	TEST_ASSERT_TRUE( pObjIndex != NULL );

	OBJ_DATA *obj = create_object( pObjIndex, 5 );
	TEST_ASSERT_TRUE( list_empty( &obj->contents ) );
	TEST_ASSERT_TRUE( list_empty( &obj->affects ) );
	extract_obj( obj );
}

/* --- Suite registration --- */

void suite_create_obj( void ) {
	RUN_TEST( test_create_obj_returns_nonnull );
	RUN_TEST( test_create_obj_copies_name );
	RUN_TEST( test_create_obj_copies_short_descr );
	RUN_TEST( test_create_obj_copies_item_type );
	RUN_TEST( test_create_obj_copies_extra_flags );
	RUN_TEST( test_create_obj_copies_wear_flags );
	RUN_TEST( test_create_obj_sets_level );
	RUN_TEST( test_create_obj_wear_loc_none );
	RUN_TEST( test_create_obj_in_global_list );
	RUN_TEST( test_create_obj_lists_initialized );
}
