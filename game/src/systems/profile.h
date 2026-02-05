/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvance copyright (C) 1992, 1993 by Michael           *
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

/***************************************************************************
 *  profile.h - Performance profiling and tick monitoring system           *
 *                                                                         *
 *  Tracks tick times, update function durations, and warns admins         *
 *  when performance degrades beyond acceptable thresholds.                *
 ***************************************************************************/

#ifndef PROFILE_H
#define PROFILE_H

#include <limits.h>

/* Maximum number of named profiling markers */
#define PROFILE_MAX_MARKERS     32
#define PROFILE_NAME_MAX        32

/* Default threshold for warning (microseconds) - 300ms */
#define PROFILE_DEFAULT_THRESHOLD_US  300000

/* Minimum interval between warnings (seconds) to avoid spam */
#define PROFILE_WARNING_INTERVAL      60

/* Expected pulse duration in microseconds (250ms for 4 pulses/sec) */
#define PROFILE_EXPECTED_PULSE_US     (1000000 / PULSE_PER_SECOND)

/*
 * Statistics for a single profiling marker
 */
typedef struct profile_marker_data {
    char            name[PROFILE_NAME_MAX]; /* Marker name (e.g., "violence_update") */
    long            total_us;               /* Total accumulated time (microseconds) */
    long            min_us;                 /* Minimum single call time */
    long            max_us;                 /* Maximum single call time */
    long            call_count;             /* Number of times called */
    struct timeval  start_time;             /* Start timestamp for current measurement */
    bool            active;                 /* Currently being measured */
} PROFILE_MARKER;

/*
 * Global profiling statistics
 */
typedef struct profile_stats_data {
    /* Tick-level statistics */
    long            tick_count;             /* Total ticks measured */
    long            tick_total_us;          /* Total time across all ticks */
    long            tick_min_us;            /* Fastest tick */
    long            tick_max_us;            /* Slowest tick */
    long            tick_overbudget_count;  /* Ticks exceeding threshold */

    /* Configuration */
    long            threshold_us;           /* Warning threshold (microseconds) */
    bool            enabled;                /* Profiling enabled/disabled */
    bool            verbose;                /* Log individual overbudget ticks */

    /* Rate limiting for warnings */
    time_t          last_warning_time;      /* Last time a warning was logged */
    int             warnings_suppressed;    /* Warnings suppressed since last log */

    /* Per-function markers */
    PROFILE_MARKER  markers[PROFILE_MAX_MARKERS];
    int             marker_count;

    /* Current tick tracking */
    struct timeval  tick_start;             /* When current tick started */
    bool            tick_active;            /* Currently measuring a tick */

    /* Top offenders tracking (for drill-down) */
    int             worst_markers[3];       /* Indices of 3 worst markers this tick */
    long            worst_times[3];         /* Times for worst markers this tick */
} PROFILE_STATS;

/* Global profiling state */
extern PROFILE_STATS profile_stats;

/*
 * Profiling Macros - Lightweight when disabled
 *
 * These macros check the enabled flag first, so there's minimal
 * overhead when profiling is turned off.
 */
#define PROFILE_TICK_START() \
    do { if (profile_stats.enabled) profile_tick_start(); } while(0)

#define PROFILE_TICK_END() \
    do { if (profile_stats.enabled) profile_tick_end(); } while(0)

#define PROFILE_START(name) \
    do { if (profile_stats.enabled) profile_marker_start(name); } while(0)

#define PROFILE_END(name) \
    do { if (profile_stats.enabled) profile_marker_end(name); } while(0)

/*
 * Function Prototypes
 */

/* Initialization */
void    profile_init        ( void );
void    profile_reset       ( void );

/* Tick-level profiling */
void    profile_tick_start  ( void );
void    profile_tick_end    ( void );

/* Marker-level profiling */
void    profile_marker_start( const char *name );
void    profile_marker_end  ( const char *name );

/* Configuration */
void    profile_set_enabled ( bool enabled );
void    profile_set_threshold( long threshold_ms );
void    profile_set_verbose ( bool verbose );

/* Reporting */
void    profile_report      ( CHAR_DATA *ch );
void    profile_report_brief( CHAR_DATA *ch );

#endif /* PROFILE_H */
