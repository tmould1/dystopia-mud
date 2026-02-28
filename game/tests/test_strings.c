/*
 * Unit tests for string utility functions (game/src/core/db.c, interp.c)
 *
 * Tests: str_cmp(), str_prefix(), one_argument()
 *
 * str_cmp/str_prefix return 0 for match, nonzero for mismatch
 * (standard C convention, like strcmp/strcasecmp).
 */

#include "test_framework.h"
#include "test_helpers.h"

/* --- str_cmp() tests --- */

void test_str_cmp_equal( void ) {
	/* Same string: should return 0 (match) */
	TEST_ASSERT_EQ( str_cmp( "hello", "hello" ), 0 );
}

void test_str_cmp_case_insensitive( void ) {
	/* Case-insensitive match: should return 0 */
	TEST_ASSERT_EQ( str_cmp( "Hello", "hello" ), 0 );
	TEST_ASSERT_EQ( str_cmp( "HELLO", "hello" ), 0 );
	TEST_ASSERT_EQ( str_cmp( "hElLo", "HeLlO" ), 0 );
}

void test_str_cmp_different( void ) {
	/* Different strings: should return nonzero */
	TEST_ASSERT_NEQ( str_cmp( "hello", "world" ), 0 );
	TEST_ASSERT_NEQ( str_cmp( "hello", "hell" ), 0 );
	TEST_ASSERT_NEQ( str_cmp( "hell", "hello" ), 0 );
}

void test_str_cmp_empty( void ) {
	/* Empty strings match each other */
	TEST_ASSERT_EQ( str_cmp( "", "" ), 0 );

	/* Empty vs non-empty: mismatch */
	TEST_ASSERT_NEQ( str_cmp( "", "hello" ), 0 );
	TEST_ASSERT_NEQ( str_cmp( "hello", "" ), 0 );
}

/* --- str_prefix() tests --- */

void test_str_prefix_match( void ) {
	/* "hel" is a prefix of "hello": should return 0 (match) */
	TEST_ASSERT_EQ( str_prefix( "hel", "hello" ), 0 );
}

void test_str_prefix_case_insensitive( void ) {
	/* Case-insensitive prefix match */
	TEST_ASSERT_EQ( str_prefix( "HEL", "hello" ), 0 );
	TEST_ASSERT_EQ( str_prefix( "Hel", "HELLO" ), 0 );
}

void test_str_prefix_exact( void ) {
	/* Exact match is also a valid prefix */
	TEST_ASSERT_EQ( str_prefix( "hello", "hello" ), 0 );
}

void test_str_prefix_no_match( void ) {
	/* Not a prefix: should return nonzero */
	TEST_ASSERT_NEQ( str_prefix( "world", "hello" ), 0 );
}

void test_str_prefix_longer( void ) {
	/* Prefix longer than string: should fail when astr has chars left
	 * but bstr runs out */
	TEST_ASSERT_NEQ( str_prefix( "hello world", "hello" ), 0 );
}

void test_str_prefix_empty( void ) {
	/* Empty prefix matches anything (loop body never executes) */
	TEST_ASSERT_EQ( str_prefix( "", "hello" ), 0 );
	TEST_ASSERT_EQ( str_prefix( "", "" ), 0 );
}

/* --- one_argument() tests --- */

void test_one_argument_simple( void ) {
	char arg[256];
	char *rest;

	rest = one_argument( "hello world", arg );
	TEST_ASSERT_STR_EQ( arg, "hello" );
	/* rest should point to "world" (after skipping spaces) */
	TEST_ASSERT_STR_EQ( rest, "world" );
}

void test_one_argument_quoted( void ) {
	char arg[256];
	char *rest;

	/* Single-quoted argument */
	rest = one_argument( "'hello world' foo", arg );
	TEST_ASSERT_STR_EQ( arg, "hello world" );
	TEST_ASSERT_STR_EQ( rest, "foo" );
}

void test_one_argument_double_quoted( void ) {
	char arg[256];
	char *rest;

	/* Double-quoted argument */
	rest = one_argument( "\"hello world\" bar", arg );
	TEST_ASSERT_STR_EQ( arg, "hello world" );
	TEST_ASSERT_STR_EQ( rest, "bar" );
}

void test_one_argument_lowercases( void ) {
	char arg[256];

	/* one_argument lowercases the parsed argument */
	one_argument( "HELLO", arg );
	TEST_ASSERT_STR_EQ( arg, "hello" );
}

void test_one_argument_leading_spaces( void ) {
	char arg[256];

	/* Leading spaces are skipped */
	one_argument( "   hello", arg );
	TEST_ASSERT_STR_EQ( arg, "hello" );
}

void test_one_argument_empty( void ) {
	char arg[256];

	/* Empty input */
	one_argument( "", arg );
	TEST_ASSERT_STR_EQ( arg, "" );
}

void test_one_argument_numbered_prefix( void ) {
	char arg[256];

	/* N.keyword pattern (e.g., "2.sword") */
	one_argument( "2.sword rest", arg );
	TEST_ASSERT_STR_EQ( arg, "2.sword" );
}

/* --- Test suite --- */

void suite_strings( void ) {
	RUN_TEST( test_str_cmp_equal );
	RUN_TEST( test_str_cmp_case_insensitive );
	RUN_TEST( test_str_cmp_different );
	RUN_TEST( test_str_cmp_empty );
	RUN_TEST( test_str_prefix_match );
	RUN_TEST( test_str_prefix_case_insensitive );
	RUN_TEST( test_str_prefix_exact );
	RUN_TEST( test_str_prefix_no_match );
	RUN_TEST( test_str_prefix_longer );
	RUN_TEST( test_str_prefix_empty );
	RUN_TEST( test_one_argument_simple );
	RUN_TEST( test_one_argument_quoted );
	RUN_TEST( test_one_argument_double_quoted );
	RUN_TEST( test_one_argument_lowercases );
	RUN_TEST( test_one_argument_leading_spaces );
	RUN_TEST( test_one_argument_empty );
	RUN_TEST( test_one_argument_numbered_prefix );
}
