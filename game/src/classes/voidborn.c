/***************************************************************************
 *  Voidborn class - beings who have surrendered to the void               *
 *  Upgrade from Cultist with enhanced corruption and reality-warping      *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "cfg.h"
#include "cultist.h"
#include "../db/db_class.h"

/* Forward declarations for class armor */
extern void do_classarmor_generic( CHAR_DATA *ch, char *argument, int class_id );

/* =========================================================================
 * VOIDBORN ABILITIES - Reality Warp
 * ========================================================================= */

/*
 * Phase Shift - Become partially ethereal (Reality Warp 1)
 */
void do_phaseshift( CHAR_DATA *ch, char *argument ) {
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_WARP] < 1 ) {
		send_to_char( "You must train Reality Warp to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_PHASE_SHIFT] > 0 ) {
		send_to_char( "You are already phase shifted.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_VOIDBORN_PHASESHIFT_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_VOIDBORN_PHASESHIFT_CORRUPT_GAIN ), VOID_CORRUPT_CAP );
	ch->pcdata->powers[VOID_PHASE_SHIFT] = cfg( CFG_VOIDBORN_PHASESHIFT_DURATION );

	if ( ch->rage > ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] = ch->rage;

	send_to_char( "#x097Your body shifts partially out of reality, becoming translucent.#n\n\r", ch );
	act( "#x097$n's form flickers and becomes translucent, phasing between realities.#n",
		ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Dimensional Rend - Tear reality, create hazard zone (Reality Warp 2)
 */
void do_dimensionalrend( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int mana_cost;
	int dam;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_WARP] < 2 ) {
		send_to_char( "You must train Reality Warp to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting.\n\r", ch );
		return;
	}

	victim = ch->fighting;
	mana_cost = cfg( CFG_VOIDBORN_DIMREND_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_VOIDBORN_DIMREND_CORRUPT_GAIN ), VOID_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] = ch->rage;

	/* Initial damage */
	dam = number_range( 200, 350 ) + ( ch->rage * 3 );
	ch->pcdata->stats[VOID_STAT_VOID_DAMAGE] += dam;

	/* Set up rift hazard */
	ch->pcdata->powers[VOID_RIFT_ROOM] = ch->in_room->vnum;
	ch->pcdata->powers[VOID_RIFT_TICKS] = cfg( CFG_VOIDBORN_DIMREND_HAZARD_DURATION );

	act( "#x097You tear a rift in reality itself! Void energy pours through!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#x097$n tears open a rift in reality! #x055Dark energy#x097 pours through the gap!#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#x097$n tears open a rift in reality, and void energy slams into you!#n",
		ch, NULL, victim, TO_VICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );

	return;
}

/*
 * Unmake - Attempt to erase target from reality (Reality Warp 3)
 */
void do_unmake( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int mana_cost;
	int dam;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_WARP] < 3 ) {
		send_to_char( "You must train Reality Warp to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting.\n\r", ch );
		return;
	}

	if ( ch->rage < cfg( CFG_VOIDBORN_UNMAKE_CORRUPT_REQ ) ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ),
			"You need at least %d corruption to attempt to unmake reality.\n\r",
			cfg( CFG_VOIDBORN_UNMAKE_CORRUPT_REQ ) );
		send_to_char( buf, ch );
		return;
	}

	victim = ch->fighting;
	mana_cost = cfg( CFG_VOIDBORN_UNMAKE_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMAX( 0, ch->rage - cfg( CFG_VOIDBORN_UNMAKE_CORRUPT_COST ) );

	/* Instakill for weak NPCs */
	if ( IS_NPC( victim ) && victim->max_hit < cfg( CFG_VOIDBORN_UNMAKE_INSTAKILL_THRESHOLD ) ) {
		act( "#x055You focus the void's power and $N simply... ceases to exist.#n",
			ch, NULL, victim, TO_CHAR );
		act( "#x055$n gestures at $N, and $N dissolves into nothingness!#n",
			ch, NULL, victim, TO_NOTVICT );
		raw_kill( victim );
		return;
	}

	/* Massive damage for everything else */
	dam = number_range( 500, 800 ) + ( ch->rage * 5 );
	ch->pcdata->stats[VOID_STAT_VOID_DAMAGE] += dam;

	act( "#x055You attempt to unmake $N from reality!#n", ch, NULL, victim, TO_CHAR );
	act( "#x055$n attempts to erase $N from existence!#n", ch, NULL, victim, TO_NOTVICT );
	act( "#x055$n attempts to unmake you - void energy tears at your very being!#n",
		ch, NULL, victim, TO_VICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );

	return;
}

/* =========================================================================
 * VOIDBORN ABILITIES - Elder Form
 * ========================================================================= */

/*
 * Void Shape - Sprout tentacles for extra attacks (Elder Form 1)
 */
void do_voidshape( CHAR_DATA *ch, char *argument ) {
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_FORM] < 1 ) {
		send_to_char( "You must train Elder Form to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_VOID_SHAPE] > 0 ) {
		send_to_char( "Your void shape is already active.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_VOIDBORN_VOIDSHAPE_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_VOIDBORN_VOIDSHAPE_CORRUPT_GAIN ), VOID_CORRUPT_CAP );
	ch->pcdata->powers[VOID_VOID_SHAPE] = cfg( CFG_VOIDBORN_VOIDSHAPE_DURATION );

	if ( ch->rage > ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] = ch->rage;

	send_to_char( "#x097Writhing tentacles of void-stuff erupt from your body!#n\n\r", ch );
	act( "#x097Writhing tentacles burst from $n's body!#n", ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Aberrant Growth - Mutate for combat bonuses (Elder Form 2)
 */
void do_aberrantgrowth( CHAR_DATA *ch, char *argument ) {
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_FORM] < 2 ) {
		send_to_char( "You must train Elder Form to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_ABERRANT_GROWTH] > 0 ) {
		send_to_char( "Aberrant growth is already active.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_VOIDBORN_ABERRANT_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_VOIDBORN_ABERRANT_CORRUPT_GAIN ), VOID_CORRUPT_CAP );
	ch->pcdata->powers[VOID_ABERRANT_GROWTH] = cfg( CFG_VOIDBORN_ABERRANT_DURATION );

	if ( ch->rage > ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] = ch->rage;

	send_to_char( "#x097Your body mutates, growing chitinous plates and barbed limbs!#n\n\r", ch );
	act( "#x097$n's body warps and mutates into something inhuman!#n", ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Final Form - Complete transformation into aberration (Elder Form 3)
 */
void do_finalform( CHAR_DATA *ch, char *argument ) {
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_FORM] < 3 ) {
		send_to_char( "You must train Elder Form to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_FINAL_FORM] > 0 ) {
		send_to_char( "You are already in your final form.\n\r", ch );
		return;
	}

	if ( ch->rage < cfg( CFG_VOIDBORN_FINALFORM_CORRUPT_REQ ) ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ),
			"You need at least %d corruption to achieve your final form.\n\r",
			cfg( CFG_VOIDBORN_FINALFORM_CORRUPT_REQ ) );
		send_to_char( buf, ch );
		return;
	}

	mana_cost = cfg( CFG_VOIDBORN_FINALFORM_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_VOIDBORN_FINALFORM_CORRUPT_GAIN ), VOID_CORRUPT_CAP );
	ch->pcdata->powers[VOID_FINAL_FORM] = cfg( CFG_VOIDBORN_FINALFORM_DURATION );

	if ( ch->rage > ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] = ch->rage;

	send_to_char(
		"#x055You surrender completely to the void!\n\r"
		"#x097Your body transforms into a towering eldritch aberration!#n\n\r", ch );
	act( "#x055$n's body twists and expands into a horrifying eldritch aberration!#n",
		ch, NULL, NULL, TO_ROOM );

	return;
}

/* =========================================================================
 * VOIDBORN ABILITIES - Cosmic Horror
 * ========================================================================= */

/*
 * Summon Thing - Pull a void creature through a rift (Cosmic Horror 1)
 */
void do_summonthing( CHAR_DATA *ch, char *argument ) {
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_COSMIC] < 1 ) {
		send_to_char( "You must train Cosmic Horror to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_SUMMON_COUNT] >= VOID_MAX_SUMMONS ) {
		send_to_char( "You cannot control any more void creatures.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_VOIDBORN_SUMMONTHING_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_VOIDBORN_SUMMONTHING_CORRUPT_GAIN ), VOID_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] = ch->rage;

	ch->pcdata->powers[VOID_SUMMON_COUNT]++;

	/* TODO: Create actual mob using VNUM_VOID_CREATURE when mob template exists */
	send_to_char( "#x097You tear open a rift and a writhing horror crawls through!#n\n\r", ch );
	act( "#x097$n tears open a rift and something terrible crawls through!#n",
		ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Star Spawn - Cosmic energy AoE attack (Cosmic Horror 2)
 */
void do_starspawn( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int mana_cost;
	int dam;
	int aoe_dam;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_COSMIC] < 2 ) {
		send_to_char( "You must train Cosmic Horror to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting.\n\r", ch );
		return;
	}

	victim = ch->fighting;
	mana_cost = cfg( CFG_VOIDBORN_STARSPAWN_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_VOIDBORN_STARSPAWN_CORRUPT_GAIN ), VOID_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] = ch->rage;

	dam = number_range( 350, 550 ) + ( ch->rage * 4 );
	aoe_dam = dam * cfg( CFG_VOIDBORN_STARSPAWN_AOE_PCT ) / 100;
	ch->pcdata->stats[VOID_STAT_VOID_DAMAGE] += dam;

	act( "#x055A bolt of cosmic energy crashes down from the void between stars!#n",
		ch, NULL, NULL, TO_CHAR );
	act( "#x055A bolt of cosmic energy crashes down from above, striking $N!#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#x055Cosmic energy from the void between stars slams into you!#n",
		ch, NULL, victim, TO_VICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );

	/* AoE secondary damage */
	for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
		vch_next = vch->next_in_room;
		if ( vch == ch || vch == victim ) continue;
		if ( !IS_NPC( vch ) ) continue;
		if ( vch->fighting != ch ) continue;

		ch->pcdata->stats[VOID_STAT_VOID_DAMAGE] += aoe_dam;
		damage( ch, vch, aoe_dam, TYPE_UNDEFINED );
	}

	return;
}

/*
 * Entropy - Ultimate AoE decay effect (Cosmic Horror 3)
 */
void do_entropy( CHAR_DATA *ch, char *argument ) {
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_TRAIN_COSMIC] < 3 ) {
		send_to_char( "You must train Cosmic Horror to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting.\n\r", ch );
		return;
	}

	if ( ch->rage < cfg( CFG_VOIDBORN_ENTROPY_CORRUPT_REQ ) ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ),
			"You need at least %d corruption to invoke entropy.\n\r",
			cfg( CFG_VOIDBORN_ENTROPY_CORRUPT_REQ ) );
		send_to_char( buf, ch );
		return;
	}

	if ( ch->pcdata->powers[VOID_ENTROPY_TICKS] > 0 ) {
		send_to_char( "Entropy is already in effect.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_VOIDBORN_ENTROPY_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMAX( 0, ch->rage - cfg( CFG_VOIDBORN_ENTROPY_CORRUPT_COST ) );
	ch->pcdata->powers[VOID_ENTROPY_TICKS] = cfg( CFG_VOIDBORN_ENTROPY_DURATION );

	send_to_char(
		"#x055You accelerate the decay of all things around you!\n\r"
		"#x097Reality itself begins to crumble!#n\n\r", ch );
	act( "#x055$n invokes entropy - everything around begins to decay and crumble!#n",
		ch, NULL, NULL, TO_ROOM );

	return;
}

/* =========================================================================
 * VOIDBORN TICK UPDATE
 * ========================================================================= */

void update_voidborn( CHAR_DATA *ch ) {
	int tier;
	int self_dam;

	if ( !IS_CLASS( ch, CLASS_VOIDBORN ) )
		return;

	/* Corruption build/decay (enhanced: +2 fighting, -1 out of combat) */
	if ( ch->fighting != NULL ) {
		ch->rage = UMIN( ch->rage + 2, VOID_CORRUPT_CAP );
	} else {
		if ( ch->rage > 0 )
			ch->rage = UMAX( 0, ch->rage - 1 );
	}

	/* Peak tracking */
	if ( ch->rage > ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[VOID_STAT_PEAK_CORRUPT] = ch->rage;

	/* Corruption self-damage (with 25% resistance) */
	if ( ch->rage >= CULT_CORRUPT_THRESH_LIGHT ) {
		if ( ch->rage >= VOID_CORRUPT_THRESH_CATA )
			tier = 4;
		else if ( ch->rage >= CULT_CORRUPT_THRESH_HEAVY )
			tier = 3;
		else if ( ch->rage >= CULT_CORRUPT_THRESH_MOD )
			tier = 2;
		else
			tier = 1;

		self_dam = ch->max_hit * tier / 100;
		self_dam = self_dam * ( 100 - VOID_CORRUPT_RESIST ) / 100;  /* 25% resistance */
		ch->hit -= self_dam;
		if ( ch->hit < 1 ) ch->hit = 1;

		if ( self_dam > 0 ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ),
				"#x055Corruption burns through your body for #R%d#x055 damage.#n\n\r", self_dam );
			send_to_char( buf, ch );
		}
	}

	/* Phase Shift countdown */
	if ( ch->pcdata->powers[VOID_PHASE_SHIFT] > 0 ) {
		ch->pcdata->powers[VOID_PHASE_SHIFT]--;
		if ( ch->pcdata->powers[VOID_PHASE_SHIFT] <= 0 )
			send_to_char( "#x097You solidify back into reality.#n\n\r", ch );
	}

	/* Void Shape countdown */
	if ( ch->pcdata->powers[VOID_VOID_SHAPE] > 0 ) {
		ch->pcdata->powers[VOID_VOID_SHAPE]--;
		if ( ch->pcdata->powers[VOID_VOID_SHAPE] <= 0 )
			send_to_char( "#x097Your void tentacles retract and dissolve.#n\n\r", ch );
	}

	/* Aberrant Growth countdown */
	if ( ch->pcdata->powers[VOID_ABERRANT_GROWTH] > 0 ) {
		ch->pcdata->powers[VOID_ABERRANT_GROWTH]--;
		if ( ch->pcdata->powers[VOID_ABERRANT_GROWTH] <= 0 )
			send_to_char( "#x097Your aberrant mutations recede.#n\n\r", ch );
	}

	/* Final Form countdown */
	if ( ch->pcdata->powers[VOID_FINAL_FORM] > 0 ) {
		ch->pcdata->powers[VOID_FINAL_FORM]--;
		if ( ch->pcdata->powers[VOID_FINAL_FORM] <= 0 )
			send_to_char( "#x055Your eldritch form collapses back to humanoid shape.#n\n\r", ch );
	}

	/* Dimensional Rend hazard processing */
	if ( ch->pcdata->powers[VOID_RIFT_TICKS] > 0 ) {
		ch->pcdata->powers[VOID_RIFT_TICKS]--;

		/* Damage enemies in the rift room */
		if ( ch->in_room != NULL && ch->in_room->vnum == ch->pcdata->powers[VOID_RIFT_ROOM] ) {
			CHAR_DATA *vch;
			CHAR_DATA *vch_next;
			int rift_dam = cfg( CFG_VOIDBORN_DIMREND_HAZARD_DAMAGE );

			for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
				vch_next = vch->next_in_room;
				if ( vch == ch ) continue;
				if ( !IS_NPC( vch ) ) continue;
				if ( vch->fighting != ch ) continue;

				damage( ch, vch, rift_dam, TYPE_UNDEFINED );
			}
		}

		if ( ch->pcdata->powers[VOID_RIFT_TICKS] <= 0 ) {
			send_to_char( "#x097The dimensional rift seals itself.#n\n\r", ch );
			ch->pcdata->powers[VOID_RIFT_ROOM] = 0;
		}
	}

	/* Entropy AoE damage processing */
	if ( ch->pcdata->powers[VOID_ENTROPY_TICKS] > 0 ) {
		ch->pcdata->powers[VOID_ENTROPY_TICKS]--;

		if ( ch->in_room != NULL ) {
			CHAR_DATA *vch;
			CHAR_DATA *vch_next;
			int ent_dam = number_range( 100, 200 );

			for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
				vch_next = vch->next_in_room;
				if ( vch == ch ) continue;
				if ( !IS_NPC( vch ) ) continue;
				if ( vch->fighting != ch ) continue;

				damage( ch, vch, ent_dam, TYPE_UNDEFINED );
			}

			/* Self-damage from entropy (50% of tick damage) */
			{
				int self_ent = ent_dam / 2;
				self_ent = self_ent * ( 100 - VOID_CORRUPT_RESIST ) / 100;
				ch->hit -= self_ent;
				if ( ch->hit < 1 ) ch->hit = 1;
			}
		}

		if ( ch->pcdata->powers[VOID_ENTROPY_TICKS] <= 0 )
			send_to_char( "#x055The entropic effect fades.#n\n\r", ch );
	}
}

/* =========================================================================
 * VOIDBORN ARMOR
 * ========================================================================= */

void do_voidbornarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_VOIDBORN );
}
