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

/***************************************************************************
 *  File: string.c                                                         *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "utf8.h"

/*****************************************************************************
 Name:		string_append
 Purpose:	Clears string and puts player into editing mode.
 Called by:	none
 ****************************************************************************/
void string_edit( CHAR_DATA *ch, char **pString ) {
	send_to_char( "#3-========- #7Entering EDIT Mode#3 -=========-\n\r",
		ch );
	send_to_char( "#7    Type /h on a new line for help\n\r", ch );
	send_to_char( "#7 Terminate with a ~ or @ on a blank line.\n\r", ch );
	send_to_char( "#3-=======================================-\n\r", ch );

	if ( *pString == NULL ) {
		*pString = str_dup( "" );
	} else {
		**pString = '\0';
	}

	ch->desc->pString = pString;

	return;
}

/*****************************************************************************
 Name:		string_append
 Purpose:	Puts player into append mode for given string.
 Called by:	(many)olc_act.c
 ****************************************************************************/
void string_append( CHAR_DATA *ch, char **pString ) {
	send_to_char( "#3-=======- #7Entering APPEND Mode #3-========-\n\r",
		ch );
	send_to_char( "#7    Type /h on a new line for help\n\r", ch );
	send_to_char( "#7 Terminate with a ~ or @ on a blank line.\n\r", ch );
	send_to_char( "#3-=======================================-\n\r", ch );

	if ( *pString == NULL ) {
		*pString = str_dup( "" );
	}
	send_to_char( *pString, ch );

	if ( *( *pString + strlen( *pString ) - 1 ) != '\r' )
		send_to_char( "\n\r", ch );

	ch->desc->pString = pString;

	return;
}

/*****************************************************************************
 Name:		string_replace
 Purpose:	Substitutes one string for another.
 Called by:	string_add(string.c) (aedit_builder)olc_act.c.
 ****************************************************************************/
char *string_replace( char *orig, char *old, char *new ) {
	char xbuf[MAX_STRING_LENGTH];
	int i;

	xbuf[0] = '\0';
	strcpy( xbuf, orig );
	if ( strstr( orig, old ) != NULL ) {
		i = (int) strlen( orig ) - (int) strlen( strstr( orig, old ) );
		xbuf[i] = '\0';
		strcat( xbuf, new );
		strcat( xbuf, &orig[i + strlen( old )] );
		free_string( orig );
	}

	return str_dup( xbuf );
}

/* OLC 1.1b */
/*****************************************************************************
 Name:		string_add
 Purpose:	Interpreter for string editing.
 Called by:	game_loop_xxxx(comm.c).
 ****************************************************************************/
void string_add( CHAR_DATA *ch, char *argument ) {
	char buf[MAX_STRING_LENGTH];

	/*
	 * Thanks to James Seng
	 */
	smash_tilde( argument );

	if ( *argument == '/' ) {
		char arg1[MAX_INPUT_LENGTH];
		char arg2[MAX_INPUT_LENGTH];
		char arg3[MAX_INPUT_LENGTH];

		argument = one_argument( argument, arg1 );
		argument = first_arg( argument, arg2, FALSE );
		argument = first_arg( argument, arg3, FALSE );

		if ( !str_cmp( arg1, "/c" ) ) {
			send_to_char( "String cleared.\n\r", ch );
			**ch->desc->pString = '\0';
			return;
		}

		if ( !str_cmp( arg1, "/s" ) ) {
			send_to_char( "String so far:\n\r", ch );
			send_to_char( *ch->desc->pString, ch );
			return;
		}

		if ( !str_cmp( arg1, "/r" ) ) {
			if ( arg2[0] == '\0' ) {
				send_to_char(
					"usage:  /r \"old string\" \"new string\"\n\r", ch );
				return;
			}

			*ch->desc->pString =
				string_replace( *ch->desc->pString, arg2, arg3 );
			sprintf( buf, "'%s' replaced with '%s'.\n\r", arg2, arg3 );
			send_to_char( buf, ch );
			return;
		}

		if ( !str_cmp( arg1, "/f" ) ) {
			*ch->desc->pString = format_string( *ch->desc->pString );
			send_to_char( "String formatted.\n\r", ch );
			return;
		}

		if ( !str_cmp( arg1, "/h" ) ) {
			send_to_char( "Sedit help (commands on blank line):   \n\r", ch );
			send_to_char( "/r 'old' 'new'   - replace a substring \n\r",
				ch );
			send_to_char( "                   (requires '', \"\") \n\r", ch );
			send_to_char( "/h               - get help (this info)\n\r",
				ch );
			send_to_char( "/s               - show string so far  \n\r",
				ch );
			send_to_char( "/f               - (word wrap) string  \n\r",
				ch );
			send_to_char( "/c               - clear string so far \n\r",
				ch );
			send_to_char( "@                - end string          \n\r", ch );
			return;
		}

		send_to_char( "SEdit:  Invalid command.\n\r", ch );
		return;
	}

	if ( *argument == '@' ) {
		ch->desc->pString = NULL;
		return;
	}

	/*
	 * Truncate strings to MAX_STRING_LENGTH.
	 * --------------------------------------
	 */
	if ( strlen( buf ) + strlen( argument ) >= ( MAX_STRING_LENGTH - 4 ) ) {
		send_to_char( "String too long, last line skipped.\n\r", ch );

		/* Force character out of editing mode. */
		ch->desc->pString = NULL;
		return;
	}

	strcpy( buf, *ch->desc->pString );
	strcat( buf, argument );
	strcat( buf, "\n\r" );
	free_string( *ch->desc->pString );
	*ch->desc->pString = str_dup( buf );
	return;
}

/*
 *  Thanks to Kalgen for the new procedure (no more bug!)
 *  Original wordwrap() written by Surreality.
 */
/*****************************************************************************
 Name:		format_string
 Purpose:	Special string formating and word-wrapping.
 Called by:	string_add(string.c) (many)olc_act.c
 ****************************************************************************/
char *format_string( char *oldstring /*, bool fSpace */ ) {
	char xbuf[MAX_STRING_LENGTH];
	char xbuf2[MAX_STRING_LENGTH];
	char *rdesc;
	int i = 0;
	bool cap = TRUE;

	xbuf[0] = xbuf2[0] = 0;

	i = 0;

	if ( strlen( oldstring ) >= ( MAX_STRING_LENGTH - 4 ) ) /* OLC 1.1b */
	{
		bug( "String to format_string() longer than MAX_STRING_LENGTH.", 0 );
		return ( oldstring );
	}

	for ( rdesc = oldstring; *rdesc; rdesc++ ) {
		if ( *rdesc == '\n' ) {
			if ( xbuf[i - 1] != ' ' ) {
				xbuf[i] = ' ';
				i++;
			}
		} else if ( *rdesc == '\r' )
			;
		else if ( *rdesc == ' ' ) {
			if ( xbuf[i - 1] != ' ' ) {
				xbuf[i] = ' ';
				i++;
			}
		} else if ( *rdesc == ')' ) {
			if ( xbuf[i - 1] == ' ' && xbuf[i - 2] == ' ' &&
				( xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!' ) ) {
				xbuf[i - 2] = *rdesc;
				xbuf[i - 1] = ' ';
				xbuf[i] = ' ';
				i++;
			} else {
				xbuf[i] = *rdesc;
				i++;
			}
		} else if ( *rdesc == '.' || *rdesc == '?' || *rdesc == '!' ) {
			if ( xbuf[i - 1] == ' ' && xbuf[i - 2] == ' ' &&
				( xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!' ) ) {
				xbuf[i - 2] = *rdesc;
				if ( *( rdesc + 1 ) != '\"' ) {
					xbuf[i - 1] = ' ';
					xbuf[i] = ' ';
					i++;
				} else {
					xbuf[i - 1] = '\"';
					xbuf[i] = ' ';
					xbuf[i + 1] = ' ';
					i += 2;
					rdesc++;
				}
			} else {
				xbuf[i] = *rdesc;
				if ( *( rdesc + 1 ) != '\"' ) {
					xbuf[i + 1] = ' ';
					xbuf[i + 2] = ' ';
					i += 3;
				} else {
					xbuf[i + 1] = '\"';
					xbuf[i + 2] = ' ';
					xbuf[i + 3] = ' ';
					i += 4;
					rdesc++;
				}
			}
			cap = TRUE;
		} else {
			xbuf[i] = *rdesc;
			if ( cap ) {
				cap = FALSE;
				xbuf[i] = UPPER( xbuf[i] );
			}
			i++;
		}
	}
	xbuf[i] = 0;
	strcpy( xbuf2, xbuf );

	rdesc = xbuf2;

	xbuf[0] = 0;

	for ( ;; ) {
		/* Walk forward counting display columns, not bytes */
		{
			const char *p = rdesc;
			int col = 0;
			while ( *p && col < 77 ) {
				if ( (unsigned char) *p >= 0x80 ) {
					int w;
					const char *prev = p;
					unsigned int cp = utf8_decode( &p );
					w = utf8_wcwidth( cp );
					if ( w > 0 ) col += w;
					(void) prev;
				} else {
					col++;
					p++;
				}
			}
			i = (int) ( p - rdesc );
			if ( col < 77 ) break; /* remaining text fits on one line */
		}
		/* Search backward for a space to break at.
		 * Space (0x20) is ASCII and cannot appear inside a UTF-8 multi-byte
		 * sequence, so byte-level backward scan is safe. */
		{
			int limit = ( xbuf[0] ? i : ( i > 3 ? i - 3 : i ) );
			int j;
			for ( j = limit; j > 0; j-- ) {
				if ( *( rdesc + j ) == ' ' ) break;
			}
			i = j;
		}
		if ( i ) {
			*( rdesc + i ) = 0;
			strcat( xbuf, rdesc );
			strcat( xbuf, "\n\r" );
			rdesc += i + 1;
			while ( *rdesc == ' ' ) rdesc++;
		} else {
			/* No spaces found - force break (avoid splitting UTF-8) */
			int brk = utf8_truncate( rdesc, 75 );
			char saved = *( rdesc + brk );
			*( rdesc + brk ) = 0;
			strcat( xbuf, rdesc );
			strcat( xbuf, "-\n\r" );
			*( rdesc + brk ) = saved;
			rdesc += brk;
		}
	}
	while ( *( rdesc + i ) && ( *( rdesc + i ) == ' ' || *( rdesc + i ) == '\n' || *( rdesc + i ) == '\r' ) )
		i--;
	*( rdesc + i + 1 ) = 0;
	strcat( xbuf, rdesc );
	if ( xbuf[strlen( xbuf ) - 2] != '\n' )
		strcat( xbuf, "\n\r" );

	free_string( oldstring );
	return ( str_dup( xbuf ) );
}

/*
 * Used above in string_add.  Because this function does not
 * modify case if fCase is FALSE and because it understands
 * parenthesis, it would probably make a nice replacement
 * for one_argument.
 */
/*****************************************************************************
 Name:		first_arg
 Purpose:	Pick off one argument from a string and return the rest.
		Understands quates, parenthesis (barring ) ('s) and
		percentages.
 Called by:	string_add(string.c)
 ****************************************************************************/
char *first_arg( char *argument, char *arg_first, bool fCase ) {
	char cEnd;

	while ( *argument == ' ' )
		argument++;

	cEnd = ' ';
	if ( *argument == '\'' || *argument == '"' || *argument == '%' || *argument == '(' ) {
		if ( *argument == '(' ) {
			cEnd = ')';
			argument++;
		} else
			cEnd = *argument++;
	}

	while ( *argument != '\0' ) {
		if ( *argument == cEnd ) {
			argument++;
			break;
		}
		if ( fCase )
			*arg_first = LOWER( *argument );
		else
			*arg_first = *argument;
		arg_first++;
		argument++;
	}
	*arg_first = '\0';

	while ( *argument == ' ' )
		argument++;

	return argument;
}

/*
 * Used in olc_act.c for aedit_builders.
 */
char *string_unpad( char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char *s;

	s = argument;

	while ( *s == ' ' )
		s++;

	strcpy( buf, s );
	s = buf;

	if ( *s != '\0' ) {
		while ( *s != '\0' )
			s++;
		s--;

		while ( *s == ' ' )
			s--;
		s++;
		*s = '\0';
	}

	free_string( argument );
	return str_dup( buf );
}

/*
 * Same as capitalize but changes the pointer's data.
 * Used in olc_act.c in aedit_builder.
 */
char *string_proper( char *argument ) {
	char *s;

	s = argument;

	while ( *s != '\0' ) {
		if ( *s != ' ' ) {
			*s = UPPER( *s );
			while ( *s != ' ' && *s != '\0' )
				s++;
		} else {
			s++;
		}
	}

	return argument;
}

/*
 * Returns an all-caps string.		OLC 1.1b
 */
char *all_capitalize( const char *str ) {
	static char strcap[MAX_STRING_LENGTH];
	int i;
	for ( i = 0; str[i] != '\0'; i++ )
		strcap[i] = UPPER( str[i] );
	strcap[i] = '\0';
	return strcap;
}

void add_commas_to_number( int number, char *out_str, size_t buf_size ) {
	char non_formatted_string[MSL];

	if ( buf_size == 0 ) return;
	out_str[0] = '\0';

	// Go through the non_formatted_xp starting from the beginning;
	// emplace each character in the formatted string, and add a comma after every 3 characters
	snprintf( non_formatted_string, sizeof( non_formatted_string ), "%d", number );
	int len = (int) strlen( non_formatted_string );
	int num_commas = ( len - 1 ) / 3;
	int new_len = len + num_commas;

	if ( (size_t)( new_len + 1 ) > buf_size ) {
		// Buffer too small for formatted number, fall back to unformatted
		snprintf( out_str, buf_size, "%d", number );
		return;
	}

	out_str[new_len] = '\0';

	for ( int i = len - 1, j = new_len - 1, k = 0;
		i >= 0;
		i--, j--, k++ ) {
		if ( k == 3 ) {
			out_str[j] = ',';
			j--;
			k = 0;
		}
		out_str[j] = non_formatted_string[i];
	}
}

/*
 * Calculate visible string length, excluding color codes.
 * Handles standard (#X), extended (#xNNN/#XNNN), and true color
 * (#tRRGGBB/#TRRGGBB) color codes.
 * UTF-8 aware: CJK wide characters count as 2 columns.
 */
int visible_strlen( const char *str ) {
	return utf8_visible_width( str );
}

/*
 * Pad a string (which may contain color codes) to a target visible width.
 * Appends spaces to reach target_width visible characters.
 */
void pad_to_visible_width( char *dest, size_t destsize, const char *src, int target_width ) {
	int visible_len = visible_strlen( src );
	int padding = target_width - visible_len;
	size_t src_len = strlen( src );
	size_t i;

	if ( padding < 0 )
		padding = 0;

	/* Copy the source string */
	if ( src_len >= destsize )
		src_len = destsize - 1;
	memcpy( dest, src, src_len );

	/* Add padding spaces */
	for ( i = 0; i < (size_t)padding && ( src_len + i ) < ( destsize - 1 ); i++ )
		dest[src_len + i] = ' ';

	dest[src_len + i] = '\0';
}