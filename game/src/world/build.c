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

/****************************************************************************
 *		       Online Building and Editing Module		    *
 *****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#if !defined( WIN32 )
#include <unistd.h>
#endif
#include "merc.h"
#include "../db/db_game.h"

/*
 * Returns value 0 - 9 based on directional text.
 */
int get_dir( char *txt ) {
	int edir;
	char c1, c2;

	c1 = txt[0];
	if ( c1 == '\0' )
		return 0;
	c2 = txt[1];
	edir = 0;
	switch ( c1 ) {
	case 'n':
		switch ( c2 ) {
		default:
			edir = 0;
			break; /* north */
		}
		break;
	case '0':
		edir = 0;
		break; /* north */
	case 'e':
	case '1':
		edir = 1;
		break; /* east  */
	case 's':
		switch ( c2 ) {
		default:
			edir = 2;
			break; /* south */
		}
		break;
	case '2':
		edir = 2;
		break; /* south */
	case 'w':
	case '3':
		edir = 3;
		break; /* west	 */
	case 'u':
	case '4':
		edir = 4;
		break; /* up	 */
	case 'd':
	case '5':
		edir = 5;
		break; /* down	 */
	}
	return edir;
}

char *const ex_flags[] =
	{
		"isdoor", "closed", "locked", "pickproof" };

char *const r_flags[] =
	{
		"dark", "no_otrans",
		"nomob", "indoors",
		"private", "safe",
		"solitary", "petshop",
		"norecall", "noteleport",
		"totaldarkness",
		"blade_barrier", "arena",
		"flaming", "silence" };

char *const o_flags[] =
	{
		"glow", "hum", "thrown",
		"keep", "vanish",
		"invis", "magic",
		"nodrop", "bless",
		"antigood", "antievil",
		"antineutral", "noremove",
		"inventory", "loyal",
		"shadowplane" };

char *const w_flags[] =
	{
		"take", "finger", "neck", "body", "head", "legs", "feet", "hands", "arms",
		"shield", "about", "waist", "wrist", "wield", "hold", "face" };

char *const o_types[] =
	{
		"none", "light", "scroll", "wand", "staff", "weapon",
		"treasure", "armor", "potion", "furniture", "trash", "container",
		"drinkcon", "key", "food", "money", "boat",
		"corpse", "corpse_pc", "fountain", "pill",
		"portal", "egg",
		"voodoo", "stake",
		"missile", "ammo",
		"quest", "questcard",
		"symbol", "book",
		"page", "tool",
		"wall", "copper",
		"iron", "steel",
		"adamantite", "gemstone",
		"hilt" };

char *const a_types[] =
	{
		"none", "strength", "dexterity", "intelligence", "wisdom", "constitution",
		"sex", "class", "level", "age", "height", "weight", "mana", "hit", "move",
		"gold", "experience", "armor", "hitroll", "damroll", "save_para",
		"save_rod", "save_petri",
		"save_breath",
		"save_spell",
		"poly" };

char *const a_flags[] =
	{
		"blind", "invisible", "detect_evil", "detect_invis", "detect_magic",
		"detect_hidden", "shadowplane", "sanctuary", "faerie_fire", "infrared",
		"curse",
		"flaming", "poison", "protect", "ethereal", "sneak", "hide", "sleep",
		"charm", "flying", "pass_door", "polymorph", "shadowsight", "webbed",
		"tendrils", "drowfire", "zuloform", "shift", "peace", "infirmity",
		"godbless", "totalblind",
		"steelshield", "monkflaming" };
/*
char *	const	act_flags [] =
{
"npc", "sentinel", "scavenger", "aggressive", "stayarea", "wimpy", "pet",
"train", "practice", "mount", "noparts", "noexp", "prototype", "r1" };
*/

char *const plr_flags[] =
	{
		"npc", "autoexit", "autoloot", "autosac", "blank", "brief", "combine",
		"prompt", "telnet_ga", "holylight", "wizinvis", "ansi", "silence",
		"vt102", "incog", "notell", "log", "deny", "freeze", "godless",
		"watcher", "acid", "challenger", "challenged",
		"r1", "r2" };

char *const wear_locs[] =
	{
		"light", "finger1", "finger2", "neck1", "neck2", "body", "head", "legs",
		"feet", "hands", "arms", "shield", "about", "waist", "wrist1", "wrist2",
		"wield", "hold", "third", "fourth", "r1", "r2" };

bool can_rmodify( CHAR_DATA *ch, ROOM_INDEX_DATA *room ) {
	if ( IS_NPC( ch ) )
		return FALSE;
	if ( get_trust( ch ) >= MAX_LEVEL - 1 )
		return TRUE;
	if ( !IS_SET( room->room_flags, ROOM_PROTOTYPE ) ) {
		send_to_char( "You cannot modify this room.\n\r", ch );
		return FALSE;
	}

	send_to_char( "That room is not in your allocated range.\n\r", ch );
	return FALSE;
}

bool can_oedit( CHAR_DATA *ch, OBJ_INDEX_DATA *obj ) {

	if ( IS_NPC( ch ) )
		return FALSE;
	if ( get_trust( ch ) >= MAX_LEVEL - 1 )
		return TRUE;
	if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
		send_to_char( "You cannot modify this object.\n\r", ch );
		return FALSE;
	}

	send_to_char( "That object is not in your allocated range.\n\r",
		ch );
	return FALSE;
}

bool can_mmodify( CHAR_DATA *ch, CHAR_DATA *mob ) {

	if ( mob == ch )
		return TRUE;

	if ( !IS_NPC( mob ) ) {
		if ( get_trust( ch ) >= MAX_LEVEL - 1 && get_trust( ch ) > get_trust( mob ) )
			return TRUE;
		else {
			send_to_char( "You can't do that.\n\r", ch );
			return FALSE;
		}
	}

	if ( IS_NPC( ch ) )
		return FALSE;
	if ( get_trust( ch ) >= MAX_LEVEL - 1 )
		return TRUE;
	if ( !IS_SET( mob->act, ACT_PROTOTYPE ) ) {
		send_to_char( "You cannot modify this mobile.\n\r", ch );
		return FALSE;
	}

	send_to_char( "That mobile is not in your allocated range.\n\r",
		ch );
	return FALSE;
}

bool can_medit( CHAR_DATA *ch, MOB_INDEX_DATA *mob ) {
	if ( IS_NPC( ch ) )
		return FALSE;
	if ( get_trust( ch ) >= MAX_LEVEL - 1 )
		return TRUE;
	if ( !IS_SET( mob->act, ACT_PROTOTYPE ) ) {
		send_to_char( "You cannot modify this mobile.\n\r", ch );
		return FALSE;
	}

	send_to_char( "That mobile is not in your allocated range.\n\r",
		ch );
	return FALSE;
}

int get_otype( char *type ) {
	int x;

	for ( x = 0; x < ( sizeof( o_types ) / sizeof( o_types[0] ) ); x++ )
		if ( !str_cmp( type, o_types[x] ) )
			return x;
	return -1;
}

int get_wearloc( char *type ) {
	int x;

	for ( x = 0; x < MAX_WEAR; x++ )
		if ( !str_cmp( type, wear_locs[x] ) )
			return x;
	return -1;
}

int get_exflag( char *flag ) {
	int x;

	for ( x = 0; x <= MAX_EXFLAG; x++ )
		if ( !str_cmp( flag, ex_flags[x] ) )
			return x;
	return -1;
}

int get_rflag( char *flag ) {
	int x;

	for ( x = 0; x < 32; x++ )
		if ( !str_cmp( flag, r_flags[x] ) )
			return x;
	return -1;
}

int get_oflag( char *flag ) {
	int x;

	for ( x = 0; x < 32; x++ )
		if ( !str_cmp( flag, o_flags[x] ) )
			return x;
	return -1;
}

int get_wflag( char *flag ) {
	int x;

	for ( x = 0; x < 32; x++ )
		if ( !str_cmp( flag, w_flags[x] ) )
			return x;
	return -1;
}

int get_plrflag( char *flag ) {
	int x;

	for ( x = 0; x < 32; x++ )
		if ( !str_cmp( flag, plr_flags[x] ) )
			return x;
	return -1;
}

/*
 * Simple but nice and handle line editor.			-Thoric
 */
void edit_buffer( CHAR_DATA *ch, char *argument ) {
	DESCRIPTOR_DATA *d;
	EDITOR_DATA *edit;
	char cmd[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int x, line, max_buf_lines;
	bool save;

	if ( ( d = ch->desc ) == NULL ) {
		send_to_char( "You have no descriptor.\n\r", ch );
		return;
	}

	if ( d->connected != CON_EDITING ) {
		send_to_char( "You can't do that!\n\r", ch );
		bug( "Edit_buffer: d->connected != CON_EDITING", 0 );
		return;
	}

	if ( ch->substate <= SUB_PAUSE ) {
		send_to_char( "You can't do that!\n\r", ch );
		snprintf( buf, sizeof( buf ), "Edit_buffer: illegal ch->substate (%d)", ch->substate );
		bug( buf, 0 );
		d->connected = CON_PLAYING;
		return;
	}

	if ( !ch->editor ) {
		send_to_char( "You can't do that!\n\r", ch );
		bug( "Edit_buffer: null editor", 0 );
		d->connected = CON_PLAYING;
		return;
	}

	edit = ch->editor;
	save = FALSE;
	max_buf_lines = 24;

	if ( ch->substate == SUB_MPROG_EDIT || ch->substate == SUB_HELP_EDIT )
		max_buf_lines = 48;

	if ( argument[0] == '/' || argument[0] == '\\' ) {
		one_argument( argument, cmd );
		if ( !str_cmp( cmd + 1, "?" ) ) {
			send_to_char( "Editing commands\n\r---------------------------------\n\r", ch );
			send_to_char( "/l              list buffer\n\r", ch );
			send_to_char( "/c              clear buffer\n\r", ch );
			send_to_char( "/d [line]       delete line\n\r", ch );
			send_to_char( "/g <line>       goto line\n\r", ch );
			send_to_char( "/i <line>       insert line\n\r", ch );
			send_to_char( "/r <old> <new>  global replace\n\r", ch );
			send_to_char( "/a              abort editing\n\r", ch );
			if ( get_trust( ch ) > MAX_LEVEL - 4 )
				send_to_char( "/! <command>    execute command (do not use another editing command)\n\r", ch );
			send_to_char( "/s              save buffer\n\r\n\r> ", ch );
			return;
		}
		if ( !str_cmp( cmd + 1, "c" ) ) {
			memset( edit, '\0', sizeof( EDITOR_DATA ) );
			edit->numlines = 0;
			edit->on_line = 0;
			send_to_char( "Buffer cleared.\n\r> ", ch );
			return;
		}
		if ( !str_cmp( cmd + 1, "r" ) ) {
			char word1[MAX_INPUT_LENGTH];
			char word2[MAX_INPUT_LENGTH];
			char *sptr, *wptr, *lwptr;
			int x, count, wordln, lineln;

			sptr = one_argument( argument, word1 );
			sptr = one_argument( sptr, word1 );
			sptr = one_argument( sptr, word2 );
			if ( word1[0] == '\0' || word2[0] == '\0' ) {
				send_to_char( "Need word to replace, and replacement.\n\r>", ch );
				return;
			}
			if ( strcmp( word1, word2 ) == 0 ) {
				send_to_char( "Done.\n\r> ", ch );
				return;
			}
			count = 0;
			wordln = (int) strlen( word1 );
			(void) strlen( word2 );
			snprintf( buf, sizeof( buf ), "Replacing all occurrences of %s with %s...\n\r", word1, word2 );
			stc( buf, ch );
			for ( x = edit->on_line; x < edit->numlines; x++ ) {
				lwptr = edit->line[x];
				while ( ( wptr = strstr( lwptr, word1 ) ) != NULL ) {
					sptr = lwptr;
					lwptr = wptr + wordln;
					snprintf( buf, sizeof( buf ), "%s%s", word2, wptr + wordln );
					lineln = (int) ( wptr - edit->line[x] ) - wordln;
					++count;
					if ( (int) strlen( buf ) + lineln > 79 ) {
						lineln = UMAX( 0, ( 79 - (int) strlen( buf ) ) );
						buf[lineln] = '\0';
						break;
					} else
						lineln = (int) strlen( buf );
					buf[lineln] = '\0';
					strcpy( wptr, buf );
				}
			}
			snprintf( buf, sizeof( buf ), "Found and replaced %d occurrence(s).\n\r> ", count );
			stc( buf, ch );
			return;
		}

		if ( !str_cmp( cmd + 1, "i" ) ) {
			if ( edit->numlines >= max_buf_lines )
				send_to_char( "Buffer is full.\n\r> ", ch );
			else {
				if ( argument[2] == ' ' )
					line = atoi( argument + 2 ) - 1;
				else
					line = edit->on_line;
				if ( line < 0 )
					line = edit->on_line;
				if ( line < 0 || line > edit->numlines )
					send_to_char( "Out of range.\n\r> ", ch );
				else {
					for ( x = ++edit->numlines; x > line; x-- )
						strcpy( edit->line[x], edit->line[x - 1] );
					strcpy( edit->line[line], "" );
					send_to_char( "Line inserted.\n\r> ", ch );
				}
			}
			return;
		}
		if ( !str_cmp( cmd + 1, "d" ) ) {
			if ( edit->numlines == 0 )
				send_to_char( "Buffer is empty.\n\r> ", ch );
			else {
				if ( argument[2] == ' ' )
					line = atoi( argument + 2 ) - 1;
				else
					line = edit->on_line;
				if ( line < 0 )
					line = edit->on_line;
				if ( line < 0 || line > edit->numlines )
					send_to_char( "Out of range.\n\r> ", ch );
				else {
					if ( line == 0 && edit->numlines == 1 ) {
						memset( edit, '\0', sizeof( EDITOR_DATA ) );
						edit->numlines = 0;
						edit->on_line = 0;
						send_to_char( "Line deleted.\n\r> ", ch );
						return;
					}
					for ( x = line; x < ( edit->numlines - 1 ); x++ )
						strcpy( edit->line[x], edit->line[x + 1] );
					strcpy( edit->line[edit->numlines--], "" );
					if ( edit->on_line > edit->numlines )
						edit->on_line = edit->numlines;
					send_to_char( "Line deleted.\n\r> ", ch );
				}
			}
			return;
		}
		if ( !str_cmp( cmd + 1, "g" ) ) {
			if ( edit->numlines == 0 )
				send_to_char( "Buffer is empty.\n\r> ", ch );
			else {
				if ( argument[2] == ' ' )
					line = atoi( argument + 2 ) - 1;
				else {
					send_to_char( "Goto what line?\n\r> ", ch );
					return;
				}
				if ( line < 0 )
					line = edit->on_line;
				if ( line < 0 || line > edit->numlines )
					send_to_char( "Out of range.\n\r> ", ch );
				else {
					edit->on_line = line;
					snprintf( buf, sizeof( buf ), "(On line %d)\n\r> ", line + 1 );
					stc( buf, ch );
				}
			}
			return;
		}
		if ( !str_cmp( cmd + 1, "l" ) ) {
			if ( edit->numlines == 0 )
				send_to_char( "Buffer is empty.\n\r> ", ch );
			else {
				send_to_char( "------------------\n\r", ch );
				for ( x = 0; x < edit->numlines; x++ ) {
					snprintf( buf, sizeof( buf ), "%2d> %s\n\r", x + 1, edit->line[x] );
					stc( buf, ch );
				}
				send_to_char( "------------------\n\r> ", ch );
			}
			return;
		}
		if ( !str_cmp( cmd + 1, "a" ) ) {
			send_to_char( "\n\rAborting... ", ch );
			stop_editing( ch );
			return;
		}
		if ( get_trust( ch ) > LEVEL_IMMORTAL && !str_cmp( cmd + 1, "!" ) ) {
			int substate = ch->substate;

			ch->substate = SUB_RESTRICTED;
			interpret( ch, argument + 3 );
			ch->substate = substate;
			send_to_char( "\n\r> ", ch );
			return;
		} /*
		 if ( !str_cmp( cmd+1, "s" ) )
		 {
			 d->connected = CON_PLAYING;
			 do_hedit(ch, "");
				 if ( !ch->last_cmd )
				   return;
				 (*ch->last_cmd) ( ch, "" );
				 return;
		 }*/
	}

	if ( edit->size + strlen( argument ) + 1 >= MAX_STRING_LENGTH - 1 )
		send_to_char( "You buffer is full.\n\r", ch );
	else {
		if ( strlen( argument ) > 79 ) {
			strncpy( buf, argument, 79 );
			buf[79] = 0;
			send_to_char( "(Long line trimmed)\n\r> ", ch );
		} else
			strcpy( buf, argument );
		strcpy( edit->line[edit->on_line++], buf );
		if ( edit->on_line > edit->numlines )
			edit->numlines++;
		if ( edit->numlines > max_buf_lines ) {
			edit->numlines = max_buf_lines;
			send_to_char( "Buffer full.\n\r", ch );
			save = TRUE;
		}
	}

	if ( save ) {
		d->connected = CON_PLAYING;
		if ( !ch->last_cmd )
			return;
		( *ch->last_cmd )( ch, "" );
		return;
	}
	send_to_char( "> ", ch );
}

/*
 * Remove carriage returns from a line
 */
char *strip_cr( char *str ) {
	static char newstr[MAX_STRING_LENGTH];
	int i, j;

	for ( i = j = 0; str[i] != '\0'; i++ )
		if ( str[i] != '\r' ) {
			newstr[j++] = str[i];
		}
	newstr[j] = '\0';
	return newstr;
}

void smush_tilde( char *str ) {
	int len;
	char last;
	char *strptr;

	strptr = str;

	len = (int) strlen( str );
	if ( len )
		last = strptr[len - 1];
	else
		last = '\0';

	for ( ; *str != '\0'; str++ ) {
		if ( *str == '~' )
			*str = '-';
	}
	if ( len )
		strptr[len - 1] = last;

	return;
}

void start_editing( CHAR_DATA *ch, char *data ) {
	EDITOR_DATA *edit;
	int lines, size, lpos;
	char c;

	if ( !ch->desc ) {
		bug( "Fatal: start_editing: no desc", 0 );
		return;
	}
	if ( ch->substate == SUB_RESTRICTED )
		bug( "NOT GOOD: start_editing: ch->substate == SUB_RESTRICTED", 0 );

	send_to_char( "Begin entering your text now (/? = help /s = save /c = clear /l = list)\n\r", ch );
	send_to_char( "-----------------------------------------------------------------------\n\r> ", ch );
	if ( ch->editor )
		stop_editing( ch );

	edit = calloc( 1, sizeof( EDITOR_DATA ) );
	edit->numlines = 0;
	edit->on_line = 0;
	edit->size = 0;
	size = 0;
	lpos = 0;
	lines = 0;
	if ( !data )
		bug( "editor: data is NULL!\n\r", 0 );
	else
		for ( ;; ) {
			c = data[size++];
			if ( c == '\0' ) {
				edit->line[lines][lpos] = '\0';
				break;
			} else if ( c == '\r' )
				;
			else if ( c == '\n' || lpos > 78 ) {
				edit->line[lines][lpos] = '\0';
				lines++;
				lpos = 0;
			} else
				edit->line[lines][lpos++] = c;
			if ( lines >= 49 || size > 4096 ) {
				edit->line[lines][lpos] = '\0';
				break;
			}
		}
	edit->numlines = lines;
	edit->size = size;
	edit->on_line = lines;
	ch->editor = edit;
	ch->desc->connected = CON_EDITING;
}

char *copy_buffer( CHAR_DATA *ch ) {
	char buf[MAX_STRING_LENGTH];
	char tmp[100];
	int x, len;

	if ( !ch ) {
		bug( "copy_buffer: null ch", 0 );
		return str_dup( "" );
	}

	if ( !ch->editor ) {
		bug( "copy_buffer: null editor", 0 );
		return str_dup( "" );
	}

	buf[0] = '\0';
	for ( x = 0; x < ch->editor->numlines; x++ ) {
		strcpy( tmp, ch->editor->line[x] );
		smush_tilde( tmp );
		len = (int) strlen( tmp );
		if ( tmp[0] != '\0' && tmp[len - 1] == '~' )
			tmp[len - 1] = '\0';
		else
			strcat( tmp, "\n\r" );
		strcat( buf, tmp );
	}
	return str_dup( buf );
}

void stop_editing( CHAR_DATA *ch ) {
	free( ch->editor );
	ch->editor = NULL;
	send_to_char( "Done.\n\r", ch );
	ch->dest_buf = NULL;
	ch->spare_ptr = NULL;
	ch->substate = SUB_NONE;
	if ( !ch->desc ) {
		bug( "Fatal: stop_editing: no desc", 0 );
		return;
	}
	ch->desc->connected = CON_PLAYING;
}

/*
 * Moved into a separate function so it can be used for other things
 * ie: online help editing				-Thoric
 */
HELP_DATA *get_help( CHAR_DATA *ch, char *argument ) {
	char argall[MAX_INPUT_LENGTH];
	char argone[MAX_INPUT_LENGTH];
	char argnew[MAX_INPUT_LENGTH];
	HELP_DATA *pHelp;
	int lev;

	if ( argument[0] == '\0' )
		argument = "summary";

	if ( isdigit( argument[0] ) ) {
		lev = number_argument( argument, argnew );
		argument = argnew;
	} else
		lev = -2;
	/*
	 * Tricky argument handling so 'help a b' doesn't match a.
	 */
	argall[0] = '\0';
	while ( argument[0] != '\0' ) {
		argument = one_argument( argument, argone );
		if ( argall[0] != '\0' )
			strcat( argall, " " );
		strcat( argall, argone );
	}

	LIST_FOR_EACH( pHelp, &g_helps, HELP_DATA, node ) {
		if ( pHelp->level > get_trust( ch ) )
			continue;
		if ( lev != -2 && pHelp->level != lev )
			continue;

		if ( is_name( argall, pHelp->keyword ) )
			return pHelp;
	}

	return NULL;
}

/*
 * Now this is cleaner
 */
void do_help( CHAR_DATA *ch, char *argument ) {
	HELP_DATA *pHelp;

	if ( ( pHelp = get_help( ch, argument ) ) == NULL ) {
		send_to_char( "No help on that word.\n\r", ch );
		return;
	}

	if ( pHelp->level >= 0 && str_cmp( argument, "imotd" ) ) {
		stc( pHelp->keyword, ch );
		stc( "\n\r", ch );
	}

	/*
	 * Strip leading '.' to allow initial blanks.
	 */
	if ( pHelp->text[0] == '.' )
		stc( pHelp->text + 1, ch );
	else
		stc( pHelp->text, ch );
	return;
}

void do_hset( CHAR_DATA *ch, char *argument ) {
	HELP_DATA *pHelp;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	smash_tilde( argument );
	argument = one_argument( argument, arg1 );
	if ( arg1[0] == '\0' ) {
		send_to_char( "Syntax: hset <field> [value] [help page]\n\r", ch );
		send_to_char( "\n\r", ch );
		send_to_char( "Field being one of:\n\r", ch );
		send_to_char( "  level keyword remove save\n\r", ch );
		return;
	}

	if ( !str_cmp( arg1, "save" ) ) {
		db_game_save_helps();
		send_to_char( "Saved.\n\r", ch );
		return;
	}
	if ( str_cmp( arg1, "remove" ) )
		argument = one_argument( argument, arg2 );

	if ( ( pHelp = get_help( ch, argument ) ) == NULL ) {
		send_to_char( "Cannot find help on that subject.\n\r", ch );
		return;
	}
	if ( !str_cmp( arg1, "remove" ) ) {
		list_remove( &g_helps, &pHelp->node );
		free( pHelp->text );
		free( pHelp->keyword );
		free( pHelp );
		send_to_char( "Removed.\n\r", ch );
		return;
	}
	if ( !str_cmp( arg1, "level" ) ) {
		pHelp->level = atoi( arg2 );
		send_to_char( "Done.\n\r", ch );
		return;
	}
	if ( !str_cmp( arg1, "keyword" ) ) {
		free( pHelp->keyword );
		pHelp->keyword = str_dup( strupper( arg2 ) );
		send_to_char( "Done.\n\r", ch );
		return;
	}

	do_hset( ch, "" );
}

/*
 * Show help topics in a level range				-Thoric
 * Idea suggested by Gorog
 */
void do_hlist( CHAR_DATA *ch, char *argument ) {
	int min, max, minlimit, maxlimit, cnt;
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	HELP_DATA *help;

	maxlimit = get_trust( ch );
	minlimit = maxlimit >= ( MAX_LEVEL - 3 ) ? -1 : 0;
	argument = one_argument( argument, arg );
	if ( arg[0] != '\0' ) {
		min = URANGE( minlimit, atoi( arg ), maxlimit );
		if ( argument[0] != '\0' )
			max = URANGE( min, atoi( argument ), maxlimit );
		else
			max = maxlimit;
	} else {
		min = minlimit;
		max = maxlimit;
	}
	snprintf( buf, sizeof( buf ), "Help Topics in level range %d to %d:\n\r\n\r", min, max );
	stc( buf, ch );
	cnt = 0;
	LIST_FOR_EACH( help, &g_helps, HELP_DATA, node )
		if ( help->level >= min && help->level <= max ) {
			snprintf( buf, sizeof( buf ), "  %3d %s\n\r", help->level, help->keyword );
			stc( buf, ch );
			++cnt;
		}
	if ( cnt ) {
		snprintf( buf, sizeof( buf ), "\n\r%d pages found.\n\r", cnt );
		stc( buf, ch );
	} else
		send_to_char( "None found.\n\r", ch );
}
