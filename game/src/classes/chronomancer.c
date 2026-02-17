/***************************************************************************
 *  Chronomancer class - temporal mages who manipulate the flow of time    *
 *  Uses "Temporal Flux" resource (ch->rage) with balance mechanic.        *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "cfg.h"
#include "chronomancer.h"
#include "../db/db_class.h"

/* Forward declarations for class armor */
extern void do_classarmor_generic( CHAR_DATA *ch, char *argument, int class_id );

/* =========================================================================
 * FLUX SCALING HELPER
 *
 * Returns a percentage modifier (40-140) based on flux position.
 * Acceleration abilities are stronger at low flux (slow state).
 * Deceleration abilities are stronger at high flux (fast state).
 * ========================================================================= */

int get_chrono_power_mod( CHAR_DATA *ch, bool is_accel )
{
	int flux = ch->rage;

	if ( IS_CLASS( ch, CLASS_PARADOX ) )
	{
		if ( is_accel )
		{
			if ( flux <= PARA_ANCHOR_MAX )  return 140;  /* +40% */
			if ( flux <= PARA_SLOW_MAX )    return 120;  /* +20% */
			if ( flux <= PARA_BALANCED_MAX ) return 100;  /* normal */
			if ( flux <= PARA_FAST_MAX )    return 70;   /* -30% */
			return 40;                                     /* -60% */
		}
		else
		{
			if ( flux <= PARA_ANCHOR_MAX )  return 40;   /* -60% */
			if ( flux <= PARA_SLOW_MAX )    return 70;   /* -30% */
			if ( flux <= PARA_BALANCED_MAX ) return 100;  /* normal */
			if ( flux <= PARA_FAST_MAX )    return 120;  /* +20% */
			return 140;                                    /* +40% */
		}
	}
	else /* Chronomancer */
	{
		if ( is_accel )
		{
			if ( flux <= CHRONO_DEEP_SLOW_MAX ) return 130;  /* +30% */
			if ( flux <= CHRONO_SLOW_MAX )      return 115;  /* +15% */
			if ( flux <= CHRONO_BALANCED_MAX )   return 100;  /* normal */
			if ( flux <= CHRONO_FAST_MAX )       return 75;   /* -25% */
			return 50;                                         /* -50% */
		}
		else
		{
			if ( flux <= CHRONO_DEEP_SLOW_MAX ) return 50;   /* -50% */
			if ( flux <= CHRONO_SLOW_MAX )      return 75;   /* -25% */
			if ( flux <= CHRONO_BALANCED_MAX )   return 100;  /* normal */
			if ( flux <= CHRONO_FAST_MAX )       return 115;  /* +15% */
			return 130;                                        /* +30% */
		}
	}
}

/* Helper: get flux zone name for display */
static const char *get_flux_zone_name( CHAR_DATA *ch )
{
	int flux = ch->rage;

	if ( IS_CLASS( ch, CLASS_PARADOX ) )
	{
		if ( flux <= PARA_ANCHOR_MAX )  return "#CTemporal Anchor#n";
		if ( flux <= PARA_SLOW_MAX )    return "#cSlow#n";
		if ( flux <= PARA_BALANCED_MAX ) return "#GBalanced#n";
		if ( flux <= PARA_FAST_MAX )    return "#yFast#n";
		return "#RTemporal Storm#n";
	}
	else
	{
		if ( flux <= CHRONO_DEEP_SLOW_MAX ) return "#CDeep Slow#n";
		if ( flux <= CHRONO_SLOW_MAX )      return "#cSlow#n";
		if ( flux <= CHRONO_BALANCED_MAX )   return "#GBalanced#n";
		if ( flux <= CHRONO_FAST_MAX )       return "#yFast#n";
		return "#RDeep Fast#n";
	}
}

/* =========================================================================
 * FLUX DISPLAY - shared by Chronomancer and Paradox
 * ========================================================================= */

void do_flux( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int flux_cap;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) && !IS_CLASS( ch, CLASS_PARADOX ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	flux_cap = IS_CLASS( ch, CLASS_PARADOX ) ? PARA_FLUX_CAP : CHRONO_FLUX_CAP;

	/* Header */
	if ( IS_CLASS( ch, CLASS_PARADOX ) )
		snprintf( buf, sizeof(buf), "%s>(%s(#n Temporal Flux %s)%s)<#n\n\r", ac, pc, pc, ac );
	else
		snprintf( buf, sizeof(buf), "%s[>%s[#n Temporal Flux %s]%s<]#n\n\r", ac, pc, pc, ac );
	send_to_char( buf, ch );

	/* Current flux */
	snprintf( buf, sizeof(buf), "Flux: %s%d#n / %d\n\r", pc, ch->rage, flux_cap );
	send_to_char( buf, ch );

	/* Zone */
	snprintf( buf, sizeof(buf), "State: %s\n\r", get_flux_zone_name( ch ) );
	send_to_char( buf, ch );

	/* Power modifiers */
	snprintf( buf, sizeof(buf), "Acceleration power: %s%d%%#n    Deceleration power: %s%d%%#n\n\r",
		pc, get_chrono_power_mod( ch, TRUE ),
		pc, get_chrono_power_mod( ch, FALSE ) );
	send_to_char( buf, ch );

	/* Active effects - Chronomancer */
	if ( IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		if ( ch->pcdata->powers[CHRONO_QUICKEN_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nQuicken: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[CHRONO_QUICKEN_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CHRONO_BLUR_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nBlur: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[CHRONO_BLUR_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CHRONO_TIMESLIP_READY] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nTime Slip: #CReady#n%s]#n\n\r", pc, pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CHRONO_FORESIGHT_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nForesight: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[CHRONO_FORESIGHT_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CHRONO_HINDSIGHT_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nHindsight: #C%d#n ticks, #C%d#n stacks%s]#n\n\r",
				pc, ch->pcdata->powers[CHRONO_HINDSIGHT_TICKS],
				ch->pcdata->powers[CHRONO_HINDSIGHT_STACKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CHRONO_ECHO_PENDING] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nTemporal Echo: #C%d#n rounds%s]#n\n\r",
				pc, ch->pcdata->powers[CHRONO_ECHO_PENDING], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CHRONO_SLOW_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nSlow: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[CHRONO_SLOW_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CHRONO_TIMETRAP_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nTime Trap: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[CHRONO_TIMETRAP_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CHRONO_STASIS_TICKS] > 0 )
		{
			snprintf( buf, sizeof(buf), "%s[#nStasis: #C%d#n rounds, #C%d#n stored damage%s]#n\n\r",
				pc, ch->pcdata->powers[CHRONO_STASIS_TICKS],
				ch->pcdata->stats[CHRONO_STAT_STASIS_DMG], pc );
			send_to_char( buf, ch );
		}
	}

	return;
}

/* =========================================================================
 * CHRONOMANCER TRAINING
 * ========================================================================= */

void do_timetrain( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int cost;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	if ( argument[0] == '\0' )
	{
		snprintf( buf, sizeof(buf), "%s[>%s[#n Time Training %s]%s<]#n\n\r", ac, pc, pc, ac );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[CHRONO_TRAIN_ACCEL] + 1 ) * cfg( CFG_CHRONO_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nAcceleration:    Level %d/3  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[CHRONO_TRAIN_ACCEL], cost, pc );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[CHRONO_TRAIN_DECEL] + 1 ) * cfg( CFG_CHRONO_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nDeceleration:    Level %d/3  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[CHRONO_TRAIN_DECEL], cost, pc );
		send_to_char( buf, ch );

		cost = ( ch->pcdata->powers[CHRONO_TRAIN_SIGHT] + 1 ) * cfg( CFG_CHRONO_TRAIN_COST_MULT );
		snprintf( buf, sizeof(buf), "%s[#nTemporal Sight:  Level %d/3  (Next: %d primal)%s]#n\n\r",
			pc, ch->pcdata->powers[CHRONO_TRAIN_SIGHT], cost, pc );
		send_to_char( buf, ch );

		send_to_char( "\n\rSyntax: timetrain <acceleration|deceleration|sight>\n\r", ch );
		return;
	}

	if ( !str_prefix( argument, "acceleration" ) )
	{
		if ( ch->pcdata->powers[CHRONO_TRAIN_ACCEL] >= 3 )
		{
			send_to_char( "You have mastered Acceleration.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[CHRONO_TRAIN_ACCEL] + 1 ) * cfg( CFG_CHRONO_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Acceleration.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[CHRONO_TRAIN_ACCEL]++;
		snprintf( buf, sizeof(buf),
			"#x215Time accelerates around you... Acceleration is now level %d.#n\n\r",
			ch->pcdata->powers[CHRONO_TRAIN_ACCEL] );
		send_to_char( buf, ch );
		return;
	}

	if ( !str_prefix( argument, "deceleration" ) )
	{
		if ( ch->pcdata->powers[CHRONO_TRAIN_DECEL] >= 3 )
		{
			send_to_char( "You have mastered Deceleration.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[CHRONO_TRAIN_DECEL] + 1 ) * cfg( CFG_CHRONO_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Deceleration.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[CHRONO_TRAIN_DECEL]++;
		snprintf( buf, sizeof(buf),
			"#x215Time slows to a crawl... Deceleration is now level %d.#n\n\r",
			ch->pcdata->powers[CHRONO_TRAIN_DECEL] );
		send_to_char( buf, ch );
		return;
	}

	if ( !str_prefix( argument, "sight" ) )
	{
		if ( ch->pcdata->powers[CHRONO_TRAIN_SIGHT] >= 3 )
		{
			send_to_char( "You have mastered Temporal Sight.\n\r", ch );
			return;
		}
		cost = ( ch->pcdata->powers[CHRONO_TRAIN_SIGHT] + 1 ) * cfg( CFG_CHRONO_TRAIN_COST_MULT );
		if ( ch->practice < cost )
		{
			snprintf( buf, sizeof(buf), "You need %d primal to advance Temporal Sight.\n\r", cost );
			send_to_char( buf, ch );
			return;
		}
		ch->practice -= cost;
		ch->pcdata->powers[CHRONO_TRAIN_SIGHT]++;
		snprintf( buf, sizeof(buf),
			"#x215Your perception expands across timelines... Temporal Sight is now level %d.#n\n\r",
			ch->pcdata->powers[CHRONO_TRAIN_SIGHT] );
		send_to_char( buf, ch );
		return;
	}

	send_to_char( "Syntax: timetrain <acceleration|deceleration|sight>\n\r", ch );
	return;
}

/* =========================================================================
 * ACCELERATION ABILITIES
 * ========================================================================= */

/*
 * Quicken - Self haste buff (Acceleration 1)
 * Accelerates personal time, gaining extra attacks.
 * Flux scaling: at flux 0-39, extra attacks increase.
 */
void do_quicken( CHAR_DATA *ch, char *argument )
{
	int mana_cost;
	int flux_change;
	int duration;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_ACCEL] < 1 )
	{
		send_to_char( "You must train Acceleration to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_QUICKEN_TICKS] > 0 )
	{
		send_to_char( "You are already quickened.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_QUICKEN_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	flux_change = cfg( CFG_CHRONO_QUICKEN_FLUX_CHANGE );
	ch->rage = UMIN( ch->rage + flux_change, CHRONO_FLUX_CAP );

	duration = cfg( CFG_CHRONO_QUICKEN_DURATION );
	duration = duration * get_chrono_power_mod( ch, TRUE ) / 100;
	ch->pcdata->powers[CHRONO_QUICKEN_TICKS] = UMAX( 1, duration );

	send_to_char( "#x215Time accelerates around you - the world slows to a crawl.#n\n\r", ch );
	act( "$n blurs as time accelerates around them.", ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Time Slip - Evasive counter (Acceleration 2)
 * Slips forward in time to avoid the next attack and counter.
 */
void do_timeslip( CHAR_DATA *ch, char *argument )
{
	int mana_cost;
	int flux_change;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_ACCEL] < 2 )
	{
		send_to_char( "You must train Acceleration to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TIMESLIP_CD] > 0 )
	{
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof(buf), "Time Slip is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[CHRONO_TIMESLIP_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TIMESLIP_READY] > 0 )
	{
		send_to_char( "Time Slip is already primed.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_TIMESLIP_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	flux_change = cfg( CFG_CHRONO_TIMESLIP_FLUX_CHANGE );
	ch->rage = UMIN( ch->rage + flux_change, CHRONO_FLUX_CAP );

	ch->pcdata->powers[CHRONO_TIMESLIP_READY] = 1;
	ch->pcdata->powers[CHRONO_TIMESLIP_CD] = cfg( CFG_CHRONO_TIMESLIP_COOLDOWN );

	send_to_char( "#x215You prepare to slip through time - the next attack will find only an afterimage.#n\n\r", ch );
	act( "$n shimmers, their outline becoming momentarily indistinct.", ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Blur - Extreme speed burst (Acceleration 3)
 * Time accelerates to a blur, gaining massive combat bonuses.
 * Can only activate when flux < 60 (not already in fast state).
 */
void do_blur( CHAR_DATA *ch, char *argument )
{
	int mana_cost;
	int flux_change;
	int duration;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_ACCEL] < 3 )
	{
		send_to_char( "You must train Acceleration to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_BLUR_CD] > 0 )
	{
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof(buf), "Blur is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[CHRONO_BLUR_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_BLUR_TICKS] > 0 )
	{
		send_to_char( "You are already blurring through time.\n\r", ch );
		return;
	}

	if ( ch->rage >= CHRONO_BALANCED_MAX )
	{
		send_to_char( "Your temporal flux is too fast - you need to be in a slower state to blur.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_BLUR_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	flux_change = cfg( CFG_CHRONO_BLUR_FLUX_CHANGE );
	ch->rage = UMIN( ch->rage + flux_change, CHRONO_FLUX_CAP );

	duration = cfg( CFG_CHRONO_BLUR_DURATION );
	duration = duration * get_chrono_power_mod( ch, TRUE ) / 100;
	ch->pcdata->powers[CHRONO_BLUR_TICKS] = UMAX( 1, duration );
	ch->pcdata->powers[CHRONO_BLUR_CD] = cfg( CFG_CHRONO_BLUR_COOLDOWN );

	send_to_char( "#x215Time SURGES around you - you become a blur of impossible speed!#n\n\r", ch );
	act( "$n becomes a #x215blur#n of motion, barely visible to the naked eye!", ch, NULL, NULL, TO_ROOM );

	return;
}

/* =========================================================================
 * DECELERATION ABILITIES
 * ========================================================================= */

/*
 * Slow - Enemy debuff (Deceleration 1)
 * Slows the target's personal time, reducing their combat effectiveness.
 * Flux scaling: at flux 61-100, duration extends.
 */
void do_slow( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *victim;
	int mana_cost;
	int flux_change;
	int duration;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_DECEL] < 1 )
	{
		send_to_char( "You must train Deceleration to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL )
	{
		send_to_char( "You must be fighting to slow your enemy.\n\r", ch );
		return;
	}

	victim = ch->fighting;

	if ( ch->pcdata->powers[CHRONO_SLOW_TICKS] > 0 )
	{
		send_to_char( "Your enemy is already slowed.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_SLOW_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	flux_change = cfg( CFG_CHRONO_SLOW_FLUX_CHANGE );
	ch->rage = UMAX( 0, ch->rage - flux_change );

	/* Duration scales with flux - longer at high flux (decel bonus zone) */
	if ( ch->rage > CHRONO_BALANCED_MAX )
		duration = cfg( CFG_CHRONO_SLOW_DURATION_BONUS );
	else
		duration = cfg( CFG_CHRONO_SLOW_DURATION );

	duration = duration * get_chrono_power_mod( ch, FALSE ) / 100;
	ch->pcdata->powers[CHRONO_SLOW_TICKS] = UMAX( 1, duration );

	act( "#x215You weave temporal energy around $N, slowing their personal time.#n",
		ch, NULL, victim, TO_CHAR );
	act( "#x215$n gestures and time seems to thicken around $N.#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#x215Time grows thick around you - your movements become sluggish.#n",
		ch, NULL, victim, TO_VICT );

	return;
}

/*
 * Time Trap - Zone of slow time (Deceleration 2)
 * Creates an area where time moves slowly for enemies.
 */
void do_timetrap( CHAR_DATA *ch, char *argument )
{
	int mana_cost;
	int flux_change;
	int duration;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_DECEL] < 2 )
	{
		send_to_char( "You must train Deceleration to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL )
	{
		send_to_char( "You must be fighting to set a time trap.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TIMETRAP_CD] > 0 )
	{
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof(buf), "Time Trap is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[CHRONO_TIMETRAP_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TIMETRAP_TICKS] > 0 )
	{
		send_to_char( "You already have a time trap active.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_TIMETRAP_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	flux_change = cfg( CFG_CHRONO_TIMETRAP_FLUX_CHANGE );
	ch->rage = UMAX( 0, ch->rage - flux_change );

	duration = cfg( CFG_CHRONO_TIMETRAP_DURATION );
	duration = duration * get_chrono_power_mod( ch, FALSE ) / 100;
	ch->pcdata->powers[CHRONO_TIMETRAP_TICKS] = UMAX( 1, duration );
	ch->pcdata->powers[CHRONO_TIMETRAP_CD] = cfg( CFG_CHRONO_TIMETRAP_COOLDOWN );

	send_to_char( "#x215You anchor temporal energy to this location - time itself bends and slows.#n\n\r", ch );
	act( "#x215$n gestures and the air shimmers with #x130temporal distortion#x215.#n",
		ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Stasis - Complete time freeze (Deceleration 3)
 * Stops time for a single target completely.
 * Target can't act, takes no damage while frozen.
 * All damage is stored and applied when stasis ends.
 * Can only activate when flux > 40 (not already in deep slow state).
 */
void do_stasis( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *victim;
	int mana_cost;
	int flux_change;
	int duration;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_DECEL] < 3 )
	{
		send_to_char( "You must train Deceleration to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL )
	{
		send_to_char( "You must be fighting to freeze your enemy in stasis.\n\r", ch );
		return;
	}

	victim = ch->fighting;

	if ( ch->pcdata->powers[CHRONO_STASIS_CD] > 0 )
	{
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof(buf), "Stasis is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[CHRONO_STASIS_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_STASIS_TICKS] > 0 )
	{
		send_to_char( "Stasis is already active.\n\r", ch );
		return;
	}

	if ( ch->rage <= CHRONO_DEEP_SLOW_MAX )
	{
		send_to_char( "Your temporal flux is too slow - you need more temporal energy to freeze time.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_STASIS_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	flux_change = cfg( CFG_CHRONO_STASIS_FLUX_CHANGE );
	ch->rage = UMAX( 0, ch->rage - flux_change );

	duration = cfg( CFG_CHRONO_STASIS_DURATION );
	ch->pcdata->powers[CHRONO_STASIS_TICKS] = duration;
	ch->pcdata->powers[CHRONO_STASIS_CD] = cfg( CFG_CHRONO_STASIS_COOLDOWN );
	ch->pcdata->stats[CHRONO_STAT_STASIS_DMG] = 0;

	act( "#x215You freeze $N in a bubble of stopped time!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#x215$n gestures and $N freezes mid-motion, trapped in a bubble of crystallized time!#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#x215Time stops. You are frozen in place, unable to move or think.#n",
		ch, NULL, victim, TO_VICT );

	return;
}

/* =========================================================================
 * TEMPORAL SIGHT ABILITIES
 * ========================================================================= */

/*
 * Foresight - Combat prediction (Temporal Sight 1)
 * Glimpse the immediate future to anticipate attacks.
 */
void do_foresight( CHAR_DATA *ch, char *argument )
{
	int mana_cost;
	int duration;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_SIGHT] < 1 )
	{
		send_to_char( "You must train Temporal Sight to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_FORESIGHT_TICKS] > 0 )
	{
		send_to_char( "Your foresight is already active.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_FORESIGHT_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	duration = cfg( CFG_CHRONO_FORESIGHT_DURATION );
	ch->pcdata->powers[CHRONO_FORESIGHT_TICKS] = duration;

	send_to_char( "#x215Your perception stretches forward through time - you see echoes of what is to come.#n\n\r", ch );
	act( "$n's eyes glow with a faint #x215silver-blue#n light.", ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Hindsight - Learning from past attacks (Temporal Sight 2)
 * After being hit, gain stacking damage bonus against that attacker.
 * +5% per hit taken, max +25% (5 stacks). Stacks tracked in combat.
 */
void do_hindsight( CHAR_DATA *ch, char *argument )
{
	int mana_cost;
	int duration;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_SIGHT] < 2 )
	{
		send_to_char( "You must train Temporal Sight to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_HINDSIGHT_TICKS] > 0 )
	{
		send_to_char( "Hindsight is already active.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_HINDSIGHT_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	duration = cfg( CFG_CHRONO_HINDSIGHT_DURATION );
	ch->pcdata->powers[CHRONO_HINDSIGHT_TICKS] = duration;
	ch->pcdata->powers[CHRONO_HINDSIGHT_STACKS] = 0;

	send_to_char( "#x215Your mind opens to the patterns of combat - each blow will teach you more.#n\n\r", ch );
	act( "$n's movements become more deliberate, studying every attack.", ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Temporal Echo - Delayed attack (Temporal Sight 3)
 * Make an attack that echoes through time, hitting again moments later.
 * 2 rounds later, the echo hits for 75% of the original damage.
 */
void do_temporalecho( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *victim;
	int mana_cost;
	int dam;
	int power_mod;

	if ( !IS_CLASS( ch, CLASS_CHRONOMANCER ) )
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_TRAIN_SIGHT] < 3 )
	{
		send_to_char( "You must train Temporal Sight to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL )
	{
		send_to_char( "You must be fighting to create a temporal echo.\n\r", ch );
		return;
	}

	victim = ch->fighting;

	if ( ch->pcdata->powers[CHRONO_ECHO_CD] > 0 )
	{
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof(buf), "Temporal Echo is on cooldown for %d more ticks.\n\r",
			ch->pcdata->powers[CHRONO_ECHO_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->pcdata->powers[CHRONO_ECHO_PENDING] > 0 )
	{
		send_to_char( "A temporal echo is already reverberating.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CHRONO_ECHO_MANA_COST );
	if ( ch->mana < mana_cost )
	{
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;

	/* Calculate initial attack damage */
	power_mod = get_chrono_power_mod( ch, FALSE );  /* Sight is neutral, use decel mod */
	dam = number_range( 200, 400 ) + ( char_damroll( ch ) * 2 );
	dam = dam * power_mod / 100;

	/* Store echo for delayed hit */
	ch->pcdata->powers[CHRONO_ECHO_PENDING] = cfg( CFG_CHRONO_ECHO_DELAY );
	ch->pcdata->powers[CHRONO_ECHO_DAMAGE] = dam * cfg( CFG_CHRONO_ECHO_DAMAGE_PCT ) / 100;
	ch->pcdata->powers[CHRONO_ECHO_CD] = cfg( CFG_CHRONO_ECHO_COOLDOWN );

	act( "#x215You strike $N with a time-infused blow that echoes into the future!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#x215$n strikes $N with a blow that leaves a shimmering #x130temporal afterimage#x215.#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#x215$n strikes you with a strange blow - you sense it will echo through time.#n",
		ch, NULL, victim, TO_VICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );

	return;
}

/* =========================================================================
 * CHRONOMANCER TICK UPDATE
 * ========================================================================= */

void update_chronomancer( CHAR_DATA *ch )
{
	char buf[MAX_STRING_LENGTH];
	int center = CHRONO_FLUX_CENTER;

	/* Flux drift toward center (+/-2 per tick) */
	if ( ch->rage > center )
	{
		ch->rage = UMAX( center, ch->rage - 2 );
	}
	else if ( ch->rage < center )
	{
		ch->rage = UMIN( center, ch->rage + 2 );
	}

	/* Instability check at extremes */
	if ( ch->rage <= CHRONO_INSTABILITY_LOW || ch->rage >= CHRONO_INSTABILITY_HIGH )
	{
		if ( number_range( 1, 100 ) <= 10 )  /* 10% chance */
		{
			ch->pcdata->stats[CHRONO_STAT_INSTABILITY]++;

			switch ( number_range( 1, 4 ) )
			{
			case 1: /* Time Skip - stunned 1 round */
				send_to_char( "#x130Temporal instability! Time skips and you lose a moment.#n\n\r", ch );
				WAIT_STATE( ch, 12 );
				break;
			case 2: /* Echo - random ability triggers description */
				send_to_char( "#x130Temporal instability! An echo of a past action reverberates.#n\n\r", ch );
				break;
			case 3: /* Rewind - heal for a small amount */
				{
					int heal = ch->max_hit / 20;
					ch->hit = UMIN( ch->max_hit, ch->hit + heal );
					snprintf( buf, sizeof(buf),
						"#x130Temporal instability! Time rewinds slightly, restoring %d health.#n\n\r", heal );
					send_to_char( buf, ch );
				}
				break;
			case 4: /* Accelerate - brief speed boost message */
				send_to_char( "#x130Temporal instability! Time surges, granting a burst of speed.#n\n\r", ch );
				break;
			}
		}
	}

	/* Quicken countdown */
	if ( ch->pcdata->powers[CHRONO_QUICKEN_TICKS] > 0 )
	{
		ch->pcdata->powers[CHRONO_QUICKEN_TICKS]--;
		if ( ch->pcdata->powers[CHRONO_QUICKEN_TICKS] == 0 )
			send_to_char( "#x215The quickening fades - time resumes its normal pace.#n\n\r", ch );
	}

	/* Blur countdown */
	if ( ch->pcdata->powers[CHRONO_BLUR_TICKS] > 0 )
	{
		ch->pcdata->powers[CHRONO_BLUR_TICKS]--;
		if ( ch->pcdata->powers[CHRONO_BLUR_TICKS] == 0 )
			send_to_char( "#x215The blur of speed fades - you slow back to normal.#n\n\r", ch );
	}

	/* Foresight countdown */
	if ( ch->pcdata->powers[CHRONO_FORESIGHT_TICKS] > 0 )
	{
		ch->pcdata->powers[CHRONO_FORESIGHT_TICKS]--;
		if ( ch->pcdata->powers[CHRONO_FORESIGHT_TICKS] == 0 )
			send_to_char( "#x215Your foresight dims - the future becomes uncertain again.#n\n\r", ch );
	}

	/* Hindsight countdown */
	if ( ch->pcdata->powers[CHRONO_HINDSIGHT_TICKS] > 0 )
	{
		ch->pcdata->powers[CHRONO_HINDSIGHT_TICKS]--;
		if ( ch->pcdata->powers[CHRONO_HINDSIGHT_TICKS] == 0 )
		{
			ch->pcdata->powers[CHRONO_HINDSIGHT_STACKS] = 0;
			send_to_char( "#x215The lessons of hindsight fade from your mind.#n\n\r", ch );
		}
	}

	/* Temporal Echo trigger */
	if ( ch->pcdata->powers[CHRONO_ECHO_PENDING] > 0 )
	{
		ch->pcdata->powers[CHRONO_ECHO_PENDING]--;
		if ( ch->pcdata->powers[CHRONO_ECHO_PENDING] == 0
		     && ch->pcdata->powers[CHRONO_ECHO_DAMAGE] > 0 )
		{
			int echo_dam = ch->pcdata->powers[CHRONO_ECHO_DAMAGE];
			ch->pcdata->powers[CHRONO_ECHO_DAMAGE] = 0;

			if ( ch->fighting != NULL )
			{
				CHAR_DATA *victim = ch->fighting;
				act( "#x130A temporal echo reverberates - your past attack strikes $N again!#n",
					ch, NULL, victim, TO_CHAR );
				act( "#x130A shimmering echo of $n's earlier attack materializes and strikes $N!#n",
					ch, NULL, victim, TO_NOTVICT );
				act( "#x130A ghostly echo of a past blow strikes you from nowhere!#n",
					ch, NULL, victim, TO_VICT );
				damage( ch, victim, echo_dam, TYPE_UNDEFINED );
			}
			else
			{
				ch->pcdata->powers[CHRONO_ECHO_DAMAGE] = 0;
				send_to_char( "#x130The temporal echo fades without a target.#n\n\r", ch );
			}
		}
	}

	/* Slow countdown */
	if ( ch->pcdata->powers[CHRONO_SLOW_TICKS] > 0 )
	{
		ch->pcdata->powers[CHRONO_SLOW_TICKS]--;
		if ( ch->pcdata->powers[CHRONO_SLOW_TICKS] == 0 )
		{
			if ( ch->fighting != NULL )
			{
				act( "#x215The temporal slow on $N fades - they resume normal speed.#n",
					ch, NULL, ch->fighting, TO_CHAR );
				act( "#x215Time resumes its normal flow around you.#n",
					ch, NULL, ch->fighting, TO_VICT );
			}
		}
	}

	/* Time Trap countdown */
	if ( ch->pcdata->powers[CHRONO_TIMETRAP_TICKS] > 0 )
	{
		ch->pcdata->powers[CHRONO_TIMETRAP_TICKS]--;
		if ( ch->pcdata->powers[CHRONO_TIMETRAP_TICKS] == 0 )
			send_to_char( "#x215The time trap dissipates - the temporal distortion fades.#n\n\r", ch );
	}

	/* Stasis countdown and stored damage release */
	if ( ch->pcdata->powers[CHRONO_STASIS_TICKS] > 0 )
	{
		ch->pcdata->powers[CHRONO_STASIS_TICKS]--;
		if ( ch->pcdata->powers[CHRONO_STASIS_TICKS] == 0 )
		{
			int stored = ch->pcdata->stats[CHRONO_STAT_STASIS_DMG];
			ch->pcdata->stats[CHRONO_STAT_STASIS_DMG] = 0;

			if ( ch->fighting != NULL && stored > 0 )
			{
				CHAR_DATA *victim = ch->fighting;
				snprintf( buf, sizeof(buf),
					"#x215Stasis shatters! Time resumes and %d stored damage crashes into $N!#n",
					stored );
				act( buf, ch, NULL, victim, TO_CHAR );
				act( "#x215The stasis bubble shatters - all the stored damage hits $N at once!#n",
					ch, NULL, victim, TO_NOTVICT );
				snprintf( buf, sizeof(buf),
					"#x215Time resumes and %d damage hits you all at once!#n\n\r", stored );
				send_to_char( buf, victim );
				damage( ch, victim, stored, TYPE_UNDEFINED );
			}
			else
			{
				if ( ch->fighting != NULL )
				{
					act( "#x215Stasis shatters - $N resumes moving.#n",
						ch, NULL, ch->fighting, TO_CHAR );
					act( "#x215Time resumes around you.#n",
						ch, NULL, ch->fighting, TO_VICT );
				}
				else
				{
					send_to_char( "#x215The stasis effect fades.#n\n\r", ch );
				}
			}
		}
	}

	/* Cooldown decrements */
	if ( ch->pcdata->powers[CHRONO_TIMESLIP_CD] > 0 )
		ch->pcdata->powers[CHRONO_TIMESLIP_CD]--;
	if ( ch->pcdata->powers[CHRONO_BLUR_CD] > 0 )
		ch->pcdata->powers[CHRONO_BLUR_CD]--;
	if ( ch->pcdata->powers[CHRONO_TIMETRAP_CD] > 0 )
		ch->pcdata->powers[CHRONO_TIMETRAP_CD]--;
	if ( ch->pcdata->powers[CHRONO_STASIS_CD] > 0 )
		ch->pcdata->powers[CHRONO_STASIS_CD]--;
	if ( ch->pcdata->powers[CHRONO_ECHO_CD] > 0 )
		ch->pcdata->powers[CHRONO_ECHO_CD]--;
}

/* =========================================================================
 * CHRONOMANCER ARMOR
 * ========================================================================= */

void do_chronoarmor( CHAR_DATA *ch, char *argument )
{
	do_classarmor_generic( ch, argument, CLASS_CHRONOMANCER );
}
