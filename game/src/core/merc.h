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

#include "types.h"
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

/* Subsystem headers (extracted from merc.h) */
#include "network.h"
#include "char.h"
#include "object.h"
#include "room.h"
#include "world.h"
#include "combat.h"


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


/* Utility macros (UMIN, UMAX, URANGE, IS_SET, etc.) are in types.h */


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

/* bit.c */

void merc_logf ( char *fmt, ... );

void copyover_recover (void);

/* class_armor.c */
void do_classarmor_generic ( CHAR_DATA *ch, char *argument, int class_id );

#endif // MERC_H