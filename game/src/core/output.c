/***************************************************************************
 *  Output functions extracted from comm.c                                  *
 *  Contains: stc, send_to_char, act, act2, kavitem, and display helpers    *
 ***************************************************************************/

#include "merc.h"
#include "utf8.h"

/*
 * Test output capture: intercepts raw output for a specific character
 * before color processing. Used by the test harness for Tier 2+ tests.
 */
static CHAR_DATA *capture_ch   = NULL;
static char      *capture_buf  = NULL;
static int        capture_len  = 0;
static int        capture_size = 0;

static void capture_append( const char *txt ) {
	int len = (int) strlen( txt );
	while ( capture_len + len + 1 > capture_size ) {
		capture_size = capture_size ? capture_size * 2 : 4096;
		capture_buf = realloc( capture_buf, capture_size );
	}
	memcpy( capture_buf + capture_len, txt, len );
	capture_len += len;
	capture_buf[capture_len] = '\0';
}

void test_output_start( CHAR_DATA *ch ) {
	capture_ch = ch;
	capture_len = 0;
	if ( capture_buf == NULL ) {
		capture_size = 4096;
		capture_buf = malloc( capture_size );
	}
	capture_buf[0] = '\0';
}

const char *test_output_get( void ) {
	return capture_buf ? capture_buf : "";
}

void test_output_clear( void ) {
	capture_len = 0;
	if ( capture_buf )
		capture_buf[0] = '\0';
}

void test_output_stop( void ) {
	capture_ch = NULL;
	capture_len = 0;
	if ( capture_buf ) {
		free( capture_buf );
		capture_buf = NULL;
	}
	capture_size = 0;
}

/*
 * Send to character - short form
 */
void stc( const char *txt, CHAR_DATA *ch ) {
	if ( txt != NULL && capture_ch == ch ) {
		capture_append( txt );
		return;
	}
	if ( txt != NULL && ch->desc != NULL )
		write_to_buffer( ch->desc, txt, (int) strlen( txt ) );
	return;
}

int col_str_len( char *txt ) {
	return utf8_visible_width( txt );
}

void cent_to_char( char *txt, CHAR_DATA *ch ) {
	int len, pos;
	char buf[MAX_STRING_LENGTH];

	len = ( naws_get_width( ch ) - col_str_len( txt ) ) / 2;
	for ( pos = 0; pos < len; pos++ ) {
		buf[pos] = ' ';
	}
	buf[pos] = '\0';
	send_to_char( buf, ch );
	send_to_char( txt, ch );
	send_to_char( "\n\r", ch );
}

void divide_to_char( CHAR_DATA *ch ) {
	send_to_char( "-------------------------------------------------------------------------------\r\n",
		ch );
}

void divide2_to_char( CHAR_DATA *ch ) {
	send_to_char( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\r\n",
		ch );
}

void divide3_to_char( CHAR_DATA *ch ) {
	send_to_char( "===============================================================================\r\n", ch );
}

void divide4_to_char( CHAR_DATA *ch ) {
	send_to_char( "#4-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6***#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-\r\n", ch );
}

void divide5_to_char( CHAR_DATA *ch ) {
	cent_to_char( "#4-=[#6***********#4]=-------------=[#6***********#4]=-#n", ch );
}

void divide6_to_char( CHAR_DATA *ch ) {
	cent_to_char( "#4-    -   -  - - -#6- ---====#7*#6====--- -#4- - -  -   - -", ch );
}

void banner_to_char( char *txt, CHAR_DATA *ch ) {
	char buf[MAX_STRING_LENGTH];
	int wdth, ln, txt_bytes;
	ln = utf8_display_width( txt );
	txt_bytes = (int) strlen( txt );
	if ( ln > 16 ) {
		sprintf( buf, "#4-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[                   ]#4=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-#n" );
		wdth = ( 17 - ln ) / 2 + 20;
	} else {
		sprintf( buf,
			"#4-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[              ] #L=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-=[#6**#4]=-#n" );
		wdth = ( 11 - ln ) / 2 + 32;
	}
	memcpy( buf + wdth + 22, txt, txt_bytes );
	cent_to_char( buf, ch );
}

void banner2_to_char( char *txt, CHAR_DATA *ch ) {
	char buf[MAX_STRING_LENGTH];
	int wdth, ln, txt_bytes;
	ln = utf8_display_width( txt );
	txt_bytes = (int) strlen( txt );
	if ( ln > 16 ) {
		sprintf( buf, "#4    -   -  - - -#6- ---===#7                               #6===--- -#4- - -  -   -\r\n" );
		wdth = ( 31 - ln ) / 2 + 24;
	} else {
		sprintf( buf, "#4     -    -   -  - - -#6- ---====#7                #6====--- -#4- - -  -   -    -\r\n" );
		wdth = ( 16 - ln ) / 2 + 32;
	}
	memcpy( buf + wdth + 6, txt, txt_bytes );
	send_to_char( buf, ch );
}

/*
 * Write to one char.
 */
void send_to_char( const char *txt, CHAR_DATA *ch ) {
	CHAR_DATA *wizard;
	CHAR_DATA *familiar;

	/* Test output capture: intercept before descriptor/color processing */
	if ( txt != NULL && capture_ch == ch ) {
		capture_append( txt );
		return;
	}

	if ( ch->desc == NULL && IS_NPC( ch ) && ( wizard = ch->wizard ) != NULL ) {
		if ( !IS_NPC( wizard ) && ( familiar = wizard->pcdata->familiar ) != NULL && familiar == ch && ch->in_room != wizard->in_room ) {
			send_to_char( "[ ", wizard );
			if ( txt != NULL && wizard->desc != NULL )
				write_to_buffer( wizard->desc, txt, (int) strlen( txt ) );
		}
	}

	if ( txt != NULL && ch->desc != NULL )
		write_to_buffer( ch->desc, txt, (int) strlen( txt ) );
	return;
}

/*
 * The primary output interface for formatted output.
 */
void act( const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type ) {
	static char *const he_she[] = { "it", "he", "she" };
	static char *const him_her[] = { "it", "him", "her" };
	static char *const his_her[] = { "its", "his", "her" };

	char buf[MAX_STRING_LENGTH];
	char fname[MAX_INPUT_LENGTH];
	CHAR_DATA *to;

	CHAR_DATA *to_old;

	CHAR_DATA *vch = (CHAR_DATA *) arg2;

	CHAR_DATA *familiar = NULL;
	CHAR_DATA *wizard = NULL;

	OBJ_DATA *obj1 = (OBJ_DATA *) arg1;
	OBJ_DATA *obj2 = (OBJ_DATA *) arg2;
	const char *str;
	const char *i;
	char *point;

	bool is_fam;

	bool is_ok;
	/*
	 * Discard null and zero-length messages.
	 */
	if ( format == NULL || format[0] == '\0' )
		return;

	/*	Skummel crash bug, tror dette check spiser buggen, vi faar se */
	if ( ch == NULL || ch->in_room == NULL ) {
		return;
	}

	{
	list_head_t *to_list = &ch->in_room->characters;
	if ( type == TO_VICT ) {
		if ( vch == NULL || vch->in_room == NULL ) {
			/*	    bug( "Act: null vch with TO_VICT.", 0 );*/
			return;
		}
		to_list = &vch->in_room->characters;
	}

	LIST_FOR_EACH( to, to_list, CHAR_DATA, room_node ) {

		is_fam = FALSE;
		to_old = to;

		if ( type == TO_CHAR && to != ch ) continue;
		if ( type == TO_VICT && ( to != vch || to == ch ) ) continue;
		if ( type == TO_ROOM && to == ch ) continue;
		if ( type == TO_NOTVICT && ( to == ch || to == vch ) ) continue;
		if ( to->desc == NULL && IS_NPC( to ) && ( wizard = to->wizard ) != NULL ) {
			if ( !IS_NPC( wizard ) && ( ( familiar = wizard->pcdata->familiar ) != NULL ) && familiar == to ) {
				if ( to->in_room == ch->in_room &&
					wizard->in_room != to->in_room ) {
					to = wizard;
					is_fam = TRUE;
				}
			}
		}

		if ( to->desc == NULL || !IS_AWAKE( to ) ) {

			if ( is_fam ) to = to_old;

			continue;
		}

		if ( ch->in_room->vnum == ROOM_VNUM_IN_OBJECT ) {
			is_ok = FALSE;

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

			if ( !is_ok ) {

				if ( is_fam ) to = to_old;

				continue;
			}
		}

		point = buf;
		str = format;
		while ( *str != '\0' ) {
			/* Check for buffer overflow using helper */
			if ( !buf_has_space( point, buf, MAX_STRING_LENGTH, 1, 10 ) ) {
				bug( "act: buffer overflow, truncating message", 0 );
				break;
			}

			if ( *str != '$' ) {
				*point++ = *str++;
				continue;
			}
			++str;

			if ( arg2 == NULL && *str >= 'A' && *str <= 'Z' ) {
				/*		bug( "Act: missing arg2 for code %d.", *str );*/
				i = " <@@@> ";
			} else {
				switch ( *str ) {
				default:
					bug( "Act: bad code %d.", *str );
					i = " <@@@> ";
					break;
				/* Thx alex for 't' idea */
				case 't':
					i = (char *) arg1;
					break;
				case 'T':
					i = (char *) arg2;
					break;
				case 'n':
					i = PERS( ch, to );
					break;
				case 'N':
					i = PERS( vch, to );
					break;
				case 'e':
					i = he_she[URANGE( 0, ch->sex, 2 )];
					break;
				case 'E':
					i = he_she[URANGE( 0, vch->sex, 2 )];
					break;
				case 'm':
					i = him_her[URANGE( 0, ch->sex, 2 )];
					break;
				case 'M':
					i = him_her[URANGE( 0, vch->sex, 2 )];
					break;
				case 's':
					i = his_her[URANGE( 0, ch->sex, 2 )];
					break;
				case 'S':
					i = his_her[URANGE( 0, vch->sex, 2 )];
					break;

				case 'p':
					i = can_see_obj( to, obj1 )
						? ( ( obj1->chobj != NULL && obj1->chobj == to )
								  ? "you"
								  : obj1->short_descr )
						: "something";
					break;

				case 'P':
					i = can_see_obj( to, obj2 )
						? ( ( obj2->chobj != NULL && obj2->chobj == to )
								  ? "you"
								  : obj2->short_descr )
						: "something";
					break;

				case 'd':
					if ( arg2 == NULL || ( (char *) arg2 )[0] == '\0' ) {
						i = "door";
					} else {
						one_argument( (char *) arg2, fname );
						i = fname;
					}
					break;
				}
			}

			++str;
			/* Copy replacement string with bounds checking using helper */
			point = buf_append_safe( point, i, buf, MAX_STRING_LENGTH, 10 );
			if ( point == NULL ) {
				bug( "act: replacement string overflow", 0 );
				break;
			}
		}

		/* Safely add newline using helper */
		if ( buf_has_space( point, buf, MAX_STRING_LENGTH, 2, 0 ) && point != NULL ) {
			*point++ = '\n';
			*point++ = '\r';
		}
		/* NUL-terminate for safety (write_to_buffer uses explicit length,
		 * but this prevents overread if the length/counter ever mismatch) */
		if ( point != NULL && point < buf + MAX_STRING_LENGTH )
			*point = '\0';

		if ( is_fam ) {
			if ( to->in_room != ch->in_room && familiar != NULL &&
				familiar->in_room == ch->in_room )
				send_to_char( "[ ", to );
			else {
				to = to_old;
				continue;
			}
		}

		buf[0] = UPPER( buf[0] );
		if ( to->desc && ( to->desc->connected == CON_PLAYING ) )
			write_to_buffer( to->desc, buf, (int) ( point - buf ) );

		if ( is_fam ) to = to_old;
	}
	}
	return;
}

void act2( const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type ) {
	static char *const he_she[] = { "it", "he", "she" };
	static char *const him_her[] = { "it", "him", "her" };
	static char *const his_her[] = { "its", "his", "her" };

	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *to;

	CHAR_DATA *to_old;

	CHAR_DATA *vch = (CHAR_DATA *) arg2;
	CHAR_DATA *familiar = NULL;
	CHAR_DATA *wizard = NULL;
	OBJ_DATA *obj1 = (OBJ_DATA *) arg1;
	OBJ_DATA *obj2 = (OBJ_DATA *) arg2;
	const char *str;
	const char *i;
	char *point;

	bool is_fam;

	bool is_ok;
	/*
	 * Discard null and zero-length messages.
	 */
	if ( format == NULL || format[0] == '\0' )
		return;

	{
	list_head_t *to_list = &ch->in_room->characters;
	if ( type == TO_VICT ) {
		if ( vch == NULL || vch->in_room == NULL ) {
			bug( "Act: null vch with TO_VICT.", 0 );
			return;
		}
		to_list = &vch->in_room->characters;
	}

	LIST_FOR_EACH( to, to_list, CHAR_DATA, room_node ) {

		is_fam = FALSE;
		to_old = to;

		if ( type == TO_CHAR && to != ch ) continue;
		if ( type == TO_VICT && ( to != vch || to == ch ) ) continue;
		if ( type == TO_ROOM && to == ch ) continue;
		if ( type == TO_NOTVICT && ( to == ch || to == vch ) ) continue;

		if ( to->desc == NULL && IS_NPC( to ) && ( wizard = to->wizard ) != NULL ) {
			if ( !IS_NPC( wizard ) && ( ( familiar = wizard->pcdata->familiar ) != NULL ) && familiar == to ) {
				if ( to->in_room == ch->in_room &&
					wizard->in_room != to->in_room ) {
					to = wizard;
					is_fam = TRUE;
				}
			}
		}

		if ( to->desc == NULL || !IS_AWAKE( to ) ) {

			if ( is_fam ) to = to_old;

			continue;
		}

		if ( ch->in_room->vnum == ROOM_VNUM_IN_OBJECT ) {
			is_ok = FALSE;

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

			if ( !is_ok ) {

				if ( is_fam ) to = to_old;

				continue;
			}
		}

		point = buf;
		str = format;
		while ( *str != '\0' ) {
			/* Check for buffer overflow using helper */
			if ( !buf_has_space( point, buf, MAX_STRING_LENGTH, 1, 10 ) ) {
				bug( "act2: buffer overflow, truncating message", 0 );
				break;
			}

			if ( *str != '$' ) {
				*point++ = *str++;
				continue;
			}
			++str;

			if ( arg2 == NULL && *str >= 'A' && *str <= 'Z' ) {

				/*		bug( "Act: missing arg2 for code %d.", *str );
				 */
				i = " <@@@> ";
			} else {
				switch ( *str ) {
				default:
					i = " ";
					break;
				case 'n':
					if ( ch != NULL )
						i = PERS( ch, to );
					else
						i = " ";
					break;
				case 'N':
					if ( vch != NULL )
						i = PERS( vch, to );
					else
						i = " ";
					break;
				case 'e':
					if ( ch != NULL )
						i = he_she[URANGE( 0, ch->sex, 2 )];
					else
						i = " ";
					break;
				case 'E':
					if ( vch != NULL )
						i = he_she[URANGE( 0, vch->sex, 2 )];
					else
						i = " ";
					break;
				case 'm':
					if ( ch != NULL )
						i = him_her[URANGE( 0, ch->sex, 2 )];
					else
						i = " ";
					break;
				case 'M':
					if ( vch != NULL )
						i = him_her[URANGE( 0, vch->sex, 2 )];
					else
						i = " ";
					break;
				case 's':
					if ( ch != NULL )
						i = his_her[URANGE( 0, ch->sex, 2 )];
					else
						i = " ";
					break;
				case 'S':
					if ( vch != NULL )
						i = his_her[URANGE( 0, vch->sex, 2 )];
					else
						i = " ";
					break;
				case 'p':
					if ( obj1 != NULL ) {
						i = can_see_obj( to, obj1 )
							? ( ( obj1->chobj != NULL && obj1->chobj == to )
									  ? "you"
									  : obj1->short_descr )
							: "something";
					} else
						i = " ";
					break;

				case 'P':
					if ( obj2 != NULL ) {
						i = can_see_obj( to, obj2 )
							? ( ( obj2->chobj != NULL && obj2->chobj == to )
									  ? "you"
									  : obj2->short_descr )
							: "something";
					} else
						i = " ";
					break;
				}
			}

			++str;
			/* Copy replacement string with bounds checking using helper */
			point = buf_append_safe( point, i, buf, MAX_STRING_LENGTH, 10 );
			if ( point == NULL ) {
				bug( "act2: replacement string overflow", 0 );
				break;
			}
		}

		/* Safely add newline using helper */
		if ( buf_has_space( point, buf, MAX_STRING_LENGTH, 2, 0 ) && point != NULL ) {
			*point++ = '\n';
			*point++ = '\r';
		}
		/* NUL-terminate for safety (write_to_buffer uses explicit length,
		 * but this prevents overread if the length/counter ever mismatch) */
		if ( point != NULL && point < buf + MAX_STRING_LENGTH )
			*point = '\0';

		if ( is_fam ) {
			if ( to->in_room != ch->in_room && familiar != NULL &&
				familiar->in_room == ch->in_room )
				send_to_char( "[ ", to );
			else {
				to = to_old;
				continue;
			}
		}

		buf[0] = UPPER( buf[0] );
		write_to_buffer( to->desc, buf, (int) ( point - buf ) );

		if ( is_fam ) to = to_old;
	}
	}
	return;
}

void kavitem( const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type ) {
	static char *const he_she[] = { "it", "he", "she" };
	static char *const him_her[] = { "it", "him", "her" };
	static char *const his_her[] = { "its", "his", "her" };

	char buf[MAX_STRING_LENGTH];
	char kav[MAX_INPUT_LENGTH];
	CHAR_DATA *to;
	CHAR_DATA *vch = (CHAR_DATA *) arg2;
	OBJ_DATA *obj1 = (OBJ_DATA *) arg1;
	const char *str;
	const char *i;
	char *point;
	bool is_ok;

	/*
	 * Discard null and zero-length messages.
	 */
	if ( format == NULL || format[0] == '\0' )
		return;

	{
	list_head_t *to_list = &ch->in_room->characters;
	if ( type == TO_VICT ) {
		if ( vch == NULL ) {
			bug( "Act: null vch with TO_VICT.", 0 );
			return;
		}
		to_list = &vch->in_room->characters;
	}

	LIST_FOR_EACH( to, to_list, CHAR_DATA, room_node ) {
		if ( to->desc == NULL || !IS_AWAKE( to ) )
			continue;

		if ( ch->in_room->vnum == ROOM_VNUM_IN_OBJECT ) {
			is_ok = FALSE;

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
		}
		if ( type == TO_CHAR && to != ch )
			continue;
		if ( type == TO_VICT && ( to != vch || to == ch ) )
			continue;
		if ( type == TO_ROOM && to == ch )
			continue;
		if ( type == TO_NOTVICT && ( to == ch || to == vch ) )
			continue;

		point = buf;
		str = format;
		while ( *str != '\0' ) {
			/* Check for buffer overflow using helper */
			if ( !buf_has_space( point, buf, MAX_STRING_LENGTH, 1, 10 ) ) {
				bug( "kavitem: buffer overflow, truncating message", 0 );
				break;
			}

			if ( *str != '$' ) {
				*point++ = *str++;
				continue;
			}
			++str;

			if ( arg2 == NULL && *str >= 'A' && *str <= 'Z' )
				i = "";
			else {
				switch ( *str ) {
				default:
					i = "";
					break;
				case 'n':
					i = PERS( ch, to );
					break;
				case 'e':
					i = he_she[URANGE( 0, ch->sex, 2 )];
					break;
				case 'm':
					i = him_her[URANGE( 0, ch->sex, 2 )];
					break;
				case 's':
					i = his_her[URANGE( 0, ch->sex, 2 )];
					break;
				case 'p':
					i = can_see_obj( to, obj1 )
						? ( ( obj1->chobj != NULL && obj1->chobj == to )
								  ? "you"
								  : obj1->short_descr )
						: "something";
					break;

				case 'o':
					if ( obj1 != NULL ) {
						snprintf( kav, sizeof(kav), "%s's", obj1->short_descr );
					}
					i = can_see_obj( to, obj1 )
						? ( ( obj1->chobj != NULL && obj1->chobj == to )
								  ? "your"
								  : kav )
						: "something's";
					break;
				}
			}

			++str;
			/* Copy replacement string with bounds checking using helper */
			point = buf_append_safe( point, i, buf, MAX_STRING_LENGTH, 10 );
			if ( point == NULL ) {
				bug( "kavitem: replacement string overflow", 0 );
				break;
			}
		}

		/* Safely add newline using helper */
		if ( buf_has_space( point, buf, MAX_STRING_LENGTH, 2, 0 ) && point != NULL ) {
			*point++ = '\n';
			*point++ = '\r';
		}
		buf[0] = UPPER( buf[0] );
		write_to_buffer( to->desc, buf, (int) ( point - buf ) );
	}
	}

	return;
}
