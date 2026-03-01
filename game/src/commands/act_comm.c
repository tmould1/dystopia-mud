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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "gmcp.h"
#include "../systems/mcmp.h"
#include "../db/db_game.h"
#include "../script/script.h"
#include "../db/db_player.h"

extern GAMECONFIG_DATA game_config;

/*
 * Local functions.
 */
void talk_channel ( CHAR_DATA * ch, char *argument,
	int channel, const char *verb );
/* Trace's Bounty code */
void do_bounty( CHAR_DATA *ch, char *argument ) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );

	if ( IS_NPC( ch ) ) return;

	if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
		send_to_char( "Place a bounty on who's head?\n\rSyntax:  Bounty <victim> <amount>\n\r", ch );
		return;
	}

	if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
		send_to_char( "They are currently not logged in!", ch );
		return;
	}

	if ( IS_NPC( victim ) ) {
		send_to_char( "You cannot put a bounty on NPCs!", ch );
		return;
	}
	if ( victim == ch ) {
		stc( "Not on yourself\n\r", ch );
		return;
	}

	if ( victim->level >= 7 ) {
		send_to_char( "You can't put a bounty on an immortal.", ch );
		return;
	}

	if ( is_number( arg2 ) ) {
		int amount;
		amount = atoi( arg2 );
		if ( amount < 100 ) {
			stc( "Needs to be at least 100 QPs, less than that you gotta be kidding.\n\r", ch );
			return;
		}
		if ( ch->pcdata->quest < amount ) {
			send_to_char( "You don't have that many QPs!", ch );
			return;
		}
		ch->pcdata->quest -= amount;
		victim->pcdata->bounty += amount;
		snprintf( buf, sizeof( buf ), "%s puts %d qps on %s's head, who now have a %d qps bounty.", ch->name, amount, victim->name, victim->pcdata->bounty );
		do_info( ch, buf );
		return;
	}
}

/*
 * Generic channel function.
 */
void talk_channel( CHAR_DATA *ch, char *argument, int channel, const char *verb ) {
	char buf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;
	int position;

	if ( !IS_NPC( ch ) && ( get_age( ch ) - 17 ) < 2 &&
		( channel == CHANNEL_CHAT || channel == CHANNEL_MUSIC || channel == CHANNEL_YELL ) ) {
		send_to_char( "Newbies are restricted to use the newbie channel.\n\r", ch );
		return;
	}

	if ( RTIMER( ch->in_room, RTIMER_SILENCE ) != 0 ) {
		send_to_char( "Something prevents you from speaking in this room.\n\r", ch );
		return;
	}
	if ( argument[0] == '\0' ) {
		snprintf( buf, sizeof( buf ), "%s what?\n\r", verb );
		buf[0] = toupper( buf[0] );
		send_to_char( buf, ch );
		return;
	}

	if ( IS_HEAD( ch, LOST_TONGUE ) ) {
		snprintf( buf, sizeof( buf ), "You can't %s without a tongue!\n\r", verb );
		send_to_char( buf, ch );
		return;
	}

	if ( IS_EXTRA( ch, GAGGED ) ) {
		snprintf( buf, sizeof( buf ), "You can't %s with a gag on!\n\r", verb );
		send_to_char( buf, ch );
		return;
	}

	REMOVE_BIT( ch->deaf, channel );

	switch ( channel ) {
	default:

		if ( ch->flag4 == 1 ) {
			snprintf( buf, sizeof( buf ), "You whine '#1%s#n'.\n\r", argument );
			send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ), "%s #Pwhines#n '#1$t#n'.", ch->name );

		} else if ( channel == CHANNEL_FLAME ) {
			snprintf( buf, sizeof( buf ), "You %s '#C%s#n'.\n\r", verb, argument );
			send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ), "%s %ses '#C$t#n'.", ch->name, verb );
		} else if ( channel == CHANNEL_CHAT ) {
			snprintf( buf, sizeof( buf ), "You %s '#1%s#n'.\n\r", verb, argument );
			send_to_char( buf, ch );
			if ( ch->trust > 6 )
				snprintf( buf, sizeof( buf ), "#y(#G*#y)#C%s#y(#G*#y)#n '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_WEREWOLF ) )
				snprintf( buf, sizeof( buf ), "%s barks '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_MAGE ) )
				snprintf( buf, sizeof( buf ), "%s chants '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_SHAPESHIFTER ) )
				snprintf( buf, sizeof( buf ), "%s whispers '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_VAMPIRE ) )
				snprintf( buf, sizeof( buf ), "%s snarls '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_ANGEL ) )
				snprintf( buf, sizeof( buf ), "%s preaches '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_TANARRI ) )
				snprintf( buf, sizeof( buf ), "%s booms '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_LICH ) )
				snprintf( buf, sizeof( buf ), "%s squicks '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_DEMON ) )
				snprintf( buf, sizeof( buf ), "%s growls '#1$t#n'.", ch->name );
			else if ( IS_CLASS( ch, CLASS_DROID ) )
				snprintf( buf, sizeof( buf ), "%s chitters '#1$t#n'.", ch->name );
			else
				snprintf( buf, sizeof( buf ), "%s %ss '#1$t#n'.", ch->name, verb );
		}

		else {
			snprintf( buf, sizeof( buf ), "You %s '#1%s#n'.\n\r", verb, argument );
			send_to_char( buf, ch );
			snprintf( buf, sizeof( buf ), "%s %ss '#1$t#n'.", ch->name, verb );
		}
		break;
	case CHANNEL_IMMTALK:
		snprintf( buf, sizeof( buf ), "#y.:#P%s#y:.#C $t.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_ANGEL:
		snprintf( buf, sizeof( buf ), "#0[#7%s#0]#C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_PRAY:
		snprintf( buf, sizeof( buf ), "#0[#R%s#0]#C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_MAGETALK:
		snprintf( buf, sizeof( buf ), "#n{{#0%s#n}}#C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_TELEPATH:
		snprintf( buf, sizeof( buf ), "#G*#C(>#R%s#C<)#G* #C'$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_COMMUNICATE:
		snprintf( buf, sizeof( buf ), "#p{#0-#p}#0%s#p{#0-#p} #C'$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_HOWL:
		snprintf( buf, sizeof( buf ), "#y((#L%s#y))#C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_MIKTALK:
		snprintf( buf, sizeof( buf ), "#C***#y%s#C*** '$t'.#n", ch->name );
		position = ch->position;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_HIGHTALK:
		snprintf( buf, sizeof( buf ), "#C-=#R%s#C=-  '$t'.#n", ch->name );
		position = ch->position;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_KNIGHTTALK:
		snprintf( buf, sizeof( buf ), "#0.x.#7%s#0.x.#C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_TANTALK:
		snprintf( buf, sizeof( buf ), "#y{#R%s#y}#C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_LICHTALK:
		snprintf( buf, sizeof( buf ), "#G>*<#7%s#G>*<#C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_NEWBIE:
		if ( ( get_age( ch ) - 17 ) < 2 )
			snprintf( buf, sizeof( buf ), "%s the newbie chats #7'#R$t#7'.#n", ch->name );
		else
			snprintf( buf, sizeof( buf ), "%s the newbie helper chats #7'#R$t#7'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_KINGDOM:
		snprintf( buf, sizeof( buf ), "#R{{{ #P%s #R}}}#n '#7$t#n'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_SIGN:
		snprintf( buf, sizeof( buf ), "#P.o0#0%s#P0o.#C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_MONK:
		snprintf( buf, sizeof( buf ), "#0.x[#c%s#0]x. #C '$t'.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;

	case CHANNEL_VAMPTALK:
		snprintf( buf, sizeof( buf ), "#R<<#0%s#R>>#C $t.#n", ch->name );
		position = ch->position;
		ch->position = POS_STANDING;
		act( buf, ch, argument, NULL, TO_CHAR );
		ch->position = position;
		break;
	}

	if ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_SILENCE ) ) return; // silenced, and they don't know it :)

	LIST_FOR_EACH( d, &g_descriptors, DESCRIPTOR_DATA, node ) {
		CHAR_DATA *och;
		CHAR_DATA *vch;

		och = d->original ? d->original : d->character;
		vch = d->character;

		if ( d->connected == CON_PLAYING && vch != ch && och != NULL && vch != NULL && !IS_SET( och->deaf, channel ) ) {

			if ( channel == CHANNEL_IMMTALK && !IS_IMMORTAL( och ) )
				continue;
			if ( channel == CHANNEL_MIKTALK && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_NINJA ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_HIGHTALK && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_SAMURAI ) && !IS_IMMORTAL( och ) ) )
				continue;

			if ( channel == CHANNEL_SIGN && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_DROW ) && !IS_IMMORTAL( och ) ) )
				continue;

			if ( channel == CHANNEL_KINGDOM && ( !IS_NPC( och ) && och->pcdata->kingdom != ch->pcdata->kingdom && !IS_IMMORTAL( och ) ) ) continue;

			if ( channel == CHANNEL_MONK && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_MONK ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_COMMUNICATE && ( !IS_NPC( och ) && !IS_IMMORTAL( och ) && !IS_CLASS( och, CLASS_DROID ) ) )
				continue;
			if ( channel == CHANNEL_VAMPTALK && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_VAMPIRE ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_MAGETALK && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_MAGE ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_PRAY && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_DEMON ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_ANGEL && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_ANGEL ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_KNIGHTTALK && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_UNDEAD_KNIGHT ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_LICHTALK && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_LICH ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_TANTALK && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_TANARRI ) && !IS_IMMORTAL( och ) ) )
				continue;

			if ( channel == CHANNEL_TELEPATH && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_SHAPESHIFTER ) && !IS_IMMORTAL( och ) ) )
				continue;
			if ( channel == CHANNEL_HOWL && ( !IS_NPC( och ) && !IS_CLASS( och, CLASS_WEREWOLF ) && !IS_POLYAFF( och, POLY_WOLF ) && !IS_IMMORTAL( och ) ) ) {
				if ( ( och->in_room ) && ( ch->in_room ) ) {
					if ( ( och->in_room == ch->in_room ) ) {
						act( "$n throws back $s head and howls loudly.", ch, argument, och, TO_VICT );
						if ( och->desc != NULL )
							mcmp_play( och->desc, "environment/howl.mp3", MCMP_SOUND, MCMP_TAG_ENVIRONMENT,
								60, 1, 30, NULL, FALSE, "A loud howl" );
						continue;
					} else if ( ( och->in_room->area == ch->in_room->area ) ) {
						act( "You hear a loud howl nearby.", ch, NULL, och, TO_VICT );
						if ( och->desc != NULL )
							mcmp_play( och->desc, "environment/howl.mp3", MCMP_SOUND, MCMP_TAG_ENVIRONMENT,
								35, 1, 30, NULL, FALSE, "A howl nearby" );
						continue;
					} else {
						act( "You hear a loud howl in the distance.", ch, NULL, och, TO_VICT );
						if ( och->desc != NULL )
							mcmp_play( och->desc, "environment/howl.mp3", MCMP_SOUND, MCMP_TAG_ENVIRONMENT,
								15, 1, 30, NULL, FALSE, "A distant howl" );
						continue;
					}
				}
			}

			if ( channel == CHANNEL_YELL && vch->in_room->area != ch->in_room->area )
				continue;

			position = vch->position;
			if ( channel != CHANNEL_YELL )
				vch->position = POS_STANDING;

			act( buf, ch, argument, vch, TO_VICT );
			vch->position = position;
			mcmp_channel_notify( vch, channel );
		}
	}

	return;
}

void room_message( ROOM_INDEX_DATA *room, char *message ) {
	CHAR_DATA *rch;

	if ( list_empty( &room->characters ) ) return;
	rch = LIST_ENTRY( room->characters.sentinel.next, CHAR_DATA, room_node );

	act( message, rch, NULL, NULL, TO_ROOM );
	act( message, rch, NULL, NULL, TO_CHAR );
}

void do_chat( CHAR_DATA *ch, char *argument ) {
	talk_channel( ch, argument, CHANNEL_CHAT, "chat" );
	return;
}

void do_flame( CHAR_DATA *ch, char *argument ) {
	talk_channel( ch, argument, CHANNEL_FLAME, "bitch" );
	return;
}

/*
 * Alander's new channels.
 */
void do_music( CHAR_DATA *ch, char *argument ) {
	talk_channel( ch, argument, CHANNEL_MUSIC, "sing" );

	return;
}

void do_yell( CHAR_DATA *ch, char *argument ) {

	talk_channel( ch, argument, CHANNEL_YELL, "yell" );

	return;
}

void do_immtalk( CHAR_DATA *ch, char *argument ) {
	talk_channel( ch, argument, CHANNEL_IMMTALK, "immtalk" );
	return;
}

void do_monktalk( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_MONK ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_MONK, "monk" );
	return;
}

void do_miktalk( CHAR_DATA *ch, char *argument ) {

	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_NINJA ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_MIKTALK, "miktalk" );
	return;
}

void do_hightalk( CHAR_DATA *ch, char *argument ) {

	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_SAMURAI ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_HIGHTALK, "hightalk" );
	return;
}
void do_knighttalk( CHAR_DATA *ch, char *argument ) {

	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_UNDEAD_KNIGHT ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_KNIGHTTALK, "sign" );
	return;
}

void do_sign( CHAR_DATA *ch, char *argument ) {

	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_DROW ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_SIGN, "sign" );
	return;
}

void do_newbie( CHAR_DATA *ch, char *argument ) {

	if ( IS_NPC( ch ) || IS_SET( ch->deaf, CHANNEL_NEWBIE ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_NEWBIE, "newbiechat" );
	return;
}

void do_ktalk( CHAR_DATA *ch, char *argument ) {

	if ( IS_NPC( ch ) || ch->pcdata->kingdom == 0 ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_KINGDOM, "ktalk" );
	return;
}

void do_vamptalk( CHAR_DATA *ch, char *argument ) {

	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_VAMPIRE ) ) )

	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_VAMPTALK, "vamptalk" );
	return;
}

void do_magetalk( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_MAGE ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_MAGETALK, "magetalk" );
	return;
}

void do_lichtalk( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_LICH ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_LICHTALK, "lichtalk" );
	return;
}

void do_tantalk( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_TANARRI ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_TANTALK, "tantalk" );
	return;
}

void do_monk( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_MONK ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_MONK, "monk" );
	return;
}

void do_angeltalk( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_ANGEL ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_ANGEL, "angel" );
	return;
}

void do_pray( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];

	act( "You mutter a few prayers.", ch, NULL, NULL, TO_CHAR );
	act( "$n mutters a quick prayer.", ch, NULL, NULL, TO_ROOM );

	if ( ch->in_room != NULL && ch->in_room->vnum == ROOM_VNUM_ALTAR &&
		ch->class == 0 && ch->position != POS_FIGHTING ) {
		send_to_char( "You glow bright blue as you are restored.\n\r", ch );
		ch->hit = ch->max_hit;
		ch->mana = ch->max_mana;
		ch->move = ch->max_move;
		ch->loc_hp[0] = 0;
		ch->loc_hp[1] = 0;
		ch->loc_hp[2] = 0;
		ch->loc_hp[3] = 0;
		ch->loc_hp[4] = 0;
		ch->loc_hp[5] = 0;
		ch->loc_hp[6] = 0;
		update_pos( ch );
	}
	if ( IS_NPC( ch ) || ( !IS_CLASS( ch, CLASS_DEMON ) && !IS_IMMORTAL( ch ) ) )
		return;

	else if ( argument[0] == '\0' ) {
		if ( ch->pcdata->stats[DEMON_CURRENT] < 1 ) {
			send_to_char( "Nothing happens.\n\r", ch );
			return;
		}
		if ( ( victim = get_char_world( ch, ch->lord ) ) == NULL ) {
			send_to_char( "Nothing happens.\n\r", ch );
			return;
		}
		act( "You hear $n's prayers in your mind.", ch, NULL, victim, TO_VICT );
		send_to_char( "You feel energy pour into your body.\n\r", victim );
		if ( ch->pcdata->stats[DEMON_CURRENT] == 1 )
			snprintf( buf, sizeof( buf ), "You receive a single point of energy.\n\r" );
		else
			snprintf( buf, sizeof( buf ), "You receive %d points of energy.\n\r",
				ch->pcdata->stats[DEMON_CURRENT] );
		send_to_char( buf, victim );
		act( "$n is briefly surrounded by a halo of energy.", victim, NULL, NULL, TO_ROOM );
		victim->pcdata->stats[DEMON_CURRENT] += ch->pcdata->stats[DEMON_CURRENT];
		victim->pcdata->stats[DEMON_TOTAL] += ch->pcdata->stats[DEMON_CURRENT];
		ch->pcdata->stats[DEMON_CURRENT] = 0;
		return;
	}
	if ( IS_SET( ch->deaf, CHANNEL_PRAY ) ) {
		send_to_char( "But you're not even on the channel!\n\r", ch );
		return;
	}

	talk_channel( ch, argument, CHANNEL_PRAY, "pray" );
	return;
}

void do_howl( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_WEREWOLF ) && !IS_POLYAFF( ch, POLY_WOLF ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_HOWL, "howls" );
	return;
}

void do_say( CHAR_DATA *ch, char *argument ) {
	char name[80];
	char poly[MAX_STRING_LENGTH];
	char speak[10];
	char speaks[10];
	char endbit[2];
	char secbit[2];
	CHAR_DATA *to;
	bool is_ok;

	if ( RTIMER( ch->in_room, RTIMER_SILENCE ) != 0 ) {
		send_to_char( "Something prevents you from speaking in this room.\n\r", ch );
		return;
	}
	if ( IS_HEAD( ch, LOST_TONGUE ) ) {
		send_to_char( "You can't speak without a tongue!\n\r", ch );
		return;
	}
	if ( IS_EXTRA( ch, GAGGED ) ) {
		send_to_char( "You can't speak with a gag on!\n\r", ch );
		return;
	}

	if ( strlen( argument ) > MAX_INPUT_LENGTH ) {
		send_to_char( "Line too long.\n\r", ch );
		return;
	}

	if ( argument[0] == '\0' ) {
		send_to_char( "Say what?\n\r", ch );
		return;
	}

	endbit[0] = argument[strlen( argument ) - 1];
	endbit[1] = '\0';

	if ( strlen( argument ) > 1 )
		secbit[0] = argument[strlen( argument ) - 2];
	else
		secbit[0] = '\0';
	secbit[1] = '\0';

	if ( IS_BODY( ch, CUT_THROAT ) ) {
		snprintf( speak, sizeof( speak ), "rasp" );
		snprintf( speaks, sizeof( speaks ), "rasps" );
	} else if ( !IS_NPC( ch ) &&
		( IS_SET( ch->special, SPC_WOLFMAN ) || IS_POLYAFF( ch, POLY_WOLF ) || ( IS_CLASS( ch, CLASS_VAMPIRE ) && ch->pcdata->stats[UNI_RAGE] > 0 ) ) ) {
		if ( number_percent() > 50 ) {
			snprintf( speak, sizeof( speak ), "growl" );
			snprintf( speaks, sizeof( speaks ), "growls" );
		} else {
			snprintf( speak, sizeof( speak ), "snarl" );
			snprintf( speaks, sizeof( speaks ), "snarls" );
		}
	} else if ( !IS_NPC( ch ) && IS_POLYAFF( ch, POLY_BAT ) ) {
		snprintf( speak, sizeof( speak ), "squeak" );
		snprintf( speaks, sizeof( speaks ), "squeaks" );
	} else if ( !IS_NPC( ch ) && IS_POLYAFF( ch, POLY_SERPENT ) ) {
		snprintf( speak, sizeof( speak ), "hiss" );
		snprintf( speaks, sizeof( speaks ), "hisses" );
	} else if ( !IS_NPC( ch ) && IS_POLYAFF( ch, POLY_FROG ) ) {
		snprintf( speak, sizeof( speak ), "croak" );
		snprintf( speaks, sizeof( speaks ), "croaks" );
	} else if ( !IS_NPC( ch ) && IS_POLYAFF( ch, POLY_RAVEN ) ) {
		snprintf( speak, sizeof( speak ), "squark" );
		snprintf( speaks, sizeof( speaks ), "squarks" );
	} else if ( IS_NPC( ch ) && ch->pIndexData->vnum == MOB_VNUM_FROG ) {
		snprintf( speak, sizeof( speak ), "croak" );
		snprintf( speaks, sizeof( speaks ), "croaks" );
	} else if ( IS_NPC( ch ) && ch->pIndexData->vnum == MOB_VNUM_RAVEN ) {
		snprintf( speak, sizeof( speak ), "squark" );
		snprintf( speaks, sizeof( speaks ), "squarks" );
	} else if ( IS_NPC( ch ) && ch->pIndexData->vnum == MOB_VNUM_CAT ) {
		snprintf( speak, sizeof( speak ), "purr" );
		snprintf( speaks, sizeof( speaks ), "purrs" );
	} else if ( IS_NPC( ch ) && ch->pIndexData->vnum == MOB_VNUM_DOG ) {
		snprintf( speak, sizeof( speak ), "bark" );
		snprintf( speaks, sizeof( speaks ), "barks" );
	} else if ( !str_cmp( endbit, "!" ) ) {
		snprintf( speak, sizeof( speak ), "exclaim" );
		snprintf( speaks, sizeof( speaks ), "exclaims" );
	} else if ( !str_cmp( endbit, "?" ) ) {
		snprintf( speak, sizeof( speak ), "ask" );
		snprintf( speaks, sizeof( speaks ), "asks" );
	} else if ( secbit[0] != '\0' && str_cmp( secbit, "." ) && !str_cmp( endbit, "." ) ) {
		snprintf( speak, sizeof( speak ), "state" );
		snprintf( speaks, sizeof( speaks ), "states" );
	} else if ( secbit[0] != '\0' && !str_cmp( secbit, "." ) && !str_cmp( endbit, "." ) ) {
		snprintf( speak, sizeof( speak ), "mutter" );
		snprintf( speaks, sizeof( speaks ), "mutters" );
	} else if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 ) {
		snprintf( speak, sizeof( speak ), "slur" );
		snprintf( speaks, sizeof( speaks ), "slurs" );
	} else {
		snprintf( speak, sizeof( speak ), "say" );
		snprintf( speaks, sizeof( speaks ), "says" );
	}
	snprintf( poly, sizeof( poly ), "You %s '#3$T#n'.", speak );

	act( poly, ch, NULL, argument, TO_CHAR );

	if ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_SILENCE ) ) return;

	snprintf( poly, sizeof( poly ), "$n %s '#3$T#n'.", speaks );
	if ( ch->in_room->vnum != ROOM_VNUM_IN_OBJECT ) {
		act( poly, ch, NULL, argument, TO_ROOM );
		script_trigger_speech( ch, strlower( argument ) );
		script_trigger_room_speech( ch, strlower( argument ) );
		return;
	}

	LIST_FOR_EACH( to, &ch->in_room->characters, CHAR_DATA, room_node ) {
		is_ok = FALSE;

		if ( to->desc == NULL || !IS_AWAKE( to ) )
			continue;

		if ( ch == to )
			continue;

		if ( !IS_NPC( ch ) && ch->pcdata->chobj != NULL &&
			ch->pcdata->chobj->in_room != NULL &&
			!IS_NPC( to ) && to->pcdata->chobj != NULL &&
			to->pcdata->chobj->in_room != NULL &&
			ch->in_room == to->in_room )
			is_ok = TRUE;
		else
			is_ok = FALSE;

		if ( !IS_NPC( ch ) && ch->pcdata->chobj != NULL &&
			ch->pcdata->chobj->in_obj != NULL &&
			!IS_NPC( to ) && to->pcdata->chobj != NULL &&
			to->pcdata->chobj->in_obj != NULL &&
			ch->pcdata->chobj->in_obj == to->pcdata->chobj->in_obj )
			is_ok = TRUE;
		else
			is_ok = FALSE;

		if ( !is_ok ) continue;

		if ( IS_NPC( ch ) )
			snprintf( name, sizeof( name ), "%s", ch->short_descr );
		else if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_POLYMORPH ) )
			snprintf( name, sizeof( name ), "%s", ch->morph );
		else if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_SHIFT ) )
			snprintf( name, sizeof( name ), "%s", ch->morph );

		else
			snprintf( name, sizeof( name ), "%s", ch->name );
		name[0] = toupper( name[0] );
		snprintf( poly, sizeof( poly ), "%s %s '%s'.\n\r", name, speaks, argument );
		send_to_char( poly, to );
	}

	script_trigger_speech( ch, strlower( argument ) );
	script_trigger_room_speech( ch, strlower( argument ) );
	return;
}

char *strlower( char *ip ) {
	static char buffer[MAX_INPUT_LENGTH];
	int pos;

	for ( pos = 0; pos < ( MAX_INPUT_LENGTH - 1 ) && ip[pos] != '\0'; pos++ ) {
		buffer[pos] = tolower( ip[pos] );
	}
	buffer[pos] = '\0';
	return buffer;
}

void do_tell( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char poly[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int position;

	if ( IS_EXTRA( ch, GAGGED ) ) {
		send_to_char( "Your message didn't get through.\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' || argument[0] == '\0' ) {
		send_to_char( "Tell whom what?\n\r", ch );
		return;
	}

	/*
	 * Can tell to PC's anywhere, but NPC's only in same room.
	 * -- Furey
	 */
	if ( ( victim = get_char_world( ch, arg ) ) == NULL || ( IS_NPC( victim ) && victim->in_room != ch->in_room ) ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) ) {
		act( "$E can't hear you.", ch, 0, victim, TO_CHAR );
		return;
	}

	if ( !IS_NPC( victim ) && victim->desc == NULL ) {
		act( "$E is currently link dead.", ch, 0, victim, TO_CHAR );
		return;
	}

	if ( IS_SET( victim->deaf, CHANNEL_TELL ) && !IS_IMMORTAL( ch ) ) {
		if ( IS_NPC( victim ) || IS_NPC( ch ) || strlen( victim->pcdata->marriage ) < 2 || str_cmp( ch->name, victim->pcdata->marriage ) ) {
			act( "$E can't hear you.", ch, 0, victim, TO_CHAR );
			return;
		}
	}
	snprintf( poly, sizeof( poly ), "You tell $N '#C$t#n'." );
	act( poly, ch, argument, victim, TO_CHAR );

	if ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_SILENCE ) ) return;

	position = victim->position;
	victim->position = POS_STANDING;

	snprintf( poly, sizeof( poly ), "$n tells you '#C$t#n'." );
	act( poly, ch, argument, victim, TO_VICT );

	victim->position = position;
	victim->reply = ch;
	mcmp_channel_notify( victim, CHANNEL_TELL );

	return;
}

void do_whisper( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	if ( IS_EXTRA( ch, GAGGED ) ) {
		send_to_char( "Not with a gag on!\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' || argument[0] == '\0' ) {
		send_to_char( "Syntax: whisper <person> <message>\n\r", ch );
		return;
	}

	if ( ( victim = get_char_world( ch, arg ) ) == NULL || ( victim->in_room != ch->in_room ) ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( !IS_AWAKE( victim ) ) {
		act( "$E cannot hear you.", ch, 0, victim, TO_CHAR );
		return;
	}

	if ( !IS_NPC( victim ) && victim->desc == NULL ) {
		act( "$E is currently link dead.", ch, 0, victim, TO_CHAR );
		return;
	}

	act( "You whisper to $N '$t'.", ch, argument, victim, TO_CHAR );
	act( "$n whispers to you '$t'.", ch, argument, victim, TO_VICT );
	act( "$n whispers something to $N.", ch, NULL, victim, TO_NOTVICT );

	return;
}

void do_reply( CHAR_DATA *ch, char *argument ) {
	char poly[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int position;

	if ( IS_EXTRA( ch, GAGGED ) ) {
		send_to_char( "Your message didn't get through.\n\r", ch );
		return;
	}

	if ( ( victim = ch->reply ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) ) {
		act( "$E can't hear you.", ch, 0, victim, TO_CHAR );
		return;
	}

	if ( !IS_NPC( victim ) && victim->desc == NULL ) {
		act( "$E is currently link dead.", ch, 0, victim, TO_CHAR );
		return;
	}
	snprintf( poly, sizeof( poly ), "You reply to $N '#C$t#n'." );
	act( poly, ch, argument, victim, TO_CHAR );

	if ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_SILENCE ) ) return;

	position = victim->position;
	victim->position = POS_STANDING;

	snprintf( poly, sizeof( poly ), "$n replies to you '#C$t#n'." );
	act( poly, ch, argument, victim, TO_VICT );

	victim->position = position;
	victim->reply = ch;

	return;
}

void do_emote( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char *plast;

	char name[80];
	char poly[MAX_INPUT_LENGTH];
	CHAR_DATA *to;
	bool is_ok;

	if ( IS_HEAD( ch, LOST_TONGUE ) || IS_HEAD( ch, LOST_HEAD ) || IS_EXTRA( ch, GAGGED ) ) {
		send_to_char( "You can't show your emotions.\n\r", ch );
		return;
	}

	if ( argument[0] == '\0' )

	{
		send_to_char( "Pose what?\n\r", ch );
		return;
	}

	for ( plast = argument; *plast != '\0'; plast++ );

	strcpy( buf, argument );
	if ( isalpha( plast[-1] ) )
		strcat( buf, "." );

	act( "$n $T", ch, NULL, buf, TO_CHAR );

	if ( ch->in_room->vnum != ROOM_VNUM_IN_OBJECT ) {
		act( "$n $T", ch, NULL, buf, TO_ROOM );
		return;
	}

	LIST_FOR_EACH( to, &ch->in_room->characters, CHAR_DATA, room_node ) {
		is_ok = FALSE;

		if ( to->desc == NULL || !IS_AWAKE( to ) )
			continue;

		if ( ch == to )
			continue;

		if ( !IS_NPC( ch ) && ch->pcdata->chobj != NULL &&
			ch->pcdata->chobj->in_room != NULL &&
			!IS_NPC( to ) && to->pcdata->chobj != NULL &&
			to->pcdata->chobj->in_room != NULL &&
			ch->in_room == to->in_room )
			is_ok = TRUE;
		else
			is_ok = FALSE;

		if ( !IS_NPC( ch ) && ch->pcdata->chobj != NULL &&
			ch->pcdata->chobj->in_obj != NULL &&
			!IS_NPC( to ) && to->pcdata->chobj != NULL &&
			to->pcdata->chobj->in_obj != NULL &&
			ch->pcdata->chobj->in_obj == to->pcdata->chobj->in_obj )
			is_ok = TRUE;
		else
			is_ok = FALSE;

		if ( !is_ok ) continue;

		if ( IS_NPC( ch ) )
			snprintf( name, sizeof( name ), "%s", ch->short_descr );
		else if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_POLYMORPH ) )
			snprintf( name, sizeof( name ), "%s", ch->morph );
		else if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_SHIFT ) )
			snprintf( name, sizeof( name ), "%s", ch->morph );

		else
			snprintf( name, sizeof( name ), "%s", ch->name );
		name[0] = toupper( name[0] );
		snprintf( poly, sizeof( poly ), "%s %s\n\r", name, buf );
		send_to_char( poly, to );
	}
	return;
}

/* Removed: do_xemote - xsocial system removed */

void do_bug( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || argument[0] == '\0' )
		return;

	db_game_append_bug(
		ch->in_room ? ch->in_room->vnum : 0,
		ch->name,
		argument );
	send_to_char( "Ok.  Thanks.\n\r", ch );
	return;
}

void do_idea( CHAR_DATA *ch, char *argument ) {
	send_to_char( "Please use board 2 for ideas.\n\r", ch );
	return;
}

void do_typo( CHAR_DATA *ch, char *argument ) {
	send_to_char( "Please post a personal note about any typo's to imm.\n\r", ch );
	return;
}

void do_rent( CHAR_DATA *ch, char *argument ) {
	send_to_char( "There is no rent here.  Just save and quit.\n\r", ch );
	return;
}

void do_qui( CHAR_DATA *ch, char *argument ) {
	send_to_char( "If you want to QUIT, you have to spell it out.\n\r", ch );
	return;
}

void do_quit( CHAR_DATA *ch, char *argument ) {
	DESCRIPTOR_DATA *d;
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *mount;

	if ( IS_NPC( ch ) )
		return;

	if ( ch->position == POS_FIGHTING ) {
		send_to_char( "No way! You are fighting.\n\r", ch );
		return;
	}

	if ( ch->position != POS_STANDING ) {
		ch->position = POS_STANDING;
	}

	if ( ch->fight_timer > 0 ) {
		send_to_char( "Not until your fight timer expires.\n\r", ch );
		return;
	}

	if ( ch->in_room != NULL ) {
		if ( IS_SET( ch->in_room->room_flags, ROOM_ARENA ) ) {
			send_to_char( "Your in the arena.\n\r", ch );
			return;
		}
	}

	if ( ch->level > 6 )
		; /* do nothing */
	else if ( IS_SET( ch->pcdata->jflags, JFLAG_SETLOGOUT ) )
		logout_message( ch );
	else if ( ch->pcdata->obj_vnum == 0 ) {
		snprintf( buf, sizeof( buf ), "#2%s #7has fled from #2%s#7.#n",
			ch->pcdata->switchname, game_config.game_name );
		leave_info( buf );
	}

	save_char_obj_backup( ch );
	if ( IS_SET( ch->extra, EXTRA_OSWITCH ) ) do_humanform( ch, "" );
	if ( ch->gladiator != NULL )
		ch->gladiator = NULL; /* set player to bet on to NULL */
	if ( ch->challenger != NULL )
		ch->challenger = NULL;
	if ( ch->challenged != NULL )
		ch->challenged = NULL;
	if ( ( mount = ch->mount ) != NULL ) do_dismount( ch, "" );

	switch ( number_range( 1, 10 ) ) {
	case 1:
		send_to_char( "      Honesty is the best policy, but insanity is a better defence\n\r", ch );
		break;
	case 2:
		send_to_char( "      Some people wish to get what they deserve, while others fear the same\n\r", ch );
		break;
	case 3:
		send_to_char( "      A wise man gets more use from his enemies than a fool from his friends\n\r", ch );
		break;
	case 4:
		send_to_char( "      The best days to drink beer are days that end in the letter, 'Y'\n\r", ch );
		break;
	case 5:
		send_to_char( "      Pain is only weakness leaving the body\n\r", ch );
		break;
	case 6:
		send_to_char( "      Trans corpus meum mortuum. - Over my dead body\n\r", ch );
		break;
	case 7:
		send_to_char( "                      \\=/, \n\r", ch );
		send_to_char( "                      |  @___oo \n\r", ch );
		send_to_char( "            /\\  /\\   / (___,,,} \n\r", ch );
		send_to_char( "          ) /^\\) ^\\/ _) \n\r", ch );
		send_to_char( "          )   /^\\/   _) \n\r", ch );
		send_to_char( "          )   _ /  / _) \n\r", ch );
		send_to_char( "       /\\  )/\\/ ||  | )_)            See you later, alligator\n\r", ch );
		send_to_char( "      <  >      |(,,) )__) \n\r", ch );
		send_to_char( "       ||      /    \\)___)\\ \n\r", ch );
		send_to_char( "       | \\____(      )___) )___ \n\r", ch );
		send_to_char( "        \\______(_______;;; __;;; \n\r", ch );
		break;
	case 8:
		send_to_char( "      To HELL with the Prime Directive.... FIRE!!! - Kirk\n\r", ch );
		break;
	case 9:
		send_to_char( "      You, in the red uniform, go see what that noise is!\n\r", ch );
		break;
	case 10:
		send_to_char( "      C.O.B.O.L - Completely Obsolete Boring Old Language\n\r", ch );
		break;
	}
	/*
	 * After extract_char the ch is no longer valid!
	 */

	if ( ch->pcdata->in_progress )
		free_note( ch->pcdata->in_progress );

	d = ch->desc;
	/* Force save (bypass rate limit) and wait for background write */
	ch->save_time = 0;
	save_char_obj( ch );
	db_player_wait_pending();  /* Wait for all pending saves to complete */
	if ( ch->pcdata->obj_vnum != 0 )
		act( "$n slowly fades out of existance.", ch, NULL, NULL, TO_ROOM );
	else
		act( "$n has left the game.", ch, NULL, NULL, TO_ROOM );

	if ( d != NULL )
		close_socket2( d, FALSE );

	if ( ch->in_room != NULL ) char_from_room( ch );
	char_to_room( ch, get_room_index( 3 ) );

	snprintf( log_buf, MAX_STRING_LENGTH, "%s has quit.", ch->name );
	log_string( log_buf );
	if ( ch->pcdata->chobj != NULL ) extract_obj( ch->pcdata->chobj );
	extract_char( ch, TRUE );
	return;
}

void do_save( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) )
		return;

	if ( ch->level < 2 ) {
		send_to_char( "You must kill at least 5 mobs before you can save.\n\r", ch );
		return;
	}
	/* Reset save_time to force through rate limit for manual saves */
	ch->save_time = 0;
	save_char_obj( ch );
	save_char_obj_backup( ch );
	send_to_char( "Saved.\n\r", ch );
	return;
}

void do_autosave( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) ) return;
	if ( ch->level < 2 ) return;
	save_char_obj( ch );
	return;
}

void do_follow( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		send_to_char( "Follow whom?\n\r", ch );
		return;
	}
	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master != NULL ) {
		act( "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
		return;
	}
	if ( victim == ch ) {
		if ( ch->master == NULL ) {
			send_to_char( "You already follow yourself.\n\r", ch );
			return;
		}
		stop_follower( ch );
		return;
	}

	if ( ch->master != NULL )
		stop_follower( ch );

	add_follower( ch, victim );
	return;
}

void add_follower( CHAR_DATA *ch, CHAR_DATA *master ) {
	if ( ch->master != NULL ) {
		bug( "Add_follower: non-null master.", 0 );
		return;
	}

	ch->master = master;
	ch->leader = NULL;

	if ( can_see( master, ch ) )
		act( "$n now follows you.", ch, NULL, master, TO_VICT );

	act( "You now follow $N.", ch, NULL, master, TO_CHAR );

	return;
}

void stop_follower( CHAR_DATA *ch ) {
	if ( ch->master == NULL ) {
		bug( "Stop_follower: null master.", 0 );
		return;
	}

	if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
		REMOVE_BIT( ch->affected_by, AFF_CHARM );
		affect_strip( ch, gsn_charm_person );
	}

	if ( can_see( ch->master, ch ) )
		act( "$n stops following you.", ch, NULL, ch->master, TO_VICT );
	act( "You stop following $N.", ch, NULL, ch->master, TO_CHAR );

	ch->master = NULL;
	ch->leader = NULL;
	return;
}

void die_follower( CHAR_DATA *ch ) {
	CHAR_DATA *fch;

	if ( ch->master != NULL )
		stop_follower( ch );

	ch->leader = NULL;

	LIST_FOR_EACH( fch, &g_characters, CHAR_DATA, char_node ) {
		if ( fch->master == ch )
			stop_follower( fch );
		if ( fch->leader == ch )
			fch->leader = fch;
	}

	return;
}

void do_order( CHAR_DATA *ch, char *argument ) {

	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *och;
	CHAR_DATA *och_next;
	bool found;
	bool fAll;

	argument = one_argument( argument, arg );

	if ( arg[0] == '\0' || argument[0] == '\0' ) {
		send_to_char( "Order whom to do what?\n\r", ch );
		return;
	}

	if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
		send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
		return;
	}

	if ( IS_SET( ch->in_room->room_flags, ROOM_ORDER ) || IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
		stc( "You can't order things around here.\n\r", ch );
		return;
	}

	if ( !str_cmp( arg, "all" ) ) {
		send_to_char( "Ordering 'all' has been disabled.\n\r", ch );
		return;
	} else {
		fAll = FALSE;
		if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
			send_to_char( "They aren't here.\n\r", ch );
			return;
		}

		if ( victim == ch ) {
			send_to_char( "Aye aye, right away!\n\r", ch );
			return;
		}

		if ( ( !IS_AFFECTED( victim, AFF_CHARM ) || victim->master != ch ) )

		{
			send_to_char( "Do it yourself!\n\r", ch );
			return;
		}

		if ( IS_CLASS( ch, CLASS_VAMPIRE ) && IS_CLASS( victim, CLASS_VAMPIRE ) && ( ( ch->pcdata->stats[UNI_GEN] > 2 ) || ch->pcdata->kingdom != victim->pcdata->kingdom ) ) {
			act( "$N ignores your order.", ch, NULL, victim, TO_CHAR );
			act( "You ignore $n's order.", ch, NULL, victim, TO_VICT );
			return;
		}
	}

	found = FALSE;
	LIST_FOR_EACH_SAFE( och, och_next, &ch->in_room->characters, CHAR_DATA, room_node ) {
		if ( och == ch ) continue;

		if ( ( IS_AFFECTED( och, AFF_CHARM ) && och->master == ch && ( fAll || och == victim ) ) || ( ch->pcdata->stats[UNI_GEN] == 2 && ( fAll || och == victim ) && ch->pcdata->kingdom == och->pcdata->kingdom ) ) {
			found = TRUE;
			act( "$n orders you to '$t'.", ch, argument, och, TO_VICT );
			interpret( och, argument );
		} else if ( !IS_NPC( ch ) && !IS_NPC( och ) && ( fAll || och == victim ) && IS_CLASS( ch, CLASS_VAMPIRE ) && IS_CLASS( och, CLASS_VAMPIRE ) && ch->pcdata->stats[UNI_GEN] < och->pcdata->stats[UNI_GEN] &&
			ch->pcdata->kingdom == och->pcdata->kingdom ) {
			found = TRUE;
			act( "$n orders you to '$t'.", ch, argument, och, TO_VICT );
			interpret( och, argument );
		}
	}

	if ( found )
		send_to_char( "Ok.\n\r", ch );
	else
		send_to_char( "You have no followers here.\n\r", ch );
	WAIT_STATE( ch, 12 );
	return;
}

void do_command( CHAR_DATA *ch, char *argument ) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char buffy[MAX_INPUT_LENGTH * 2]; /* arg2 + argument */
	CHAR_DATA *victim;
	int awe;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );

	if ( IS_NPC( ch ) ) return;
	if ( !IS_CLASS( ch, CLASS_VAMPIRE ) && !IS_CLASS( ch, CLASS_UNDEAD_KNIGHT ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	if ( ch->power[DISC_VAMP_DOMI] < 1 && IS_CLASS( ch, CLASS_VAMPIRE ) ) {
		send_to_char( "You must obtain at least level 1 in Dominate to use Command.\n\r", ch );
		return;
	}
	if ( IS_CLASS( ch, CLASS_UNDEAD_KNIGHT ) && ch->pcdata->powers[NECROMANCY] < 4 ) {
		send_to_char( "You need level 4 necromancry to use command.\n\r", ch );
		return;
	}
	if ( ch->spl[RED_MAGIC] < 1 ) {
		send_to_char( "Your mind is too weak.\n\r", ch );
		return;
	}
	if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
		send_to_char( "Command whom to do what?\n\r", ch );
		return;
	}
	if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}
	if ( IS_AFFECTED( victim, AFF_CHARM ) ) {
		send_to_char( "Their mind is controlled by someone else.\n\r", ch );
		return;
	}
	if ( IS_SET( ch->in_room->room_flags, ROOM_ORDER ) || IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
		stc( "You can't order things around here.\n\r", ch );
		return;
	}
	if ( victim == ch ) {
		send_to_char( "How can you command yourself??\n\r", ch );
		return;
	}

	if ( ch->power[DISC_VAMP_DOMI] > 2 )
		awe = 50;
	else if ( ch->power[DISC_VAMP_DOMI] > 3 )
		awe = 75;
	else if ( ch->power[DISC_VAMP_DOMI] > 4 )
		awe = 100;
	else
		awe = 25;

	if ( IS_EXTRA( ch, EXTRA_AWE ) ) {
		awe += 75;
	}

	if ( !IS_NPC( victim ) && victim->level != 3 ) {
		send_to_char( "You can only command other avatars.\n\r", ch );
		return;
	}

	if ( ch->power[DISC_VAMP_DOMI] > 1 ) {
		snprintf( buffy, sizeof( buffy ), "%s %s", arg2, argument );
		if ( IS_NPC( victim ) )
			snprintf( buf, sizeof( buf ), "I think %s wants to %s", victim->short_descr, buffy );
		else if ( !IS_NPC( victim ) && IS_AFFECTED( victim, AFF_POLYMORPH ) )
			snprintf( buf, sizeof( buf ), "I think %s wants to %s", victim->morph, buffy );
		else
			snprintf( buf, sizeof( buf ), "I think %s wants to %s", victim->name, buffy );
		do_say( ch, buf );
	} else {
		if ( IS_NPC( victim ) )
			snprintf( buf, sizeof( buf ), "I think %s wants to %s", victim->short_descr, arg2 );
		else if ( !IS_NPC( victim ) && IS_AFFECTED( victim, AFF_POLYMORPH ) )
			snprintf( buf, sizeof( buf ), "I think %s wants to %s", victim->morph, arg2 );
		else
			snprintf( buf, sizeof( buf ), "I think %s wants to %s", victim->name, arg2 );
		do_say( ch, buf );
	}

	if ( IS_NPC( victim ) && ( victim->level >= awe * ch->spl[RED_MAGIC] * 2 || victim->level > 500 ) ) {
		act( "You shake off $N's suggestion.", victim, NULL, ch, TO_CHAR );
		act( "$n shakes off $N's suggestion.", victim, NULL, ch, TO_NOTVICT );
		act( "$n shakes off your suggestion.", victim, NULL, ch, TO_VICT );
		act( "$s mind is too strong to overcome.", victim, NULL, ch, TO_VICT );
		return;
	}

	else if ( victim->spl[BLUE_MAGIC] >= ( ch->spl[RED_MAGIC] / 2 ) ) {
		act( "You shake off $N's suggestion.", victim, NULL, ch, TO_CHAR );
		act( "$n shakes off $N's suggestion.", victim, NULL, ch, TO_NOTVICT );
		act( "$n shakes off your suggestion.", victim, NULL, ch, TO_VICT );
		act( "$s mind is too strong to overcome.", victim, NULL, ch, TO_VICT );
		return;
	}

	act( "You blink in confusion.", victim, NULL, NULL, TO_CHAR );
	act( "$n blinks in confusion.", victim, NULL, NULL, TO_ROOM );
	strcpy( buf, "Yes, you're right, I do..." );
	do_say( victim, buf );
	if ( ch->power[DISC_VAMP_DOMI] > 1 )
		interpret( victim, buffy );
	else
		interpret( victim, arg2 );
	WAIT_STATE( ch, 4 );
	return;
}

void do_group( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument( argument, arg );

	if ( arg[0] == '\0' ) {
		CHAR_DATA *gch;
		CHAR_DATA *leader;

		leader = ( ch->leader != NULL ) ? ch->leader : ch;
		snprintf( buf, sizeof( buf ), "%s's group:\n\r", PERS( leader, ch ) );
		send_to_char( buf, ch );

		LIST_FOR_EACH( gch, &g_characters, CHAR_DATA, char_node ) {
			if ( is_same_group( gch, ch ) ) {
				snprintf( buf, sizeof( buf ),
					"[%-16s] %4d/%4d hp %4d/%4d mana %4d/%4d mv %5d xp\n\r",
					capitalize( PERS( gch, ch ) ),
					gch->hit, gch->max_hit,
					gch->mana, gch->max_mana,
					gch->move, gch->max_move,
					gch->exp );
				send_to_char( buf, ch );
			}
		}
		return;
	}

	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	}

	if ( ch->master != NULL || ( ch->leader != NULL && ch->leader != ch ) ) {
		send_to_char( "But you are following someone else!\n\r", ch );
		return;
	}

	if ( victim->master != ch && ch != victim ) {
		act( "$N isn't following you.", ch, NULL, victim, TO_CHAR );
		return;
	}

	if ( is_same_group( victim, ch ) && ch != victim ) {
		victim->leader = NULL;
		act( "$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT );
		act( "$n removes you from $s group.", ch, NULL, victim, TO_VICT );
		act( "You remove $N from your group.", ch, NULL, victim, TO_CHAR );
		return;
	}

	victim->leader = ch;
	act( "$N joins $n's group.", ch, NULL, victim, TO_NOTVICT );
	act( "You join $n's group.", ch, NULL, victim, TO_VICT );
	act( "$N joins your group.", ch, NULL, victim, TO_CHAR );
	return;
}

void do_gtell( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *gch;

	if ( argument[0] == '\0' ) {
		send_to_char( "Tell your group what?\n\r", ch );
		return;
	}

	if ( IS_SET( ch->act, PLR_NO_TELL ) ) {
		send_to_char( "Your message didn't get through!\n\r", ch );
		return;
	}

	/*
	 * Note use of send_to_char, so gtell works on sleepers.
	 */
	snprintf( buf, sizeof( buf ), "#G%s tells the group #R'#G%s#R'\n\r#n", ch->name, argument );
	//    LIST_FOR_EACH( gch, &g_characters, CHAR_DATA, char_node )
	LIST_FOR_EACH( d, &g_descriptors, DESCRIPTOR_DATA, node ) {
		if ( d->character != NULL )
			gch = d->character;
		else
			continue;
		if ( is_same_group( gch, ch ) )
			send_to_char( buf, gch );
	}

	return;
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch ) {
	if ( ach->leader != NULL ) ach = ach->leader;
	if ( bch->leader != NULL ) bch = bch->leader;
	return ach == bch;
}

void do_telepath( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_SHAPESHIFTER ) ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_TELEPATH, "telepaths" );
	return;
}

void do_communicate( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) || ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, CLASS_DROID ) ) ) {
		stc( "Huh?\n\r", ch );
		return;
	}
	talk_channel( ch, argument, CHANNEL_COMMUNICATE, "communicate" );
	return;
}

/*
 * Show MUD protocol status for the connection
 */
void do_protocols( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char naws_status[64];
	char ttype_status[128];
	char padded_name[128];
	char padded_desc[128];
	char padded_status[128];
	char margin[64];
	char border[128];
	const char *title = "#CMUD Protocol Status#n";
	int term_width, inner_width, outer_width, left_pad;
	int max_name, max_desc, max_status;
	int title_visible, title_left, title_right;
	int i, num_rows;

	struct {
		const char *name;
		const char *desc;
		const char *status;
	} rows[7];

	if ( ch->desc == NULL ) {
		send_to_char( "No descriptor.\n\r", ch );
		return;
	}

	/* Populate protocol rows */
	rows[0].name = "#yMCCP#n";
	rows[0].desc = "Compression";
	if ( ch->desc->out_compress != NULL ) {
		if ( ch->desc->mccp_version == 2 )
			rows[0].status = "#GOn (v2)#n";
		else if ( ch->desc->mccp_version == 1 )
			rows[0].status = "#GOn (v1)#n";
		else
			rows[0].status = "#GOn#n";
	} else
		rows[0].status = "#rOff#n";

	rows[1].name = "#yGMCP#n";
	rows[1].desc = "Data Channel";
	rows[1].status = ch->desc->gmcp_enabled ? "#GOn#n" : "#rOff#n";

	rows[2].name = "#yMCMP#n";
	rows[2].desc = "Client.Media";
	rows[2].status = mcmp_enabled( ch->desc ) ? "#GOn#n" : "#rOff#n";

	rows[3].name = "#yMXP#n";
	rows[3].desc = "Extensions";
	rows[3].status = ch->desc->mxp_enabled ? "#GOn#n" : "#rOff#n";

	rows[4].name = "#yMSSP#n";
	rows[4].desc = "Server Status";
	rows[4].status = "#yAvail#n";

	rows[5].name = "#yNAWS#n";
	rows[5].desc = "Window Size";
	if ( ch->desc->naws_enabled )
		snprintf( naws_status, sizeof( naws_status ), "#GOn#n (%dx%d)",
			ch->desc->client_width, ch->desc->client_height );
	else
		snprintf( naws_status, sizeof( naws_status ), "#rOff#n" );
	rows[5].status = naws_status;

	rows[6].name = "#yTTYPE#n";
	rows[6].desc = "Terminal Type";
	if ( ch->desc->ttype_enabled )
		snprintf( ttype_status, sizeof( ttype_status ), "#GOn#n (%s)",
			ch->desc->client_name[0] ? ch->desc->client_name : "?" );
	else
		snprintf( ttype_status, sizeof( ttype_status ), "#rOff#n" );
	rows[6].status = ttype_status;

	num_rows = 7;

	/* Calculate column widths from content */
	max_name = max_desc = max_status = 0;
	for ( i = 0; i < num_rows; i++ ) {
		int vn = visible_strlen( rows[i].name );
		int vd = visible_strlen( rows[i].desc );
		int vs = visible_strlen( rows[i].status );
		if ( vn > max_name ) max_name = vn;
		if ( vd > max_desc ) max_desc = vd;
		if ( vs > max_status ) max_status = vs;
	}

	/* inner_width: 2 + name + 2 + desc + 3 + status + 2 */
	inner_width = 2 + max_name + 2 + max_desc + 3 + max_status + 2;
	outer_width = inner_width + 2; /* for | borders */

	/* Ensure title fits */
	title_visible = visible_strlen( title );
	if ( inner_width < title_visible + 4 )
		inner_width = title_visible + 4;
	outer_width = inner_width + 2;

	/* Center table in terminal */
	term_width = naws_get_width( ch );
	left_pad = ( term_width - outer_width ) / 2;
	if ( left_pad < 0 )
		left_pad = 0;
	if ( left_pad > (int)sizeof( margin ) - 1 )
		left_pad = (int)sizeof( margin ) - 1;
	for ( i = 0; i < left_pad; i++ )
		margin[i] = ' ';
	margin[left_pad] = '\0';

	/* Build border: +---...---+ */
	border[0] = '+';
	for ( i = 1; i <= inner_width; i++ )
		border[i] = '-';
	border[inner_width + 1] = '+';
	border[inner_width + 2] = '\0';

	/* Title centering within inner width */
	title_left = ( inner_width - title_visible ) / 2;
	title_right = inner_width - title_visible - title_left;

	/* Top border */
	snprintf( buf, sizeof( buf ), "%s#w%s#n\n\r", margin, border );
	send_to_char( buf, ch );

	/* Title row */
	snprintf( buf, sizeof( buf ), "%s#w|#n%*s%s%*s#w|#n\n\r",
		margin, title_left, "", title, title_right, "" );
	send_to_char( buf, ch );

	/* Middle border */
	snprintf( buf, sizeof( buf ), "%s#w%s#n\n\r", margin, border );
	send_to_char( buf, ch );

	/* Data rows */
	for ( i = 0; i < num_rows; i++ ) {
		pad_to_visible_width( padded_name, sizeof( padded_name ),
			rows[i].name, max_name );
		pad_to_visible_width( padded_desc, sizeof( padded_desc ),
			rows[i].desc, max_desc );
		pad_to_visible_width( padded_status, sizeof( padded_status ),
			rows[i].status, max_status );

		snprintf( buf, sizeof( buf ), "%s#w|#n  %s  %s   %s  #w|#n\n\r",
			margin, padded_name, padded_desc, padded_status );
		send_to_char( buf, ch );
	}

	/* Bottom border */
	snprintf( buf, sizeof( buf ), "%s#w%s#n\n\r", margin, border );
	send_to_char( buf, ch );

	/* Show additional GMCP details if enabled */
	if ( ch->desc->gmcp_enabled && ch->desc->gmcp_packages > 0 ) {
		snprintf( buf, sizeof( buf ),
			"\n\r#yGMCP Packages:#n %s%s%s%s%s%s\n\r",
			( ch->desc->gmcp_packages & GMCP_PACKAGE_CORE ) ? "Core " : "",
			( ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR ) ? "Char " : "",
			( ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_VITALS ) ? "Char.Vitals " : "",
			( ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_STATUS ) ? "Char.Status " : "",
			( ch->desc->gmcp_packages & GMCP_PACKAGE_CHAR_INFO ) ? "Char.Info " : "",
			( ch->desc->gmcp_packages & GMCP_PACKAGE_CLIENT_MEDIA ) ? "Client.Media " : "" );
		send_to_char( buf, ch );
	}
}
