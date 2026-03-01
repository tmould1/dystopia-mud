#ifndef WORLD_H
#define WORLD_H

/* Area/shop/reset/world structures and OLC helpers */
/* Extracted from merc.h — Phase 3 struct decomposition */

#include "types.h"

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
/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

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

#endif /* WORLD_H */
