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

/***************************************************************************
 *  File: olc_save.c                                                       *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/
/* OLC_SAVE.C
 * This takes care of saving all the .are information.
 * Notes:
 * -If a good syntax checker is used for setting vnum ranges of areas
 *  then it would become possible to just cycle through vnums instead
 *  of using the iHash stuff and checking that the room or reset or
 *  mob etc is part of that area.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "olc.h"
#include "../db/db_sql.h"
#include "../db/db_game.h"

void save_help() {
	db_game_save_helps();
}

/*****************************************************************************
 Name:		save_area
 Purpose:	Save an area, note that this format is new.
 Called by:	do_asave(olc_save.c).
 ****************************************************************************/
void save_area( AREA_DATA *pArea ) {
	/* Save to SQLite .db file */
	db_sql_save_area( pArea );

	/* Recalculate area difficulty after saving changes */
	calculate_area_difficulty( pArea );

	return;
}

/* OLC 1.1b */
/*****************************************************************************
 Name:		do_asave
 Purpose:	Entry point for saving area data.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_asave( CHAR_DATA *ch, char *argument ) {
	char arg1[MAX_INPUT_LENGTH];
	AREA_DATA *pArea;
	int value;

	if ( !ch ) /* Do an autosave */
	{
		LIST_FOR_EACH( pArea, &g_areas, AREA_DATA, node ) {
			save_area( pArea );
			REMOVE_BIT( pArea->area_flags, AREA_CHANGED | AREA_ADDED );
		}
		return;
	}
	if ( IS_NPC( ch ) ) return;

	if ( ch->pcdata->security < 2 ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg1 );

	if ( arg1[0] == '\0' ) {
		send_to_char( "Syntax:\n\r", ch );
		send_to_char( "  asave <vnum>    - saves a particular area\n\r", ch );
		send_to_char( "  asave helps     - saves the help file\n\r", ch );
		send_to_char( "  asave area      - saves the area being edited\n\r", ch );
		send_to_char( "  asave changed   - saves all changed zones\n\r", ch );
		send_to_char( "  asave world     - saves the world! (db dump)\n\r", ch );
		send_to_char( "  asave ^ verbose - saves in verbose mode\n\r", ch );
		send_to_char( "\n\r", ch );
		return;
	}

	/* Snarf the value (which need not be numeric). */
	value = atoi( arg1 );

	/* Save the area of given vnum. */
	/* ---------------------------- */

	if ( !( pArea = get_area_data( value ) ) && is_number( arg1 ) ) {
		send_to_char( "That area does not exist.\n\r", ch );
		return;
	}

	if ( is_number( arg1 ) ) {
		if ( !IS_BUILDER( ch, pArea ) ) {
			send_to_char( "You are not a builder for this area.\n\r", ch );
			return;
		}

		if ( !str_cmp( "verbose", argument ) )
			SET_BIT( pArea->area_flags, AREA_VERBOSE );
		save_area( pArea );
		REMOVE_BIT( pArea->area_flags, AREA_VERBOSE );
		return;
	}

	/* Save the world, only authorized areas. */
	/* -------------------------------------- */

	if ( !str_cmp( "world", arg1 ) ) {
		LIST_FOR_EACH( pArea, &g_areas, AREA_DATA, node ) {
			/* Builder must be assigned this area. */
			if ( !IS_BUILDER( ch, pArea ) )
				continue;

			if ( !str_cmp( "verbose", argument ) )
				SET_BIT( pArea->area_flags, AREA_VERBOSE );
			save_area( pArea );
			REMOVE_BIT( pArea->area_flags, AREA_CHANGED | AREA_ADDED | AREA_VERBOSE );
		}
		send_to_char( "You saved the world.\n\r", ch );
		return;
	}
	if ( !str_cmp( arg1, "helps" ) ) {
		if ( ch->level > 6 ) {
			save_help();
			send_to_char( "Helps saved.\n\r", ch );
		}
		return;
	}

	/* Save changed areas, only authorized areas. */
	/* ------------------------------------------ */

	if ( !str_cmp( "changed", arg1 ) ) {
		char buf[MAX_INPUT_LENGTH];


		send_to_char( "Saved zones:\n\r", ch );
		sprintf( buf, "None.\n\r" );

		LIST_FOR_EACH( pArea, &g_areas, AREA_DATA, node ) {
			/* Builder must be assigned this area. */
			if ( !IS_BUILDER( ch, pArea ) )
				continue;

			/* Save changed areas. */
			if ( IS_SET( pArea->area_flags, AREA_CHANGED ) || IS_SET( pArea->area_flags, AREA_ADDED ) ) {
				if ( !str_cmp( "verbose", argument ) )
					SET_BIT( pArea->area_flags, AREA_VERBOSE );
				save_area( pArea );
				REMOVE_BIT( pArea->area_flags, AREA_CHANGED | AREA_ADDED | AREA_VERBOSE );
				sprintf( buf, "%24s - '%s'\n\r", pArea->name, pArea->filename );
				send_to_char( buf, ch );
			}
		}
		if ( !str_cmp( buf, "None.\n\r" ) )
			send_to_char( buf, ch );
		return;
	}

	/* Save area being edited, if authorized. */
	/* -------------------------------------- */
	if ( !str_cmp( arg1, "area" ) ) {
		/* Find the area to save. */
		switch ( ch->desc->editor ) {
		case ED_AREA:
			pArea = (AREA_DATA *) ch->desc->pEdit;
			break;
		case ED_ROOM:
			pArea = ch->in_room->area;
			break;
		case ED_OBJECT:
			pArea = ( (OBJ_INDEX_DATA *) ch->desc->pEdit )->area;
			break;
		case ED_MOBILE:
			pArea = ( (MOB_INDEX_DATA *) ch->desc->pEdit )->area;
			break;
		default:
			pArea = ch->in_room->area;
			break;
		}

		if ( !IS_BUILDER( ch, pArea ) ) {
			send_to_char( "You are not a builder for this area.\n\r", ch );
			return;
		}

		if ( !str_cmp( "verbose", argument ) )
			SET_BIT( pArea->area_flags, AREA_VERBOSE );
		save_area( pArea );
		REMOVE_BIT( pArea->area_flags, AREA_CHANGED | AREA_ADDED | AREA_VERBOSE );
		send_to_char( "Area saved.\n\r", ch );
		return;
	}

	/* Show correct syntax. */
	/* -------------------- */
	do_asave( ch, "" );
	return;
}
