/*
 * Unit tests for RNG and math functions (game/src/core/db.c)
 *
 * Tests: dice(), number_range(), number_percent(), number_bits(), interpolate()
 */

#include "test_framework.h"
#include "test_helpers.h"

/* --- interpolate() tests --- */

void test_interpolate_boundaries( void ) {
	/* interpolate(0, v00, v32) should return v00 */
	TEST_ASSERT_EQ( interpolate( 0, 100, 200 ), 100 );

	/* interpolate(32, v00, v32) should return v32 */
	TEST_ASSERT_EQ( interpolate( 32, 100, 200 ), 200 );
}

void test_interpolate_midpoint( void ) {
	/* interpolate(16, 0, 320) = 0 + 16 * 320 / 32 = 160 */
	TEST_ASSERT_EQ( interpolate( 16, 0, 320 ), 160 );

	/* interpolate(16, 100, 200) = 100 + 16 * 100 / 32 = 150 */
	TEST_ASSERT_EQ( interpolate( 16, 100, 200 ), 150 );
}

void test_interpolate_negative_range( void ) {
	/* Descending range: interpolate(16, 200, 100) = 200 + 16 * -100 / 32 = 150 */
	TEST_ASSERT_EQ( interpolate( 16, 200, 100 ), 150 );
}

void test_interpolate_zero_range( void ) {
	/* Same values: should return the value at any level */
	TEST_ASSERT_EQ( interpolate( 0, 50, 50 ), 50 );
	TEST_ASSERT_EQ( interpolate( 16, 50, 50 ), 50 );
	TEST_ASSERT_EQ( interpolate( 32, 50, 50 ), 50 );
}

/* --- RNG determinism tests --- */

void test_rng_deterministic( void ) {
	int results1[10], results2[10];
	int i;

	/* Seed with same value, get same sequence */
	seed_rng( 42 );
	for ( i = 0; i < 10; i++ )
		results1[i] = number_range( 1, 100 );

	seed_rng( 42 );
	for ( i = 0; i < 10; i++ )
		results2[i] = number_range( 1, 100 );

	for ( i = 0; i < 10; i++ )
		TEST_ASSERT_EQ( results1[i], results2[i] );
}

void test_rng_different_seeds( void ) {
	int r1, r2;

	seed_rng( 1 );
	r1 = number_range( 1, 1000000 );

	seed_rng( 999 );
	r2 = number_range( 1, 1000000 );

	/* Extremely unlikely to be equal with different seeds */
	TEST_ASSERT_NEQ( r1, r2 );
}

/* --- number_range() tests --- */

void test_number_range_bounds( void ) {
	int i, val;

	seed_rng( 100 );
	for ( i = 0; i < 1000; i++ ) {
		val = number_range( 5, 10 );
		TEST_ASSERT( val >= 5 && val <= 10 );
	}
}

void test_number_range_single_value( void ) {
	int i;

	seed_rng( 200 );
	for ( i = 0; i < 100; i++ ) {
		/* When from == to, should always return that value */
		TEST_ASSERT_EQ( number_range( 7, 7 ), 7 );
	}
}

void test_number_range_reversed( void ) {
	/* When to < from, (to - from + 1) <= 1, so returns from */
	TEST_ASSERT_EQ( number_range( 10, 5 ), 10 );
}

/* --- number_percent() tests --- */

void test_number_percent_bounds( void ) {
	int i, val;

	seed_rng( 300 );
	for ( i = 0; i < 1000; i++ ) {
		val = number_percent();
		TEST_ASSERT( val >= 1 && val <= 100 );
	}
}

/* --- number_bits() tests --- */

void test_number_bits_bounds( void ) {
	int i, val;

	seed_rng( 400 );
	/* 3 bits: should be 0-7 */
	for ( i = 0; i < 200; i++ ) {
		val = number_bits( 3 );
		TEST_ASSERT( val >= 0 && val <= 7 );
	}

	/* 1 bit: should be 0 or 1 */
	for ( i = 0; i < 200; i++ ) {
		val = number_bits( 1 );
		TEST_ASSERT( val >= 0 && val <= 1 );
	}
}

void test_number_bits_zero( void ) {
	/* 0 bits: mask is (1 << 0) - 1 = 0, so always 0 */
	seed_rng( 500 );
	TEST_ASSERT_EQ( number_bits( 0 ), 0 );
}

/* --- dice() tests --- */

void test_dice_basic( void ) {
	int val;

	seed_rng( 600 );
	/* 1d6: should be 1-6 */
	val = dice( 1, 6 );
	TEST_ASSERT( val >= 1 && val <= 6 );
}

void test_dice_bounds( void ) {
	int i, val;

	seed_rng( 700 );
	/* 2d6: should be 2-12 */
	for ( i = 0; i < 500; i++ ) {
		val = dice( 2, 6 );
		TEST_ASSERT( val >= 2 && val <= 12 );
	}
}

void test_dice_zero_size( void ) {
	/* size 0: should return 0 */
	TEST_ASSERT_EQ( dice( 5, 0 ), 0 );
}

void test_dice_size_one( void ) {
	/* size 1: Nd1 should return N */
	TEST_ASSERT_EQ( dice( 3, 1 ), 3 );
	TEST_ASSERT_EQ( dice( 1, 1 ), 1 );
	TEST_ASSERT_EQ( dice( 10, 1 ), 10 );
}

void test_dice_distribution( void ) {
	int counts[13]; /* index 2-12 for 2d6 */
	int i, val;
	int has_variety = 0;

	memset( counts, 0, sizeof( counts ) );

	seed_rng( 800 );
	for ( i = 0; i < 10000; i++ ) {
		val = dice( 2, 6 );
		if ( val >= 2 && val <= 12 )
			counts[val]++;
	}

	/* Check that we get some variety - at least 5 different values */
	for ( i = 2; i <= 12; i++ ) {
		if ( counts[i] > 0 )
			has_variety++;
	}
	TEST_ASSERT( has_variety >= 5 );
}

/* --- Test suite --- */

void suite_dice( void ) {
	RUN_TEST( test_interpolate_boundaries );
	RUN_TEST( test_interpolate_midpoint );
	RUN_TEST( test_interpolate_negative_range );
	RUN_TEST( test_interpolate_zero_range );
	RUN_TEST( test_rng_deterministic );
	RUN_TEST( test_rng_different_seeds );
	RUN_TEST( test_number_range_bounds );
	RUN_TEST( test_number_range_single_value );
	RUN_TEST( test_number_range_reversed );
	RUN_TEST( test_number_percent_bounds );
	RUN_TEST( test_number_bits_bounds );
	RUN_TEST( test_number_bits_zero );
	RUN_TEST( test_dice_basic );
	RUN_TEST( test_dice_bounds );
	RUN_TEST( test_dice_zero_size );
	RUN_TEST( test_dice_size_one );
	RUN_TEST( test_dice_distribution );
}
