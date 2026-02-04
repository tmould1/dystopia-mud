/***************************************************************************
 *  Siren class - upgrade from Dirgesinger                                 *
 *  Supernatural voice: domination, mass charm, devastating sonic power.   *
 *  Uses the "Resonance" resource (ch->rage) with a higher cap of 150.    *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "ability_config.h"
#include "dirgesinger.h"

/* Forward declarations for update functions */
void update_siren( CHAR_DATA *ch );

/*
 * Banshee Wail - devastating AoE, can instakill weak mobs
 */
void do_bansheewail( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_DEVASTATION] < 1 ) {
		send_to_char( "You need Devastation level 1 to use banshee wail. See #Rvoicetrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.bansheewail.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "siren.bansheewail.resonance_req" ) ) {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "You need at least %d resonance to unleash a banshee wail.\n\r", acfg( "siren.bansheewail.resonance_req" ) );
		send_to_char( buf, ch );
		return;
	}
	WAIT_STATE( ch, acfg( "siren.bansheewail.cooldown" ) );
	ch->mana -= acfg( "siren.bansheewail.mana_cost" );
	ch->rage -= acfg( "siren.bansheewail.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	act( "You open your mouth and release an unearthly #Rbanshee wail#n!", ch, NULL, NULL, TO_CHAR );
	act( "$n opens $s mouth and releases an unearthly #Rbanshee wail#n!", ch, NULL, NULL, TO_ROOM );

	for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
		vch_next = vch->next_in_room;
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		/* Instakill weak mobs */
		if ( IS_NPC( vch ) && vch->max_hit < acfg( "siren.bansheewail.instakill_threshold" ) ) {
			act( "The wail tears $N apart!", ch, NULL, vch, TO_CHAR );
			act( "The wail tears you apart!", ch, NULL, vch, TO_VICT );
			raw_kill( vch );
			continue;
		}

		dam = number_range( 300, 600 ) + ch->rage * 4;
		damage( ch, vch, dam, gsn_punch );
	}
	return;
}

/*
 * Soulrend - spirit damage bypassing physical resistance
 */
void do_soulrend( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam;

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_DEVASTATION] < 2 ) {
		send_to_char( "You need Devastation level 2 to use soulrend. See #Rvoicetrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.soulrend.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "siren.soulrend.resonance_cost" ) ) {
		send_to_char( "You don't have enough resonance.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, acfg( "siren.soulrend.cooldown" ) );
	ch->mana -= acfg( "siren.soulrend.mana_cost" );
	ch->rage -= acfg( "siren.soulrend.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	dam = number_range( 250, 500 ) + ch->rage * 3;

	act( "You sing a note of pure agony, rending $N's very soul!", ch, NULL, victim, TO_CHAR );
	act( "$n sings a note of pure agony that tears at your soul!", ch, NULL, victim, TO_VICT );
	act( "$n sings a note of pure agony at $N!", ch, NULL, victim, TO_NOTVICT );

	/* Split damage: bypass_pct goes direct to HP, rest through normal damage */
	{
		int bypass_pct = acfg( "siren.soulrend.bypass_pct" );
		int spirit_dam = dam * bypass_pct / 100;
		int normal_dam = dam - spirit_dam;
		damage( ch, victim, normal_dam, gsn_punch );
		if ( victim->position > POS_DEAD ) {
			victim->hit -= spirit_dam;
			if ( victim->hit < -10 ) {
				act( "$N collapses, soul torn asunder!", ch, NULL, victim, TO_CHAR );
				act( "Your soul is torn asunder!", ch, NULL, victim, TO_VICT );
				raw_kill( victim );
			}
		}
	}
	return;
}

/*
 * Crescendo - multi-round building attack with devastating finale
 */
void do_crescendo( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int stage;
	int dam;

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_DEVASTATION] < 3 ) {
		send_to_char( "You need Devastation level 3 to use crescendo. See #Rvoicetrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.crescendo.mana_per_stage" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "siren.crescendo.mana_per_stage" );
	stage = ch->pcdata->powers[DIRGE_CRESCENDO_STAGE];

	if ( stage < 3 ) {
		ch->pcdata->powers[DIRGE_CRESCENDO_STAGE]++;
		stage = ch->pcdata->powers[DIRGE_CRESCENDO_STAGE];

		if ( stage == 1 ) {
			act( "You begin building a crescendo... #Gpiano#n.", ch, NULL, victim, TO_CHAR );
			act( "$n begins humming softly, building toward something...", ch, NULL, victim, TO_ROOM );
		} else if ( stage == 2 ) {
			act( "Your crescendo builds... #Ymezzo forte#n!", ch, NULL, victim, TO_CHAR );
			act( "$n's voice grows louder, the air vibrating!", ch, NULL, victim, TO_ROOM );
			dam = number_range( 50, 100 );
			damage( ch, victim, dam, gsn_punch );
		} else if ( stage == 3 ) {
			act( "Your crescendo surges... #Rforte#n!!", ch, NULL, victim, TO_CHAR );
			act( "$n's voice surges to a powerful volume!", ch, NULL, victim, TO_ROOM );
			dam = number_range( 100, 200 );
			damage( ch, victim, dam, gsn_punch );
		}
		return;
	}

	/* Stage 3 -> Finale: devastating release */
	ch->pcdata->powers[DIRGE_CRESCENDO_STAGE] = 0;
	dam = number_range( 400, 800 ) * acfg( "siren.crescendo.finale_dam_mult" );
	dam += ch->rage * 5;

	/* Consume all resonance for the finale */
	ch->rage = 0;

	act( "#R*** FORTISSIMO! ***#n You unleash your crescendo in a devastating finale at $N!", ch, NULL, victim, TO_CHAR );
	act( "#R*** FORTISSIMO! ***#n $n unleashes a devastating sonic finale upon you!", ch, NULL, victim, TO_VICT );
	act( "#R*** FORTISSIMO! ***#n $n unleashes a devastating sonic finale at $N!", ch, NULL, victim, TO_NOTVICT );

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Cacophony - area madness/confusion + damage
 */
void do_cacophony( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_UNMAKING] < 2 ) {
		send_to_char( "You need Unmaking level 2 to use cacophony. See #Rvoicetrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.cacophony.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "siren.cacophony.resonance_cost" ) ) {
		send_to_char( "You don't have enough resonance.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, acfg( "siren.cacophony.cooldown" ) );
	ch->mana -= acfg( "siren.cacophony.mana_cost" );
	ch->rage -= acfg( "siren.cacophony.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	act( "You unleash a cacophony of maddening dissonance!", ch, NULL, NULL, TO_CHAR );
	act( "$n unleashes a cacophony of maddening dissonance!", ch, NULL, NULL, TO_ROOM );

	for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
		vch_next = vch->next_in_room;
		if ( vch == ch ) continue;
		if ( is_safe( ch, vch ) ) continue;
		if ( !IS_NPC( vch ) && !IS_NPC( ch ) && vch->fighting != ch ) continue;
		if ( IS_NPC( vch ) && vch->fighting != ch ) continue;

		dam = number_range( 150, 350 ) + ch->rage * 3;
		damage( ch, vch, dam, gsn_punch );

		/* Confusion effect - random flee chance */
		if ( number_percent() < 20 && IS_NPC( vch ) ) {
			act( "$N staggers about in confusion!", ch, NULL, vch, TO_CHAR );
			do_flee( vch, "" );
		}
	}
	return;
}

/*
 * Enthrall - charm target to fight for you temporarily
 */
void do_enthrall( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_DOMINATION] < 1 ) {
		send_to_char( "You need Domination level 1 to use enthrall. See #Rvoicetrain#n.\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "Enthrall whom?\n\r", ch );
		return;
	}
	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}
	if ( victim == ch ) {
		send_to_char( "You cannot enthrall yourself.\n\r", ch );
		return;
	}
	if ( !IS_NPC( victim ) ) {
		send_to_char( "You can only enthrall NPCs.\n\r", ch );
		return;
	}
	if ( IS_AFFECTED( victim, AFF_CHARM ) ) {
		send_to_char( "They are already charmed.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.enthrall.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "siren.enthrall.resonance_cost" ) ) {
		send_to_char( "You don't have enough resonance.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_ECHOSHIELD_ACTIVE] != 0 ) {
		/* Reuse a field to count enthralled - or just check followers */
	}

	ch->mana -= acfg( "siren.enthrall.mana_cost" );
	ch->rage -= acfg( "siren.enthrall.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	if ( victim->fighting != NULL ) stop_fighting( victim, TRUE );

	SET_BIT( victim->affected_by, AFF_CHARM );
	victim->master = ch;
	victim->leader = ch;

	act( "You weave an irresistible melody, enthralling $N to your will!", ch, NULL, victim, TO_CHAR );
	act( "$n weaves an irresistible melody, and $N becomes enthralled!", ch, NULL, victim, TO_NOTVICT );
	act( "$n's beautiful voice fills your mind... you must obey.", ch, NULL, victim, TO_VICT );
	return;
}

/*
 * Siren Song - mass charm/pacification of a room
 */
void do_sirensong( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_DOMINATION] < 2 ) {
		send_to_char( "You need Domination level 2 to use siren song. See #Rvoicetrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.sirensong.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "siren.sirensong.resonance_cost" ) ) {
		send_to_char( "You don't have enough resonance.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, acfg( "siren.sirensong.cooldown" ) );
	ch->mana -= acfg( "siren.sirensong.mana_cost" );
	ch->rage -= acfg( "siren.sirensong.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	act( "You sing an enchanting siren song that fills the room!", ch, NULL, NULL, TO_CHAR );
	act( "$n sings an enchanting siren song that fills the room!", ch, NULL, NULL, TO_ROOM );

	for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
		vch_next = vch->next_in_room;
		if ( vch == ch ) continue;
		if ( !IS_NPC( vch ) ) continue; /* Only affects NPCs */

		if ( vch->fighting != NULL ) {
			stop_fighting( vch, TRUE );
			act( "$N stops fighting, mesmerized by the song.", ch, NULL, vch, TO_CHAR );
		}
	}
	return;
}

/*
 * Command Voice - force a target to perform an action
 */
void do_commandvoice( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_DOMINATION] < 3 ) {
		send_to_char( "You need Domination level 3 to use command voice. See #Rvoicetrain#n.\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );
	one_argument( argument, arg2 );

	if ( arg[0] == '\0' || arg2[0] == '\0' ) {
		send_to_char( "Syntax: commandvoice <target> <flee|drop>\n\r", ch );
		return;
	}
	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}
	if ( victim == ch ) {
		send_to_char( "You cannot command yourself.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.commandvoice.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "siren.commandvoice.resonance_cost" ) ) {
		send_to_char( "You don't have enough resonance.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, acfg( "siren.commandvoice.cooldown" ) );
	ch->mana -= acfg( "siren.commandvoice.mana_cost" );
	ch->rage -= acfg( "siren.commandvoice.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	act( "You speak with the #x147Command Voice#n, compelling $N!", ch, NULL, victim, TO_CHAR );
	act( "$n speaks with an irresistible voice, compelling $N!", ch, NULL, victim, TO_NOTVICT );

	if ( !str_cmp( arg2, "flee" ) ) {
		act( "$n's voice compels you to flee!", ch, NULL, victim, TO_VICT );
		do_flee( victim, "" );
	} else if ( !str_cmp( arg2, "drop" ) ) {
		OBJ_DATA *obj = get_eq_char( victim, WEAR_WIELD );
		if ( obj != NULL ) {
			act( "$n's voice compels you to drop your weapon!", ch, NULL, victim, TO_VICT );
			obj_from_char( obj );
			obj_to_room( obj, victim->in_room );
			act( "$N drops $p!", ch, obj, victim, TO_CHAR );
		} else {
			send_to_char( "They aren't wielding anything.\n\r", ch );
		}
	} else {
		send_to_char( "You can command them to 'flee' or 'drop'.\n\r", ch );
	}
	return;
}

/*
 * Mesmerize - stun/paralyze through hypnotic melody
 */
void do_mesmerize( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_DOMINATION] < 4 ) {
		send_to_char( "You need Domination level 4 to use mesmerize. See #Rvoicetrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.mesmerize.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "siren.mesmerize.resonance_cost" ) ) {
		send_to_char( "You don't have enough resonance.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, acfg( "siren.mesmerize.cooldown" ) );
	ch->mana -= acfg( "siren.mesmerize.mana_cost" );
	ch->rage -= acfg( "siren.mesmerize.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	act( "You sing a hypnotic melody, mesmerizing $N!", ch, NULL, victim, TO_CHAR );
	act( "$n sings a hypnotic melody, and you are unable to move!", ch, NULL, victim, TO_VICT );
	act( "$n sings a hypnotic melody, mesmerizing $N!", ch, NULL, victim, TO_NOTVICT );

	WAIT_STATE( victim, acfg( "siren.mesmerize.stun_duration" ) * PULSE_VIOLENCE );
	return;
}

/*
 * Echoshield - reflects a portion of damage as sonic
 */
void do_echoshield( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_UNMAKING] < 1 ) {
		send_to_char( "You need Unmaking level 1 to use echoshield. See #Rvoicetrain#n.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[DIRGE_ECHOSHIELD_ACTIVE] > 0 ) {
		send_to_char( "Your echoshield is already active.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.echoshield.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	ch->mana -= acfg( "siren.echoshield.mana_cost" );
	ch->pcdata->powers[DIRGE_ECHOSHIELD_ACTIVE] = acfg( "siren.echoshield.duration" );

	act( "You surround yourself with an #x147echoshield#n of reverberating sound!", ch, NULL, NULL, TO_CHAR );
	act( "$n surrounds $mself with an #x147echoshield#n of reverberating sound!", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Aria of Unmaking - strip buffs/protections from a target
 */
void do_ariaofunmaking( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;
	int stripped = 0;

	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SIREN_TRAIN_UNMAKING] < 3 ) {
		send_to_char( "You need Unmaking level 3 to use aria of unmaking. See #Rvoicetrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "siren.ariaofunmaking.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "siren.ariaofunmaking.resonance_cost" ) ) {
		send_to_char( "You don't have enough resonance.\n\r", ch );
		return;
	}
	WAIT_STATE( ch, acfg( "siren.ariaofunmaking.cooldown" ) );
	ch->mana -= acfg( "siren.ariaofunmaking.mana_cost" );
	ch->rage -= acfg( "siren.ariaofunmaking.resonance_cost" );
	if ( ch->rage < 0 ) ch->rage = 0;

	act( "You sing the #RAria of Unmaking#n, stripping $N's protections!", ch, NULL, victim, TO_CHAR );
	act( "$n sings the #RAria of Unmaking#n, and your protections crumble!", ch, NULL, victim, TO_VICT );
	act( "$n sings the #RAria of Unmaking#n at $N!", ch, NULL, victim, TO_NOTVICT );

	/* Strip magical affects */
	for ( paf = victim->affected; paf != NULL; paf = paf_next ) {
		paf_next = paf->next;
		affect_remove( victim, paf );
		stripped++;
		if ( stripped >= 3 ) break; /* Cap at 3 affects stripped */
	}

	/* PvP resist: players have a chance to resist protection stripping */
	if ( !IS_NPC( victim ) && number_percent() <= acfg( "siren.ariaofunmaking.pvp_resist_pct" ) ) {
		send_to_char( "Your target's willpower resists the unmaking of their protections!\n\r", ch );
	} else {
		if ( IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
			REMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
			act( "$N's sanctuary is shattered!", ch, NULL, victim, TO_CHAR );
			stripped++;
		}
		if ( IS_AFFECTED( victim, AFF_PROTECT ) ) {
			REMOVE_BIT( victim->affected_by, AFF_PROTECT );
			stripped++;
		}
	}

	if ( stripped == 0 )
		send_to_char( "They had no protections to strip.\n\r", ch );
	else {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "You stripped %d protections from your target!\n\r", stripped );
		send_to_char( buf, ch );
	}
	return;
}

/*
 * Voice Training - display and improve Siren training category levels
 */
void do_voicetrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int *path = NULL;
	int max_level = 0;
	int cost;
	const char *path_name = NULL;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SIREN ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "#x039~#x147[#n Voice Training #x147]#x039~#n\n\r", ch );
		sprintf( buf, "  #x147Devastation#n  [%d/3]  Raw sonic destruction\n\r", ch->pcdata->powers[SIREN_TRAIN_DEVASTATION] );
		send_to_char( buf, ch );
		sprintf( buf, "  #x147Domination#n   [%d/4]  Mind control and compulsion\n\r", ch->pcdata->powers[SIREN_TRAIN_DOMINATION] );
		send_to_char( buf, ch );
		sprintf( buf, "  #x147Unmaking#n     [%d/3]  Defense stripping and disruption\n\r", ch->pcdata->powers[SIREN_TRAIN_UNMAKING] );
		send_to_char( buf, ch );
		send_to_char( "\n\rSyntax: voicetrain <devastation|domination|unmaking>\n\r", ch );

		send_to_char( "\n\r#x147Devastation:#n 1:bansheewail  2:soulrend  3:crescendo\n\r", ch );
		send_to_char( "#x147Domination:#n  1:enthrall  2:sirensong  3:commandvoice  4:mesmerize\n\r", ch );
		send_to_char( "#x147Unmaking:#n    1:echoshield  2:cacophony  3:ariaofunmaking\n\r", ch );
		return;
	}

	if ( !str_cmp( arg, "devastation" ) ) {
		path = &ch->pcdata->powers[SIREN_TRAIN_DEVASTATION];
		max_level = 3;
		path_name = "Devastation";
	} else if ( !str_cmp( arg, "domination" ) ) {
		path = &ch->pcdata->powers[SIREN_TRAIN_DOMINATION];
		max_level = 4;
		path_name = "Domination";
	} else if ( !str_cmp( arg, "unmaking" ) ) {
		path = &ch->pcdata->powers[SIREN_TRAIN_UNMAKING];
		max_level = 3;
		path_name = "Unmaking";
	} else {
		send_to_char( "Valid paths: devastation, domination, unmaking\n\r", ch );
		return;
	}

	if ( *path >= max_level ) {
		sprintf( buf, "Your %s training is already at maximum (%d).\n\r", path_name, max_level );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *path + 1 ) * 50;

	if ( ch->practice < cost ) {
		sprintf( buf, "You need %d primal to advance %s to level %d.\n\r", cost, path_name, *path + 1 );
		send_to_char( buf, ch );
		return;
	}

	ch->practice -= cost;
	(*path)++;

	sprintf( buf, "You advance your #x147%s#n training to level %d!\n\r", path_name, *path );
	send_to_char( buf, ch );

	if ( !str_cmp( arg, "devastation" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #Rbansheewail#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #Rsoulrend#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #Rcrescendo#n!\n\r", ch );
	} else if ( !str_cmp( arg, "domination" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #Renthrall#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #Rsirensong#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #Rcommandvoice#n!\n\r", ch );
		else if ( *path == 4 ) send_to_char( "You have learned #Rmesmerize#n!\n\r", ch );
	} else if ( !str_cmp( arg, "unmaking" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #Rechoshield#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #Rcacophony#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #Rariaofunmaking#n!\n\r", ch );
	}
	return;
}

/*
 * Siren armor - creates class-specific equipment
 */
void do_sirenarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_SIREN );
}

/*
 * Tick update for Siren - called from update.c
 */
void update_siren( CHAR_DATA *ch ) {
	/* Resonance build/decay - higher cap for Siren */
	if ( ch->position == POS_FIGHTING ) {
		if ( ch->rage < 150 )
			ch->rage = UMIN( ch->rage + 3, 150 );
	} else if ( ch->rage > 0 ) {
		ch->rage -= 2;
		if ( ch->rage < 0 ) ch->rage = 0;
	}

	/* Track peak resonance */
	if ( ch->rage > ch->pcdata->stats[DIRGE_RESONANCE_PEAK] )
		ch->pcdata->stats[DIRGE_RESONANCE_PEAK] = ch->rage;

	/* Echoshield countdown */
	if ( ch->pcdata->powers[DIRGE_ECHOSHIELD_ACTIVE] > 0 ) {
		ch->pcdata->powers[DIRGE_ECHOSHIELD_ACTIVE]--;
		if ( ch->pcdata->powers[DIRGE_ECHOSHIELD_ACTIVE] <= 0 )
			send_to_char( "Your echoshield fades away.\n\r", ch );
	}

	/* Crescendo timeout - resets if not fighting */
	if ( ch->pcdata->powers[DIRGE_CRESCENDO_STAGE] > 0 && ch->position != POS_FIGHTING ) {
		ch->pcdata->powers[DIRGE_CRESCENDO_STAGE] = 0;
		send_to_char( "Your crescendo fades without a finale.\n\r", ch );
	}
}
