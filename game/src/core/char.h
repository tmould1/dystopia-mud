#ifndef CHAR_H
#define CHAR_H

/* Character system structures, constants, and macros */
/* Extracted from merc.h — Phase 3 struct decomposition */

#include "types.h"

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
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data {
	MOB_INDEX_DATA *next;
	list_head_t scripts;      /* Lua scripts attached to this mob template */
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
 * Description macros.
 */
#define PERS( ch, looker ) ( can_see( looker, ( ch ) ) ? ( IS_NPC( ch ) ? ( ch )->short_descr                                                       \
																		: ( IS_AFFECTED( ( ch ), AFF_POLYMORPH ) ? ( ch )->morph : ( ch )->name ) ) \
													   : "someone" )

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

#endif /* CHAR_H */
