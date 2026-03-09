/*
 * Player database persistence tests for Dystopia MUD
 *
 * Tests save/load round-trip, backup recovery on corruption,
 * and graceful failure when both primary and backup are corrupt.
 *
 * Tier 2: Requires boot (for player DB paths, object prototypes, etc.)
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "merc.h"
#include "../db/db_player.h"
#include "../db/db_quest.h"

extern char mud_db_dir[MUD_PATH_MAX];

/* Test player name — unique to avoid conflicts with real players */
#define TEST_PLAYER_NAME "Zzztestrecov"

/*
 * Create a minimal mock descriptor for db_player_load.
 */
static DESCRIPTOR_DATA *make_mock_descriptor( void ) {
	DESCRIPTOR_DATA *d = calloc( 1, sizeof( *d ) );
	d->descriptor = -1;
	d->connected = CON_GET_NAME;
	d->outsize = 2000;
	d->outbuf = calloc( 1, d->outsize );
	list_node_init( &d->node );
	return d;
}

/*
 * Free a mock descriptor and its loaded character (if any).
 */
static void free_mock_descriptor( DESCRIPTOR_DATA *d ) {
	if ( d == NULL ) return;
	if ( d->character ) {
		free_char( d->character );
		d->character = NULL;
	}
	if ( d->outbuf ) free( d->outbuf );
	free( d );
}

/*
 * Create a save-compatible test character (level >= 2, pcdata, switchname).
 * clear_char() initializes ch strings; pcdata strings default to NULL
 * which safe_str() in the save code handles as empty.
 */
static CHAR_DATA *make_saveable_player( void ) {
	CHAR_DATA *ch = calloc( 1, sizeof( *ch ) );
	clear_char( ch );
	ch->pcdata = calloc( 1, sizeof( *ch->pcdata ) );
	list_init( &ch->pcdata->aliases );
	ch->pcdata->quest_tracker = quest_tracker_new();

	/* Overwrite the empty name from clear_char */
	free( ch->name );
	ch->name = str_dup( TEST_PLAYER_NAME );
	ch->pcdata->switchname = str_dup( TEST_PLAYER_NAME );
	ch->clan = str_dup( "" );

	ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
	ch->level = 3;
	ch->max_hit = 500;
	ch->hit = 500;
	ch->max_mana = 200;
	ch->mana = 200;
	ch->max_move = 300;
	ch->move = 300;
	ch->gold = 42;
	ch->alignment = 100;

	return ch;
}

/*
 * Build path to the test player's primary .db file.
 */
static void get_player_path( char *buf, size_t bufsize ) {
	snprintf( buf, bufsize, "%s%splayers%s%s.db",
		mud_db_dir, PATH_SEPARATOR, PATH_SEPARATOR, TEST_PLAYER_NAME );
}

/*
 * Build path to the test player's backup .db file.
 */
static void get_backup_path( char *buf, size_t bufsize ) {
	snprintf( buf, bufsize, "%s%splayers%sbackup%s%s.db",
		mud_db_dir, PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR,
		TEST_PLAYER_NAME );
}

/*
 * Delete the test player's primary and backup files.
 */
static void cleanup_test_files( void ) {
	char path[MUD_PATH_MAX];

	get_player_path( path, sizeof( path ) );
	remove( path );

	get_backup_path( path, sizeof( path ) );
	remove( path );
}

/*
 * Corrupt a file by truncating it to 10 bytes of garbage.
 */
static void corrupt_file( const char *path ) {
	FILE *fp = fopen( path, "wb" );
	if ( fp ) {
		fwrite( "CORRUPT!!!", 1, 10, fp );
		fclose( fp );
	}
}

/*
 * Copy a file byte-for-byte (for creating backup).
 */
static void test_copy_file( const char *src, const char *dst ) {
	FILE *fin = fopen( src, "rb" );
	FILE *fout;
	char buf[4096];
	size_t n;

	if ( fin == NULL ) return;
	fout = fopen( dst, "wb" );
	if ( fout == NULL ) { fclose( fin ); return; }

	while ( ( n = fread( buf, 1, sizeof( buf ), fin ) ) > 0 )
		fwrite( buf, 1, n, fout );

	fclose( fin );
	fclose( fout );
}

/*--------------------------------------------------------------------------
 * Test: save + load round-trip preserves key fields
 *--------------------------------------------------------------------------*/

static void test_player_save_load_roundtrip( void ) {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;
	bool loaded;

	ensure_booted();
	cleanup_test_files();

	/* Save a test character */
	ch = make_saveable_player();
	db_player_save( ch );
	db_player_wait_pending();
	free_char( ch );

	/* Verify file was created */
	TEST_ASSERT_TRUE( db_player_exists( TEST_PLAYER_NAME ) );

	/* Load it back */
	d = make_mock_descriptor();
	loaded = db_player_load( d, TEST_PLAYER_NAME );
	TEST_ASSERT_TRUE( loaded );

	if ( loaded && d->character ) {
		TEST_ASSERT_STR_EQ( d->character->name, TEST_PLAYER_NAME );
		TEST_ASSERT_EQ( d->character->level, 3 );
		TEST_ASSERT_EQ( d->character->max_hit, 500 );
		TEST_ASSERT_EQ( d->character->gold, 42 );
		TEST_ASSERT_EQ( d->character->alignment, 100 );
	}

	free_mock_descriptor( d );
	cleanup_test_files();
}

/*--------------------------------------------------------------------------
 * Test: corrupt primary, valid backup -> recovery succeeds
 *--------------------------------------------------------------------------*/

static void test_player_backup_recovery( void ) {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;
	char primary[MUD_PATH_MAX];
	char backup[MUD_PATH_MAX];
	bool loaded;

	ensure_booted();
	cleanup_test_files();

	/* Save a test character (creates primary) */
	ch = make_saveable_player();
	db_player_save( ch );
	db_player_wait_pending();
	free_char( ch );

	/* Copy primary to backup location */
	get_player_path( primary, sizeof( primary ) );
	get_backup_path( backup, sizeof( backup ) );
	test_copy_file( primary, backup );

	/* Corrupt the primary */
	corrupt_file( primary );

	/* Load should succeed via backup fallback */
	d = make_mock_descriptor();
	loaded = db_player_load( d, TEST_PLAYER_NAME );
	TEST_ASSERT_TRUE( loaded );

	if ( loaded && d->character ) {
		TEST_ASSERT_STR_EQ( d->character->name, TEST_PLAYER_NAME );
		TEST_ASSERT_EQ( d->character->level, 3 );
		TEST_ASSERT_EQ( d->character->max_hit, 500 );
	}

	free_mock_descriptor( d );
	cleanup_test_files();
}

/*--------------------------------------------------------------------------
 * Test: corrupt primary, no backup -> graceful failure (not crash)
 *--------------------------------------------------------------------------*/

static void test_player_corrupt_no_backup_fails( void ) {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;
	char primary[MUD_PATH_MAX];
	bool loaded;

	ensure_booted();
	cleanup_test_files();

	/* Save a test character (creates primary, no backup) */
	ch = make_saveable_player();
	db_player_save( ch );
	db_player_wait_pending();
	free_char( ch );

	/* Corrupt the primary -- no backup exists */
	get_player_path( primary, sizeof( primary ) );
	corrupt_file( primary );

	/* Load should fail gracefully, not crash */
	d = make_mock_descriptor();
	loaded = db_player_load( d, TEST_PLAYER_NAME );
	TEST_ASSERT_FALSE( loaded );

	free_mock_descriptor( d );
	cleanup_test_files();
}

/*--------------------------------------------------------------------------
 * Test: both primary and backup corrupt -> graceful failure (not crash)
 *--------------------------------------------------------------------------*/

static void test_player_both_corrupt_fails( void ) {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;
	char primary[MUD_PATH_MAX];
	char backup[MUD_PATH_MAX];
	bool loaded;

	ensure_booted();
	cleanup_test_files();

	/* Save a test character */
	ch = make_saveable_player();
	db_player_save( ch );
	db_player_wait_pending();
	free_char( ch );

	/* Create a corrupt backup too */
	get_player_path( primary, sizeof( primary ) );
	get_backup_path( backup, sizeof( backup ) );
	test_copy_file( primary, backup );

	/* Corrupt both */
	corrupt_file( primary );
	corrupt_file( backup );

	/* Load should fail gracefully, not crash */
	d = make_mock_descriptor();
	loaded = db_player_load( d, TEST_PLAYER_NAME );
	TEST_ASSERT_FALSE( loaded );

	free_mock_descriptor( d );
	cleanup_test_files();
}

/*--------------------------------------------------------------------------
 * Test: nonexistent player -> returns FALSE (not crash)
 *--------------------------------------------------------------------------*/

static void test_player_load_nonexistent( void ) {
	DESCRIPTOR_DATA *d;
	bool loaded;

	ensure_booted();

	d = make_mock_descriptor();
	loaded = db_player_load( d, "Zzznobodyexists" );
	TEST_ASSERT_FALSE( loaded );

	free_mock_descriptor( d );
}

/*--------------------------------------------------------------------------
 * Test: db_player_exists returns FALSE for nonexistent, TRUE after save
 *--------------------------------------------------------------------------*/

static void test_player_exists_check( void ) {
	CHAR_DATA *ch;

	ensure_booted();
	cleanup_test_files();

	TEST_ASSERT_FALSE( db_player_exists( TEST_PLAYER_NAME ) );

	ch = make_saveable_player();
	db_player_save( ch );
	db_player_wait_pending();
	free_char( ch );

	TEST_ASSERT_TRUE( db_player_exists( TEST_PLAYER_NAME ) );

	cleanup_test_files();
}

/*--------------------------------------------------------------------------
 * Test: db_player_delete removes the file
 *--------------------------------------------------------------------------*/

static void test_player_delete( void ) {
	CHAR_DATA *ch;

	ensure_booted();
	cleanup_test_files();

	ch = make_saveable_player();
	db_player_save( ch );
	db_player_wait_pending();
	free_char( ch );

	TEST_ASSERT_TRUE( db_player_exists( TEST_PLAYER_NAME ) );
	TEST_ASSERT_TRUE( db_player_delete( TEST_PLAYER_NAME ) );
	TEST_ASSERT_FALSE( db_player_exists( TEST_PLAYER_NAME ) );

	cleanup_test_files();
}

/*--------------------------------------------------------------------------
 * Test: backup recovery restores primary file for future loads
 *--------------------------------------------------------------------------*/

static void test_player_backup_restores_primary( void ) {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;
	char primary[MUD_PATH_MAX];
	char backup[MUD_PATH_MAX];
	bool loaded;
	FILE *fp;

	ensure_booted();
	cleanup_test_files();

	/* Save, create backup, corrupt primary */
	ch = make_saveable_player();
	db_player_save( ch );
	db_player_wait_pending();
	free_char( ch );

	get_player_path( primary, sizeof( primary ) );
	get_backup_path( backup, sizeof( backup ) );
	test_copy_file( primary, backup );
	corrupt_file( primary );

	/* First load recovers from backup and should restore primary */
	d = make_mock_descriptor();
	loaded = db_player_load( d, TEST_PLAYER_NAME );
	TEST_ASSERT_TRUE( loaded );
	free_mock_descriptor( d );

	/* Verify primary was restored (file should be larger than 10 bytes) */
	fp = fopen( primary, "rb" );
	if ( fp ) {
		long size;
		fseek( fp, 0, SEEK_END );
		size = ftell( fp );
		fclose( fp );
		TEST_ASSERT_TRUE( size > 100 ); /* Valid DB is much larger than 10 */
	}

	/* Second load should work directly from restored primary */
	d = make_mock_descriptor();
	loaded = db_player_load( d, TEST_PLAYER_NAME );
	TEST_ASSERT_TRUE( loaded );

	if ( loaded && d->character )
		TEST_ASSERT_EQ( d->character->level, 3 );

	free_mock_descriptor( d );
	cleanup_test_files();
}

/*--------------------------------------------------------------------------
 * Suite registration
 *--------------------------------------------------------------------------*/

static void test_player_create_free_smoke( void ) {
	CHAR_DATA *ch;
	ensure_booted();
	ch = make_saveable_player();
	TEST_ASSERT_TRUE( ch != NULL );
	TEST_ASSERT_TRUE( ch->pcdata != NULL );
	TEST_ASSERT_STR_EQ( ch->name, TEST_PLAYER_NAME );
	free_char( ch );
	TEST_ASSERT_TRUE( TRUE ); /* survived free_char */
}

static void test_player_save_smoke( void ) {
	CHAR_DATA *ch;
	ensure_booted();
	cleanup_test_files();
	ch = make_saveable_player();
	db_player_save( ch );
	db_player_wait_pending();
	TEST_ASSERT_TRUE( db_player_exists( TEST_PLAYER_NAME ) );
	free_char( ch );
	cleanup_test_files();
}

void suite_db_player( void ) {
	RUN_TEST( test_player_create_free_smoke );
	RUN_TEST( test_player_save_smoke );
	RUN_TEST( test_player_exists_check );
	RUN_TEST( test_player_save_load_roundtrip );
	RUN_TEST( test_player_delete );
	RUN_TEST( test_player_load_nonexistent );
	RUN_TEST( test_player_backup_recovery );
	RUN_TEST( test_player_corrupt_no_backup_fails );
	RUN_TEST( test_player_both_corrupt_fails );
	RUN_TEST( test_player_backup_restores_primary );
}
