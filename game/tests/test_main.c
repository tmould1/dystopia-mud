/*
 * Dystopia MUD Test Runner
 *
 * Runs all unit test suites. Links against game object files with
 * TEST_BUILD defined to exclude the production main().
 */

#include "test_framework.h"
#include "test_helpers.h"

/* Define the global test counters (declared extern in test_framework.h) */
int test_passes = 0;
int test_failures = 0;
int test_total = 0;
const char *current_test_name = "";

int test_summary( void ) {
	printf( "\n========================================\n" );
	printf( "Results: %d passed, %d failed, %d total\n", test_passes, test_failures, test_total );
	printf( "========================================\n" );
	return test_failures > 0 ? 1 : 0;
}

/* Existing suite declarations */
extern void suite_dice( void );
extern void suite_strings( void );
extern void suite_stats( void );
extern void suite_list( void );
extern void suite_boot( void );

/* New suite declarations */
extern void suite_affects( void );
extern void suite_isname( void );
extern void suite_interp( void );
extern void suite_handler( void );
extern void suite_create_obj( void );
extern void suite_combat( void );
extern void suite_scripting( void );

int main( int argc, char **argv ) {
	(void) argc;
	(void) argv;

	printf( "Dystopia MUD - Unit Test Runner\n" );
	printf( "========================================\n" );

	/* Tier 1: No boot required */
	RUN_SUITE( "Dice & Math", suite_dice );
	RUN_SUITE( "String Utilities", suite_strings );
	RUN_SUITE( "Character Stats", suite_stats );
	RUN_SUITE( "Intrusive List", suite_list );
	RUN_SUITE( "Affect System", suite_affects );
	RUN_SUITE( "Name Matching", suite_isname );
	RUN_SUITE( "Command Interpreter", suite_interp );

	/* Tier 2: Boot required (boot suite runs first to initialize) */
	RUN_SUITE( "Game Boot", suite_boot );
	RUN_SUITE( "Character/Object Movement", suite_handler );
	RUN_SUITE( "Object Creation", suite_create_obj );
	RUN_SUITE( "Combat Engagement", suite_combat );
	RUN_SUITE( "Lua Scripting", suite_scripting );

	return test_summary();
}
