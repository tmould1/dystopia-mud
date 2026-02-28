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

/*
 * Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
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

#include "list.h"

/* mccp: support bits */
#include "zlib.h"
#define TELOPT_COMPRESS	  85
#define COMPRESS_BUF_SIZE 16384

/*
 * Structure types.
 */
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct ban_data BAN_DATA;
typedef struct char_data CHAR_DATA;
typedef struct alias_data ALIAS_DATA;

typedef struct top_board TOP_BOARD;
typedef struct l_board LEADER_BOARD;

typedef struct editor_data EDITOR_DATA;

typedef struct dns_lookup DNS_LOOKUP;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct help_data HELP_DATA;
typedef struct kill_data KILL_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct war_data WAR_DATA;
typedef struct note_data NOTE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct pc_data PC_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;

typedef struct disabled_data DISABLED_DATA;

/* one disabled command */
struct disabled_data {
	list_node_t node;
	struct cmd_type const *command; /* pointer to the command struct*/
	char *disabled_by;				/* name of disabler */
	int level;					/* level of disabler */
};
extern list_head_t disabled_list;

/*
 * Function types.
 */
typedef void DO_FUN ( CHAR_DATA * ch, char *argument );
typedef bool SPEC_FUN ( CHAR_DATA * ch );
typedef void SPELL_FUN ( int sn, int level, CHAR_DATA *ch, void *vo );

/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH	  1024
#define MAX_STRING_LENGTH 8192
#define MAX_INPUT_LENGTH  400

/*
 * Rotains Gobal Procedures
 */
void clear_stats ( CHAR_DATA * ch );
void room_is_total_darkness ( ROOM_INDEX_DATA * pRoomIndex );
void improve_wpn ( CHAR_DATA * ch, int dtype, int right_hand );
void improve_stance ( CHAR_DATA * ch );
void skillstance ( CHAR_DATA * ch, CHAR_DATA *victim );
void show_spell ( CHAR_DATA * ch, int dtype );
void fightaction ( CHAR_DATA * ch, CHAR_DATA *victim, int actype,
	int dtype, int wpntype );
void crack_head ( CHAR_DATA * ch, OBJ_DATA *obj, char *argument );
void critical_hit ( CHAR_DATA * ch, CHAR_DATA *victim, int dt, int dam );

void take_item ( CHAR_DATA * ch, OBJ_DATA *obj );
void raw_kill ( CHAR_DATA * victim );
void trip ( CHAR_DATA * ch, CHAR_DATA *victim );
void disarm ( CHAR_DATA * ch, CHAR_DATA *victim );
void make_corpse ( CHAR_DATA * ch );
void one_hit ( CHAR_DATA * ch, CHAR_DATA *victim, int dt, int handtype );

void special_hurl ( CHAR_DATA * ch, CHAR_DATA *victim );
void make_part ( CHAR_DATA * ch, char *argument );
void home_write ();
void behead ( CHAR_DATA * victim );
void paradox ( CHAR_DATA * ch );

bool fair_fight ( CHAR_DATA * ch, CHAR_DATA *victim );

/*
 * file stuff
 */
void load_topboard (void);
void save_topboard (void);
void load_leaderboard (void);
void save_leaderboard (void);
void load_bans (void);
void save_bans (void);
void load_kingdoms (void);
void save_kingdoms (void);

void load_gameconfig (void);
void save_gameconfig (void);

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

#include "board.h"

/*
 * Site ban structure.
 */
struct ban_data {
	list_node_t node;
	char *name;
	char *reason;
};

/*
 * Time and weather stuff.
 */
#define SUN_DARK  0
#define SUN_RISE  1
#define SUN_LIGHT 2
#define SUN_SET	  3

#define SKY_CLOUDLESS 0
#define SKY_CLOUDY	  1
#define SKY_RAINING	  2
#define SKY_LIGHTNING 3

struct time_info_data {
	int hour;
	int day;
	int month;
	int year;
	int tick;	/* Sub-hour tick counter for time_scale */
};

struct weather_data {
	int mmhg;
	int change;
	int sky;
	int sunlight;
};

/*
 * J.O.P.E.
 */
struct jope_type {
	char *const name;
	DO_FUN *do_fun;
	int level;
};

struct bit_type {
	char *const name;
	int bit_value;
};

typedef struct kingdom_data {
	char *whoname; // the name used in do_who().
	char *name;	   // the keyword name.
	char *leader;  // who runs the place.
	char *general; // who's the right hand man.
	int kills;	   // amount of pkills done by kingdom members.
	int deaths;	   // amount of pkills done agains kingdom members.
	int qps;	   // the size of the kingdoms wealth.
	int req_hit;   // hps req to join.
	int req_move;  // move req to join.
	int req_mana;  // mana req to join.
	int req_qps;   // qps cost to join (will be donated to the kingdom vault).
} KINGDOM_DATA;

typedef struct config_data {
	int base_xp;
	int max_xp_per_kill;
	char *game_name;
	char *gui_url;		/* URL to Mudlet mpackage for GMCP Client.GUI */
	char *gui_version;	/* Version string for Client.GUI updates */
	char *banner_left;	/* Left banner endcap, e.g. "#0<>#n" */
	char *banner_right; /* Right banner endcap, e.g. "#0<>#n" */
	char *banner_fill;	/* Banner fill pattern, e.g. "#0==#n" */
	char *audio_url;	/* Base URL for MCMP audio files */
} GAMECONFIG_DATA;

/*
 * Directions.
 * Used in #ROOMS.
 */
typedef enum {
	DIR_NORTH,
	DIR_EAST,
	DIR_SOUTH,
	DIR_WEST,
	DIR_UP,
	DIR_DOWN,
	DIR_NORTHEAST,
	DIR_NORTHWEST,
	DIR_SOUTHEAST,
	DIR_SOUTHWEST,
	DIR_SOMEWHERE
} dir_types;
#define SUB_NORTH DIR_NORTH
#define SUB_EAST  DIR_EAST
#define SUB_SOUTH DIR_SOUTH
#define SUB_WEST  DIR_WEST
#define SUB_UP	  DIR_UP
#define SUB_DOWN  DIR_DOWN
#define SUB_NE	  DIR_NORTHEAST
#define SUB_NW	  DIR_NORTHWEST
#define SUB_SE	  DIR_SOUTHEAST
#define SUB_SW	  DIR_SOUTHWEST

/*
 * threaded status - Jobo
 */
#define STATUS_LOOKUP 0 // New Descriptor, in lookup pr. default.
#define STATUS_DONE	  1 // The lookup is done.
#define STATUS_WAIT	  2 // Closed while in thread.
#define STATUS_CLOSED 3 // Closed, ready to be recycled.

/*
 * Connected state for a channel.
 */
#define CON_PLAYING				 0
#define CON_GET_NAME			 1
#define CON_GET_OLD_PASSWORD	 2
#define CON_CONFIRM_NEW_NAME	 3
#define CON_GET_NEW_PASSWORD	 4
#define CON_CONFIRM_NEW_PASSWORD 5
#define CON_GET_NEW_SEX			 6
#define CON_GET_NEW_CLASS		 7
#define CON_GET_NEW_VT102		 8
#define CON_GET_NEW_ANSI		 9
#define CON_READ_MOTD			 10
#define CON_NOT_PLAYING			 11
#define CON_EDITING				 12
#define CON_COPYOVER_RECOVER	 13
/* Values 14-18 reserved for CON_NOTE_* in board.h */
#define CON_PFILE			 20
#define CON_GET_NEW_GMCP	 21
#define CON_GET_NEW_MXP		 22
#define CON_GET_NEW_EXPLEVEL 23
#define CON_DETECT_CAPS		 24

/*
 * Character substates
 */
typedef enum {
	SUB_NONE,
	SUB_PAUSE,
	SUB_PERSONAL_DESC,
	SUB_OBJ_SHORT,
	SUB_OBJ_LONG,
	SUB_OBJ_EXTRA,
	SUB_MOB_LONG,
	SUB_MOB_DESC,
	SUB_ROOM_DESC,
	SUB_ROOM_EXTRA,
	SUB_ROOM_EXIT_DESC,
	SUB_WRITING_NOTE,
	SUB_MPROG_EDIT,
	SUB_HELP_EDIT,
	SUB_WRITING_MAP,
	SUB_PERSONAL_BIO,
	SUB_REPEATCMD,
	SUB_RESTRICTED,
	SUB_DEITYDESC,
	/* timer types ONLY below this point */
	SUB_TIMER_DO_ABORT = 128,
	SUB_TIMER_CANT_ABORT
} char_substates;

/*
 * Timer macros.
 */

#define TIMER( ch, tmr )		 ( ( ch )->tick_timer[( tmr )] )
#define SET_TIMER( ch, tmr, tm ) ( ( ch )->tick_timer[( tmr )] = ( tm ) )
#define ADD_TIMER( ch, tmr, tm ) ( ( ch )->tick_timer[( tmr )] += ( tm ) )
#define SUB_TIMER( ch, tmr, tm ) ( ( ch )->tick_timer[( tmr )] -= ( tm ) )
#define TIME_UP( ch, tmr )		 ( ( ch )->tick_timer[( tmr )] == 0 ? TRUE : FALSE )

#define RTIMER( room, rtmr )		  ( ( room )->tick_timer[( rtmr )] )
#define SET_RTIMER( room, rtmr, rtm ) ( ( room )->tick_timer[( rtmr )] = ( rtm ) )
#define ADD_RTIMER( room, rtmr, rtm ) ( ( room )->tick_timer[( rtmr )] += ( rtm ) )
#define SUB_RTIMER( room, rtmr, rtm ) ( ( room )->tick_timer[( rtmr )] -= ( rtm ) )
#define RTIME_UP( room, rtmr )		  ( ( room )->tick_timer[( rtmr )] == 0 ? TRUE : FALSE )

#define TIMER_LAYONHANDS		 0
#define TIMER_WRENCH			 1
#define TIMER_WRENCHED			 2
#define TIMER_VAMPCALL			 3
#define TIMER_UNCONCIOUS		 4
#define TIMER_VAMP_INHABIT		 5
#define TIMER_DAEMONIC_TRANSFORM 6
#define TIMER_MESMERISE			 7
#define TIMER_MESMERISED		 8
#define TIMER_FORAGE			 9
#define TIMER_NO_ARBOREA		 11
#define TIMER_TREE_WALK			 12
#define TIMER_CAN_PRAC			 13
#define TIMER_CAN_EAT_ARMS		 14
#define TIMER_THIRD_ARM_GROWING	 15
#define TIMER_FOURTH_ARM_GROWING 16
#define TIMER_THIRD_ARM_GOING	 17
#define TIMER_FOURTH_ARM_GOING	 18
#define TIMER_SCALPED			 19
#define TIMER_CAN_CALL_ROCKS	 20
#define TIMER_CANT_BE_TURNED	 21
#define TIMER_CANT_TURN			 22
#define TIMER_FIGHT_LAG			 23
#define TIMER_CAN_CHANGE_HAWK	 24
#define TIMER_CAN_CREATE_SHARD	 25
#define TIMER_CAN_GUST			 26
#define TIMER_CAN_ENTER_STASIS	 27
#define TIMER_MAKE_SNOWMAN		 28
#define TIMER_ENTOMB			 29
#define TIMER_CAN_BREATHE_FROST	 30
#define TIMER_HELLFIRE_SUMMON	 31
#define TIMER_ON_SPEED			 32
#define TIMER_ON_LSD			 33
#define TIMER_CAN_CALL_WAR_HORSE 35
#define TIMER_WAR_HORSE_GO		 36
#define TIMER_CAN_SPIT_VENOM	 37
#define TIMER_CAN_GAIN_VOODOO	 38
#define TIMER_CAN_FEATHER		 39
#define TIMER_CAN_SHRIEK		 40
#define TIMER_CAN_POLYMORPH		 41
#define TIMER_DRAGON_GROW		 42
#define TIMER_VAMPIRE_GROW		 43
#define TIMER_SKILL_LEV1		 44
#define TIMER_SKILL_LEV2		 45
#define TIMER_CANT_SWARM		 46
#define TIMER_CANT_BORROWLIFE	 47
#define TIMER_TREE				 48
#define TIMER_NEXUS_STUNNED		 49
#define TIMER_GOLEM				 50
#define TIMER_TAINT				 51
#define TIMER_NEWBIE_IMM		 52
#define TIMER_CAN_DO_NEXUS		 53
#define TIMER_CAN_USE_HEALER	 54
#define TIMER_DISCORD			 55
#define TIMER_SPHINX_ROAR		 56
#define TIMER_INFERNO			 57
#define TIMER_CHAOSPORT			 58
#define TIMER_CANMAJESTY		 59
#define TIMER_MAJESTY			 60
#define TIMER_DSLEEP			 61

#define MAX_TIMER 62

#define RTIMER_STINKING_CLOUD	0
#define RTIMER_LIFE_VORTEX		1
#define RTIMER_DEATH_VORTEX		2
#define RTIMER_GLYPH_PROTECTION 3
#define RTIMER_HIDE_ROOM		4
#define RTIMER_SWARM_BEES		5
#define RTIMER_SWARM_RATS		6
#define RTIMER_SWARM_BATS		7
#define RTIMER_GHOST_LIGHT		8
#define RTIMER_NEXUS_FLAME		9
#define RTIMER_NEXUS_WATER		10
#define RTIMER_NEXUS_AIR		11
#define RTIMER_NEXUS_EARTH		12
#define RTIMER_NEXUS_ENTROPY	13
#define RTIMER_WALL_NORTH		14
#define RTIMER_WALL_EAST		15
#define RTIMER_WALL_SOUTH		16
#define RTIMER_WALL_WEST		17
#define RTIMER_WALL_UP			18
#define RTIMER_WALL_DOWN		19
#define RTIMER_DISCORD			20
#define RTIMER_DARK_ROOM		21
#define RTIMER_SILENCE			22
#define MAX_RTIMER				30

/*
 * DNS reverse-lookup task, passed to a worker thread.
 */
struct dns_lookup {
	list_node_t node;
	DESCRIPTOR_DATA *d;
	char *buf;
	int status;
};

/*
 * Descriptor (channel) structure.
 */
struct descriptor_data {
	list_node_t node;
	DESCRIPTOR_DATA *snoop_by;
	CHAR_DATA *character;
	CHAR_DATA *original;
	char *host;
	int descriptor;
	int connected;
	int lookup_status;
	bool fcommand;
	bool vt102;
	char inbuf[4 * MAX_INPUT_LENGTH];
	char incomm[MAX_INPUT_LENGTH];
	char inlast[MAX_INPUT_LENGTH];
	int repeat;
	char *showstr_head;	 /* From ENVY code to compile */
	char *showstr_point; /* From ENVY code to compile */
	char *outbuf;
	int outsize;
	int outtop;
	void *pEdit;	/* OLC */
	char **pString; /* OLC */
	int editor;		/* OLC */
	/* mccp: support data */
	z_stream *out_compress;
	unsigned char *out_compress_buf;
	int mccp_version; /* 0=none, 1=v1, 2=v2 */
	/* gmcp: support data */
	bool gmcp_enabled; /* GMCP negotiation successful */
	int gmcp_packages; /* Bitmask of supported packages */
	/* mxp: MUD eXtension Protocol support */
	bool mxp_enabled; /* MXP negotiation successful */
	/* naws: window size support (RFC 1073) */
	bool naws_enabled; /* NAWS negotiation successful */
	int client_width;  /* Terminal width in columns */
	int client_height; /* Terminal height in rows */
	/* ttype: Terminal Type / MTTS support (RFC 1091) */
	bool ttype_enabled; /* TTYPE negotiation successful */
	int  ttype_round;   /* Current TTYPE round (1-3) */
	int  mtts_flags;    /* MTTS capability bitfield */
	char client_name[64];    /* Terminal/client name from round 1 */
	char terminal_type[64];  /* Standard terminal name from round 2 */
	/* charset: client encoding support (RFC 2066) */
	int  client_charset;     /* CHARSET_UNKNOWN=0, CHARSET_ASCII=1, CHARSET_UTF8=2 */
	bool charset_negotiated; /* TRUE once charset is determined */
	/* intro: capability detection timing */
	int  intro_pulse;        /* Pulses elapsed since connection (for CON_DETECT_CAPS) */
};

/*
 * Attribute bonus structures.
 */
struct str_app_type {
	int tohit;
	int todam;
	int carry;
	int wield;
};

struct int_app_type {
	int learn;
};

struct wis_app_type {
	int practice;
};

struct dex_app_type {
	int defensive;
};

struct con_app_type {
	int hitp;
	int shock;
};

/*
 * TO types for act.
 */
#define TO_ROOM	   0
#define TO_NOTVICT 1
#define TO_VICT	   2
#define TO_CHAR	   3
#define TO_ALL	   4

/*
 * Help table types.
 */
struct help_data {
	list_node_t node;
	AREA_DATA *area;
	int level;
	char *keyword;
	char *text;
};

/*
 * Shop types.
 */
#define MAX_TRADE 5

struct shop_data {
	int keeper;					/* Vnum of shop keeper mob	*/
	int buy_type[MAX_TRADE]; /* Item types shop will buy	*/
	int profit_buy;			/* Cost multiplier for buying	*/
	int profit_sell;			/* Cost multiplier for selling	*/
	int open_hour;			/* First opening hour		*/
	int close_hour;			/* First closing hour		*/
};

/*
 * Data structure for notes.
 */
struct note_data {
	list_node_t node;
	char *sender;
	char *date;
	char *to_list;
	char *subject;
	char *text;
	time_t date_stamp;
	time_t expire;
};

/*
 * An affect.
 */
struct affect_data {
	list_node_t node;
	int type;
	int duration;
	int location;
	int modifier;
	int bitvector;
};

/*
 * An alias
 */
struct alias_data {
	list_node_t node;
	char *short_n;
	char *long_n;
};

/*
 * A kill structure (indexed by level).
 */
struct kill_data {
	int number;
	int killed;
};

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/* Bit position macros removed -- flag constants now use explicit (1u << N) */

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES. flags2
 */

#define VAMP_ASHES		(1 << 0)
#define VAMP_CLONE		(1 << 1)
#define VAMP_OBJMASK	(1 << 2)
#define AFF_TOTALBLIND	(1 << 3)
#define AFF_SPIRITGUARD (1 << 4)

#define AFF_CLAW (1 << 11)
#define AFF_BITE (1 << 12)
#define AFF_TAIL (1 << 13)
#define AFF_WING (1 << 14)

// flag3
#define AFF3_BLINK_1ST_RD (1 << 0)
#define AFF3_BLINK_2ND_RD (1 << 1)

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_VAMPIRE 3404

/*
 * Immunities, for players.  KaVir.
 */
#define IMM_SLASH	  1		   /* Resistance to slash, slice. 		*/
#define IMM_STAB	  2		   /* Resistance to stab, pierce. 		*/
#define IMM_SMASH	  4		   /* Resistance to blast, pound, crush. 	*/
#define IMM_ANIMAL	  8		   /* Resistance to bite, claw. 		*/
#define IMM_MISC	  16	   /* Resistance to grep, suck, whip. 	*/
#define IMM_CHARM	  32	   /* Immune to charm spell. 		*/
#define IMM_HEAT	  64	   /* Immune to fire/heat spells. 		*/
#define IMM_COLD	  128	   /* Immune to frost/cold spells.		*/
#define IMM_LIGHTNING 256	   /* Immune to lightning spells.		*/
#define IMM_ACID	  512	   /* Immune to acid spells.		*/
#define IMM_SUMMON	  1024	   /* Immune to being summoned.		*/
#define IMM_VOODOO	  2048	   /* Immune to voodoo magic.		*/
#define IMM_VAMPIRE	  4096	   /* Allow yourself to become a vampire.	*/
#define IMM_STAKE	  8192	   /* Immune to being staked (vamps only). */
#define IMM_SUNLIGHT  16384	   /* Immune to sunlight (vamps only).	*/
#define IMM_SHIELDED  32768	   /* For Obfuscate. Block scry, etc.	*/
#define IMM_HURL	  65536	   /* Cannot be hurled.			*/
#define IMM_BACKSTAB  131072   /* Cannot be backstabbed.		*/
#define IMM_KICK	  262144   /* Cannot be kicked.			*/
#define IMM_DISARM	  524288   /* Cannot be disarmed.			*/
#define IMM_STEAL	  1048576  /* Cannot have stuff stolen.		*/
#define IMM_SLEEP	  2097152  /* Immune to sleep spell.		*/
#define IMM_DRAIN	  4194304  /* Immune to energy drain.		*/
#define IMM_SHIELD2	  8388608  /* Chaotic shield			*/
#define IMM_TRANSPORT 16777216 /* Objects can't be transported to you.	*/
#define IMM_TRAVEL	  33554432

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC	   (1 << 0) /* Auto set for mobs	*/
#define ACT_SENTINEL   (1 << 1) /* Stays in one room	*/
#define ACT_SCAVENGER  (1 << 2) /* Picks up objects	*/
#define ACT_AGGRESSIVE (1 << 3) /* Attacks PC's		*/
#define ACT_STAY_AREA  (1 << 4) /* Won't leave area	*/
#define ACT_WIMPY	   (1 << 5) /* Flees when hurt	*/
#define ACT_PET		   (1 << 6) /* Auto set for pets	*/
#define ACT_TRAIN	   (1 << 7) /* Can train PC's	*/
#define ACT_PRACTICE   (1 << 8) /* Can practice PC's	*/
#define ACT_MOUNT	   (1 << 9) /* Can be mounted	*/
#define ACT_NOPARTS	   (1 << 10) /* Dead = no body parts	*/
#define ACT_NOEXP	   (1 << 11) /* No exp for killing   */
#define ACT_PROTOTYPE  (1 << 12)
#define ACT_NOAUTOKILL (1 << 13)
#define ACT_NOEXP2	   (1 << 14)

/*
 * Thingers for Demon Warps
 */

#define WARP_CBODY		1
#define WARP_SBODY		2
#define WARP_STRONGARMS 4
#define WARP_STRONGLEGS 8
#define WARP_VENOMTONG	16
#define WARP_SPIKETAIL	32
#define WARP_BADBREATH	64
#define WARP_QUICKNESS	128
#define WARP_STAMINA	256
#define WARP_HUNT		512
#define WARP_DEVOUR		1024
#define WARP_TERROR		2048
#define WARP_REGENERATE 4096
#define WARP_STEED		8192
#define WARP_WEAPON		16384
#define WARP_INFIRMITY	32768
#define WARP_GBODY		65536
#define WARP_SCARED		131072
#define WARP_MAGMA		262144
#define WARP_WEAK		524288
#define WARP_SLOW		1048576
#define WARP_VULNER		2097152
#define WARP_SHARDS		4194304
#define WARP_WINGS		8388608
#define WARP_CLUMSY		16777216
#define WARP_STUPID		33554432
#define WARP_SPOON		67108864
#define WARP_FORK		134217728
#define WARP_KNIFE		268435456
#define WARP_SALADBOWL	536870912

/* Bits for the Discie thing Numbers.. really.. not bits */

#define DISC_VAMP_CELE 2
#define DISC_VAMP_FORT 3
#define DISC_VAMP_OBTE 4
#define DISC_VAMP_PRES 5
#define DISC_VAMP_QUIE 6
#define DISC_VAMP_THAU 7
#define DISC_VAMP_AUSP 8
#define DISC_VAMP_DOMI 9
#define DISC_VAMP_OBFU 10
#define DISC_VAMP_POTE 11
#define DISC_VAMP_PROT 12
#define DISC_VAMP_SERP 13
#define DISC_VAMP_VICI 14
#define DISC_VAMP_DAIM 15
#define DISC_VAMP_ANIM 16

#define DISC_WERE_BEAR 18
#define DISC_WERE_LYNX 19
#define DISC_WERE_BOAR 20
#define DISC_WERE_OWL  21
#define DISC_WERE_SPID 22
#define DISC_WERE_WOLF 23
#define DISC_WERE_HAWK 24
#define DISC_WERE_MANT 25
#define DISC_WERE_RAPT 26
#define DISC_WERE_LUNA 27
#define DISC_WERE_PAIN 28
#define DISC_WERE_CONG 29

#define DISC_DAEM_HELL 30
#define DISC_DAEM_ATTA 31
#define DISC_DAEM_TEMP 32
#define DISC_DAEM_MORP 33
#define DISC_DAEM_CORR 34
#define DISC_DAEM_GELU 35
#define DISC_DAEM_DISC 36
#define DISC_DAEM_NETH 37
#define DISC_DAEM_IMMU 38
#define DISC_VAMP_CHIM 39
#define DISC_VAMP_THAN 40
#define DISC_VAMP_OBEA 41
#define DISC_VAMP_NECR 42
#define DISC_VAMP_MELP 43

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND		  1
#define AFF_INVISIBLE	  2
#define AFF_DETECT_EVIL	  4
#define AFF_DETECT_INVIS  8
#define AFF_DETECT_MAGIC  16
#define AFF_DETECT_HIDDEN 32
#define AFF_SHADOWPLANE	  64 /* Creatures in shadow plane - KaVir */
#define AFF_SANCTUARY	  128
#define AFF_FAERIE_FIRE	  256
#define AFF_INFRARED	  512
#define AFF_CURSE		  1024
#define AFF_FLAMING		  2048 /* For burning creatures - KaVir */
#define AFF_POISON		  4096
#define AFF_PROTECT		  8192
#define AFF_ETHEREAL	  16384 /* For ethereal creatures - KaVir */
#define AFF_SNEAK		  32768
#define AFF_HIDE		  65536
#define AFF_SLEEP		  131072
#define AFF_CHARM		  262144
#define AFF_FLYING		  524288
#define AFF_PASS_DOOR	  1048576
#define AFF_POLYMORPH	  2097152 /* For polymorphed creatures - KaVir */
#define AFF_SHADOWSIGHT	  4194304 /* Can see between planes - KaVir */
#define AFF_WEBBED		  8388608 /* Cannot move - KaVir */
#define AFF_PROTECT_GOOD  16777216
#define AFF_DROWFIRE	  33554432 /* Drow Darkness - Rotain */
#define AFF_ZULOFORM	  67108864
#define AFF_SHIFT		  134217728
#define AFF_PEACE		  268435456
#define AFF_INFIRMITY	  536870912

/* Affected_by 2 */
#define AFF_CONTRACEPTION 1
#define AFF_BEASTIE		  2

#define PLR_IMPLAG	 8
#define EXTRA_BLINKY 16

#define OBJ_VNUM_LEGS 46

/*
 * Bits for 'itemaffect'.
 * Used in #MOBILES.
 */
#define ITEMA_SHOCKSHIELD  1
#define ITEMA_FIRESHIELD   2
#define ITEMA_ICESHIELD	   4
#define ITEMA_ACIDSHIELD   8
#define ITEMA_MONKCHI	   16
#define ITEMA_CHAOSSHIELD  32
#define ITEMA_ARTIFACT	   64
#define ITEMA_REGENERATE   128
#define ITEMA_SPEED		   256
#define ITEMA_VORPAL	   512
#define ITEMA_PEACE		   1024
#define ITEMA_RIGHT_SILVER 2048
#define ITEMA_LEFT_SILVER  4096
#define ITEMA_REFLECT	   8192
#define ITEMA_RESISTANCE   16384
#define ITEMA_VISION	   32768
#define ITEMA_STALKER	   65536
#define ITEMA_VANISH	   131072
#define ITEMA_RAGER		   262144
#define ITEMA_TALON		   524288
#define ITEMA_CHAOSHANDS   1048576
#define ITEMA_AFFMANTIS	   2097152
#define ITEMA_AFFENTROPY   4194304
#define ITEMA_AFFEYE	   8388608
#define ITEMA_MAGESHIELD   16777216
#define ITEMA_STEELSHIELD  33554432	 /* Mage tougness */
#define ITEMA_DEFLECTOR	   67108864	 /* Mage dodge/parry */
#define ITEMA_ILLUSIONS	   134217728 /* Mage dodge/parry */
#define ITEMA_BEAST		   268435456 /* Mage extra attacks */

/*
 * Color is now handled entirely through # codes in write_to_buffer().
 * Use col_scale_code(current, max) for dynamic color scaling.
 * See "help color" in-game for the full code table.
 */

/*
 * Bits for 'vampire'.
 * Used for player vampires.
 */
#define VAM_FANGS	   1
#define VAM_CLAWS	   2
#define VAM_NIGHTSIGHT 4
#define VAM_FLYING	   8  /* For flying creatures */
#define VAM_SONIC	   16 /* For creatures with full detect */
#define VAM_CHANGED	   32 /* Changed using a vampire power */

#define VAM_PROTEAN		  64   /* Claws, nightsight, and change */
#define VAM_CELERITY	  128  /* 66%/33% chance 1/2 extra attacks */
#define VAM_FORTITUDE	  256  /* 5 hp less per hit taken */
#define VAM_POTENCE		  512  /* Deal out 1.5 times normal damage */
#define VAM_OBFUSCATE	  1024 /* Disguise and invis */
#define VAM_AUSPEX		  2048 /* Truesight, etc */
#define VAM_OBTENEBRATION 4096 /* Shadowplane/sight and shadowbody */
#define VAM_SERPENTIS	  8192 /* Eyes/serpent, heart/darkness, etc */

#define VAM_DISGUISED 16384 /* For the Obfuscate disguise ability */
#define VAM_MORTAL	  32768 /* For Obfuscate mortal ability. */

#define VAM_DOMINATE 65536 /* Evileye, command */

#define VAM_EVILEYE 131072 /* Evileye, command */

#define VAM_PRESENCE	262144	/* Presence discipline */
#define VAM_VICISSITUDE 524288	/* Vicissitude discipline */
#define VAM_THAU		1048576 /* Thaumaturgy discipline */
#define VAM_ANIMAL		2097152 /* Animalism discipline */
#define VAM_SHIFTED		4194304 /* Non-poly shift */
#define VAM_QUIETUS		8388608 /* Quietus discipline */
#define VAM_HEAD		16777216
#define VAM_TAIL		33554432
#define VAM_EXOSKELETON 67108864
#define VAM_HORNS		134217728
#define VAM_WINGS		268435456

/*
 * Bits for 'polymorph'.
 * Used for players.
 */
#define POLY_BAT	  1
#define POLY_WOLF	  2
#define POLY_MIST	  4
#define POLY_SERPENT  8
#define POLY_RAVEN	  16
#define POLY_FISH	  32
#define POLY_FROG	  64
#define POLY_ZULOFORM 128
#define POLY_SHIFT	  256
#define POLY_SPIDER	  512
#define POLY_DRAGON	  1024
/*
 * Languages.
 */
#define LANG_COMMON 0
#define DIA_OLDE	1
#define DIA_BAD		2
#define LANG_DARK	4

/*
 * Score.
 */
#define SCORE_TOTAL_XP	  0
#define SCORE_HIGH_XP	  1
#define SCORE_TOTAL_LEVEL 2
#define SCORE_HIGH_LEVEL  3
#define SCORE_QUEST		  4
#define SCORE_NUM_QUEST	  5

/*
 * Zombie Lord.
 */
#define ZOMBIE_NOTHING	0
#define ZOMBIE_TRACKING 1
#define ZOMBIE_ANIMATE	2
#define ZOMBIE_CAST		3
#define ZOMBIE_REST		4

/*
 * Damcap values.
 */
#define DAM_CAP	   0
#define DAM_CHANGE 1

/* return values for check_imm */
#define IS_NORMAL	  0
#define IS_DIMMUNE	  1
#define IS_RESISTANT  2
#define IS_VULNERABLE 3

/* damage classes */
#define DAM_NONE	  0
#define DAM_BASH	  1
#define DAM_PIERCE	  2
#define DAM_SLASH	  3
#define DAM_FIRE	  4
#define DAM_COLD	  5
#define DAM_LIGHTNING 6
#define DAM_ACID	  7
#define DAM_POISON	  8
#define DAM_NEGATIVE  9
#define DAM_HOLY	  10
#define DAM_ENERGY	  11
#define DAM_MENTAL	  12
#define DAM_DISEASE	  13
#define DAM_DROWNING  14
#define DAM_LIGHT	  15
#define DAM_OTHER	  16
#define DAM_HARM	  17
#define DAM_CHARM	  18
#define DAM_SOUND	  19

/* IMM bits for mobs */
#define DIMM_SUMMON	   (1 << 0)
#define DIMM_CHARM	   (1 << 1)
#define DIMM_MAGIC	   (1 << 2)
#define DIMM_WEAPON	   (1 << 3)
#define DIMM_BASH	   (1 << 4)
#define DIMM_PIERCE	   (1 << 5)
#define DIMM_SLASH	   (1 << 6)
#define DIMM_FIRE	   (1 << 7)
#define DIMM_COLD	   (1 << 8)
#define DIMM_LIGHTNING (1 << 9)
#define DIMM_ACID	   (1 << 10)
#define DIMM_POISON	   (1 << 11)
#define DIMM_NEGATIVE  (1 << 12)
#define DIMM_HOLY	   (1 << 13)
#define DIMM_ENERGY	   (1 << 14)
#define DIMM_MENTAL	   (1 << 15)
#define DIMM_DISEASE   (1 << 16)
#define DIMM_DROWNING  (1 << 17)
#define DIMM_LIGHT	   (1 << 18)
#define DIMM_SOUND	   (1 << 19)
#define DIMM_WOOD	   (1 << 23)
#define DIMM_SILVER	   (1 << 24)
#define DIMM_IRON	   (1 << 25)

/*
 * Mounts
 */
#define IS_ON_FOOT	0
#define IS_MOUNT	1
#define IS_RIDING	2
#define IS_CARRIED	4
#define IS_CARRYING 8

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL 0
#define SEX_MALE	1
#define SEX_FEMALE	2

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_MONEY_ONE	2
#define OBJ_VNUM_MONEY_SOME 3

/* Demonic Transformation */

#define OBJ_VNUM_DHELM	  27924
#define OBJ_VNUM_DGREAVES 27925
#define OBJ_VNUM_DCLOAK	  27926
#define OBJ_VNUM_DARMOR	  27927
#define OBJ_VNUM_DRING	  27928
#define OBJ_VNUM_DSWORD	  27929
#define OBJ_VNUM_DSHIELD  27930

#define OBJ_VNUM_BROD		  30073
#define OBJ_VNUM_LKLAIVE	  33114
#define OBJ_VNUM_STAKE		  30011
#define OBJ_VNUM_MEDAL		  29521
#define OBJ_VNUM_KHORNE		  29664
#define OBJ_VNUM_CORPSE_NPC	  10
#define OBJ_VNUM_CORPSE_PC	  11
#define OBJ_VNUM_SEVERED_HEAD 12
#define OBJ_VNUM_TORN_HEART	  13
#define OBJ_VNUM_SLICED_ARM	  14
#define OBJ_VNUM_SLICED_LEG	  15
#define OBJ_VNUM_FINAL_TURD	  16

#define OBJ_VNUM_MUSHROOM	  20
#define OBJ_VNUM_LIGHT_BALL	  21
#define OBJ_VNUM_SPRING		  22
#define OBJ_VNUM_BLOOD_SPRING 23

#define OBJ_VNUM_SCHOOL_MACE   3700
#define OBJ_VNUM_SCHOOL_DAGGER 3701
#define OBJ_VNUM_SCHOOL_SWORD  3702
#define OBJ_VNUM_SCHOOL_VEST   3703
#define OBJ_VNUM_SCHOOL_SHIELD 3704
#define OBJ_VNUM_SCHOOL_BANNER 3716

/* For KaVir's stuff */
#define OBJ_VNUM_SOULBLADE		  30000
#define OBJ_VNUM_GATE			  30042
#define OBJ_VNUM_GATE2			  30072
#define OBJ_VNUM_PORTAL			  30001
#define OBJ_VNUM_EGG			  30002
#define OBJ_VNUM_EMPTY_EGG		  30003
#define OBJ_VNUM_SPILLED_ENTRAILS 30004
#define OBJ_VNUM_QUIVERING_BRAIN  30005
#define OBJ_VNUM_SQUIDGY_EYEBALL  30006
#define OBJ_VNUM_SPILT_BLOOD	  30007
#define OBJ_VNUM_VOODOO_DOLL	  30010
#define OBJ_VNUM_RIPPED_FACE	  30012
#define OBJ_VNUM_TORN_WINDPIPE	  30013
#define OBJ_VNUM_CRACKED_HEAD	  30014
#define OBJ_VNUM_SLICED_EAR		  30025
#define OBJ_VNUM_SLICED_NOSE	  30026
#define OBJ_VNUM_KNOCKED_TOOTH	  30027
#define OBJ_VNUM_TORN_TONGUE	  30028
#define OBJ_VNUM_SEVERED_HAND	  30029
#define OBJ_VNUM_SEVERED_FOOT	  30030
#define OBJ_VNUM_SEVERED_THUMB	  30031
#define OBJ_VNUM_SEVERED_INDEX	  30032
#define OBJ_VNUM_SEVERED_MIDDLE	  30033
#define OBJ_VNUM_SEVERED_RING	  30034
#define OBJ_VNUM_SEVERED_LITTLE	  30035
#define OBJ_VNUM_SEVERED_TOE	  30036
#define OBJ_VNUM_PROTOPLASM		  30037
#define OBJ_VNUM_QUESTCARD		  30039
#define OBJ_VNUM_QUESTMACHINE	  30040
#define OBJ_VNUM_COPPER			  30049
#define OBJ_VNUM_IRON			  30050
#define OBJ_VNUM_STEEL			  30051
#define OBJ_VNUM_ADAMANTITE		  30052

#define MOB_VNUM_GUARDIAN 33001
#define MOB_VNUM_SERVANT  33002
#define MOB_VNUM_MOUNT	  6
#define MOB_VNUM_FROG	  7
#define MOB_VNUM_RAVEN	  8
#define MOB_VNUM_CAT	  9
#define MOB_VNUM_DOG	  10
#define MOB_VNUM_EYE	  12
#define MOB_VNUM_SATAN	  30003
#define MOB_VNUM_DEMON	  30005
#define MOB_VNUM_SERPENT  33003
#define MOB_VNUM_ILLUSION 33004
#define MOB_VNUM_FIRE	  93361
#define MOB_VNUM_STONE	  93362
#define MOB_VNUM_IRON	  93363
#define MOB_VNUM_CLAY	  93364

/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT		  1
#define ITEM_SCROLL		  2
#define ITEM_WAND		  3
#define ITEM_STAFF		  4
#define ITEM_WEAPON		  5
#define ITEM_TREASURE	  8
#define ITEM_ARMOR		  9
#define ITEM_POTION		  10
#define ITEM_FURNITURE	  12
#define ITEM_TRASH		  13
#define ITEM_CONTAINER	  15
#define ITEM_DRINK_CON	  17
#define ITEM_KEY		  18
#define ITEM_FOOD		  19
#define ITEM_MONEY		  20
#define ITEM_BOAT		  22
#define ITEM_CORPSE_NPC	  23
#define ITEM_CORPSE_PC	  24
#define ITEM_FOUNTAIN	  25
#define ITEM_PILL		  26
#define ITEM_PORTAL		  27
#define ITEM_EGG		  28
#define ITEM_VOODOO		  29
#define ITEM_STAKE		  30
#define ITEM_MISSILE	  31 /* Ammo vnum, cur, max, type */
#define ITEM_AMMO		  32 /* ???, dam min, dam max, type */
#define ITEM_QUEST		  33
#define ITEM_QUESTCARD	  34
#define ITEM_QUESTMACHINE 35
#define ITEM_SYMBOL		  36
#define ITEM_BOOK		  37
#define ITEM_PAGE		  38
#define ITEM_TOOL		  39
#define ITEM_WALL		  40
#define ITEM_COPPER		  41
#define ITEM_IRON		  42
#define ITEM_STEEL		  43
#define ITEM_ADAMANTITE	  44
#define ITEM_GEMSTONE	  45
#define ITEM_HILT		  46
#define ITEM_DTOKEN		  47
#define ITEM_HEAD		  48
#define ITEM_COOKINGPOT	  50
#define ITEM_DRAGONGEM	  51
#define ITEM_WGATE		  52
#define ITEM_INSTRUMENT	  53

/*
 * Weapon Stats
 */

#define WEAPON_FLAMING		   (1 << 0)
#define WEAPON_FROST		   (1 << 1)
#define WEAPON_VAMPIRIC		   (1 << 2)
#define WEAPON_SHARP		   (1 << 3)
#define WEAPON_VORPAL		   (1 << 4)
#define WEAPON_TWO_HANDS	   (1 << 5)
#define WEAPON_SHOCKING		   (1 << 6)
#define WEAPON_POISON		   (1 << 7)
#define WEAPON_SUNBLADE		   (1 << 8)
#define WEAPON_DRAGONLANCE	   (1 << 9)
#define WEAPON_SILVER		   (1 << 10)
#define WEAPON_RUNE_FORCE_BOLT (1 << 11)
#define WEAPON_RUNE_SMITE_EVIL (1 << 12)
#define WEAPON_RUNE_BLAZE	   (1 << 13)
#define WEAPON_RUNE_LIGHTNING  (1 << 14)
#define WEAPON_RUNE_DANCING	   (1 << 15)
#define WEAPON_ELE_FLAME	   (1 << 16)
#define WEAPON_ELE_WATER	   (1 << 17)
#define WEAPON_ELE_EARTH	   (1 << 18)
#define WEAPON_ELE_AIR		   (1 << 19)

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW		  1
#define ITEM_HUM		  2
#define ITEM_THROWN		  4
#define ITEM_KEEP		  8
#define ITEM_VANISH		  16
#define ITEM_INVIS		  32
#define ITEM_MAGIC		  64
#define ITEM_NODROP		  128
#define ITEM_BLESS		  256
#define ITEM_ANTI_GOOD	  512
#define ITEM_ANTI_EVIL	  1024
#define ITEM_ANTI_NEUTRAL 2048
#define ITEM_NOREMOVE	  4096
#define ITEM_INVENTORY	  8192
#define ITEM_LOYAL		  16384
#define ITEM_SHADOWPLANE  32768
#define ITEM_PROTOTYPE	  (1 << 11)
#define ITEM_MENCHANT	  65536

/* Item extra flags II, the return of the item flags!  */

#define ITEM_ARM			   (1 << 0)
#define ITEM_NYSTUL			   (1 << 1)
#define ITEM_NO_INTERRUPT	   (1 << 2)
#define ITEM_DAEMONSEED		   (1 << 3)
#define ITEM_JUJU_BAG		   (1 << 5)
#define ITEM_VOO_HEAD		   (1 << 6)
#define ITEM_VOO_DEAD		   (1 << 7)
#define ITEM_VOO_BODY		   (1 << 8)
#define ITEM_VOO_THREAD		   (1 << 9)
#define ITEM_INGRED_GLOIN	   (1 << 10)
#define ITEM_INGRED_FROGBREATH (1 << 11)
#define ITEM_INGRED_PAPACOCO   (1 << 12)
#define ITEM_INGRED_MULDALEAF  (1 << 13)
#define ITEM_INGRED_SCULLYWEED (1 << 14)
#define ITEM_INGRED_WORMWART   (1 << 15)
#define ITEM_INGRED_TILLIFREEN (1 << 16)
#define ITEM_INGRED_BAJUJU	   (1 << 17)
#define ITEM_ATTACK_GOOD	   (1 << 18)
#define ITEM_ITEMHIDE		   (1 << 19)
#define ITEM_ICE			   (1 << 20)

/* artifact and relic flags */

#define ITEM_TELEPORTS			 (1 << 22)	/* teleports upon owner death */
#define ITEM_DESTROYED			 (1 << 23)	/* destroyed upon owner death */
#define ITEM_UNIQUE				 (1 << 24)	/* only 1 in game ever */
#define ITEM_DESIRED			 (1 << 25)	/* you can't let go of it ever */
#define ITEM_INDESTRUCTABLE		 (1 << 26) /* guess */
#define ITEM_TELEPORT_PROTECTION (1 << 27) /* teleports when attempts to destroy it */
#define ITEM_KNOW_OWNER			 (1 << 28) /* owner displayed on artifact command */

#define ITEM_FLYING		 (1 << 29)
#define ITEM_FORM_MELDED (1 << 30)

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE		  1
#define ITEM_WEAR_FINGER  2
#define ITEM_WEAR_NECK	  4
#define ITEM_WEAR_BODY	  8
#define ITEM_WEAR_HEAD	  16
#define ITEM_WEAR_LEGS	  32
#define ITEM_WEAR_FEET	  64
#define ITEM_WEAR_HANDS	  128
#define ITEM_WEAR_ARMS	  256
#define ITEM_WEAR_SHIELD  512
#define ITEM_WEAR_ABOUT	  1024
#define ITEM_WEAR_WAIST	  2048
#define ITEM_WEAR_WRIST	  4096
#define ITEM_WIELD		  8192
#define ITEM_HOLD		  16384
#define ITEM_WEAR_FACE	  32768
#define ITEM_WEAR_SPECIAL 65536
#define ITEM_WEAR_BODYART 131072
#define ITEM_WEAR_MEDAL	  262144
#define ITEM_WEAR_FLOAT	  524288

/*
 * Special types.
 * Used in #OBJECTS for special items - KaVir.
 */
#define SITEM_ACTIVATE	  1
#define SITEM_TWIST		  2
#define SITEM_PRESS		  4
#define SITEM_PULL		  8
#define SITEM_TARGET	  16
#define SITEM_SPELL		  32
#define SITEM_TRANSPORTER 64
#define SITEM_TELEPORTER  128
#define SITEM_DELAY1	  256
#define SITEM_DELAY2	  512
#define SITEM_OBJECT	  1024
#define SITEM_MOBILE	  2048
#define SITEM_ACTION	  4096
#define SITEM_MORPH		  8192
#define SITEM_SILVER	  16384
#define SITEM_WOLFWEAPON  32768
#define SITEM_DROW		  65536
#define SITEM_CHAMPWEAPON 131072
#define SITEM_DEMONIC	  262144
#define SITEM_COPPER	  1048576
#define SITEM_MAGE		  2097152
#define SITEM_STEEL		  4194304
#define SITEM_ADAMANTITE  8388608
#define SITEM_GEMSTONE	  16777216
#define SITEM_HILT		  33554432
#define SITEM_PDEMONIC	  67108864
#define SITEM_MONK		  134217728
#define SITEM_IRON		  268435456
/*
 * Apply types (for quest affects).
 * Used in #OBJECTS.
 */
#define QUEST_STR	  1
#define QUEST_DEX	  2
#define QUEST_INT	  4
#define QUEST_WIS	  8
#define QUEST_CON	  16
#define QUEST_HITROLL 32
#define QUEST_DAMROLL 64
#define QUEST_HIT	  128
#define QUEST_MANA	  256
#define QUEST_MOVE	  512
#define QUEST_AC	  1024

#define QUEST_NAME	   2048
#define QUEST_SHORT	   4096
#define QUEST_LONG	   8192
#define QUEST_FREENAME 16384

#define QUEST_ENCHANTED	 32768
#define QUEST_SPELLPROOF 65536
#define QUEST_ARTIFACT	 131072
#define QUEST_IMPROVED	 262144
#define QUEST_PRIZE		 524288
#define QUEST_RELIC		 1048576
#define QUEST_BLOODA	 2097152
#define QUEST_CLONED	 4194304
#define QUEST_ZOMBIE	 8388608
#define QUEST_FORGE		 16777216
#define ITEM_EQUEST		 33554432

/*
 * Tool types.
 */
#define TOOL_PEN	 1
#define TOOL_PLIERS	 2
#define TOOL_SCALPEL 4

/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE			0
#define APPLY_STR			1
#define APPLY_DEX			2
#define APPLY_INT			3
#define APPLY_WIS			4
#define APPLY_CON			5
#define APPLY_SEX			6
#define APPLY_CLASS			7
#define APPLY_LEVEL			8
#define APPLY_AGE			9
#define APPLY_HEIGHT		10
#define APPLY_WEIGHT		11
#define APPLY_MANA			12
#define APPLY_HIT			13
#define APPLY_MOVE			14
#define APPLY_GOLD			15
#define APPLY_EXP			16
#define APPLY_AC			17
#define APPLY_HITROLL		18
#define APPLY_DAMROLL		19
#define APPLY_SAVING_PARA	20
#define APPLY_SAVING_ROD	21
#define APPLY_SAVING_PETRI	22
#define APPLY_SAVING_BREATH 23
#define APPLY_SAVING_SPELL	24
#define APPLY_POLY			25

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE 1
#define CONT_PICKPROOF 2
#define CONT_CLOSED	   4
#define CONT_LOCKED	   8

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO			2
#define ROOM_VNUM_CHAT			1200
#define ROOM_VNUM_TEMPLE		3005
#define ROOM_VNUM_ALTAR			3054
#define ROOM_VNUM_SCHOOL		3700
#define ROOM_VNUM_SCHOOL_SEC1	3701 /* Section 1: New to MUDs */
#define ROOM_VNUM_SCHOOL_SEC2	3713 /* Section 2: New to Dystopia */
#define ROOM_VNUM_HELL			93420
#define ROOM_VNUM_CRYPT			30001
#define ROOM_VNUM_DISCONNECTION 3
#define ROOM_VNUM_IN_OBJECT		33000
#define ROOM_VNUM_AWINNER		70
#define ROOM_VNUM_ALOSER		69
#define ROOM_VNUM_CAINE			27000
#define ROOM_VNUM_DEVOUR		30006
#define ROOM_VNUM_ELKOR			100300

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK			1
#define ROOM_NO_OTRANS		2
#define ROOM_NO_MOB			4
#define ROOM_INDOORS		8
#define ROOM_SEX			16
#define ROOM_PRIVATE		512
#define ROOM_SAFE			1024
#define ROOM_SOLITARY		2048
#define ROOM_PET_SHOP		4096
#define ROOM_NO_RECALL		8192
#define ROOM_NO_TELEPORT	16384
#define ROOM_TOTAL_DARKNESS 32768
#define ROOM_BLADE_BARRIER	65536
#define ROOM_ARENA			131072
#define ROOM_FLAMING		262144
#define ROOM_SILENCE		524288
#define ROOM_ASTRAL			1048576
#define ROOM_PROTOTYPE		2097152
#define ROOM_ORDER			4194304
#define ROOM_NO_CHANT		8388608

/*
 * Room text flags (KaVir).
 * Used in #ROOMS.
 */
#define RT_LIGHTS	  1 /* Toggles lights on/off */
#define RT_SAY		  2 /* Use this if no others powers */
#define RT_ENTER	  4
#define RT_CAST		  8
#define RT_THROWOUT	  16	/* Erm...can't remember ;) */
#define RT_OBJECT	  32	/* Creates an object */
#define RT_MOBILE	  64	/* Creates a mobile */
#define RT_LIGHT	  128	/* Lights on ONLY */
#define RT_DARK		  256	/* Lights off ONLY */
#define RT_OPEN_LIFT  512	/* Open lift */
#define RT_CLOSE_LIFT 1024	/* Close lift */
#define RT_MOVE_LIFT  2048	/* Move lift */
#define RT_SPELL	  4096	/* Cast a spell */
#define RT_PORTAL	  8192	/* Creates a one-way portal */
#define RT_TELEPORT	  16384 /* Teleport player to room */

#define RT_ACTION  32768
#define RT_BLANK_1 65536
#define RT_BLANK_2 131072

#define RT_RETURN	1048576 /* Perform once */
#define RT_PERSONAL 2097152 /* Only shows message to char */
#define RT_TIMER	4194304 /* Sets object timer to 1 tick */

/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH 0
#define DIR_EAST  1
#define DIR_SOUTH 2
#define DIR_WEST  3
#define DIR_UP	  4
#define DIR_DOWN  5

/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR		  1
#define EX_CLOSED		  2
#define EX_LOCKED		  4
#define EX_PICKPROOF	  32
#define EX_NOPASS		  64
#define EX_EASY			  128
#define EX_HARD			  256
#define EX_INFURIATING	  512
#define EX_NOCLOSE		  1024
#define EX_NOLOCK		  2048
#define EX_ICE_WALL		  4096
#define EX_FIRE_WALL	  8192
#define EX_SWORD_WALL	  16384
#define EX_PRISMATIC_WALL 32768
#define EX_IRON_WALL	  65536
#define EX_MUSHROOM_WALL  131072
#define EX_CALTROP_WALL	  262144
#define EX_ASH_WALL		  524288
#define EX_WARDING		  1048576

#define MAX_EXFLAG 20
#define MAX_WALL   8

/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE		  0
#define SECT_CITY		  1
#define SECT_FIELD		  2
#define SECT_FOREST		  3
#define SECT_HILLS		  4
#define SECT_MOUNTAIN	  5
#define SECT_WATER_SWIM	  6
#define SECT_WATER_NOSWIM 7
#define SECT_UNUSED		  8
#define SECT_AIR		  9
#define SECT_DESERT		  10
#define SECT_MAX		  11

/*
 * Equipment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE		-1
#define WEAR_LIGHT		0
#define WEAR_FINGER_L	1
#define WEAR_FINGER_R	2
#define WEAR_NECK_1		3
#define WEAR_NECK_2		4
#define WEAR_BODY		5
#define WEAR_HEAD		6
#define WEAR_LEGS		7
#define WEAR_FEET		8
#define WEAR_HANDS		9
#define WEAR_ARMS		10
#define WEAR_SHIELD		11
#define WEAR_ABOUT		12
#define WEAR_WAIST		13
#define WEAR_WRIST_L	14
#define WEAR_WRIST_R	15
#define WEAR_WIELD		16
#define WEAR_HOLD		17
#define WEAR_THIRD		18
#define WEAR_FOURTH		19
#define WEAR_FACE		20
#define WEAR_SCABBARD_L 21
#define WEAR_SCABBARD_R 22
#define WEAR_SPECIAL	23
#define WEAR_FLOAT		24
#define WEAR_MEDAL		25
#define WEAR_BODYART	26
#define MAX_WEAR		27

/*
 * Locations for damage.
 */
#define LOC_HEAD  0
#define LOC_BODY  1
#define LOC_ARM_L 2
#define LOC_ARM_R 3
#define LOC_LEG_L 4
#define LOC_LEG_R 5

/*
 * For Head
 */
#define LOST_EYE_L	  1
#define LOST_EYE_R	  2
#define LOST_EAR_L	  4
#define LOST_EAR_R	  8
#define LOST_NOSE	  16
#define BROKEN_NOSE	  32
#define BROKEN_JAW	  64
#define BROKEN_SKULL  128
#define LOST_HEAD	  256
#define LOST_TOOTH_1  512  /* These should be added..... */
#define LOST_TOOTH_2  1024 /* ...together to caculate... */
#define LOST_TOOTH_4  2048 /* ...the total number of.... */
#define LOST_TOOTH_8  4096 /* ...teeth lost.  Total..... */
#define LOST_TOOTH_16 8192 /* ...possible is 31 teeth.   */
#define LOST_TONGUE	  16384

/*
 * For Body
 */
#define BROKEN_RIBS_1  1  /* Remember there are a total */
#define BROKEN_RIBS_2  2  /* of 12 pairs of ribs in the */
#define BROKEN_RIBS_4  4  /* human body, so not all of  */
#define BROKEN_RIBS_8  8  /* these bits should be set   */
#define BROKEN_RIBS_16 16 /* at the same time.          */
#define BROKEN_SPINE   32
#define BROKEN_NECK	   64
#define CUT_THROAT	   128
#define CUT_STOMACH	   256
#define CUT_CHEST	   512

/*
 * For Arms
 */
#define BROKEN_ARM		1
#define LOST_ARM		2
#define LOST_HAND		4
#define LOST_FINGER_I	8  /* Index finger */
#define LOST_FINGER_M	16 /* Middle finger */
#define LOST_FINGER_R	32 /* Ring finger */
#define LOST_FINGER_L	64 /* Little finger */
#define LOST_THUMB		128
#define BROKEN_FINGER_I 256	 /* Index finger */
#define BROKEN_FINGER_M 512	 /* Middle finger */
#define BROKEN_FINGER_R 1024 /* Ring finger */
#define BROKEN_FINGER_L 2048 /* Little finger */
#define BROKEN_THUMB	4096

/*
 * For Legs
 */
#define BROKEN_LEG	   1
#define LOST_LEG	   2
#define LOST_FOOT	   4
#define LOST_TOE_A	   8
#define LOST_TOE_B	   16
#define LOST_TOE_C	   32
#define LOST_TOE_D	   64 /* Smallest toe */
#define LOST_TOE_BIG   128
#define BROKEN_TOE_A   256
#define BROKEN_TOE_B   512
#define BROKEN_TOE_C   1024
#define BROKEN_TOE_D   2048 /* Smallest toe */
#define BROKEN_TOE_BIG 4096

/*
 * For Bleeding
 */
#define BLEEDING_HEAD	1
#define BLEEDING_THROAT 2
#define BLEEDING_ARM_L	4
#define BLEEDING_ARM_R	8
#define BLEEDING_HAND_L 16
#define BLEEDING_HAND_R 32
#define BLEEDING_LEG_L	64
#define BLEEDING_LEG_R	128
#define BLEEDING_FOOT_L 256
#define BLEEDING_FOOT_R 512

/*
 * For Spec powers on players
 */
#define EYE_SPELL	   1 /* Spell when they look at you */
#define EYE_SELFACTION 2 /* You do action when they look */
#define EYE_ACTION	   4 /* Others do action when they look */

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK	0
#define COND_FULL	1
#define COND_THIRST 2

/*
 * Stats - KaVir.
 */
#define STAT_STR 0
#define STAT_END 1
#define STAT_REF 2
#define STAT_FLE 2

/*
 * Positions.
 */
#define POS_DEAD	   0
#define POS_MORTAL	   1
#define POS_INCAP	   2
#define POS_STUNNED	   3
#define POS_SLEEPING   4
#define POS_MEDITATING 5
#define POS_SITTING	   6
#define POS_RESTING	   7
#define POS_FIGHTING   8
#define POS_STANDING   9
/* Status of Arena */

#define FIGHT_OPEN	0
#define FIGHT_START 1
#define FIGHT_BUSY	2
#define FIGHT_LOCK	3

/*
 * ACT bits for players.
 */

#define PLR_IS_NPC		1 /* Don't EVER set.	*/
#define PLR_BRIEF3		2
#define PLR_LEFTHAND	4
#define PLR_AUTOEXIT	8
#define PLR_AUTOLOOT	16
#define PLR_AUTOSAC		32
#define PLR_BLANK		64
#define PLR_BRIEF		128
#define PLR_COMBINE		512
#define PLR_PROMPT		1024
#define PLR_TELNET_GA	2048
#define PLR_HOLYLIGHT	4096
#define PLR_WIZINVIS	8192
#define PLR_ANSI		16384
#define PLR_SILENCE		32768
#define PLR_VT102		65536
#define PLR_RIGHTHAND	131072
#define PLR_NO_TELL		262144
#define PLR_LOG			524288
#define PLR_DENY		1048576
#define PLR_FREEZE		2097152
#define PLR_BRIEF2		16777216
#define PLR_WATCHER		33554432
#define PLR_ACID		67108864
#define PLR_BRIEF4		134217728
#define PLR_AMBI		268435456	// Can use both hands well.
#define PLR_XTERM		536870912	// Full xterm-256 color support
#define PLR_PREFER_GMCP 1073741824	// User prefers GMCP enabled
#define PLR_SCREENREADER 256		// Screen reader accessibility mode
#define PLR_PREFER_MCMP	4194304		// User prefers MCMP (Client.Media) enabled
#define PLR_PREFER_MXP	2147483648U // User prefers MXP enabled
#define PLR_AUTOMAP		8388608		// Auto-show map on movement

/*New bits for playrs (Infidel)*/
#define NEW_SLAM		 1
#define NEW_QUILLS		 2
#define NEW_JAWLOCK		 4
#define NEW_PERCEPTION	 8
#define NEW_SKIN		 16
#define NEW_TIDE		 32
#define NEW_COIL		 64
#define NEW_REND		 128
#define NEW_MONKFLAME	 256
#define NEW_SCLAWS		 512
#define NEW_IRONMIND	 1024
#define NEW_MONKCLOAK	 2048
#define NEW_MONKADAM	 4096
#define NEW_MONKSKIN	 8192
#define NEW_MONKFAVOR	 16384
#define NEW_CLOAK		 32768
#define NEW_DROWHATE	 65536
#define NEW_DARKNESS	 131072
#define THIRD_HAND		 262144
#define FOURTH_HAND		 524288
#define NEW_MENTALBLOCK	 1048576
#define NEW_VISION		 2097152
#define NEW_NATURAL		 4194304
#define NEW_POWER		 8388608
#define NEW_DFORM		 16777216
#define NEW_MASTERY		 33554432
#define NEW_DARKTENDRILS 67108864
#define NEW_MULTIARMS	 134217728
#define NEW_BLADESPIN	 268435456
#define NEW_FIGHTDANCE	 536870912
#define NEW_CUBEFORM	 1073741824

/*
 * JFLAGS : ch->pcdata->jflags
 */
#define JFLAG_SETDECAP	   1
#define JFLAG_SETLOGIN	   2
#define JFLAG_SETLOGOUT	   4
#define JFLAG_SETAVATAR	   8
#define JFLAG_SETTIE	   16
#define JFLAG_BULLY		   32
#define JFLAG_WANT_KINGDOM 64
#define JFLAG_NOSET		   128

/*
 * special flags
 */

#define SPC_WOLFMAN	 4
#define SPC_INCONNU	 64
#define SPC_DROW_WAR 128
#define SPC_DROW_MAG 256
#define SPC_DROW_CLE 512

/*
 * EXTRA bits for players. (KaVir)
 */
/*    1 */
/*    2 */
#define EXTRA_TRUSTED	   4
#define EXTRA_NEWPASS	   8
#define EXTRA_OSWITCH	   16
#define EXTRA_SWITCH	   32
#define EXTRA_FAKE_CON	   64
#define TIED_UP			   128
#define GAGGED			   256
#define BLINDFOLDED		   512
#define EXTRA_STANCE	   1024
#define EXTRA_DONE		   2048
#define EXTRA_EXP		   4096
#define EXTRA_PREGNANT	   8192
#define EXTRA_LABOUR	   16384
#define EXTRA_BORN		   32768
#define EXTRA_PROMPT	   65536
#define EXTRA_MARRIED	   131072
#define EXTRA_AFK		   262144
#define EXTRA_DRAGON	   524288
#define EXTRA_CALL_ALL	   1048576
#define EXTRA_ANTI_GODLESS 2097152 /* unused */
#define EXTRA_BSD		   4194304
#define EXTRA_EARTHMELD	   8388608
#define EXTRA_PLASMA	   16777216
#define EXTRA_TRUECOLOR	   33554432 /* Xterm true color (24-bit RGB) support */
#define EXTRA_AWE		   67108864
#define EXTRA_ROT		   134217728
#define EXTRA_ZOMBIE	   268435456
#define EXTRA_BAAL		   536870912
#define EXTRA_FLASH		   1073741824

/*
 * AGE Bits.
 */
#define AGE_CHILDE	   0
#define AGE_NEONATE	   1
#define AGE_ANCILLA	   2
#define AGE_ELDER	   3
#define AGE_METHUSELAH 4
#define AGE_LA_MAGRA   5
#define AGE_TRUEBLOOD  6
#define BELT_ONE	   7
#define BELT_TWO	   8
#define BELT_THREE	   9
#define BELT_FOUR	   10
#define BELT_FIVE	   11
#define BELT_SIX	   12
#define BELT_SEVEN	   13
#define BELT_EIGHT	   14
#define BELT_NINE	   15
#define BELT_TEN	   16

/*
 * Stances for combat
 */
#define STANCE_NONE		-1
#define STANCE_NORMAL	0
#define STANCE_VIPER	1
#define STANCE_CRANE	2
#define STANCE_CRAB		3
#define STANCE_MONGOOSE 4
#define STANCE_BULL		5
#define STANCE_MANTIS	6
#define STANCE_DRAGON	7
#define STANCE_TIGER	8
#define STANCE_MONKEY	9
#define STANCE_SWALLOW	10
#define STANCE_WOLF		11

#define STANCE_SS1 13
#define STANCE_SS2 14
#define STANCE_SS3 15
#define STANCE_SS4 16
#define STANCE_SS5 17

/*
 *  Bits for superstances
 */
#define STANCEPOWER_DODGE		 1	   /* more dodge */
#define STANCEPOWER_PARRY		 2	   /* more parry */
#define STANCEPOWER_SPEED		 4	   /* 2 extra attack */
#define STANCEPOWER_BYPASS		 8	   /* bypass dodge/parry */
#define STANCEPOWER_DAMAGE_1	 16	   /* lesser increase damage and chance to hit */
#define STANCEPOWER_DAMAGE_2	 32	   /* greater increase damage and chance to hit */
#define STANCEPOWER_DAMAGE_3	 64	   /* supreme increase damage and chance to hit */
#define STANCEPOWER_RESIST_1	 128   /* lesser resist damage */
#define STANCEPOWER_RESIST_2	 256   /* greater resist damage */
#define STANCEPOWER_RESIST_3	 512   /* supreme resist damage */
#define STANCEPOWER_DAMCAP_1	 1024  /* lesser damcap bonus */
#define STANCEPOWER_DAMCAP_2	 2048  /* greater damcap bonus */
#define STANCEPOWER_DAMCAP_3	 4096  /* supreme damcap bonus */
#define STANCEPOWER_REV_DAMCAP_1 8192  /* lesser damcap penalty for opponent */
#define STANCEPOWER_REV_DAMCAP_2 16384 /* greater damcap penalty for opponent */
#define STANCEPOWER_REV_DAMCAP_3 32768 /* supreme damcap penalty for opponent */

/*
 * Channel bits.
 */

#define CHANNEL_KINGDOM 1
#define CHANNEL_CHAT	2
#define CHANNEL_ANGEL	4
#define CHANNEL_IMMTALK 8
#define CHANNEL_MUSIC	16

#define CHANNEL_YELL		128
#define CHANNEL_VAMPTALK	256
#define CHANNEL_HOWL		512
#define CHANNEL_LOG			1024
#define CHANNEL_PRAY		2048
#define CHANNEL_INFO		4096
#define CHANNEL_FLAME		8192
#define CHANNEL_TELL		16384
#define CHANNEL_MAGETALK	32768
#define CHANNEL_HIGHTALK	65536
#define CHANNEL_NEWBIE		131072
#define CHANNEL_SIGN		262144
#define CHANNEL_MONK		524288
#define CHANNEL_MIKTALK		1048576
#define CHANNEL_TELEPATH	2097152
#define CHANNEL_COMMUNICATE 4194304
#define CHANNEL_KNIGHTTALK	8388608
#define CHANNEL_LICHTALK	16777216
#define CHANNEL_TANTALK		33554432

struct top_board {
	int pkscore;
	char *name;
};

struct l_board {
	char *pk_name;
	int pk_number;
	char *pd_name;
	int pd_number;
	char *tt_name;
	int tt_number;
	char *qc_name;
	int qc_number;
	char *mk_name;
	int mk_number;
	char *md_name;
	int md_number;
	char *bestpk_name;
	int bestpk_number;
};

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data {
	MOB_INDEX_DATA *next;
	SPEC_FUN *spec_fun;
	SHOP_DATA *pShop;
	CHAR_DATA *mount;
	CHAR_DATA *wizard;
	AREA_DATA *area; /* OLC */
	char *hunting;
	char *player_name;
	char *short_descr;
	char *long_descr;
	char *description;
	char *lord;
	char *morph;
	char *createtime;
	char *pload;
	char *lasttime;
	char *lasthost;
	char *powertype;
	char *poweraction;
	char *prompt;
	char *cprompt;
	int spectype;
	int specpower;
	int loc_hp[7];
	int vnum;
	int count;
	int killed;
	int sex;
	int mounted;
	int home;
	int level;
	uint32_t immune;
	uint32_t polyaff;
	int vampaff;
	uint32_t itemaffect;
	uint32_t form;
	uint32_t act;
	uint32_t extra;
	uint32_t affected_by;
	uint32_t affected_by2;
	int alignment;
	int hitroll;	 /* Unused */
	int ac;			 /* Unused */
	int hitnodice;	 /* Unused */
	int hitsizedice; /* Unused */
	int hitplus;	 /* Unused */
	int damnodice;	 /* Unused */
	int damsizedice; /* Unused */
	int damplus;	 /* Unused */
	int gold;		 /* Unused */
					 /*int                 special;
					  int                 class; */
	int death_teleport_vnum;    /* Room vnum to teleport killer on death, 0=none */
	char *death_teleport_msg;      /* Message to char on teleport (NULL=default) */
	char *death_teleport_msg_room; /* Message to room on teleport (NULL=default) */
};

struct editor_data {
	int numlines;
	int on_line;
	int size;
	char line[49][81];
};

/*
 * One character (PC or NPC).
 */
struct char_data {
	list_node_t char_node;
	list_node_t room_node;
	CHAR_DATA *master;
	CHAR_DATA *leader;
	CHAR_DATA *fighting;
	CHAR_DATA *embracing;
	CHAR_DATA *embraced;
	CHAR_DATA *blinkykill;
	CHAR_DATA *reply;
	CHAR_DATA *mount;
	CHAR_DATA *wizard;
	CHAR_DATA *challenger; /*  person who challenged you */
	CHAR_DATA *challenged; /*  person who you challenged */
	CHAR_DATA *gladiator;  /*  ARENA player wagered on */
	SPEC_FUN *spec_fun;
	MOB_INDEX_DATA *pIndexData;
	DESCRIPTOR_DATA *desc;
	list_head_t affects;
	list_head_t carrying;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *was_in_room;
	PC_DATA *pcdata;
	DO_FUN *last_cmd;
	DO_FUN *prev_cmd; /* mapping */
	char *hunting;
	char *name;
	char *pload;
	char *short_descr;
	char *long_descr;
	char *description;
	char *lord;
	char *morph;
	char *createtime;
	char *lasttime;
	char *lasthost;
	char *poweraction;
	char *powertype;
	char *prompt;
	char *cprompt;
	char *prefix;
	int sex;
	int class;
	uint32_t immune;
	uint32_t polyaff;
	uint32_t vampaff_a;
	int fight_timer;
	uint32_t itemaffect;
	uint32_t form;
	int warp;
	int explevel;
	int expgained;
	int power[MAX_DISCIPLINES];
	int xhitroll;
	int xdamroll;

	/* SMAUUUUUUUUUUUG */
	void *dest_buf;
	void *spare_ptr;
	int tempnum;
	EDITOR_DATA *editor;
	int substate;
	int pagelen;			/* BUILD INTERFACE */
	int inter_page;		/* BUILD INTERFACE */
	int inter_type;		/* BUILD INTERFACE */
	char *inter_editing;	/* BUILD INTERFACE */
	int inter_editing_vnum; /* BUILD INTERFACE */
	int inter_substate;	/* BUILD INTERFACE */
							/* End O' Smaug */

	/* Dh Flags */
	int cclan;
	uint32_t flag2;
	uint32_t flag3;
	uint32_t flag4;
	int generation;
	int primary;
	int proper_size;
	int size;
	int cur_form;
	int dragtype;
	int rage;
	int siltol;
	/* end */
	int tick_timer[MAX_TIMER];
	int warpcount;
	int vampgen_a;
	int spectype;
	int specpower;
	int loc_hp[7];
	int wpn[13];
	int spl[8];
	int cmbt[8];
	int stance[24];
	int beast;
	int mounted;
	int home;
	int level;
	int trust;
	int played;
	time_t logon;
	time_t save_time;
	int timer;
	int wait;
	int pkill;
	int pdeath;
	int mkill;
	int mdeath;
	int hit;
	int max_hit;
	int mana;
	int max_mana;
	int move;
	int max_move;
	int gold;
	int exp;
	uint32_t act;
	uint32_t extra;
	uint32_t newbits;
	uint32_t special;
	uint32_t affected_by;
	uint32_t affected_by2;
	int position;
	int practice;
	int carry_weight;
	int carry_number;
	int saving_throw;
	int alignment;
	int hitroll;
	int damroll;
	int armor;
	int wimpy;
	uint32_t deaf;
	int paradox[3];
	int damcap[2];
	int monkstuff;
	int monkcrap;
	int monkab[4];
	int chi[2];
	char *clan;
	int gifts[21];
	int garou1;
	int garou2;
	int gnosis[2];
	CHAR_DATA *unveil;
	char *objdesc;
	int monkblock;
	int focus[2];
};

/*
 * Data which only PC's have.
 */
struct pc_data {
	CHAR_DATA *familiar;
	CHAR_DATA *partner;
	CHAR_DATA *propose;
	CHAR_DATA *pfile;
	OBJ_DATA *chobj;
	OBJ_DATA *memorised;
	BOARD_DATA *board;			 /* The current board */
	time_t last_note[MAX_BOARD]; /* last note for the boards */
	NOTE_DATA *in_progress;
	list_head_t aliases;
	char *last_decap[2];
	char *pwd;
	char *bamfin;
	char *bamfout;
	char *title;
	char *conception;
	char *parents;
	char *cparents;
	char *marriage;
	char *switchname;
	char *decapmessage;
	char *loginmessage;
	char *logoutmessage;
	char *avatarmessage;
	char *tiemessage;
	int revision;
	int rune_count;
	int souls;
	int upgrade_level;
	int mean_paradox_counter;
	int relrank;
	int faith;
	int safe_counter;
	int perm_str;
	int perm_int;
	int perm_wis;
	int perm_dex;
	int perm_con;
	int mod_str;
	int mod_int;
	int mod_wis;
	int mod_dex;
	int mod_con;
	int jflags;
	int questsrun;
	int questtotal;
	int quest;
	int kingdom;
	int pagelen;
	int sit_safe;
	int mortal;
	int powers[20];
	int stats[12];
	int disc_points;
	int disc_research;
	bool lwrgen;
	int wolf;
	int rank;
	int demonic_a;
	int language[2];
	int stage[3];
	int wolfform[2];
	int score[6];
	int disc_a[11];
	int genes[10];
	int fake_skill;
	int fake_stance;
	int fake_hit;
	int fake_dam;
	int fake_hp;
	int fake_mana;
	int fake_move;
	int fake_ac;
	int obj_vnum;
	int condition[3];
	int learned[MAX_SKILL];
	int stat_ability[4];
	int stat_amount[4];
	int stat_duration[4];
	int exhaustion;
	int followers;
	int awins;	 /*  ARENA number of wins     */
	int alosses; /*  ARENA number of losses  */
	int comm;
	int security; /* OLC - Builder security */
	int bounty;
	int explevel; /* FTUE: 0=never MUD, 1=MUD not Dystopia, 2=veteran */
	bool stats_dirty; /* TRUE when pkill/pdeath/etc changed, triggers leaderboard update */
};

/*
 * Liquids.
 */
#define LIQ_WATER 0
#define LIQ_MAX	  16

struct liq_type {
	char *liq_name;
	char *liq_color;
	int liq_affect[3];
};

/*
 * Extra description data for a room or object.
 */
struct extra_descr_data {
	list_node_t node;
	char *keyword;			/* Keyword in look/examine          */
	char *description;		/* What to see                      */
};

/*
 * Prototype for an object.
 */
struct obj_index_data {
	OBJ_INDEX_DATA *next;
	list_head_t extra_descr;
	list_head_t affects;
	AREA_DATA *area; /* OLC */
	char *name;
	char *short_descr;
	char *description;
	char *chpoweron;
	char *chpoweroff;
	char *chpoweruse;
	char *victpoweron;
	char *victpoweroff;
	char *victpoweruse;
	char *questmaker;
	char *questowner;
	int vnum;
	int item_type;
	uint32_t extra_flags;
	uint32_t extra_flags2;
	uint32_t wear_flags;
	int count;
	int weight;
	uint32_t weapflags;
	int spectype;
	int specpower;
	int condition;
	int toughness;
	int resistance;
	int quest;
	int points;
	int cost; /* Unused */
	int value[4];
};

/*
 * One object.
 */
struct obj_data {
	list_node_t obj_node;
	list_node_t room_node;
	list_node_t content_node;
	list_head_t contents;
	OBJ_DATA *in_obj;
	CHAR_DATA *carried_by;
	CHAR_DATA *chobj;
	list_head_t extra_descr;
	list_head_t affects;
	OBJ_INDEX_DATA *pIndexData;
	ROOM_INDEX_DATA *in_room;
	char *name;
	char *short_descr;
	char *description;
	char *chpoweron;
	char *chpoweroff;
	char *chpoweruse;
	char *victpoweron;
	char *victpoweroff;
	char *victpoweruse;
	char *questmaker;
	char *questowner;
	int item_type;
	uint32_t extra_flags;
	uint32_t extra_flags2;
	uint32_t wear_flags;
	int wear_loc;
	int weight;
	uint32_t weapflags;
	int spectype;
	int specpower;
	int condition;
	int toughness;
	int resistance;
	int quest;
	int points;
	int cost;
	int level;
	int timer;
	int value[4];
};

/*
 * Exit data.
 */
struct exit_data {
	EXIT_DATA *rexit;		  /* Reverse exit pointer		*/
	ROOM_INDEX_DATA *to_room; /* Pointer to destination room	*/
	char *keyword;			  /* Keywords for exit or door	*/
	char *description;		  /* Description of exit		*/
	int vnum;				  /* Vnum of room exit leads to	*/
	int rvnum;				  /* Vnum of room in opposite dir	*/
	uint32_t exit_info;			  /* door states & other flags	*/
	int key;				  /* Key vnum			*/
	int vdir;			  /* 0,5 N\E\S\W\U\D shit		*/
	int rs_flags;			  /* OLC */
	int orig_door;			  /* OLC */
};

/*
 * Room text checking data.
 */
typedef struct roomtext_data {
	int type;
	int power;
	int mob;
	char *input;
	char *output;
	char *choutput;
	char *name;
	list_node_t node;
} ROOMTEXT_DATA;

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct reset_data {
	list_node_t node;
	char command;
	int arg1;
	int arg2;
	int arg3;
};

/*
 * Area definition.
 */
struct area_data {
	list_node_t node;
	char *name;
	int recall;
	int age;
	int nplayer;
	char *filename; /* OLC */
	char *builders; /* OLC - Listing of builders */
	int security;	/* OLC - Value 0-infinity  */
	int lvnum;		/* OLC - Lower vnum */
	int uvnum;		/* OLC - Upper vnum */
	int vnum;		/* OLC - Area vnum  */
	int area_flags; /* OLC */

	/* Runtime difficulty stats - not saved to area files */
	int mob_count;		 /* Number of mobs in area */
	int avg_mob_level;	 /* Average mob level */
	int min_mob_level;	 /* Lowest mob level */
	int max_mob_level;	 /* Highest mob level */
	int avg_difficulty;	 /* Calculated difficulty score */
	int difficulty_tier; /* 0=trivial,1=easy,2=normal,3=hard,4=deadly */
	bool is_hidden;		 /* Hidden from player areas list */
	bool needs_reset;	 /* Deferred reset: true when area should reset on player entry */

	/* Room list for efficient area reset (avoids sparse vnum iteration) */
	ROOM_INDEX_DATA *room_first;  /* Head of linked list of rooms in this area */
	int room_count;               /* Number of rooms in this area */

	/* Per-area profiling statistics (reset with profile reset) */
	long profile_reset_count;     /* Times this area was reset during profiling */
	long profile_reset_time_us;   /* Total microseconds spent resetting this area */
};

/*
 * Room type.
 */
struct room_index_data {
	ROOM_INDEX_DATA *next;
	ROOM_INDEX_DATA *next_room;
	list_head_t characters;
	list_head_t objects;
	list_head_t extra_descr;
	AREA_DATA *area;
	EXIT_DATA *exit[6];
	list_head_t roomtext;
	list_head_t resets;      /* OLC */

	char *track[5];
	char *name;
	char *description;
	int vnum;
	uint32_t room_flags;
	int light;
	int blood;
	int track_dir[5];
	int sector_type;
	int tick_timer[MAX_RTIMER];

	/* Area room list linkage for efficient reset iteration */
	ROOM_INDEX_DATA *next_in_area;  /* Next room in same area */
};

/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED -1
#define TYPE_HIT	   1000

/*
 *  Target types.
 */
#define TAR_IGNORE		   0
#define TAR_CHAR_OFFENSIVE 1
#define TAR_CHAR_DEFENSIVE 2
#define TAR_CHAR_SELF	   3
#define TAR_OBJ_INV		   4

#define TAR_OBJ_CHAR_DEF 5
#define TAR_OBJ_CHAR_OFF 6
#define TAR_OBJ_ROOM	 7
#define TAR_EXIT		 8
#define TAR_CHAR_WORLD	 9

#define TARGET_CHAR 0
#define TARGET_OBJ	1
#define TARGET_ROOM 2
#define TARGET_NONE 3

#define PURPLE_MAGIC 0
#define RED_MAGIC	 1
#define BLUE_MAGIC	 2
#define GREEN_MAGIC	 3
#define YELLOW_MAGIC 4

/*
 * Skills include spells as a particular case.
 */
struct skill_type {
	char *name;				 /* Name of skill		*/
	int skill_level;		 /* Level needed by class	*/
	SPELL_FUN *spell_fun;	 /* Spell pointer (for spells)	*/
	int target;			 /* Legal targets		*/
	int minimum_position; /* Position for caster / user	*/
	int *pgsn;			 /* Pointer to associated gsn	*/
	int slot;			 /* Slot for #OBJECT loading	*/
	int min_mana;		 /* Minimum mana used		*/
	int beats;			 /* Waiting time after use	*/
	char *noun_damage;		 /* Damage message		*/
	char *msg_off;			 /* Wear off message		*/
};

/*
 * These are skill_lookup return values for common skills and spells.
 */
extern int gsn_bash;
extern int gsn_smack;
extern int gsn_thwack;
extern int gsn_telekinetic;
extern int gsn_plasma;
extern int gsn_potato;
extern int gsn_mocha;
extern int gsn_thrownpie;
extern int gsn_stuntubes;
extern int gsn_laser;
extern int gsn_stinger;
extern int gsn_quills;
extern int gsn_cheapshot;
extern int gsn_shred;
extern int gsn_heavenlyaura;
extern int gsn_bladespin;
extern int gsn_fiery;
extern int gsn_hooves;
extern int gsn_claws;
extern int gsn_fireball;
extern int gsn_tentacle;
extern int gsn_lightning;
extern int gsn_supreme;
extern int gsn_deathaura;
extern int gsn_lightningslash;
extern int gsn_wrathofgod;
extern int gsn_darktendrils;
extern int gsn_mageshield;
extern int gsn_breath;
extern int gsn_venomtong;
extern int gsn_spiketail;
extern int gsn_badbreath;
extern int gsn_magma;
extern int gsn_hellfire;
extern int gsn_shards;
extern int gsn_spiderform;
extern int gsn_garotte;
extern int gsn_backstab;
extern int gsn_hide;
extern int gsn_peek;
extern int gsn_pick_lock;
extern int gsn_sneak;
extern int gsn_steal;
extern int gsn_godbless;	  /* Vic - Monks */
extern int gsn_totalblind; /* Vic - Monks */
extern int gsn_tendrils;
extern int gsn_berserk;
extern int gsn_punch;
extern int gsn_headbutt;
extern int gsn_spiket;
extern int gsn_venomt;
extern int gsn_magma;
extern int gsn_shards;
extern int gsn_shiroken;
extern int gsn_blinky;
extern int gsn_inferno;
extern int gsn_fangs;
extern int gsn_buffet;
extern int gsn_rfangs;
extern int gsn_sweep;
extern int gsn_knee;
extern int gsn_spinkick;
extern int gsn_backfist;
extern int gsn_jumpkick;
extern int gsn_monksweep;
extern int gsn_thrustkick;
extern int gsn_spinkick;
extern int gsn_elbow;
extern int gsn_palmstrike;
extern int gsn_shinkick;
extern int gsn_lightningkick;
extern int gsn_tornadokick;
extern int gsn_disarm;
extern int gsn_hurl;
extern int gsn_kick;
extern int gsn_chillhand;
extern int gsn_circle;
extern int gsn_booming;
extern int gsn_rescue;
extern int gsn_track;
extern int gsn_polymorph;
extern int gsn_web;
extern int gsn_infirmity;
extern int gsn_drowfire;
extern int gsn_blindness;
extern int gsn_charm_person;
extern int gsn_curse;
extern int gsn_invis;
extern int gsn_mass_invis;
extern int gsn_poison;
extern int gsn_sleep;
extern int gsn_paradox;
extern int gsn_spew;
extern int gsn_darkness;
extern int gsn_multiplearms;

/*
 * Utility macros.
 */
static inline int UMIN( int a, int b ) { return a < b ? a : b; }
static inline int UMAX( int a, int b ) { return a > b ? a : b; }
static inline int URANGE( int a, int b, int c ) { return b < a ? a : ( b > c ? c : b ); }
#define IS_SET( flag, bit )	   ( ( flag ) & ( bit ) )
#define SET_BIT( var, bit )	   ( ( var ) |= ( bit ) )
#define REMOVE_BIT( var, bit ) ( ( var ) &= ~( bit ) )
#define TOGGLE_BIT( var, bit ) ( ( var ) ^= ( bit ) )

/*
 * Character macros.
 */

#define IS_COMB( ch, sn )	  ( IS_SET( ( ch )->monkcrap, ( sn ) ) )
#define IS_FS( ch, sn )		  ( IS_SET( ( ch )->monkstuff, ( sn ) ) )
#define IS_NEWFLAG( ch, sn )  ( IS_SET( ( ch )->flag2, ( sn ) ) )
#define IS_CREATOR( ch )	  ( get_trust( ch ) >= MAX_LEVEL )
#define GET_FORM( ch )		  ( ( form_data[( ch )->cur_form].short_desc == NULL || form_data[( ch )->cur_form].short_desc[0] == '\0' ) ? form_data[( ch )->cur_form].name : "" )
#define GET_PROPER_NAME( ch ) ( IS_NPC( ( ch ) ) ? ( ch )->short_descr : ( ch )->pcdata->switchname )
#define GET_PC_NAME( ch )	  ( IS_NPC( ( ch ) ) ? "<npc>" : ( ch )->pcdata->switchname )

#define IS_NPC( ch )		  ( IS_SET( ( ch )->act, ACT_IS_NPC ) )
#define IS_SCREENREADER( ch ) ( !IS_NPC( ch ) && IS_SET( ( ch )->act, PLR_SCREENREADER ) )
#define IS_TRUECOLOR( ch )   ( !IS_NPC( ch ) && IS_EXTRA( ( ch ), EXTRA_TRUECOLOR ) )
#define IS_JUDGE( ch )	  ( get_trust( ch ) >= LEVEL_JUDGE )
#define IS_IMMORTAL( ch ) ( get_trust( ch ) >= LEVEL_IMMORTAL )
#define IS_HERO( ch )	  ( get_trust( ch ) >= LEVEL_HERO )

#define IS_AFFECTED( ch, sn ) ( IS_SET( ( ch )->affected_by, ( sn ) ) )
#define IS_AFF2( ch, sn )	  ( IS_SET( ( ch )->affected_by2, ( sn ) ) )
#define IS_SPEAKING( ch, sn ) ( IS_SET( ( ch )->pcdata->language[0], ( sn ) ) )
#define CAN_SPEAK( ch, sn )	  ( IS_SET( ( ch )->pcdata->language[1], ( sn ) ) )
#define IS_ITEMAFF( ch, sn )  ( IS_SET( ( ch )->itemaffect, ( sn ) ) )
#define IS_IMMUNE( ch, sn )	  ( IS_SET( ( ch )->immune, ( sn ) ) )
#define IS_VAMPAFF( ch, sn )  ( IS_SET( ( ch )->pcdata->stats[UNI_AFF], ( sn ) ) )
#define IS_VAMPPASS( ch, sn ) ( IS_SET( ( ch )->pcdata->stats[UNI_CURRENT], ( sn ) ) )
#define IS_FORM( ch, sn )	  ( IS_SET( ( ch )->form, ( sn ) ) )
#define IS_POLYAFF( ch, sn )  ( IS_SET( ( ch )->polyaff, ( sn ) ) )
#define IS_EXTRA( ch, sn )	  ( IS_SET( ( ch )->extra, ( sn ) ) )
#define IS_STANCE( ch, sn )	  ( ch->stance[0] == sn )
#define IS_DEMPOWER( ch, sn ) ( IS_SET( ( ch )->pcdata->powers[DPOWER_FLAGS], ( sn ) ) )
#define IS_DEMAFF( ch, sn )	  ( IS_SET( ( ch )->pcdata->powers[DPOWER_CURRENT], ( sn ) ) )
#define IS_CLASS( ch, CLASS ) ( IS_SET( ( ch )->class, CLASS ) && ( ch->level >= LEVEL_AVATAR ) )
#define IS_HEAD( ch, sn )	  ( IS_SET( ( ch )->loc_hp[0], ( sn ) ) )
#define IS_BODY( ch, sn )	  ( IS_SET( ( ch )->loc_hp[1], ( sn ) ) )
#define IS_ARM_L( ch, sn )	  ( IS_SET( ( ch )->loc_hp[2], ( sn ) ) )
#define IS_ARM_R( ch, sn )	  ( IS_SET( ( ch )->loc_hp[3], ( sn ) ) )
// #define IS_ARM_T(ch, sn)	(IS_SET((ch)->loc_hp[7], (sn)))
// #define IS_ARM_F(ch, sn)	(IS_SET((ch)->loc_hp[8], (sn)))
#define IS_LEG_L( ch, sn )	  ( IS_SET( ( ch )->loc_hp[4], ( sn ) ) )
#define IS_LEG_R( ch, sn )	  ( IS_SET( ( ch )->loc_hp[5], ( sn ) ) )
#define IS_BLEEDING( ch, sn ) ( IS_SET( ( ch )->loc_hp[6], ( sn ) ) )

#define IS_PLAYING( d ) ( d->connected == CON_PLAYING )

#define IS_GOOD( ch )	 ( ch->alignment >= 1 )
#define IS_EVIL( ch )	 ( ch->alignment <= -1 )
#define IS_NEUTRAL( ch ) ( !IS_GOOD( ch ) && !IS_EVIL( ch ) )

#define IS_AWAKE( ch )	  ( ch->position > POS_SLEEPING )

#define IS_OUTSIDE( ch ) ( !IS_SET( \
	( ch )->in_room->room_flags,    \
	ROOM_INDOORS ) )

int get_trust ( CHAR_DATA * ch );
static inline void WAIT_STATE( CHAR_DATA *ch, int npulse ) {
	if ( !IS_CREATOR( ch ) )
		ch->wait = UMAX( ch->wait, npulse );
}

/*
 * Object Macros.
 */
#define CAN_WEAR( obj, part )	  ( IS_SET( ( obj )->wear_flags, ( part ) ) )
#define IS_OBJ_STAT( obj, stat )  ( IS_SET( ( obj )->extra_flags, ( stat ) ) )
#define IS_OBJ_STAT2( obj, stat ) ( IS_SET( ( obj )->extra_flags2, ( stat ) ) )
#define IS_WEAP( obj, stat )	  ( IS_SET( ( obj )->weapflags, ( stat ) ) )

/*
 * Description macros.
 */
#define PERS( ch, looker ) ( can_see( looker, ( ch ) ) ? ( IS_NPC( ch ) ? ( ch )->short_descr                                                       \
																		: ( IS_AFFECTED( ( ch ), AFF_POLYMORPH ) ? ( ch )->morph : ( ch )->name ) ) \
													   : "someone" )

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

/*
 * Global constants.
 */

extern const struct str_app_type str_app[36];
extern const struct int_app_type int_app[36];
extern const struct wis_app_type wis_app[36];
extern const struct dex_app_type dex_app[36];
extern const struct con_app_type con_app[36];

extern const struct jope_type jope_table[];

extern const struct cmd_type cmd_table[];
extern struct liq_type liq_table[LIQ_MAX];
extern const struct skill_type skill_table[MAX_SKILL];
extern const char *discipline[MAX_DISCIPLINES];
extern const char *wwgift[MAX_GIFTS];
extern struct social_type social_table[];
extern int social_count;
/* Removed: xsocial_table extern - xsocial system removed */

/*
 * Global variables.
 */
extern list_head_t g_helps;
extern list_head_t ban_list;
extern list_head_t g_characters;
extern list_head_t g_descriptors;
extern list_head_t g_objects;

extern ROOM_INDEX_DATA *room_list;
extern list_head_t g_dns_lookups;
extern char bug_buf[];
extern time_t current_time;
extern time_t boot_time;
extern time_t last_copyover_time;
extern bool fLogAll;
extern FILE *fpReserve;
extern KILL_DATA kill_table[];
extern char log_buf[];
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;
extern int arena;
extern bool global_exp;
extern bool arena_open;
extern bool arena_base;
extern bool global_qp;
extern bool extra_log;
extern int players_logged;
extern int players_decap;
extern int players_gstolen;
extern int thread_count;
extern int iDelete;
extern bool arenaopen;
extern bool ragnarok;
extern int ragnarok_on_timer;
extern int ragnarok_safe_timer;
extern int ragnarok_cost;
extern int pulse_arena;
extern int pulse_doubleexp;
extern int pulse_doubleqp;

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */

/* Shop Commands */
DO_FUN do_buy;
DO_FUN do_sell;
DO_FUN do_list;
DO_FUN do_value;

/* New Commands */
DO_FUN do_affects;
DO_FUN do_chands;
DO_FUN do_resetpassword;
DO_FUN do_pstat;
DO_FUN do_implag;
DO_FUN do_doublexp;

DO_FUN do_showsilence;
DO_FUN do_showcompress;
DO_FUN do_openthearena;
DO_FUN do_ragnarok;
DO_FUN do_timer;

/* SPIDER DROID */
DO_FUN do_cubeform;
DO_FUN do_dridereq;
DO_FUN do_stuntubes;
DO_FUN do_infravision;
DO_FUN do_implant;

/* Mages */
DO_FUN do_thirdeye;
DO_FUN do_objectgate;
DO_FUN do_discharge;
DO_FUN do_enchant;

// Vampies
// Daimoinon
DO_FUN do_guardian;
DO_FUN do_fear;
DO_FUN do_portal;
DO_FUN do_curse;
DO_FUN do_vtwist;

// Melpominee
DO_FUN do_scream;

// Chimerstry
DO_FUN do_mirror;
DO_FUN do_formillusion;
DO_FUN do_control;
// DO_FUN do_objmask;

// Necromancy
DO_FUN do_thanatosis;
DO_FUN do_preserve;
DO_FUN do_spiritgate;
DO_FUN do_spiritguard;
DO_FUN do_zombie;

// Thanatosis
DO_FUN do_hagswrinkles;
DO_FUN do_rot;
DO_FUN do_withering;
DO_FUN do_drain;

// Auspex
DO_FUN do_truesight;
DO_FUN do_readaura;
DO_FUN do_scry;
DO_FUN do_astralwalk;
DO_FUN do_unveil;

// Obfuscate
DO_FUN do_vanish;
DO_FUN do_mask;
DO_FUN do_shield;
DO_FUN do_mortal;
DO_FUN do_objectmask;

/* Garou */

// Persuasion
DO_FUN do_staredown;
DO_FUN do_disquiet;
DO_FUN do_reshape;
DO_FUN do_cocoon;
DO_FUN do_quills;
DO_FUN do_burrow;
DO_FUN do_nightsight;
DO_FUN do_wither;
DO_FUN do_totemgift;

DO_FUN do_razorclaws;

/* End of Garou */

DO_FUN do_research;
DO_FUN do_disciplines;
DO_FUN do_top;
DO_FUN do_topclear;
DO_FUN do_classself;
DO_FUN do_knee;
DO_FUN do_elbow;
DO_FUN do_sweep;
DO_FUN do_reverse;
DO_FUN do_palmstrike;
DO_FUN do_shinkick;
DO_FUN do_chi;
DO_FUN do_thrustkick;
DO_FUN do_spinkick;
DO_FUN do_backfist;
DO_FUN do_spiritpower;
DO_FUN do_healingtouch;
DO_FUN do_deathtouch;
DO_FUN do_relax;
DO_FUN do_relset;

DO_FUN do_learn;
DO_FUN do_mitsukeru;
DO_FUN do_koryou;
DO_FUN do_udc;
DO_FUN do_labour;
DO_FUN do_abortion;
DO_FUN do_god_fight;
DO_FUN do_damn;
DO_FUN do_darkness;
DO_FUN do_refresh;
DO_FUN do_vt102;
DO_FUN do_retrn;
DO_FUN do_far;
DO_FUN do_dragon;
DO_FUN do_rot;
DO_FUN do_forget;
DO_FUN do_forge;
DO_FUN do_spew;
DO_FUN do_spiderform;
DO_FUN do_flash;
DO_FUN do_infirmity;
DO_FUN do_coil;
DO_FUN do_tide;
DO_FUN do_death;
DO_FUN do_acid;
DO_FUN do_awe;
DO_FUN do_satan;
/*ninjas*/
DO_FUN do_ninjaarmor;
DO_FUN do_hara_kiri;
DO_FUN do_miktalk;
DO_FUN do_principles;
DO_FUN do_michi;
DO_FUN do_kakusu;
DO_FUN do_kanzuite;
DO_FUN do_strangle;
DO_FUN do_mienaku;
DO_FUN do_flashbomb;
DO_FUN do_belt;
DO_FUN do_target;
DO_FUN do_bomuzite;
DO_FUN do_tsume;
DO_FUN do_circle;
DO_FUN do_omni;
DO_FUN do_frostbreath;
DO_FUN do_dgate;
DO_FUN do_tick;
DO_FUN do_resetarea;
DO_FUN do_graft;
DO_FUN do_blink;
DO_FUN do_dinferno;
DO_FUN do_immolate;
DO_FUN do_seed;
DO_FUN do_hellfire;
DO_FUN do_newbiepack;
DO_FUN do_runeeq;
DO_FUN do_afk;
DO_FUN do_unnerve;
DO_FUN do_wfreeze;
DO_FUN do_chaosport;
DO_FUN do_caust;
DO_FUN do_gust;
DO_FUN do_entomb;
DO_FUN do_leech;
DO_FUN do_configure;
DO_FUN do_stealsoul;
DO_FUN do_deathsense;
DO_FUN do_glamour;
DO_FUN do_prefix;

/* OLC Stuff Ooo */
DO_FUN do_hlist;
DO_FUN do_hedit;
DO_FUN do_hset;
DO_FUN do_ocreate;
DO_FUN do_mcreate;
DO_FUN do_redit;

/* Dunkirk's shit SuckDick */
DO_FUN do_bloodwall;
DO_FUN do_bloodrod;

DO_FUN do_sclaws;
/* Protean */
DO_FUN do_healing;

/* Obtene */
DO_FUN do_grab;
DO_FUN do_shadowgaze;

/* Luna Powers */

DO_FUN do_klaive;
DO_FUN do_tribe;
DO_FUN do_flameclaws;
DO_FUN do_moonarmour;
DO_FUN do_motherstouch;
DO_FUN do_gmotherstouch;
DO_FUN do_moongate;
DO_FUN do_moonbeam;

/* No more Luna Powers */

DO_FUN do_skin;
DO_FUN do_bonemod;
DO_FUN do_cauldron;
DO_FUN do_flamehands;
DO_FUN do_conceal;
DO_FUN do_drain;
DO_FUN do_shroud;
DO_FUN do_share;
DO_FUN do_frenzy;
DO_FUN do_chaosblast;
DO_FUN do_accept;
DO_FUN do_obtain;
DO_FUN do_warps;

/* End of Dunkirk's Shit */

DO_FUN do_activate;
DO_FUN do_alignment;

DO_FUN do_alias;
DO_FUN do_showalias;
DO_FUN do_removealias;

DO_FUN do_allow;
DO_FUN do_bounty;
DO_FUN do_bountylist;
DO_FUN do_ansi;
DO_FUN do_truecolor;
DO_FUN do_colortest;
DO_FUN do_areas;
DO_FUN do_artifact;
DO_FUN do_at;
DO_FUN do_autoexit;
DO_FUN do_autoloot;
DO_FUN do_autosac;
DO_FUN do_autosave;
DO_FUN do_autostance;
DO_FUN do_backstab;
DO_FUN do_bamfin;
DO_FUN do_bamfout;
DO_FUN do_ban;
DO_FUN do_beastlike;
DO_FUN do_berserk;
DO_FUN do_berserk2;
DO_FUN do_bind;
DO_FUN do_blank;
DO_FUN do_blindfold;
DO_FUN do_bloodline;
DO_FUN do_brandish;
DO_FUN do_breakup;
DO_FUN do_brief;
DO_FUN do_brief2;
DO_FUN do_brief3;
DO_FUN do_brief4;
DO_FUN do_bug;
DO_FUN do_call;
DO_FUN do_calm;
DO_FUN do_cast;
DO_FUN do_change;
DO_FUN do_changelight;
DO_FUN do_channels;
DO_FUN do_compress;
DO_FUN do_compres;
DO_FUN do_mxp;
DO_FUN do_gmcp;
DO_FUN do_protocols;
DO_FUN do_screenreader;
DO_FUN do_knightarmor;
DO_FUN do_gain;
DO_FUN do_weaponpractice;
DO_FUN do_aura;
DO_FUN do_powerword;
DO_FUN do_knighttalk;
DO_FUN do_ride;
DO_FUN do_unholyrite;
/* Mages */
DO_FUN do_chant;
DO_FUN do_reveal;
DO_FUN do_magics;
DO_FUN do_blast;
DO_FUN do_teleport;
DO_FUN do_invoke;
DO_FUN do_magearmor;
DO_FUN do_chaosmagic;

/* Angels */
DO_FUN do_gpeace;
DO_FUN do_innerpeace;
DO_FUN do_houseofgod;
DO_FUN do_angelicaura;
DO_FUN do_gbanish;
DO_FUN do_harmony;
DO_FUN do_gsenses;
DO_FUN do_gfavor;
DO_FUN do_forgivness;
DO_FUN do_martyr;
DO_FUN do_swoop;
DO_FUN do_awings;
DO_FUN do_halo;
DO_FUN do_sinsofthepast;
DO_FUN do_eyeforaneye;
DO_FUN do_angelicarmor;
DO_FUN do_angeltalk;
DO_FUN do_touchofgod;
DO_FUN do_spiritform;

/* Shapeshifters */
DO_FUN do_shift;
DO_FUN do_formlearn;
DO_FUN do_camouflage;
DO_FUN do_shapeshift;
DO_FUN do_hatform;
DO_FUN do_mistwalk;
DO_FUN do_shapearmor;
DO_FUN do_shaperoar;
DO_FUN do_charge;
DO_FUN do_faerieblink;
DO_FUN do_stomp;
DO_FUN do_faeriecurse;
DO_FUN do_phase;
DO_FUN do_breath;

DO_FUN do_monkarmor;
DO_FUN do_vampirearmor;
DO_FUN do_purification;
DO_FUN do_chaosblast;
DO_FUN do_chat;
DO_FUN do_claim;
DO_FUN do_flame;

DO_FUN do_arenastats;
DO_FUN do_arenajoin;
DO_FUN do_resign;

DO_FUN do_kingdoms;
DO_FUN do_ktalk;
DO_FUN do_kinduct;
DO_FUN do_wantkingdom;
DO_FUN do_kset;
DO_FUN do_koutcast;
DO_FUN do_kstats;
DO_FUN do_kleave;
DO_FUN do_kingset;

DO_FUN do_leader;
DO_FUN do_leaderclear;
DO_FUN do_clandisc;
DO_FUN do_introduce;
DO_FUN do_claws;
DO_FUN do_class;
DO_FUN do_clearstats;
DO_FUN do_clearstats2;
DO_FUN do_close;
DO_FUN do_command;
DO_FUN do_commands;
DO_FUN do_complete;
DO_FUN do_config;
DO_FUN do_consider;
DO_FUN do_cprompt;
DO_FUN do_crack;
DO_FUN do_create;
DO_FUN do_credits;
DO_FUN do_darkheart;
DO_FUN do_vampdarkness;
DO_FUN do_decapitate;
DO_FUN do_gifts;
DO_FUN do_trueform;
DO_FUN do_cone;
DO_FUN do_dstake;
DO_FUN do_demonarmour;
DO_FUN do_deny;
DO_FUN do_description;
DO_FUN do_diagnose;
DO_FUN do_dismount;
DO_FUN do_disable;
DO_FUN do_disarm;
DO_FUN do_disconnect;
DO_FUN do_divorce;
DO_FUN do_down;
DO_FUN do_draw;
DO_FUN do_drink;
DO_FUN do_drop;

DO_FUN do_settie;
DO_FUN do_setlogout;
DO_FUN do_setlogin;
DO_FUN do_setdecap;
DO_FUN do_setavatar;

// drows
DO_FUN send_grantlist;
DO_FUN do_drowcreate;
DO_FUN do_heal;
DO_FUN do_drowfire;
DO_FUN do_drowhate;
DO_FUN do_drowpowers;
DO_FUN do_drowshield;
DO_FUN do_lloth;
DO_FUN do_drowsight;
DO_FUN do_darktendrils;
DO_FUN do_fightdance;
DO_FUN do_ntrust;
DO_FUN do_offer;
DO_FUN do_grant;
DO_FUN do_burrow;
DO_FUN do_earthmeld;
DO_FUN do_east;
DO_FUN do_eat;
DO_FUN do_echo;
DO_FUN do_empty;
DO_FUN do_escape;
DO_FUN do_emote;
DO_FUN do_enter;
DO_FUN do_equipment;
DO_FUN do_evileye;
DO_FUN do_examine;
DO_FUN do_exits;
DO_FUN do_exlist;
DO_FUN do_eyespy;
DO_FUN do_familiar;
DO_FUN do_fangs;
DO_FUN do_favor;
DO_FUN do_favour;
DO_FUN do_fcommand;
DO_FUN do_fightstyle;
DO_FUN do_fileupdate;
DO_FUN do_fill;
DO_FUN do_finger;
DO_FUN do_flee;
DO_FUN do_flex;
DO_FUN do_follow;
DO_FUN do_force;
DO_FUN do_asperson;
DO_FUN do_offline;
DO_FUN do_forceauto;
DO_FUN do_freeze;
DO_FUN do_earthshatter;
DO_FUN do_confuse;
DO_FUN do_confusable;
DO_FUN do_nameban;
DO_FUN do_gag;
DO_FUN do_garotte;
DO_FUN do_dark_garotte;
DO_FUN do_get;
DO_FUN do_generation;
DO_FUN do_gift;
DO_FUN do_give;
DO_FUN do_goto;
DO_FUN do_grant;
DO_FUN do_group;
DO_FUN do_gtell; /*
DO_FUN do_heal;*/
DO_FUN do_help;
DO_FUN do_hide;
DO_FUN do_hightalk;
DO_FUN do_monktalk;
DO_FUN do_holylight;
DO_FUN do_home;
DO_FUN do_horns;
DO_FUN do_howl;
DO_FUN do_telepath;
DO_FUN do_communicate;
DO_FUN do_huh;
DO_FUN do_humanform;
DO_FUN do_humanity;
DO_FUN do_hunt;
DO_FUN do_hurl;
DO_FUN do_idea;
DO_FUN do_immune;
DO_FUN do_immtalk;
DO_FUN do_inconnu;
DO_FUN do_info;
DO_FUN do_inpart;
DO_FUN do_demonarmour;
DO_FUN do_inventory;
DO_FUN do_invis;
DO_FUN do_kick;
DO_FUN do_godsfavor;

/* Tanar'ri */
DO_FUN do_earthquake;
DO_FUN do_tornado;
DO_FUN do_infernal;
DO_FUN do_bloodsac;
DO_FUN do_enmity;
DO_FUN do_enrage;
DO_FUN do_booming;
DO_FUN do_chaosgate;
DO_FUN do_fury;
DO_FUN do_tantalk;
DO_FUN do_taneq;
DO_FUN do_lavablast;
DO_FUN do_chaossurge;

/* Power Lich */
DO_FUN do_lore;
DO_FUN do_studylore;
DO_FUN do_lichtalk;
DO_FUN do_chaosshield;
DO_FUN do_summongolem;
DO_FUN do_planartravel;
DO_FUN do_planarstorm;
DO_FUN do_powertransfer;
DO_FUN do_polarity;
DO_FUN do_chillhand;
DO_FUN do_creepingdoom;
DO_FUN do_painwreck;
DO_FUN do_earthswallow;
DO_FUN do_licharmor;
DO_FUN do_soulsuck;
DO_FUN do_pentagram;
DO_FUN do_planeshift;

/* Monk */
DO_FUN do_reseteq;
DO_FUN do_mantra;
DO_FUN do_wrathofgod;
DO_FUN do_guide;
DO_FUN do_makezerg;
DO_FUN do_monk;
DO_FUN do_zergpower;
DO_FUN do_zclan;
DO_FUN do_superinvis;
DO_FUN do_supershield;
DO_FUN do_celestial;
DO_FUN do_godseye;
DO_FUN do_godsbless;
DO_FUN do_flaminghands;
DO_FUN do_cloak;
DO_FUN do_prayofages;
DO_FUN do_darkblaze;
DO_FUN do_adamantium;
DO_FUN do_steelskin;
DO_FUN do_godsheal;
DO_FUN do_ghold;
DO_FUN do_callgod;

DO_FUN do_holytrain;
DO_FUN do_holyvision;
DO_FUN do_prayer;
DO_FUN do_palmthrust;
DO_FUN do_sacredinvis;
DO_FUN do_kill;
DO_FUN do_combatswitch;
DO_FUN do_killperson;
DO_FUN do_leap;
DO_FUN do_lifespan;
DO_FUN do_locate;
DO_FUN do_lock;
DO_FUN do_log;
DO_FUN do_look;
DO_FUN do_majesty;
DO_FUN do_makepreg;
DO_FUN do_map;
DO_FUN do_amap;
DO_FUN do_marry;
DO_FUN do_mask;
DO_FUN do_meditate;
DO_FUN do_memory;
DO_FUN do_mfind;
DO_FUN do_mload;
DO_FUN do_mount;
DO_FUN do_mortal;
DO_FUN do_mortalvamp;
DO_FUN do_undeny;
DO_FUN do_mstat;
DO_FUN do_mwhere;
DO_FUN do_music;
DO_FUN do_nightsight;
DO_FUN do_north;
DO_FUN do_note;
DO_FUN do_oclone;
DO_FUN do_ofind;
DO_FUN do_oload;
DO_FUN do_open;
DO_FUN do_order;
DO_FUN do_oreturn;
DO_FUN do_pedit;
DO_FUN do_outcast;
DO_FUN do_reimb;
DO_FUN do_reimburse;
DO_FUN do_oset;
DO_FUN do_ostat;
DO_FUN do_oswitch;
DO_FUN do_otransfer;
DO_FUN do_champions;
DO_FUN do_paradox;
DO_FUN do_bully;
DO_FUN do_password;
DO_FUN do_peace;
DO_FUN do_pick;
DO_FUN do_plasma;
DO_FUN do_pload;
DO_FUN do_poison;
DO_FUN do_possession;
DO_FUN do_practice;
DO_FUN do_pray;
DO_FUN do_press;
DO_FUN do_preturn;
DO_FUN do_prompt;
DO_FUN do_propose;
DO_FUN do_pull;
DO_FUN do_punch;
DO_FUN do_purge;
DO_FUN do_put;
DO_FUN do_qmake;
DO_FUN do_quaff;
DO_FUN do_qset;
DO_FUN do_qstat;
DO_FUN do_qtrust;
DO_FUN do_quest;
DO_FUN do_qui;
DO_FUN do_quit;
DO_FUN do_rage;
DO_FUN do_readaura;
DO_FUN do_copyover;
DO_FUN do_recall;
DO_FUN do_recharge;
DO_FUN do_recho;
DO_FUN do_recite;
DO_FUN do_regenerate;
DO_FUN do_release;
DO_FUN do_relevel;
DO_FUN do_superadmin;
DO_FUN do_pretitle;
DO_FUN do_nike;
DO_FUN do_hreload;
DO_FUN do_remove;
DO_FUN do_rent;
DO_FUN do_reply;
DO_FUN do_report;
DO_FUN do_rescue;
DO_FUN do_rest;
DO_FUN do_restore;
DO_FUN do_return;
DO_FUN do_roll;
DO_FUN do_rset;
DO_FUN do_rstat;
DO_FUN do_sacrifice;
DO_FUN do_safe;
DO_FUN do_save;
DO_FUN do_say;
DO_FUN do_scan;
DO_FUN do_stat;
DO_FUN do_spit;
DO_FUN do_tongue;
DO_FUN do_mindblast;
DO_FUN do_powers;
DO_FUN do_score;
DO_FUN do_weaplist;
DO_FUN do_level;
DO_FUN do_mastery;
DO_FUN do_exp;
DO_FUN do_upgrade;
DO_FUN do_healme;
DO_FUN do_pkpowers;
DO_FUN do_gensteal;
DO_FUN do_setstance;

DO_FUN do_multicheck;

DO_FUN do_mudstat;
DO_FUN do_profile;
DO_FUN do_scry;
DO_FUN do_pkscry;
DO_FUN do_pkmind;
DO_FUN do_pkcall;
DO_FUN do_pkheal;
DO_FUN do_pkaura;
DO_FUN do_pkportal;
DO_FUN do_pkobjscry;
DO_FUN do_pkvision;

DO_FUN do_serenity;
DO_FUN do_serpent;
DO_FUN do_shadowplane;
DO_FUN do_shadowsight;
DO_FUN do_shadowstep;
DO_FUN do_shadowwalk;
DO_FUN do_sheath;
DO_FUN do_shield;
DO_FUN do_shutdow;
DO_FUN do_shutdown;
DO_FUN do_side;
DO_FUN do_learn;
DO_FUN do_stalk;
DO_FUN do_bladespin;
DO_FUN do_hologramtransfer;
DO_FUN do_sign;

DO_FUN do_martial;
DO_FUN do_katana;
DO_FUN do_focus;
DO_FUN do_slide;
DO_FUN do_sidestep;
DO_FUN do_block;
DO_FUN do_countermove;

DO_FUN do_dktalk;
DO_FUN do_newbie;
DO_FUN do_silence;
DO_FUN do_sit;
DO_FUN do_skill;
DO_FUN do_sla;
DO_FUN do_slay;
DO_FUN do_slay2;
DO_FUN do_sleep;
DO_FUN do_slookup;
DO_FUN do_spell;
DO_FUN do_tendrils;
DO_FUN do_lamprey;
DO_FUN do_stake;
DO_FUN do_stance;
DO_FUN do_smother;
DO_FUN do_sneak;
DO_FUN do_snoop;
DO_FUN do_socials;
DO_FUN do_south;
DO_FUN do_spy;
DO_FUN do_spydirection;
DO_FUN do_stand;
DO_FUN do_steal;
DO_FUN do_summon;
DO_FUN do_nosummon;
DO_FUN do_notravel;
DO_FUN do_switch;
DO_FUN do_embrace;
DO_FUN do_diablerise;
DO_FUN do_entrance;
DO_FUN do_fleshcraft;
DO_FUN do_zombie;
DO_FUN do_quills;
DO_FUN do_perception;
DO_FUN do_jawlock;
DO_FUN do_rend;
DO_FUN do_hardskin;
DO_FUN do_slam;
DO_FUN do_roar;
DO_FUN do_shred;

DO_FUN do_talons;
DO_FUN do_devour;
DO_FUN do_inferno;
DO_FUN do_wall;
DO_FUN do_facade;
DO_FUN do_baal;
DO_FUN do_obj;
DO_FUN do_dragonform;
DO_FUN do_bloodwater;
DO_FUN do_gourge;
DO_FUN do_sharpen;
DO_FUN do_dub;
DO_FUN do_shroud;
DO_FUN do_taste;
DO_FUN do_assassinate;
DO_FUN do_offer;
DO_FUN do_gate;
DO_FUN do_pigeon;
DO_FUN do_bloodagony;
DO_FUN do_decay;
DO_FUN do_tell;
DO_FUN do_theft;
DO_FUN do_throw;
DO_FUN do_tie;
DO_FUN do_time;
DO_FUN do_title;
DO_FUN do_token;
DO_FUN do_totems;
DO_FUN do_track;
DO_FUN do_tradition;
DO_FUN do_train;
DO_FUN do_transfer;
DO_FUN do_transport;
DO_FUN do_travel;
DO_FUN do_ztravel;
DO_FUN do_truesight;
DO_FUN do_trust;
DO_FUN do_turn;
DO_FUN do_twist;
DO_FUN do_typo;
DO_FUN do_unlock;
DO_FUN do_unpolymorph;
DO_FUN do_untie;
DO_FUN do_unwerewolf;
DO_FUN do_up;
DO_FUN do_upkeep;
DO_FUN do_users;
DO_FUN do_mudclients;
DO_FUN do_dwho;
DO_FUN do_vampire;
DO_FUN do_vamptalk;
DO_FUN do_tail;
DO_FUN do_hooves;
DO_FUN do_magetalk;
DO_FUN do_vanish;
DO_FUN do_version;
DO_FUN do_visible;
DO_FUN do_voodoo;
DO_FUN do_web;
DO_FUN do_wake;
DO_FUN do_watcher;
DO_FUN do_watching;
DO_FUN do_weaponform;
DO_FUN do_wear;
DO_FUN do_wearaffect;
DO_FUN do_werewolf;
DO_FUN do_west;
DO_FUN do_where;
DO_FUN do_whisper;
DO_FUN do_who;
DO_FUN do_wings;
DO_FUN do_wizhelp;
DO_FUN do_linkdead;
DO_FUN do_wizlist;
DO_FUN do_closemud;
DO_FUN do_wizlock;
/* Removed: do_xemote, do_xsocials - xsocial system removed */
DO_FUN do_yell;
DO_FUN do_zap;
DO_FUN do_zuloform;
DO_FUN do_demonform;
DO_FUN do_beckon;
DO_FUN do_illusion;
DO_FUN do_cserpent;
DO_FUN do_scales;
DO_FUN do_guardian;
DO_FUN do_servant;
/* Removed: do_contraception - xsocial system removed */
DO_FUN do_relearn;

DO_FUN do_gameconfig;
DO_FUN do_mcmptest;
DO_FUN do_audioconfig;
DO_FUN do_cfg;

/*

DO_FUN do_weapmod;
*/
/*
 * Spell functions.
 * Defined in magic.c.
 */
SPELL_FUN spell_contraception;
SPELL_FUN spell_spew;
SPELL_FUN spell_infirmity;
SPELL_FUN spell_null;
SPELL_FUN spell_make_bag;
SPELL_FUN spell_acid_blast;

// SPELL_FUN spell_tendrils;

SPELL_FUN spell_armor;
SPELL_FUN spell_godbless;
SPELL_FUN spell_bless;
SPELL_FUN spell_blindness;
SPELL_FUN spell_burning_hands;
SPELL_FUN spell_call_lightning;
SPELL_FUN spell_cause_critical;
SPELL_FUN spell_cause_light;
SPELL_FUN spell_cause_serious;
SPELL_FUN spell_change_sex;
SPELL_FUN spell_charm_person;
SPELL_FUN spell_chill_touch;
SPELL_FUN spell_colour_spray;
SPELL_FUN spell_continual_light;
SPELL_FUN spell_control_weather;
SPELL_FUN spell_create_food;
SPELL_FUN spell_create_spring;
SPELL_FUN spell_create_water;
SPELL_FUN spell_cure_blindness;
SPELL_FUN spell_cure_critical;
SPELL_FUN spell_cure_light;
SPELL_FUN spell_cure_poison;
SPELL_FUN spell_cure_serious;
SPELL_FUN spell_curse;
SPELL_FUN spell_darkness;
SPELL_FUN spell_detect_evil;
SPELL_FUN spell_detect_hidden;
SPELL_FUN spell_detect_invis;
SPELL_FUN spell_detect_magic;
SPELL_FUN spell_detect_poison;
SPELL_FUN spell_dispel_evil;
SPELL_FUN spell_dispel_magic;
SPELL_FUN spell_drowfire;
SPELL_FUN spell_earthquake;
SPELL_FUN spell_enchant_weapon;
SPELL_FUN spell_enchant_armor;
SPELL_FUN spell_energy_drain;
SPELL_FUN spell_faerie_fire;
SPELL_FUN spell_faerie_fog;
SPELL_FUN spell_fireball;
SPELL_FUN spell_visage;
SPELL_FUN spell_desanct;
SPELL_FUN spell_imp_heal;
SPELL_FUN spell_imp_fireball;
SPELL_FUN spell_imp_faerie_fire;
SPELL_FUN spell_imp_teleport;
SPELL_FUN spell_flamestrike;
SPELL_FUN spell_fly;
SPELL_FUN spell_gate;
SPELL_FUN spell_general_purpose;
SPELL_FUN spell_giant_strength;
SPELL_FUN spell_harm;
SPELL_FUN spell_heal;
SPELL_FUN spell_group_heal;
SPELL_FUN spell_high_explosive;
SPELL_FUN spell_identify;
SPELL_FUN spell_readaura;
SPELL_FUN spell_infravision;
SPELL_FUN spell_invis;
SPELL_FUN spell_know_alignment;
SPELL_FUN spell_lightning_bolt;
SPELL_FUN spell_locate_object;
SPELL_FUN spell_magic_missile;
SPELL_FUN spell_mass_invis;
SPELL_FUN spell_pass_door;
SPELL_FUN spell_poison;
SPELL_FUN spell_protection;
SPELL_FUN spell_protection_vs_good;
SPELL_FUN spell_refresh;
SPELL_FUN spell_remove_curse;
SPELL_FUN spell_sanctuary;
SPELL_FUN spell_shocking_grasp;
SPELL_FUN spell_shield;
SPELL_FUN spell_sleep;
SPELL_FUN spell_stone_skin;
SPELL_FUN spell_summon;
SPELL_FUN spell_teleport;
SPELL_FUN spell_ventriloquate;
SPELL_FUN spell_weaken;
SPELL_FUN spell_word_of_recall;
SPELL_FUN spell_acid_breath;
SPELL_FUN spell_fire_breath;
SPELL_FUN spell_frost_breath;
SPELL_FUN spell_gas_breath;
SPELL_FUN spell_godbless;
SPELL_FUN spell_lightning_breath;
SPELL_FUN spell_cone;
SPELL_FUN spell_guardian;
SPELL_FUN spell_soulblade;
SPELL_FUN spell_mana;
SPELL_FUN spell_frenzy;
SPELL_FUN spell_darkblessing;
SPELL_FUN spell_foodfrenzy;
SPELL_FUN spell_portal;
SPELL_FUN spell_energyflux;
SPELL_FUN spell_voodoo;
SPELL_FUN spell_transport;
SPELL_FUN spell_regenerate;
SPELL_FUN spell_clot;
SPELL_FUN spell_mend;
SPELL_FUN spell_quest;
SPELL_FUN spell_minor_creation;
SPELL_FUN spell_spiritkiss;
SPELL_FUN spell_brew;
SPELL_FUN spell_jailwater;
SPELL_FUN spell_scribe;
SPELL_FUN spell_carve;
SPELL_FUN spell_engrave;
SPELL_FUN spell_bake;
SPELL_FUN spell_mount;
SPELL_FUN spell_scan;
SPELL_FUN spell_repair;
SPELL_FUN spell_spellproof;
SPELL_FUN spell_preserve;
SPELL_FUN spell_major_creation;
SPELL_FUN spell_copy;
SPELL_FUN spell_insert_page;
SPELL_FUN spell_chaos_blast;
SPELL_FUN spell_resistance;
SPELL_FUN spell_web;
SPELL_FUN spell_polymorph;
SPELL_FUN spell_contraception;
SPELL_FUN spell_find_familiar;
SPELL_FUN spell_improve;
SPELL_FUN spell_clay;

/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.
 */
#if defined( _AIX )
char *crypt ( const char *key, const char *salt );
#endif

/* crypt() declaration - on Linux/Unix, comes from unistd.h or crypt.h
 * On Windows, crypt() is defined in compat.h as win32_crypt_sha256
 */
#if defined( linux ) || defined( __linux__ )
char *crypt ( const char *key, const char *salt );
#endif

/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 * On Windows, crypt() is defined in compat.h as win32_crypt_sha256
 */
#if defined( NOCRYPT )
#define crypt( s1, s2 ) ( s1 )
#endif

/*
 * Data files used by the server.
 *
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */

/*
 * Path management - allows executable to run from any directory
 */
#define MUD_PATH_MAX 512

#if defined( WIN32 )
#define PATH_SEPARATOR "\\"
#define NULL_FILE	   "nul"
#else
#define PATH_SEPARATOR "/"
#define NULL_FILE	   "/dev/null"
#endif

/* Base directory paths - initialized at startup */
extern char mud_base_dir[MUD_PATH_MAX];	  /* Base directory (gamedata/) */
extern char mud_run_dir[MUD_PATH_MAX];	  /* Runtime files (copyover, crash, shutdown) */
extern char mud_log_dir[MUD_PATH_MAX];	  /* Log files */

/* Path building function - returns static buffer, use immediately or copy */
char *mud_path( const char *dir, const char *filename );

/* Initialize all paths based on executable location or current directory */
void mud_init_paths( const char *exe_path );

#define SHUTDOWN_FILE	mud_path( mud_run_dir, "shutdown.txt" )
#define CRASH_TEMP_FILE mud_path( mud_run_dir, "crashtmp.txt" )
#define COPYOVER_FILE	mud_path( mud_run_dir, "copyover.data" )
#define UPTIME_FILE		mud_path( mud_run_dir, "uptime.dat" )

/* Windows socket info file for copyover (WSADuplicateSocket data) */
#if defined( WIN32 )
#define COPYOVER_SOCKET_FILE mud_path( mud_run_dir, "copyover_socket.dat" )
#endif

/* Executable file for copyover/crash recovery */
#if defined( WIN32 )
#define EXE_FILE	 mud_path( mud_base_dir, "dystopia.exe" )
#define EXE_FILE_NEW mud_path( mud_base_dir, "dystopia_new.exe" )
#else
#define EXE_FILE mud_path( mud_base_dir, "dystopia" )
#endif

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */

/* act_comm.c */
void add_follower ( CHAR_DATA * ch, CHAR_DATA *master );
void stop_follower ( CHAR_DATA * ch );
void die_follower ( CHAR_DATA * ch );
bool is_same_group ( CHAR_DATA * ach, CHAR_DATA *bch );
void room_text ( CHAR_DATA * ch, char *argument );
char *strlower ( char *ip );
void excessive_cpu ( int blx );
bool check_parse_name ( char *name );

void room_message ( ROOM_INDEX_DATA * room, char *message );

/* act_info.c */
void set_title ( CHAR_DATA * ch, char *title );
void show_list_to_char ( void *list, CHAR_DATA *ch,
	bool fShort, bool fShowNothing, bool in_room );
int char_hitroll ( CHAR_DATA * ch );
int char_damroll ( CHAR_DATA * ch );
int char_ac ( CHAR_DATA * ch );

/* darkheart.c */

void set_pc_name ( CHAR_DATA * ch, char *title );
void set_switchname ( CHAR_DATA * ch, char *title );

/* act_move.c */
void move_char ( CHAR_DATA * ch, int door );
void open_lift ( CHAR_DATA * ch );
void close_lift ( CHAR_DATA * ch );
void move_lift ( CHAR_DATA * ch, int to_room );
void move_door ( CHAR_DATA * ch );
void thru_door ( CHAR_DATA * ch, int doorexit );
void open_door ( CHAR_DATA * ch, bool be_open );
bool is_open ( CHAR_DATA * ch );
bool same_floor ( CHAR_DATA * ch, int cmp_room );
void check_hunt ( CHAR_DATA * ch );

int disc_points_needed ( CHAR_DATA * ch );
void gain_disc_points ( CHAR_DATA * ch, int points );
ROOM_INDEX_DATA *get_random_room ( CHAR_DATA * ch );
ROOM_INDEX_DATA *get_rand_room ();
ROOM_INDEX_DATA *get_treemeld_room ();
ROOM_INDEX_DATA *get_rand_room_by_sect ( int sect );

/* act_obj.c */
bool is_ok_to_wear ( CHAR_DATA * ch, bool wolf_ok, char *argument );
void quest_object ( CHAR_DATA * ch, OBJ_DATA *obj );
bool remove_obj ( CHAR_DATA * ch, int iWear, bool fReplace );
void wear_obj ( CHAR_DATA * ch, OBJ_DATA *obj, bool fReplace );

/* act_wiz.c */
void get_reboot_string (void);
void bind_char ( CHAR_DATA * ch );
void logchan ( char *argument );

/* arena.c */
void close_arena (void);
void open_arena (void);

/* jobo_util.c */
void recycle_dns_lookups (void);

/* build.c */

char *copy_buffer ( CHAR_DATA * ch );
void edit_buffer ( CHAR_DATA * ch, char *argument );
char *strip_cr ( char *str );
void start_editing ( CHAR_DATA * ch, char *data );
void stop_editing ( CHAR_DATA * ch );

/* comm.c */
void boot_headless ( const char *exe_path );
void game_tick ( void );
void close_socket ( DESCRIPTOR_DATA * dclose );
void close_socket2 ( DESCRIPTOR_DATA * dclose, bool kickoff );
void write_to_buffer ( DESCRIPTOR_DATA * d, const char *txt,
	int length );
bool write_to_descriptor ( DESCRIPTOR_DATA * d, char *txt, int length );
bool process_output ( DESCRIPTOR_DATA * d, bool fPrompt );
const char *col_scale_code ( int current, int max );
const char *col_scale_code_tc ( int current, int max, CHAR_DATA *ch );
int rgb_to_xterm256 ( int r, int g, int b );
const char *rgb_to_ansi16 ( int r, int g, int b );
void send_to_char ( const char *txt, CHAR_DATA *ch );
void act ( const char *format, CHAR_DATA *ch,
	const void *arg1, const void *arg2, int type );
void act2 ( const char *format, CHAR_DATA *ch,
	const void *arg1, const void *arg2, int type );
void kavitem ( const char *format, CHAR_DATA *ch,
	const void *arg1, const void *arg2, int type );

void banner_to_char ( char *txt, CHAR_DATA *ch );
void banner2_to_char ( char *txt, CHAR_DATA *ch );
void divide_to_char ( CHAR_DATA * ch );
void divide2_to_char ( CHAR_DATA * ch );
void divide3_to_char ( CHAR_DATA * ch );
void divide4_to_char ( CHAR_DATA * ch );
void divide5_to_char ( CHAR_DATA * ch );
void divide6_to_char ( CHAR_DATA * ch );
void stc ( const char *txt, CHAR_DATA *ch );
void cent_to_char ( char *txt, CHAR_DATA *ch );

/* output.c - test output capture */
void test_output_start ( CHAR_DATA *ch );
const char *test_output_get ( void );
void test_output_clear ( void );
void test_output_stop ( void );

/*
 * Buffer-safe string copy helpers - prevent buffer overflows from color codes
 * Use these when building strings that may contain xterm color codes like #x###
 * which expand significantly during output processing.
 */

/*
 * Safe string copy into a buffer with bounds checking.
 * Returns updated pointer, or NULL if buffer full.
 * margin: bytes to reserve at end of buffer (for terminators, etc.)
 */
static inline char *buf_append_safe( char *dest, const char *src,
                                     char *buf_start, size_t buf_size, size_t margin ) {
	char *buf_end = buf_start + buf_size - margin;
	if ( dest >= buf_end || dest < buf_start )
		return NULL;
	while ( *src != '\0' && dest < buf_end )
		*dest++ = *src++;
	return dest;
}

/*
 * Check if there's room in buffer for at least 'needed' bytes.
 * margin: bytes to reserve at end of buffer.
 */
static inline bool buf_has_space( const char *ptr, const char *buf_start,
                                   size_t buf_size, size_t needed, size_t margin ) {
	return ( ptr + needed + margin ) <= ( buf_start + buf_size );
}

/* prototypes from interp.c */
void load_disabled (void);
void save_disabled (void);

/* db.c */
void init_mm ( void );
void boot_db ( bool fCopyOver );
void area_update (void);
int calculate_mob_difficulty ( MOB_INDEX_DATA * pMob );
void calculate_area_difficulty ( AREA_DATA * pArea );
void calculate_all_area_difficulties (void);
CHAR_DATA *create_mobile ( MOB_INDEX_DATA * pMobIndex );
OBJ_DATA *create_object ( OBJ_INDEX_DATA * pObjIndex, int level );
void clear_char ( CHAR_DATA * ch );
void free_char ( CHAR_DATA * ch );
char *get_extra_descr ( char *name, list_head_t *ed_list );
MOB_INDEX_DATA *get_mob_index ( int vnum );
OBJ_INDEX_DATA *get_obj_index ( int vnum );
ROOM_INDEX_DATA *get_room_index ( int vnum );
void mem_debug_check_freelists ( void );
char *str_dup ( const char *str );
int number_fuzzy ( int number );
int number_range ( int from, int to );
int number_percent (void);
int number_door (void);
int number_bits ( int width );
int number_mm (void);
int dice ( int number, int size );
int interpolate ( int level, int value_00, int value_32 );
void smash_tilde ( char *str );
int str_cmp ( const char *astr, const char *bstr );
int str_prefix ( const char *astr, const char *bstr );
int str_infix ( const char *astr, const char *bstr );
int str_suffix ( const char *astr, const char *bstr );
char *capitalize ( const char *str );
int visible_strlen ( const char *str );
void pad_to_visible_width ( char *dest, size_t destsize, const char *src, int target_width );
const char *mask_ip ( const char *ip );
void append_file ( CHAR_DATA * ch, char *file, char *str );
void bug ( const char *str, int param );
void log_string ( const char *str );
void log_flush  ( void );
void tail_chain (void);

void add_help ( HELP_DATA * pHelp );
HELP_DATA *get_help ( CHAR_DATA *ch, char *argument );
char *strupper ( const char *str );
/*
ROOM_INDEX_DATA *make_room   ( int vnum );
OBJ_INDEX_DATA  *make_object ( int vnum, int cvnum, char *name );
MOB_INDEX_DATA  *make_mobile ( int vnum, int cvnum, char *name );
EXIT_DATA       *make_exit   ( ROOM_INDEX_DATA *pRoomIndex, ROOM_INDEX_DATA *to_room, int door );
*/

/*
 * jobo_*.c
 */
void ragnarok_stop (void);
void dump_last_command (void);
int get_ratio ( CHAR_DATA * ch );
void win_prize ( CHAR_DATA * ch );
bool is_upgrade ( CHAR_DATA * ch );
bool is_contained ( const char *astr, const char *bstr );
bool is_contained2 ( const char *astr, const char *bstr );
void aggr_test ( CHAR_DATA * ch );
int strlen2 ( const char *b );
void logout_message ( CHAR_DATA * ch );
void login_message ( CHAR_DATA * ch );
void avatar_message ( CHAR_DATA * ch );
void special_decap_message ( CHAR_DATA * ch, CHAR_DATA *victim );
void powerdown ( CHAR_DATA * ch );
bool multicheck ( CHAR_DATA * ch );
bool reachedDecapLimit ( CHAR_DATA * ch );
void enter_info ( char *str );
void leave_info ( char *str );
void death_info ( char *str );
void avatar_info ( char *str );
int getMight ( CHAR_DATA * ch );
void forge_affect ( OBJ_DATA * obj, int value );
void update_revision ( CHAR_DATA * ch );
void recycle_descriptors (void);
void update_ragnarok (void);
void update_arena (void);
void update_doubleexp (void);
void update_doubleqps (void);

/*
 * pedit.c - Unified Player Editor
 */
void pedit_interp ( CHAR_DATA * ch, char *argument );
void pedit_save_offline ( CHAR_DATA * ch );

extern int thread_status;
extern char *last_command;

/* daemon.c */

ROOM_INDEX_DATA *locate_obj ( OBJ_DATA * obj );
void shock_effect ( void *vo, int level, int dam, int target );
void cold_effect ( void *vo, int level, int dam, int target );
void acid_effect ( void *vo, int level, int dam, int target );
void fire_effect ( void *vo, int level, int dam, int target );
void make_wall ( ROOM_INDEX_DATA * room, int dir, int wall );

/* fight.c */

int cap_dam ( CHAR_DATA * ch, CHAR_DATA *victim, int dam );
int randomize_damage ( CHAR_DATA * ch, int dam, int am );

void violence_update (void);
void multi_hit ( CHAR_DATA * ch, CHAR_DATA *victim, int dt );
void damage ( CHAR_DATA * ch, CHAR_DATA *victim, int dam, int dt );
void update_pos ( CHAR_DATA * victim );
void stop_fighting ( CHAR_DATA * ch, bool fBoth );
void stop_embrace ( CHAR_DATA * ch, CHAR_DATA *victim );
bool is_safe ( CHAR_DATA * ch, CHAR_DATA *victim );
void hurt_person ( CHAR_DATA * ch, CHAR_DATA *victim, int dam );
void set_fighting ( CHAR_DATA * ch, CHAR_DATA *victim );
bool has_timer ( CHAR_DATA * ch );
bool has_bad_chars ( CHAR_DATA * ch, char *argument );
void check_leaderboard ( CHAR_DATA * ch );
void update_top_board ( CHAR_DATA * ch );
void crashrecov (int);
void retell_protocols ( DESCRIPTOR_DATA * d );

/* handler.c */

int get_trust ( CHAR_DATA * ch );
int get_age ( CHAR_DATA * ch );
int get_curr_str ( CHAR_DATA * ch );
int get_curr_int ( CHAR_DATA * ch );
int get_curr_wis ( CHAR_DATA * ch );
int get_curr_dex ( CHAR_DATA * ch );
int get_curr_con ( CHAR_DATA * ch );
int can_carry_n ( CHAR_DATA * ch );
int can_carry_w ( CHAR_DATA * ch );
bool is_name ( char *str, char *namelist );
bool is_full_name ( const char *str, char *namelist );
void affect_to_char ( CHAR_DATA * ch, AFFECT_DATA *paf );
void affect_to_obj ( OBJ_DATA * obj, AFFECT_DATA *paf );
void affect_remove ( CHAR_DATA * ch, AFFECT_DATA *paf );
void alias_remove ( CHAR_DATA * ch, ALIAS_DATA *ali );
void affect_strip ( CHAR_DATA * ch, int sn );
bool is_affected ( CHAR_DATA * ch, int sn );
void affect_join ( CHAR_DATA * ch, AFFECT_DATA *paf );
void char_from_room ( CHAR_DATA * ch );
void char_to_room ( CHAR_DATA * ch, ROOM_INDEX_DATA *pRoomIndex );
void obj_to_char ( OBJ_DATA * obj, CHAR_DATA *ch );
void obj_from_char ( OBJ_DATA * obj );
int apply_ac ( OBJ_DATA * obj, int iWear );
OBJ_DATA *get_eq_char ( CHAR_DATA * ch, int iWear );
void equip_char ( CHAR_DATA * ch, OBJ_DATA *obj, int iWear );
void unequip_char ( CHAR_DATA * ch, OBJ_DATA *obj );
int count_obj_list ( OBJ_INDEX_DATA * obj, list_head_t *list );
int count_obj_room ( OBJ_INDEX_DATA * obj,
	list_head_t *list );
void obj_from_room ( OBJ_DATA * obj );
void obj_to_room ( OBJ_DATA * obj, ROOM_INDEX_DATA *pRoomIndex );
void obj_to_obj ( OBJ_DATA * obj, OBJ_DATA *obj_to );
void obj_from_obj ( OBJ_DATA * obj );
void extract_obj ( OBJ_DATA * obj );
void extract_char ( CHAR_DATA * ch, bool fPull );
CHAR_DATA *get_char_room ( CHAR_DATA * ch, char *argument );
CHAR_DATA *get_char_world ( CHAR_DATA * ch, char *argument );
CHAR_DATA *get_char_world2 ( CHAR_DATA * ch, char *argument );
OBJ_DATA *get_obj_type ( OBJ_INDEX_DATA * pObjIndexData );
OBJ_DATA *get_obj_list ( CHAR_DATA * ch, char *argument,
	list_head_t *list );
OBJ_DATA *get_obj_content ( CHAR_DATA * ch, char *argument,
	list_head_t *list );
OBJ_DATA *get_obj_in_obj ( CHAR_DATA * ch, char *argument );
OBJ_DATA *get_obj_carry ( CHAR_DATA * ch, char *argument );
OBJ_DATA *get_obj_wear ( CHAR_DATA * ch, char *argument );
OBJ_DATA *get_obj_here ( CHAR_DATA * ch, char *argument );
OBJ_DATA *get_obj_room ( CHAR_DATA * ch, char *argument );
OBJ_DATA *get_obj_world ( CHAR_DATA * ch, char *argument );
OBJ_DATA *get_obj_world2 ( CHAR_DATA * ch, char *argument );
OBJ_DATA *create_money ( int amount );
int get_obj_number ( OBJ_DATA * obj );
int get_obj_weight ( OBJ_DATA * obj );
bool room_is_dark ( ROOM_INDEX_DATA * pRoomIndex );
bool room_is_private ( ROOM_INDEX_DATA * pRoomIndex );
bool can_see ( CHAR_DATA * ch, CHAR_DATA *victim );
bool can_see_obj ( CHAR_DATA * ch, OBJ_DATA *obj );
bool can_drop_obj ( CHAR_DATA * ch, OBJ_DATA *obj );
char *item_type_name ( OBJ_DATA * obj );
char *affect_loc_name ( int location );
char *affect_bit_name ( int vector );
char *extra_bit_name ( int extra_flags );
void affect_modify ( CHAR_DATA * ch, AFFECT_DATA *paf, bool fAdd, OBJ_DATA *obj );

/* Vitals modification with GMCP update */
void modify_vitals ( CHAR_DATA * ch, int hp_delta, int mana_delta, int move_delta );
void heal_char ( CHAR_DATA * ch, int amount );
void heal_char_over ( CHAR_DATA * ch, int amount, int max_cap );
void use_mana ( CHAR_DATA * ch, int cost );
void use_move ( CHAR_DATA * ch, int cost );

void set_learnable_disciplines ( CHAR_DATA * ch );
void update_disc ( CHAR_DATA * ch );

/* interp.c */
void interpret ( CHAR_DATA * ch, char *argument );
bool is_number ( char *arg );
int number_argument ( char *argument, char *arg );
char *one_argument ( char *argument, char *arg_first );
char *one_argument_case ( char *argument, char *arg_first );
void stage_update ( CHAR_DATA * ch, CHAR_DATA *victim, int stage, char *argument );
void make_preg ( CHAR_DATA * mother, CHAR_DATA *father );

/* magic.c */
int skill_lookup ( const char *name );
int slot_lookup ( int slot );
bool saves_spell ( int level, CHAR_DATA *victim );
void obj_cast_spell ( int sn, int level, CHAR_DATA *ch,
	CHAR_DATA *victim, OBJ_DATA *obj );
void enhance_stat ( int sn, int level, CHAR_DATA *ch,
	CHAR_DATA *victim, int apply_bit,
	int bonuses, int affect_bit );

/* save.c */
void save_char_obj ( CHAR_DATA * ch );
void save_char_obj_backup ( CHAR_DATA * ch );
bool load_char_obj ( DESCRIPTOR_DATA * d, char *name );
bool load_char_short ( DESCRIPTOR_DATA * d, char *name );

/* special.c */
SPEC_FUN *spec_lookup ( const char *name );

/* mccp.c */
bool compressStart( DESCRIPTOR_DATA *desc, int version );
bool compressEnd( DESCRIPTOR_DATA *desc );
bool compressEnd2( DESCRIPTOR_DATA *desc ); // threadsafe version.
bool processCompressed( DESCRIPTOR_DATA *desc );
bool writeCompressed( DESCRIPTOR_DATA *desc, char *txt, int length );

/* mssp.c */
void mssp_send( DESCRIPTOR_DATA *d );

/* mxp.c */
bool mxpStart( DESCRIPTOR_DATA *desc );
void mxpEnd( DESCRIPTOR_DATA *desc );

/* gmcp.c */
void gmcp_init( DESCRIPTOR_DATA *d );
void gmcp_send( DESCRIPTOR_DATA *d, const char *package, const char *data );
void gmcp_send_vitals( CHAR_DATA *ch );
void gmcp_send_status( CHAR_DATA *ch );
void gmcp_send_info( CHAR_DATA *ch );
void gmcp_send_char_data( CHAR_DATA *ch );
void gmcp_send_room_info( CHAR_DATA *ch );
void gmcp_handle_subnegotiation( DESCRIPTOR_DATA *d, unsigned char *data, int len );

/* act_map.c */
void show_automap( CHAR_DATA *ch );

/* naws.c */
void naws_init( DESCRIPTOR_DATA *d );
void naws_handle_subnegotiation( DESCRIPTOR_DATA *d, unsigned char *data, int len );
int naws_get_width( CHAR_DATA *ch );

/* ttype.c */
void ttype_init( DESCRIPTOR_DATA *d );
void ttype_request( DESCRIPTOR_DATA *d );
void ttype_handle_subnegotiation( DESCRIPTOR_DATA *d, unsigned char *data, int len );

/* intro.c */
void intro_load( void );
void intro_check_ready( DESCRIPTOR_DATA *d );

/* update.c */
void gain_exp ( CHAR_DATA * ch, int gain );
void gain_condition ( CHAR_DATA * ch, int iCond, int value );
void update_handler (void);
void mobile_update (void);
void weather_update (void);
void char_update (void);
void char_update2 (void);
void obj_update (void);
void ww_update (void);

void room_update (void);

/* kav_fight.c */
void special_move ( CHAR_DATA * ch, CHAR_DATA *victim );

/* kav_info.c */
void birth_date ( CHAR_DATA * ch, bool is_self );
void other_age ( CHAR_DATA * ch, int extra, bool is_preg,
	char *argument );
int years_old ( CHAR_DATA * ch );

/* kav_wiz.c */
void oset_affect ( CHAR_DATA * ch, OBJ_DATA *obj, int value, int affect, bool is_quest );

/* clan.c */
void reg_mend ( CHAR_DATA * ch );
void vamp_rage ( CHAR_DATA * ch );
bool char_exists ( bool backup, char *argument );
OBJ_DATA *get_page ( OBJ_DATA * book, int page_num );

/* vic.c */
DO_FUN do_relevel2;
void reset_weapon ( CHAR_DATA * ch, int dtype );
void reset_spell ( CHAR_DATA * ch, int dtype );

/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * This structure is used in special.c to lookup spec funcs and
 * also in olc_act.c to display listings of spec funcs.
 */
struct spec_type {
	char *spec_name;
	SPEC_FUN *spec_fun;
};

/*
 * This structure is used in bit.c to lookup flags and stats.
 */
struct flag_type {
	char *name;
	int bit;
	bool settable;
};

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY 1

/*
 * Area flags.
 */
#define AREA_NONE	 0
#define AREA_CHANGED 1 /* Area has been modified. */
#define AREA_ADDED	 2 /* Area has been added to. */
#define AREA_LOADING 4 /* Used for counting in db.c */
#define AREA_VERBOSE 8

#define MAX_DIR 6
#define NO_FLAG -99 /* Must not be used in flags or stats. */

/*
 * Interp.c
 */
DO_FUN do_aedit; /* OLC 1.1b */
DO_FUN do_redit; /* OLC 1.1b */
DO_FUN do_oedit; /* OLC 1.1b */
DO_FUN do_medit; /* OLC 1.1b */
DO_FUN do_asave;
DO_FUN do_alist;
DO_FUN do_resets;

/* Dirgesinger */
DO_FUN do_warcry;
DO_FUN do_shatter;
DO_FUN do_battlehymn;
DO_FUN do_dirge;
DO_FUN do_thunderclap;
DO_FUN do_ironsong;
DO_FUN do_cadence;
DO_FUN do_dissonance;
DO_FUN do_rally;
DO_FUN do_warsong;
DO_FUN do_resonance;
DO_FUN do_songtrain;
DO_FUN do_dirgesingerarmor;

/* Siren */
DO_FUN do_bansheewail;
DO_FUN do_soulrend;
DO_FUN do_crescendo;
DO_FUN do_cacophony;
DO_FUN do_enthrall;
DO_FUN do_sirensong;
DO_FUN do_commandvoice;
DO_FUN do_mesmerize;
DO_FUN do_echoshield;
DO_FUN do_ariaofunmaking;
DO_FUN do_voicetrain;
DO_FUN do_sirenarmor;

/* Psion */
DO_FUN do_psifocus;
DO_FUN do_psimeditate;
DO_FUN do_psitrain;
DO_FUN do_psionarmor;
DO_FUN do_mindscan;
DO_FUN do_thoughtshield;
DO_FUN do_mentallink;
DO_FUN do_forcepush;
DO_FUN do_levitate;
DO_FUN do_kineticbarrier;
DO_FUN do_mindspike;
DO_FUN do_psychicscream;
DO_FUN do_brainburn;

/* Mindflayer */
DO_FUN do_mindtrain;
DO_FUN do_mindflayerarmor;
DO_FUN do_enthral;
DO_FUN do_puppet;
DO_FUN do_hivemind;
DO_FUN do_massdomination;
DO_FUN do_mindfeed;
DO_FUN do_memorydrain;
DO_FUN do_intellectdevour;
DO_FUN do_psychicmaelstrom;
DO_FUN do_psiblast;
DO_FUN do_realityfracture;
DO_FUN do_dismiss;

/* Dragonkin */
DO_FUN do_dragonstatus;
DO_FUN do_attune;
DO_FUN do_essencemeditate;
DO_FUN do_dragonbreath;
DO_FUN do_searingblast;
DO_FUN do_infernalstorm;
DO_FUN do_scaleshield;
DO_FUN do_dragonhide;
DO_FUN do_primalwarding;
DO_FUN do_dragonclaw;
DO_FUN do_drakewings;
DO_FUN do_dragonrush;
DO_FUN do_dragontrain;
DO_FUN do_dragonarmor;

/* Wyrm */
DO_FUN do_wyrmbreath;
DO_FUN do_cataclysm;
DO_FUN do_annihilate;
DO_FUN do_apocalypse;
DO_FUN do_dragonfear;
DO_FUN do_terrainshift;
DO_FUN do_dragonlord;
DO_FUN do_wyrmform;
DO_FUN do_ancientwrath;
DO_FUN do_primordial;
DO_FUN do_wyrmtrain;
DO_FUN do_wyrmarmor;

/* Artificer */
DO_FUN do_power;
DO_FUN do_powercharge;
DO_FUN do_overcharge;
DO_FUN do_turret;
DO_FUN do_decoy;
DO_FUN do_grapple;
DO_FUN do_energyblade;
DO_FUN do_blaster;
DO_FUN do_grenade;
DO_FUN do_forcefield;
DO_FUN do_repairbot;
DO_FUN do_artcloak;
DO_FUN do_techtrain;
DO_FUN do_artificerarmor;

/* Mechanist */
DO_FUN do_neuraljack;
DO_FUN do_servoarms;
DO_FUN do_reactiveplating;
DO_FUN do_combatdrone;
DO_FUN do_repairswarm;
DO_FUN do_bomberdrone;
DO_FUN do_artdetonate;
DO_FUN do_dronearmy;
DO_FUN do_railgun;
DO_FUN do_empburst;
DO_FUN do_orbitalstrike;
DO_FUN do_mechimplant;
DO_FUN do_cybtrain;
DO_FUN do_mechanistarmor;
DO_FUN do_dronestatus;
DO_FUN do_dronerecall;

/* Cultist */
DO_FUN do_corruption;
DO_FUN do_cultpurge;
DO_FUN do_eldritchsight;
DO_FUN do_whispers;
DO_FUN do_unravel;
DO_FUN do_voidtendril;
DO_FUN do_grasp;
DO_FUN do_constrict;
DO_FUN do_maddeninggaze;
DO_FUN do_gibbering;
DO_FUN do_insanity;
DO_FUN do_voidtrain;
DO_FUN do_cultistarmor;

/* Voidborn */
DO_FUN do_phaseshift;
DO_FUN do_dimensionalrend;
DO_FUN do_unmake;
DO_FUN do_voidshape;
DO_FUN do_aberrantgrowth;
DO_FUN do_finalform;
DO_FUN do_summonthing;
DO_FUN do_starspawn;
DO_FUN do_entropy;
DO_FUN do_voidbornarmor;

/* Chronomancer */
DO_FUN do_flux;
DO_FUN do_quicken;
DO_FUN do_timeslip;
DO_FUN do_blur;
DO_FUN do_slow;
DO_FUN do_timetrap;
DO_FUN do_stasis;
DO_FUN do_foresight;
DO_FUN do_hindsight;
DO_FUN do_temporalecho;
DO_FUN do_timetrain;
DO_FUN do_chronoarmor;

/* Paradox */
DO_FUN do_destabilize;
DO_FUN do_rewind;
DO_FUN do_splittimeline;
DO_FUN do_convergence;
DO_FUN do_futurestrike;
DO_FUN do_pastself;
DO_FUN do_timeloop;
DO_FUN do_paradoxstrike;
DO_FUN do_age;
DO_FUN do_temporalcollapse;
DO_FUN do_eternity;
DO_FUN do_paratrain;
DO_FUN do_paradoxarmor;

/* Shaman abilities */
DO_FUN do_tether;
DO_FUN do_wardtotem;
DO_FUN do_wrathtotem;
DO_FUN do_spirittotem;
DO_FUN do_spiritbolt;
DO_FUN do_spiritward;
DO_FUN do_spiritwalk;
DO_FUN do_ancestorcall;
DO_FUN do_spiritsight;
DO_FUN do_soullink;
DO_FUN do_spirittrain;
DO_FUN do_shamanarmor;

/* Spirit Lord abilities */
DO_FUN do_embody;
DO_FUN do_ancestralform;
DO_FUN do_spiritfusion;
DO_FUN do_compel;
DO_FUN do_possess;
DO_FUN do_spiritarmy;
DO_FUN do_soulstorm;
DO_FUN do_ancestralwisdom;
DO_FUN do_spiritcleanse;
DO_FUN do_ascension;
DO_FUN do_lordtrain;
DO_FUN do_lordarmor;

/*
 * Global Constants
 */
extern char *const dir_name[];
extern const int rev_dir[];
extern const struct spec_type spec_table[];

/*
 * Global variables
 */

extern list_head_t g_areas;
extern AREA_DATA *area_last;

extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;

extern int top_vnum_mob;
extern int top_vnum_obj;
extern int top_vnum_room;

extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

/* db.c */
void reset_area ( AREA_DATA * pArea );
void reset_room ( ROOM_INDEX_DATA * pRoom );

/* string.c */
void string_edit ( CHAR_DATA * ch, char **pString );
void string_append ( CHAR_DATA * ch, char **pString );
char *string_replace ( char *orig, char *old, char *new );
void string_add ( CHAR_DATA * ch, char *argument );
char *format_string ( char *oldstring /*, bool fSpace */ );
char *first_arg ( char *argument, char *arg_first, bool fCase );
char *string_unpad ( char *argument );
char *string_proper ( char *argument );
char *all_capitalize ( const char *str );
void add_commas_to_number ( int number, char *buf, size_t buf_size );

/* olc.c */
bool run_olc_editor ( DESCRIPTOR_DATA * d );
char *olc_ed_name ( CHAR_DATA * ch );
char *olc_ed_vnum ( CHAR_DATA * ch );

/* special.c */
char *spec_string ( SPEC_FUN * fun ); /* OLC */

/* bit.c */
extern const struct flag_type area_flags[];
extern const struct flag_type sex_flags[];
extern const struct flag_type exit_flags[];
extern const struct flag_type door_resets[];
extern const struct flag_type room_flags[];
extern const struct flag_type sector_flags[];
extern const struct flag_type type_flags[];
extern const struct flag_type extra_flags[];
extern const struct flag_type wear_flags[];
extern const struct flag_type act_flags[];
extern const struct flag_type affect_flags[];
extern const struct flag_type apply_flags[];
extern const struct flag_type wear_loc_strings[];
extern const struct flag_type wear_loc_flags[];
extern const struct flag_type weapon_flags[];
extern const struct flag_type container_flags[];
extern const struct flag_type liquid_flags[];

/* Flag utility functions (defined in bit.c) */
char *flag_string ( const struct flag_type *flag_table, int bits );
int flag_value ( const struct flag_type *flag_table, char *argument );

/* NOTE: This file only includes two examples! write your own! */

struct slay_type {
	char *owner;	/* only this player can use this option */
	char *title;	/* each one must have a unique title!   */
	char *char_msg; /* is sent to the "slayer"		*/
	char *vict_msg; /* is sent to the "slayee" (poor sucker)*/
	char *room_msg; /* is sent to everyone else in the room */
};

#define MAX_SLAY_TYPES 16

extern struct slay_type slay_table[];
extern int slay_count;

void merc_logf ( char *fmt, ... );

void copyover_recover (void);

/* class_armor.c */
void do_classarmor_generic ( CHAR_DATA *ch, char *argument, int class_id );

#endif // MERC_H