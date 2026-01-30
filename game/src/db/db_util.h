/***************************************************************************
 *  db_util.h - Shared SQLite utility helpers
 *
 *  Common functions and macros used across all db modules (db_sql.c,
 *  db_game.c, db_player.c).
 *
 *  IMPORTANT: sqlite3.h must be included BEFORE this header (and before
 *  merc.h) due to N/Z macro conflicts.
 ***************************************************************************/

#ifndef DB_UTIL_H
#define DB_UTIL_H

#include "sqlite3.h"
#include "../core/merc.h"
#undef N    /* merc.h: #define N 8192  - conflicts with sqlite3 API */
#undef Z    /* merc.h: #define Z 33554432 - conflicts with sqlite3 API */

/*
 * Return the text value of a column, or "" if NULL.
 */
const char *col_text( sqlite3_stmt *stmt, int col );

/*
 * Bind text to a statement parameter, binding NULL for empty strings.
 */
void db_bind_text_or_null( sqlite3_stmt *stmt, int col, const char *val );

/*
 * Return the string if non-NULL and non-empty, otherwise "".
 */
const char *safe_str( const char *s );

/*
 * Transaction helpers - wrap sqlite3_exec with error handling.
 */
void db_begin( sqlite3 *db );
void db_commit( sqlite3 *db );
void db_rollback( sqlite3 *db );

#endif /* DB_UTIL_H */
