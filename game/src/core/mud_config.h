#ifndef MUD_CONFIG_H
#define MUD_CONFIG_H

/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH	  1024
#define MAX_STRING_LENGTH 8192
#define MAX_INPUT_LENGTH  400

/*
 * Godwars Game Parameters
 * By Rotain
 */

#define SKILL_ADEPT			 100
#define SKILL_THAC0_32		 18
#define SKILL_THAC0_00		 6
#define VERSION_NUMBER		 1
#define DONATION_ROOM_WEAPON 3207
#define DONATION_ROOM_ARMOR	 3207
#define DONATION_ROOM_REST	 3207
#define MAX_VAMPIRE_POWER	 3
#define MAX_CLAN			 11
#define MAX_DISCIPLINES		 44
#define MAX_ART				 12
#define MAX_SONGS			 1

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_ALIAS		 30
#define MAX_KINGDOM		 5
#define CURRENT_REVISION 1 // change this each time you update revision of pfiles
#define PARADOX_TICK	 30
#define MAX_SKILL		 208
#define MAX_SPELL		 72
#define MAX_LEVEL		 12
#define MAX_TOP_PLAYERS	 20
#define NO_WATCH		 10
#define LEVEL_HERO		 ( MAX_LEVEL - 9 )
#define LEVEL_IMMORTAL	 ( MAX_LEVEL - 5 )

#define LEVEL_MORTAL	  ( MAX_LEVEL - 10 )
#define LEVEL_AVATAR	  ( MAX_LEVEL - 9 )
#define LEVEL_APPRENTICE  ( MAX_LEVEL - 8 )
#define LEVEL_MAGE		  ( MAX_LEVEL - 7 )
#define LEVEL_ARCHMAGE	  ( MAX_LEVEL - 6 )
#define LEVEL_NINJA		  ( MAX_LEVEL - 6 )
#define LEVEL_MONK		  ( MAX_LEVEL - 6 )
#define LEVEL_BUILDER	  ( MAX_LEVEL - 5 )
#define LEVEL_QUESTMAKER  ( MAX_LEVEL - 4 )
#define LEVEL_ENFORCER	  ( MAX_LEVEL - 3 )
#define LEVEL_JUDGE		  ( MAX_LEVEL - 2 )
#define LEVEL_HIGHJUDGE	  ( MAX_LEVEL - 1 )
#define LEVEL_IMPLEMENTOR ( MAX_LEVEL )

#define PULSE_PER_SECOND 4
#define PULSE_VIOLENCE	 ( 3 * PULSE_PER_SECOND )
/* Save the database - OLC 1.1b */
#define PULSE_DB_DUMP ( 1800 * PULSE_PER_SECOND ) /* 30 minutes  */

#define PULSE_EMBRACE ( 4 * PULSE_PER_SECOND )
#define PULSE_MOBILE  ( 4 * PULSE_PER_SECOND )
#define PULSE_PLAYERS ( 4 * PULSE_PER_SECOND )
#define PULSE_TICK	  ( 30 * PULSE_PER_SECOND )
#define PULSE_AREA	  ( 60 * PULSE_PER_SECOND )
#define PULSE_WW	  ( 4 * PULSE_PER_SECOND )
#define PULSE_MINUTE  ( 60 * PULSE_PER_SECOND )

/* semi-pulses */
#define PULSE_ARENA	   120 /* 120 minutes */
#define PULSE_RAGNAROK 15  /*  15 minutes */

#endif /* MUD_CONFIG_H */
