/*
 * Command interpreter tests for Dystopia MUD
 *
 * Tests can_interpret() from interp.c.
 * cmd_table is a compile-time array, no boot needed.
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"

extern const struct cmd_type cmd_table[];

/* Cached command indices, set up in suite init */
static int cmd_north = -1;
static int cmd_look = -1;
static int cmd_inventory = -1;
static int cmd_wizhelp = -1;  /* high-level imm command */

static void find_test_commands( void ) {
	cmd_north = find_cmd_index( "north" );
	cmd_look = find_cmd_index( "look" );
	cmd_inventory = find_cmd_index( "inventory" );
	cmd_wizhelp = find_cmd_index( "wizhelp" );
}

/* --- Basic permission tests --- */

void test_interp_basic_allowed( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->level = 1;
	ch->position = POS_STANDING;

	TEST_ASSERT_TRUE( cmd_north >= 0 );
	TEST_ASSERT_EQ( can_interpret( ch, cmd_north ), 1 );
	free_test_char( ch );
}

void test_interp_level_denied( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->level = 1;
	ch->position = POS_STANDING;

	if ( cmd_wizhelp >= 0 && cmd_table[cmd_wizhelp].level > 1 ) {
		TEST_ASSERT_EQ( can_interpret( ch, cmd_wizhelp ), 0 );
	}
	free_test_char( ch );
}

void test_interp_imm_allowed( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->level = MAX_LEVEL;
	ch->position = POS_STANDING;

	if ( cmd_wizhelp >= 0 ) {
		TEST_ASSERT_EQ( can_interpret( ch, cmd_wizhelp ), 1 );
	}
	free_test_char( ch );
}

void test_interp_wrong_position( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->level = 1;
	ch->position = POS_SLEEPING;

	/* "north" requires POS_STANDING */
	if ( cmd_north >= 0 && cmd_table[cmd_north].position > POS_SLEEPING ) {
		TEST_ASSERT_EQ( can_interpret( ch, cmd_north ), -1 );
	}
	free_test_char( ch );
}

void test_interp_dead_pos_inventory( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->level = 1;
	ch->position = POS_DEAD;

	/* "inventory" should have a very low position requirement */
	if ( cmd_inventory >= 0 && cmd_table[cmd_inventory].position <= POS_DEAD ) {
		TEST_ASSERT_EQ( can_interpret( ch, cmd_inventory ), 1 );
	}
	free_test_char( ch );
}

void test_interp_npc_level_denied( void ) {
	CHAR_DATA *ch = make_test_npc();
	ch->level = 1;
	ch->position = POS_STANDING;

	if ( cmd_wizhelp >= 0 && cmd_table[cmd_wizhelp].level > 1 ) {
		TEST_ASSERT_EQ( can_interpret( ch, cmd_wizhelp ), 0 );
	}
	free_test_char( ch );
}

void test_interp_npc_basic( void ) {
	CHAR_DATA *ch = make_test_npc();
	ch->level = 5;
	ch->position = POS_STANDING;

	TEST_ASSERT_TRUE( cmd_north >= 0 );
	TEST_ASSERT_EQ( can_interpret( ch, cmd_north ), 1 );
	free_test_char( ch );
}

void test_interp_look_while_resting( void ) {
	CHAR_DATA *ch = make_test_player();
	ch->level = 1;

	if ( cmd_look >= 0 ) {
		/* "look" should be usable while resting (low position requirement) */
		ch->position = POS_STUNNED;
		int result = can_interpret( ch, cmd_look );
		/* Should be allowed or position-denied based on look's min position */
		if ( cmd_table[cmd_look].position <= POS_STUNNED )
			TEST_ASSERT_EQ( result, 1 );
		else
			TEST_ASSERT_EQ( result, -1 );
	}
	free_test_char( ch );
}

void test_interp_race_restricted_cmd( void ) {
	/* Find a race-restricted command (race > 0, discipline == 0) */
	int cmd;
	int race_cmd = -1;
	for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ ) {
		if ( cmd_table[cmd].race > 0 && cmd_table[cmd].discipline == 0
		     && cmd_table[cmd].level <= 1 ) {
			race_cmd = cmd;
			break;
		}
	}

	if ( race_cmd >= 0 ) {
		/* Wrong class should be denied */
		CHAR_DATA *ch = make_test_player();
		ch->level = 1;
		ch->position = POS_STANDING;
		ch->class = 0; /* No class */
		TEST_ASSERT_EQ( can_interpret( ch, race_cmd ), 0 );

		/* Right class should be allowed */
		ch->class = cmd_table[race_cmd].race;
		TEST_ASSERT_EQ( can_interpret( ch, race_cmd ), 1 );
		free_test_char( ch );
	}
}

void test_interp_discipline_restricted_cmd( void ) {
	/* Find a discipline-restricted command */
	int cmd;
	int disc_cmd = -1;
	for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ ) {
		if ( cmd_table[cmd].discipline > 0 && cmd_table[cmd].disclevel > 0
		     && cmd_table[cmd].level <= 1 ) {
			disc_cmd = cmd;
			break;
		}
	}

	if ( disc_cmd >= 0 ) {
		CHAR_DATA *ch = make_test_player();
		ch->level = 1;
		ch->position = POS_STANDING;
		ch->class = cmd_table[disc_cmd].race;

		/* Insufficient discipline level */
		ch->pcdata->power[cmd_table[disc_cmd].discipline] = cmd_table[disc_cmd].disclevel - 1;
		TEST_ASSERT_EQ( can_interpret( ch, disc_cmd ), 0 );

		/* Sufficient discipline level */
		ch->pcdata->power[cmd_table[disc_cmd].discipline] = cmd_table[disc_cmd].disclevel;
		TEST_ASSERT_EQ( can_interpret( ch, disc_cmd ), 1 );
		free_test_char( ch );
	}
}

/* --- Suite registration --- */

void suite_interp( void ) {
	find_test_commands();
	RUN_TEST( test_interp_basic_allowed );
	RUN_TEST( test_interp_level_denied );
	RUN_TEST( test_interp_imm_allowed );
	RUN_TEST( test_interp_wrong_position );
	RUN_TEST( test_interp_dead_pos_inventory );
	RUN_TEST( test_interp_npc_level_denied );
	RUN_TEST( test_interp_npc_basic );
	RUN_TEST( test_interp_look_while_resting );
	RUN_TEST( test_interp_race_restricted_cmd );
	RUN_TEST( test_interp_discipline_restricted_cmd );
}
