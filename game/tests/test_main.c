/*
 * Dystopia MUD Test Runner
 *
 * Runs all unit test suites without booting the game database.
 * Links against game object files with TEST_BUILD defined to
 * exclude the production main().
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

/* Suite declarations */
extern void suite_dice( void );
extern void suite_strings( void );
extern void suite_stats( void );

int main( int argc, char **argv ) {
	(void) argc;
	(void) argv;

	printf( "Dystopia MUD - Unit Test Runner\n" );
	printf( "========================================\n" );

	RUN_SUITE( "Dice & Math", suite_dice );
	RUN_SUITE( "String Utilities", suite_strings );
	RUN_SUITE( "Character Stats", suite_stats );

	return test_summary();
}
