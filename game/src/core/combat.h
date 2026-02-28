#ifndef COMBAT_H
#define COMBAT_H

/* Combat/magic constants, skill system, and spell prototypes */
/* Extracted from merc.h — Phase 3 struct decomposition */

#include "types.h"

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

#endif /* COMBAT_H */
