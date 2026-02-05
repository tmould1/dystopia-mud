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
 *  profile.c - Performance profiling implementation                       *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "../core/merc.h"
#include "profile.h"

/* Global profiling state */
PROFILE_STATS profile_stats;

/*
 * Helper: Get elapsed time in microseconds
 */
static long profile_elapsed_us( struct timeval *start, struct timeval *end ) {
    return ( end->tv_sec - start->tv_sec ) * 1000000L
         + ( end->tv_usec - start->tv_usec );
}

/*
 * Helper: Find or create a marker by name
 */
static int profile_find_marker( const char *name ) {
    int i;

    /* Search existing markers */
    for ( i = 0; i < profile_stats.marker_count; i++ ) {
        if ( !strcmp( profile_stats.markers[i].name, name ) )
            return i;
    }

    /* Create new marker if room */
    if ( profile_stats.marker_count < PROFILE_MAX_MARKERS ) {
        i = profile_stats.marker_count++;
        strncpy( profile_stats.markers[i].name, name, PROFILE_NAME_MAX - 1 );
        profile_stats.markers[i].name[PROFILE_NAME_MAX - 1] = '\0';
        profile_stats.markers[i].min_us = LONG_MAX;
        profile_stats.markers[i].max_us = 0;
        profile_stats.markers[i].total_us = 0;
        profile_stats.markers[i].call_count = 0;
        profile_stats.markers[i].active = FALSE;
        return i;
    }

    return -1;  /* No room */
}

/*
 * Initialize profiling system
 */
void profile_init( void ) {
    int i;

    memset( &profile_stats, 0, sizeof( profile_stats ) );
    profile_stats.threshold_us = PROFILE_DEFAULT_THRESHOLD_US;
    profile_stats.tick_min_us = LONG_MAX;
    profile_stats.enabled = FALSE;  /* Disabled by default */
    profile_stats.verbose = FALSE;
    profile_stats.tick_multiplier = 1;  /* Normal speed */
    profile_stats.sample_start_time = current_time;

    /* Initialize marker min values */
    for ( i = 0; i < PROFILE_MAX_MARKERS; i++ ) {
        profile_stats.markers[i].min_us = LONG_MAX;
    }

    /* Initialize worst markers */
    for ( i = 0; i < 3; i++ ) {
        profile_stats.worst_markers[i] = -1;
        profile_stats.worst_times[i] = 0;
    }
}

/*
 * Reset all statistics (but keep configuration)
 */
void profile_reset( void ) {
    bool was_enabled = profile_stats.enabled;
    bool was_verbose = profile_stats.verbose;
    long threshold = profile_stats.threshold_us;
    int tick_mult = profile_stats.tick_multiplier;

    profile_init();

    profile_stats.enabled = was_enabled;
    profile_stats.verbose = was_verbose;
    profile_stats.threshold_us = threshold;
    profile_stats.tick_multiplier = tick_mult;
    profile_stats.sample_start_time = current_time;
}

/*
 * Start measuring a tick
 */
void profile_tick_start( void ) {
    int i;

    if ( !profile_stats.enabled )
        return;

    gettimeofday( &profile_stats.tick_start, NULL );
    profile_stats.tick_active = TRUE;

    /* Reset worst markers for this tick */
    for ( i = 0; i < 3; i++ ) {
        profile_stats.worst_markers[i] = -1;
        profile_stats.worst_times[i] = 0;
    }
}

/*
 * End measuring a tick, log if overbudget
 */
void profile_tick_end( void ) {
    struct timeval now;
    long elapsed_us;
    char buf[MAX_STRING_LENGTH];

    if ( !profile_stats.enabled || !profile_stats.tick_active )
        return;

    gettimeofday( &now, NULL );
    elapsed_us = profile_elapsed_us( &profile_stats.tick_start, &now );
    profile_stats.tick_active = FALSE;

    /* Update statistics */
    profile_stats.tick_count++;
    profile_stats.tick_total_us += elapsed_us;

    if ( elapsed_us < profile_stats.tick_min_us )
        profile_stats.tick_min_us = elapsed_us;
    if ( elapsed_us > profile_stats.tick_max_us )
        profile_stats.tick_max_us = elapsed_us;

    /* Check if overbudget */
    if ( elapsed_us > profile_stats.threshold_us ) {
        profile_stats.tick_overbudget_count++;

        /* Rate-limited warning */
        if ( current_time - profile_stats.last_warning_time >= PROFILE_WARNING_INTERVAL ) {
            /* Build warning message */
            snprintf( buf, sizeof( buf ),
                "PERF WARNING: Tick took %ldms (threshold %ldms)",
                elapsed_us / 1000, profile_stats.threshold_us / 1000 );

            if ( profile_stats.warnings_suppressed > 0 ) {
                char tmp[64];
                snprintf( tmp, sizeof( tmp ), " [%d warnings suppressed]",
                    profile_stats.warnings_suppressed );
                strncat( buf, tmp, sizeof( buf ) - strlen( buf ) - 1 );
            }

            log_string( buf );

            /* Report top 3 offenders if verbose */
            if ( profile_stats.verbose ) {
                int i;
                for ( i = 0; i < 3 && profile_stats.worst_markers[i] >= 0; i++ ) {
                    int idx = profile_stats.worst_markers[i];
                    snprintf( buf, sizeof( buf ), "  #%d: %s took %ldms",
                        i + 1,
                        profile_stats.markers[idx].name,
                        profile_stats.worst_times[i] / 1000 );
                    log_string( buf );
                }
            }

            profile_stats.last_warning_time = current_time;
            profile_stats.warnings_suppressed = 0;
        }
        else {
            profile_stats.warnings_suppressed++;
        }
    }
}

/*
 * Start measuring a named marker
 */
void profile_marker_start( const char *name ) {
    int idx;

    if ( !profile_stats.enabled )
        return;

    idx = profile_find_marker( name );
    if ( idx < 0 )
        return;

    gettimeofday( &profile_stats.markers[idx].start_time, NULL );
    profile_stats.markers[idx].active = TRUE;
}

/*
 * End measuring a named marker
 */
void profile_marker_end( const char *name ) {
    int idx, i, j;
    struct timeval now;
    long elapsed_us;
    PROFILE_MARKER *m;

    if ( !profile_stats.enabled )
        return;

    idx = profile_find_marker( name );
    if ( idx < 0 || !profile_stats.markers[idx].active )
        return;

    m = &profile_stats.markers[idx];
    gettimeofday( &now, NULL );
    elapsed_us = profile_elapsed_us( &m->start_time, &now );
    m->active = FALSE;

    /* Update marker statistics */
    m->call_count++;
    m->total_us += elapsed_us;
    if ( elapsed_us < m->min_us )
        m->min_us = elapsed_us;
    if ( elapsed_us > m->max_us )
        m->max_us = elapsed_us;

    /* Track top 3 worst for this tick */
    for ( i = 0; i < 3; i++ ) {
        if ( elapsed_us > profile_stats.worst_times[i] ) {
            /* Shift down */
            for ( j = 2; j > i; j-- ) {
                profile_stats.worst_markers[j] = profile_stats.worst_markers[j - 1];
                profile_stats.worst_times[j] = profile_stats.worst_times[j - 1];
            }
            profile_stats.worst_markers[i] = idx;
            profile_stats.worst_times[i] = elapsed_us;
            break;
        }
    }
}

/*
 * Configuration functions
 */
void profile_set_enabled( bool enabled ) {
    profile_stats.enabled = enabled;
}

void profile_set_threshold( long threshold_ms ) {
    profile_stats.threshold_us = threshold_ms * 1000L;
}

void profile_set_verbose( bool verbose ) {
    profile_stats.verbose = verbose;
}

/*
 * Generate full profiling report
 */
void profile_report( CHAR_DATA *ch ) {
    char buf[MAX_STRING_LENGTH];
    long avg_tick_us;
    int i;

    send_to_char( "#R===== #yPerformance Profile Report #R=====#n\n\r\n\r", ch );

    /* Status */
    snprintf( buf, sizeof( buf ), "Status: %s  |  Verbose: %s  |  Threshold: %ldms\n\r",
        profile_stats.enabled ? "#GEnabled#n" : "#RDisabled#n",
        profile_stats.verbose ? "On" : "Off",
        profile_stats.threshold_us / 1000 );
    send_to_char( buf, ch );

    /* Speed and timing info */
    {
        int mult = profile_stats.tick_multiplier > 0 ? profile_stats.tick_multiplier : 1;
        int effective_pps = PULSE_PER_SECOND * mult;
        time_t real_elapsed = current_time - profile_stats.sample_start_time;
        time_t effective_elapsed = real_elapsed * mult;

        snprintf( buf, sizeof( buf ), "Speed: #C%dx#n (%d pulses/sec)  |  Real: %lds  |  Effective: %lds\n\r",
            mult, effective_pps, (long)real_elapsed, (long)effective_elapsed );
        send_to_char( buf, ch );
    }

    if ( profile_stats.tick_count == 0 ) {
        send_to_char( "\n\rNo data collected yet.\n\r", ch );
        return;
    }

    /* Tick statistics */
    avg_tick_us = profile_stats.tick_total_us / profile_stats.tick_count;

    send_to_char( "\n\r#CTick Statistics:#n\n\r", ch );
    snprintf( buf, sizeof( buf ), "  Ticks measured: %ld\n\r", profile_stats.tick_count );
    send_to_char( buf, ch );
    {
        int mult = profile_stats.tick_multiplier > 0 ? profile_stats.tick_multiplier : 1;
        double expected_us = 1000000.0 / ( PULSE_PER_SECOND * mult );
        snprintf( buf, sizeof( buf ), "  Average: %.2fms  (expected: %.2fms at %dx)\n\r",
            avg_tick_us / 1000.0, expected_us / 1000.0, mult );
    }
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Min: %.2fms  Max: %.2fms\n\r",
        profile_stats.tick_min_us / 1000.0, profile_stats.tick_max_us / 1000.0 );
    send_to_char( buf, ch );
    snprintf( buf, sizeof( buf ), "  Overbudget: %ld ticks (%.1f%%)\n\r",
        profile_stats.tick_overbudget_count,
        100.0 * profile_stats.tick_overbudget_count / profile_stats.tick_count );
    send_to_char( buf, ch );

    /* Function breakdown */
    if ( profile_stats.marker_count > 0 ) {
        long total_work_us = 0;
        const char *pct_basis = "tick";

        /* Find game_loop_work marker to use as percentage basis */
        for ( i = 0; i < profile_stats.marker_count; i++ ) {
            if ( !strcmp( profile_stats.markers[i].name, "game_loop_work" ) ) {
                total_work_us = profile_stats.markers[i].total_us;
                pct_basis = "loop";
                break;
            }
        }
        /* Fall back to tick time if no game_loop marker */
        if ( total_work_us == 0 )
            total_work_us = profile_stats.tick_total_us;

        send_to_char( "\n\r#CFunction Breakdown:#n\n\r", ch );
        snprintf( buf, sizeof( buf ), "  Name                 Calls     Avg(ms)   Max(ms)  %%(%s)\n\r", pct_basis );
        send_to_char( buf, ch );
        send_to_char( "  -------------------  --------  --------  -------  ------\n\r", ch );

        for ( i = 0; i < profile_stats.marker_count; i++ ) {
            PROFILE_MARKER *m = &profile_stats.markers[i];
            long avg_us;
            double pct;

            if ( m->call_count == 0 )
                continue;

            avg_us = m->total_us / m->call_count;
            pct = ( total_work_us > 0 )
                ? 100.0 * m->total_us / total_work_us
                : 0.0;

            snprintf( buf, sizeof( buf ), "  %-19s  %8ld  %8.2f  %7.2f  %5.1f%%\n\r",
                m->name, m->call_count,
                avg_us / 1000.0, m->max_us / 1000.0, pct );
            send_to_char( buf, ch );
        }
    }
}

/*
 * Brief report (current tick status only)
 */
void profile_report_brief( CHAR_DATA *ch ) {
    char buf[MAX_STRING_LENGTH];
    long avg;

    if ( profile_stats.tick_count == 0 ) {
        send_to_char( "No profiling data.\n\r", ch );
        return;
    }

    avg = profile_stats.tick_total_us / profile_stats.tick_count;
    snprintf( buf, sizeof( buf ),
        "Ticks: %ld  Avg: %.1fms  Max: %.1fms  Overbudget: %ld\n\r",
        profile_stats.tick_count, avg / 1000.0,
        profile_stats.tick_max_us / 1000.0,
        profile_stats.tick_overbudget_count );
    send_to_char( buf, ch );
}

/*
 * Admin command: profile [on|off|reset|verbose|threshold <ms>|speed <1-16>|report|brief]
 */
void do_profile( CHAR_DATA *ch, char *argument ) {
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || !str_cmp( arg, "report" ) ) {
        /* Full report */
        profile_report( ch );
        return;
    }

    if ( !str_cmp( arg, "on" ) ) {
        profile_stats.enabled = TRUE;
        send_to_char( "Profiling enabled.\n\r", ch );
        snprintf( buf, sizeof( buf ), "%s enabled profiling", ch->name );
        log_string( buf );
        return;
    }

    if ( !str_cmp( arg, "off" ) ) {
        profile_stats.enabled = FALSE;
        send_to_char( "Profiling disabled.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg, "reset" ) ) {
        profile_reset();
        send_to_char( "Profiling statistics reset.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg, "verbose" ) ) {
        profile_stats.verbose = !profile_stats.verbose;
        snprintf( buf, sizeof( buf ), "Verbose logging %s.\n\r",
            profile_stats.verbose ? "enabled" : "disabled" );
        send_to_char( buf, ch );
        return;
    }

    if ( !str_cmp( arg, "threshold" ) ) {
        int ms;
        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' || !is_number( arg ) ) {
            send_to_char( "Usage: profile threshold <milliseconds>\n\r", ch );
            return;
        }
        ms = atoi( arg );
        if ( ms < 50 || ms > 5000 ) {
            send_to_char( "Threshold must be 50-5000 milliseconds.\n\r", ch );
            return;
        }
        profile_stats.threshold_us = ms * 1000L;
        snprintf( buf, sizeof( buf ), "Warning threshold set to %dms.\n\r", ms );
        send_to_char( buf, ch );
        return;
    }

    if ( !str_cmp( arg, "brief" ) ) {
        profile_report_brief( ch );
        return;
    }

    if ( !str_cmp( arg, "speed" ) ) {
        int mult;
        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' || !is_number( arg ) ) {
            snprintf( buf, sizeof( buf ), "Current speed: %dx (%d pulses/sec)\n\r"
                "Usage: profile speed <1-16>\n\r",
                profile_stats.tick_multiplier > 0 ? profile_stats.tick_multiplier : 1,
                PULSE_PER_SECOND * ( profile_stats.tick_multiplier > 0 ? profile_stats.tick_multiplier : 1 ) );
            send_to_char( buf, ch );
            return;
        }
        mult = atoi( arg );
        if ( mult < 1 || mult > 512 ) {
            send_to_char( "Speed multiplier must be 1-512.\n\r", ch );
            return;
        }
        profile_stats.tick_multiplier = mult;
        snprintf( buf, sizeof( buf ), "Tick speed set to %dx (effective %d pulses/sec).\n\r",
            mult, PULSE_PER_SECOND * mult );
        send_to_char( buf, ch );
        snprintf( buf, sizeof( buf ), "%s set profile speed to %dx", ch->name, mult );
        log_string( buf );
        return;
    }

    send_to_char( "Usage: profile [on|off|reset|verbose|threshold <ms>|speed <1-16>|report|brief]\n\r", ch );
}
