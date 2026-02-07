/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "cfg.h"

void check_samuraiattack args( ( CHAR_DATA * ch, CHAR_DATA *victim ) );

void do_katana( CHAR_DATA *ch, char *argument ) {
	OBJ_DATA *obj;
	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( cfg( CFG_ABILITY_SAMURAI_KATANA_PRIMAL_COST ) > ch->practice ) {
		send_to_char( "It costs 250 points of primal to create a Katana.\n\r", ch );
		return;
	}
	ch->practice -= cfg( CFG_ABILITY_SAMURAI_KATANA_PRIMAL_COST );
	obj = create_object( get_obj_index( 33176 ), 50 );
	SET_BIT( obj->quest, QUEST_RELIC );
	obj->condition = 100;
	obj->toughness = 100;
	obj->resistance = 1;
	obj->value[1] = cfg( CFG_ABILITY_SAMURAI_KATANA_WEAPON_DICE_COUNT );
	obj->value[2] = cfg( CFG_ABILITY_SAMURAI_KATANA_WEAPON_DICE_SIZE );
	obj->questowner = str_dup( ch->pcdata->switchname );
	obj_to_char( obj, ch );
	act( "A katana flashes into existance.", ch, NULL, NULL, TO_CHAR );
	act( "A katana suddenly apear in $n's hands.", ch, NULL, NULL, TO_ROOM );
	return;
}

void do_bladespin( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		stc( "Huh?\n\r", ch );
		return;
	}
	if ( ch->wpn[3] < cfg( CFG_ABILITY_SAMURAI_BLADESPIN_WPN_SKILL_REQ ) || ch->wpn[0] < cfg( CFG_ABILITY_SAMURAI_BLADESPIN_WPN_SKILL_REQ ) || ch->wpn[1] < cfg( CFG_ABILITY_SAMURAI_BLADESPIN_WPN_SKILL_REQ ) ) {
		stc( "You are not a true samurai yet.\n\r", ch );
		return;
	}
	if ( IS_SET( ch->newbits, NEW_BLADESPIN ) ) {
		stc( "You relax your combat techniques.\n\r", ch );
		act( "$n relaxes.", ch, NULL, NULL, TO_ROOM );
		REMOVE_BIT( ch->newbits, NEW_BLADESPIN );
		return;
	}
	if ( !IS_SET( ch->newbits, NEW_BLADESPIN ) ) {
		stc( "Your concentrate on your combat techniques.\n\r", ch );
		act( "$n's muscles flexes, looking for prey that one, better beware!", ch, NULL, NULL, TO_ROOM );
		SET_BIT( ch->newbits, NEW_BLADESPIN );
		return;
	}
	return;
}

void do_hologramtransfer( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	if ( IS_NPC( ch ) )
		return;

	argument = one_argument( argument, arg );

	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if ( arg[0] == '\0' ) {
		send_to_char( "Walk the ancestrial path to whom?\n\r", ch );
		return;
	}

	if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
		send_to_char( "They are not of this world.\n\r", ch );
		return;
	}

	if ( ch->move < cfg( CFG_ABILITY_SAMURAI_HOLOGRAMTRANSFER_MOVE_COST ) ) {
		stc( "The path escapes your senses as you loose control over your inner energies.\n\r", ch );
		return;
	}

	if ( IS_SET( victim->in_room->room_flags, ROOM_ASTRAL ) ) {
		stc( "You can't find it's room.\n\r", ch );
		return;
	}
	if ( IS_SET( ch->in_room->room_flags, ROOM_ASTRAL ) ) {
		stc( "You cannot sense any paths leading from this room.\n\r", ch );
		return;
	}

	if ( ch == victim ) {
		send_to_char( "But you're already at yourself!\n\r", ch );
		return;
	}
	if ( room_is_private( victim->in_room ) ) {
		send_to_char( "That room is private right now.\n\r", ch );
		return;
	}
	if ( !IS_NPC( victim ) && !IS_IMMUNE( victim, IMM_SUMMON ) ) {
		send_to_char( "They don't want you near them.\n\r", ch );
		return;
	}
	if ( victim->in_room == ch->in_room ) {
		send_to_char( "But you're already there!\n\r", ch );
		return;
	}

	act( "You follow the path to $N.", ch, NULL, victim, TO_CHAR );
	act( "$n steps into the air, leaving behind no trace whatsoever.", ch, NULL, victim, TO_ROOM );
	char_from_room( ch );
	char_to_room( ch, victim->in_room );
	use_move( ch, cfg( CFG_ABILITY_SAMURAI_HOLOGRAMTRANSFER_MOVE_COST ) );
	act( "$n steps out of the air in front of $N.", ch, NULL, victim, TO_NOTVICT );
	act( "$n steps out of the air in front of you.", ch, NULL, victim, TO_VICT );
	do_look( ch, "auto" );
	return;
}

void do_focus( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SAMURAI_FOCUS] < 10 ) {
		send_to_char( "You cannot focus your weaponcontrol any more.\n\r", ch );
		return;
	}
	ch->pcdata->powers[SAMURAI_FOCUS] -= number_range( 1, ch->pcdata->powers[SAMURAI_FOCUS] );
	send_to_char( "You focus your control, letting the battle slow to a halt.\n\r", ch );
	WAIT_STATE( ch, cfg( CFG_ABILITY_SAMURAI_FOCUS_COOLDOWN ) );
	return;
}

void do_slide( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( !IS_SET( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_SLIDE ) ) {
		send_to_char( "You need to learn that combo first.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SAMURAI_FOCUS] > cfg( CFG_ABILITY_SAMURAI_SLIDE_FOCUS_MAX ) ) {
		send_to_char( "You are to exhausted.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	ch->pcdata->powers[SAMURAI_FOCUS] += cfg( CFG_ABILITY_SAMURAI_SLIDE_FOCUS_COST );
	act( "$n slides into a better fighting position, hitting $N while $e passes.", ch, NULL, victim, TO_NOTVICT );
	act( "You slide into a better fighting position, hitting $N while you pass $E.", ch, NULL, victim, TO_CHAR );
	act( "$n moves past you, lightning fast and strikes you before you can react.", ch, NULL, victim, TO_VICT );
	one_hit( ch, victim, gsn_lightningslash, 1 );
	check_samuraiattack( ch, victim );
	WAIT_STATE( ch, cfg( CFG_ABILITY_SAMURAI_SLIDE_COOLDOWN ) );
	return;
}

void do_sidestep( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( !IS_SET( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_SIDESTEP ) ) {
		send_to_char( "You need to learn that combo first.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SAMURAI_FOCUS] > cfg( CFG_ABILITY_SAMURAI_SIDESTEP_FOCUS_MAX ) ) {
		send_to_char( "You are to exhausted.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	ch->pcdata->powers[SAMURAI_FOCUS] += cfg( CFG_ABILITY_SAMURAI_SIDESTEP_FOCUS_COST );
	act( "$n sidesteps $N's attack and scores a hit.", ch, NULL, victim, TO_NOTVICT );
	act( "You sidestep $N's attack, and scores a counterattack before $E can react.", ch, NULL, victim, TO_CHAR );
	act( "$n sidesteps your feeble attempt to get near $m and strikes back at you.", ch, NULL, victim, TO_VICT );
	one_hit( ch, victim, gsn_lightningslash, 1 );
	check_samuraiattack( ch, victim );
	WAIT_STATE( ch, cfg( CFG_ABILITY_SAMURAI_SIDESTEP_COOLDOWN ) );
	return;
}

void do_block( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( !IS_SET( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_BLOCK ) ) {
		send_to_char( "You need to learn that combo first.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SAMURAI_FOCUS] > cfg( CFG_ABILITY_SAMURAI_BLOCK_FOCUS_MAX ) ) {
		send_to_char( "You are to exhausted.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	ch->pcdata->powers[SAMURAI_FOCUS] += cfg( CFG_ABILITY_SAMURAI_BLOCK_FOCUS_COST );
	act( "$n blocks $N's simpleminded attack and strikes back with a perfect attack.", ch, NULL, victim, TO_NOTVICT );
	act( "You block $N's simple attack and return with one of your own design.", ch, NULL, victim, TO_CHAR );
	act( "$n blocks your attack, and strikes back before you get a chance to react.", ch, NULL, victim, TO_VICT );
	one_hit( ch, victim, gsn_lightningslash, 1 );
	check_samuraiattack( ch, victim );
	WAIT_STATE( ch, cfg( CFG_ABILITY_SAMURAI_BLOCK_COOLDOWN ) );
	return;
}

void do_countermove( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( !IS_SET( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_COUNTERMOVE ) ) {
		send_to_char( "You need to learn that combo first.\n\r", ch );
		return;
	}
	if ( ch->pcdata->powers[SAMURAI_FOCUS] > cfg( CFG_ABILITY_SAMURAI_COUNTERMOVE_FOCUS_MAX ) ) {
		send_to_char( "You are to exhausted.\n\r", ch );
		return;
	}
	if ( ( victim = ch->fighting ) == NULL ) {
		send_to_char( "You aren't fighting anyone.\n\r", ch );
		return;
	}
	ch->pcdata->powers[SAMURAI_FOCUS] += cfg( CFG_ABILITY_SAMURAI_COUNTERMOVE_FOCUS_COST );
	act( "$n strikes out at $N before $E can even get a weapon out to defend $Mself.", ch, NULL, victim, TO_NOTVICT );
	act( "You cut $N with a lightning fast attack.", ch, NULL, victim, TO_CHAR );
	act( "$n attacks with a flurry of lightning fast attacks, one of them scores a hit.", ch, NULL, victim, TO_VICT );
	one_hit( ch, victim, gsn_lightningslash, 1 );
	check_samuraiattack( ch, victim );
	WAIT_STATE( ch, cfg( CFG_ABILITY_SAMURAI_COUNTERMOVE_COOLDOWN ) );
	return;
}

void check_samuraiattack( CHAR_DATA *ch, CHAR_DATA *victim ) {
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) return;
	if ( !victim || victim->hit < cfg( CFG_ABILITY_SAMURAI_COMBO_VICTIM_MIN_HP ) ) return;

	switch ( ch->pcdata->powers[SAMURAI_FOCUS] ) {
	default:
		break;
	case 10:
		if ( ch->pcdata->powers[SAMURAI_LAST] == 10 ) {
			send_to_char( "You cannot use the same combo twice.\n\r", ch );
			break;
		}
		ch->pcdata->powers[SAMURAI_LAST] = 10;
		do_say( ch, "What a worthless opponent you are. Here, let me show you how a real fighter fights." );
		one_hit( ch, victim, gsn_lightningslash, 1 );
		one_hit( ch, victim, gsn_lightningslash, 1 );
		one_hit( ch, victim, gsn_lightningslash, 1 );
		break;
	case 15:
		if ( ch->pcdata->powers[SAMURAI_LAST] == 15 ) {
			send_to_char( "You cannot use the same combo twice.\n\r", ch );
			break;
		}
		ch->pcdata->powers[SAMURAI_LAST] = 15;
		if ( number_range( 1, cfg( CFG_ABILITY_SAMURAI_COMBO_15_FAIL_RANGE_MAX ) ) == 2 ) {
			send_to_char( "You fail your attempt to disarm.\n\r", ch );
			break;
		}
		act( "#GYou strike out at $N's weapon.#n", ch, NULL, victim, TO_CHAR );
		act( "#G$n's attack strikes your weaponarm.#n", ch, NULL, victim, TO_VICT );
		disarm( ch, victim );
		break;
	case 20:
		if ( ch->pcdata->powers[SAMURAI_LAST] == 20 ) {
			send_to_char( "You cannot use the same combo twice.\n\r", ch );
			break;
		}
		ch->pcdata->powers[SAMURAI_LAST] = 20;
		act( "#GYour hit $N on a central nerve, paralysing $M.#n", ch, NULL, victim, TO_CHAR );
		act( "#G$n's attack paralyses you, you cannot move.#n", ch, NULL, victim, TO_VICT );
		WAIT_STATE( victim, cfg( CFG_ABILITY_SAMURAI_COMBO_20_VICTIM_COOLDOWN ) );
		break;
	case 25:
		if ( ch->pcdata->powers[SAMURAI_LAST] == 25 ) {
			send_to_char( "You cannot use the same combo twice.\n\r", ch );
			break;
		}
		ch->pcdata->powers[SAMURAI_LAST] = 25;
		if ( number_range( 1, cfg( CFG_ABILITY_SAMURAI_COMBO_25_FAIL_RANGE_MAX ) ) == 2 ) {
			send_to_char( "You fail your attempt to hurl.\n\r", ch );
			break;
		}
		act( "#GYou grab $N and toss $M over your shoulder.#n", ch, NULL, victim, TO_CHAR );
		act( "#G$n grabs you, and tosses you over $s shoulder.#n", ch, NULL, victim, TO_VICT );
		special_hurl( ch, victim );
		break;
	case 30:
		if ( ch->pcdata->powers[SAMURAI_LAST] == 30 ) {
			send_to_char( "You cannot use the same combo twice.\n\r", ch );
			break;
		}
		ch->pcdata->powers[SAMURAI_LAST] = 30;
		heal_char( ch, number_range( cfg( CFG_ABILITY_SAMURAI_COMBO_30_HEAL_MIN ), cfg( CFG_ABILITY_SAMURAI_COMBO_30_HEAL_MAX ) ) );
		act( "#GYou feel adrenalin pump through your body, awakening your senses.#n", ch, NULL, victim, TO_CHAR );
		act( "#G$n flashes a wicked smile.#n", ch, NULL, victim, TO_VICT );
		break;
	case 35:
		if ( ch->pcdata->powers[SAMURAI_LAST] == 35 ) {
			send_to_char( "You cannot use the same combo twice.\n\r", ch );
			break;
		}
		ch->pcdata->powers[SAMURAI_LAST] = 35;
		do_say( ch, "I am a master of arms, you cannot even begin to understand what I am able to do!" );
		one_hit( ch, victim, gsn_backfist, 1 );
		one_hit( ch, victim, gsn_thrustkick, 1 );
		one_hit( ch, victim, gsn_monksweep, 1 );
		one_hit( ch, victim, gsn_jumpkick, 1 );
		one_hit( ch, victim, gsn_lightningslash, 1 );
		break;
	}
	return;
}

void do_martial( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int cost = cfg( CFG_ABILITY_SAMURAI_MARTIAL_EXP_COST );

	one_argument( argument, arg );

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_SAMURAI ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( arg[0] == '\0' ) {
		send_to_char( "What martial art do you wish to learn : slide, sidestep, block or countermove ?\n\r", ch );
		return;
	}
	if ( !str_cmp( arg, "slide" ) ) {
		if ( IS_SET( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_SLIDE ) ) {
			send_to_char( "You have already learned that technique.\n\r", ch );
			return;
		}
		if ( ch->exp < cost ) {
			sprintf( buf, "You need %d more exp to learn this technique.\n\r", cost - ch->exp );
			send_to_char( buf, ch );
			return;
		}
		ch->exp -= cost;
		SET_BIT( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_SLIDE );
		send_to_char( "Ok.\n\r", ch );
		return;
	} else if ( !str_cmp( arg, "sidestep" ) ) {
		if ( IS_SET( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_SIDESTEP ) ) {
			send_to_char( "You have already learned that technique.\n\r", ch );
			return;
		}
		if ( ch->exp < cost ) {
			sprintf( buf, "You need %d more exp to learn this technique.\n\r", cost - ch->exp );
			send_to_char( buf, ch );
			return;
		}
		ch->exp -= cost;
		SET_BIT( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_SIDESTEP );
		send_to_char( "Ok.\n\r", ch );
		return;
	} else if ( !str_cmp( arg, "block" ) ) {
		if ( IS_SET( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_BLOCK ) ) {
			send_to_char( "You have already learned that technique.\n\r", ch );
			return;
		}
		if ( ch->exp < cost ) {
			sprintf( buf, "You need %d more exp to learn this technique.\n\r", cost - ch->exp );
			send_to_char( buf, ch );
			return;
		}
		ch->exp -= cost;
		SET_BIT( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_BLOCK );
		send_to_char( "Ok.\n\r", ch );
		return;
	} else if ( !str_cmp( arg, "countermove" ) ) {
		if ( IS_SET( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_COUNTERMOVE ) ) {
			send_to_char( "You have already learned that technique.\n\r", ch );
			return;
		}
		if ( ch->exp < cost ) {
			sprintf( buf, "You need %d more exp to learn this technique.\n\r", cost - ch->exp );
			send_to_char( buf, ch );
			return;
		}
		ch->exp -= cost;
		SET_BIT( ch->pcdata->powers[SAMURAI_MARTIAL], SAM_COUNTERMOVE );
		send_to_char( "Ok.\n\r", ch );
		return;
	} else
		do_martial( ch, "" );
	return;
}
