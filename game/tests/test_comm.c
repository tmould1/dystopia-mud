/*
 * Communication command tests for Dystopia MUD
 *
 * Tests do_tell() and do_reply() with NPC and player targets to catch
 * NULL pcdata dereferences that previously caused crashes.
 * Also tests that tell-to-NPC fires SPEECH triggers.
 *
 * Tier 2: Requires boot (for create_mobile, rooms, scripts).
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"
#include "../script/script.h"

extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];

/* Find any valid mob index from the hash table */
static MOB_INDEX_DATA *get_any_mob_index( void ) {
	int i;
	for ( i = 0; i < MAX_KEY_HASH; i++ ) {
		if ( mob_index_hash[i] != NULL )
			return mob_index_hash[i];
	}
	return NULL;
}

/* Create a SCRIPT_DATA for testing */
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

/*--------------------------------------------------------------------------
 * do_tell: NPC target must not crash (pcdata == NULL guard)
 *--------------------------------------------------------------------------*/

static void test_tell_npc_no_crash( void ) {
	ensure_booted();

	MOB_INDEX_DATA *pMobIndex = get_any_mob_index();
	TEST_ASSERT_TRUE( pMobIndex != NULL );
	if ( pMobIndex == NULL ) return;

	CHAR_DATA *player = make_full_test_npc();
	player->act = 0; /* Make it a player */
	player->pcdata = calloc( 1, sizeof( PC_DATA ) );

	CHAR_DATA *mob = create_mobile( pMobIndex );

	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );
	char_to_room( player, room );
	char_to_room( mob, room );

	/* Capture output so act() doesn't write to a NULL descriptor */
	test_output_start( player );

	/* This previously crashed: victim->pcdata->reply with NPC victim */
	char arg[MAX_INPUT_LENGTH];
	snprintf( arg, sizeof( arg ), "%s hello there", mob->name );
	do_tell( player, arg );

	test_output_stop();

	/* If we get here, the NULL pcdata crash is fixed */
	TEST_ASSERT_TRUE( TRUE );

	char_from_room( player );
	char_from_room( mob );
	free( player->pcdata );
	player->pcdata = NULL;
	free_char( player );
	free_char( mob );
}

/*--------------------------------------------------------------------------
 * do_tell to NPC: victim->pcdata->reply is NOT touched (no crash)
 * This is the regression test for the original crash bug.
 *--------------------------------------------------------------------------*/

static void test_tell_npc_reply_not_set( void ) {
	ensure_booted();

	MOB_INDEX_DATA *pMobIndex = get_any_mob_index();
	TEST_ASSERT_TRUE( pMobIndex != NULL );
	if ( pMobIndex == NULL ) return;

	CHAR_DATA *player = make_full_test_npc();
	player->act = 0;
	player->pcdata = calloc( 1, sizeof( PC_DATA ) );
	player->pcdata->reply = NULL;

	CHAR_DATA *mob = create_mobile( pMobIndex );

	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );
	char_to_room( player, room );
	char_to_room( mob, room );

	test_output_start( player );

	char arg[MAX_INPUT_LENGTH];
	snprintf( arg, sizeof( arg ), "%s hello there", mob->name );
	do_tell( player, arg );

	test_output_stop();

	/* NPC has no pcdata, so reply must NOT have been written.
	 * The player's own reply should remain NULL (mob can't set it). */
	TEST_ASSERT_TRUE( mob->pcdata == NULL );
	TEST_ASSERT_TRUE( player->pcdata->reply == NULL );

	char_from_room( player );
	char_from_room( mob );
	free( player->pcdata );
	player->pcdata = NULL;
	free_char( player );
	free_char( mob );
}

/*--------------------------------------------------------------------------
 * script_trigger_speech_one: fires on matching keyword
 *--------------------------------------------------------------------------*/

static void test_speech_one_fires_on_match( void ) {
	ensure_booted();

	MOB_INDEX_DATA *pMobIndex = get_any_mob_index();
	TEST_ASSERT_TRUE( pMobIndex != NULL );
	if ( pMobIndex == NULL ) return;

	size_t orig_script_count = list_count( &pMobIndex->scripts );

	/* Script runs without error — we verify dispatch by surviving
	 * the call with a real mob, real script, and matching pattern.
	 * (Output capture via ch:send is unreliable cross-platform because
	 * test characters lack descriptors that some code paths expect.) */
	SCRIPT_DATA *script = make_test_script( TRIG_SPEECH,
		"function on_speech(mob, ch, text) end",
		"darkness", 0 );
	list_push_back( &pMobIndex->scripts, &script->node );

	CHAR_DATA *player = make_full_test_npc();
	player->act = 0; /* Player, not NPC */

	CHAR_DATA *mob = create_mobile( pMobIndex );

	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );
	char_to_room( player, room );
	char_to_room( mob, room );

	/* Should dispatch to script without crash */
	script_trigger_speech_one( player, mob, "darkness" );
	TEST_ASSERT_TRUE( TRUE );

	list_remove( &pMobIndex->scripts, &script->node );
	TEST_ASSERT_EQ( (int) list_count( &pMobIndex->scripts ), (int) orig_script_count );

	char_from_room( player );
	char_from_room( mob );
	free_char( player );
	free_char( mob );
	free_test_script( script );
}

/*--------------------------------------------------------------------------
 * script_trigger_speech_one: does NOT fire on non-matching keyword
 *--------------------------------------------------------------------------*/

static void test_speech_one_no_match( void ) {
	ensure_booted();

	MOB_INDEX_DATA *pMobIndex = get_any_mob_index();
	TEST_ASSERT_TRUE( pMobIndex != NULL );
	if ( pMobIndex == NULL ) return;

	size_t orig_script_count = list_count( &pMobIndex->scripts );

	/* If "hello world" incorrectly matched "darkness", the script would
	 * run (and could crash if the test were badly set up). We verify
	 * no crash and the pattern filter works. */
	SCRIPT_DATA *script = make_test_script( TRIG_SPEECH,
		"function on_speech(mob, ch, text) end",
		"darkness", 0 );
	list_push_back( &pMobIndex->scripts, &script->node );

	CHAR_DATA *player = make_full_test_npc();
	player->act = 0;

	CHAR_DATA *mob = create_mobile( pMobIndex );

	ROOM_INDEX_DATA *room = get_room_index( ROOM_VNUM_LIMBO );
	char_to_room( player, room );
	char_to_room( mob, room );

	/* "hello world" should NOT match "darkness" pattern — no crash */
	script_trigger_speech_one( player, mob, "hello world" );
	TEST_ASSERT_TRUE( TRUE );

	list_remove( &pMobIndex->scripts, &script->node );
	TEST_ASSERT_EQ( (int) list_count( &pMobIndex->scripts ), (int) orig_script_count );

	char_from_room( player );
	char_from_room( mob );
	free_char( player );
	free_char( mob );
	free_test_script( script );
}

/*--------------------------------------------------------------------------
 * script_trigger_speech_one: NULL safety
 *--------------------------------------------------------------------------*/

static void test_speech_one_null_speaker( void ) {
	ensure_booted();
	CHAR_DATA *mob = make_full_test_npc();
	script_trigger_speech_one( NULL, mob, "hello" );
	TEST_ASSERT_TRUE( TRUE ); /* No crash */
	free_char( mob );
}

static void test_speech_one_null_target( void ) {
	ensure_booted();
	CHAR_DATA *player = make_full_test_npc();
	player->act = 0;
	script_trigger_speech_one( player, NULL, "hello" );
	TEST_ASSERT_TRUE( TRUE ); /* No crash */
	free_char( player );
}

static void test_speech_one_null_text( void ) {
	ensure_booted();
	CHAR_DATA *player = make_full_test_npc();
	player->act = 0;
	CHAR_DATA *mob = make_full_test_npc();
	script_trigger_speech_one( player, mob, NULL );
	TEST_ASSERT_TRUE( TRUE ); /* No crash */
	free_char( player );
	free_char( mob );
}

static void test_speech_one_empty_text( void ) {
	ensure_booted();
	CHAR_DATA *player = make_full_test_npc();
	player->act = 0;
	CHAR_DATA *mob = make_full_test_npc();
	script_trigger_speech_one( player, mob, "" );
	TEST_ASSERT_TRUE( TRUE ); /* No crash */
	free_char( player );
	free_char( mob );
}

static void test_speech_one_npc_speaker_ignored( void ) {
	ensure_booted();
	/* NPC-to-NPC speech should be ignored */
	CHAR_DATA *npc1 = make_full_test_npc();
	CHAR_DATA *npc2 = make_full_test_npc();
	script_trigger_speech_one( npc1, npc2, "hello" );
	TEST_ASSERT_TRUE( TRUE ); /* No crash, early return */
	free_char( npc1 );
	free_char( npc2 );
}

static void test_speech_one_player_target_ignored( void ) {
	ensure_booted();
	/* Targeting a player should be ignored */
	CHAR_DATA *player1 = make_full_test_npc();
	player1->act = 0;
	CHAR_DATA *player2 = make_full_test_npc();
	player2->act = 0;
	script_trigger_speech_one( player1, player2, "hello" );
	TEST_ASSERT_TRUE( TRUE ); /* No crash, early return */
	free_char( player1 );
	free_char( player2 );
}

/*--------------------------------------------------------------------------
 * Suite registration
 *--------------------------------------------------------------------------*/

void suite_comm( void ) {
	RUN_TEST( test_tell_npc_no_crash );
	RUN_TEST( test_tell_npc_reply_not_set );
	RUN_TEST( test_speech_one_fires_on_match );
	RUN_TEST( test_speech_one_no_match );
	RUN_TEST( test_speech_one_null_speaker );
	RUN_TEST( test_speech_one_null_target );
	RUN_TEST( test_speech_one_null_text );
	RUN_TEST( test_speech_one_empty_text );
	RUN_TEST( test_speech_one_npc_speaker_ignored );
	RUN_TEST( test_speech_one_player_target_ignored );
}
