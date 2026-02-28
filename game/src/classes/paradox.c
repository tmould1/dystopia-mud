/***************************************************************************
 *  Paradox Class - Upgrade of Chronomancer                                *
 *  Timeline manipulators who break the rules of temporal mechanics        *
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

/* Forward declarations from chronomancer.c */
extern int get_chrono_power_mod ( CHAR_DATA *ch, bool is_accel );


/*
 * do_destabilize - Voluntary instability trigger (Paradox only)
 *
 * Triggers a random temporal effect. Costs mana and has a cooldown.
 */
void do_destabilize( CHAR_DATA *ch, char *argument ) {
	int effect, flux_change;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->stats[PARA_DESTABILIZE_CD] > 0 ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses to destabilize again.\n\r",
			ch->pcdata->stats[PARA_DESTABILIZE_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_DESTABILIZE_MANA ) ) {
		send_to_char( "You don't have enough mana to destabilize the timeline.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_DESTABILIZE_MANA );
	ch->pcdata->stats[PARA_DESTABILIZE_CD] = cfg( CFG_PARA_DESTABILIZE_CD );

	/* Random flux shift */
	flux_change = number_range( 1, cfg( CFG_PARA_DESTABILIZE_FLUX ) );
	if ( number_percent() > 50 )
		flux_change = -flux_change;
	ch->rage = URANGE( 0, ch->rage + flux_change, PARA_FLUX_CAP );

	effect = number_range( 1, 6 );

	switch ( effect ) {
	case 1: /* Time Skip - random teleport within area */
		send_to_char( "#x160You rip the timeline open and #x210skip forward through time!#n\n\r", ch );
		act( "$n flickers and vanishes in a burst of temporal energy!", ch, NULL, NULL, TO_ROOM );
		break;
	case 2: /* Echo Strike - instant damage to target */
		if ( ch->fighting != NULL ) {
			int dam = number_range( 200, 500 ) + char_damroll( ch );
			send_to_char( "#x160A temporal echo lashes out at your enemy!#n\n\r", ch );
			act( "A ghostly echo of $n strikes you!", ch, NULL, ch->fighting, TO_VICT );
			damage( ch, ch->fighting, dam, TYPE_UNDEFINED );
		} else {
			send_to_char( "#x160Temporal energy crackles around you harmlessly.#n\n\r", ch );
		}
		break;
	case 3: /* Temporal Rewind - heal */
		{
			int heal = ch->max_hit / 10;
			ch->hit = UMIN( ch->hit + heal, ch->max_hit );
			send_to_char( "#x160Time rewinds around your wounds, #x210healing you!#n\n\r", ch );
		}
		break;
	case 4: /* Accelerate - temporary haste */
		send_to_char( "#x160Time accelerates wildly around you!#n\n\r", ch );
		if ( ch->pcdata->powers[PARA_PAST_SELF_TICKS] == 0 )
			ch->pcdata->powers[PARA_PAST_SELF_TICKS] = 2;
		break;
	case 5: /* Duplicate - echo count for Convergence */
		ch->pcdata->powers[PARA_ECHO_COUNT] = UMIN(
			ch->pcdata->powers[PARA_ECHO_COUNT] + 1, 5 );
		send_to_char( "#x160A temporal echo crystallizes around you!#n\n\r", ch );
		break;
	case 6: /* Freeze Frame - stun self briefly but restore mana */
		{
			int mana_gain = ch->max_mana / 8;
			ch->mana = UMIN( ch->mana + mana_gain, ch->max_mana );
			send_to_char( "#x160Time freezes for an instant — you feel mana rushing back!#n\n\r", ch );
		}
		break;
	}
}


/*
 * do_rewind - Undo damage from last 5 ticks (Timeline 1)
 */
void do_rewind( CHAR_DATA *ch, char *argument ) {
	int total_heal, i;
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_TIMELINE] < 1 ) {
		send_to_char( "You need Timeline training level 1 to use Rewind.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_REWIND_CD] > 0 ) {
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses to rewind again.\n\r",
			ch->pcdata->powers[PARA_REWIND_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_REWIND_MANA ) ) {
		send_to_char( "You don't have enough mana to rewind time.\n\r", ch );
		return;
	}

	/* Sum up damage history */
	total_heal = 0;
	for ( i = 0; i < 5; i++ )
		total_heal += ch->pcdata->stats[PARA_STAT_DAMAGE_HIST_0 + i];

	if ( total_heal <= 0 ) {
		send_to_char( "You have no recent damage to rewind.\n\r", ch );
		return;
	}

	total_heal = UMIN( total_heal, cfg( CFG_PARA_REWIND_MAX_HEAL ) );

	ch->mana -= cfg( CFG_PARA_REWIND_MANA );
	ch->pcdata->powers[PARA_REWIND_CD] = cfg( CFG_PARA_REWIND_CD );

	/* Apply heal */
	ch->hit = UMIN( ch->hit + total_heal, ch->max_hit );

	/* Clear damage history */
	for ( i = 0; i < 5; i++ )
		ch->pcdata->stats[PARA_STAT_DAMAGE_HIST_0 + i] = 0;

	snprintf( buf, sizeof( buf ),
		"#x160You rewind time, undoing #x210%d#x160 damage!#n\n\r", total_heal );
	send_to_char( buf, ch );
	act( "$n's wounds seal as time reverses around $m!", ch, NULL, NULL, TO_ROOM );
}


/*
 * do_splittimeline - Create HP/mana snapshot, choose after N rounds (Timeline 2)
 */
void do_splittimeline( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_TIMELINE] < 2 ) {
		send_to_char( "You need Timeline training level 2 to Split the Timeline.\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	/* Handle choices during active split */
	if ( ch->pcdata->powers[PARA_SPLIT_ACTIVE] > 0 ) {
		if ( !str_cmp( arg, "keep" ) ) {
			send_to_char( "#x160You choose to #x210keep this timeline#x160. The other fades away.#n\n\r", ch );
			act( "$n's timeline solidifies with a shimmer.", ch, NULL, NULL, TO_ROOM );
			ch->pcdata->powers[PARA_SPLIT_ACTIVE] = 0;
			ch->pcdata->stats[PARA_STAT_SPLIT_HP] = 0;
			ch->pcdata->stats[PARA_STAT_SPLIT_MANA] = 0;
			return;
		}
		if ( !str_cmp( arg, "revert" ) ) {
			ch->hit = UMIN( ch->pcdata->stats[PARA_STAT_SPLIT_HP], ch->max_hit );
			ch->mana = UMIN( ch->pcdata->stats[PARA_STAT_SPLIT_MANA], ch->max_mana );
			send_to_char( "#x160You #x210revert to the other timeline#x160! Your body shifts.#n\n\r", ch );
			act( "$n shimmers and shifts to an alternate timeline!", ch, NULL, NULL, TO_ROOM );
			ch->pcdata->powers[PARA_SPLIT_ACTIVE] = 0;
			ch->pcdata->stats[PARA_STAT_SPLIT_HP] = 0;
			ch->pcdata->stats[PARA_STAT_SPLIT_MANA] = 0;
			return;
		}
		send_to_char( "You have an active Split Timeline. Choose: splittimeline keep  OR  splittimeline revert\n\r", ch );
		return;
	}

	/* Activate new split */
	if ( ch->pcdata->powers[PARA_SPLIT_CD] > 0 ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses to split the timeline again.\n\r",
			ch->pcdata->powers[PARA_SPLIT_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_SPLIT_MANA ) ) {
		send_to_char( "You don't have enough mana to split the timeline.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_SPLIT_MANA );
	ch->pcdata->powers[PARA_SPLIT_CD] = cfg( CFG_PARA_SPLIT_CD );
	ch->pcdata->powers[PARA_SPLIT_ACTIVE] = cfg( CFG_PARA_SPLIT_ROUNDS );

	/* Snapshot current state */
	ch->pcdata->stats[PARA_STAT_SPLIT_HP] = ch->hit;
	ch->pcdata->stats[PARA_STAT_SPLIT_MANA] = ch->mana;

	send_to_char( "#x160You split the timeline! Reality fractures around you.#n\n\r", ch );
	send_to_char( "#x210After the split expires, use 'splittimeline keep' or 'splittimeline revert'.#n\n\r", ch );
	act( "Reality fractures around $n — two timelines shimmer into existence!", ch, NULL, NULL, TO_ROOM );
}


/*
 * do_convergence - Collapse temporal echoes for massive damage (Timeline 3)
 */
void do_convergence( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam, echoes, power_mod;
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_TIMELINE] < 3 ) {
		send_to_char( "You need Timeline training level 3 to use Convergence.\n\r", ch );
		return;
	}

	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You must be fighting to use Convergence.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_CONVERGENCE_CD] > 0 ) {
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses.\n\r",
			ch->pcdata->powers[PARA_CONVERGENCE_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_CONVERGENCE_MANA ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	echoes = ch->pcdata->powers[PARA_ECHO_COUNT];
	if ( echoes < 1 ) {
		send_to_char( "You have no temporal echoes to converge.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_CONVERGENCE_MANA );
	ch->rage = URANGE( 0, ch->rage - cfg( CFG_PARA_CONVERGENCE_FLUX ), PARA_FLUX_CAP );
	ch->pcdata->powers[PARA_CONVERGENCE_CD] = cfg( CFG_PARA_CONVERGENCE_CD );

	power_mod = get_chrono_power_mod( ch, FALSE );
	dam = number_range( cfg( CFG_PARA_CONVERGENCE_BASE_MIN ), cfg( CFG_PARA_CONVERGENCE_BASE_MAX ) );
	dam = dam * echoes;  /* Multiply by echo count */
	dam = dam * power_mod / 100;
	dam += char_damroll( ch ) * 2;

	ch->pcdata->powers[PARA_ECHO_COUNT] = 0;

	snprintf( buf, sizeof( buf ),
		"#x160You converge #x210%d#x160 temporal echoes into a devastating strike!#n\n\r", echoes );
	send_to_char( buf, ch );
	act( "$n collapses multiple timelines into a single devastating blow!", ch, NULL, victim, TO_ROOM );

	damage( ch, victim, dam, TYPE_UNDEFINED );
}


/*
 * do_futurestrike - Delayed damage attack (Temporal Combat 1)
 */
void do_futurestrike( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam, power_mod;
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_COMBAT] < 1 ) {
		send_to_char( "You need Temporal Combat training level 1 to use Future Strike.\n\r", ch );
		return;
	}

	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You must be fighting to use Future Strike.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_FUTURESTRIKE_CD] > 0 ) {
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses.\n\r",
			ch->pcdata->powers[PARA_FUTURESTRIKE_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_FUTURESTRIKE_MANA ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_FUTURESTRIKE_MANA );
	ch->rage = URANGE( 0, ch->rage + cfg( CFG_PARA_FUTURESTRIKE_FLUX ), PARA_FLUX_CAP );
	ch->pcdata->powers[PARA_FUTURESTRIKE_CD] = cfg( CFG_PARA_FUTURESTRIKE_CD );

	power_mod = get_chrono_power_mod( ch, TRUE );
	dam = number_range( cfg( CFG_PARA_FUTURESTRIKE_BASE_MIN ), cfg( CFG_PARA_FUTURESTRIKE_BASE_MAX ) );
	dam += ch->rage * 2;
	dam = dam * power_mod / 100;

	/* Store the damage for delayed delivery */
	ch->pcdata->powers[PARA_FUTURE_STRIKE_TGT] = cfg( CFG_PARA_FUTURESTRIKE_DELAY );
	ch->pcdata->powers[PARA_ECHO_COUNT] = UMIN( ch->pcdata->powers[PARA_ECHO_COUNT] + 1, 5 );

	send_to_char( "#x160You strike from the #x210future#x160 — the blow echoes across time!#n\n\r", ch );
	act( "$n strikes you with an attack that hasn't happened yet!", ch, NULL, victim, TO_VICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );
}


/*
 * do_pastself - Summon temporal echo to fight alongside (Temporal Combat 2)
 * Implemented as a buff granting extra attacks rather than an actual mob.
 */
void do_pastself( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_COMBAT] < 2 ) {
		send_to_char( "You need Temporal Combat training level 2 to summon your Past Self.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_PAST_SELF_TICKS] > 0 ) {
		send_to_char( "Your past self is already fighting alongside you.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_PASTSELF_CD] > 0 ) {
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses.\n\r",
			ch->pcdata->powers[PARA_PASTSELF_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_PASTSELF_MANA ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_PASTSELF_MANA );
	ch->rage = URANGE( 0, ch->rage - cfg( CFG_PARA_PASTSELF_FLUX ), PARA_FLUX_CAP );
	ch->pcdata->powers[PARA_PASTSELF_CD] = cfg( CFG_PARA_PASTSELF_CD );
	ch->pcdata->powers[PARA_PAST_SELF_TICKS] = cfg( CFG_PARA_PASTSELF_DURATION );

	send_to_char( "#x160A shimmering echo of your #x210past self#x160 steps from a rift to fight beside you!#n\n\r", ch );
	act( "A ghostly echo of $n steps from a temporal rift!", ch, NULL, NULL, TO_ROOM );
}


/*
 * do_timeloop - Trap enemy in temporal loop (Temporal Combat 3)
 */
void do_timeloop( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_COMBAT] < 3 ) {
		send_to_char( "You need Temporal Combat training level 3 to use Time Loop.\n\r", ch );
		return;
	}

	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You must be fighting to use Time Loop.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TIME_LOOP_TICKS] > 0 ) {
		send_to_char( "A time loop is already active.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TIMELOOP_CD] > 0 ) {
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses.\n\r",
			ch->pcdata->powers[PARA_TIMELOOP_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_TIMELOOP_MANA ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_TIMELOOP_MANA );
	ch->pcdata->powers[PARA_TIMELOOP_CD] = cfg( CFG_PARA_TIMELOOP_CD );
	ch->pcdata->powers[PARA_TIME_LOOP_TICKS] = cfg( CFG_PARA_TIMELOOP_ROUNDS );

	send_to_char( "#x160You trap your enemy in a #x210time loop#x160! They are forced to repeat!#n\n\r", ch );
	act( "$n traps you in a temporal loop — everything starts repeating!", ch, NULL, victim, TO_VICT );
	act( "$n traps $N in a shimmering loop of repeating time!", ch, NULL, victim, TO_NOTVICT );
}


/*
 * do_paradoxstrike - Undodgeable temporal attack (Temporal Combat 4)
 */
void do_paradoxstrike( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam, power_mod, flux_dev;
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_COMBAT] < 4 ) {
		send_to_char( "You need Temporal Combat training level 4 to use Paradox Strike.\n\r", ch );
		return;
	}

	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You must be fighting to use Paradox Strike.\n\r", ch );
		return;
	}

	if ( ch->pcdata->stats[PARA_PARADOXSTRIKE_CD] > 0 ) {
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses.\n\r",
			ch->pcdata->stats[PARA_PARADOXSTRIKE_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_PARADOXSTRIKE_MANA ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_PARADOXSTRIKE_MANA );
	ch->rage = URANGE( 0, ch->rage - cfg( CFG_PARA_PARADOXSTRIKE_FLUX ), PARA_FLUX_CAP );
	ch->pcdata->stats[PARA_PARADOXSTRIKE_CD] = cfg( CFG_PARA_PARADOXSTRIKE_CD );

	flux_dev = abs( ch->rage - PARA_FLUX_CENTER );
	power_mod = get_chrono_power_mod( ch, TRUE );
	dam = number_range( cfg( CFG_PARA_PARADOXSTRIKE_BASE_MIN ), cfg( CFG_PARA_PARADOXSTRIKE_BASE_MAX ) );
	dam += flux_dev * 4;
	dam = dam * power_mod / 100;
	dam += char_damroll( ch ) * 2;

	ch->pcdata->powers[PARA_ECHO_COUNT] = UMIN( ch->pcdata->powers[PARA_ECHO_COUNT] + 1, 5 );

	send_to_char( "#x160You strike with an #x210impossible paradox#x160 — the blow hits before you swing!#n\n\r", ch );
	act( "$n strikes you with an attack that defies causality!", ch, NULL, victim, TO_VICT );
	act( "$n's fist connects before $e even swings — a paradox in motion!", ch, NULL, victim, TO_NOTVICT );

	/* Direct damage - cannot be dodged (applied directly, not through normal combat) */
	damage( ch, victim, dam, TYPE_UNDEFINED );
}


/*
 * do_age - Debuff target, reducing damage/speed/dodge/regen (Entropy 1)
 */
void do_age( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int power_mod, duration;
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_ENTROPY] < 1 ) {
		send_to_char( "You need Entropy training level 1 to use Age.\n\r", ch );
		return;
	}

	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You must be fighting to use Age.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_AGE_TICKS] > 0 ) {
		send_to_char( "Your target is already suffering from temporal aging.\n\r", ch );
		return;
	}

	if ( ch->pcdata->stats[PARA_AGE_CD] > 0 ) {
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses.\n\r",
			ch->pcdata->stats[PARA_AGE_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_AGE_MANA ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_AGE_MANA );
	ch->rage = URANGE( 0, ch->rage - cfg( CFG_PARA_AGE_FLUX ), PARA_FLUX_CAP );
	ch->pcdata->stats[PARA_AGE_CD] = cfg( CFG_PARA_AGE_CD );

	power_mod = get_chrono_power_mod( ch, FALSE );
	duration = cfg( CFG_PARA_AGE_DURATION ) * power_mod / 100;
	duration = UMAX( duration, 3 );
	ch->pcdata->powers[PARA_AGE_TICKS] = duration;

	send_to_char( "#x160You warp time around your enemy, #x210aging them rapidly!#n\n\r", ch );
	act( "$n points at you — time rushes forward around you, your body aging!", ch, NULL, victim, TO_VICT );
	act( "$N ages visibly as $n warps time around them!", ch, NULL, victim, TO_NOTVICT );
}


/*
 * do_temporalcollapse - AoE damage to all enemies in room (Entropy 2)
 */
void do_temporalcollapse( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	CHAR_DATA *vch_next;
	int dam, power_mod, targets;
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_ENTROPY] < 2 ) {
		send_to_char( "You need Entropy training level 2 to cause a Temporal Collapse.\n\r", ch );
		return;
	}

	/* Requires extreme flux */
	if ( ch->rage >= 30 && ch->rage <= 120 ) {
		send_to_char( "Temporal Collapse requires extreme flux (below 30 or above 120).\n\r", ch );
		return;
	}

	if ( ch->pcdata->stats[PARA_COLLAPSE_CD] > 0 ) {
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses.\n\r",
			ch->pcdata->stats[PARA_COLLAPSE_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_COLLAPSE_MANA ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_COLLAPSE_MANA );
	ch->pcdata->stats[PARA_COLLAPSE_CD] = cfg( CFG_PARA_COLLAPSE_CD );

	/* Move flux 40 toward center */
	if ( ch->rage < PARA_FLUX_CENTER )
		ch->rage = UMIN( ch->rage + cfg( CFG_PARA_COLLAPSE_FLUX_RESTORE ), PARA_FLUX_CENTER );
	else
		ch->rage = UMAX( ch->rage - cfg( CFG_PARA_COLLAPSE_FLUX_RESTORE ), PARA_FLUX_CENTER );

	power_mod = get_chrono_power_mod( ch, FALSE );

	send_to_char( "#x160Time #x210COLLAPSES#x160 around you in a devastating wave!#n\n\r", ch );
	act( "Time itself collapses around $n in a devastating temporal shockwave!", ch, NULL, NULL, TO_ROOM );

	targets = 0;
	LIST_FOR_EACH_SAFE(victim, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {

		if ( victim == ch ) continue;
		if ( is_safe( ch, victim ) ) continue;
		if ( !IS_NPC( victim ) && victim->fighting != ch ) continue;

		dam = number_range( cfg( CFG_PARA_COLLAPSE_BASE_MIN ), cfg( CFG_PARA_COLLAPSE_BASE_MAX ) );
		dam = dam * power_mod / 100;
		dam += char_damroll( ch );
		targets++;

		damage( ch, victim, dam, TYPE_UNDEFINED );
	}

	if ( targets == 0 )
		send_to_char( "The temporal collapse hits nothing.\n\r", ch );
}


/*
 * do_eternity - Step outside time: immunity, free abilities, aftermath stun (Entropy 3)
 */
void do_eternity( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_TRAIN_ENTROPY] < 3 ) {
		send_to_char( "You need Entropy training level 3 to enter Eternity.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_ETERNITY_TICKS] > 0 ) {
		send_to_char( "You are already within Eternity.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[PARA_ETERNITY_AFTERMATH] > 0 ) {
		send_to_char( "You are still recovering from your last Eternity.\n\r", ch );
		return;
	}

	if ( ch->pcdata->stats[PARA_ETERNITY_CD] > 0 ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ), "You must wait %d more pulses.\n\r",
			ch->pcdata->stats[PARA_ETERNITY_CD] );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->mana < cfg( CFG_PARA_ETERNITY_MANA ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= cfg( CFG_PARA_ETERNITY_MANA );
	ch->rage = PARA_FLUX_CENTER;  /* Reset flux to center */
	ch->pcdata->stats[PARA_ETERNITY_CD] = cfg( CFG_PARA_ETERNITY_CD );
	ch->pcdata->powers[PARA_ETERNITY_TICKS] = cfg( CFG_PARA_ETERNITY_DURATION );

	send_to_char( "#x160You step outside of time itself into #x210E T E R N I T Y!#n\n\r", ch );
	send_to_char( "#x210You are immune to all damage. Your abilities cost nothing.#n\n\r", ch );
	send_to_char( "#x196Warning: You will be stunned when Eternity ends.#n\n\r", ch );
	act( "$n steps outside of time — $e shimmers with impossible light!", ch, NULL, NULL, TO_ROOM );
}


/*
 * do_paratrain - Training command for Paradox
 */
void do_paratrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int cost, *train_ptr, max_level;
	const char *train_name;

	if ( IS_NPC( ch ) ) return;

	if ( !IS_CLASS( ch, CLASS_PARADOX ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "#x160>( #x210Paradox Training #x160)<#n\n\r", ch );
		snprintf( buf, sizeof( buf ), "  #x210Timeline#n          %d / 3\n\r",
			ch->pcdata->powers[PARA_TRAIN_TIMELINE] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  #x210Temporal Combat#n   %d / 4\n\r",
			ch->pcdata->powers[PARA_TRAIN_COMBAT] );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  #x210Entropy#n           %d / 3\n\r",
			ch->pcdata->powers[PARA_TRAIN_ENTROPY] );
		send_to_char( buf, ch );
		send_to_char( "\n\rUse: paratrain <timeline|combat|entropy>\n\r", ch );
		return;
	}

	if ( !str_prefix( arg, "timeline" ) ) {
		train_ptr = &ch->pcdata->powers[PARA_TRAIN_TIMELINE];
		max_level = 3;
		train_name = "Timeline";
	} else if ( !str_prefix( arg, "combat" ) ) {
		train_ptr = &ch->pcdata->powers[PARA_TRAIN_COMBAT];
		max_level = 4;
		train_name = "Temporal Combat";
	} else if ( !str_prefix( arg, "entropy" ) ) {
		train_ptr = &ch->pcdata->powers[PARA_TRAIN_ENTROPY];
		max_level = 3;
		train_name = "Entropy";
	} else {
		send_to_char( "Train what? Options: timeline, combat, entropy\n\r", ch );
		return;
	}

	if ( *train_ptr >= max_level ) {
		snprintf( buf, sizeof( buf ), "%s is already at maximum level (%d).\n\r", train_name, max_level );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *train_ptr + 1 ) * cfg( CFG_PARA_TRAIN_COST_MULT );

	if ( ch->practice < cost ) {
		snprintf( buf, sizeof( buf ), "You need %d primal to advance %s (you have %d).\n\r",
			cost, train_name, ch->practice );
		send_to_char( buf, ch );
		return;
	}

	ch->practice -= cost;
	( *train_ptr )++;

	snprintf( buf, sizeof( buf ),
		"#x160You advance %s to level #x210%d#x160!#n\n\r", train_name, *train_ptr );
	send_to_char( buf, ch );
}


/*
 * do_paradoxarmor - Paradox armor wrapper
 */
void do_paradoxarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_PARADOX );
}


/*
 * update_paradox - Tick update for Paradox class (called from update.c)
 */
void update_paradox( CHAR_DATA *ch ) {
	int i;

	if ( IS_NPC( ch ) || !IS_CLASS( ch, CLASS_PARADOX ) )
		return;

	/* 1. Flux drift toward center (75) at +/-1 per tick (slower than Chronomancer) */
	if ( ch->rage < PARA_FLUX_CENTER )
		ch->rage++;
	else if ( ch->rage > PARA_FLUX_CENTER )
		ch->rage--;

	/* 2. Instability at extremes (same as Chronomancer but wider range) */
	if ( ( ch->rage <= 15 || ch->rage >= ( PARA_FLUX_CAP - 15 ) ) && number_percent() <= 10 ) {
		int effect = number_range( 1, 4 );
		switch ( effect ) {
		case 1:
			send_to_char( "#x160Temporal instability: Time #x210skips#x160 around you!#n\n\r", ch );
			break;
		case 2:
			if ( ch->fighting != NULL ) {
				int echo_dam = number_range( 100, 300 );
				send_to_char( "#x160Temporal instability: An #x210echo#x160 strikes your foe!#n\n\r", ch );
				damage( ch, ch->fighting, echo_dam, TYPE_UNDEFINED );
			}
			break;
		case 3:
			{
				int heal = ch->max_hit / 15;
				ch->hit = UMIN( ch->hit + heal, ch->max_hit );
				send_to_char( "#x160Temporal instability: Time #x210rewinds#x160 your wounds!#n\n\r", ch );
			}
			break;
		case 4:
			ch->pcdata->powers[PARA_ECHO_COUNT] = UMIN(
				ch->pcdata->powers[PARA_ECHO_COUNT] + 1, 5 );
			send_to_char( "#x160Temporal instability: A temporal #x210echo#x160 crystallizes!#n\n\r", ch );
			break;
		}
	}

	/* 3. Damage history tracking for Rewind (shift history, record current tick) */
	for ( i = 4; i > 0; i-- )
		ch->pcdata->stats[PARA_STAT_DAMAGE_HIST_0 + i] =
			ch->pcdata->stats[PARA_STAT_DAMAGE_HIST_0 + i - 1];
	ch->pcdata->stats[PARA_STAT_DAMAGE_HIST_0] = 0;

	/* 4. Past Self duration countdown */
	if ( ch->pcdata->powers[PARA_PAST_SELF_TICKS] > 0 ) {
		ch->pcdata->powers[PARA_PAST_SELF_TICKS]--;
		if ( ch->pcdata->powers[PARA_PAST_SELF_TICKS] == 0 ) {
			send_to_char( "#x160Your past self fades back into its own timeline.#n\n\r", ch );
			act( "The ghostly echo of $n fades away.", ch, NULL, NULL, TO_ROOM );
		}
	}

	/* 5. Time Loop countdown */
	if ( ch->pcdata->powers[PARA_TIME_LOOP_TICKS] > 0 ) {
		ch->pcdata->powers[PARA_TIME_LOOP_TICKS]--;
		if ( ch->pcdata->powers[PARA_TIME_LOOP_TICKS] == 0 ) {
			send_to_char( "#x160The time loop collapses — the cycle ends.#n\n\r", ch );
			if ( ch->fighting != NULL )
				act( "The temporal loop around $N shatters!", ch, NULL, ch->fighting, TO_ROOM );
		}
	}

	/* 6. Split Timeline countdown */
	if ( ch->pcdata->powers[PARA_SPLIT_ACTIVE] > 0 ) {
		ch->pcdata->powers[PARA_SPLIT_ACTIVE]--;
		if ( ch->pcdata->powers[PARA_SPLIT_ACTIVE] == 0 ) {
			send_to_char( "#x160The split timeline is closing! Choose now:#n\n\r", ch );
			send_to_char( "  #x210splittimeline keep#n  — Stay in this timeline\n\r", ch );
			send_to_char( "  #x210splittimeline revert#n — Revert to your snapshot\n\r", ch );
			/* Give them 1 more tick to choose, mark as "choosing" */
			ch->pcdata->powers[PARA_SPLIT_ACTIVE] = 1;
		}
	}

	/* 7. Age debuff countdown */
	if ( ch->pcdata->powers[PARA_AGE_TICKS] > 0 ) {
		ch->pcdata->powers[PARA_AGE_TICKS]--;
		if ( ch->pcdata->powers[PARA_AGE_TICKS] == 0 )
			send_to_char( "#x160The aging effect on your enemy fades.#n\n\r", ch );
	}

	/* 8. Future Strike delayed trigger */
	if ( ch->pcdata->powers[PARA_FUTURE_STRIKE_TGT] > 0 ) {
		ch->pcdata->powers[PARA_FUTURE_STRIKE_TGT]--;
		/* The damage was already dealt on activation (pre-damage model) */
	}

	/* 9. Eternity duration and aftermath */
	if ( ch->pcdata->powers[PARA_ETERNITY_TICKS] > 0 ) {
		ch->pcdata->powers[PARA_ETERNITY_TICKS]--;
		if ( ch->pcdata->powers[PARA_ETERNITY_TICKS] == 0 ) {
			ch->pcdata->powers[PARA_ETERNITY_AFTERMATH] = cfg( CFG_PARA_ETERNITY_AFTERMATH );
			send_to_char( "#x196Eternity ends! Time crashes back — you are #x160STUNNED!#n\n\r", ch );
			act( "$n snaps back into the timestream, dazed and reeling!", ch, NULL, NULL, TO_ROOM );
		}
	}
	if ( ch->pcdata->powers[PARA_ETERNITY_AFTERMATH] > 0 ) {
		ch->pcdata->powers[PARA_ETERNITY_AFTERMATH]--;
		if ( ch->pcdata->powers[PARA_ETERNITY_AFTERMATH] == 0 )
			send_to_char( "#x160You recover from the aftermath of Eternity.#n\n\r", ch );
	}

	/* 10. Echo count slow decay (lose 1 echo per tick if not in combat) */
	if ( ch->fighting == NULL && ch->pcdata->powers[PARA_ECHO_COUNT] > 0 )
		ch->pcdata->powers[PARA_ECHO_COUNT]--;

	/* 11. Cooldown decrements (powers[]) */
	if ( ch->pcdata->powers[PARA_REWIND_CD] > 0 )       ch->pcdata->powers[PARA_REWIND_CD]--;
	if ( ch->pcdata->powers[PARA_SPLIT_CD] > 0 )         ch->pcdata->powers[PARA_SPLIT_CD]--;
	if ( ch->pcdata->powers[PARA_CONVERGENCE_CD] > 0 )   ch->pcdata->powers[PARA_CONVERGENCE_CD]--;
	if ( ch->pcdata->powers[PARA_FUTURESTRIKE_CD] > 0 )  ch->pcdata->powers[PARA_FUTURESTRIKE_CD]--;
	if ( ch->pcdata->powers[PARA_PASTSELF_CD] > 0 )      ch->pcdata->powers[PARA_PASTSELF_CD]--;
	if ( ch->pcdata->powers[PARA_TIMELOOP_CD] > 0 )      ch->pcdata->powers[PARA_TIMELOOP_CD]--;

	/* 12. Cooldown decrements (stats[] overflow) */
	if ( ch->pcdata->stats[PARA_ETERNITY_CD] > 0 )       ch->pcdata->stats[PARA_ETERNITY_CD]--;
	if ( ch->pcdata->stats[PARA_PARADOXSTRIKE_CD] > 0 )  ch->pcdata->stats[PARA_PARADOXSTRIKE_CD]--;
	if ( ch->pcdata->stats[PARA_AGE_CD] > 0 )            ch->pcdata->stats[PARA_AGE_CD]--;
	if ( ch->pcdata->stats[PARA_COLLAPSE_CD] > 0 )       ch->pcdata->stats[PARA_COLLAPSE_CD]--;
	if ( ch->pcdata->stats[PARA_DESTABILIZE_CD] > 0 )    ch->pcdata->stats[PARA_DESTABILIZE_CD]--;
}
