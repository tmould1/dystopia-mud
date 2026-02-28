/***************************************************************************
 *  Cultist class - devotees of the void who channel eldritch energies     *
 *  Uses the "Corruption" resource (ch->rage) with risk/reward mechanic.   *
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

/*
 * Corruption status display - shared by Cultist and Voidborn
 */
void do_corruption( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int corrupt_cap;
	const char *tier_desc;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) && !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	corrupt_cap = IS_CLASS( ch, CLASS_VOIDBORN ) ? VOID_CORRUPT_CAP : CULT_CORRUPT_CAP;

	snprintf( buf, sizeof( buf ), "%s{~%s[#n Corruption Status %s]%s~}#n\n\r", ac, pc, pc, ac );
	send_to_char( buf, ch );

	snprintf( buf, sizeof( buf ), "Corruption: %s%d#n / %d\n\r", pc, ch->rage, corrupt_cap );
	send_to_char( buf, ch );

	/* Corruption damage tier */
	if ( ch->rage < CULT_CORRUPT_THRESH_LIGHT )
		tier_desc = "#GSafe#n";
	else if ( ch->rage < CULT_CORRUPT_THRESH_MOD )
		tier_desc = "#yLight burn#n (1% HP/tick)";
	else if ( ch->rage < CULT_CORRUPT_THRESH_HEAVY )
		tier_desc = "#RModerate burn#n (2% HP/tick)";
	else if ( IS_CLASS( ch, CLASS_VOIDBORN ) && ch->rage >= VOID_CORRUPT_THRESH_CATA )
		tier_desc = "#D#RCatastrophic burn#n (4% HP/tick)";
	else
		tier_desc = "#RHeavy burn#n (3% HP/tick)";

	snprintf( buf, sizeof( buf ), "Damage Tier: %s\n\r", tier_desc );
	send_to_char( buf, ch );

	if ( IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Void Resistance: #C25%%#n self-damage reduction\n\r", ch );
	}

	/* Active effects */
	if ( IS_CLASS( ch, CLASS_CULTIST ) ) {
		if ( ch->pcdata->powers[CULT_ELDRITCH_SIGHT] > 0 ) {
			snprintf( buf, sizeof( buf ), "%s[#nEldritch Sight: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[CULT_ELDRITCH_SIGHT], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CULT_GRASP_TICKS] > 0 ) {
			snprintf( buf, sizeof( buf ), "%s[#nGrasp: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[CULT_GRASP_TICKS], pc );
			send_to_char( buf, ch );
		}
		if ( ch->pcdata->powers[CULT_CONSTRICT_TICKS] > 0 ) {
			snprintf( buf, sizeof( buf ), "%s[#nConstrict: #C%d#n ticks%s]#n\n\r",
				pc, ch->pcdata->powers[CULT_CONSTRICT_TICKS], pc );
			send_to_char( buf, ch );
		}
	}

	return;
}

/*
 * Emergency corruption dump - costs HP to reduce corruption
 */
void do_cultpurge( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int hp_cost;
	int corrupt_removed;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) && !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->rage <= 0 ) {
		send_to_char( "You have no corruption to purge.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_WHISPERS_TICKS] > 0 ) {
		/* Reuse slot 4 as purge cooldown for simplicity */
	}

	hp_cost = ch->max_hit * cfg( CFG_CULTIST_PURGE_HP_COST ) / 100;
	corrupt_removed = cfg( CFG_CULTIST_PURGE_CORRUPTION_REMOVED );

	if ( ch->hit <= hp_cost + 1 ) {
		send_to_char( "You don't have enough life force to purge corruption.\n\r", ch );
		return;
	}

	ch->hit -= hp_cost;
	ch->rage = UMAX( 0, ch->rage - corrupt_removed );

	snprintf( buf, sizeof( buf ),
		"#x120You wrench the corruption from your body, gasping as vital energy bleeds away.#n\n\r" );
	send_to_char( buf, ch );
	act( "$n convulses as sickly #x120green#n energy pours from their body.", ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Training command - display and improve categories
 */
void do_voidtrain( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	const CLASS_BRACKET *br;
	const char *ac, *pc;
	int cost;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) && !IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	br = db_class_get_bracket( ch->class );
	ac = br ? br->accent_color : "";
	pc = br ? br->primary_color : "";

	if ( IS_CLASS( ch, CLASS_CULTIST ) ) {
		if ( argument[0] == '\0' ) {
			snprintf( buf, sizeof( buf ), "%s{~%s[#n Void Training %s]%s~}#n\n\r", ac, pc, pc, ac );
			send_to_char( buf, ch );

			cost = ( ch->pcdata->powers[CULT_TRAIN_LORE] + 1 ) * cfg( CFG_CULTIST_TRAIN_COST_MULT );
			snprintf( buf, sizeof( buf ), "%s[#nForbidden Lore:  Level %d/3  (Next: %d primal)%s]#n\n\r",
				pc, ch->pcdata->powers[CULT_TRAIN_LORE], cost, pc );
			send_to_char( buf, ch );

			cost = ( ch->pcdata->powers[CULT_TRAIN_TENTACLE] + 1 ) * cfg( CFG_CULTIST_TRAIN_COST_MULT );
			snprintf( buf, sizeof( buf ), "%s[#nTentacle Arts:   Level %d/3  (Next: %d primal)%s]#n\n\r",
				pc, ch->pcdata->powers[CULT_TRAIN_TENTACLE], cost, pc );
			send_to_char( buf, ch );

			cost = ( ch->pcdata->powers[CULT_TRAIN_MADNESS] + 1 ) * cfg( CFG_CULTIST_TRAIN_COST_MULT );
			snprintf( buf, sizeof( buf ), "%s[#nMadness:         Level %d/3  (Next: %d primal)%s]#n\n\r",
				pc, ch->pcdata->powers[CULT_TRAIN_MADNESS], cost, pc );
			send_to_char( buf, ch );

			send_to_char( "\n\rSyntax: voidtrain <lore|tentacle|madness>\n\r", ch );
			return;
		}

		if ( !str_prefix( argument, "lore" ) ) {
			if ( ch->pcdata->powers[CULT_TRAIN_LORE] >= 3 ) {
				send_to_char( "You have mastered Forbidden Lore.\n\r", ch );
				return;
			}
			cost = ( ch->pcdata->powers[CULT_TRAIN_LORE] + 1 ) * cfg( CFG_CULTIST_TRAIN_COST_MULT );
			if ( ch->practice < cost ) {
				snprintf( buf, sizeof( buf ), "You need %d primal to advance Forbidden Lore.\n\r", cost );
				send_to_char( buf, ch );
				return;
			}
			ch->practice -= cost;
			ch->pcdata->powers[CULT_TRAIN_LORE]++;
			snprintf( buf, sizeof( buf ),
				"#x120The void whispers deeper secrets... Forbidden Lore is now level %d.#n\n\r",
				ch->pcdata->powers[CULT_TRAIN_LORE] );
			send_to_char( buf, ch );
			return;
		}

		if ( !str_prefix( argument, "tentacle" ) ) {
			if ( ch->pcdata->powers[CULT_TRAIN_TENTACLE] >= 3 ) {
				send_to_char( "You have mastered the Tentacle Arts.\n\r", ch );
				return;
			}
			cost = ( ch->pcdata->powers[CULT_TRAIN_TENTACLE] + 1 ) * cfg( CFG_CULTIST_TRAIN_COST_MULT );
			if ( ch->practice < cost ) {
				snprintf( buf, sizeof( buf ), "You need %d primal to advance Tentacle Arts.\n\r", cost );
				send_to_char( buf, ch );
				return;
			}
			ch->practice -= cost;
			ch->pcdata->powers[CULT_TRAIN_TENTACLE]++;
			snprintf( buf, sizeof( buf ),
				"#x120Void tendrils writhe with new purpose... Tentacle Arts is now level %d.#n\n\r",
				ch->pcdata->powers[CULT_TRAIN_TENTACLE] );
			send_to_char( buf, ch );
			return;
		}

		if ( !str_prefix( argument, "madness" ) ) {
			if ( ch->pcdata->powers[CULT_TRAIN_MADNESS] >= 3 ) {
				send_to_char( "You have mastered Madness.\n\r", ch );
				return;
			}
			cost = ( ch->pcdata->powers[CULT_TRAIN_MADNESS] + 1 ) * cfg( CFG_CULTIST_TRAIN_COST_MULT );
			if ( ch->practice < cost ) {
				snprintf( buf, sizeof( buf ), "You need %d primal to advance Madness.\n\r", cost );
				send_to_char( buf, ch );
				return;
			}
			ch->practice -= cost;
			ch->pcdata->powers[CULT_TRAIN_MADNESS]++;
			snprintf( buf, sizeof( buf ),
				"#x120Your mind cracks further open... Madness is now level %d.#n\n\r",
				ch->pcdata->powers[CULT_TRAIN_MADNESS] );
			send_to_char( buf, ch );
			return;
		}

		send_to_char( "Syntax: voidtrain <lore|tentacle|madness>\n\r", ch );
		return;
	}

	/* Voidborn training */
	if ( IS_CLASS( ch, CLASS_VOIDBORN ) ) {
		if ( argument[0] == '\0' ) {
			snprintf( buf, sizeof( buf ), "%s*(%s(#n Void Training %s)%s)*#n\n\r", ac, pc, pc, ac );
			send_to_char( buf, ch );

			cost = ( ch->pcdata->powers[VOID_TRAIN_WARP] + 1 ) * cfg( CFG_VOIDBORN_TRAIN_COST_MULT );
			snprintf( buf, sizeof( buf ), "%s(#nReality Warp:    Level %d/3  (Next: %d primal)%s)#n\n\r",
				pc, ch->pcdata->powers[VOID_TRAIN_WARP], cost, pc );
			send_to_char( buf, ch );

			cost = ( ch->pcdata->powers[VOID_TRAIN_FORM] + 1 ) * cfg( CFG_VOIDBORN_TRAIN_COST_MULT );
			snprintf( buf, sizeof( buf ), "%s(#nElder Form:      Level %d/3  (Next: %d primal)%s)#n\n\r",
				pc, ch->pcdata->powers[VOID_TRAIN_FORM], cost, pc );
			send_to_char( buf, ch );

			cost = ( ch->pcdata->powers[VOID_TRAIN_COSMIC] + 1 ) * cfg( CFG_VOIDBORN_TRAIN_COST_MULT );
			snprintf( buf, sizeof( buf ), "%s(#nCosmic Horror:   Level %d/3  (Next: %d primal)%s)#n\n\r",
				pc, ch->pcdata->powers[VOID_TRAIN_COSMIC], cost, pc );
			send_to_char( buf, ch );

			send_to_char( "\n\rSyntax: voidtrain <warp|form|cosmic>\n\r", ch );
			return;
		}

		if ( !str_prefix( argument, "warp" ) ) {
			if ( ch->pcdata->powers[VOID_TRAIN_WARP] >= 3 ) {
				send_to_char( "You have mastered Reality Warp.\n\r", ch );
				return;
			}
			cost = ( ch->pcdata->powers[VOID_TRAIN_WARP] + 1 ) * cfg( CFG_VOIDBORN_TRAIN_COST_MULT );
			if ( ch->practice < cost ) {
				snprintf( buf, sizeof( buf ), "You need %d primal to advance Reality Warp.\n\r", cost );
				send_to_char( buf, ch );
				return;
			}
			ch->practice -= cost;
			ch->pcdata->powers[VOID_TRAIN_WARP]++;
			snprintf( buf, sizeof( buf ),
				"#x097Reality bends to your will... Reality Warp is now level %d.#n\n\r",
				ch->pcdata->powers[VOID_TRAIN_WARP] );
			send_to_char( buf, ch );
			return;
		}

		if ( !str_prefix( argument, "form" ) ) {
			if ( ch->pcdata->powers[VOID_TRAIN_FORM] >= 3 ) {
				send_to_char( "You have mastered the Elder Form.\n\r", ch );
				return;
			}
			cost = ( ch->pcdata->powers[VOID_TRAIN_FORM] + 1 ) * cfg( CFG_VOIDBORN_TRAIN_COST_MULT );
			if ( ch->practice < cost ) {
				snprintf( buf, sizeof( buf ), "You need %d primal to advance Elder Form.\n\r", cost );
				send_to_char( buf, ch );
				return;
			}
			ch->practice -= cost;
			ch->pcdata->powers[VOID_TRAIN_FORM]++;
			snprintf( buf, sizeof( buf ),
				"#x097Your form shifts beyond humanity... Elder Form is now level %d.#n\n\r",
				ch->pcdata->powers[VOID_TRAIN_FORM] );
			send_to_char( buf, ch );
			return;
		}

		if ( !str_prefix( argument, "cosmic" ) ) {
			if ( ch->pcdata->powers[VOID_TRAIN_COSMIC] >= 3 ) {
				send_to_char( "You have mastered Cosmic Horror.\n\r", ch );
				return;
			}
			cost = ( ch->pcdata->powers[VOID_TRAIN_COSMIC] + 1 ) * cfg( CFG_VOIDBORN_TRAIN_COST_MULT );
			if ( ch->practice < cost ) {
				snprintf( buf, sizeof( buf ), "You need %d primal to advance Cosmic Horror.\n\r", cost );
				send_to_char( buf, ch );
				return;
			}
			ch->practice -= cost;
			ch->pcdata->powers[VOID_TRAIN_COSMIC]++;
			snprintf( buf, sizeof( buf ),
				"#x097The cosmos reveals its horrors... Cosmic Horror is now level %d.#n\n\r",
				ch->pcdata->powers[VOID_TRAIN_COSMIC] );
			send_to_char( buf, ch );
			return;
		}

		send_to_char( "Syntax: voidtrain <warp|form|cosmic>\n\r", ch );
		return;
	}
}

/* =========================================================================
 * CULTIST ABILITIES - Forbidden Lore
 * ========================================================================= */

/*
 * Eldritch Sight - Void detection (Forbidden Lore 1)
 */
void do_eldritchsight( CHAR_DATA *ch, char *argument ) {
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_LORE] < 1 ) {
		send_to_char( "You must train Forbidden Lore to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_ELDRITCH_SIGHT] > 0 ) {
		send_to_char( "Your eldritch sight is already active.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CULTIST_ELDRITCHSIGHT_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_ELDRITCHSIGHT_CORRUPT_GAIN ), CULT_CORRUPT_CAP );
	ch->pcdata->powers[CULT_ELDRITCH_SIGHT] = cfg( CFG_CULTIST_ELDRITCHSIGHT_DURATION );

	/* Track peak corruption */
	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	SET_BIT( ch->affected_by, AFF_DETECT_HIDDEN );
	SET_BIT( ch->affected_by, AFF_DETECT_INVIS );

	send_to_char( "#x120Your eyes burn with eldritch light as you peer beyond the veil.#n\n\r", ch );
	act( "$n's eyes flare with an unsettling #x120green#n glow.", ch, NULL, NULL, TO_ROOM );

	return;
}

/*
 * Whispers - Weakness detection (Forbidden Lore 2)
 */
void do_whispers( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	int mana_cost;
	int debuff_bonus;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_LORE] < 2 ) {
		send_to_char( "You must train Forbidden Lore to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting to use whispers.\n\r", ch );
		return;
	}

	victim = ch->fighting;
	mana_cost = cfg( CFG_CULTIST_WHISPERS_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_WHISPERS_CORRUPT_GAIN ), CULT_CORRUPT_CAP );
	ch->pcdata->powers[CULT_WHISPERS_TICKS] = cfg( CFG_CULTIST_WHISPERS_DEBUFF_DURATION );

	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	/* Show target info */
	snprintf( buf, sizeof( buf ),
		"#x120The void whispers of %s's weaknesses...#n\n\r"
		"Health: %d%%\n\r",
		IS_NPC( victim ) ? victim->short_descr : victim->name,
		( victim->hit * 100 ) / UMAX( 1, victim->max_hit ) );
	send_to_char( buf, ch );

	/* Exposed debuff - bonus damage from all sources */
	debuff_bonus = ( ch->rage >= 50 ) ? 15 : 10;
	snprintf( buf, sizeof( buf ),
		"#x120Exposed! %s takes +%d%% damage from all sources.#n\n\r",
		IS_NPC( victim ) ? victim->short_descr : victim->name, debuff_bonus );
	send_to_char( buf, ch );

	act( "#x120Otherworldly whispers swirl around $N, exposing their weaknesses.#n",
		ch, NULL, victim, TO_ROOM );

	return;
}

/*
 * Unravel - Strip magical protections (Forbidden Lore 3)
 */
void do_unravel( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int mana_cost;
	int max_removes;
	int removed = 0;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_LORE] < 3 ) {
		send_to_char( "You must train Forbidden Lore to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting to unravel protections.\n\r", ch );
		return;
	}

	victim = ch->fighting;
	mana_cost = cfg( CFG_CULTIST_UNRAVEL_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_UNRAVEL_CORRUPT_GAIN ), CULT_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	max_removes = ( ch->rage >= 75 ) ?
		cfg( CFG_CULTIST_UNRAVEL_MAX_REMOVES_HIGH ) :
		cfg( CFG_CULTIST_UNRAVEL_MAX_REMOVES );

	/* Strip sanctuary */
	if ( IS_AFFECTED( victim, AFF_SANCTUARY ) && removed < max_removes ) {
		REMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
		removed++;
		act( "#x120Void energy tears away $N's sanctuary!#n", ch, NULL, victim, TO_CHAR );
		act( "#x120Your sanctuary is ripped away by tendrils of void!#n", ch, NULL, victim, TO_VICT );
	}

	/* Strip protection */
	if ( IS_AFFECTED( victim, AFF_PROTECT ) && removed < max_removes ) {
		REMOVE_BIT( victim->affected_by, AFF_PROTECT );
		removed++;
		act( "#x120Void energy tears away $N's protection!#n", ch, NULL, victim, TO_CHAR );
	}

	if ( removed == 0 ) {
		send_to_char( "#x120The void finds nothing to unravel.#n\n\r", ch );
	} else {
		act( "#x120$n tears apart the magical protections surrounding $N!#n",
			ch, NULL, victim, TO_NOTVICT );
	}

	return;
}

/* =========================================================================
 * CULTIST ABILITIES - Tentacle Arts
 * ========================================================================= */

/*
 * Void Tendril - Basic void damage attack (Tentacle Arts 1)
 */
void do_voidtendril( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int mana_cost;
	int dam;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_TENTACLE] < 1 ) {
		send_to_char( "You must train Tentacle Arts to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting to lash with a void tendril.\n\r", ch );
		return;
	}

	victim = ch->fighting;
	mana_cost = cfg( CFG_CULTIST_VOIDTENDRIL_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_VOIDTENDRIL_CORRUPT_GAIN ), CULT_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	/* Base damage + corruption bonus */
	dam = number_range( 80, 150 ) + ( ch->rage * 2 );
	dam = dam * ( 100 + ch->rage / 10 ) / 100;  /* corruption % bonus */

	ch->pcdata->stats[CULT_STAT_VOID_DAMAGE] += dam;

	act( "#x120A tendril of pure void lashes out at $N!#n", ch, NULL, victim, TO_CHAR );
	act( "#x120A tendril of void-stuff lashes out from $n, striking $N!#n", ch, NULL, victim, TO_NOTVICT );
	act( "#x120A writhing void tendril strikes you!#n", ch, NULL, victim, TO_VICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );

	return;
}

/*
 * Grasp - Grapple target, preventing flee (Tentacle Arts 2)
 */
void do_grasp( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_TENTACLE] < 2 ) {
		send_to_char( "You must train Tentacle Arts to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting to grasp your prey.\n\r", ch );
		return;
	}

	victim = ch->fighting;

	if ( ch->pcdata->powers[CULT_GRASP_TICKS] > 0 ) {
		send_to_char( "You already have a target grasped.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CULTIST_GRASP_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_GRASP_CORRUPT_GAIN ), CULT_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	ch->pcdata->powers[CULT_GRASP_TICKS] = ( ch->rage >= 50 ) ?
		cfg( CFG_CULTIST_GRASP_DURATION_HIGH ) :
		cfg( CFG_CULTIST_GRASP_DURATION );

	act( "#x120Void tendrils wrap around $N, holding them fast!#n", ch, NULL, victim, TO_CHAR );
	act( "#x120Writhing void tendrils coil around $N!#n", ch, NULL, victim, TO_NOTVICT );
	act( "#x120Void tendrils wrap around you, holding you in place!#n", ch, NULL, victim, TO_VICT );

	return;
}

/*
 * Constrict - Crushing DoT on grasped target (Tentacle Arts 3)
 */
void do_constrict( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_TENTACLE] < 3 ) {
		send_to_char( "You must train Tentacle Arts to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting.\n\r", ch );
		return;
	}

	victim = ch->fighting;

	if ( ch->pcdata->powers[CULT_GRASP_TICKS] <= 0 ) {
		send_to_char( "You must have a target grasped first.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_CONSTRICT_TICKS] > 0 ) {
		send_to_char( "You are already constricting your target.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CULTIST_CONSTRICT_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_CONSTRICT_CORRUPT_GAIN ), CULT_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	ch->pcdata->powers[CULT_CONSTRICT_TICKS] = cfg( CFG_CULTIST_CONSTRICT_DURATION );

	act( "#x120The void tendrils tighten, crushing $N with immense pressure!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#x120The void tendrils around $N tighten, crushing the life from them!#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#x120The void tendrils constrict, crushing you!#n",
		ch, NULL, victim, TO_VICT );

	return;
}

/* =========================================================================
 * CULTIST ABILITIES - Madness
 * ========================================================================= */

/*
 * Maddening Gaze - Single-target sanity damage + debuff (Madness 1)
 */
void do_maddeninggaze( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int mana_cost;
	int dam;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_MADNESS] < 1 ) {
		send_to_char( "You must train Madness to level 1 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting.\n\r", ch );
		return;
	}

	victim = ch->fighting;
	mana_cost = cfg( CFG_CULTIST_MADDENINGGAZE_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_MADDENINGGAZE_CORRUPT_GAIN ), CULT_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	dam = number_range( 100, 180 ) + ( ch->rage * 2 );
	ch->pcdata->stats[CULT_STAT_VOID_DAMAGE] += dam;

	act( "#x120You lock eyes with $N, forcing them to gaze into the void!#n",
		ch, NULL, victim, TO_CHAR );
	act( "#x120$n locks eyes with $N, and $N recoils in horror!#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#x120$n's eyes become windows to the void - you cannot look away!#n",
		ch, NULL, victim, TO_VICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );

	return;
}

/*
 * Gibbering - AoE confusion effect (Madness 2)
 */
void do_gibbering( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int mana_cost;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_MADNESS] < 2 ) {
		send_to_char( "You must train Madness to level 2 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting.\n\r", ch );
		return;
	}

	mana_cost = cfg( CFG_CULTIST_GIBBERING_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_GIBBERING_CORRUPT_GAIN ), CULT_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	ch->pcdata->powers[CULT_GIBBERING_TICKS] = cfg( CFG_CULTIST_GIBBERING_CONFUSION_DURATION );

	send_to_char( "#x120You speak in tongues that shatter reason itself!#n\n\r", ch );
	act( "#x120$n speaks in tongues - maddening gibberish assaults your mind!#n",
		ch, NULL, NULL, TO_ROOM );

	/* AoE flee chance for NPCs */
	LIST_FOR_EACH_SAFE(vch, vch_next, &ch->in_room->characters, CHAR_DATA, room_node) {
		if ( vch == ch ) continue;
		if ( !IS_NPC( vch ) ) continue;
		if ( vch->fighting != ch ) continue;

		if ( number_percent() < cfg( CFG_CULTIST_GIBBERING_FLEE_CHANCE ) ) {
			act( "$N flees in confusion!", ch, NULL, vch, TO_CHAR );
			do_flee( vch, "" );
		}
	}

	return;
}

/*
 * Insanity - Ultimate madness attack (Madness 3)
 */
void do_insanity( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int mana_cost;
	int dam;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[CULT_TRAIN_MADNESS] < 3 ) {
		send_to_char( "You must train Madness to level 3 first.\n\r", ch );
		return;
	}

	if ( ch->fighting == NULL ) {
		send_to_char( "You must be fighting.\n\r", ch );
		return;
	}

	victim = ch->fighting;
	mana_cost = cfg( CFG_CULTIST_INSANITY_MANA_COST );
	if ( ch->mana < mana_cost ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= mana_cost;
	ch->rage = UMIN( ch->rage + cfg( CFG_CULTIST_INSANITY_CORRUPT_GAIN ), CULT_CORRUPT_CAP );

	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	dam = number_range( 250, 400 ) + ( ch->rage * 3 );
	dam = dam * ( 100 + ch->rage / 10 ) / 100;
	ch->pcdata->stats[CULT_STAT_VOID_DAMAGE] += dam;

	act( "#x120You shatter $N's grip on reality!#n", ch, NULL, victim, TO_CHAR );
	act( "#x120$n unleashes madness upon $N - $N screams in abject terror!#n",
		ch, NULL, victim, TO_NOTVICT );
	act( "#x120Your mind shatters as $n forces the void into your consciousness!#n",
		ch, NULL, victim, TO_VICT );

	damage( ch, victim, dam, TYPE_UNDEFINED );

	/* NPC self-attack chance */
	if ( IS_NPC( victim ) && victim->position > POS_STUNNED ) {
		if ( number_percent() < cfg( CFG_CULTIST_INSANITY_SELFATTACK_CHANCE ) ) {
			act( "#x120$N turns on $Mself in a fit of madness!#n", ch, NULL, victim, TO_CHAR );
			act( "#x120$N attacks $Mself in blind insanity!#n", ch, NULL, victim, TO_NOTVICT );
			damage( victim, victim, victim->hit / 10, TYPE_UNDEFINED );
		}
	}

	return;
}

/* =========================================================================
 * CULTIST TICK UPDATE
 * ========================================================================= */

void update_cultist( CHAR_DATA *ch ) {
	int tier;
	int self_dam;

	if ( !IS_CLASS( ch, CLASS_CULTIST ) )
		return;

	/* Corruption build/decay */
	if ( ch->fighting != NULL ) {
		ch->rage = UMIN( ch->rage + 1, CULT_CORRUPT_CAP );
	} else {
		if ( ch->rage > 0 )
			ch->rage = UMAX( 0, ch->rage - 2 );
	}

	/* Peak tracking */
	if ( ch->rage > ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] )
		ch->pcdata->stats[CULT_STAT_PEAK_CORRUPT] = ch->rage;

	/* Corruption self-damage */
	if ( ch->rage >= CULT_CORRUPT_THRESH_LIGHT ) {
		if ( ch->rage >= CULT_CORRUPT_THRESH_HEAVY )
			tier = 3;
		else if ( ch->rage >= CULT_CORRUPT_THRESH_MOD )
			tier = 2;
		else
			tier = 1;

		self_dam = ch->max_hit * tier / 100;
		ch->hit -= self_dam;
		if ( ch->hit < 1 ) ch->hit = 1;

		if ( self_dam > 0 ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ),
				"#x064Corruption burns through your body for #R%d#x064 damage.#n\n\r", self_dam );
			send_to_char( buf, ch );
		}
	}

	/* Eldritch Sight countdown */
	if ( ch->pcdata->powers[CULT_ELDRITCH_SIGHT] > 0 ) {
		ch->pcdata->powers[CULT_ELDRITCH_SIGHT]--;
		if ( ch->pcdata->powers[CULT_ELDRITCH_SIGHT] <= 0 ) {
			send_to_char( "#x120Your eldritch sight fades.#n\n\r", ch );
			/* Note: don't remove AFF bits here - they may come from other sources */
		}
	}

	/* Whispers debuff countdown */
	if ( ch->pcdata->powers[CULT_WHISPERS_TICKS] > 0 )
		ch->pcdata->powers[CULT_WHISPERS_TICKS]--;

	/* Grasp duration + damage */
	if ( ch->pcdata->powers[CULT_GRASP_TICKS] > 0 ) {
		ch->pcdata->powers[CULT_GRASP_TICKS]--;
		if ( ch->fighting != NULL ) {
			int grasp_dam = cfg( CFG_CULTIST_GRASP_TICK_DAMAGE );
			damage( ch, ch->fighting, grasp_dam, TYPE_UNDEFINED );
		}
		if ( ch->pcdata->powers[CULT_GRASP_TICKS] <= 0 ) {
			send_to_char( "#x120Your void grasp releases.#n\n\r", ch );
			ch->pcdata->powers[CULT_CONSTRICT_TICKS] = 0;
		}
	}

	/* Constrict duration + damage */
	if ( ch->pcdata->powers[CULT_CONSTRICT_TICKS] > 0 ) {
		ch->pcdata->powers[CULT_CONSTRICT_TICKS]--;
		if ( ch->fighting != NULL ) {
			int con_dam = number_range( 60, 100 ) + ch->rage;
			ch->pcdata->stats[CULT_STAT_VOID_DAMAGE] += con_dam;
			act( "#x120The void tendrils crush $N!#n", ch, NULL, ch->fighting, TO_CHAR );
			damage( ch, ch->fighting, con_dam, TYPE_UNDEFINED );
		}
	}

	/* Gibbering confusion countdown */
	if ( ch->pcdata->powers[CULT_GIBBERING_TICKS] > 0 )
		ch->pcdata->powers[CULT_GIBBERING_TICKS]--;
}

/* =========================================================================
 * CULTIST ARMOR
 * ========================================================================= */

void do_cultistarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_CULTIST );
}
