/***************************************************************************
 *  Prompt and pager functions extracted from comm.c                        *
 *  Contains: bust_a_header, bust_a_prompt, show_string                     *
 ***************************************************************************/

#include "merc.h"

void bust_a_header( DESCRIPTOR_DATA *d ) {
	/* Disabled - header bar no longer used */
	return;
}
/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt( DESCRIPTOR_DATA *d ) {
	CHAR_DATA *ch;
	CHAR_DATA *victim;
	CHAR_DATA *tank;
	const char *str;
	const char *i;
	char *point;
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	bool is_fighting = TRUE;

	if ( ( ch = d->character ) == NULL ) return;
	if ( ch->pcdata == NULL ) {
		send_to_char( "\n\r\n\r", ch );
		return;
	}
	if ( ch->position == POS_FIGHTING && ch->cprompt[0] == '\0' ) {
		if ( ch->prompt[0] == '\0' ) {
			send_to_char( "\n\r\n\r", ch );
			return;
		}
		is_fighting = FALSE;
	} else if ( ch->position != POS_FIGHTING && ch->prompt[0] == '\0' ) {
		send_to_char( "\n\r\n\r", ch );
		return;
	}

	point = buf;
	if ( ch->position == POS_FIGHTING && is_fighting )
		str = d->original ? d->original->cprompt : d->character->cprompt;
	else
		str = d->original ? d->original->prompt : d->character->prompt;
	while ( *str != '\0' ) {
		if ( point >= buf + sizeof( buf ) - 10 )
			break;
		if ( *str != '%' ) {
			*point++ = *str++;
			continue;
		}
		++str;
		switch ( *str ) {
		default:
			i = " ";
			break;
		case 'h':
			snprintf( buf2, sizeof(buf2), "%s%d#n", col_scale_code(ch->hit, ch->max_hit), ch->hit );
			i = buf2;
			break;
		case 'H':
			snprintf( buf2, sizeof(buf2), "#C%d#n", ch->max_hit );
			i = buf2;
			break;
		case 'm':
			snprintf( buf2, sizeof(buf2), "%s%d#n", col_scale_code(ch->mana, ch->max_mana), ch->mana );
			i = buf2;
			break;
		case 'M':
			snprintf( buf2, sizeof(buf2), "#C%d#n", ch->max_mana );
			i = buf2;
			break;
		case 'v':
			snprintf( buf2, sizeof(buf2), "%s%d#n", col_scale_code(ch->move, ch->max_move), ch->move );
			i = buf2;
			break;
		case 'V':
			snprintf( buf2, sizeof(buf2), "#C%d#n", ch->max_move );
			i = buf2;
			break;
		case 'x': {
			char xp_tmp[32];
			add_commas_to_number( ch->exp, xp_tmp, sizeof( xp_tmp ) );
			snprintf( buf2, sizeof( buf2 ), "%s%s#n", col_scale_code( ch->exp, 10000000 ), xp_tmp );
			i = buf2;
			break;
		}
		case 'g':
			snprintf( buf2, sizeof(buf2), "#C%d#n", ch->gold );
			i = buf2;
			break;
		case 'q':
			snprintf( buf2, sizeof(buf2), "#C%d#n", ch->pcdata->quest );
			i = buf2;
			break;
		case 'f':
			if ( ( victim = ch->fighting ) == NULL ) {
				snprintf( buf2, sizeof( buf2 ), "#CN/A#n" );
			} else {
				if ( ( victim->hit * 100 / victim->max_hit ) < 25 ) {
					snprintf( buf2, sizeof( buf2 ), "#RAwful#n" );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 50 ) {
					snprintf( buf2, sizeof( buf2 ), "#LPoor#n" );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 75 ) {
					snprintf( buf2, sizeof( buf2 ), "#GFair#n" );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 100 ) {
					snprintf( buf2, sizeof( buf2 ), "#yGood#n" );
				} else {
					snprintf( buf2, sizeof( buf2 ), "#CPerfect#n" );
				}
			}
			i = buf2;
			break;
		case 'F':
			if ( ( victim = ch->fighting ) == NULL ) {
				snprintf( buf2, sizeof( buf2 ), "#CN/A#n" );
			} else if ( ( tank = victim->fighting ) == NULL ) {
				snprintf( buf2, sizeof( buf2 ), "#CN/A#n" );
			} else {
				if ( ( tank->hit * 100 / tank->max_hit ) < 25 ) {
					snprintf( buf2, sizeof( buf2 ), "#RAwful#n" );
				} else if ( ( tank->hit * 100 / tank->max_hit ) < 50 ) {
					snprintf( buf2, sizeof( buf2 ), "#LPoor#n" );
				} else if ( ( tank->hit * 100 / tank->max_hit ) < 75 ) {
					snprintf( buf2, sizeof( buf2 ), "#GFair#n" );
				} else if ( ( tank->hit * 100 / tank->max_hit ) < 100 ) {
					snprintf( buf2, sizeof( buf2 ), "#yGood#n" );
				} else {
					snprintf( buf2, sizeof( buf2 ), "#CPerfect#n" );
				}
			}
			i = buf2;
			break;
		case 'n':
			if ( ( victim = ch->fighting ) == NULL )
				snprintf( buf2, sizeof( buf2 ), "N/A" );
			else {
				if ( IS_AFFECTED( victim, AFF_POLYMORPH ) )
					snprintf( buf2, sizeof( buf2 ), "%s", victim->morph );
				else if ( IS_NPC( victim ) )
					snprintf( buf2, sizeof( buf2 ), "%s", victim->short_descr );
				else
					snprintf( buf2, sizeof( buf2 ), "%s", victim->name );
				buf2[0] = UPPER( buf2[0] );
			}
			i = buf2;
			break;
		case 'N':
			if ( ( victim = ch->fighting ) == NULL )
				snprintf( buf2, sizeof( buf2 ), "N/A" );
			else if ( ( tank = victim->fighting ) == NULL )
				snprintf( buf2, sizeof( buf2 ), "N/A" );
			else {
				if ( ch == tank )
					snprintf( buf2, sizeof( buf2 ), "You" );
				else if ( IS_AFFECTED( tank, AFF_POLYMORPH ) )
					snprintf( buf2, sizeof( buf2 ), "%s", tank->morph );
				else if ( IS_NPC( victim ) )
					snprintf( buf2, sizeof( buf2 ), "%s", tank->short_descr );
				else
					snprintf( buf2, sizeof( buf2 ), "%s", tank->name );
				buf2[0] = UPPER( buf2[0] );
			}
			i = buf2;
			break;
		case 'a':
			snprintf( buf2, sizeof( buf2 ), "#C%s#n", IS_GOOD( ch ) ? "good" : IS_EVIL( ch ) ? "evil"
																							   : "neutral" );
			i = buf2;
			break;
		case 'A':
			snprintf( buf2, sizeof( buf2 ), "#C%d#n", ch->alignment );
			i = buf2;
			break;
		case 't':
			snprintf( buf2, sizeof( buf2 ), "#C%d#n", ch->fight_timer );
			i = buf2;
			break;
		case 'k':
			if ( IS_CLASS( ch, CLASS_SHAPESHIFTER ) && !IS_NPC( ch ) ) {
				snprintf( buf2, sizeof( buf2 ), "#C%d#n", ch->pcdata->powers[SHAPE_COUNTER] );
				i = buf2;
				break;
			} else if ( IS_CLASS( ch, CLASS_SAMURAI ) && !IS_NPC( ch ) ) {
				snprintf( buf2, sizeof( buf2 ), "%d", ch->pcdata->powers[SAMURAI_FOCUS] );
				i = buf2;
				break;
			} else {
				snprintf( buf2, sizeof( buf2 ), " " );
				i = buf2;
				break;
			}
		case 'r':
			if ( ch->in_room )
				snprintf( buf2, sizeof( buf2 ), "#C%s#n", ch->in_room->name );
			else
				snprintf( buf2, sizeof( buf2 ), " " );
			i = buf2;
			break;
		case 'R':
			if ( !IS_NPC( ch ) && ( IS_CLASS( ch, CLASS_WEREWOLF ) || IS_CLASS( ch, CLASS_VAMPIRE ) || IS_CLASS( ch, CLASS_NINJA ) || IS_CLASS( ch, CLASS_DIRGESINGER ) || IS_CLASS( ch, CLASS_SIREN ) || IS_CLASS( ch, CLASS_PSION ) || IS_CLASS( ch, CLASS_MINDFLAYER ) || IS_CLASS( ch, CLASS_DRAGONKIN ) || IS_CLASS( ch, CLASS_WYRM ) || IS_CLASS( ch, CLASS_ARTIFICER ) || IS_CLASS( ch, CLASS_MECHANIST ) ) ) {
				snprintf( buf2, sizeof( buf2 ), "#r%d#n", ch->rage );
			} else
				snprintf( buf2, sizeof( buf2 ), "0" );
			i = buf2;
			break;
		case 'E':
			if ( !IS_NPC( ch ) && ch->pcdata != NULL &&
				( IS_CLASS( ch, CLASS_DRAGONKIN ) || IS_CLASS( ch, CLASS_WYRM ) ) ) {
				const char *attune_names[] = { "Fire", "Frost", "Storm", "Earth" };
				int attune = ch->pcdata->powers[0];  /* DRAGON_ATTUNEMENT */
				if ( attune < 0 || attune > 3 ) attune = 0;
				snprintf( buf2, sizeof( buf2 ), "#C%s#n", attune_names[attune] );
			} else
				snprintf( buf2, sizeof( buf2 ), " " );
			i = buf2;
			break;
		case 'G':
			if ( !IS_NPC( ch ) && IS_CLASS( ch, CLASS_WEREWOLF ) && ch->gnosis[GMAXIMUM] > 0 ) {
				snprintf( buf2, sizeof( buf2 ), "%s%d#n", col_scale_code( ch->gnosis[GCURRENT], ch->gnosis[GMAXIMUM] ), ch->gnosis[GCURRENT] );
			} else
				snprintf( buf2, sizeof( buf2 ), "0" );
			i = buf2;
			break;
		case 'i':
			if ( !IS_NPC( ch ) && IS_CLASS( ch, CLASS_MONK ) && ch->chi[MAXIMUM] > 0 ) {
				snprintf( buf2, sizeof( buf2 ), "%s%d#n", col_scale_code( ch->chi[CURRENT], ch->chi[MAXIMUM] ), ch->chi[CURRENT] );
			} else
				snprintf( buf2, sizeof( buf2 ), "0" );
			i = buf2;
			break;
		case 'I':
			if ( !IS_NPC( ch ) && IS_CLASS( ch, CLASS_MONK ) ) {
				snprintf( buf2, sizeof( buf2 ), "#C%d#n", ch->chi[MAXIMUM] );
			} else
				snprintf( buf2, sizeof( buf2 ), "0" );
			i = buf2;
			break;
		case 'd':
			if ( !IS_NPC( ch ) && ch->pcdata != NULL &&
				( IS_CLASS( ch, CLASS_DEMON ) || IS_CLASS( ch, CLASS_DROW ) || IS_CLASS( ch, CLASS_TANARRI ) || IS_CLASS( ch, CLASS_DROID ) ) ) {
				snprintf( buf2, sizeof( buf2 ), "%s%d#n", col_scale_code( ch->pcdata->stats[8], ch->pcdata->stats[9] > 0 ? ch->pcdata->stats[9] : ch->pcdata->stats[8] ), ch->pcdata->stats[8] );
			} else
				snprintf( buf2, sizeof( buf2 ), "0" );
			i = buf2;
			break;
		case 'D':
			if ( !IS_NPC( ch ) && ch->pcdata != NULL &&
				( IS_CLASS( ch, CLASS_DEMON ) || IS_CLASS( ch, CLASS_DROW ) ) ) {
				snprintf( buf2, sizeof( buf2 ), "#C%d#n", ch->pcdata->stats[9] );
			} else
				snprintf( buf2, sizeof( buf2 ), "0" );
			i = buf2;
			break;
		case 'b':
			snprintf( buf2, sizeof( buf2 ), "#C%d#n", ch->beast );
			i = buf2;
			break;
		case 'B':
			if ( !IS_NPC( ch ) && IS_CLASS( ch, CLASS_VAMPIRE ) ) {
				snprintf( buf2, sizeof( buf2 ), "#r%d#n", ch->pcdata->condition[COND_THIRST] );
			} else if ( !IS_NPC( ch ) && IS_CLASS( ch, CLASS_SHAPESHIFTER ) ) {
				snprintf( buf2, sizeof( buf2 ), "#r%d#n", ch->pcdata->condition[COND_FULL] );
			} else
				snprintf( buf2, sizeof( buf2 ), "0" );
			i = buf2;
			break;
		case 'c':
			snprintf( buf2, sizeof( buf2 ), "%d", char_ac( ch ) );
			i = buf2;
			break;
		case 'p':
			snprintf( buf2, sizeof( buf2 ), "%s%d#n", col_scale_code( char_hitroll( ch ), 200 ), char_hitroll( ch ) );
			i = buf2;
			break;
		case 'P':
			snprintf( buf2, sizeof( buf2 ), "%s%d#n", col_scale_code( char_damroll( ch ), 200 ), char_damroll( ch ) );
			i = buf2;
			break;
		case 's':
			if ( !IS_NPC( ch ) && ch->pcdata->stage[2] + 25 >= ch->pcdata->stage[1] && ch->pcdata->stage[1] > 0 ) {
				snprintf( buf2, sizeof( buf2 ), "#Cyes#n" );
			} else
				snprintf( buf2, sizeof( buf2 ), "no" );
			i = buf2;
			break;
		case 'O':
			if ( ( victim = ch->pcdata->partner ) == NULL )
				snprintf( buf2, sizeof( buf2 ), "no" );
			else if ( !IS_NPC( victim ) && victim != NULL && victim->pcdata->stage[1] > 0 && victim->pcdata->stage[2] + 25 >= victim->pcdata->stage[1] ) {
				snprintf( buf2, sizeof( buf2 ), "#Cyes#n" );
			} else
				snprintf( buf2, sizeof( buf2 ), "no" );
			i = buf2;
			break;
		case 'l':
			if ( ( victim = ch->pcdata->partner ) == NULL )
				snprintf( buf2, sizeof( buf2 ), "Nobody" );
			else {
				if ( IS_AFFECTED( victim, AFF_POLYMORPH ) )
					snprintf( buf2, sizeof( buf2 ), "%s", victim->morph );
				else if ( IS_NPC( victim ) )
					snprintf( buf2, sizeof( buf2 ), "%s", victim->short_descr );
				else
					snprintf( buf2, sizeof( buf2 ), "%s", victim->name );
				buf2[0] = UPPER( buf2[0] );
			}
			i = buf2;
			break;
		case '%':
			snprintf( buf2, sizeof( buf2 ), "%%" );
			i = buf2;
			break;
		}
		++str;
		point = buf_append_safe( point, i, buf, sizeof( buf ), 10 );
		if ( point == NULL ) {
			point = buf + sizeof( buf ) - 10;
			break;
		}
	}
	*point = '\0';
	write_to_buffer( d, buf, (int) ( point - buf ) );
	return;
}

/* OLC, new pager for editing long descriptions. */
/* ========================================================================= */
/* - The heart of the pager.  Thanks to N'Atas-Ha, ThePrincedom for porting  */
/*   this SillyMud code for MERC 2.0 and laying down the groundwork.         */
/* - Thanks to Blackstar, hopper.cs.uiowa.edu 4000 for which the improvements*/
/*   to the pager was modeled from.  - Kahn                                  */
/* - Safer, allows very large pagelen now, and allows to page while switched */
/*   Zavod of jcowan.reslife.okstate.edu 4000.                               */
/* ========================================================================= */

void show_string( DESCRIPTOR_DATA *d, char *input ) {
	char *start, *end;
	char arg[MAX_INPUT_LENGTH];
	int lines = 0, pagelen;

	/* Set the page length */
	/* ------------------- */

	pagelen = d->original ? d->original->pcdata->pagelen
						  : d->character->pcdata->pagelen;

	/* Check for the command entered */
	/* ----------------------------- */

	one_argument( input, arg );

	switch ( UPPER( *arg ) ) {
		/* Show the next page */

	case '\0':
	case 'C':
		lines = 0;
		break;

		/* Scroll back a page */

	case 'B':
		lines = -2 * pagelen;
		break;

		/* Help for show page */

	case 'H':
		write_to_buffer( d, "B     - Scroll back one page.\n\r", 0 );
		write_to_buffer( d, "C     - Continue scrolling.\n\r", 0 );
		write_to_buffer( d, "H     - This help menu.\n\r", 0 );
		write_to_buffer( d, "R     - Refresh the current page.\n\r",
			0 );
		write_to_buffer( d, "Enter - Continue Scrolling.\n\r", 0 );
		return;

		/* refresh the current page */

	case 'R':
		lines = -1 - pagelen;
		break;

		/* stop viewing */

	default:
		free_string( d->showstr_head );
		d->showstr_head = NULL;
		d->showstr_point = NULL;
		return;
	}

	/* do any backing up necessary to find the starting point */
	/* ------------------------------------------------------ */

	if ( lines < 0 ) {
		for ( start = d->showstr_point; start > d->showstr_head && lines < 0;
			start-- )
			if ( *start == '\r' )
				lines++;
	} else
		start = d->showstr_point;

	/* Find the ending point based on the page length */
	/* ---------------------------------------------- */

	lines = 0;

	for ( end = start; *end && lines < pagelen; end++ )
		if ( *end == '\r' )
			lines++;

	d->showstr_point = end;

	if ( end - start )
		write_to_buffer( d, start, (int) ( end - start ) );

	/* See if this is the end (or near the end) of the string */
	/* ------------------------------------------------------ */

	for ( ; isspace( *end ); end++ );

	if ( !*end ) {
		free_string( d->showstr_head );
		d->showstr_head = NULL;
		d->showstr_point = NULL;
	}

	return;
}
