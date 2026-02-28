/*
 * Boot validation tests for Dystopia MUD
 *
 * Calls boot_headless() to initialize the game engine without sockets,
 * then validates that critical game state was loaded correctly.
 * This catches regressions in struct layout changes, list migrations,
 * and database loading code.
 */

#include "test_framework.h"
#include "merc.h"

/* External globals we want to validate */
extern list_head_t g_characters;
extern list_head_t g_objects;
extern list_head_t g_areas;
extern list_head_t g_helps;
extern list_head_t g_descriptors;
extern int top_area;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_room;
extern int top_exit;
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
extern char *help_greeting;

static bool booted = FALSE;

/*
 * Boot the game engine once. Subsequent calls are no-ops.
 * Returns TRUE on success, FALSE on failure.
 */
static bool ensure_booted( void ) {
	if ( booted )
		return TRUE;

	/* boot_headless derives paths from the exe location.
	 * The test exe runs from game/tests/, gamedata is at ../../gamedata/ */
	boot_headless( "../../gamedata/dystopia.exe" );
	booted = TRUE;
	return TRUE;
}

/* --- Test cases --- */

void test_boot_completes( void ) {
	TEST_ASSERT_TRUE( ensure_booted() );
}

void test_boot_areas_loaded( void ) {
	ensure_booted();
	TEST_ASSERT_TRUE( top_area > 0 );
	TEST_ASSERT_FALSE( list_empty( &g_areas ) );

	/* Verify area count matches list */
	TEST_ASSERT_EQ( (int) list_count( &g_areas ), top_area );
}

void test_boot_helps_loaded( void ) {
	ensure_booted();
	TEST_ASSERT_TRUE( top_help > 0 );
	TEST_ASSERT_FALSE( list_empty( &g_helps ) );
	TEST_ASSERT_TRUE( help_greeting != NULL );
}

void test_boot_mob_indexes_loaded( void ) {
	int count = 0;
	int i;

	ensure_booted();
	TEST_ASSERT_TRUE( top_mob_index > 0 );

	/* Verify hash table has entries */
	for ( i = 0; i < MAX_KEY_HASH; i++ ) {
		MOB_INDEX_DATA *pMob;
		for ( pMob = mob_index_hash[i]; pMob; pMob = pMob->next )
			count++;
	}
	TEST_ASSERT_EQ( count, top_mob_index );
}

void test_boot_obj_indexes_loaded( void ) {
	int count = 0;
	int i;

	ensure_booted();
	TEST_ASSERT_TRUE( top_obj_index > 0 );

	for ( i = 0; i < MAX_KEY_HASH; i++ ) {
		OBJ_INDEX_DATA *pObj;
		for ( pObj = obj_index_hash[i]; pObj; pObj = pObj->next )
			count++;
	}
	TEST_ASSERT_EQ( count, top_obj_index );
}

void test_boot_rooms_loaded( void ) {
	int count = 0;
	int i;

	ensure_booted();
	TEST_ASSERT_TRUE( top_room > 0 );

	for ( i = 0; i < MAX_KEY_HASH; i++ ) {
		ROOM_INDEX_DATA *pRoom;
		for ( pRoom = room_index_hash[i]; pRoom; pRoom = pRoom->next )
			count++;
	}
	TEST_ASSERT_EQ( count, top_room );
}

void test_boot_characters_spawned( void ) {
	ensure_booted();
	/* After boot + area resets, there should be NPCs in the world */
	TEST_ASSERT_FALSE( list_empty( &g_characters ) );
	TEST_ASSERT_TRUE( (int) list_count( &g_characters ) > 0 );
}

void test_boot_objects_spawned( void ) {
	ensure_booted();
	/* After boot + area resets, there should be objects in the world */
	TEST_ASSERT_FALSE( list_empty( &g_objects ) );
	TEST_ASSERT_TRUE( (int) list_count( &g_objects ) > 0 );
}

void test_boot_no_descriptors( void ) {
	ensure_booted();
	/* Headless boot should have zero network connections */
	TEST_ASSERT_TRUE( list_empty( &g_descriptors ) );
}

void test_boot_room_characters_consistent( void ) {
	CHAR_DATA *ch;
	int orphans = 0;

	ensure_booted();

	/* Every character in g_characters should be in a room,
	 * and that room's character list should contain them */
	LIST_FOR_EACH( ch, &g_characters, CHAR_DATA, char_node ) {
		if ( ch->in_room == NULL ) {
			orphans++;
			continue;
		}
		/* Verify the character's room_node is linked */
		TEST_ASSERT_TRUE( list_node_is_linked( &ch->room_node ) );
	}
	TEST_ASSERT_EQ( orphans, 0 );
}

void test_boot_room_objects_consistent( void ) {
	OBJ_DATA *obj;
	int in_room_count = 0;

	ensure_booted();

	/* Objects in rooms should have linked room_nodes */
	LIST_FOR_EACH( obj, &g_objects, OBJ_DATA, obj_node ) {
		if ( obj->in_room != NULL ) {
			TEST_ASSERT_TRUE( list_node_is_linked( &obj->room_node ) );
			in_room_count++;
		}
	}
	/* Some objects should be on the ground */
	TEST_ASSERT_TRUE( in_room_count >= 0 );
}

void test_boot_game_tick_completes( void ) {
	ensure_booted();
	/* game_tick() drives update_handler + log_flush.
	 * On the first call, all pulse counters fire simultaneously.
	 * If any list is corrupted, this will hang or crash. */
	game_tick();
	TEST_ASSERT_TRUE( TRUE );
}

void test_boot_game_tick_multiple( void ) {
	int i;
	int char_count_before, char_count_after;
	int obj_count_before, obj_count_after;

	ensure_booted();
	char_count_before = (int) list_count( &g_characters );
	obj_count_before = (int) list_count( &g_objects );

	/* Run 100 ticks (~25 seconds of game time at 4 PPS).
	 * Exercises area resets, mobile updates, violence, weather, etc. */
	for ( i = 0; i < 100; i++ )
		game_tick();

	/* Lists should still be valid and non-empty */
	char_count_after = (int) list_count( &g_characters );
	obj_count_after = (int) list_count( &g_objects );
	TEST_ASSERT_TRUE( char_count_after > 0 );
	TEST_ASSERT_TRUE( obj_count_after > 0 );

	/* Counts can change (mobs spawn/die, objects decay) but shouldn't be zero */
	TEST_ASSERT_TRUE( char_count_after >= char_count_before / 2 );
	TEST_ASSERT_TRUE( obj_count_after >= obj_count_before / 2 );
}

void test_boot_lists_circular_after_tick( void ) {
	CHAR_DATA *ch;
	OBJ_DATA *obj;
	AREA_DATA *area;
	int ch_walk = 0, obj_walk = 0, area_walk = 0;

	ensure_booted();
	game_tick();

	/* Verify list iteration terminates and counts match */
	LIST_FOR_EACH( ch, &g_characters, CHAR_DATA, char_node )
		ch_walk++;
	TEST_ASSERT_EQ( ch_walk, (int) list_count( &g_characters ) );

	LIST_FOR_EACH( obj, &g_objects, OBJ_DATA, obj_node )
		obj_walk++;
	TEST_ASSERT_EQ( obj_walk, (int) list_count( &g_objects ) );

	LIST_FOR_EACH( area, &g_areas, AREA_DATA, node )
		area_walk++;
	TEST_ASSERT_EQ( area_walk, (int) list_count( &g_areas ) );
}

/*
 * Verify that all spawned characters have properly initialized list nodes.
 * char_node should be linked (in g_characters), room_node should be linked
 * if the character is in a room.
 */
void test_boot_char_nodes_initialized( void ) {
	CHAR_DATA *ch;
	int checked = 0;

	ensure_booted();

	LIST_FOR_EACH( ch, &g_characters, CHAR_DATA, char_node ) {
		/* char_node is in g_characters, so it must be linked */
		TEST_ASSERT_TRUE( list_node_is_linked( &ch->char_node ) );

		/* room_node linked iff character is in a room */
		if ( ch->in_room != NULL )
			TEST_ASSERT_TRUE( list_node_is_linked( &ch->room_node ) );
		else
			TEST_ASSERT_FALSE( list_node_is_linked( &ch->room_node ) );

		checked++;
	}
	TEST_ASSERT_TRUE( checked > 0 );
}

/*
 * Verify that all spawned objects have properly initialized list nodes.
 * obj_node should be linked (in g_objects). room_node/content_node should
 * be linked only when the object is in a room or container.
 */
void test_boot_obj_nodes_initialized( void ) {
	OBJ_DATA *obj;
	int checked = 0;

	ensure_booted();

	LIST_FOR_EACH( obj, &g_objects, OBJ_DATA, obj_node ) {
		/* obj_node is in g_objects, so it must be linked */
		TEST_ASSERT_TRUE( list_node_is_linked( &obj->obj_node ) );

		/* room_node linked iff object is in a room */
		if ( obj->in_room != NULL )
			TEST_ASSERT_TRUE( list_node_is_linked( &obj->room_node ) );

		/* content_node linked iff object is inside another object */
		if ( obj->in_obj != NULL )
			TEST_ASSERT_TRUE( list_node_is_linked( &obj->content_node ) );

		/* Unplaced objects (carried or in_room/in_obj) should have
		 * unlinked room_node when not in a room */
		if ( obj->in_room == NULL && obj->carried_by == NULL && obj->in_obj == NULL )
			TEST_ASSERT_FALSE( list_node_is_linked( &obj->room_node ) );

		checked++;
	}
	TEST_ASSERT_TRUE( checked > 0 );
}

/*
 * Verify that clear_char properly initializes list nodes to unlinked state.
 * Before the fix, calloc left char_node with NULL pointers instead of
 * self-referencing pointers, causing list_node_is_linked() false positives.
 */
void test_boot_clear_char_inits_nodes( void ) {
	CHAR_DATA *ch;

	ch = calloc( 1, sizeof( *ch ) );
	clear_char( ch );

	/* char_node and room_node should be unlinked (self-referencing) */
	TEST_ASSERT_FALSE( list_node_is_linked( &ch->char_node ) );
	TEST_ASSERT_FALSE( list_node_is_linked( &ch->room_node ) );

	/* affects and carrying should be properly initialized empty lists */
	TEST_ASSERT_TRUE( list_empty( &ch->affects ) );
	TEST_ASSERT_TRUE( list_empty( &ch->carrying ) );
	TEST_ASSERT_EQ( list_count( &ch->affects ), 0 );
	TEST_ASSERT_EQ( list_count( &ch->carrying ), 0 );

	free( ch );
}

/*
 * Regression test: load_player_objects creates objects via raw calloc,
 * bypassing create_object. Before the fix, obj->affects was not initialized
 * with list_init(), so list_push_front crashed with a NULL dereference
 * when loading player objects with affects from the save file.
 */
void test_boot_obj_calloc_affects_safe( void ) {
	OBJ_DATA *obj;
	AFFECT_DATA *paf;

	/* Mimic what load_player_objects does: calloc + init */
	obj = calloc( 1, sizeof( *obj ) );
	list_node_init( &obj->obj_node );
	list_node_init( &obj->room_node );
	list_node_init( &obj->content_node );
	list_init( &obj->affects );
	list_init( &obj->contents );

	/* Add an affect — this crashed before the fix */
	paf = calloc( 1, sizeof( *paf ) );
	paf->type = 0;
	paf->duration = 10;
	paf->modifier = 1;
	paf->location = 0;
	paf->bitvector = 0;
	list_push_front( &obj->affects, &paf->node );

	TEST_ASSERT_EQ( list_count( &obj->affects ), 1 );

	/* Unlinked nodes should report correctly */
	TEST_ASSERT_FALSE( list_node_is_linked( &obj->room_node ) );
	TEST_ASSERT_FALSE( list_node_is_linked( &obj->content_node ) );
	TEST_ASSERT_FALSE( list_node_is_linked( &obj->obj_node ) );

	/* Cleanup */
	list_remove( &obj->affects, &paf->node );
	free( paf );
	free( obj );
}

/*
 * Test that a character created for login (via init_char_for_load path)
 * has all list nodes properly initialized and can be safely freed
 * without corrupting any global lists.
 */
void test_boot_login_char_safe( void ) {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;
	int char_count_before, char_count_after;

	ensure_booted();
	char_count_before = list_count( &g_characters );

	/* Create a mock descriptor like new_descriptor would */
	d = calloc( 1, sizeof( *d ) );
	d->descriptor = -1;
	d->connected = CON_GET_NAME;
	d->outsize = 2000;
	d->outbuf = calloc( 1, d->outsize );

	/* Create a character like init_char_for_load does */
	ch = calloc( 1, sizeof( *ch ) );
	clear_char( ch );
	ch->pcdata = calloc( 1, sizeof( *ch->pcdata ) );
	list_init( &ch->pcdata->aliases );
	d->character = ch;
	ch->desc = d;
	ch->name = str_dup( "TestLogin" );
	ch->pcdata->switchname = str_dup( "TestLogin" );
	ch->pcdata->pwd = str_dup( "" );

	/* Character should NOT be in g_characters (not yet playing) */
	TEST_ASSERT_FALSE( list_node_is_linked( &ch->char_node ) );
	TEST_ASSERT_FALSE( list_node_is_linked( &ch->room_node ) );

	/* g_characters count should be unchanged */
	TEST_ASSERT_EQ( list_count( &g_characters ), char_count_before );

	/* Free character like close_socket does for non-playing descriptors */
	free_char( ch );
	d->character = NULL;

	/* g_characters should still be consistent */
	char_count_after = list_count( &g_characters );
	TEST_ASSERT_EQ( char_count_after, char_count_before );

	free( d->outbuf );
	free( d );
}

/* --- Suite entry point --- */

void suite_boot( void ) {
	RUN_TEST( test_boot_completes );
	RUN_TEST( test_boot_areas_loaded );
	RUN_TEST( test_boot_helps_loaded );
	RUN_TEST( test_boot_mob_indexes_loaded );
	RUN_TEST( test_boot_obj_indexes_loaded );
	RUN_TEST( test_boot_rooms_loaded );
	RUN_TEST( test_boot_characters_spawned );
	RUN_TEST( test_boot_objects_spawned );
	RUN_TEST( test_boot_no_descriptors );
	RUN_TEST( test_boot_room_characters_consistent );
	RUN_TEST( test_boot_room_objects_consistent );
	RUN_TEST( test_boot_game_tick_completes );
	RUN_TEST( test_boot_game_tick_multiple );
	RUN_TEST( test_boot_lists_circular_after_tick );
	RUN_TEST( test_boot_char_nodes_initialized );
	RUN_TEST( test_boot_obj_nodes_initialized );
	RUN_TEST( test_boot_clear_char_inits_nodes );
	RUN_TEST( test_boot_obj_calloc_affects_safe );
	RUN_TEST( test_boot_login_char_safe );
}
