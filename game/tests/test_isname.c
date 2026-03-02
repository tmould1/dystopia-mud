/*
 * Name matching tests for Dystopia MUD
 *
 * Tests is_name() and is_full_name() from handler.c.
 * These are pure string functions with no boot dependency.
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"

/* --- is_name tests --- */

void test_isname_exact_match( void ) {
	char str[] = "sword";
	char names[] = "sword";
	TEST_ASSERT_TRUE( is_name( str, names ) );
}

void test_isname_prefix_match( void ) {
	char str[] = "swo";
	char names[] = "sword";
	TEST_ASSERT_TRUE( is_name( str, names ) );
}

void test_isname_multi_word_namelist( void ) {
	char str[] = "sword";
	char names[] = "a sharp sword";
	TEST_ASSERT_TRUE( is_name( str, names ) );
}

void test_isname_no_match( void ) {
	char str[] = "axe";
	char names[] = "sword shield";
	TEST_ASSERT_FALSE( is_name( str, names ) );
}

void test_isname_empty_str( void ) {
	char str[] = "";
	char names[] = "sword";
	TEST_ASSERT_FALSE( is_name( str, names ) );
}

void test_isname_null_namelist( void ) {
	char str[] = "sword";
	TEST_ASSERT_FALSE( is_name( str, NULL ) );
}

void test_isname_empty_namelist( void ) {
	char str[] = "sword";
	char names[] = "";
	TEST_ASSERT_FALSE( is_name( str, names ) );
}

void test_isname_case_insensitive( void ) {
	char str[] = "Sword";
	char names[] = "sword";
	TEST_ASSERT_TRUE( is_name( str, names ) );
}

void test_isname_short_prefix( void ) {
	char str[] = "s";
	char names[] = "sword";
	TEST_ASSERT_TRUE( is_name( str, names ) );
}

void test_isname_multi_word_query( void ) {
	char str[] = "sharp sword";
	char names[] = "a sharp sword";
	TEST_ASSERT_TRUE( is_name( str, names ) );
}

void test_isname_multi_word_query_fail( void ) {
	char str[] = "sharp axe";
	char names[] = "a sharp sword";
	TEST_ASSERT_FALSE( is_name( str, names ) );
}

void test_isname_first_char_no_match( void ) {
	char str[] = "z";
	char names[] = "sword shield";
	TEST_ASSERT_FALSE( is_name( str, names ) );
}

/* --- is_full_name tests --- */

void test_is_full_name_exact( void ) {
	char names[] = "a sharp sword";
	TEST_ASSERT_TRUE( is_full_name( "sword", names ) );
}

void test_is_full_name_prefix_fail( void ) {
	char names[] = "sword";
	TEST_ASSERT_FALSE( is_full_name( "swo", names ) );
}

/* --- Suite registration --- */

void suite_isname( void ) {
	RUN_TEST( test_isname_exact_match );
	RUN_TEST( test_isname_prefix_match );
	RUN_TEST( test_isname_multi_word_namelist );
	RUN_TEST( test_isname_no_match );
	RUN_TEST( test_isname_empty_str );
	RUN_TEST( test_isname_null_namelist );
	RUN_TEST( test_isname_empty_namelist );
	RUN_TEST( test_isname_case_insensitive );
	RUN_TEST( test_isname_short_prefix );
	RUN_TEST( test_isname_multi_word_query );
	RUN_TEST( test_isname_multi_word_query_fail );
	RUN_TEST( test_isname_first_char_no_match );
	RUN_TEST( test_is_full_name_exact );
	RUN_TEST( test_is_full_name_prefix_fail );
}
