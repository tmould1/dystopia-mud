/*
 * Minimal unit test framework for Dystopia MUD
 *
 * Provides assertion macros and a test runner structure.
 * No external dependencies beyond standard C.
 *
 * Counters are defined in test_main.c and shared across all test files.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

/* Global test counters (defined in test_main.c) */
extern int test_passes;
extern int test_failures;
extern int test_total;
extern const char *current_test_name;

/* --- Assertion macros --- */

#define TEST_ASSERT( cond ) do { \
	test_total++; \
	if ( !( cond ) ) { \
		fprintf( stderr, "    FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond ); \
		test_failures++; \
	} else { \
		test_passes++; \
	} \
} while ( 0 )

#define TEST_ASSERT_EQ( a, b ) do { \
	int _a = (int)( a ); \
	int _b = (int)( b ); \
	test_total++; \
	if ( _a != _b ) { \
		fprintf( stderr, "    FAIL: %s:%d: %s == %s (got %d, expected %d)\n", \
			__FILE__, __LINE__, #a, #b, _a, _b ); \
		test_failures++; \
	} else { \
		test_passes++; \
	} \
} while ( 0 )

#define TEST_ASSERT_NEQ( a, b ) do { \
	int _a = (int)( a ); \
	int _b = (int)( b ); \
	test_total++; \
	if ( _a == _b ) { \
		fprintf( stderr, "    FAIL: %s:%d: %s != %s (both %d)\n", \
			__FILE__, __LINE__, #a, #b, _a ); \
		test_failures++; \
	} else { \
		test_passes++; \
	} \
} while ( 0 )

#define TEST_ASSERT_TRUE( cond )  TEST_ASSERT( cond )
#define TEST_ASSERT_FALSE( cond ) TEST_ASSERT( !( cond ) )

#define TEST_ASSERT_STR_EQ( a, b ) do { \
	const char *_a = ( a ); \
	const char *_b = ( b ); \
	test_total++; \
	if ( _a == NULL || _b == NULL || strcmp( _a, _b ) != 0 ) { \
		fprintf( stderr, "    FAIL: %s:%d: %s == %s (got \"%s\", expected \"%s\")\n", \
			__FILE__, __LINE__, #a, #b, _a ? _a : "(null)", _b ? _b : "(null)" ); \
		test_failures++; \
	} else { \
		test_passes++; \
	} \
} while ( 0 )

/* Assert that a value is in range [lo, hi] inclusive */
#define TEST_ASSERT_RANGE( val, lo, hi ) do { \
	int _v = (int)( val ); \
	int _lo = (int)( lo ); \
	int _hi = (int)( hi ); \
	test_total++; \
	if ( _v < _lo || _v > _hi ) { \
		fprintf( stderr, "    FAIL: %s:%d: %s in [%d..%d] (got %d)\n", \
			__FILE__, __LINE__, #val, _lo, _hi, _v ); \
		test_failures++; \
	} else { \
		test_passes++; \
	} \
} while ( 0 )

/* --- Test runner macros --- */

#define RUN_TEST( fn ) do { \
	int _before = test_failures; \
	current_test_name = #fn; \
	fn(); \
	if ( test_failures == _before ) \
		printf( "  PASS: %s\n", #fn ); \
	else \
		printf( "  FAIL: %s (%d failures)\n", #fn, test_failures - _before ); \
} while ( 0 )

#define RUN_SUITE( name, fn ) do { \
	printf( "\n--- %s ---\n", name ); \
	fn(); \
} while ( 0 )

/* Call at end of main() to print summary and return exit code */
int test_summary( void );

#endif /* TEST_FRAMEWORK_H */
