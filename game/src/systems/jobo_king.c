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

/***********************************
 * Dystopian Kingdom Code, by Jobo *
 ***********************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "../db/db_game.h"

KINGDOM_DATA kingdom_table[MAX_KINGDOM + 1];

void imm_kset ( CHAR_DATA * ch, char *argument );

void save_kingdoms() {
	db_game_save_kingdoms();
}

void load_kingdoms() {
	int i;

	kingdom_table[0].name = "";
	kingdom_table[0].whoname = "";
	kingdom_table[0].leader = "";
	kingdom_table[0].general = "";
	kingdom_table[0].kills = 0;
	kingdom_table[0].deaths = 0;
	kingdom_table[0].qps = 0;
	kingdom_table[0].req_hit = 0;
	kingdom_table[0].req_move = 0;
	kingdom_table[0].req_mana = 0;
	kingdom_table[0].req_qps = 0;

	for ( i = 1; i <= MAX_KINGDOM; i++ ) {
		kingdom_table[i].name = str_dup( "None" );
		kingdom_table[i].whoname = str_dup( "None" );
		kingdom_table[i].leader = str_dup( "None" );
		kingdom_table[i].general = str_dup( "None" );
		kingdom_table[i].kills = 0;
		kingdom_table[i].deaths = 0;
		kingdom_table[i].qps = 0;
		kingdom_table[i].req_hit = 0;
		kingdom_table[i].req_move = 0;
		kingdom_table[i].req_mana = 0;
		kingdom_table[i].req_qps = 0;
	}
	db_game_load_kingdoms();
}

void do_kingdoms( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char pkratio[20]; // perhaps some colors :)
	int i;

	if ( IS_NPC( ch ) ) return;

	snprintf( buf, sizeof( buf ), "Name          Pkills  Pdeaths  Ratio   REQS : hps    move    mana     qps\n\r" );
	for ( i = 1; i <= MAX_KINGDOM; i++ ) {
		/*
		 * calculation ratio
		 */
		if ( kingdom_table[i].kills > 0 ) {
			if ( 100 * kingdom_table[i].kills / ( kingdom_table[i].kills + kingdom_table[i].deaths ) < 100 )
				snprintf( pkratio, sizeof( pkratio ), "0.%-2d",
					( 100 * kingdom_table[i].kills / ( kingdom_table[i].kills + kingdom_table[i].deaths ) ) );
			else
				snprintf( pkratio, sizeof( pkratio ), "1.00" );
		} else
			snprintf( pkratio, sizeof( pkratio ), "0.00" );

		/*
		 * buffering everything
		 */
		snprintf( buf2, sizeof( buf2 ), "%-14s  %3d      %3d    %s        %5d   %5d   %5d   %5d\n\r",
			kingdom_table[i].name, kingdom_table[i].kills, kingdom_table[i].deaths, pkratio,
			kingdom_table[i].req_hit, kingdom_table[i].req_move, kingdom_table[i].req_mana, kingdom_table[i].req_qps );
		strcat( buf, buf2 );
	}
	send_to_char( buf, ch );
	return;
}

void do_kinduct( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	one_argument( argument, arg );

	if ( IS_NPC( ch ) ) return;
	if ( ch->pcdata->kingdom == 0 ) {
		send_to_char( "What kingdoms did you say you where from ?\n\r", ch );
		return;
	}
	if ( str_cmp( kingdom_table[ch->pcdata->kingdom].leader, ch->name ) &&
		str_cmp( kingdom_table[ch->pcdata->kingdom].general, ch->name ) ) {
		send_to_char( "Talk to your kingdom leaders, they are the only persons allowed to induct new members.\n\r", ch );
		return;
	}
	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "They are not here.\n\r", ch );
		return;
	}
	if ( IS_NPC( victim ) ) {
		send_to_char( "That's silly.\n\r", ch );
		return;
	}
	if ( victim->pcdata->kingdom != 0 ) {
		send_to_char( "They are already a member of another kingdom.\n\r", ch );
		return;
	}
	if ( victim->max_hit < kingdom_table[ch->pcdata->kingdom].req_hit ||
		victim->max_move < kingdom_table[ch->pcdata->kingdom].req_move ||
		victim->pcdata->quest < kingdom_table[ch->pcdata->kingdom].req_qps ||
		victim->max_mana < kingdom_table[ch->pcdata->kingdom].req_mana ) {
		send_to_char( "They don't have the reqs to get inducted.\n\r", ch );
		return;
	}
	if ( !IS_SET( victim->pcdata->jflags, JFLAG_WANT_KINGDOM ) ) {
		send_to_char( "They don't want to be inducted.\n\r", ch );
		return;
	}
	victim->pcdata->kingdom = ch->pcdata->kingdom;
	victim->pcdata->quest -= kingdom_table[ch->pcdata->kingdom].req_qps;
	kingdom_table[ch->pcdata->kingdom].qps += kingdom_table[ch->pcdata->kingdom].req_qps;
	send_to_char( "You induct them into your kingdom.\n\r", ch );
	send_to_char( "You have been inducted.\n\r", victim );
	return;
}

void do_wantkingdom( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) ) return;

	if ( IS_SET( ch->pcdata->jflags, JFLAG_WANT_KINGDOM ) ) {
		REMOVE_BIT( ch->pcdata->jflags, JFLAG_WANT_KINGDOM );
		send_to_char( "You can no longer be inducted.\n\r", ch );
		return;
	}
	SET_BIT( ch->pcdata->jflags, JFLAG_WANT_KINGDOM );
	send_to_char( "You can now be inducted.\n\r", ch );
	return;
}

void do_kset( CHAR_DATA *ch, char *argument ) {
	char keyword[MAX_INPUT_LENGTH];
	char value[MAX_INPUT_LENGTH];

	if ( IS_NPC( ch ) ) return;
	if ( ch->level > 6 ) {
		imm_kset( ch, argument );
		return;
	}
	argument = one_argument( argument, keyword );
	one_argument( argument, value );
	if ( ch->pcdata->kingdom == 0 ) {
		send_to_char( "But you are not a member of a kingdom.\n\r", ch );
		return;
	}
	if ( str_cmp( kingdom_table[ch->pcdata->kingdom].leader, ch->name ) ) {
		send_to_char( "Only the leader can change the kingdom settings.\n\r", ch );
		return;
	}
	if ( keyword[0] == '\0' || value[0] == '\0' ) {
		send_to_char( "What do you want to change ?\n\r\n\r", ch );
		send_to_char( "req_mana, req_hit, req_move, req_qps or general.\n\r", ch );
		return;
	}
	if ( !str_cmp( keyword, "req_mana" ) ) {
		kingdom_table[ch->pcdata->kingdom].req_mana = atoi( value );
	} else if ( !str_cmp( keyword, "req_qps" ) ) {
		kingdom_table[ch->pcdata->kingdom].req_qps = atoi( value );
	} else if ( !str_cmp( keyword, "req_hit" ) ) {
		kingdom_table[ch->pcdata->kingdom].req_hit = atoi( value );
	} else if ( !str_cmp( keyword, "req_move" ) ) {
		kingdom_table[ch->pcdata->kingdom].req_move = atoi( value );
	} else if ( !str_cmp( keyword, "general" ) ) {
		free(kingdom_table[ch->pcdata->kingdom].general);
		kingdom_table[ch->pcdata->kingdom].general = str_dup( value );
	} else {
		do_kset( ch, "" );
		return;
	}
	send_to_char( "Done.\n\r", ch );
	save_kingdoms();
	return;
}

void imm_kset( CHAR_DATA *ch, char *argument ) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	int i;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	one_argument_case( argument, arg3 );

	if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ) {
		send_to_char( "Syntax       : kset <kingdom> <field> <value>\n\r", ch );
		send_to_char( "Valid fields : leader, name, whoname, general, kills, deaths\n\r", ch );
		return;
	}
	if ( !is_number( arg1 ) ) {
		send_to_char( "Please pick a number as the kingdom.\n\r", ch );
		return;
	}
	i = atoi( arg1 );
	if ( i < 1 || i > MAX_KINGDOM ) {
		send_to_char( "Please pick a real kingdom.\n\r", ch );
		return;
	}
	if ( !str_cmp( arg2, "leader" ) ) {
		free(kingdom_table[i].leader);
		arg3[0] = UPPER( arg3[0] );
		kingdom_table[i].leader = str_dup( arg3 );
	} else if ( !str_cmp( arg2, "name" ) ) {
		free(kingdom_table[i].name);
		arg3[0] = UPPER( arg3[0] );
		kingdom_table[i].name = str_dup( arg3 );
	} else if ( !str_cmp( arg2, "whoname" ) ) {
		free(kingdom_table[i].whoname);
		kingdom_table[i].whoname = str_dup( arg3 );
	} else if ( !str_cmp( arg2, "general" ) ) {
		free(kingdom_table[i].general);
		arg3[0] = UPPER( arg3[0] );
		kingdom_table[i].general = str_dup( arg3 );
	} else if ( !str_cmp( arg2, "kills" ) ) {
		kingdom_table[i].kills = atoi( arg3 );
	} else if ( !str_cmp( arg2, "deaths" ) ) {
		kingdom_table[i].deaths = atoi( arg3 );
	} else {
		imm_kset( ch, "" );
		return;
	}
	send_to_char( "Done.\n\r", ch );
	save_kingdoms();
	return;
}

void do_koutcast( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	one_argument( argument, arg );

	if ( IS_NPC( ch ) ) return;
	if ( ch->pcdata->kingdom == 0 ) {
		send_to_char( "What kingdoms did you say you where from ?\n\r", ch );
		return;
	}
	if ( str_cmp( kingdom_table[ch->pcdata->kingdom].leader, ch->name ) &&
		str_cmp( kingdom_table[ch->pcdata->kingdom].general, ch->name ) ) {
		send_to_char( "You are not allowed to outcast members.\n\r", ch );
		return;
	}
	if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
		send_to_char( "Outcast whom?\n\r", ch );
		return;
	}
	if ( IS_NPC( victim ) ) {
		send_to_char( "That's a monster, not a player.\n\r", ch );
		return;
	}
	if ( ch == victim ) {
		send_to_char( "You cannot outcast yourself.\n\r", ch );
		return;
	}
	if ( victim->pcdata->kingdom != ch->pcdata->kingdom ) {
		send_to_char( "They are not a member of your kingdom.\n\r", ch );
		return;
	}
	if ( !str_cmp( victim->name, kingdom_table[ch->pcdata->kingdom].leader ) ) {
		send_to_char( "That is not a good plan.\n\r", ch );
		return;
	}
	victim->pcdata->kingdom = 0;
	send_to_char( "Done.\n\r", ch );
	send_to_char( "You have been outcasted from your kingdom.\n\r", victim );
	return;
}

void do_kleave( CHAR_DATA *ch, char *argument ) {
	DESCRIPTOR_DATA *d;
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int kingdom;

	one_argument( argument, arg );

	if ( IS_NPC( ch ) ) return;

	if ( ch->pcdata->kingdom == 0 ) {
		send_to_char( "You are not a member of any kingdom.\n\r", ch );
		return;
	}

	/* Require confirmation */
	if ( str_cmp( arg, "confirm" ) ) {
		send_to_char( "Type 'kleave confirm' to leave your kingdom.\n\r", ch );

		/* Warn leaders and generals */
		if ( !str_cmp( ch->name, kingdom_table[ch->pcdata->kingdom].leader ) )
			send_to_char( "#RWARNING: You are the kingdom leader!#n\n\r", ch );
		else if ( !str_cmp( ch->name, kingdom_table[ch->pcdata->kingdom].general ) )
			send_to_char( "#RWARNING: You are the kingdom general!#n\n\r", ch );
		return;
	}

	kingdom = ch->pcdata->kingdom;

	/* Notify kingdom members before leaving */
	snprintf( buf, sizeof( buf ), "#R%s has left the kingdom.#n\n\r", ch->name );
	LIST_FOR_EACH( d, &g_descriptors, DESCRIPTOR_DATA, node ) {
		if ( d->connected == CON_PLAYING && d->character != NULL
			&& !IS_NPC( d->character )
			&& d->character->pcdata->kingdom == kingdom
			&& d->character != ch )
		{
			send_to_char( buf, d->character );
		}
	}

	/* Leave the kingdom */
	ch->pcdata->kingdom = 0;
	send_to_char( "You have left the kingdom.\n\r", ch );
	return;
}

void do_kstats( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	int i;

	if ( IS_NPC( ch ) ) return;
	if ( ( i = ch->pcdata->kingdom ) == 0 ) {
		send_to_char( "You are not a member of any kingdom.\n\r", ch );
		return;
	}
	snprintf( buf, sizeof( buf ), " [***]  The Kingdom stats of %s  [***]\n\r\n\r", kingdom_table[i].whoname );
	snprintf( buf2, sizeof( buf2 ), " Current Leader    : %s\n\r", kingdom_table[i].leader );
	strcat( buf, buf2 );
	snprintf( buf2, sizeof( buf2 ), " Current General   : %s\n\r", kingdom_table[i].general );
	strcat( buf, buf2 );
	snprintf( buf2, sizeof( buf2 ), " Treasury          : %d qps\n\r", kingdom_table[i].qps );
	strcat( buf, buf2 );
	send_to_char( buf, ch );
	return;
}

void do_kingset( CHAR_DATA *ch, char *argument ) {
	CHAR_DATA *victim;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int i;

	if ( IS_NPC( ch ) || ch->level < 7 ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}
	argument = one_argument( argument, arg1 );
	one_argument( argument, arg2 );
	if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
		send_to_char( "Syntax : kingset <player> <kingdom number>\n\r", ch );
		return;
	}
	if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
		send_to_char( "They are not here.\n\r", ch );
		return;
	}
	if ( ( i = atoi( arg2 ) ) < 1 || i > MAX_KINGDOM ) {
		send_to_char( "Please pick a valid kingdom.\n\r", ch );
		return;
	}
	if ( IS_NPC( victim ) ) {
		send_to_char( "Please pick a player.\n\r", ch );
		return;
	}
	victim->pcdata->kingdom = i;
	send_to_char( "Ok.\n\r", ch );
	send_to_char( "Your kingdom has been changed.\n\r", victim );
	return;
}
