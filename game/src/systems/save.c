/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "../db/db_player.h"

extern char mud_db_dir[MUD_PATH_MAX];

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch ) {
	if ( IS_NPC( ch ) || ch->level < 2 )
		return;

	if ( ch->desc != NULL && ch->desc->connected != CON_PLAYING )
		return;

	if ( ch->desc != NULL && ch->desc->original != NULL )
		ch = ch->desc->original;

	/* Only update leaderboards when combat stats have changed */
	if ( ch->pcdata->stats_dirty ) {
		check_leaderboard( ch );
		update_top_board( ch );
		ch->pcdata->stats_dirty = FALSE;
	}

	ch->save_time = current_time;
	db_player_save( ch );
	return;
}

void save_char_obj_backup( CHAR_DATA *ch ) {
	char src[MUD_PATH_MAX];
	char dst[MUD_PATH_MAX];
	FILE *fin, *fout;
	char buf[4096];
	size_t n;

	if ( IS_NPC( ch ) || ch->level < 2 )
		return;

	if ( ch->desc != NULL && ch->desc->original != NULL )
		ch = ch->desc->original;

	/* Copy the .db file to backup/ directory under db/players/
	 * Note: Caller (do_save) must call save_char_obj() first to ensure
	 * the .db file is up-to-date before copying. */
	if ( snprintf( src, sizeof( src ), "%s%splayers%s%s.db",
		mud_db_dir, PATH_SEPARATOR, PATH_SEPARATOR,
		capitalize( ch->pcdata->switchname ) ) >= (int)sizeof( src ) ) {
		bug( "backup_char: source path truncated", 0 );
		return;
	}
	if ( snprintf( dst, sizeof( dst ), "%s%splayers%sbackup%s%s.db",
		mud_db_dir, PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR,
		capitalize( ch->pcdata->switchname ) ) >= (int)sizeof( dst ) ) {
		bug( "backup_char: destination path truncated", 0 );
		return;
	}

	fin = fopen( src, "rb" );
	if ( !fin )
		return;
	fout = fopen( dst, "wb" );
	if ( !fout ) {
		fclose( fin );
		return;
	}
	while ( ( n = fread( buf, 1, sizeof( buf ), fin ) ) > 0 )
		fwrite( buf, 1, n, fout );
	fclose( fin );
	fclose( fout );
}

/*
 * Load a char (short) for finger lookups - no inventory.
 */
bool load_char_short( DESCRIPTOR_DATA *d, char *name ) {
	return db_player_load_short( d, name );
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( DESCRIPTOR_DATA *d, char *name ) {
	return db_player_load( d, name );
}
