/***************************************************************************
 *  Mindflayer class - upgrade from Psion with enhanced Focus and thralls  *
 *  Domination, Consumption, and Psionic Storm abilities                   *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "ability_config.h"
#include "psion.h"

/* Forward declarations for update functions */
void update_mindflayer( CHAR_DATA *ch );

/*
 * Mindflayer Training command
 */
void do_mindtrain( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int *path = NULL;
	int max_level = 0;
	int cost;
	const char *path_name = "";

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "#x029~#x035[#n Mindflayer Training #x035]#x029~#n\n\r", ch );
		sprintf( buf, "Domination:    %d/4  (#x035enthral#n, #x035puppet#n, #x035hivemind#n, #x035massdomination#n)\n\r",
			ch->pcdata->powers[MIND_TRAIN_DOMINATION] );
		send_to_char( buf, ch );
		sprintf( buf, "Consumption:   %d/3  (#x035mindfeed#n, #x035memorydrain#n, #x035intellectdevour#n)\n\r",
			ch->pcdata->powers[MIND_TRAIN_CONSUMPTION] );
		send_to_char( buf, ch );
		sprintf( buf, "Psionic Storm: %d/3  (#x035psychicmaelstrom#n, #x035mindblast#n, #x035realityfracture#n)\n\r",
			ch->pcdata->powers[MIND_TRAIN_PSIONIC] );
		send_to_char( buf, ch );
		send_to_char( "\n\rSyntax: mindtrain <category>\n\r", ch );
		send_to_char( "Cost: (current_level + 1) * 50 primal\n\r", ch );
		return;
	}

	if ( !str_cmp( arg, "domination" ) ) {
		path = &ch->pcdata->powers[MIND_TRAIN_DOMINATION];
		max_level = 4;
		path_name = "Domination";
	}
	else if ( !str_cmp( arg, "consumption" ) ) {
		path = &ch->pcdata->powers[MIND_TRAIN_CONSUMPTION];
		max_level = 3;
		path_name = "Consumption";
	}
	else if ( !str_cmp( arg, "psionic" ) || !str_cmp( arg, "storm" ) || !str_cmp( arg, "psionicstorm" ) ) {
		path = &ch->pcdata->powers[MIND_TRAIN_PSIONIC];
		max_level = 3;
		path_name = "Psionic Storm";
	}
	else {
		send_to_char( "Valid training paths: domination, consumption, psionic\n\r", ch );
		return;
	}

	if ( *path >= max_level ) {
		sprintf( buf, "You have already mastered %s.\n\r", path_name );
		send_to_char( buf, ch );
		return;
	}

	cost = ( *path + 1 ) * 50;

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
	if ( !str_cmp( arg, "domination" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #x035enthral#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #x035puppet#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #x035hivemind#n!\n\r", ch );
		else if ( *path == 4 ) send_to_char( "You have learned #x035massdomination#n!\n\r", ch );
	}
	else if ( !str_cmp( arg, "consumption" ) ) {
		if ( *path == 1 ) send_to_char( "You have learned #x035mindfeed#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #x035memorydrain#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #x035intellectdevour#n!\n\r", ch );
	}
	else {
		if ( *path == 1 ) send_to_char( "You have learned #x035psychicmaelstrom#n!\n\r", ch );
		else if ( *path == 2 ) send_to_char( "You have learned #x035mindblast#n!\n\r", ch );
		else if ( *path == 3 ) send_to_char( "You have learned #x035realityfracture#n!\n\r", ch );
	}
	return;
}

/*
 * Helper: Count current thralls
 */
int count_thralls( CHAR_DATA *ch ) {
	CHAR_DATA *fch;
	int count = 0;

	for ( fch = char_list; fch != NULL; fch = fch->next ) {
		if ( IS_NPC( fch ) && IS_AFFECTED( fch, AFF_CHARM ) && fch->master == ch )
			count++;
	}
	return count;
}

/*
 * Enthral - Domination 1: Charm an NPC to fight for you
 */
void do_enthral( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	int thrall_count;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_DOMINATION] < 1 ) {
		send_to_char( "You haven't trained Domination yet. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.enthral.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.enthral.focus_req" ) ) {
		send_to_char( "You need more Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.enthral.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "Enthral whom?\n\r", ch );
		return;
	}

	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( !IS_NPC( victim ) ) {
		send_to_char( "You can only enthral NPCs.\n\r", ch );
		return;
	}

	if ( IS_AFFECTED( victim, AFF_CHARM ) ) {
		send_to_char( "They are already under someone's control.\n\r", ch );
		return;
	}

	thrall_count = count_thralls( ch );
	if ( thrall_count >= acfg( "mindflayer.enthral.max_thralls" ) ) {
		send_to_char( "You already have the maximum number of thralls.\n\r", ch );
		return;
	}

	/* Level-based resistance */
	if ( victim->level > ch->level + 10 && number_percent() < 50 ) {
		act( "$N resists your mental domination!", ch, NULL, victim, TO_CHAR );
		act( "You resist $n's attempt to dominate your mind!", ch, NULL, victim, TO_VICT );
		ch->mana -= acfg( "mindflayer.enthral.mana_cost" ) / 2;
		WAIT_STATE( ch, 12 );
		return;
	}

	ch->mana -= acfg( "mindflayer.enthral.mana_cost" );
	ch->rage -= acfg( "mindflayer.enthral.focus_cost" );
	WAIT_STATE( ch, 8 );

	/* Apply charm */
	SET_BIT( victim->affected_by, AFF_CHARM );
	victim->master = ch;
	victim->leader = ch;

	if ( victim->fighting != NULL )
		stop_fighting( victim, TRUE );

	act( "You dominate $N's mind, bending them to your will.", ch, NULL, victim, TO_CHAR );
	act( "$n dominates your mind! You must serve...", ch, NULL, victim, TO_VICT );
	act( "$N's eyes glaze over as $n dominates their mind.", ch, NULL, victim, TO_NOTVICT );

	ch->pcdata->powers[MIND_THRALL_COUNT] = count_thralls( ch );
	return;
}

/*
 * Puppet - Domination 2: Direct control of a thrall
 */
void do_puppet( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_DOMINATION] < 2 ) {
		send_to_char( "You need Domination level 2. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.puppet.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.puppet.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg1 );
	strcpy( arg2, argument );

	if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
		send_to_char( "Syntax: puppet <thrall> <command>\n\r", ch );
		return;
	}

	if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
		send_to_char( "That thrall isn't here.\n\r", ch );
		return;
	}

	if ( !IS_NPC( victim ) || victim->master != ch ) {
		send_to_char( "That is not your thrall.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.puppet.mana_cost" );
	ch->rage -= acfg( "mindflayer.puppet.focus_cost" );

	act( "You puppet $N's actions.", ch, NULL, victim, TO_CHAR );
	interpret( victim, arg2 );
	return;
}

/*
 * Hivemind - Domination 3: Link thralls for coordinated attacks
 */
void do_hivemind( CHAR_DATA *ch, char *argument ) {
	int thrall_count;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_DOMINATION] < 3 ) {
		send_to_char( "You need Domination level 3. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.hivemind.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.hivemind.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	thrall_count = count_thralls( ch );
	if ( thrall_count < 2 ) {
		send_to_char( "You need at least 2 thralls to form a hivemind.\n\r", ch );
		return;
	}

	if ( ch->pcdata->powers[MIND_HIVEMIND] > 0 ) {
		send_to_char( "Your hivemind is already active.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.hivemind.mana_cost" );
	ch->rage -= acfg( "mindflayer.hivemind.focus_cost" );
	WAIT_STATE( ch, 8 );

	ch->pcdata->powers[MIND_HIVEMIND] = acfg( "mindflayer.hivemind.duration" );

	send_to_char( "You link your thralls into a unified hivemind.\n\r", ch );
	act( "$n's eyes glow as a psychic network forms between their thralls.", ch, NULL, NULL, TO_ROOM );
	return;
}

/*
 * Mass Domination - Domination 4: AoE charm attempt
 */
void do_massdomination( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	CHAR_DATA *victim_next;
	int thrall_count;
	int max_thralls;
	int charmed = 0;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_DOMINATION] < 4 ) {
		send_to_char( "You need Domination level 4. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.massdomination.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.massdomination.focus_req" ) ) {
		send_to_char( "You need more Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.massdomination.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	thrall_count = count_thralls( ch );
	max_thralls = acfg( "mindflayer.enthral.max_thralls" );

	if ( thrall_count >= max_thralls ) {
		send_to_char( "You already have the maximum number of thralls.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.massdomination.mana_cost" );
	ch->rage -= acfg( "mindflayer.massdomination.focus_cost" );
	WAIT_STATE( ch, acfg( "mindflayer.massdomination.cooldown" ) );

	act( "You unleash a wave of psychic domination!", ch, NULL, NULL, TO_CHAR );
	act( "$n unleashes a wave of psychic domination!", ch, NULL, NULL, TO_ROOM );

	for ( victim = ch->in_room->people; victim != NULL; victim = victim_next ) {
		victim_next = victim->next_in_room;

		if ( victim == ch ) continue;
		if ( !IS_NPC( victim ) ) continue;
		if ( IS_AFFECTED( victim, AFF_CHARM ) ) continue;
		if ( thrall_count >= max_thralls ) break;

		if ( number_percent() < acfg( "mindflayer.massdomination.success_rate" ) ) {
			SET_BIT( victim->affected_by, AFF_CHARM );
			victim->master = ch;
			victim->leader = ch;
			if ( victim->fighting != NULL )
				stop_fighting( victim, TRUE );
			act( "$N succumbs to your domination!", ch, NULL, victim, TO_CHAR );
			thrall_count++;
			charmed++;
		}
	}

	if ( charmed == 0 ) {
		send_to_char( "No one succumbed to your mental assault.\n\r", ch );
	}

	ch->pcdata->powers[MIND_THRALL_COUNT] = count_thralls( ch );
	return;
}

/*
 * Mind Feed - Consumption 1: Drain mental energy, heal self
 */
void do_mindfeed( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam;
	int heal;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_CONSUMPTION] < 1 ) {
		send_to_char( "You haven't trained Consumption yet. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.mindfeed.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.mindfeed.mana_cost" );
	WAIT_STATE( ch, acfg( "mindflayer.mindfeed.cooldown" ) );

	dam = number_range( 150, 300 );
	dam += ch->rage * 2;

	/* Mental damage - reduce by target INT */
	dam = dam * ( 100 - UMIN( get_curr_int( victim ) / 10, 50 ) ) / 100;

	/* Heal based on damage */
	heal = dam * acfg( "mindflayer.mindfeed.heal_pct" ) / 100;
	ch->hit = UMIN( ch->hit + heal, ch->max_hit );

	/* Gain focus */
	ch->rage = UMIN( ch->rage + acfg( "mindflayer.mindfeed.focus_gain" ), 150 );

	act( "You sink psychic tendrils into $N's mind, draining their essence!", ch, NULL, victim, TO_CHAR );
	act( "Psychic tendrils tear into your mind as $n feeds on your essence!", ch, NULL, victim, TO_VICT );
	act( "$n drains psychic energy from $N!", ch, NULL, victim, TO_NOTVICT );

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Memory Drain - Consumption 2: Strip buffs and seal abilities
 */
void do_memorydrain( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_CONSUMPTION] < 2 ) {
		send_to_char( "You need Consumption level 2. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.memorydrain.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.memorydrain.focus_req" ) ) {
		send_to_char( "You need more Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.memorydrain.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.memorydrain.mana_cost" );
	ch->rage -= acfg( "mindflayer.memorydrain.focus_cost" );
	WAIT_STATE( ch, acfg( "mindflayer.memorydrain.cooldown" ) );

	act( "You tear through $N's memories, shredding their mental defenses!", ch, NULL, victim, TO_CHAR );
	act( "$n tears through your memories! Your mind feels violated!", ch, NULL, victim, TO_VICT );
	act( "$N staggers as $n rips through their memories!", ch, NULL, victim, TO_NOTVICT );

	/* Strip sanctuary if present */
	if ( IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
		REMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
		act( "$N's sanctuary is stripped away!", ch, NULL, victim, TO_CHAR );
	}

	/* Could add more buff stripping here */
	return;
}

/*
 * Intellect Devour - Consumption 3: Massive damage + max HP reduction
 */
void do_intellectdevour( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	int dam;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_CONSUMPTION] < 3 ) {
		send_to_char( "You need Consumption level 3. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.intellectdevour.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.intellectdevour.focus_req" ) ) {
		send_to_char( "You need more Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.intellectdevour.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.intellectdevour.mana_cost" );
	ch->rage -= acfg( "mindflayer.intellectdevour.focus_cost" );
	WAIT_STATE( ch, acfg( "mindflayer.intellectdevour.cooldown" ) );

	dam = number_range( 400, 700 );
	dam += ch->rage * 5;

	/* Mental damage - reduce by target INT */
	dam = dam * ( 100 - UMIN( get_curr_int( victim ) / 10, 50 ) ) / 100;

	act( "You devour $N's intellect, consuming their very mind!", ch, NULL, victim, TO_CHAR );
	act( "Your mind is being devoured! Agony beyond comprehension!", ch, NULL, victim, TO_VICT );
	act( "$N screams as $n devours their intellect!", ch, NULL, victim, TO_NOTVICT );

	/* Max HP reduction for NPCs only */
	if ( IS_NPC( victim ) ) {
		int reduction = victim->max_hit * acfg( "mindflayer.intellectdevour.maxhp_reduction" ) / 100;
		victim->max_hit -= reduction;
		if ( victim->hit > victim->max_hit )
			victim->hit = victim->max_hit;
	}

	damage( ch, victim, dam, gsn_punch );
	return;
}

/*
 * Psychic Maelstrom - Psionic Storm 1: AoE mental damage with confusion
 */
void do_psychicmaelstrom( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	CHAR_DATA *victim_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_PSIONIC] < 1 ) {
		send_to_char( "You haven't trained Psionic Storm yet. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.psychicmaelstrom.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.psychicmaelstrom.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.psychicmaelstrom.mana_cost" );
	ch->rage -= acfg( "mindflayer.psychicmaelstrom.focus_cost" );
	WAIT_STATE( ch, acfg( "mindflayer.psychicmaelstrom.cooldown" ) );

	act( "You unleash a psychic maelstrom!", ch, NULL, NULL, TO_CHAR );
	act( "$n unleashes a psychic maelstrom!", ch, NULL, NULL, TO_ROOM );

	for ( victim = ch->in_room->people; victim != NULL; victim = victim_next ) {
		victim_next = victim->next_in_room;

		if ( victim == ch ) continue;
		if ( is_same_group( ch, victim ) ) continue;
		if ( IS_AFFECTED( victim, AFF_CHARM ) && victim->master == ch ) continue;
		if ( !IS_NPC( victim ) && !IS_NPC( ch ) && victim->fighting != ch ) continue;

		dam = number_range( 180, 320 );
		dam += ch->rage * 3;

		/* Mental damage - reduce by target INT */
		dam = dam * ( 100 - UMIN( get_curr_int( victim ) / 10, 50 ) ) / 100;

		/* Confusion chance */
		if ( number_percent() < acfg( "mindflayer.psychicmaelstrom.confuse_chance" ) ) {
			act( "$N is confused by the psychic assault!", ch, NULL, victim, TO_CHAR );
			/* Could trigger flee here */
		}

		damage( ch, victim, dam, gsn_punch );
	}
	return;
}

/*
 * Mind Blast - Psionic Storm 2: Cone stun attack
 */
void do_psiblast( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	CHAR_DATA *victim_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_PSIONIC] < 2 ) {
		send_to_char( "You need Psionic Storm level 2. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.mindblast.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.mindblast.focus_req" ) ) {
		send_to_char( "You need more Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.mindblast.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.mindblast.mana_cost" );
	ch->rage -= acfg( "mindflayer.mindblast.focus_cost" );
	WAIT_STATE( ch, acfg( "mindflayer.mindblast.cooldown" ) );

	act( "You project a devastating cone of psionic force!", ch, NULL, NULL, TO_CHAR );
	act( "$n projects a devastating cone of psionic force!", ch, NULL, NULL, TO_ROOM );

	for ( victim = ch->in_room->people; victim != NULL; victim = victim_next ) {
		victim_next = victim->next_in_room;

		if ( victim == ch ) continue;
		if ( is_same_group( ch, victim ) ) continue;
		if ( IS_AFFECTED( victim, AFF_CHARM ) && victim->master == ch ) continue;
		if ( !IS_NPC( victim ) && !IS_NPC( ch ) && victim->fighting != ch ) continue;

		dam = number_range( 250, 450 );
		dam += ch->rage * 4;

		/* Mental damage - reduce by target INT */
		dam = dam * ( 100 - UMIN( get_curr_int( victim ) / 10, 50 ) ) / 100;

		/* Stun */
		act( "$N is stunned by the mind blast!", ch, NULL, victim, TO_CHAR );
		WAIT_STATE( victim, acfg( "mindflayer.mindblast.stun_duration" ) * 12 );

		damage( ch, victim, dam, gsn_punch );
	}
	return;
}

/*
 * Reality Fracture - Psionic Storm 3: Ultimate AoE with madness
 */
void do_realityfracture( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	CHAR_DATA *victim_next;
	int dam;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[MIND_TRAIN_PSIONIC] < 3 ) {
		send_to_char( "You need Psionic Storm level 3. See #x035mindtrain#n.\n\r", ch );
		return;
	}
	if ( ch->fighting == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	if ( ch->mana < acfg( "mindflayer.realityfracture.mana_cost" ) ) {
		send_to_char( "You don't have enough mana.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.realityfracture.focus_req" ) ) {
		send_to_char( "You need more Focus.\n\r", ch );
		return;
	}
	if ( ch->rage < acfg( "mindflayer.realityfracture.focus_cost" ) ) {
		send_to_char( "You don't have enough Focus.\n\r", ch );
		return;
	}

	ch->mana -= acfg( "mindflayer.realityfracture.mana_cost" );
	ch->rage -= acfg( "mindflayer.realityfracture.focus_cost" );
	WAIT_STATE( ch, acfg( "mindflayer.realityfracture.cooldown" ) );

	act( "#x035You tear at the very fabric of reality!#n", ch, NULL, NULL, TO_CHAR );
	act( "#x035$n tears at the very fabric of reality!#n", ch, NULL, NULL, TO_ROOM );

	for ( victim = ch->in_room->people; victim != NULL; victim = victim_next ) {
		victim_next = victim->next_in_room;

		if ( victim == ch ) continue;
		if ( is_same_group( ch, victim ) ) continue;
		if ( IS_AFFECTED( victim, AFF_CHARM ) && victim->master == ch ) continue;
		if ( !IS_NPC( victim ) && !IS_NPC( ch ) && victim->fighting != ch ) continue;

		dam = number_range( 400, 700 );
		dam += ch->rage * 5;

		/* Mental damage - reduce by target INT */
		dam = dam * ( 100 - UMIN( get_curr_int( victim ) / 10, 50 ) ) / 100;

		act( "$N's sanity shatters!", ch, NULL, victim, TO_CHAR );

		damage( ch, victim, dam, gsn_punch );
	}
	return;
}

/*
 * Dismiss - Release a thrall from your control
 */
void do_dismiss( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "Dismiss which thrall?\n\r", ch );
		return;
	}

	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( !IS_NPC( victim ) || victim->master != ch ) {
		send_to_char( "That is not your thrall.\n\r", ch );
		return;
	}

	REMOVE_BIT( victim->affected_by, AFF_CHARM );
	victim->master = NULL;
	victim->leader = NULL;

	act( "You release $N from your mental control.", ch, NULL, victim, TO_CHAR );
	act( "Your mind is suddenly free! You are no longer $n's thrall.", ch, NULL, victim, TO_VICT );
	act( "$n releases $N from mental domination.", ch, NULL, victim, TO_NOTVICT );

	ch->pcdata->powers[MIND_THRALL_COUNT] = count_thralls( ch );
	return;
}

/*
 * Mindflayer Armor creation command
 */
void do_mindflayerarmor( CHAR_DATA *ch, char *argument ) {
	do_classarmor_generic( ch, argument, CLASS_MINDFLAYER );
}

/*
 * Mindflayer tick update function
 */
void update_mindflayer( CHAR_DATA *ch ) {
	CHAR_DATA *fch;
	CHAR_DATA *fch_next;

	if ( !IS_CLASS( ch, CLASS_MINDFLAYER ) ) return;

	/* Focus build/decay */
	if ( ch->position == POS_FIGHTING ) {
		/* +2 per tick in combat (vs +1 for Psion) */
		if ( ch->rage < 150 )
			ch->rage = UMIN( ch->rage + 2, 150 );
	} else if ( ch->rage > 0 ) {
		/* -1 per tick out of combat (slower than Psion's -2) */
		ch->rage -= 1;
		if ( ch->rage < 0 ) ch->rage = 0;
	}

	/* Track peak focus */
	if ( ch->rage > ch->pcdata->stats[MIND_PEAK_FOCUS] )
		ch->pcdata->stats[MIND_PEAK_FOCUS] = ch->rage;

	/* Hivemind duration countdown */
	if ( ch->pcdata->powers[MIND_HIVEMIND] > 0 ) {
		ch->pcdata->powers[MIND_HIVEMIND]--;
		if ( ch->pcdata->powers[MIND_HIVEMIND] <= 0 ) {
			send_to_char( "Your hivemind connection fades.\n\r", ch );
		}
	}

	/* Update thrall count */
	ch->pcdata->powers[MIND_THRALL_COUNT] = count_thralls( ch );

	return;
}
