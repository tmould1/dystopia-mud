#ifndef OBJECT_H
#define OBJECT_H

/* Object system structures, constants, and macros */
/* Extracted from merc.h — Phase 3 struct decomposition */

#include "types.h"

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
	list_head_t scripts;      /* Lua scripts (future use) */
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
 * Object Macros.
 */
#define CAN_WEAR( obj, part )	  ( IS_SET( ( obj )->wear_flags, ( part ) ) )
#define IS_OBJ_STAT( obj, stat )  ( IS_SET( ( obj )->extra_flags, ( stat ) ) )
#define IS_OBJ_STAT2( obj, stat ) ( IS_SET( ( obj )->extra_flags2, ( stat ) ) )
#define IS_WEAP( obj, stat )	  ( IS_SET( ( obj )->weapflags, ( stat ) ) )

#endif /* OBJECT_H */
