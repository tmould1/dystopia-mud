/***************************************************************************
 *  Prompt and pager functions extracted from comm.c                        *
 *  Contains: bust_a_header, bust_a_prompt, show_string                     *
 ***************************************************************************/

#include "merc.h"

void bust_a_header( DESCRIPTOR_DATA *d ) {
	char class[16];	  /* Class name: "Werewolf" is longest (8 chars) */
	char class2[48];  /* "Name the Class" format */
	char header[128]; /* ANSI escape sequence header */
	char header1[64]; /* "Name the Class  Align:1234" */
	char blanklin[8];
	CHAR_DATA *ch;
	char cls[32]; /* Color code + " " + NORMAL - needs space for ANSI sequences */
	sprintf( cls, " " );

	ch = d->character;
	if ( ch == NULL ) return;
	ADD_COLOUR( ch, cls, NORMAL );

	if ( IS_CLASS( ch, CLASS_VAMPIRE ) )
		sprintf( class, "Vampire" );
	else if ( IS_CLASS( ch, CLASS_DEMON ) )
		sprintf( class, "Demon" );
	else if ( IS_CLASS( ch, CLASS_WEREWOLF ) )
		sprintf( class, "Werewolf" );
	else
		sprintf( class, "Classless" );
	sprintf( class2, "%s the %s", ch->name, class );
	sprintf( blanklin, " " );
	sprintf( header1, "%-30s Align:%-4d", class2, ch->alignment );
	sprintf( header,
		"\0337\033[1;1H\033[1;44m\033[1;37m%-79s%s\0338", header1, cls );
	send_to_char( header, ch );
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
			sprintf( buf2, "%d", ch->hit );
			COL_SCALE( buf2, ch, ch->hit, ch->max_hit );
			i = buf2;
			break;
		case 'H':
			sprintf( buf2, "%d", ch->max_hit );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'm':
			sprintf( buf2, "%d", ch->mana );
			COL_SCALE( buf2, ch, ch->mana, ch->max_mana );
			i = buf2;
			break;
		case 'M':
			sprintf( buf2, "%d", ch->max_mana );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'v':
			sprintf( buf2, "%d", ch->move );
			COL_SCALE( buf2, ch, ch->move, ch->max_move );
			i = buf2;
			break;
		case 'V':
			sprintf( buf2, "%d", ch->max_move );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'x':
			add_commas_to_number( ch->exp, buf2, sizeof( buf2 ) );
			COL_SCALE( buf2, ch, ch->exp, 10000000 );
			i = buf2;
			break;
		case 'g':
			sprintf( buf2, "%d", ch->gold );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'q':
			sprintf( buf2, "%d", ch->pcdata->quest );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'f':
			if ( ( victim = ch->fighting ) == NULL ) {
				sprintf( buf2, "N/A" );
				ADD_COLOUR( ch, buf2, L_CYAN );
			} else {
				if ( ( victim->hit * 100 / victim->max_hit ) < 25 ) {
					sprintf( buf2, "Awful" );
					ADD_COLOUR( ch, buf2, L_RED );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 50 ) {
					sprintf( buf2, "Poor" );
					ADD_COLOUR( ch, buf2, L_BLUE );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 75 ) {
					sprintf( buf2, "Fair" );
					ADD_COLOUR( ch, buf2, L_GREEN );
				} else if ( ( victim->hit * 100 / victim->max_hit ) < 100 ) {
					sprintf( buf2, "Good" );
					ADD_COLOUR( ch, buf2, YELLOW );
				} else if ( ( victim->hit * 100 / victim->max_hit ) >= 100 ) {
					sprintf( buf2, "Perfect" );
					ADD_COLOUR( ch, buf2, L_CYAN );
				}
			}
			i = buf2;
			break;
		case 'F':
			if ( ( victim = ch->fighting ) == NULL ) {
				sprintf( buf2, "N/A" );
				ADD_COLOUR( ch, buf2, L_CYAN );
			} else if ( ( tank = victim->fighting ) == NULL ) {
				sprintf( buf2, "N/A" );
				ADD_COLOUR( ch, buf2, L_CYAN );
			} else {
				if ( ( tank->hit * 100 / tank->max_hit ) < 25 ) {
					sprintf( buf2, "Awful" );
					ADD_COLOUR( ch, buf2, L_RED );
				} else if ( ( tank->hit * 100 / tank->max_hit ) < 50 ) {
					sprintf( buf2, "Poor" );
					ADD_COLOUR( ch, buf2, L_BLUE );
				} else if ( ( tank->hit * 100 / tank->max_hit ) < 75 ) {
					sprintf( buf2, "Fair" );
					ADD_COLOUR( ch, buf2, L_GREEN );
				} else if ( ( tank->hit * 100 / tank->max_hit ) < 100 ) {
					sprintf( buf2, "Good" );
					ADD_COLOUR( ch, buf2, YELLOW );
				} else if ( ( tank->hit * 100 / tank->max_hit ) >= 100 ) {
					sprintf( buf2, "Perfect" );
					ADD_COLOUR( ch, buf2, L_CYAN );
				}
			}
			i = buf2;
			break;
		case 'n':
			if ( ( victim = ch->fighting ) == NULL )
				sprintf( buf2, "N/A" );
			else {
				if ( IS_AFFECTED( victim, AFF_POLYMORPH ) )
					strcpy( buf2, victim->morph );
				else if ( IS_NPC( victim ) )
					strcpy( buf2, victim->short_descr );
				else
					strcpy( buf2, victim->name );
				buf2[0] = UPPER( buf2[0] );
			}
			i = buf2;
			break;
		case 'N':
			if ( ( victim = ch->fighting ) == NULL )
				sprintf( buf2, "N/A" );
			else if ( ( tank = victim->fighting ) == NULL )
				sprintf( buf2, "N/A" );
			else {
				if ( ch == tank )
					sprintf( buf2, "You" );
				else if ( IS_AFFECTED( tank, AFF_POLYMORPH ) )
					strcpy( buf2, tank->morph );
				else if ( IS_NPC( victim ) )
					strcpy( buf2, tank->short_descr );
				else
					strcpy( buf2, tank->name );
				buf2[0] = UPPER( buf2[0] );
			}
			i = buf2;
			break;
		case 'a':
			sprintf( buf2, "%s", IS_GOOD( ch ) ? "good" : IS_EVIL( ch ) ? "evil"
																		: "neutral" );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'A':
			sprintf( buf2, "%d", ch->alignment );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 't':
			sprintf( buf2, "%d", ch->fight_timer );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'k':
			if ( IS_CLASS( ch, CLASS_SHAPESHIFTER ) && !IS_NPC( ch ) ) {
				sprintf( buf2, "%d", ch->pcdata->powers[SHAPE_COUNTER] );
				ADD_COLOUR( ch, buf2, L_CYAN );
				i = buf2;
				break;
			} else if ( IS_CLASS( ch, CLASS_SAMURAI ) && !IS_NPC( ch ) ) {
				sprintf( buf2, "%d", ch->pcdata->powers[SAMURAI_FOCUS] );
				i = buf2;
				break;
			} else {
				sprintf( buf2, " " );
				i = buf2;
				break;
			}
		case 'r':
			if ( ch->in_room )
				sprintf( buf2, "%s", ch->in_room->name );
			else
				sprintf( buf2, " " );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'R':
			if ( !IS_NPC( ch ) && ( IS_CLASS( ch, CLASS_WEREWOLF ) || IS_CLASS( ch, CLASS_VAMPIRE ) || IS_CLASS( ch, CLASS_NINJA ) ) ) {
				sprintf( buf2, "%d", ch->rage );
				ADD_COLOUR( ch, buf2, D_RED );
			} else
				sprintf( buf2, "0" );
			i = buf2;
			break;
		case 'b':
			sprintf( buf2, "%d", ch->beast );
			ADD_COLOUR( ch, buf2, L_CYAN );
			i = buf2;
			break;
		case 'B':
			if ( !IS_NPC( ch ) && IS_CLASS( ch, CLASS_VAMPIRE ) ) {
				sprintf( buf2, "%d", ch->pcdata->condition[COND_THIRST] );
				ADD_COLOUR( ch, buf2, D_RED );
			} else if ( !IS_NPC( ch ) && IS_CLASS( ch, CLASS_SHAPESHIFTER ) ) {
				sprintf( buf2, "%d", ch->pcdata->condition[COND_FULL] );
				ADD_COLOUR( ch, buf2, D_RED );
			} else
				sprintf( buf2, "0" );
			i = buf2;
			break;
		case 'c':
			sprintf( buf2, "%d", char_ac( ch ) );
			i = buf2;
			break;
		case 'p':
			sprintf( buf2, "%d", char_hitroll( ch ) );
			COL_SCALE( buf2, ch, char_hitroll( ch ), 200 );
			i = buf2;
			break;
		case 'P':
			sprintf( buf2, "%d", char_damroll( ch ) );
			COL_SCALE( buf2, ch, char_damroll( ch ), 200 );
			i = buf2;
			break;
		case 's':

			if ( !IS_NPC( ch ) && ch->pcdata->stage[2] + 25 >= ch->pcdata->stage[1] && ch->pcdata->stage[1] > 0 ) {
				sprintf( buf2, "yes" );
				ADD_COLOUR( ch, buf2, WHITE );
			} else
				sprintf( buf2, "no" );
			i = buf2;
			break;
		case 'O':
			if ( ( victim = ch->pcdata->partner ) == NULL )
				sprintf( buf2, "no" );
			else if ( !IS_NPC( victim ) && victim != NULL && victim->pcdata->stage[1] > 0 && victim->pcdata->stage[2] + 25 >= victim->pcdata->stage[1] ) {
				sprintf( buf2, "yes" );
				ADD_COLOUR( ch, buf2, WHITE );
			} else
				sprintf( buf2, "no" );
			i = buf2;
			break;
		case 'l':
			if ( ( victim = ch->pcdata->partner ) == NULL )
				sprintf( buf2, "Nobody" );
			else {
				if ( IS_AFFECTED( victim, AFF_POLYMORPH ) )
					strcpy( buf2, victim->morph );
				else if ( IS_NPC( victim ) )
					strcpy( buf2, victim->short_descr );
				else
					strcpy( buf2, victim->name );
				buf2[0] = UPPER( buf2[0] );
			}
			i = buf2;
			break;
		case '%':
			sprintf( buf2, "%%" );
			i = buf2;
			break;
		}
		++str;
		while ( ( *point = *i ) != '\0' )
			++point, ++i;
	}
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
