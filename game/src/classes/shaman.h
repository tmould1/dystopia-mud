/***************************************************************************
 *  Shaman / Spirit Lord Class Header                                      *
 *  Spirit world conduits with Tether balance mechanic                     *
 ***************************************************************************/

#ifndef SHAMAN_H
#define SHAMAN_H

/*
 * Class bit values (power-of-2)
 * These are also defined in class.h for global access
 */
#define CLASS_SHAMAN        67108864   /* 2^26 - Base class */
#define CLASS_SPIRITLORD    134217728  /* 2^27 - Upgrade class */

/*
 * Spirit Tether caps and centers
 */
#define SHAMAN_TETHER_CAP       100
#define SHAMAN_TETHER_CENTER     50
#define SL_TETHER_CAP           150
#define SL_TETHER_CENTER         75

/*
 * Tether balance zones (Shaman: 0-100)
 * Grounded:       0-19  - Material +30%, Spirit -50%
 * Earthbound:    20-39  - Material +15%, Spirit -25%
 * Balanced:      40-60  - All abilities normal
 * Spirit-Touched: 61-80 - Spirit +15%, Material -25%
 * Unmoored:      81-100 - Spirit +30%, Material -50%
 */
#define SHAMAN_GROUNDED_MAX     19
#define SHAMAN_EARTHBOUND_MAX   39
#define SHAMAN_BALANCED_MAX     60
#define SHAMAN_SPIRITTOUCH_MAX  80

/*
 * Tether instability trigger ranges
 * At these extremes, 10% chance per tick of spirit manifestation
 */
#define SHAMAN_MANIFEST_LOW     10   /* tether 0-10 triggers manifestation */
#define SHAMAN_MANIFEST_HIGH    90   /* tether 90-100 triggers manifestation */

/*
 * Spirit Lord expanded tether zones (0-150)
 * Anchored:       0-29  - Material +40%, Spirit -60%, immune to spirit attacks
 * Earthbound:    30-59  - Material +20%, Spirit -30%
 * Balanced:      60-90  - All abilities normal, can use Embodiment
 * Spirit-Touched: 91-120 - Spirit +20%, Material -30%
 * Ascended:     121-150 - Spirit +40%, Material -60%, phase through walls
 */
#define SL_ANCHORED_MAX         29
#define SL_EARTHBOUND_MAX       59
#define SL_BALANCED_MAX         90
#define SL_SPIRITTOUCH_MAX     120

/*
 * Spirit Lord instability trigger ranges
 */
#define SL_MANIFEST_LOW         15   /* tether 0-15 triggers manifestation */
#define SL_MANIFEST_HIGH       135   /* tether 135-150 triggers manifestation */

/*
 * Mob VNUMs for totems and spirit warriors
 */
#define VNUM_SHAMAN_TOTEM       33596
#define VNUM_SL_SPIRIT_WARRIOR  33616

/* =========================================================================
 * SHAMAN pcdata->powers[] indices
 * ========================================================================= */

/* Ability state (0-9) */
#define SHAMAN_WARD_TICKS         0   /* Ward Totem ticks remaining */
#define SHAMAN_WRATH_TICKS        1   /* Wrath Totem ticks remaining */
#define SHAMAN_SPIRIT_TOTEM_TICKS 2   /* Spirit Totem ticks remaining */
#define SHAMAN_SPIRIT_WARD_CHARGES 3  /* Spirit Ward charges remaining */
#define SHAMAN_SPIRIT_WALK_TICKS  4   /* Spirit Walk ticks remaining */
#define SHAMAN_SPIRIT_SIGHT_TICKS 5   /* Spirit Sight ticks remaining */
#define SHAMAN_SOUL_LINK_TICKS    6   /* Soul Link ticks remaining */
#define SHAMAN_SOUL_LINK_TYPE     7   /* Soul Link type (0=enemy via ch->fighting) */
#define SHAMAN_ACTIVE_TOTEM       8   /* Active totem type (0=none, 1=ward, 2=wrath, 3=spirit) */
/*                                9      (reserved) */

/* Training levels (10-12) */
#define SHAMAN_TRAIN_TOTEM       10   /* Totem training level (0-3) */
#define SHAMAN_TRAIN_SPIRIT      11   /* Spirit training level (0-3) */
#define SHAMAN_TRAIN_COMMUNE     12   /* Communion training level (0-3) */
/*                               13      (reserved) */

/* Cooldowns (14+) */
#define SHAMAN_WRATHTOTEM_CD     14   /* Wrath Totem cooldown ticks */
#define SHAMAN_SPIRITTOTEM_CD    15   /* Spirit Totem cooldown ticks */
#define SHAMAN_SPIRITWALK_CD     16   /* Spirit Walk cooldown ticks */
#define SHAMAN_SOULLINK_CD       17   /* Soul Link cooldown ticks */
#define SHAMAN_SPIRITBOLT_CD     18   /* Spirit Bolt cooldown ticks */

/* pcdata->stats[] indices */
#define SHAMAN_STAT_TOTEM_HP      0   /* Current totem HP remaining */
#define SHAMAN_STAT_MANIFEST_CNT  1   /* Times manifestation triggered (tracking) */

/* Totem type constants */
#define TOTEM_NONE                0
#define TOTEM_WARD                1
#define TOTEM_WRATH               2
#define TOTEM_SPIRIT              3

/* =========================================================================
 * SPIRIT LORD pcdata->powers[] indices
 * ========================================================================= */

/* Ability state (0-9) - REPLACES Shaman states on upgrade */
#define SL_EMBODY_TYPE            0   /* Current aura (0=none, 1=ward, 2=wrath, 3=spirit) */
#define SL_ANCESTRAL_FORM_TICKS   1   /* Ancestral Form ticks remaining */
#define SL_SPIRIT_FUSION_TICKS    2   /* Spirit Fusion ticks remaining */
#define SL_SPIRIT_ARMY_TICKS      3   /* Spirit Army ticks remaining */
#define SL_POSSESS_TARGET         4   /* Possess target NPC ID */
#define SL_POSSESS_TICKS          5   /* Possess rounds remaining */
#define SL_ASCENSION_TICKS        6   /* Ascension ticks remaining */
#define SL_ASCENSION_AFTERMATH    7   /* Ascension aftermath rounds */
#define SL_WISDOM_TICKS           8   /* Ancestral Wisdom ticks remaining */
/*                                9      (reserved) */

/* Training levels (10-12) - REUSES same indices as Shaman */
#define SL_TRAIN_EMBODY          10   /* Embodiment training level (0-3) */
#define SL_TRAIN_DOMINION        11   /* Dominion training level (0-4) */
#define SL_TRAIN_TRANSCEND       12   /* Transcendence training level (0-3) */
/*                               13      (reserved) */

/* Cooldowns (14+) */
#define SL_ANCESTRALFORM_CD      14   /* Ancestral Form cooldown ticks */
#define SL_SPIRITFUSION_CD       15   /* Spirit Fusion cooldown ticks */
#define SL_COMPEL_CD             16   /* Compel cooldown ticks */
#define SL_POSSESS_CD            17   /* Possess cooldown ticks */
#define SL_SPIRITARMY_CD         18   /* Spirit Army cooldown ticks */
#define SL_SOULSTORM_CD          19   /* Soul Storm cooldown ticks */

/* pcdata->stats[] indices */
#define SL_STAT_ASCENSIONS_USED   0   /* Times Ascension used (tracking) */
#define SL_STAT_WISDOM_CD         1   /* Ancestral Wisdom cooldown ticks */
#define SL_STAT_CLEANSE_CD        2   /* Spirit Cleanse cooldown ticks */
#define SL_STAT_ASCENSION_CD      3   /* Ascension cooldown ticks */
#define SL_STAT_ARMY_COUNT        4   /* Current spirit warrior count */

/* Aura type constants (reuses totem constants) */
#define AURA_NONE                 0
#define AURA_WARD                 1
#define AURA_WRATH                2
#define AURA_SPIRIT               3

/* =========================================================================
 * Function prototypes
 * ========================================================================= */

/* Shared command */
void do_tether          ( CHAR_DATA *ch, char *argument );

/* Shaman abilities */
void do_wardtotem       ( CHAR_DATA *ch, char *argument );
void do_wrathtotem      ( CHAR_DATA *ch, char *argument );
void do_spirittotem     ( CHAR_DATA *ch, char *argument );
void do_spiritbolt      ( CHAR_DATA *ch, char *argument );
void do_spiritward      ( CHAR_DATA *ch, char *argument );
void do_spiritwalk      ( CHAR_DATA *ch, char *argument );
void do_ancestorcall    ( CHAR_DATA *ch, char *argument );
void do_spiritsight     ( CHAR_DATA *ch, char *argument );
void do_soullink        ( CHAR_DATA *ch, char *argument );
void do_spirittrain     ( CHAR_DATA *ch, char *argument );
void do_shamanarmor     ( CHAR_DATA *ch, char *argument );

/* Spirit Lord abilities */
void do_embody          ( CHAR_DATA *ch, char *argument );
void do_ancestralform   ( CHAR_DATA *ch, char *argument );
void do_spiritfusion    ( CHAR_DATA *ch, char *argument );
void do_compel          ( CHAR_DATA *ch, char *argument );
void do_possess         ( CHAR_DATA *ch, char *argument );
void do_spiritarmy      ( CHAR_DATA *ch, char *argument );
void do_soulstorm       ( CHAR_DATA *ch, char *argument );
void do_ancestralwisdom ( CHAR_DATA *ch, char *argument );
void do_spiritcleanse   ( CHAR_DATA *ch, char *argument );
void do_ascension       ( CHAR_DATA *ch, char *argument );
void do_lordtrain       ( CHAR_DATA *ch, char *argument );
void do_lordarmor       ( CHAR_DATA *ch, char *argument );

/* Tether scaling helpers */
int get_shaman_power_mod ( CHAR_DATA *ch, bool is_material );
int get_sl_power_mod     ( CHAR_DATA *ch, bool is_material );

/* Update functions (called from update.c) */
void update_shaman      ( CHAR_DATA *ch );
void update_spiritlord  ( CHAR_DATA *ch );

#endif /* SHAMAN_H */
