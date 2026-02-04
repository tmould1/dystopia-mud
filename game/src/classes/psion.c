/***************************************************************************
 *  Psion class - mental/psionic combat using Focus resource               *
 *  Mental damage bypasses physical armor, resisted by Intelligence        *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "ability_config.h"
#include "psion.h"

/* Forward declarations for update functions */
void update_psion( CHAR_DATA *ch );

/*
 * Focus status display - shared between Psion and Mindflayer
 */
void do_psifocus( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int focus_cap;

	if ( !IS_CLASS( ch, CLASS_PSION ) && !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	focus_cap = IS_CLASS( ch, CLASS_MINDFLAYER ) ? 150 : 100;

	sprintf( buf, "#x033~#x039[#n Focus Status #x039]#x033~#n\n\r" );
	send_to_char( buf, ch );
	sprintf( buf, "Current Focus: #x039%d#n / %d\n\r", ch->rage, focus_cap );
	send_to_char( buf, ch );

	if ( ch->pcdata->powers[PSION_THOUGHT_SHIELD] > 0 ) {
		sprintf( buf, "#x039[#nThought Shield: #C%d#n HP, #C%d#n ticks remaining#x039]#n\n\r",
			ch->pcdata->stats[PSION_THOUGHT_SHIELD_HP], ch->pcdata->powers[PSION_THOUGHT_SHIELD] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[PSION_KINETIC_BARRIER] > 0 ) {
		sprintf( buf, "#x039[#nKinetic Barrier: #C%d#n HP, #C%d#n ticks remaining#x039]#n\n\r",
			ch->pcdata->stats[PSION_KINETIC_BARRIER_HP], ch->pcdata->powers[PSION_KINETIC_BARRIER] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[PSION_LEVITATE] > 0 ) {
		sprintf( buf, "#x039[#nLevitate: #C%d#n ticks remaining#x039]#n\n\r",
			ch->pcdata->powers[PSION_LEVITATE] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[PSION_MENTAL_LINK] > 0 ) {
		sprintf( buf, "#x039[#nMental Link: #C%d#n ticks remaining#x039]#n\n\r",
			ch->pcdata->powers[PSION_MENTAL_LINK] );
		send_to_char( buf, ch );
	}
	if ( ch->pcdata->powers[PSION_MINDSCAN] > 0 ) {
		sprintf( buf, "#x039[#nMindscan: #C%d#n ticks remaining#x039]#n\n\r",
			ch->pcdata->powers[PSION_MINDSCAN] );
		send_to_char( buf, ch );
	}
	return;
}

/*
 * Meditate - out of combat Focus recovery. Shared between Psion and Mindflayer.
 */
void do_psimeditate( CHAR_DATA *ch, char *argument ) {
	int focus_gain;
	int focus_cap;

	if ( !IS_CLASS( ch, CLASS_PSION ) && !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->fighting != NULL ) {
		send_to_char( "You can't meditate while fighting!\n\r", ch );
		return;
	}
	if ( ch->position < POS_RESTING ) {
		send_to_char( "You need to be at least resting to meditate.\n\r", ch );
		return;
	}

	WAIT_STATE( ch, acfg( "psion.meditate.cooldown" ) );

	focus_gain = IS_CLASS( ch, CLASS_MINDFLAYER ) ? 15 : 10;
	focus_cap = IS_CLASS( ch, CLASS_MINDFLAYER ) ? 150 : 100;

	ch->rage = UMIN( ch->rage + focus_gain, focus_cap );

	send_to_char( "You enter a meditative state, gathering your mental energies.\n\r", ch );
	act( "$n enters a meditative trance.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Psion Training command
 */
void do_psitrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int *path = NULL;
	int max_level = 0;
	int cost;
	const char *path_name = "";

	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "#x033~#x039[#n Psion Training #x039]#x033~#n\n\r", ch );
		sprintf( buf, "Telepathy:      %d/3  (#x039mindscan#n, #x039thoughtshield#n, #x039mentallink#n)\n\r",
			ch->pcdata->powers[PSION_TRAIN_TELEPATHY] );
		send_to_char( buf, ch );
		sprintf( buf, "Telekinesis:    %d/3  (#x039forcepush#n, #x039levitate#n, #x039kineticbarrier#n)\n\r",
			ch->pcdata->powers[PSION_TRAIN_TELEKINESIS] );
		send_to_char( buf, ch );
		sprintf( buf, "Psychic Combat: %d/3  (#x039mindspike#n, #x039psychicscream#n, #x039brainburn#n)\n\r",
			ch->pcdata->powers[PSION_TRAIN_COMBAT] );
		send_to_char( buf, ch );
		send_to_char( "\n\rSyntax: psitrain <category>\n\r", ch );
		send_to_char( "Cost: (current_level + 1) * 40 primal\n\r", ch );
		return;
	}

	if ( !str_cmp( arg, "telepathy" ) ) {
		path = &ch->pcdata->powers[PSION_TRAIN_TELEPATHY];
		max_level = 3;
		path_name = "Telepathy";
	}
	else if ( !str_cmp( arg, "telekinesis" ) ) {
		path = &ch->pcdata->powers[PSION_TRAIN_TELEKINESIS];
		max_level = 3;
		path_name = "Telekinesis";
	}
	else if ( !str_cmp( arg, "psychic" ) || !str_cmp( arg, "combat" ) || !str_cmp( arg, "psychiccombat" ) ) {
		path = &ch->pcdata->powers[PSION_TRAIN_COMBAT];
		max_level = 3;
		path_name = "Psychic Combat";
	}
	else {
		send_to_char( "Valid training paths: telepathy, telekinesis, psychic\n\r", ch );
		return;
	}

	if ( *path >= max_level ) {
		sprintf( buf, "You have already mastered %s.\n\r", path_name );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *path + 1 ) * 40;

	if ( ch->practice < cost ) {
		sprintf( buf, "You need %d primal to advance %s.\n\r", cost, path_name );
		send_to_char( buf, ch );
		return;
	}

	ch->practice -= cost;
	(*path)++;

	sprintf( buf, "You have advanced your %s training to level %d!\n\r", path_name, *path );
	send_to_char( buf, ch );

	/* Announce unlocks */
	if ( !str_cmp( arg, "telepathy" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #x039mindscan#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #x039thoughtshield#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #x039mentallink#n!\n\r", ch );
	}
	else if ( !str_cmp( arg, "telekinesis" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #x039forcepush#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #x039levitate#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #x039kineticbarrier#n!\n\r", ch );
	}
	else {
		if ( *path == 1 ) send_to_char( "You have learned #x039mindspike#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #x039psychicscream#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #x039brainburn#n!\n\r", ch );
	}
	return;
}

/*
 * Mindscan - Telepathy 1: Detection and surface thought reading
 */
void do_mindscan( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_TELEPATHY] < 1 ) {
		send_to_char( "You haven't trained Telepathy yet. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.mindscan.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.mindscan.focus_req" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	ch->mana -= acfg( "psion.mindscan.mana_cost" );
	WAIT_STATE( ch, 4 );

	/* If no argument, grant detection aura */
	if ( arg[0] == '\0' ) {
		ch->pcdata->powers[PSION_MINDSCAN] = acfg( "psion.mindscan.duration" );
		SET_BIT( ch->affected_by, AFF_DETECT_HIDDEN );
		SET_BIT( ch->affected_by, AFF_DETECT_INVIS );
		send_to_char( "You extend your psychic senses, detecting hidden presences.\n\r", ch );
		act( "$n's eyes glow faintly with psychic energy.", ch, NULL, NULL, TO_ROOM );
		return;
	}

	/* Target specified - read surface thoughts */
	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	act( "You probe $N's surface thoughts.", ch, NULL, victim, TO_CHAR );
	act( "You feel a strange tingling in your mind as $n probes your thoughts.", ch, NULL, victim, TO_VICT );

	sprintf( buf, "$N's status: %d%% HP, %d%% Mana",
		( victim->hit * 100 ) / UMAX( 1, victim->max_hit ),
		( victim->mana * 100 ) / UMAX( 1, victim->max_mana ) );
	act( buf, ch, NULL, victim, TO_CHAR );

	if ( victim->fighting != NULL ) {
		sprintf( buf, "$N is fighting %s.", victim->fighting->name );
		act( buf, ch, NULL, victim, TO_CHAR );
	}
	return;
}

/*
 * Thought Shield - Telepathy 2: Mental damage absorption barrier
 */
void do_thoughtshield( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_TELEPATHY] < 2 ) {
		send_to_char( "You need Telepathy level 2. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.thoughtshield.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_THOUGHT_SHIELD] > 0 ) {
		send_to_char( "Your Thought Shield is already active.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_KINETIC_BARRIER] > 0 ) {
		send_to_char( "You cannot have both shields active. Deactivate Kinetic Barrier first.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "psion.thoughtshield.mana_cost" );
	WAIT_STATE( ch, 4 );

	ch->pcdata->powers[PSION_THOUGHT_SHIELD] = acfg( "psion.thoughtshield.duration" );
	ch->pcdata->stats[PSION_THOUGHT_SHIELD_HP] = acfg( "psion.thoughtshield.absorb_amount" );

	send_to_char( "You erect a psychic barrier around your mind.\n\r", ch );
	act( "A faint shimmer surrounds $n's head.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Mental Link - Telepathy 3: Telepathic bond with an ally
 */
void do_mentallink( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_TELEPATHY] < 3 ) {
		send_to_char( "You need Telepathy level 3. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.mentallink.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.mentallink.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "Syntax: mentallink <player>\n\r", ch );
		return;
	}

	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}
	if ( IS_NPC( victim ) ) {
		send_to_char( "You can only link with players.\n\r", ch );
		return;
	}
	if ( victim == ch ) {
		send_to_char( "You cannot link with yourself.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "psion.mentallink.mana_cost" );
	ch->rage -= acfg( "psion.mentallink.focus_cost" );
	WAIT_STATE( ch, 4 );

	ch->pcdata->powers[PSION_MENTAL_LINK] = acfg( "psion.mentallink.duration" );
	ch->pcdata->powers[PSION_LINKED_PLAYER] = 1; /* Just track that link is active */

	act( "You establish a telepathic link with $N.", ch, NULL, victim, TO_CHAR );
	act( "$n establishes a telepathic link with you.", ch, NULL, victim, TO_VICT );
	act( "$n and $N share a moment of psychic connection.", ch, NULL, victim, TO_NOTVICT );
	return;
}

/*
 * Force Push - Telekinesis 1: Knockback attack with disarm chance
 */
void do_forcepush( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam;
	int focus_cap;

	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_TELEKINESIS] < 1 ) {
		send_to_char( "You haven't trained Telekinesis yet. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.forcepush.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.forcepush.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "psion.forcepush.mana_cost" );
	ch->rage -= acfg( "psion.forcepush.focus_cost" );
	WAIT_STATE( ch, acfg( "psion.forcepush.cooldown" ) );

	dam = number_range( 80, 160 );
	dam += ch->rage * 2;

	act( "You unleash a telekinetic blast at $N!", ch, NULL, victim, TO_CHAR );
	act( "$n unleashes a telekinetic blast at you!", ch, NULL, victim, TO_VICT );
	act( "$n unleashes a telekinetic blast at $N!", ch, NULL, victim, TO_NOTVICT );

	/* Knockback chance */
	if ( number_percent() < acfg( "psion.forcepush.knockback_chance" ) ) {
		act( "$N is knocked back by the force!", ch, NULL, victim, TO_CHAR );
		act( "You are knocked back by the telekinetic force!", ch, NULL, victim, TO_VICT );
		/* Could add flee effect here */
	}

	/* Disarm chance */
	if ( number_percent() < acfg( "psion.forcepush.disarm_chance" ) ) {
		do_disarm( ch, "" );
	}

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Levitate - Telekinesis 2: Self-flight with evasion bonus
 */
void do_levitate( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_TELEKINESIS] < 2 ) {
		send_to_char( "You need Telekinesis level 2. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.levitate.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.levitate.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_LEVITATE] > 0 ) {
		send_to_char( "You are already levitating.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "psion.levitate.mana_cost" );
	ch->rage -= acfg( "psion.levitate.focus_cost" );
	WAIT_STATE( ch, 4 );

	ch->pcdata->powers[PSION_LEVITATE] = acfg( "psion.levitate.duration" );
	SET_BIT( ch->affected_by, AFF_FLYING );

	send_to_char( "You lift yourself off the ground with telekinetic power.\n\r", ch );
	act( "$n rises into the air, suspended by psychic energy.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Kinetic Barrier - Telekinesis 3: Physical damage absorption shield
 */
void do_kineticbarrier( CHAR_DATA *ch, char *argument ) {
	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_TELEKINESIS] < 3 ) {
		send_to_char( "You need Telekinesis level 3. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.kineticbarrier.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.kineticbarrier.focus_req" ) ) {
		send_to_char( "You need at least %d Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.kineticbarrier.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_KINETIC_BARRIER] > 0 ) {
		send_to_char( "Your Kinetic Barrier is already active.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_THOUGHT_SHIELD] > 0 ) {
		send_to_char( "You cannot have both shields active. Deactivate Thought Shield first.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "psion.kineticbarrier.mana_cost" );
	ch->rage -= acfg( "psion.kineticbarrier.focus_cost" );
	WAIT_STATE( ch, 4 );

	ch->pcdata->powers[PSION_KINETIC_BARRIER] = acfg( "psion.kineticbarrier.duration" );
	ch->pcdata->stats[PSION_KINETIC_BARRIER_HP] = acfg( "psion.kineticbarrier.absorb_amount" );

	send_to_char( "You form a telekinetic barrier around yourself.\n\r", ch );
	act( "A shimmering force field surrounds $n.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Mind Spike - Psychic Combat 1: Basic mental damage attack
 */
void do_mindspike( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam;
	int focus_cap;

	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_COMBAT] < 1 ) {
		send_to_char( "You haven't trained Psychic Combat yet. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.mindspike.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "psion.mindspike.mana_cost" );
	WAIT_STATE( ch, acfg( "psion.mindspike.cooldown" ) );

	dam = number_range( 100, 200 );
	dam += ch->rage * 2;

	/* Mental damage - reduce by target INT */
	dam = dam * ( 100 - UMIN( get_curr_int( victim ) / 10, 50 ) ) / 100;

	act( "You drive a spike of psychic energy into $N's mind!", ch, NULL, victim, TO_CHAR );
	act( "A spike of psychic pain tears through your mind!", ch, NULL, victim, TO_VICT );
	act( "$N winces in psychic agony from $n's mental attack!", ch, NULL, victim, TO_NOTVICT );

	/* Gain focus */
	focus_cap = IS_CLASS( ch, CLASS_MINDFLAYER ) ? 150 : 100;
	ch->rage = UMIN( ch->rage + acfg( "psion.mindspike.focus_gain" ), focus_cap );

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Psychic Scream - Psychic Combat 2: AoE mental damage
 */
void do_psychicscream( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	CHAR_DATA *victim_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_COMBAT] < 2 ) {
		send_to_char( "You need Psychic Combat level 2. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.psychicscream.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.psychicscream.focus_req" ) ) {
		send_to_char( "You need more Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.psychicscream.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "psion.psychicscream.mana_cost" );
	ch->rage -= acfg( "psion.psychicscream.focus_cost" );
	WAIT_STATE( ch, acfg( "psion.psychicscream.cooldown" ) );

	act( "You unleash a devastating psychic scream!", ch, NULL, NULL, TO_CHAR );
	act( "$n unleashes a devastating psychic scream!", ch, NULL, NULL, TO_ROOM );

	for ( victim = ch->in_room->people; victim != NULL; victim = victim_next ) {
		victim_next = victim->next_in_room;

		if ( victim == ch ) continue;
		if ( is_same_group( ch, victim ) ) continue;
		if ( !IS_NPC( victim ) && !IS_NPC( ch ) && victim->fighting != ch ) continue;

		dam = number_range( 120, 240 );
		dam += ch->rage * 2;

		/* Mental damage - reduce by target INT */
		dam = dam * ( 100 - UMIN( get_curr_int( victim ) / 10, 50 ) ) / 100;

		damage( ch, victim, dam, gsn_punch );
	}
	return;
}

/*
 * Brain Burn - Psychic Combat 3: Heavy mental strike with stun chance
 */
void do_brainburn( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam;

	if ( !IS_CLASS( ch, CLASS_PSION ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[PSION_TRAIN_COMBAT] < 3 ) {
		send_to_char( "You need Psychic Combat level 3. See #x039psitrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "psion.brainburn.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.brainburn.focus_req" ) ) {
		send_to_char( "You need more Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "psion.brainburn.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "psion.brainburn.mana_cost" );
	ch->rage -= acfg( "psion.brainburn.focus_cost" );
	WAIT_STATE( ch, acfg( "psion.brainburn.cooldown" ) );

	dam = number_range( 300, 500 );
	dam += ch->rage * 4;

	/* Mental damage - reduce by target INT */
	dam = dam * ( 100 - UMIN( get_curr_int( victim ) / 10, 50 ) ) / 100;

	act( "You overload $N's mind with psychic fire!", ch, NULL, victim, TO_CHAR );
	act( "Your mind erupts in psychic flames as $n attacks!", ch, NULL, victim, TO_VICT );
	act( "$N screams as $n unleashes psychic fire into their mind!", ch, NULL, victim, TO_NOTVICT );

	/* Stun chance */
	if ( number_percent() < acfg( "psion.brainburn.stun_chance" ) ) {
		act( "$N's mind is overwhelmed - they are stunned!", ch, NULL, victim, TO_CHAR );
		act( "Your mind is overwhelmed - you are stunned!", ch, NULL, victim, TO_VICT );
		WAIT_STATE( victim, 24 ); /* 2 rounds */
	}

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Psion Armor creation command
 */
void do_psionarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_PSION );
}

/*
 * Psion tick update function
 */
void update_psion( CHAR_DATA *ch ) {
	int focus_cap;

	if ( !IS_CLASS( ch, CLASS_PSION ) ) return;

	focus_cap = 100;

	/* Focus build/decay */
	if ( ch->position == POS_FIGHTING ) {
		/* +1 per tick in combat */
		if ( ch->rage < focus_cap )
			ch->rage = UMIN( ch->rage + 1, focus_cap );
	} else if ( ch->rage > 0 ) {
		/* -2 per tick out of combat */
		ch->rage -= 2;
		if ( ch->rage < 0 ) ch->rage = 0;
	}

	/* Track peak focus */
	if ( ch->rage > ch->pcdata->stats[PSION_PEAK_FOCUS] )
		ch->pcdata->stats[PSION_PEAK_FOCUS] = ch->rage;

	/* Duration countdowns */
	if ( ch->pcdata->powers[PSION_THOUGHT_SHIELD] > 0 ) {
		ch->pcdata->powers[PSION_THOUGHT_SHIELD]--;
		if ( ch->pcdata->powers[PSION_THOUGHT_SHIELD] <= 0 ) {
			ch->pcdata->stats[PSION_THOUGHT_SHIELD_HP] = 0;
			send_to_char( "Your Thought Shield fades away.\n\r", ch );
		}
	}

	if ( ch->pcdata->powers[PSION_KINETIC_BARRIER] > 0 ) {
		ch->pcdata->powers[PSION_KINETIC_BARRIER]--;
		if ( ch->pcdata->powers[PSION_KINETIC_BARRIER] <= 0 ) {
			ch->pcdata->stats[PSION_KINETIC_BARRIER_HP] = 0;
			send_to_char( "Your Kinetic Barrier dissipates.\n\r", ch );
		}
	}

	if ( ch->pcdata->powers[PSION_LEVITATE] > 0 ) {
		ch->pcdata->powers[PSION_LEVITATE]--;
		if ( ch->pcdata->powers[PSION_LEVITATE] <= 0 ) {
			REMOVE_BIT( ch->affected_by, AFF_FLYING );
			send_to_char( "You slowly descend as your telekinetic lift fades.\n\r", ch );
		}
	}

	if ( ch->pcdata->powers[PSION_MENTAL_LINK] > 0 ) {
		ch->pcdata->powers[PSION_MENTAL_LINK]--;
		if ( ch->pcdata->powers[PSION_MENTAL_LINK] <= 0 ) {
			ch->pcdata->powers[PSION_LINKED_PLAYER] = 0;
			send_to_char( "Your mental link fades.\n\r", ch );
		}
	}

	if ( ch->pcdata->powers[PSION_MINDSCAN] > 0 ) {
		ch->pcdata->powers[PSION_MINDSCAN]--;
		if ( ch->pcdata->powers[PSION_MINDSCAN] <= 0 ) {
			REMOVE_BIT( ch->affected_by, AFF_DETECT_HIDDEN );
			REMOVE_BIT( ch->affected_by, AFF_DETECT_INVIS );
			send_to_char( "Your psychic awareness fades.\n\r", ch );
		}
	}

	return;
}
