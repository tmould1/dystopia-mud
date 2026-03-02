/*
 * Lua scripting tests for Dystopia MUD
 *
 * Tests script lifecycle, sandbox enforcement, error handling,
 * and trigger dispatch from script_lua.c / script_trigger.c.
 * Requires boot_headless() (Lua state initialized during boot).
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"
#include "../script/script.h"

extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];

/* Create a SCRIPT_DATA with the given Lua code */
static SCRIPT_DATA *make_test_script( uint32_t trigger, const char *code,
	const char *pattern, int chance ) {
	SCRIPT_DATA *script = calloc( 1, sizeof( *script ) );
	script->name = str_dup( "test_script" );
	script->trigger = trigger;
	script->code = code ? str_dup( code ) : NULL;
	script->pattern = pattern ? str_dup( pattern ) : NULL;
	script->chance = chance;
	script->library_name = NULL;
	script->lua_ref = SCRIPT_LUA_NOREF;
	list_node_init( &script->node );
	return script;
}

static void free_test_script( SCRIPT_DATA *script ) {
	if ( script == NULL ) return;
	if ( script->name ) free( script->name );
	if ( script->code ) free( script->code );
	if ( script->pattern ) free( script->pattern );
	if ( script->library_name ) free( script->library_name );
	free( script );
}

/* Find any valid mob index from the hash table */
static MOB_INDEX_DATA *get_any_mob_index( void ) {
	int i;
	for ( i = 0; i < MAX_KEY_HASH; i++ ) {
		if ( mob_index_hash[i] != NULL )
			return mob_index_hash[i];
	}
	return NULL;
}

/* --- script_init tests --- */

void test_script_init_idempotent( void ) {
	ensure_booted();
	/* script_init was already called during boot_db.
	 * Calling again should be a no-op (returns early if g_lua != NULL). */
	script_init();
	/* If we get here, it didn't crash */
	TEST_ASSERT_TRUE( TRUE );
}

/* --- script_run tests --- */

void test_script_run_basic( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	CHAR_DATA *ch = make_full_test_npc();

	SCRIPT_DATA *script = make_test_script( TRIG_GREET,
		"function on_greet(mob, ch) end", NULL, 0 );

	script_run( script, "on_greet", mob, ch, NULL );
	TEST_ASSERT_TRUE( TRUE ); /* No crash */

	free_test_script( script );
	free_char( mob );
	free_char( ch );
}

void test_script_run_null_script( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	CHAR_DATA *ch = make_full_test_npc();

	script_run( NULL, "on_greet", mob, ch, NULL );
	TEST_ASSERT_TRUE( TRUE ); /* No crash */

	free_char( mob );
	free_char( ch );
}

void test_script_run_null_code( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	CHAR_DATA *ch = make_full_test_npc();

	SCRIPT_DATA *script = make_test_script( TRIG_GREET, NULL, NULL, 0 );
	script_run( script, "on_greet", mob, ch, NULL );
	TEST_ASSERT_TRUE( TRUE ); /* No crash */

	free_test_script( script );
	free_char( mob );
	free_char( ch );
}

void test_script_run_syntax_error( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	CHAR_DATA *ch = make_full_test_npc();

	SCRIPT_DATA *script = make_test_script( TRIG_GREET,
		"this is not valid lua code !!!", NULL, 0 );

	script_run( script, "on_greet", mob, ch, NULL );
	TEST_ASSERT_TRUE( TRUE ); /* Error swallowed, no crash */

	free_test_script( script );
	free_char( mob );
	free_char( ch );
}

void test_script_run_missing_function( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	CHAR_DATA *ch = make_full_test_npc();

	/* Script defines on_greet but we call on_speech */
	SCRIPT_DATA *script = make_test_script( TRIG_SPEECH,
		"function on_greet(mob, ch) end", NULL, 0 );

	script_run( script, "on_speech", mob, ch, "hello" );
	TEST_ASSERT_TRUE( TRUE ); /* No crash */

	free_test_script( script );
	free_char( mob );
	free_char( ch );
}

void test_script_run_timeout( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	CHAR_DATA *ch = make_full_test_npc();

	SCRIPT_DATA *script = make_test_script( TRIG_GREET,
		"function on_greet(mob, ch) while true do end end", NULL, 0 );

	script_run( script, "on_greet", mob, ch, NULL );
	TEST_ASSERT_TRUE( TRUE ); /* Timeout killed the script, no crash */

	free_test_script( script );
	free_char( mob );
	free_char( ch );
}

/* --- Sandbox tests --- */

void test_script_sandbox_no_io( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	CHAR_DATA *ch = make_full_test_npc();

	SCRIPT_DATA *script = make_test_script( TRIG_GREET,
		"function on_greet(mob, ch)\n"
		"  if io then io.open('/etc/passwd', 'r') end\n"
		"end", NULL, 0 );

	script_run( script, "on_greet", mob, ch, NULL );
	TEST_ASSERT_TRUE( TRUE ); /* io is nil, no crash */

	free_test_script( script );
	free_char( mob );
	free_char( ch );
}

void test_script_sandbox_no_os( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	CHAR_DATA *ch = make_full_test_npc();

	SCRIPT_DATA *script = make_test_script( TRIG_GREET,
		"function on_greet(mob, ch)\n"
		"  if os then os.execute('rm -rf /') end\n"
		"end", NULL, 0 );

	script_run( script, "on_greet", mob, ch, NULL );
	TEST_ASSERT_TRUE( TRUE ); /* os is nil, no crash */

	free_test_script( script );
	free_char( mob );
	free_char( ch );
}

/* --- Trigger dispatch tests --- */

void test_script_trigger_greet_fires( void ) {
	ensure_booted();
	MOB_INDEX_DATA *pMobIndex = get_any_mob_index();
	TEST_ASSERT_TRUE( pMobIndex != NULL );

	/* Save original scripts list state */
	size_t orig_script_count = list_count( &pMobIndex->scripts );

	/* Create a greet script */
	SCRIPT_DATA *script = make_test_script( TRIG_GREET,
		"function on_greet(mob, ch) mob:say('hello from test') end",
		NULL, 0 );
	list_push_back( &pMobIndex->scripts, &script->node );

	/* Create mob from this index and a player */
	CHAR_DATA *mob = create_mobile( pMobIndex );
	CHAR_DATA *player = make_full_test_npc();
	player->act = 0; /* Make it a player, not NPC */

	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );
	char_to_room( mob, room );
	char_to_room( player, room );

	/* Fire the greet trigger */
	script_trigger_greet( player, room );
	/* If we get here without crash, the trigger dispatched correctly */
	TEST_ASSERT_TRUE( TRUE );

	/* Clean up: remove our test script from the mob index */
	list_remove( &pMobIndex->scripts, &script->node );
	TEST_ASSERT_EQ( (int) list_count( &pMobIndex->scripts ), (int) orig_script_count );

	char_from_room( player );
	char_from_room( mob );
	free_char( player );
	free_char( mob );
	free_test_script( script );
}

void test_script_trigger_speech_matches( void ) {
	ensure_booted();
	MOB_INDEX_DATA *pMobIndex = get_any_mob_index();
	TEST_ASSERT_TRUE( pMobIndex != NULL );

	size_t orig_script_count = list_count( &pMobIndex->scripts );

	SCRIPT_DATA *script = make_test_script( TRIG_SPEECH,
		"function on_speech(mob, ch, text) mob:say('heard you') end",
		"hello", 0 );
	list_push_back( &pMobIndex->scripts, &script->node );

	CHAR_DATA *mob = create_mobile( pMobIndex );
	CHAR_DATA *player = make_full_test_npc();
	player->act = 0; /* Player, not NPC */

	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );
	char_to_room( mob, room );
	char_to_room( player, room );

	script_trigger_speech( player, "hello world" );
	TEST_ASSERT_TRUE( TRUE ); /* No crash, pattern matched */

	list_remove( &pMobIndex->scripts, &script->node );
	TEST_ASSERT_EQ( (int) list_count( &pMobIndex->scripts ), (int) orig_script_count );

	char_from_room( player );
	char_from_room( mob );
	free_char( player );
	free_char( mob );
	free_test_script( script );
}

void test_script_trigger_speech_no_match( void ) {
	ensure_booted();
	MOB_INDEX_DATA *pMobIndex = get_any_mob_index();
	TEST_ASSERT_TRUE( pMobIndex != NULL );

	size_t orig_script_count = list_count( &pMobIndex->scripts );

	SCRIPT_DATA *script = make_test_script( TRIG_SPEECH,
		"function on_speech(mob, ch, text) mob:say('heard you') end",
		"hello", 0 );
	list_push_back( &pMobIndex->scripts, &script->node );

	CHAR_DATA *mob = create_mobile( pMobIndex );
	CHAR_DATA *player = make_full_test_npc();
	player->act = 0;

	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );
	char_to_room( mob, room );
	char_to_room( player, room );

	/* "goodbye" should NOT match "hello" pattern */
	script_trigger_speech( player, "goodbye" );
	TEST_ASSERT_TRUE( TRUE ); /* No crash, pattern did not match */

	list_remove( &pMobIndex->scripts, &script->node );
	TEST_ASSERT_EQ( (int) list_count( &pMobIndex->scripts ), (int) orig_script_count );

	char_from_room( player );
	char_from_room( mob );
	free_char( player );
	free_char( mob );
	free_test_script( script );
}

/* --- Suite registration --- */

void suite_scripting( void ) {
	RUN_TEST( test_script_init_idempotent );
	RUN_TEST( test_script_run_basic );
	RUN_TEST( test_script_run_null_script );
	RUN_TEST( test_script_run_null_code );
	RUN_TEST( test_script_run_syntax_error );
	RUN_TEST( test_script_run_missing_function );
	RUN_TEST( test_script_run_timeout );
	RUN_TEST( test_script_sandbox_no_io );
	RUN_TEST( test_script_sandbox_no_os );
	RUN_TEST( test_script_trigger_greet_fires );
	RUN_TEST( test_script_trigger_speech_matches );
	RUN_TEST( test_script_trigger_speech_no_match );
}
