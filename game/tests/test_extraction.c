/*
 * Tests for the character extraction lifecycle.
 *
 * Verifies that list_detach on char_node + list_push_back on extracted_node
 * (the pattern used by extract_char) allows LIST_FOR_EACH_SAFE over
 * g_characters to survive mid-loop extraction of arbitrary characters.
 *
 * These tests operate on local lists to avoid needing a full game boot.
 */

#include "test_framework.h"
#include "test_helpers.h"

/*
 * Simulate extract_char's list operations on a character:
 *   1. list_detach from characters list (preserves char_node pointers)
 *   2. mark extracted
 *   3. list_push_back onto extracted list via extracted_node
 */
static void fake_extract( list_head_t *chars, list_head_t *extracted,
						  CHAR_DATA *ch ) {
	list_detach( chars, &ch->char_node );
	ch->extracted = true;
	list_push_back( extracted, &ch->extracted_node );
}

/*
 * After fake extraction, the character must be:
 *   - marked extracted
 *   - absent from the characters list
 *   - present in the extracted list via extracted_node
 *   - char_node pointers still pointing to old neighbors
 */
static void test_extract_uses_extracted_node( void ) {
	list_head_t chars, extracted;
	CHAR_DATA *a = make_test_npc();
	CHAR_DATA *b = make_test_npc();
	CHAR_DATA *c = make_test_npc();
	CHAR_DATA *pos;
	bool found;

	list_init( &chars );
	list_init( &extracted );

	list_push_back( &chars, &a->char_node );
	list_push_back( &chars, &b->char_node );
	list_push_back( &chars, &c->char_node );

	fake_extract( &chars, &extracted, b );

	/* b must be marked extracted */
	TEST_ASSERT_TRUE( b->extracted );

	/* b must NOT be in chars list */
	found = false;
	LIST_FOR_EACH( pos, &chars, CHAR_DATA, char_node ) {
		if ( pos == b ) { found = true; break; }
	}
	TEST_ASSERT_FALSE( found );
	TEST_ASSERT_EQ( list_count( &chars ), 2 );

	/* b must be in extracted list via extracted_node */
	found = false;
	LIST_FOR_EACH( pos, &extracted, CHAR_DATA, extracted_node ) {
		if ( pos == b ) { found = true; break; }
	}
	TEST_ASSERT_TRUE( found );
	TEST_ASSERT_EQ( list_count( &extracted ), 1 );

	/* b's char_node pointers still reference old neighbors */
	TEST_ASSERT( b->char_node.next == &c->char_node );
	TEST_ASSERT( b->char_node.prev == &a->char_node );

	/* Clean up */
	list_remove( &chars, &a->char_node );
	list_remove( &chars, &c->char_node );
	list_remove( &extracted, &b->extracted_node );
	free_test_char( a );
	free_test_char( b );
	free_test_char( c );
}

/*
 * Simulate the violence_update crash scenario:
 * During LIST_FOR_EACH_SAFE over a characters list, extract the saved
 * "next" element. With list_detach (not list_remove), iteration must
 * continue through all remaining characters correctly.
 */
static void test_safe_iter_survives_next_extraction( void ) {
	list_head_t chars, extracted;
	CHAR_DATA *npcs[5];
	CHAR_DATA *pos, *tmp;
	int visited = 0;
	int i;

	list_init( &chars );
	list_init( &extracted );

	for ( i = 0; i < 5; i++ ) {
		npcs[i] = make_test_npc();
		list_push_back( &chars, &npcs[i]->char_node );
	}

	/*
	 * Iterate. When we visit npcs[1], extract npcs[2] (the saved "next").
	 * With list_detach, npcs[2]->char_node.next still points to npcs[3],
	 * so iteration continues correctly.
	 */
	LIST_FOR_EACH_SAFE( pos, tmp, &chars, CHAR_DATA, char_node ) {
		if ( pos == npcs[1] )
			fake_extract( &chars, &extracted, npcs[2] );

		if ( !pos->extracted )
			visited++;
	}

	/* Should visit 4 non-extracted chars: npcs[0,1,3,4] */
	TEST_ASSERT_EQ( visited, 4 );
	TEST_ASSERT_EQ( list_count( &chars ), 4 );
	TEST_ASSERT_EQ( list_count( &extracted ), 1 );

	/* Clean up */
	for ( i = 0; i < 5; i++ ) {
		if ( npcs[i]->extracted )
			list_remove( &extracted, &npcs[i]->extracted_node );
		else
			list_remove( &chars, &npcs[i]->char_node );
		free_test_char( npcs[i] );
	}
}

/*
 * Extract multiple characters during iteration, including both the
 * current element and a future element. Verify iteration completes.
 */
static void test_safe_iter_survives_multi_extraction( void ) {
	list_head_t chars, extracted;
	CHAR_DATA *npcs[6];
	CHAR_DATA *pos, *tmp;
	int visited = 0;
	int i;

	list_init( &chars );
	list_init( &extracted );

	for ( i = 0; i < 6; i++ ) {
		npcs[i] = make_test_npc();
		list_push_back( &chars, &npcs[i]->char_node );
	}

	/*
	 * When visiting npcs[1], extract both npcs[1] (current) and
	 * npcs[2] (the saved next). Then when visiting npcs[3], extract
	 * npcs[4]. This tests multiple extractions in one pass.
	 */
	LIST_FOR_EACH_SAFE( pos, tmp, &chars, CHAR_DATA, char_node ) {
		if ( pos == npcs[1] ) {
			fake_extract( &chars, &extracted, npcs[1] );
			fake_extract( &chars, &extracted, npcs[2] );
		}
		if ( pos == npcs[3] )
			fake_extract( &chars, &extracted, npcs[4] );

		if ( !pos->extracted )
			visited++;
	}

	/* Should visit npcs[0,3,5] = 3 non-extracted chars */
	TEST_ASSERT_EQ( visited, 3 );
	TEST_ASSERT_EQ( list_count( &chars ), 3 );
	TEST_ASSERT_EQ( list_count( &extracted ), 3 );

	/* Clean up */
	for ( i = 0; i < 6; i++ ) {
		if ( npcs[i]->extracted )
			list_remove( &extracted, &npcs[i]->extracted_node );
		else
			list_remove( &chars, &npcs[i]->char_node );
		free_test_char( npcs[i] );
	}
}

/* --- Suite --- */

void suite_extraction( void ) {
	RUN_TEST( test_extract_uses_extracted_node );
	RUN_TEST( test_safe_iter_survives_next_extraction );
	RUN_TEST( test_safe_iter_survives_multi_extraction );
}
