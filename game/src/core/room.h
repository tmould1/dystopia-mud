#ifndef ROOM_H
#define ROOM_H

/* Room/exit system structures and constants */
/* Extracted from merc.h — Phase 3 struct decomposition */

#include "types.h"

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
#define RTIMER( room, rtmr )		  ( ( room )->tick_timer[( rtmr )] )
#define SET_RTIMER( room, rtmr, rtm ) ( ( room )->tick_timer[( rtmr )] = ( rtm ) )
#define ADD_RTIMER( room, rtmr, rtm ) ( ( room )->tick_timer[( rtmr )] += ( rtm ) )
#define SUB_RTIMER( room, rtmr, rtm ) ( ( room )->tick_timer[( rtmr )] -= ( rtm ) )
#define RTIME_UP( room, rtmr )		  ( ( room )->tick_timer[( rtmr )] == 0 ? TRUE : FALSE )
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
	list_head_t resets;      /* OLC */
	list_head_t scripts;     /* Lua scripts attached to this room */

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

#endif /* ROOM_H */
