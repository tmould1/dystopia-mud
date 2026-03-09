/*
 * Unit tests for list.h — intrusive doubly-linked list library
 */

#include "test_framework.h"
#include "../src/core/list.h"

/* Test struct with an embedded list node */
typedef struct test_item {
	int value;
	list_node_t node;
} test_item_t;

/* Helper: get item from node */
#define ITEM( ptr ) LIST_ENTRY( ptr, test_item_t, node )

/* --- Tests --- */

static void test_list_init_empty( void ) {
	list_head_t list;
	list_init( &list );
	TEST_ASSERT_TRUE( list_empty( &list ) );
	TEST_ASSERT_EQ( list_count( &list ), 0 );
	TEST_ASSERT( list_first( &list ) == NULL );
	TEST_ASSERT( list_last( &list ) == NULL );
}

static void test_list_push_front_single( void ) {
	list_head_t list;
	test_item_t a = { .value = 42 };
	list_init( &list );
	list_node_init( &a.node );

	list_push_front( &list, &a.node );

	TEST_ASSERT_FALSE( list_empty( &list ) );
	TEST_ASSERT_EQ( list_count( &list ), 1 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 42 );
	TEST_ASSERT_EQ( ITEM( list_last( &list ) )->value, 42 );
}

static void test_list_push_back_single( void ) {
	list_head_t list;
	test_item_t a = { .value = 7 };
	list_init( &list );
	list_node_init( &a.node );

	list_push_back( &list, &a.node );

	TEST_ASSERT_EQ( list_count( &list ), 1 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 7 );
}

static void test_list_push_front_order( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	test_item_t c = { .value = 3 };
	list_init( &list );

	/* Push front: c, b, a → list order should be 3, 2, 1 */
	list_push_front( &list, &a.node );
	list_push_front( &list, &b.node );
	list_push_front( &list, &c.node );

	TEST_ASSERT_EQ( list_count( &list ), 3 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 3 );
	TEST_ASSERT_EQ( ITEM( list_last( &list ) )->value, 1 );
}

static void test_list_push_back_order( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	test_item_t c = { .value = 3 };
	list_init( &list );

	/* Push back: a, b, c → list order should be 1, 2, 3 */
	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );
	list_push_back( &list, &c.node );

	TEST_ASSERT_EQ( list_count( &list ), 3 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 1 );
	TEST_ASSERT_EQ( ITEM( list_last( &list ) )->value, 3 );
}

static void test_list_remove_middle( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	test_item_t c = { .value = 3 };
	list_init( &list );

	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );
	list_push_back( &list, &c.node );

	list_remove( &list, &b.node );

	TEST_ASSERT_EQ( list_count( &list ), 2 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 1 );
	TEST_ASSERT_EQ( ITEM( list_last( &list ) )->value, 3 );
	/* Verify b is unlinked */
	TEST_ASSERT_FALSE( list_node_is_linked( &b.node ) );
}

static void test_list_remove_first( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	list_init( &list );

	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );

	list_remove( &list, &a.node );

	TEST_ASSERT_EQ( list_count( &list ), 1 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 2 );
}

static void test_list_remove_last( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	list_init( &list );

	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );

	list_remove( &list, &b.node );

	TEST_ASSERT_EQ( list_count( &list ), 1 );
	TEST_ASSERT_EQ( ITEM( list_last( &list ) )->value, 1 );
}

static void test_list_remove_only( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	list_init( &list );

	list_push_back( &list, &a.node );
	list_remove( &list, &a.node );

	TEST_ASSERT_TRUE( list_empty( &list ) );
	TEST_ASSERT_EQ( list_count( &list ), 0 );
}

static void test_list_for_each( void ) {
	list_head_t list;
	test_item_t items[5];
	int i, sum = 0;
	list_init( &list );

	for ( i = 0; i < 5; i++ ) {
		items[i].value = i + 1;
		list_push_back( &list, &items[i].node );
	}

	{
		test_item_t *pos;
		LIST_FOR_EACH( pos, &list, test_item_t, node ) {
			sum += pos->value;
		}
	}

	/* 1+2+3+4+5 = 15 */
	TEST_ASSERT_EQ( sum, 15 );
}

static void test_list_for_each_reverse( void ) {
	list_head_t list;
	test_item_t items[3];
	int values[3], idx = 0;
	list_init( &list );

	items[0].value = 10;
	items[1].value = 20;
	items[2].value = 30;
	list_push_back( &list, &items[0].node );
	list_push_back( &list, &items[1].node );
	list_push_back( &list, &items[2].node );

	{
		test_item_t *pos;
		LIST_FOR_EACH_REVERSE( pos, &list, test_item_t, node ) {
			values[idx++] = pos->value;
		}
	}

	TEST_ASSERT_EQ( values[0], 30 );
	TEST_ASSERT_EQ( values[1], 20 );
	TEST_ASSERT_EQ( values[2], 10 );
}

static void test_list_for_each_safe_remove( void ) {
	list_head_t list;
	test_item_t items[5];
	int i;
	list_init( &list );

	for ( i = 0; i < 5; i++ ) {
		items[i].value = i + 1;
		list_push_back( &list, &items[i].node );
	}

	/* Remove even-valued items during iteration */
	{
		test_item_t *pos, *tmp;
		LIST_FOR_EACH_SAFE( pos, tmp, &list, test_item_t, node ) {
			if ( pos->value % 2 == 0 )
				list_remove( &list, &pos->node );
		}
	}

	/* Should have 1, 3, 5 remaining */
	TEST_ASSERT_EQ( list_count( &list ), 3 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 1 );
	TEST_ASSERT_EQ( ITEM( list_last( &list ) )->value, 5 );
}

static void test_list_for_each_safe_remove_all( void ) {
	list_head_t list;
	test_item_t items[3];
	int i;
	list_init( &list );

	for ( i = 0; i < 3; i++ ) {
		items[i].value = i;
		list_push_back( &list, &items[i].node );
	}

	/* Remove everything during iteration */
	{
		test_item_t *pos, *tmp;
		LIST_FOR_EACH_SAFE( pos, tmp, &list, test_item_t, node ) {
			list_remove( &list, &pos->node );
		}
	}

	TEST_ASSERT_TRUE( list_empty( &list ) );
	TEST_ASSERT_EQ( list_count( &list ), 0 );
}

static void test_list_for_each_empty( void ) {
	list_head_t list;
	int count = 0;
	list_init( &list );

	{
		test_item_t *pos;
		LIST_FOR_EACH( pos, &list, test_item_t, node ) {
			count++;
		}
	}

	TEST_ASSERT_EQ( count, 0 );
}

static void test_list_node_init_unlinked( void ) {
	list_node_t node;
	list_node_init( &node );
	TEST_ASSERT_FALSE( list_node_is_linked( &node ) );
}

static void test_list_node_linked_after_push( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	list_init( &list );
	list_node_init( &a.node );

	TEST_ASSERT_FALSE( list_node_is_linked( &a.node ) );
	list_push_back( &list, &a.node );
	TEST_ASSERT_TRUE( list_node_is_linked( &a.node ) );
	list_remove( &list, &a.node );
	TEST_ASSERT_FALSE( list_node_is_linked( &a.node ) );
}

static void test_list_insert_before( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 3 };
	test_item_t c = { .value = 2 };
	list_init( &list );

	list_push_back( &list, &a.node ); /* [1] */
	list_push_back( &list, &b.node ); /* [1, 3] */
	list_insert_before( &list, &c.node, &b.node ); /* [1, 2, 3] */

	TEST_ASSERT_EQ( list_count( &list ), 3 );

	{
		test_item_t *pos;
		int expected[] = { 1, 2, 3 };
		int i = 0;
		LIST_FOR_EACH( pos, &list, test_item_t, node ) {
			TEST_ASSERT_EQ( pos->value, expected[i] );
			i++;
		}
		TEST_ASSERT_EQ( i, 3 );
	}
}

static void test_list_for_each_null_after_loop( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	test_item_t *pos;

	list_init( &list );
	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );

	/* After LIST_FOR_EACH, pos must be NULL (not a sentinel-derived pointer) */
	LIST_FOR_EACH( pos, &list, test_item_t, node ) {
		/* just iterate */
	}
	TEST_ASSERT_TRUE( pos == NULL );
}

static void test_list_for_each_safe_null_after_loop( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	test_item_t *pos, *tmp;

	list_init( &list );
	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );

	LIST_FOR_EACH_SAFE( pos, tmp, &list, test_item_t, node ) {
		/* just iterate */
	}
	TEST_ASSERT_TRUE( pos == NULL );
}

static void test_list_for_each_empty_null( void ) {
	list_head_t list;
	test_item_t *pos;

	list_init( &list );

	/* Empty list should also yield NULL */
	LIST_FOR_EACH( pos, &list, test_item_t, node ) {
		/* never executes */
	}
	TEST_ASSERT_TRUE( pos == NULL );
}

static void test_list_for_each_break_not_null( void ) {
	list_head_t list;
	test_item_t a = { .value = 10 };
	test_item_t b = { .value = 20 };
	test_item_t *pos;

	list_init( &list );
	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );

	/* If we break early, pos should still point to a valid element */
	LIST_FOR_EACH( pos, &list, test_item_t, node ) {
		if ( pos->value == 10 )
			break;
	}
	TEST_ASSERT_TRUE( pos != NULL );
	TEST_ASSERT_EQ( pos->value, 10 );
}

/* --- list_detach tests --- */

static void test_list_detach_preserves_pointers( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	test_item_t c = { .value = 3 };
	list_init( &list );

	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );
	list_push_back( &list, &c.node );

	/* Detach middle item — its next/prev must still point to neighbors */
	list_detach( &list, &b.node );

	TEST_ASSERT_EQ( list_count( &list ), 2 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 1 );
	TEST_ASSERT_EQ( ITEM( list_last( &list ) )->value, 3 );

	/* b's pointers should still reference a and c (stale but valid) */
	TEST_ASSERT( b.node.next == &c.node );
	TEST_ASSERT( b.node.prev == &a.node );
}

static void test_list_detach_first( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	list_init( &list );

	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );

	list_detach( &list, &a.node );

	TEST_ASSERT_EQ( list_count( &list ), 1 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 2 );
	/* a.node.next still points to b */
	TEST_ASSERT( a.node.next == &b.node );
}

static void test_list_detach_last( void ) {
	list_head_t list;
	test_item_t a = { .value = 1 };
	test_item_t b = { .value = 2 };
	list_init( &list );

	list_push_back( &list, &a.node );
	list_push_back( &list, &b.node );

	list_detach( &list, &b.node );

	TEST_ASSERT_EQ( list_count( &list ), 1 );
	TEST_ASSERT_EQ( ITEM( list_first( &list ) )->value, 1 );
	/* b.node.prev still points to a */
	TEST_ASSERT( b.node.prev == &a.node );
}

/*
 * Simulate the extract_char crash scenario: during LIST_FOR_EACH_SAFE,
 * the saved "next" item (tmp) gets detached and moved to another list.
 * With list_remove this would corrupt iteration; with list_detach the
 * loop must still visit all remaining items correctly.
 */
static void test_list_safe_iter_survives_detach_of_next( void ) {
	list_head_t main_list;
	list_head_t other_list;
	test_item_t items[5];
	int i, sum = 0;
	list_init( &main_list );
	list_init( &other_list );

	for ( i = 0; i < 5; i++ ) {
		items[i].value = i + 1; /* 1,2,3,4,5 */
		list_push_back( &main_list, &items[i].node );
	}

	/*
	 * Iterate main_list. When we visit item 2, detach item 3 (the saved
	 * "next") and push it onto other_list via a DIFFERENT mechanism
	 * (simulating extracted_node — here we just re-add to other_list
	 * after detach since detach preserves pointers).
	 */
	{
		test_item_t *pos, *tmp;
		LIST_FOR_EACH_SAFE( pos, tmp, &main_list, test_item_t, node ) {
			if ( pos->value == 2 ) {
				/* Detach item 3 (the saved tmp) from main_list */
				list_detach( &main_list, &items[2].node );
			}
			sum += pos->value;
		}
	}

	/* We should have visited all 5 items: 1+2+3+4+5 = 15.
	 * Item 3 was detached while being "tmp", but detach preserved its
	 * next pointer so the loop continued to item 3, then 4, then 5. */
	TEST_ASSERT_EQ( sum, 15 );
	/* main_list should have 4 items (item 3 was detached) */
	TEST_ASSERT_EQ( list_count( &main_list ), 4 );
}

/*
 * Verify that list_remove of the saved "next" breaks iteration
 * (to confirm that list_detach is necessary — this is the bug scenario).
 * After list_remove, the removed node is self-pointing, so iteration
 * gets stuck or escapes to the wrong list.
 */
static void test_list_safe_iter_remove_next_breaks( void ) {
	list_head_t main_list;
	test_item_t items[4];
	int i, count = 0;
	list_init( &main_list );

	for ( i = 0; i < 4; i++ ) {
		items[i].value = i + 1; /* 1,2,3,4 */
		list_push_back( &main_list, &items[i].node );
	}

	/*
	 * When visiting item 1, remove item 2 (the saved tmp) with
	 * list_remove. This sets item 2's next to itself (self-loop).
	 * The loop will get stuck on item 2 — cap iterations to detect.
	 */
	{
		test_item_t *pos, *tmp;
		LIST_FOR_EACH_SAFE( pos, tmp, &main_list, test_item_t, node ) {
			if ( pos->value == 1 )
				list_remove( &main_list, &items[1].node );
			count++;
			if ( count > 10 ) break; /* prevent infinite loop */
		}
	}

	/* With list_remove, the loop gets stuck and exceeds normal count.
	 * Normal would be 3 (visit 1,3,4 — skipping removed 2) or 4.
	 * The self-loop causes it to hit the cap. */
	TEST_ASSERT_TRUE( count > 4 );
}

/* --- Suite --- */

void suite_list( void ) {
	RUN_TEST( test_list_init_empty );
	RUN_TEST( test_list_push_front_single );
	RUN_TEST( test_list_push_back_single );
	RUN_TEST( test_list_push_front_order );
	RUN_TEST( test_list_push_back_order );
	RUN_TEST( test_list_remove_middle );
	RUN_TEST( test_list_remove_first );
	RUN_TEST( test_list_remove_last );
	RUN_TEST( test_list_remove_only );
	RUN_TEST( test_list_for_each );
	RUN_TEST( test_list_for_each_reverse );
	RUN_TEST( test_list_for_each_safe_remove );
	RUN_TEST( test_list_for_each_safe_remove_all );
	RUN_TEST( test_list_for_each_empty );
	RUN_TEST( test_list_node_init_unlinked );
	RUN_TEST( test_list_node_linked_after_push );
	RUN_TEST( test_list_insert_before );
	RUN_TEST( test_list_for_each_null_after_loop );
	RUN_TEST( test_list_for_each_safe_null_after_loop );
	RUN_TEST( test_list_for_each_empty_null );
	RUN_TEST( test_list_for_each_break_not_null );
	RUN_TEST( test_list_detach_preserves_pointers );
	RUN_TEST( test_list_detach_first );
	RUN_TEST( test_list_detach_last );
	RUN_TEST( test_list_safe_iter_survives_detach_of_next );
	RUN_TEST( test_list_safe_iter_remove_next_breaks );
}
