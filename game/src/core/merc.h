#ifndef MERC_H
#define MERC_H

/***************************************************************************
 *  Original Diku Mud copyright (1 << 2) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (1 << 2) 1992, 1993 by Michael          *
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

/* Standard includes */
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Windows compatibility - must come before POSIX headers */
#include "compat.h"

#if !defined( WIN32 )
#include <sys/cdefs.h>
#include <sys/time.h>
#include <pthread.h>
#endif

/* Class constant headers */
#include "monk.h"
#include "garou.h"
#include "angel.h"
#include "lich.h"
#include "shapeshifter.h"
#include "undead_knight.h"
#include "tanarri.h"
#include "spiderdroid.h"
#include "mage.h"
#include "ninja.h"
#include "drow.h"
#include "demon.h"
#include "samurai.h"
#include "class.h"

/* Boolean compatibility */
#if !defined( FALSE )
#define FALSE 0
#endif

#if !defined( TRUE )
#define TRUE 1
#endif

/* sh_int typedef removed — use int directly (was typedef short int from Diku era) */

#include <stdint.h>

/* bool type - use stdbool.h on modern compilers, define manually on old ones */
#if defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#else
typedef unsigned char bool;
#endif

/* Libraries */
#include "list.h"

/* mccp: support bits */
#include "zlib.h"
#define TELOPT_COMPRESS	  85
#define COMPRESS_BUF_SIZE 16384

/* Core subsystem headers */
#include "types.h"
#include "mud_config.h"
#include "board.h"
#include "network.h"
#include "char.h"
#include "object.h"
#include "room.h"
#include "world.h"
#include "combat.h"

/*
 * TO types for act.
 */
#define TO_ROOM	   0
#define TO_NOTVICT 1
#define TO_VICT	   2
#define TO_CHAR	   3
#define TO_ALL	   4

/*
 * Structure for a command in the command lookup table.
 */
struct cmd_type {
	char *const name;
	DO_FUN *do_fun;
	int position;
	int level;
	int log;
	int race;		   /* 0 = all, other = specific race */
	int discipline; /* USE THE DISC_VAMP_???? etc.... */
	int disclevel;  /* level in disc the command is granted */
};

/*
 * Structure for a social in the socials table.
 */
struct social_type {
	char *name;
	char *char_no_arg;
	char *others_no_arg;
	char *char_found;
	char *others_found;
	char *vict_found;
	char *char_auto;
	char *others_auto;
};

/* Removed: xsocial_type structure - xsocial system removed */

/* Declarations */
#include "globals.h"
#include "paths.h"
#include "prototypes.h"

#endif /* MERC_H */
