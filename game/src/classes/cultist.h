/***************************************************************************
 *  Cultist / Voidborn Class Header                                        *
 *  Lovecraftian cosmic horror classes with corruption risk/reward          *
 ***************************************************************************/

#ifndef CULTIST_H
#define CULTIST_H

/*
 * Class bit values (power-of-2)
 * These are also defined in class.h for global access
 */
#define CLASS_CULTIST       4194304   /* 2^22 - Base class */
#define CLASS_VOIDBORN      8388608   /* 2^23 - Upgrade class */

/*
 * Corruption caps
 */
#define CULT_CORRUPT_CAP            100
#define VOID_CORRUPT_CAP            150

/*
 * Corruption self-damage thresholds
 */
#define CULT_CORRUPT_THRESH_LIGHT   50   /* 1% max HP per tick */
#define CULT_CORRUPT_THRESH_MOD     75   /* 2% max HP per tick */
#define CULT_CORRUPT_THRESH_HEAVY   100  /* 3% max HP per tick */
#define VOID_CORRUPT_THRESH_CATA   125   /* 4% max HP per tick (Voidborn only) */
#define VOID_CORRUPT_RESIST         25   /* Voidborn 25% self-damage resistance */

/*
 * Maximum void creatures (Voidborn)
 */
#define VOID_MAX_SUMMONS    2

/* =========================================================================
 * CULTIST pcdata->powers[] indices
 * ========================================================================= */

/* Ability state (0-9) */
#define CULT_ELDRITCH_SIGHT     0   /* Eldritch Sight ticks remaining */
#define CULT_GRASP_TARGET       1   /* Grasp target (char ID) */
#define CULT_GRASP_TICKS        2   /* Grasp ticks remaining */
#define CULT_CONSTRICT_TICKS    3   /* Constrict ticks remaining */
#define CULT_WHISPERS_TICKS     4   /* Whispers debuff ticks remaining */
#define CULT_GIBBERING_TICKS    5   /* Gibbering confusion ticks remaining */

/* Training levels (10+) */
#define CULT_TRAIN_LORE         10  /* Forbidden Lore level (0-3) */
#define CULT_TRAIN_TENTACLE     11  /* Tentacle Arts level (0-3) */
#define CULT_TRAIN_MADNESS      12  /* Madness level (0-3) */

/* pcdata->stats[] indices */
#define CULT_STAT_PEAK_CORRUPT  0   /* Highest corruption this session */
#define CULT_STAT_VOID_DAMAGE   1   /* Total void damage dealt (tracking) */

/* =========================================================================
 * VOIDBORN pcdata->powers[] indices
 * ========================================================================= */

/* Ability state (0-9) - REPLACES Cultist states on upgrade */
#define VOID_PHASE_SHIFT        0   /* Phase Shift ticks remaining */
#define VOID_VOID_SHAPE         1   /* Void Shape ticks remaining */
#define VOID_ABERRANT_GROWTH    2   /* Aberrant Growth ticks remaining */
#define VOID_FINAL_FORM         3   /* Final Form ticks remaining */
#define VOID_RIFT_ROOM          4   /* Room vnum of active rift */
#define VOID_RIFT_TICKS         5   /* Dimensional Rend rift ticks remaining */
#define VOID_ENTROPY_TICKS      6   /* Entropy effect ticks remaining */
#define VOID_SUMMON_COUNT       7   /* Number of active void creatures */

/* Training levels (10+) - REUSES same indices as Cultist */
#define VOID_TRAIN_WARP         10  /* Reality Warp level (0-3) */
#define VOID_TRAIN_FORM         11  /* Elder Form level (0-3) */
#define VOID_TRAIN_COSMIC       12  /* Cosmic Horror level (0-3) */

/* Cooldowns (14+) */
#define VOID_UNMAKE_CD          14  /* Unmake cooldown pulses */
#define VOID_FINALFORM_CD       15  /* Final Form cooldown pulses */
#define VOID_STARSPAWN_CD       16  /* Star Spawn cooldown pulses */
#define VOID_ENTROPY_CD         17  /* Entropy cooldown pulses */
#define VOID_DIMREND_CD         18  /* Dimensional Rend cooldown pulses */

/* pcdata->stats[] indices */
#define VOID_STAT_PEAK_CORRUPT  0   /* Highest corruption this session */
#define VOID_STAT_VOID_DAMAGE   1   /* Total void damage dealt (tracking) */

/* =========================================================================
 * Void creature mob vnum
 * ========================================================================= */

#define VNUM_VOID_CREATURE      33540

/* =========================================================================
 * Function prototypes
 * ========================================================================= */

/* Cultist abilities */
void do_corruption      args( ( CHAR_DATA *ch, char *argument ) );
void do_cultpurge       args( ( CHAR_DATA *ch, char *argument ) );
void do_eldritchsight   args( ( CHAR_DATA *ch, char *argument ) );
void do_whispers        args( ( CHAR_DATA *ch, char *argument ) );
void do_unravel         args( ( CHAR_DATA *ch, char *argument ) );
void do_voidtendril     args( ( CHAR_DATA *ch, char *argument ) );
void do_grasp           args( ( CHAR_DATA *ch, char *argument ) );
void do_constrict       args( ( CHAR_DATA *ch, char *argument ) );
void do_maddeninggaze   args( ( CHAR_DATA *ch, char *argument ) );
void do_gibbering       args( ( CHAR_DATA *ch, char *argument ) );
void do_insanity        args( ( CHAR_DATA *ch, char *argument ) );
void do_voidtrain       args( ( CHAR_DATA *ch, char *argument ) );

/* Voidborn abilities */
void do_phaseshift      args( ( CHAR_DATA *ch, char *argument ) );
void do_dimensionalrend args( ( CHAR_DATA *ch, char *argument ) );
void do_unmake          args( ( CHAR_DATA *ch, char *argument ) );
void do_voidshape       args( ( CHAR_DATA *ch, char *argument ) );
void do_aberrantgrowth  args( ( CHAR_DATA *ch, char *argument ) );
void do_finalform       args( ( CHAR_DATA *ch, char *argument ) );
void do_summonthing     args( ( CHAR_DATA *ch, char *argument ) );
void do_starspawn       args( ( CHAR_DATA *ch, char *argument ) );
void do_entropy         args( ( CHAR_DATA *ch, char *argument ) );

/* Update functions (called from update.c) */
void update_cultist     args( ( CHAR_DATA *ch ) );
void update_voidborn    args( ( CHAR_DATA *ch ) );

#endif /* CULTIST_H */
