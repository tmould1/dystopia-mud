/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license._doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

/*
 * The social table.
 *
 * Populated from tables.db at boot time by db_tables_load_socials().
 * To add or modify socials, edit gamedata/db/game/tables.db using the
 * mudedit editor or seed_tables_db.py script.
 *
 * The array is sized to 128 entries to allow room for SQL-loaded data.
 * Entry [0] starts as a sentinel; after loading, social_count reflects
 * the actual number of entries.
 */

struct social_type social_table[128] =
	{
		{ "", NULL, NULL, NULL, NULL, NULL, NULL, NULL }
	};

int social_count = 0;
