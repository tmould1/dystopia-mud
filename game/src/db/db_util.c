/***************************************************************************
 *  db_util.c - Shared SQLite utility helpers
 *
 *  Common functions used across all db modules.
 ***************************************************************************/

#include "db_util.h"

const char *col_text( sqlite3_stmt *stmt, int col ) {
	const char *val = (const char *)sqlite3_column_text( stmt, col );
	return val ? val : "";
}

void db_bind_text_or_null( sqlite3_stmt *stmt, int col, const char *val ) {
	if ( val && val[0] != '\0' )
		sqlite3_bind_text( stmt, col, val, -1, SQLITE_TRANSIENT );
	else
		sqlite3_bind_null( stmt, col );
}

const char *safe_str( const char *s ) {
	return ( s && s[0] != '\0' ) ? s : "";
}

void db_begin( sqlite3 *db ) {
	sqlite3_exec( db, "BEGIN TRANSACTION", NULL, NULL, NULL );
}

void db_commit( sqlite3 *db ) {
	sqlite3_exec( db, "COMMIT", NULL, NULL, NULL );
}

void db_rollback( sqlite3 *db ) {
	sqlite3_exec( db, "ROLLBACK", NULL, NULL, NULL );
}
