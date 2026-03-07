/***************************************************************************
 *  quest_new.c - Quest system engine
 *
 *  Implements the quest command, progress checking, event hooks,
 *  and quest state management.
 ***************************************************************************/

#include "../core/merc.h"
#include "../db/db_quest.h"
#include "quest_new.h"

#include <string.h>
#include <time.h>

/*--------------------------------------------------------------------------
 * Forward declarations
 *--------------------------------------------------------------------------*/

static void quest_show_active( CHAR_DATA *ch );
static void quest_show_list( CHAR_DATA *ch, char *argument );
static void quest_show_progress( CHAR_DATA *ch );
static void quest_show_path( CHAR_DATA *ch );
static void quest_show_history( CHAR_DATA *ch );
static void quest_do_accept( CHAR_DATA *ch, char *argument );
static void quest_do_complete( CHAR_DATA *ch, char *argument );
static void quest_do_abandon( CHAR_DATA *ch, char *argument );

static bool quest_is_visible( CHAR_DATA *ch, const QUEST_DEF *q );
static bool quest_prereqs_met( CHAR_DATA *ch, const QUEST_DEF *q );
static bool quest_objectives_met( CHAR_DATA *ch, int quest_index );
static void quest_award_rewards( CHAR_DATA *ch, const QUEST_DEF *q );
static void quest_auto_complete( CHAR_DATA *ch, int quest_index );

/*--------------------------------------------------------------------------
 * UTF-8 display symbols
 *--------------------------------------------------------------------------*/

#define U_CHECK    "\xe2\x9c\x93"   /* ✓ */
#define U_CROSS    "\xe2\x9c\x97"   /* ✗ */
#define U_ARROW    "\xe2\x96\xb8"   /* ▸ */
#define U_DIAMOND  "\xe2\x97\x86"   /* ◆ */
#define U_CIRCLE   "\xe2\x97\x8b"   /* ○ */
#define U_STAR     "\xe2\x9c\xa6"   /* ✦ */
#define U_BULLET   "\xe2\x80\xa2"   /* • */

/*--------------------------------------------------------------------------
 * Helper: category color code
 *--------------------------------------------------------------------------*/

static const char *quest_cat_color( const char *cat ) {
    if ( !strcmp( cat, "M"  ) ) return "#tFF6B35";
    if ( !strcmp( cat, "T"  ) ) return "#x039";
    if ( !strcmp( cat, "C"  ) ) return "#x160";
    if ( !strcmp( cat, "E"  ) ) return "#x035";
    if ( !strcmp( cat, "CL" ) ) return "#x135";
    if ( !strcmp( cat, "CR" ) ) return "#x208";
    if ( !strcmp( cat, "D"  ) ) return "#x220";
    if ( !strcmp( cat, "A"  ) ) return "#x096";
    return "#w";
}

/*--------------------------------------------------------------------------
 * Main command: do_quest
 *--------------------------------------------------------------------------*/

void do_quest( CHAR_DATA *ch, char *argument ) {
    char arg[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) ) {
        send_to_char( "NPCs cannot use quests.\n\r", ch );
        return;
    }

    if ( !ch->pcdata->quest_tracker ) {
        send_to_char( "Your quest tracker is not initialized.\n\r", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        quest_show_progress( ch );
        return;
    }

    if ( !str_cmp( arg, "list" ) )     { quest_show_list( ch, argument );    return; }
    if ( !str_cmp( arg, "accept" ) )   { quest_do_accept( ch, argument );    return; }
    if ( !str_cmp( arg, "progress" ) ) { quest_show_progress( ch );          return; }
    if ( !str_cmp( arg, "complete" ) ) { quest_do_complete( ch, argument );  return; }
    if ( !str_cmp( arg, "abandon" ) )  { quest_do_abandon( ch, argument );   return; }
    if ( !str_cmp( arg, "path" ) )     { quest_show_path( ch );              return; }
    if ( !str_cmp( arg, "history" ) )  { quest_show_history( ch );           return; }

    send_to_char( "\n\r #tFFD700" U_STAR " Quest Commands " U_STAR "#n\n\r\n\r", ch );
    send_to_char( "  #Cquest#n              Show current progress\n\r", ch );
    send_to_char( "  #Cquest list#n [cat]   Show available quests\n\r", ch );
    send_to_char( "  #Cquest accept#n <id>  Accept a quest\n\r", ch );
    send_to_char( "  #Cquest progress#n     Show detailed progress\n\r", ch );
    send_to_char( "  #Cquest complete#n     Turn in completed quests\n\r", ch );
    send_to_char( "  #Cquest abandon#n <id> Abandon a quest\n\r", ch );
    send_to_char( "  #Cquest path#n         Show main progression\n\r", ch );
    send_to_char( "  #Cquest history#n      Show completed quests\n\r", ch );
}

/*--------------------------------------------------------------------------
 * Visibility: is this quest visible to the character?
 *--------------------------------------------------------------------------*/

static bool quest_is_visible( CHAR_DATA *ch, const QUEST_DEF *q ) {
    if ( !q ) return FALSE;

    /* Class restriction */
    if ( q->required_class != 0 && !IS_SET( ch->class, q->required_class ) )
        return FALSE;

    /* FTUE level restriction */
    if ( ch->explevel < q->min_explevel )
        return FALSE;
    if ( ch->explevel > q->max_explevel )
        return FALSE;

    return TRUE;
}

/*--------------------------------------------------------------------------
 * Prerequisites check
 *--------------------------------------------------------------------------*/

static bool quest_prereqs_met( CHAR_DATA *ch, const QUEST_DEF *q ) {
    QUEST_TRACKER *tracker = ch->pcdata->quest_tracker;
    int i;
    bool has_any = FALSE;

    if ( !q || !tracker ) return FALSE;

    /* No prerequisites = always met */
    if ( q->prereq_count == 0 )
        return TRUE;

    /*
     * Tutorial quests (T01-T08) use OR logic for M01's prerequisites:
     * M01 requires EITHER T05 OR T08 (different FTUE paths).
     * For simplicity, if the quest is M01, use OR logic.
     * All other quests use AND logic (all prereqs required).
     */
    if ( !strcmp( q->id, "M01" ) ) {
        /* OR: any completed prereq suffices */
        for ( i = 0; i < q->prereq_count; i++ ) {
            QUEST_PROGRESS *pp = quest_tracker_find( tracker, q->prereq_indices[i] );
            if ( pp && pp->status >= QSTATUS_TURNED_IN )
                return TRUE;

            /* Also check if the prereq quest was auto-skipped (FTUE) */
            const QUEST_DEF *prereq_q = quest_def_by_index( q->prereq_indices[i] );
            if ( prereq_q && !quest_is_visible( ch, prereq_q ) ) {
                has_any = TRUE;
            }
        }
        /* If all prereqs were FTUE-skipped, allow it */
        return has_any;
    }

    /* AND: all prereqs must be turned in or FTUE-skipped */
    for ( i = 0; i < q->prereq_count; i++ ) {
        const QUEST_DEF *prereq_q = quest_def_by_index( q->prereq_indices[i] );

        /* If prereq quest is not visible (FTUE filtered), skip it */
        if ( prereq_q && !quest_is_visible( ch, prereq_q ) )
            continue;

        /* CL_xxx prereqs: skip if it's a class quest for a different class */
        if ( prereq_q && prereq_q->required_class != 0
                && !IS_SET( ch->class, prereq_q->required_class ) )
            continue;

        QUEST_PROGRESS *pp = quest_tracker_find( tracker, q->prereq_indices[i] );
        if ( !pp || pp->status < QSTATUS_TURNED_IN )
            return FALSE;
    }
    return TRUE;
}

/*--------------------------------------------------------------------------
 * Quest Init: evaluate all quests for a newly logged-in player
 *--------------------------------------------------------------------------*/

void quest_init_player( CHAR_DATA *ch ) {
    QUEST_TRACKER *tracker;
    const char *entry_id;
    int qi;
    QUEST_PROGRESS *p;

    if ( IS_NPC( ch ) || !ch->pcdata->quest_tracker )
        return;

    tracker = ch->pcdata->quest_tracker;

    /*
     * Auto-grant first quest for brand new players.
     * Must run BEFORE quest_evaluate_availability() which creates AVAILABLE
     * entries for prerequisite-free quests — those would cause the
     * has_progress check below to incorrectly think the player already
     * has quest activity.
     */
    {
        int i;
        bool has_progress = FALSE;
        for ( i = 0; i < tracker->count; i++ ) {
            if ( tracker->entries[i].status > QSTATUS_LOCKED ) {
                has_progress = TRUE;
                break;
            }
        }

        if ( !has_progress && quest_def_count() > 0 ) {
            /* Pick entry quest based on FTUE experience level */
            switch ( ch->explevel ) {
                case 0:  entry_id = "T01"; break;  /* complete newbie */
                case 1:  entry_id = "T06"; break;  /* MUD experience, not Dystopia */
                default: entry_id = "M01"; break;  /* veteran */
            }

            qi = quest_def_index_by_id( entry_id );
            if ( qi >= 0 ) {
                p = quest_tracker_get( tracker, qi );
                if ( p && p->status == QSTATUS_LOCKED ) {
                    p->status     = QSTATUS_ACTIVE;
                    p->started_at = (int) current_time;

                    {
                        const QUEST_DEF *q = quest_def_by_index( qi );
                        char buf[MAX_STRING_LENGTH];
                        snprintf( buf, sizeof( buf ),
                            "\n\r  #tFFD700" U_STAR " New Quest: #C%s#n\n\r"
                            "  %s\n\r"
                            "  Type '#tFFD700quest progress#n' to see your objectives.\n\r\n\r",
                            q ? q->name : entry_id,
                            q ? q->description : "" );
                        send_to_char( buf, ch );
                    }
                }
            }
        }
    }

    /* Evaluate what's available based on existing progress */
    quest_evaluate_availability( ch );
    quest_check_milestones( ch );
}

/*--------------------------------------------------------------------------
 * Grant entry quest: backup method for players who missed auto-grant.
 * Picks T01/T06/M01 based on explevel, activates if LOCKED or AVAILABLE.
 * Returns TRUE if a quest was granted.
 *--------------------------------------------------------------------------*/

bool quest_grant_entry( CHAR_DATA *ch ) {
    QUEST_TRACKER *tracker;
    const char *entry_id;
    int qi;
    QUEST_PROGRESS *p;

    if ( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->quest_tracker )
        return FALSE;

    tracker = ch->pcdata->quest_tracker;

    if ( quest_def_count() == 0 )
        return FALSE;

    switch ( ch->explevel ) {
        case 0:  entry_id = "T01"; break;
        case 1:  entry_id = "T06"; break;
        default: entry_id = "M01"; break;
    }

    qi = quest_def_index_by_id( entry_id );
    if ( qi < 0 )
        return FALSE;

    p = quest_tracker_get( tracker, qi );
    if ( !p || p->status >= QSTATUS_ACTIVE )
        return FALSE;  /* already active or beyond */

    p->status     = QSTATUS_ACTIVE;
    p->started_at = (int) current_time;

    {
        const QUEST_DEF *q = quest_def_by_index( qi );
        char buf[MAX_STRING_LENGTH];
        snprintf( buf, sizeof( buf ),
            "\n\r  #tFFD700" U_STAR " New Quest: #C%s#n\n\r"
            "  %s\n\r"
            "  Type '#tFFD700quest progress#n' to see your objectives.\n\r\n\r",
            q ? q->name : entry_id,
            q ? q->description : "" );
        send_to_char( buf, ch );
    }

    quest_check_milestones( ch );
    return TRUE;
}

/*--------------------------------------------------------------------------
 * Evaluate availability: unlock quests whose prereqs are met
 *--------------------------------------------------------------------------*/

void quest_evaluate_availability( CHAR_DATA *ch ) {
    QUEST_TRACKER *tracker;
    int i, changed;

    if ( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->quest_tracker )
        return;

    tracker = ch->pcdata->quest_tracker;

    /* Iterate until no more changes (cascading unlocks) */
    do {
        changed = 0;
        for ( i = 0; i < quest_def_count(); i++ ) {
            const QUEST_DEF *q = quest_def_by_index( i );
            QUEST_PROGRESS *p;

            if ( !quest_is_visible( ch, q ) )
                continue;

            p = quest_tracker_find( tracker, i );

            /* Already beyond locked state */
            if ( p && p->status > QSTATUS_LOCKED )
                continue;

            /* Check prerequisites */
            if ( !quest_prereqs_met( ch, q ) )
                continue;

            /* Unlock this quest */
            p = quest_tracker_get( tracker, i );
            if ( p && p->status == QSTATUS_LOCKED ) {
                /* Auto-complete quests skip to active immediately */
                if ( IS_SET( q->flags, QFLAG_AUTO_COMPLETE ) ) {
                    p->status     = QSTATUS_ACTIVE;
                    p->started_at = (int) current_time;
                } else {
                    p->status = QSTATUS_AVAILABLE;
                }
                changed = 1;
            }
        }
    } while ( changed );
}

/*--------------------------------------------------------------------------
 * Check if all objectives are met for a quest
 *--------------------------------------------------------------------------*/

static bool quest_objectives_met( CHAR_DATA *ch, int quest_index ) {
    const QUEST_DEF *q = quest_def_by_index( quest_index );
    QUEST_PROGRESS *p;
    int i;

    if ( !q || !ch->pcdata->quest_tracker ) return FALSE;

    p = quest_tracker_find( ch->pcdata->quest_tracker, quest_index );
    if ( !p ) return FALSE;

    for ( i = 0; i < q->obj_count; i++ ) {
        if ( p->obj_progress[i].current < q->objectives[i].threshold )
            return FALSE;
    }
    return TRUE;
}

/*--------------------------------------------------------------------------
 * Award rewards for completing a quest
 *--------------------------------------------------------------------------*/

static void quest_award_rewards( CHAR_DATA *ch, const QUEST_DEF *q ) {
    char buf[MAX_STRING_LENGTH];

    if ( q->qp_reward > 0 ) {
        ch->pcdata->quest += q->qp_reward;
        ch->pcdata->questtotal += q->qp_reward;
        snprintf( buf, sizeof( buf ),
            "  #x035" U_BULLET " #x035+%d#n quest points\n\r", q->qp_reward );
        send_to_char( buf, ch );
    }

    if ( q->exp_reward > 0 ) {
        ch->exp += q->exp_reward;
        snprintf( buf, sizeof( buf ),
            "  #x035" U_BULLET " #x035+%d#n experience\n\r", q->exp_reward );
        send_to_char( buf, ch );
    }

    if ( q->primal_reward > 0 ) {
        ch->practice += q->primal_reward;
        snprintf( buf, sizeof( buf ),
            "  #x035" U_BULLET " #x035+%d#n primal energy\n\r", q->primal_reward );
        send_to_char( buf, ch );
    }
}

/*--------------------------------------------------------------------------
 * Auto-complete: check and complete auto-complete quests
 *--------------------------------------------------------------------------*/

static void quest_auto_complete( CHAR_DATA *ch, int quest_index ) {
    const QUEST_DEF *q = quest_def_by_index( quest_index );
    QUEST_PROGRESS *p;
    char buf[MAX_STRING_LENGTH];

    if ( !q || !IS_SET( q->flags, QFLAG_AUTO_COMPLETE ) )
        return;

    p = quest_tracker_find( ch->pcdata->quest_tracker, quest_index );
    if ( !p || p->status != QSTATUS_ACTIVE )
        return;

    if ( !quest_objectives_met( ch, quest_index ) )
        return;

    /* Complete and award */
    p->status       = QSTATUS_TURNED_IN;
    p->completed_at = (int) current_time;

    snprintf( buf, sizeof( buf ),
        "\n\r  #tFFD700" U_STAR " Quest Complete: #x035%s #tFFD700" U_STAR "#n\n\r", q->name );
    send_to_char( buf, ch );
    quest_award_rewards( ch, q );

    /* Re-evaluate availability for cascading unlocks */
    quest_evaluate_availability( ch );

    /* Check milestones on newly unlocked quests */
    quest_check_milestones( ch );
}

/*--------------------------------------------------------------------------
 * Event Hook: quest_check_progress
 *--------------------------------------------------------------------------*/

void quest_check_progress( CHAR_DATA *ch, const char *type,
                           const char *target, int value ) {
    QUEST_TRACKER *tracker;
    int i, j;

    if ( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->quest_tracker )
        return;

    tracker = ch->pcdata->quest_tracker;

    for ( i = 0; i < tracker->count; i++ ) {
        QUEST_PROGRESS *p = &tracker->entries[i];
        const QUEST_DEF *q;

        if ( p->status != QSTATUS_ACTIVE )
            continue;

        q = quest_def_by_index( p->quest_index );
        if ( !q ) continue;

        for ( j = 0; j < q->obj_count; j++ ) {
            QUEST_OBJ_DEF *obj = (QUEST_OBJ_DEF *)&q->objectives[j];

            /* Already met this objective */
            if ( p->obj_progress[j].current >= obj->threshold )
                continue;

            /* Type must match */
            if ( strcmp( obj->type, type ) != 0 )
                continue;

            /* Target matching: "any" matches everything */
            if ( strcmp( obj->target, "any" ) != 0
                    && strcmp( obj->target, target ) != 0 )
                continue;

            /* Update progress */
            p->obj_progress[j].current += value;
            if ( p->obj_progress[j].current > obj->threshold )
                p->obj_progress[j].current = obj->threshold;
        }

        /* Check for auto-complete */
        quest_auto_complete( ch, p->quest_index );
    }
}

/*--------------------------------------------------------------------------
 * Milestone Check: evaluate REACH_* objectives by character state
 *--------------------------------------------------------------------------*/

void quest_check_milestones( CHAR_DATA *ch ) {
    QUEST_TRACKER *tracker;
    int i, j;

    if ( IS_NPC( ch ) || !ch->pcdata || !ch->pcdata->quest_tracker )
        return;

    tracker = ch->pcdata->quest_tracker;

    for ( i = 0; i < tracker->count; i++ ) {
        QUEST_PROGRESS *p = &tracker->entries[i];
        const QUEST_DEF *q;

        if ( p->status != QSTATUS_ACTIVE )
            continue;

        q = quest_def_by_index( p->quest_index );
        if ( !q ) continue;

        for ( j = 0; j < q->obj_count; j++ ) {
            QUEST_OBJ_DEF *obj = (QUEST_OBJ_DEF *)&q->objectives[j];
            int cur_val = 0;

            /* Already met */
            if ( p->obj_progress[j].current >= obj->threshold )
                continue;

            /* Evaluate milestone objectives by checking current state */
            if ( !strcmp( obj->type, QOBJ_REACH_STAT ) ) {
                if ( !strcmp( obj->target, "hp" ) )
                    cur_val = ch->max_hit;
                else if ( !strcmp( obj->target, "mana" ) )
                    cur_val = ch->max_mana;
                else if ( !strcmp( obj->target, "move" ) )
                    cur_val = ch->max_move;
                else if ( !strcmp( obj->target, "class" ) )
                    cur_val = ( ch->class != 0 ) ? 1 : 0;
            }
            else if ( !strcmp( obj->type, QOBJ_REACH_GEN ) ) {
                /* Lower gen = better; check if gen <= threshold */
                if ( ch->generation >= 1 && ch->generation <= obj->threshold )
                    cur_val = obj->threshold;
            }
            else if ( !strcmp( obj->type, QOBJ_REACH_PKSCORE ) ) {
                cur_val = get_ratio( ch );
            }
            else if ( !strcmp( obj->type, QOBJ_REACH_UPGRADE ) ) {
                cur_val = ch->pcdata->upgrade_level;
            }
            else if ( !strcmp( obj->type, QOBJ_EARN_QP ) ) {
                cur_val = ch->pcdata->questtotal;
            }
            else if ( !strcmp( obj->type, QOBJ_MASTERY ) ) {
                if ( IS_SET( ch->newbits, NEW_MASTERY ) )
                    cur_val = 1;
            }
            else {
                continue;  /* Not a milestone type */
            }

            /* Always store current value so progress is visible */
            if ( cur_val > p->obj_progress[j].current )
                p->obj_progress[j].current = cur_val;
        }

        /* Check for auto-complete after milestone evaluation */
        quest_auto_complete( ch, p->quest_index );
    }
}

/*--------------------------------------------------------------------------
 * Display: Show active quests
 *--------------------------------------------------------------------------*/

static void quest_show_active( CHAR_DATA *ch ) {
    QUEST_TRACKER *tracker = ch->pcdata->quest_tracker;
    char buf[MAX_STRING_LENGTH];
    int i, count = 0;

    send_to_char( "\n\r #tFFD700" U_STAR " Active Quests " U_STAR "#n\n\r\n\r", ch );

    for ( i = 0; i < tracker->count; i++ ) {
        QUEST_PROGRESS *p = &tracker->entries[i];
        const QUEST_DEF *q;

        if ( p->status != QSTATUS_ACTIVE && p->status != QSTATUS_COMPLETE )
            continue;

        q = quest_def_by_index( p->quest_index );
        if ( !q ) continue;

        snprintf( buf, sizeof( buf ), "  %s #C%-14s#n %s",
            ( p->status == QSTATUS_COMPLETE )
                ? "#x035" U_CHECK "#n" : "#x039" U_ARROW "#n",
            q->id, q->name );
        send_to_char( buf, ch );

        if ( p->status == QSTATUS_COMPLETE )
            send_to_char( "  #x035" U_CHECK " Ready#n", ch );

        send_to_char( "\n\r", ch );
        count++;
    }

    if ( count == 0 )
        send_to_char( "  No active quests. Use '#tFFD700quest list#n' to see available quests.\n\r", ch );

    send_to_char( "\n\r", ch );
}

/*--------------------------------------------------------------------------
 * Display: List available quests
 *--------------------------------------------------------------------------*/

static void quest_show_list( CHAR_DATA *ch, char *argument ) {
    QUEST_TRACKER *tracker = ch->pcdata->quest_tracker;
    char buf[MAX_STRING_LENGTH];
    char cat_filter[MAX_INPUT_LENGTH];
    int i, count = 0;

    one_argument( argument, cat_filter );

    send_to_char( "\n\r #tFFD700" U_STAR " Available Quests " U_STAR "#n\n\r\n\r", ch );

    for ( i = 0; i < quest_def_count(); i++ ) {
        const QUEST_DEF *q = quest_def_by_index( i );
        QUEST_PROGRESS *p;
        const char *icon;

        if ( !quest_is_visible( ch, q ) )
            continue;

        /* Category filter */
        if ( cat_filter[0] != '\0' && str_cmp( cat_filter, q->category ) )
            continue;

        p = quest_tracker_find( tracker, i );

        /* Show available and active */
        if ( !p || p->status == QSTATUS_LOCKED )
            continue;
        if ( p->status >= QSTATUS_TURNED_IN )
            continue;

        if ( p->status == QSTATUS_COMPLETE )
            icon = "#x035" U_CHECK "#n";
        else if ( p->status == QSTATUS_ACTIVE )
            icon = "#x039" U_ARROW "#n";
        else
            icon = "#tFFD700" U_DIAMOND "#n";

        snprintf( buf, sizeof( buf ), "  %s #C%-14s#n %s%-2s#n %s",
            icon, q->id, quest_cat_color( q->category ),
            q->category, q->name );
        send_to_char( buf, ch );

        if ( p->status == QSTATUS_ACTIVE )
            send_to_char( " #x039Active#n", ch );
        else if ( p->status == QSTATUS_COMPLETE )
            send_to_char( " #x035Complete#n", ch );

        if ( q->qp_reward > 0 ) {
            snprintf( buf, sizeof( buf ), " #x243(#tFFD700+%d QP#x243)#n", q->qp_reward );
            send_to_char( buf, ch );
        }

        send_to_char( "\n\r", ch );
        count++;
    }

    if ( count == 0 )
        send_to_char( "  No quests available.\n\r", ch );

    send_to_char( "\n\r", ch );
}

/*--------------------------------------------------------------------------
 * Display: Show detailed progress for active quests
 *--------------------------------------------------------------------------*/

static void quest_show_progress( CHAR_DATA *ch ) {
    QUEST_TRACKER *tracker = ch->pcdata->quest_tracker;
    char buf[MAX_STRING_LENGTH];
    int i, j, count = 0;

    send_to_char( "\n\r #tFFD700" U_STAR " Quest Progress " U_STAR "#n\n\r\n\r", ch );

    for ( i = 0; i < tracker->count; i++ ) {
        QUEST_PROGRESS *p = &tracker->entries[i];
        const QUEST_DEF *q;

        if ( p->status != QSTATUS_ACTIVE && p->status != QSTATUS_COMPLETE )
            continue;

        q = quest_def_by_index( p->quest_index );
        if ( !q ) continue;

        snprintf( buf, sizeof( buf ), "  #C%s#n #x243-#n %s\n\r", q->id, q->name );
        send_to_char( buf, ch );

        for ( j = 0; j < q->obj_count; j++ ) {
            bool met = ( p->obj_progress[j].current >= q->objectives[j].threshold );
            snprintf( buf, sizeof( buf ), "    %s %s #x243(%d/%d)#n\n\r",
                met ? "#x035" U_CHECK "#n" : "#x160" U_CIRCLE "#n",
                q->objectives[j].description,
                p->obj_progress[j].current,
                q->objectives[j].threshold );
            send_to_char( buf, ch );
        }
        send_to_char( "\n\r", ch );
        count++;
    }

    if ( count == 0 )
        send_to_char( "  No active quests.\n\r\n\r", ch );
}

/*--------------------------------------------------------------------------
 * Display: Show main progression path
 *--------------------------------------------------------------------------*/

static void quest_show_path( CHAR_DATA *ch ) {
    QUEST_TRACKER *tracker = ch->pcdata->quest_tracker;
    char buf[MAX_STRING_LENGTH];
    int i;

    send_to_char( "\n\r #tFFD700" U_STAR " Main Progression " U_STAR "#n\n\r\n\r", ch );

    for ( i = 0; i < quest_def_count(); i++ ) {
        const QUEST_DEF *q = quest_def_by_index( i );
        QUEST_PROGRESS *p;
        const char *status_str;

        if ( !q || strcmp( q->category, "M" ) != 0 )
            continue;

        p = quest_tracker_find( tracker, i );

        if ( !p || p->status == QSTATUS_LOCKED )
            status_str = "#x240" U_CIRCLE "#n";
        else if ( p->status == QSTATUS_AVAILABLE )
            status_str = "#tFFD700" U_DIAMOND "#n";
        else if ( p->status == QSTATUS_ACTIVE )
            status_str = "#x039" U_ARROW "#n";
        else if ( p->status == QSTATUS_COMPLETE )
            status_str = "#x226" U_STAR "#n";
        else
            status_str = "#x035" U_CHECK "#n";

        snprintf( buf, sizeof( buf ), "  %s  #C%-6s#n %s",
            status_str, q->id, q->name );
        send_to_char( buf, ch );

        if ( q->qp_reward > 0 ) {
            snprintf( buf, sizeof( buf ), " #x243(#tFFD700+%d QP#x243)#n", q->qp_reward );
            send_to_char( buf, ch );
        }

        send_to_char( "\n\r", ch );
    }
    send_to_char( "\n\r  #x240" U_CIRCLE "#n Locked  #tFFD700" U_DIAMOND "#n Available  "
        "#x039" U_ARROW "#n Active  #x226" U_STAR "#n Complete  "
        "#x035" U_CHECK "#n Done\n\r\n\r", ch );
}

/*--------------------------------------------------------------------------
 * Display: Show completed quest history
 *--------------------------------------------------------------------------*/

static void quest_show_history( CHAR_DATA *ch ) {
    QUEST_TRACKER *tracker = ch->pcdata->quest_tracker;
    char buf[MAX_STRING_LENGTH];
    int i, count = 0;

    send_to_char( "\n\r #tFFD700" U_STAR " Completed Quests " U_STAR "#n\n\r\n\r", ch );

    for ( i = 0; i < tracker->count; i++ ) {
        QUEST_PROGRESS *p = &tracker->entries[i];
        const QUEST_DEF *q;

        if ( p->status != QSTATUS_TURNED_IN )
            continue;

        q = quest_def_by_index( p->quest_index );
        if ( !q ) continue;

        snprintf( buf, sizeof( buf ), "  #x035" U_CHECK "#n #C%-14s#n %s%-2s#n %s\n\r",
            q->id, quest_cat_color( q->category ),
            q->category, q->name );
        send_to_char( buf, ch );
        count++;
    }

    if ( count == 0 )
        send_to_char( "  No completed quests yet.\n\r", ch );

    snprintf( buf, sizeof( buf ), "\n\r  Total completed: #tFFD700%d#n\n\r\n\r", count );
    send_to_char( buf, ch );
}

/*--------------------------------------------------------------------------
 * Action: Accept a quest
 *--------------------------------------------------------------------------*/

static void quest_do_accept( CHAR_DATA *ch, char *argument ) {
    char arg[MAX_INPUT_LENGTH];
    const QUEST_DEF *q;
    QUEST_PROGRESS *p;
    int qi;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Accept which quest? Use 'quest list' to see available quests.\n\r", ch );
        return;
    }

    qi = quest_def_index_by_id( arg );
    if ( qi < 0 ) {
        send_to_char( "No such quest.\n\r", ch );
        return;
    }

    q = quest_def_by_index( qi );
    if ( !quest_is_visible( ch, q ) ) {
        send_to_char( "That quest is not available to you.\n\r", ch );
        return;
    }

    p = quest_tracker_find( ch->pcdata->quest_tracker, qi );
    if ( !p || p->status != QSTATUS_AVAILABLE ) {
        if ( p && p->status == QSTATUS_ACTIVE )
            send_to_char( "You have already accepted that quest.\n\r", ch );
        else if ( p && p->status >= QSTATUS_TURNED_IN )
            send_to_char( "You have already completed that quest.\n\r", ch );
        else
            send_to_char( "That quest is not available to you yet.\n\r", ch );
        return;
    }

    p->status     = QSTATUS_ACTIVE;
    p->started_at = (int) current_time;

    snprintf( buf, sizeof( buf ),
        "\n\r  #tFFD700" U_STAR " Quest Accepted: #C%s#n\n\r\n\r  %s\n\r\n\r",
        q->name, q->description );
    send_to_char( buf, ch );

    /* Check if objectives are already met */
    quest_check_milestones( ch );
}

/*--------------------------------------------------------------------------
 * Action: Complete (turn in) a quest
 *--------------------------------------------------------------------------*/

static void quest_do_complete( CHAR_DATA *ch, char *argument ) {
    QUEST_TRACKER *tracker = ch->pcdata->quest_tracker;
    int i, completed = 0;

    for ( i = 0; i < tracker->count; i++ ) {
        QUEST_PROGRESS *p = &tracker->entries[i];
        const QUEST_DEF *q;

        if ( p->status != QSTATUS_COMPLETE )
            continue;

        q = quest_def_by_index( p->quest_index );
        if ( !q ) continue;

        /* Auto-complete quests shouldn't reach here, but handle anyway */
        p->status       = QSTATUS_TURNED_IN;
        p->completed_at = (int) current_time;

        {
            char buf[MAX_STRING_LENGTH];
            snprintf( buf, sizeof( buf ),
                "\n\r  #tFFD700" U_STAR " Quest Complete: #x035%s #tFFD700" U_STAR "#n\n\r", q->name );
            send_to_char( buf, ch );
        }
        quest_award_rewards( ch, q );
        completed++;
    }

    if ( completed == 0 )
        send_to_char( "You have no quests ready to turn in.\n\r", ch );
    else
        quest_evaluate_availability( ch );
}

/*--------------------------------------------------------------------------
 * Action: Abandon a quest
 *--------------------------------------------------------------------------*/

static void quest_do_abandon( CHAR_DATA *ch, char *argument ) {
    char arg[MAX_INPUT_LENGTH];
    const QUEST_DEF *q;
    QUEST_PROGRESS *p;
    int qi, j;

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Abandon which quest?\n\r", ch );
        return;
    }

    qi = quest_def_index_by_id( arg );
    if ( qi < 0 ) {
        send_to_char( "No such quest.\n\r", ch );
        return;
    }

    q = quest_def_by_index( qi );
    p = quest_tracker_find( ch->pcdata->quest_tracker, qi );

    if ( !p || ( p->status != QSTATUS_ACTIVE && p->status != QSTATUS_COMPLETE ) ) {
        send_to_char( "You are not currently on that quest.\n\r", ch );
        return;
    }

    /* Can't abandon main quests */
    if ( !strcmp( q->category, "M" ) ) {
        send_to_char( "You cannot abandon main progression quests.\n\r", ch );
        return;
    }

    /* Reset progress */
    p->status     = QSTATUS_AVAILABLE;
    p->started_at = 0;
    for ( j = 0; j < MAX_QUEST_OBJECTIVES; j++ )
        p->obj_progress[j].current = 0;

    {
        char buf[MAX_STRING_LENGTH];
        snprintf( buf, sizeof( buf ),
            "Quest abandoned: %s\n\r", q->name );
        send_to_char( buf, ch );
    }
}

/*--------------------------------------------------------------------------
 * Admin: do_qadmin - Immortal quest administration
 *--------------------------------------------------------------------------*/

static const char *qstatus_name( int status ) {
    switch ( status ) {
        case QSTATUS_LOCKED:    return "LOCKED";
        case QSTATUS_AVAILABLE: return "AVAILABLE";
        case QSTATUS_ACTIVE:    return "ACTIVE";
        case QSTATUS_COMPLETE:  return "COMPLETE";
        case QSTATUS_TURNED_IN: return "TURNED_IN";
        default:                return "UNKNOWN";
    }
}

static void qadmin_list_defs( CHAR_DATA *ch, char *argument ) {
    char buf[MAX_STRING_LENGTH];
    char cat_filter[MAX_INPUT_LENGTH];
    int i, count = 0;

    one_argument( argument, cat_filter );

    send_to_char( "\n\r #tFFD700" U_STAR " Quest Definitions " U_STAR "#n\n\r\n\r", ch );
    send_to_char( "  #CID             Cat  Tier  Flags  Name#n\n\r", ch );
    send_to_char( "  #x243----------------------------------------------#n\n\r", ch );

    for ( i = 0; i < quest_def_count(); i++ ) {
        const QUEST_DEF *q = quest_def_by_index( i );
        if ( !q ) continue;

        if ( cat_filter[0] != '\0' && str_cmp( cat_filter, q->category ) )
            continue;

        snprintf( buf, sizeof( buf ),
            "  %-14s  %-3s  %d     %c%c%c   %s\n\r",
            q->id, q->category, q->tier,
            IS_SET( q->flags, QFLAG_AUTO_COMPLETE ) ? 'A' : '-',
            IS_SET( q->flags, QFLAG_REPEATABLE )    ? 'R' : '-',
            IS_SET( q->flags, QFLAG_FTUE_SKIP )     ? 'F' : '-',
            q->name );
        send_to_char( buf, ch );
        count++;
    }

    snprintf( buf, sizeof( buf ), "\n\r  %d definitions. Flags: A=auto R=repeat F=ftue_skip\n\r\n\r", count );
    send_to_char( buf, ch );
}

static void qadmin_info( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    QUEST_TRACKER *tracker = victim->pcdata->quest_tracker;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int i, j;

    one_argument( argument, arg );

    /* Specific quest detail */
    if ( arg[0] != '\0' ) {
        int qi = quest_def_index_by_id( arg );
        const QUEST_DEF *q;
        QUEST_PROGRESS *p;

        if ( qi < 0 ) {
            send_to_char( "No such quest ID.\n\r", ch );
            return;
        }

        q = quest_def_by_index( qi );
        p = quest_tracker_find( tracker, qi );

        snprintf( buf, sizeof( buf ),
            "\n\r #tFFD700" U_STAR " Quest: #C%s#n - %s\n\r\n\r"
            "  Category:  %s%-2s#n    Tier: %d\n\r"
            "  Status:    #C%s#n (%d)\n\r"
            "  Flags:     %c%c%c    Class: %d    FTUE: %d-%d\n\r"
            "  Rewards:   QP=%d  Exp=%d  Primal=%d  Item=%d\n\r\n\r",
            q->id, q->name,
            quest_cat_color( q->category ), q->category, q->tier,
            p ? qstatus_name( p->status ) : "UNTRACKED", p ? p->status : -1,
            IS_SET( q->flags, QFLAG_AUTO_COMPLETE ) ? 'A' : '-',
            IS_SET( q->flags, QFLAG_REPEATABLE )    ? 'R' : '-',
            IS_SET( q->flags, QFLAG_FTUE_SKIP )     ? 'F' : '-',
            q->required_class, q->min_explevel, q->max_explevel,
            q->qp_reward, q->exp_reward, q->primal_reward, q->item_reward_vnum );
        send_to_char( buf, ch );

        send_to_char( "  Objectives:\n\r", ch );
        for ( j = 0; j < q->obj_count; j++ ) {
            int cur = ( p ) ? p->obj_progress[j].current : 0;
            snprintf( buf, sizeof( buf ),
                "    [%d] %-16s %-16s %d/%d  %s\n\r",
                j, q->objectives[j].type, q->objectives[j].target,
                cur, q->objectives[j].threshold,
                q->objectives[j].description );
            send_to_char( buf, ch );
        }

        if ( q->prereq_count > 0 ) {
            send_to_char( "\n\r  Prerequisites: ", ch );
            for ( j = 0; j < q->prereq_count; j++ ) {
                const QUEST_DEF *pq = quest_def_by_index( q->prereq_indices[j] );
                if ( pq ) {
                    snprintf( buf, sizeof( buf ), "%s%s",
                        j > 0 ? ", " : "", pq->id );
                    send_to_char( buf, ch );
                }
            }
            send_to_char( "\n\r", ch );
        }
        send_to_char( "\n\r", ch );
        return;
    }

    /* Overview: all tracked quests for this player */
    snprintf( buf, sizeof( buf ),
        "\n\r #tFFD700" U_STAR " Quest State for %s " U_STAR "#n\n\r\n\r",
        victim->name );
    send_to_char( buf, ch );
    send_to_char( "  #CID             Status      Obj Progress#n\n\r", ch );
    send_to_char( "  #x243----------------------------------------------#n\n\r", ch );

    for ( i = 0; i < tracker->count; i++ ) {
        QUEST_PROGRESS *p = &tracker->entries[i];
        const QUEST_DEF *q = quest_def_by_index( p->quest_index );
        if ( !q ) continue;

        /* Only show non-locked entries by default */
        if ( p->status == QSTATUS_LOCKED )
            continue;

        snprintf( buf, sizeof( buf ), "  %-14s  %-10s",
            q->id, qstatus_name( p->status ) );
        send_to_char( buf, ch );

        for ( j = 0; j < q->obj_count; j++ ) {
            snprintf( buf, sizeof( buf ), "  %d/%d",
                p->obj_progress[j].current,
                q->objectives[j].threshold );
            send_to_char( buf, ch );
        }
        send_to_char( "\n\r", ch );
    }
    send_to_char( "\n\r", ch );
}

static void qadmin_set( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg_id[MAX_INPUT_LENGTH];
    char arg_val[MAX_INPUT_LENGTH];
    QUEST_PROGRESS *p;
    int qi, status;
    char buf[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg_id );
    one_argument( argument, arg_val );

    if ( arg_id[0] == '\0' || arg_val[0] == '\0' ) {
        send_to_char( "Usage: qadmin <player> set <quest_id> <status 0-4>\n\r", ch );
        send_to_char( "  0=LOCKED 1=AVAILABLE 2=ACTIVE 3=COMPLETE 4=TURNED_IN\n\r", ch );
        return;
    }

    qi = quest_def_index_by_id( arg_id );
    if ( qi < 0 ) {
        send_to_char( "No such quest ID.\n\r", ch );
        return;
    }

    status = atoi( arg_val );
    if ( status < 0 || status > 4 ) {
        send_to_char( "Status must be 0-4.\n\r", ch );
        return;
    }

    p = quest_tracker_get( victim->pcdata->quest_tracker, qi );
    if ( !p ) {
        send_to_char( "Failed to get tracker entry.\n\r", ch );
        return;
    }

    p->status = status;
    if ( status == QSTATUS_ACTIVE && p->started_at == 0 )
        p->started_at = (int) current_time;
    if ( status >= QSTATUS_TURNED_IN && p->completed_at == 0 )
        p->completed_at = (int) current_time;

    snprintf( buf, sizeof( buf ), "Set %s quest %s to %s.\n\r",
        victim->name, arg_id, qstatus_name( status ) );
    send_to_char( buf, ch );
}

static void qadmin_complete( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg_id[MAX_INPUT_LENGTH];
    const QUEST_DEF *q;
    QUEST_PROGRESS *p;
    int qi;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg_id );
    if ( arg_id[0] == '\0' ) {
        send_to_char( "Usage: qadmin <player> complete <quest_id>\n\r", ch );
        return;
    }

    qi = quest_def_index_by_id( arg_id );
    if ( qi < 0 ) {
        send_to_char( "No such quest ID.\n\r", ch );
        return;
    }

    q = quest_def_by_index( qi );
    p = quest_tracker_get( victim->pcdata->quest_tracker, qi );
    if ( !p ) {
        send_to_char( "Failed to get tracker entry.\n\r", ch );
        return;
    }

    /* Max out all objectives */
    {
        int j;
        for ( j = 0; j < q->obj_count; j++ )
            p->obj_progress[j].current = q->objectives[j].threshold;
    }

    p->status       = QSTATUS_TURNED_IN;
    p->started_at   = ( p->started_at == 0 ) ? (int) current_time : p->started_at;
    p->completed_at = (int) current_time;

    snprintf( buf, sizeof( buf ),
        "Completed %s quest %s (%s) with rewards.\n\r",
        victim->name, q->id, q->name );
    send_to_char( buf, ch );

    /* Award rewards to the victim */
    quest_award_rewards( victim, q );

    /* Re-evaluate availability for cascading unlocks */
    quest_evaluate_availability( victim );
}

static void qadmin_reset( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    QUEST_TRACKER *tracker = victim->pcdata->quest_tracker;
    char arg_id[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int i, j;

    one_argument( argument, arg_id );

    if ( arg_id[0] == '\0' ) {
        send_to_char( "Usage: qadmin <player> reset <quest_id|all>\n\r", ch );
        return;
    }

    if ( !str_cmp( arg_id, "all" ) ) {
        for ( i = 0; i < tracker->count; i++ ) {
            tracker->entries[i].status       = QSTATUS_LOCKED;
            tracker->entries[i].started_at   = 0;
            tracker->entries[i].completed_at = 0;
            for ( j = 0; j < MAX_QUEST_OBJECTIVES; j++ )
                tracker->entries[i].obj_progress[j].current = 0;
        }
        snprintf( buf, sizeof( buf ), "Reset ALL quest progress for %s.\n\r", victim->name );
        send_to_char( buf, ch );

        /* Re-evaluate from scratch */
        quest_evaluate_availability( victim );
        return;
    }

    {
        int qi = quest_def_index_by_id( arg_id );
        QUEST_PROGRESS *p;

        if ( qi < 0 ) {
            send_to_char( "No such quest ID.\n\r", ch );
            return;
        }

        p = quest_tracker_find( tracker, qi );
        if ( !p ) {
            send_to_char( "Player has no progress on that quest.\n\r", ch );
            return;
        }

        p->status       = QSTATUS_LOCKED;
        p->started_at   = 0;
        p->completed_at = 0;
        for ( j = 0; j < MAX_QUEST_OBJECTIVES; j++ )
            p->obj_progress[j].current = 0;

        snprintf( buf, sizeof( buf ), "Reset %s quest %s to LOCKED.\n\r",
            victim->name, arg_id );
        send_to_char( buf, ch );
        quest_evaluate_availability( victim );
    }
}

static void qadmin_obj( CHAR_DATA *ch, CHAR_DATA *victim, char *argument ) {
    char arg_id[MAX_INPUT_LENGTH];
    char arg_idx[MAX_INPUT_LENGTH];
    char arg_val[MAX_INPUT_LENGTH];
    const QUEST_DEF *q;
    QUEST_PROGRESS *p;
    int qi, obj_idx, val;
    char buf[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg_id );
    argument = one_argument( argument, arg_idx );
    one_argument( argument, arg_val );

    if ( arg_id[0] == '\0' || arg_idx[0] == '\0' || arg_val[0] == '\0' ) {
        send_to_char( "Usage: qadmin <player> obj <quest_id> <obj_index> <value>\n\r", ch );
        return;
    }

    qi = quest_def_index_by_id( arg_id );
    if ( qi < 0 ) {
        send_to_char( "No such quest ID.\n\r", ch );
        return;
    }

    q = quest_def_by_index( qi );
    obj_idx = atoi( arg_idx );
    if ( obj_idx < 0 || obj_idx >= q->obj_count ) {
        snprintf( buf, sizeof( buf ), "Objective index must be 0-%d.\n\r", q->obj_count - 1 );
        send_to_char( buf, ch );
        return;
    }

    val = atoi( arg_val );

    p = quest_tracker_get( victim->pcdata->quest_tracker, qi );
    if ( !p ) {
        send_to_char( "Failed to get tracker entry.\n\r", ch );
        return;
    }

    p->obj_progress[obj_idx].current = val;

    snprintf( buf, sizeof( buf ),
        "Set %s quest %s objective [%d] to %d/%d.\n\r",
        victim->name, arg_id, obj_idx, val,
        q->objectives[obj_idx].threshold );
    send_to_char( buf, ch );
}

void do_qadmin( CHAR_DATA *ch, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "\n\r #tFFD700" U_STAR " Quest Administration " U_STAR "#n\n\r\n\r", ch );
        send_to_char( "  #Cqadmin list#n [cat]                  List quest definitions\n\r", ch );
        send_to_char( "  #Cqadmin#n <player> #Cinfo#n [quest_id]    Show quest state\n\r", ch );
        send_to_char( "  #Cqadmin#n <player> #Cset#n <id> <0-4>     Set quest status\n\r", ch );
        send_to_char( "  #Cqadmin#n <player> #Ccomplete#n <id>      Complete + award quest\n\r", ch );
        send_to_char( "  #Cqadmin#n <player> #Creset#n <id|all>     Reset quest(s)\n\r", ch );
        send_to_char( "  #Cqadmin#n <player> #Cobj#n <id> <n> <val> Set objective progress\n\r", ch );
        send_to_char( "\n\r  Status: 0=LOCKED 1=AVAILABLE 2=ACTIVE 3=COMPLETE 4=TURNED_IN\n\r\n\r", ch );
        return;
    }

    /* "qadmin list" shows definitions without needing a player */
    if ( !str_cmp( arg1, "list" ) ) {
        qadmin_list_defs( ch, argument );
        return;
    }

    /* All other subcommands need a player target */
    victim = get_char_world( ch, arg1 );
    if ( !victim ) {
        send_to_char( "Player not found.\n\r", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPCs.\n\r", ch );
        return;
    }
    if ( !victim->pcdata->quest_tracker ) {
        send_to_char( "That player's quest tracker is not initialized.\n\r", ch );
        return;
    }

    argument = one_argument( argument, arg2 );

    if ( arg2[0] == '\0' || !str_cmp( arg2, "info" ) ) {
        qadmin_info( ch, victim, argument );
        return;
    }
    if ( !str_cmp( arg2, "set" ) )      { qadmin_set( ch, victim, argument );      return; }
    if ( !str_cmp( arg2, "complete" ) ) { qadmin_complete( ch, victim, argument ); return; }
    if ( !str_cmp( arg2, "reset" ) )    { qadmin_reset( ch, victim, argument );    return; }
    if ( !str_cmp( arg2, "obj" ) )      { qadmin_obj( ch, victim, argument );      return; }

    send_to_char( "Unknown qadmin subcommand. Type 'qadmin' for help.\n\r", ch );
}
