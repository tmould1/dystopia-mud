/*
 * Unit tests for character stat functions (game/src/core/handler.c, const.c)
 *
 * Tests: get_curr_str/int/wis/dex/con(), can_carry_n(), can_carry_w(), get_trust()
 * Also validates lookup tables: str_app[], int_app[], dex_app[], con_app[]
 */

#include "test_framework.h"
#include "test_helpers.h"

/* --- get_curr_str() tests --- */

void test_get_curr_str_default( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->pcdata->perm_str = 13;
	ch->pcdata->mod_str = 0;
	TEST_ASSERT_EQ( get_curr_str( ch ), 13 );

	free_test_char( ch );
}

void test_get_curr_str_with_modifier( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->pcdata->perm_str = 15;
	ch->pcdata->mod_str = 5;
	TEST_ASSERT_EQ( get_curr_str( ch ), 20 );

	free_test_char( ch );
}

void test_get_curr_str_cap_at_25( void ) {
	CHAR_DATA *ch = make_test_player();

	/* Normal player: capped at 25 */
	ch->pcdata->perm_str = 25;
	ch->pcdata->mod_str = 10;
	TEST_ASSERT_EQ( get_curr_str( ch ), 25 );

	free_test_char( ch );
}

void test_get_curr_str_samurai_cap_35( void ) {
	CHAR_DATA *ch = make_test_player();

	/* Samurai with level >= LEVEL_AVATAR can go to 35 */
	ch->class = CLASS_SAMURAI;
	ch->level = MAX_LEVEL - 9; /* LEVEL_AVATAR */
	ch->pcdata->perm_str = 30;
	ch->pcdata->mod_str = 5;
	TEST_ASSERT_EQ( get_curr_str( ch ), 35 );

	free_test_char( ch );
}

void test_get_curr_str_minimum_3( void ) {
	CHAR_DATA *ch = make_test_player();

	/* Even with negative modifiers, minimum is 3 */
	ch->pcdata->perm_str = 5;
	ch->pcdata->mod_str = -10;
	TEST_ASSERT_EQ( get_curr_str( ch ), 3 );

	free_test_char( ch );
}

void test_get_curr_str_npc_returns_13( void ) {
	CHAR_DATA *ch = make_test_npc();
	TEST_ASSERT_EQ( get_curr_str( ch ), 13 );
	free_test_char( ch );
}

/* --- get_curr_dex() tests --- */

void test_get_curr_dex_default( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->pcdata->perm_dex = 18;
	ch->pcdata->mod_dex = 0;
	TEST_ASSERT_EQ( get_curr_dex( ch ), 18 );

	free_test_char( ch );
}

void test_get_curr_dex_npc_returns_13( void ) {
	CHAR_DATA *ch = make_test_npc();
	TEST_ASSERT_EQ( get_curr_dex( ch ), 13 );
	free_test_char( ch );
}

/* --- get_curr_int/wis/con() basic tests --- */

void test_get_curr_int_basic( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->pcdata->perm_int = 20;
	ch->pcdata->mod_int = 3;
	TEST_ASSERT_EQ( get_curr_int( ch ), 23 );

	free_test_char( ch );
}

void test_get_curr_wis_basic( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->pcdata->perm_wis = 10;
	ch->pcdata->mod_wis = -2;
	TEST_ASSERT_EQ( get_curr_wis( ch ), 8 );

	free_test_char( ch );
}

void test_get_curr_con_basic( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->pcdata->perm_con = 25;
	ch->pcdata->mod_con = 0;
	TEST_ASSERT_EQ( get_curr_con( ch ), 25 );

	free_test_char( ch );
}

/* --- can_carry_n() tests --- */

void test_can_carry_n_normal( void ) {
	CHAR_DATA *ch = make_test_player();

	/* can_carry_n = MAX_WEAR + 2 * get_curr_dex(ch) / 3 */
	ch->pcdata->perm_dex = 18;
	ch->pcdata->mod_dex = 0;
	/* MAX_WEAR = 27, dex = 18, so: 27 + 2 * 18 / 3 = 27 + 12 = 39 */
	TEST_ASSERT_EQ( can_carry_n( ch ), 27 + 2 * 18 / 3 );

	free_test_char( ch );
}

void test_can_carry_n_npc_pet( void ) {
	CHAR_DATA *ch = make_test_npc();

	/* Pet NPCs can carry nothing */
	ch->act |= ACT_PET;
	TEST_ASSERT_EQ( can_carry_n( ch ), 0 );

	free_test_char( ch );
}

/* --- can_carry_w() tests --- */

void test_can_carry_w_normal( void ) {
	CHAR_DATA *ch = make_test_player();

	/* can_carry_w = str_app[get_curr_str(ch)].carry */
	ch->pcdata->perm_str = 18;
	ch->pcdata->mod_str = 0;
	/* str_app[18].carry = 250 */
	TEST_ASSERT_EQ( can_carry_w( ch ), str_app[18].carry );

	free_test_char( ch );
}

void test_can_carry_w_high_str( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->pcdata->perm_str = 25;
	ch->pcdata->mod_str = 0;
	/* str_app[25].carry = 999 */
	TEST_ASSERT_EQ( can_carry_w( ch ), str_app[25].carry );

	free_test_char( ch );
}

/* --- get_trust() tests --- */

void test_get_trust_normal_player( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->level = 5;
	ch->trust = 0;
	ch->desc = NULL;
	/* With trust == 0, returns level */
	TEST_ASSERT_EQ( get_trust( ch ), 5 );

	free_test_char( ch );
}

void test_get_trust_with_trust_override( void ) {
	CHAR_DATA *ch = make_test_player();

	ch->level = 5;
	ch->trust = 10;
	ch->desc = NULL;
	/* With trust != 0, returns trust */
	TEST_ASSERT_EQ( get_trust( ch ), 10 );

	free_test_char( ch );
}

void test_get_trust_npc_capped( void ) {
	CHAR_DATA *ch = make_test_npc();

	/* NPC with level >= LEVEL_HERO gets capped to LEVEL_HERO - 1 */
	ch->level = MAX_LEVEL;
	ch->trust = 0;
	TEST_ASSERT_EQ( get_trust( ch ), LEVEL_HERO - 1 );

	free_test_char( ch );
}

void test_get_trust_npc_below_hero( void ) {
	CHAR_DATA *ch = make_test_npc();

	/* NPC below LEVEL_HERO returns level directly */
	ch->level = 1;
	ch->trust = 0;
	TEST_ASSERT_EQ( get_trust( ch ), 1 );

	free_test_char( ch );
}

/* --- Lookup table validation --- */

void test_str_app_table_bounds( void ) {
	/* str_app[0] should have worst stats */
	TEST_ASSERT_EQ( str_app[0].tohit, -5 );
	TEST_ASSERT_EQ( str_app[0].todam, -4 );
	TEST_ASSERT_EQ( str_app[0].carry, 0 );

	/* str_app[25] should be strong */
	TEST_ASSERT_EQ( str_app[25].tohit, 10 );
	TEST_ASSERT_EQ( str_app[25].todam, 12 );
	TEST_ASSERT_EQ( str_app[25].carry, 999 );

	/* str_app[35] is max */
	TEST_ASSERT_EQ( str_app[35].tohit, 30 );
	TEST_ASSERT_EQ( str_app[35].todam, 32 );
}

void test_str_app_table_monotonic_carry( void ) {
	int i;

	/* Carry capacity should generally increase with strength */
	for ( i = 1; i <= 35; i++ ) {
		TEST_ASSERT( str_app[i].carry >= str_app[i - 1].carry );
	}
}

void test_dex_app_table( void ) {
	/* dex_app[0].defensive = 60 (worst, positive = bad AC) */
	TEST_ASSERT_EQ( dex_app[0].defensive, 60 );

	/* dex_app[25].defensive = -120 (good, negative = good AC) */
	TEST_ASSERT_EQ( dex_app[25].defensive, -120 );
}

void test_con_app_table( void ) {
	/* con_app[0]: low HP per level, low shock */
	TEST_ASSERT_EQ( con_app[0].hitp, -4 );
	TEST_ASSERT_EQ( con_app[0].shock, 20 );

	/* con_app[18]: good HP per level */
	TEST_ASSERT_EQ( con_app[18].hitp, 3 );
	TEST_ASSERT_EQ( con_app[18].shock, 99 );
}

/* --- Test suite --- */

void suite_stats( void ) {
	RUN_TEST( test_get_curr_str_default );
	RUN_TEST( test_get_curr_str_with_modifier );
	RUN_TEST( test_get_curr_str_cap_at_25 );
	RUN_TEST( test_get_curr_str_samurai_cap_35 );
	RUN_TEST( test_get_curr_str_minimum_3 );
	RUN_TEST( test_get_curr_str_npc_returns_13 );
	RUN_TEST( test_get_curr_dex_default );
	RUN_TEST( test_get_curr_dex_npc_returns_13 );
	RUN_TEST( test_get_curr_int_basic );
	RUN_TEST( test_get_curr_wis_basic );
	RUN_TEST( test_get_curr_con_basic );
	RUN_TEST( test_can_carry_n_normal );
	RUN_TEST( test_can_carry_n_npc_pet );
	RUN_TEST( test_can_carry_w_normal );
	RUN_TEST( test_can_carry_w_high_str );
	RUN_TEST( test_get_trust_normal_player );
	RUN_TEST( test_get_trust_with_trust_override );
	RUN_TEST( test_get_trust_npc_capped );
	RUN_TEST( test_get_trust_npc_below_hero );
	RUN_TEST( test_str_app_table_bounds );
	RUN_TEST( test_str_app_table_monotonic_carry );
	RUN_TEST( test_dex_app_table );
	RUN_TEST( test_con_app_table );
}
