/***************************************************************************
 *  Chronomancer / Paradox Class Header                                    *
 *  Temporal mages with Flux balance mechanic                              *
 ***************************************************************************/

#ifndef CHRONOMANCER_H
#define CHRONOMANCER_H

/*
 * Class bit values (power-of-2)
 * These are also defined in class.h for global access
 */
#define CLASS_CHRONOMANCER  16777216  /* 2^24 - Base class */
#define CLASS_PARADOX       33554432  /* 2^25 - Upgrade class */

/*
 * Temporal Flux caps and centers
 */
#define CHRONO_FLUX_CAP         100
#define CHRONO_FLUX_CENTER       50
#define PARA_FLUX_CAP           150
#define PARA_FLUX_CENTER         75

/*
 * Flux balance zones (Chronomancer: 0-100)
 * Deep Slow: 0-19   — Accel +30%, Decel -50%
 * Slow:      20-39  — Accel +15%, Decel -25%
 * Balanced:  40-60  — All abilities normal
 * Fast:      61-80  — Decel +15%, Accel -25%
 * Deep Fast: 81-100 — Decel +30%, Accel -50%
 */
#define CHRONO_DEEP_SLOW_MAX    19
#define CHRONO_SLOW_MAX         39
#define CHRONO_BALANCED_MAX     60
#define CHRONO_FAST_MAX         80

/*
 * Flux instability trigger ranges
 * At these extremes, 10% chance per tick of random temporal effects
 */
#define CHRONO_INSTABILITY_LOW  10   /* flux 0-10 triggers instability */
#define CHRONO_INSTABILITY_HIGH 90   /* flux 90-100 triggers instability */

/*
 * Paradox expanded flux zones (0-150)
 * Temporal Anchor: 0-29   — Accel +40%, Decel -60%, immune to time effects
 * Slow:            30-59  — Accel +20%, Decel -30%
 * Balanced:        60-90  — All abilities normal, can use Timeline abilities
 * Fast:            91-120 — Decel +20%, Accel -30%
 * Temporal Storm:  121-150 — Decel +40%, Accel -60%, extra random effects
 */
#define PARA_ANCHOR_MAX         29
#define PARA_SLOW_MAX           59
#define PARA_BALANCED_MAX       90
#define PARA_FAST_MAX          120

/* =========================================================================
 * CHRONOMANCER pcdata->powers[] indices
 * ========================================================================= */

/* Ability state (0-9) */
#define CHRONO_QUICKEN_TICKS     0   /* Quicken haste ticks remaining */
#define CHRONO_BLUR_TICKS        1   /* Blur extreme speed ticks remaining */
#define CHRONO_TIMESLIP_READY    2   /* Time Slip counter-attack ready (0/1) */
#define CHRONO_FORESIGHT_TICKS   3   /* Foresight dodge buff ticks remaining */
#define CHRONO_HINDSIGHT_TICKS   4   /* Hindsight learning buff ticks remaining */
#define CHRONO_HINDSIGHT_STACKS  5   /* Hindsight damage bonus stacks (0-5) */
#define CHRONO_ECHO_PENDING      6   /* Temporal Echo rounds until trigger */
#define CHRONO_ECHO_DAMAGE       7   /* Temporal Echo stored damage */
#define CHRONO_STASIS_TICKS      8   /* Stasis freeze ticks remaining */
#define CHRONO_TIMETRAP_TICKS    9   /* Time Trap zone ticks remaining */

/* Training levels (10-12) */
#define CHRONO_TRAIN_ACCEL      10   /* Acceleration level (0-3) */
#define CHRONO_TRAIN_DECEL      11   /* Deceleration level (0-3) */
#define CHRONO_TRAIN_SIGHT      12   /* Temporal Sight level (0-3) */

/* Debuff tracking (13) */
#define CHRONO_SLOW_TICKS       13   /* Slow debuff ticks remaining on target */

/* Cooldowns (14+) */
#define CHRONO_TIMESLIP_CD      14   /* Time Slip cooldown pulses */
#define CHRONO_BLUR_CD          15   /* Blur cooldown pulses */
#define CHRONO_TIMETRAP_CD      16   /* Time Trap cooldown pulses */
#define CHRONO_STASIS_CD        17   /* Stasis cooldown pulses */
#define CHRONO_ECHO_CD          18   /* Temporal Echo cooldown pulses */

/* pcdata->stats[] indices */
#define CHRONO_STAT_STASIS_DMG   0   /* Stasis stored damage */
#define CHRONO_STAT_INSTABILITY  1   /* Times instability triggered (tracking) */

/* =========================================================================
 * PARADOX pcdata->powers[] indices
 * ========================================================================= */

/* Ability state (0-9) - REPLACES Chronomancer states on upgrade */
#define PARA_PAST_SELF_TICKS     0   /* Past Self echo duration */
#define PARA_TIME_LOOP_TICKS     1   /* Time Loop rounds remaining */
#define PARA_TIME_LOOP_TARGET    2   /* Time Loop target ID */
#define PARA_SPLIT_ACTIVE        3   /* Split Timeline active (0/1) */
#define PARA_AGE_TARGET          4   /* Age effect target ID */
#define PARA_AGE_TICKS           5   /* Age debuff ticks remaining */
#define PARA_ETERNITY_TICKS      6   /* Eternity invulnerability ticks */
#define PARA_ETERNITY_AFTERMATH  7   /* Eternity aftermath stun ticks */
#define PARA_ECHO_COUNT          8   /* Temporal echoes for Convergence (0-5) */
#define PARA_FUTURE_STRIKE_TGT   9   /* Future Strike delayed target ID */

/* Training levels (10-12) - REUSES same indices as Chronomancer */
#define PARA_TRAIN_TIMELINE     10   /* Timeline level (0-3) */
#define PARA_TRAIN_COMBAT       11   /* Temporal Combat level (0-4) */
#define PARA_TRAIN_ENTROPY      12   /* Entropy level (0-3) */

/* Cooldowns (14+) */
#define PARA_REWIND_CD          14   /* Rewind cooldown pulses */
#define PARA_SPLIT_CD           15   /* Split Timeline cooldown pulses */
#define PARA_CONVERGENCE_CD     16   /* Convergence cooldown pulses */
#define PARA_FUTURESTRIKE_CD    17   /* Future Strike cooldown pulses */
#define PARA_PASTSELF_CD        18   /* Past Self cooldown pulses */
#define PARA_TIMELOOP_CD        19   /* Time Loop cooldown pulses */

/* pcdata->stats[] indices */
#define PARA_STAT_DAMAGE_HIST_0  0   /* Rewind: damage history tick 0 */
#define PARA_STAT_DAMAGE_HIST_1  1   /* Rewind: damage history tick 1 */
#define PARA_STAT_DAMAGE_HIST_2  2   /* Rewind: damage history tick 2 */
#define PARA_STAT_DAMAGE_HIST_3  3   /* Rewind: damage history tick 3 */
#define PARA_STAT_DAMAGE_HIST_4  4   /* Rewind: damage history tick 4 */
#define PARA_ETERNITY_CD         5   /* Eternity cooldown pulses */
#define PARA_STAT_SPLIT_HP       6   /* Split Timeline HP snapshot */
#define PARA_STAT_SPLIT_MANA     7   /* Split Timeline mana snapshot */
#define PARA_PARADOXSTRIKE_CD    8   /* Paradox Strike cooldown */
#define PARA_AGE_CD              9   /* Age cooldown */
#define PARA_COLLAPSE_CD        10   /* Temporal Collapse cooldown */
#define PARA_DESTABILIZE_CD     11   /* Destabilize cooldown */

/* =========================================================================
 * Function prototypes
 * ========================================================================= */

/* Chronomancer abilities */
void do_flux            ( CHAR_DATA *ch, char *argument );
void do_quicken         ( CHAR_DATA *ch, char *argument );
void do_timeslip        ( CHAR_DATA *ch, char *argument );
void do_blur            ( CHAR_DATA *ch, char *argument );
void do_slow            ( CHAR_DATA *ch, char *argument );
void do_timetrap        ( CHAR_DATA *ch, char *argument );
void do_stasis          ( CHAR_DATA *ch, char *argument );
void do_foresight       ( CHAR_DATA *ch, char *argument );
void do_hindsight       ( CHAR_DATA *ch, char *argument );
void do_temporalecho    ( CHAR_DATA *ch, char *argument );
void do_timetrain       ( CHAR_DATA *ch, char *argument );

/* Paradox abilities */
void do_destabilize     ( CHAR_DATA *ch, char *argument );
void do_rewind          ( CHAR_DATA *ch, char *argument );
void do_splittimeline   ( CHAR_DATA *ch, char *argument );
void do_convergence     ( CHAR_DATA *ch, char *argument );
void do_futurestrike    ( CHAR_DATA *ch, char *argument );
void do_pastself        ( CHAR_DATA *ch, char *argument );
void do_timeloop        ( CHAR_DATA *ch, char *argument );
void do_paradoxstrike   ( CHAR_DATA *ch, char *argument );
void do_age             ( CHAR_DATA *ch, char *argument );
void do_temporalcollapse ( CHAR_DATA *ch, char *argument );
void do_eternity        ( CHAR_DATA *ch, char *argument );
void do_paratrain       ( CHAR_DATA *ch, char *argument );

/* Armor wrappers */
void do_chronoarmor     ( CHAR_DATA *ch, char *argument );
void do_paradoxarmor    ( CHAR_DATA *ch, char *argument );

/* Flux scaling helper */
int get_chrono_power_mod ( CHAR_DATA *ch, bool is_accel );

/* Update functions (called from update.c) */
void update_chronomancer ( CHAR_DATA *ch );
void update_paradox      ( CHAR_DATA *ch );

#endif /* CHRONOMANCER_H */
