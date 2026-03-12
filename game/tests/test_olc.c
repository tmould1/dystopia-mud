/*
 * Unit tests for OLC (Online Creation) systems
 *
 * Tests:
 * - OLC command table sentinel consistency (hedit crash fix)
 * - string_add() truncation check correctness (uninitialized buffer fix)
 *
 * Tier 1 tests (no boot): table sentinel checks
 * Tier 2 tests (boot required): string_add with descriptor
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"
#include "../world/olc.h"

/*--------------------------------------------------------------------------
 * OLC command table sentinels: every table must end with "" not NULL.
 * show_olc_cmds() iterates with name[0] != '\0', so NULL sentinel crashes.
 *--------------------------------------------------------------------------*/

static void test_hedit_table_sentinel_not_null( void ) {
	/* Verify the sentinel is a non-NULL empty string (not NULL).
	 * show_olc_cmds() uses name[0] != '\0', so NULL would crash. */
	int cmd;
	for ( cmd = 0; hedit_table[cmd].name[0] != '\0'; cmd++ )
		;
	/* The sentinel name pointer itself must not be NULL */
	TEST_ASSERT_TRUE( hedit_table[cmd].name != NULL );
}

static void test_hedit_table_sentinel_empty( void ) {
	int cmd;
	for ( cmd = 0; hedit_table[cmd].name != NULL && hedit_table[cmd].name[0] != '\0'; cmd++ )
		;
	/* Should have stopped at empty string sentinel */
	TEST_ASSERT_TRUE( hedit_table[cmd].name != NULL );
	TEST_ASSERT_EQ( hedit_table[cmd].name[0], '\0' );
}

static void test_aedit_table_sentinel( void ) {
	int cmd;
	for ( cmd = 0; aedit_table[cmd].name[0] != '\0'; cmd++ )
		;
	TEST_ASSERT_EQ( aedit_table[cmd].name[0], '\0' );
}

static void test_redit_table_sentinel( void ) {
	int cmd;
	for ( cmd = 0; redit_table[cmd].name[0] != '\0'; cmd++ )
		;
	TEST_ASSERT_EQ( redit_table[cmd].name[0], '\0' );
}

static void test_oedit_table_sentinel( void ) {
	int cmd;
	for ( cmd = 0; oedit_table[cmd].name[0] != '\0'; cmd++ )
		;
	TEST_ASSERT_EQ( oedit_table[cmd].name[0], '\0' );
}

static void test_medit_table_sentinel( void ) {
	int cmd;
	for ( cmd = 0; medit_table[cmd].name[0] != '\0'; cmd++ )
		;
	TEST_ASSERT_EQ( medit_table[cmd].name[0], '\0' );
}

/*--------------------------------------------------------------------------
 * string_add: appending text to pString
 *
 * Verifies that string_add correctly appends text and uses the actual
 * pString length (not an uninitialized buffer) for truncation checks.
 *--------------------------------------------------------------------------*/

static void test_string_add_appends_text( void ) {
	ensure_booted();

	CHAR_DATA *ch = make_full_test_npc();
	ch->act = 0; /* player */

	/* Set up a mock descriptor with pString */
	DESCRIPTOR_DATA desc;
	memset( &desc, 0, sizeof( desc ) );
	desc.descriptor = -1;
	ch->desc = &desc;

	char *edit_string = str_dup( "" );
	desc.pString = &edit_string;

	test_output_start( ch );

	/* Add a line of text */
	string_add( ch, "Hello world" );

	test_output_stop();

	/* pString should now contain "Hello world\n\r" */
	TEST_ASSERT_TRUE( desc.pString != NULL );
	TEST_ASSERT_STR_EQ( edit_string, "Hello world\n\r" );

	free( edit_string );
	ch->desc = NULL;
	free_char( ch );
}

static void test_string_add_multiple_lines( void ) {
	ensure_booted();

	CHAR_DATA *ch = make_full_test_npc();
	ch->act = 0;

	DESCRIPTOR_DATA desc;
	memset( &desc, 0, sizeof( desc ) );
	desc.descriptor = -1;
	ch->desc = &desc;

	char *edit_string = str_dup( "" );
	desc.pString = &edit_string;

	test_output_start( ch );

	string_add( ch, "Line one" );
	string_add( ch, "Line two" );

	test_output_stop();

	TEST_ASSERT_STR_EQ( edit_string, "Line one\n\rLine two\n\r" );

	free( edit_string );
	ch->desc = NULL;
	free_char( ch );
}

static void test_string_add_at_exits( void ) {
	ensure_booted();

	CHAR_DATA *ch = make_full_test_npc();
	ch->act = 0;

	DESCRIPTOR_DATA desc;
	memset( &desc, 0, sizeof( desc ) );
	desc.descriptor = -1;
	ch->desc = &desc;

	char *edit_string = str_dup( "existing text\n\r" );
	desc.pString = &edit_string;

	test_output_start( ch );

	/* @ should exit editing mode (set pString to NULL) */
	string_add( ch, "@" );

	test_output_stop();

	TEST_ASSERT_TRUE( desc.pString == NULL );

	free( edit_string );
	ch->desc = NULL;
	free_char( ch );
}

/*--------------------------------------------------------------------------
 * Suite registration
 *--------------------------------------------------------------------------*/

void suite_olc( void ) {
	/* Tier 1: No boot needed - static table checks */
	RUN_TEST( test_hedit_table_sentinel_not_null );
	RUN_TEST( test_hedit_table_sentinel_empty );
	RUN_TEST( test_aedit_table_sentinel );
	RUN_TEST( test_redit_table_sentinel );
	RUN_TEST( test_oedit_table_sentinel );
	RUN_TEST( test_medit_table_sentinel );

	/* Tier 2: Boot needed - string_add tests */
	RUN_TEST( test_string_add_appends_text );
	RUN_TEST( test_string_add_multiple_lines );
	RUN_TEST( test_string_add_at_exits );
}
