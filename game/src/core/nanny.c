/***************************************************************************
 *  Nanny and login functions extracted from comm.c                        *
 *  Contains: nanny, check_parse_name, check_reconnect, check_kickoff,     *
 *            check_playing, check_pedit_editing, stop_idling              *
 ***************************************************************************/

#include "merc.h"
#include "utf8.h"
#include "../db/db_player.h"
#include "../db/db_game.h"
#include "../systems/gmcp.h"
#include "../systems/mcmp.h"
#include "../systems/ttype.h"
#include "../systems/charset.h"

/* External variables from comm.c */
extern char echo_off_str[];
extern char echo_on_str[];
extern bool wizlock;
extern GAMECONFIG_DATA game_config;

/* Forward declarations for functions defined later in this file */
bool check_parse_name( char *name );
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn );
bool check_kickoff( DESCRIPTOR_DATA *d, char *name, bool fConn );
bool check_playing( DESCRIPTOR_DATA *d, char *name );
void check_pedit_editing( const char *name );

/* External function from comm.c */
bool check_banned( DESCRIPTOR_DATA *d );

/*
 * Clear disciplines for classes the character doesn't belong to.
 * Called when a character finishes logging in.
 */
static void clear_foreign_disciplines( CHAR_DATA *ch ) {
	if ( IS_IMMORTAL( ch ) )
		return;

	if ( !IS_CLASS( ch, CLASS_DEMON ) ) {
		ch->power[DISC_DAEM_ATTA] = -2;
		ch->power[DISC_DAEM_IMMU] = -2;
		ch->power[DISC_DAEM_TEMP] = -2;
		ch->power[DISC_DAEM_CORR] = -2;
		ch->power[DISC_DAEM_HELL] = -2;
		ch->power[DISC_DAEM_GELU] = -2;
		ch->power[DISC_DAEM_MORP] = -2;
		ch->power[DISC_DAEM_NETH] = -2;
		ch->power[DISC_DAEM_DISC] = -2;
	}
	if ( !IS_CLASS( ch, CLASS_WEREWOLF ) ) {
		ch->power[DISC_WERE_BEAR] = -2;
		ch->power[DISC_WERE_LYNX] = -2;
		ch->power[DISC_WERE_BOAR] = -2;
		ch->power[DISC_WERE_OWL] = -2;
		ch->power[DISC_WERE_SPID] = -2;
		ch->power[DISC_WERE_WOLF] = -2;
		ch->power[DISC_WERE_HAWK] = -2;
		ch->power[DISC_WERE_MANT] = -2;
		ch->power[DISC_WERE_RAPT] = -2;
		ch->power[DISC_WERE_LUNA] = -2;
		ch->power[DISC_WERE_PAIN] = -2;
		ch->power[DISC_WERE_CONG] = -2;
	}
	if ( !IS_CLASS( ch, CLASS_VAMPIRE ) ) {
		ch->power[DISC_VAMP_FORT] = -2;
		ch->power[DISC_VAMP_CELE] = -2;
		ch->power[DISC_VAMP_OBTE] = -2;
		ch->power[DISC_VAMP_PRES] = -2;
		ch->power[DISC_VAMP_QUIE] = -2;
		ch->power[DISC_VAMP_THAU] = -2;
		ch->power[DISC_VAMP_AUSP] = -2;
		ch->power[DISC_VAMP_DOMI] = -2;
		ch->power[DISC_VAMP_OBFU] = -2;
		ch->power[DISC_VAMP_POTE] = -2;
		ch->power[DISC_VAMP_PROT] = -2;
		ch->power[DISC_VAMP_SERP] = -2;
		ch->power[DISC_VAMP_VICI] = -2;
		ch->power[DISC_VAMP_DAIM] = -2;
		ch->power[DISC_VAMP_ANIM] = -2;
		ch->power[DISC_VAMP_CHIM] = -2;
		ch->power[DISC_VAMP_MELP] = -2;
		ch->power[DISC_VAMP_NECR] = -2;
		ch->power[DISC_VAMP_THAN] = -2;
		ch->power[DISC_VAMP_OBEA] = -2;
	}
}

/*
 * Remove illegal items from character's inventory on login.
 * - Weapons with value 81 (mod 1000)
 * - Artifacts (for returning players only, not new characters)
 */
static void remove_illegal_items( CHAR_DATA *ch, bool remove_artifacts ) {
	OBJ_DATA *obj, *obj_next;

	if ( IS_NPC( ch ) )
		return;

	LIST_FOR_EACH_SAFE( obj, obj_next, &ch->carrying, OBJ_DATA, content_node ) {
		if ( remove_artifacts && IS_SET( obj->quest, QUEST_ARTIFACT ) ) {
			extract_obj( obj );
			continue;
		}
		if ( obj->item_type == ITEM_WEAPON ) {
			int value = obj->value[0] % 1000;
			if ( value == 81 ) {
				do_remove( ch, obj->name );
				extract_obj( obj );
			}
		}
	}
}

/*
 * Per-state handler functions for the login state machine.
 * Each handles one CON_* state, called from the nanny() dispatch below.
 */

static void nanny_get_name( DESCRIPTOR_DATA *d, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char kav[MAX_STRING_LENGTH];
	CHAR_DATA *ch = d->character;
	bool fOld;
	int char_age;

	/* Finalize charset as ASCII if negotiation got no response */
	charset_finalize( d );

	if ( argument[0] == '\0' ) {
		close_socket( d );
		return;
	}

	if ( d->lookup_status == STATUS_LOOKUP ) {
		write_to_buffer( d, "\n\r* Making a dns lookup, please have patience. (this could take 10-15 seconds)\n\r What be thy name? ", 0 );
		return;
	}

	if ( check_banned( d ) ) {
		write_to_buffer( d, " Your site has been banned from this mud\n\r", 0 );
		close_socket( d );
		return;
	}

	argument[0] = UPPER( argument[0] );
	/* Normalize name: first letter upper, rest lower for case-insensitive uniqueness */
	for ( int i = 1; argument[i] != '\0'; i++ )
		argument[i] = LOWER( argument[i] );
	if ( !check_parse_name( argument ) && strcmp( argument, "Vladd" ) != 0 ) {
		write_to_buffer( d, " Illegal name, try another.\n\r Enter thy name brave traveler: ", 0 );
		return;
	}

	sprintf( kav, "%s trying to connect.", argument );
	log_string( kav );
	fOld = load_char_short( d, argument );
	ch = d->character;

	/* For new players, d->character is NULL - create properly initialized character */
	if ( !fOld || ch == NULL ) {
		/* New player - use proper initialization function */
		init_char_for_load( d, argument );
		ch = d->character;

		sprintf( buf, " You want %s engraved on your tombstone (Y/N)? ", argument );
		write_to_buffer( d, buf, 0 );
		d->connected = CON_CONFIRM_NEW_NAME;
		return;
	}

	char_age = years_old( ch );
	if ( IS_SET( ch->act, PLR_DENY ) ) {
		sprintf( log_buf, "Denying access to %s@%s.", argument, ch->lasthost );
		log_string( log_buf );
		write_to_buffer( d, " You are denied access.\n\r", 0 );
		close_socket( d );
		return;
	} else if ( IS_EXTRA( ch, EXTRA_BORN ) && char_age < 15 ) {
		char agebuf[MAX_INPUT_LENGTH];
		if ( char_age == 14 )
			sprintf( agebuf, "You cannot play for another year.\n\r" );
		else
			sprintf( agebuf, "You cannot play for another %d years.\n\r",
				( 15 - years_old( ch ) ) );
		write_to_buffer( d, agebuf, 0 );
		close_socket( d );
		return;
	}

	if ( check_reconnect( d, argument, FALSE ) ) {
		fOld = TRUE;
	} else {
		/* Check max number of players - KaVir */

		int countdesc = list_count( &g_descriptors );
		int max_players = 150;

		if ( countdesc > max_players && !IS_IMMORTAL( ch ) ) {
			write_to_buffer( d, " Too many players connected, please try again in a couple of minutes.\n\r", 0 );
			close_socket( d );
			return;
		}

		if ( wizlock && !IS_IMMORTAL( ch ) ) {
			write_to_buffer( d, " Currenly wizlocked.\n\r", 0 );
			close_socket( d );
			return;
		}
	}

	/* If we got here, it's an old player */
	write_to_buffer( d, " Please enter password: ", 0 );
	write_to_buffer( d, echo_off_str, 0 );
	d->connected = CON_GET_OLD_PASSWORD;
}

static void nanny_get_old_password( DESCRIPTOR_DATA *d, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	char kav[MAX_STRING_LENGTH];
	CHAR_DATA *ch = d->character;
	char *strtime;
	bool fOld;
	int i;

#if defined( unix )
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( ch == NULL || ( !IS_EXTRA( ch, EXTRA_NEWPASS ) && strcmp( argument, ch->pcdata->pwd ) && strcmp( crypt( argument, ch->name ), ch->pcdata->pwd ) ) ) {
		write_to_buffer( d, " Wrong password.\n\r", 0 );
		close_socket( d );
		return;
	}

	else if ( ch == NULL || ( IS_EXTRA( ch, EXTRA_NEWPASS ) && strcmp( crypt( argument, ch->name ), ch->pcdata->pwd ) ) ) {
		write_to_buffer( d, " Wrong password.\n\r", 0 );
		close_socket( d );
		return;
	}

	write_to_buffer( d, echo_on_str, 0 );

	if ( check_reconnect( d, GET_PC_NAME( ch ), TRUE ) )
		return;

	if ( check_playing( d, GET_PC_NAME( ch ) ) )
		return;

	/* If someone is editing this player via pedit, save and close their session */
	check_pedit_editing( GET_PC_NAME( ch ) );

	if ( ch->level > 1 ) {
		sprintf( kav, "%s", ch->pcdata->switchname );
		free_char( d->character );
		d->character = NULL;
		fOld = load_char_obj( d, kav );
		ch = d->character;
	}

	if ( !IS_EXTRA( ch, EXTRA_NEWPASS ) && strlen( argument ) > 1 ) {
		sprintf( kav, "%s %s", argument, argument );
		do_password( ch, kav );
	}

	if ( ch->lasthost != NULL ) free(ch->lasthost);
	if ( ch->desc != NULL && ch->desc->host != NULL ) {
		ch->lasthost = str_dup( ch->desc->host );
	} else {
		ch->lasthost = str_dup( "(unknown)" );
	}
	strtime = ctime( &current_time );
	strtime[strlen( strtime ) - 1] = '\0';
	free(ch->lasttime);
	ch->lasttime = str_dup( strtime );
	sprintf( log_buf, "%s@%s has connected.", ch->name, mask_ip( ch->lasthost ) );
	log_string( log_buf );

	/* In case we have level 4+ players from another merc mud, or
	 * players who have somehow got file access and changed their pfiles.
	 */

	if ( ch->level < 4 && ch->trust < 4 )
		ch->trust = 0;
	if ( ch->level > 3 && ch->trust == 0 )
		ch->level = 3;
	else {

		if ( ch->level > MAX_LEVEL )
			ch->level = MAX_LEVEL;
		if ( ch->trust > MAX_LEVEL )
			ch->trust = MAX_LEVEL;
		/* To temporarily grant higher powers...
		if ( ch->trust > ch->level)
		ch->trust = ch->level;
		*/
	}

	list_push_back( &g_characters, &ch->char_node );
	d->connected = CON_PLAYING;

	/* MTTS auto-upgrade: apply detected terminal capabilities */
	ttype_apply_mtts( d, ch );

	if ( IS_HERO( ch ) ) {
		do_help( ch, "motd" );
		send_to_char( "\n\r", ch );
	}

	if ( IS_CLASS( ch, CLASS_WEREWOLF ) ) {
		ch->gnosis[GCURRENT] = ch->gnosis[GMAXIMUM] - 5;
		if ( ch->gnosis[GCURRENT] < 0 ) ch->gnosis[GCURRENT] = 0;
	}

	if ( ch->mounted == IS_RIDING ) do_dismount( ch, "" );

	if ( !IS_SET( ch->extra, EXTRA_TRUSTED ) )
		SET_BIT( ch->extra, EXTRA_TRUSTED );

	if ( ch->level == 0 ) {
		ch->pcdata->awins = 0;	 /* arena wins           */
		ch->pcdata->alosses = 0; /* arena losses         */
		ch->pcdata->board = &boards[0];
		ch->gladiator = NULL; /* set player to bet on to NULL */
		ch->challenger = NULL;
		ch->challenged = NULL;
		ch->level = 1;
		ch->exp = 0;
		ch->hit = ch->max_hit;
		ch->mana = ch->max_mana;
		ch->move = ch->max_move;
		ch->special = 0;
		set_switchname( ch, ch->name );
		char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
		do_look( ch, "auto" );
	} else if ( !IS_NPC( ch ) && ch->pcdata->obj_vnum != 0 ) {
		if ( ch->in_room != NULL )
			char_to_room( ch, ch->in_room );
		else
			char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
		bind_char( ch );
		return;
	} else if ( ch->in_room != NULL ) {
		char_to_room( ch, ch->in_room );
		do_look( ch, "auto" );
	} else if ( IS_IMMORTAL( ch ) ) {
		char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
		do_look( ch, "auto" );
	} else {
		char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
		do_look( ch, "auto" );
	}
	do_board( ch, "" );

	/* Send initial GMCP data now that character is fully loaded */
	gmcp_send_char_data( ch );

	players_logged++;
	if ( ch->level > 6 )
		; /* no info */
	else if ( IS_SET( ch->pcdata->jflags, JFLAG_SETLOGIN ) )
		login_message( ch );
	else {
		if ( !ragnarok )
			sprintf( buf, "#2%s #7enters #R%s.#n",
				ch->name, game_config.game_name );
		else
			sprintf( buf, "#2%s #7enters #R%s #y(#0Ragnarok#y).#n",
				ch->name, game_config.game_name );
		enter_info( buf );
	}

	ch->fight_timer = 0;
	update_revision( ch ); // changed the pfile structure ?
	act( "$n has entered the game.", ch, NULL, NULL, TO_ROOM );

	remove_illegal_items( ch, FALSE );
	room_text( ch, ">ENTER<" );

	for ( i = 0; i < MAX_DISCIPLINES; i++ ) {
		if ( ch->power[i] > 10 && !IS_SET( ch->extra, EXTRA_FLASH ) && !IS_SET( ch->extra, EXTRA_BAAL ) )
			ch->power[i] = 10;
	}

	if ( ch->level > 2 )
		clear_foreign_disciplines( ch );

	ch->embraced = NULL;
	ch->embracing = NULL;
	multicheck( ch );
}

static void nanny_confirm_new_name( DESCRIPTOR_DATA *d, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *ch = d->character;

	switch ( *argument ) {
	case 'y':
	case 'Y':
		sprintf( buf, " New character.\n\r Give me a password for %s: %s",
			ch->name, echo_off_str );
		write_to_buffer( d, buf, 0 );
		d->connected = CON_GET_NEW_PASSWORD;
		break;

	case 'n':
	case 'N':
		write_to_buffer( d, " Ok, what IS it, then? ", 0 );
		free_char( d->character );
		d->character = NULL;
		d->connected = CON_GET_NAME;
		break;

	default:
		write_to_buffer( d, " Please type Yes or No? ", 0 );
		break;
	}
}

static void nanny_get_new_password( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;
	char *pwdnew;
	char *p;

#if defined( unix )
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( strlen( argument ) < 5 ) {
		write_to_buffer( d,
			" Password must be at least five characters long.\n\r Password: ",
			0 );
		return;
	}

	pwdnew = crypt( argument, ch->name );

	for ( p = pwdnew; *p != '\0'; p++ ) {
		if ( *p == '~' ) {
			write_to_buffer( d,
				" New password not acceptable, try again.\n\r Password: ", 0 );
			return;
		}
	}

	free(ch->pcdata->pwd);
	ch->pcdata->pwd = str_dup( pwdnew );

	write_to_buffer( d, " Please retype password: ", 0 );
	d->connected = CON_CONFIRM_NEW_PASSWORD;
}

static void nanny_confirm_new_password( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;

#if defined( unix )
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( strcmp( crypt( argument, ch->name ), ch->pcdata->pwd ) ) {
		write_to_buffer( d, " Passwords don't match.\n\r Retype password: ",
			0 );
		d->connected = CON_GET_NEW_PASSWORD;
		return;
	}

	write_to_buffer( d, echo_on_str, 0 );
	write_to_buffer( d, " What is your sex (M/F)? ", 0 );
	d->connected = CON_GET_NEW_SEX;
}

static void nanny_get_new_sex( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;

	switch ( argument[0] ) {
	case 'm':
	case 'M':
		ch->sex = SEX_MALE;
		break;
	case 'f':
	case 'F':
		ch->sex = SEX_FEMALE;
		break;
	default:
		write_to_buffer( d, " That's not a sex.\n\r What IS your sex? ", 0 );
		return;
	}
	write_to_buffer( d, echo_on_str, 0 );
	write_to_buffer( d, "\n\r What is your experience level?\n\r", 0 );
	write_to_buffer( d, "   [1] Never played a MUD before\n\r", 0 );
	write_to_buffer( d, "   [2] Played MUDs, but new to Dystopia\n\r", 0 );
	write_to_buffer( d, "   [3] Dystopia veteran\n\r", 0 );
	write_to_buffer( d, " Your choice (1/2/3)? ", 0 );
	d->connected = CON_GET_NEW_EXPLEVEL;
}

static void nanny_get_new_explevel( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;

	switch ( argument[0] ) {
	case '1':
		ch->pcdata->explevel = 0;
		break; /* Never MUD'd */
	case '2':
		ch->pcdata->explevel = 1;
		break; /* MUD experience */
	case '3':
		ch->pcdata->explevel = 2;
		break; /* Dystopia veteran */
	default:
		write_to_buffer( d, " Please choose 1, 2, or 3: ", 0 );
		return;
	}
	write_to_buffer( d, "\n\r Choose your display mode:\n\r", 0 );
	write_to_buffer( d, "   [N]one  - Plain text, no colors\n\r", 0 );
	write_to_buffer( d, "   [A]NSI  - Standard 16 colors (works with most clients)\n\r", 0 );
	write_to_buffer( d, "   [X]term - Full 256 colors (modern clients only)\n\r", 0 );
	write_to_buffer( d, "   [T]rue  - 24-bit RGB true color (Mudlet, TinTin++, etc.)\n\r", 0 );
	write_to_buffer( d, "   [S]creen reader - Optimized for JAWS, NVDA, VoiceOver\n\r", 0 );
	write_to_buffer( d, " Your choice (N/A/X/T/S)? ", 0 );
	d->connected = CON_GET_NEW_ANSI;
}

static void nanny_get_new_ansi( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;

	switch ( argument[0] ) {
	case 't':
	case 'T':
		SET_BIT( ch->act, PLR_ANSI );
		SET_BIT( ch->act, PLR_XTERM );
		SET_BIT( ch->extra, EXTRA_TRUECOLOR );
		break;
	case 'x':
	case 'X':
		SET_BIT( ch->act, PLR_ANSI );
		SET_BIT( ch->act, PLR_XTERM );
		break;
	case 'a':
	case 'A':
		SET_BIT( ch->act, PLR_ANSI );
		break;
	case 's':
	case 'S':
		SET_BIT( ch->act, PLR_SCREENREADER );
		SET_BIT( ch->act, PLR_BLANK );
		SET_BIT( ch->act, PLR_AUTOEXIT );
		d->mxp_enabled = FALSE;
		write_to_buffer( d, " Screen reader mode enabled.\n\r", 0 );
		break;
	case 'n':
	case 'N':
		break;
	default:
		write_to_buffer( d, " Please choose [N]one, [A]NSI, [X]term, [T]rue, or [S]creen reader: ", 0 );
		return;
	}
	/* Auto-detect GMCP/MXP/MCMP - enable silently if client negotiated them */
	if ( d->gmcp_enabled )
		SET_BIT( ch->act, PLR_PREFER_GMCP );
	if ( d->mxp_enabled && !IS_SET( ch->act, PLR_SCREENREADER ) )
		SET_BIT( ch->act, PLR_PREFER_MXP );
	if ( d->gmcp_enabled && ( d->gmcp_packages & GMCP_PACKAGE_CLIENT_MEDIA ) )
		SET_BIT( ch->act, PLR_PREFER_MCMP );
	/* MTTS auto-upgrade: apply detected terminal capabilities */
	ttype_apply_mtts( d, ch );
	/* Character creation finalization */
	ch->pcdata->perm_str = number_range( 10, 16 );
	ch->pcdata->perm_int = number_range( 10, 16 );
	ch->pcdata->perm_wis = number_range( 10, 16 );
	ch->pcdata->perm_dex = number_range( 10, 16 );
	ch->pcdata->perm_con = number_range( 10, 16 );
	ch->class = 0;
	set_learnable_disciplines( ch );
	sprintf( log_buf, "%s@%s new player.", ch->name, mask_ip( d->host ) );
	log_string( log_buf );
	write_to_buffer( d, "\n\r", 2 );
	do_help( ch, "motd" );
	d->connected = CON_READ_MOTD;
}

static void nanny_read_motd( DESCRIPTOR_DATA *d, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *ch = d->character;
	int i;

	list_push_back( &g_characters, &ch->char_node );
	d->connected = CON_PLAYING;

	/* Apply saved protocol preferences for returning players */
	if ( ch->level > 0 ) {
		/* GMCP preference handling */
		if ( IS_SET( ch->act, PLR_PREFER_GMCP ) && !d->gmcp_enabled )
			gmcp_init( d );
		else if ( !IS_SET( ch->act, PLR_PREFER_GMCP ) && d->gmcp_enabled )
			d->gmcp_enabled = FALSE;

		/* MXP preference handling */
		if ( IS_SET( ch->act, PLR_PREFER_MXP ) && !d->mxp_enabled )
			mxpStart( d );
		else if ( !IS_SET( ch->act, PLR_PREFER_MXP ) && d->mxp_enabled )
			d->mxp_enabled = FALSE;
	}

	/* MTTS auto-upgrade: apply detected terminal capabilities */
	ttype_apply_mtts( d, ch );

	if ( IS_CLASS( ch, CLASS_WEREWOLF ) ) {
		ch->gnosis[GCURRENT] = ch->gnosis[GMAXIMUM] - 5;
		if ( ch->gnosis[GCURRENT] < 0 ) ch->gnosis[GCURRENT] = 0;
	}

	if ( ch->mounted == IS_RIDING ) do_dismount( ch, "" );

	if ( !IS_SET( ch->extra, EXTRA_TRUSTED ) )
		SET_BIT( ch->extra, EXTRA_TRUSTED );

	if ( ch->level == 0 ) {
		ch->pcdata->awins = 0;	 /* arena wins           */
		ch->pcdata->alosses = 0; /* arena losses         */
		ch->pcdata->board = &boards[0];
		ch->gladiator = NULL; /* set player to bet on to NULL */
		ch->challenger = NULL;
		ch->challenged = NULL;
		ch->level = 1;
		ch->generation = 6;
		/* Default config options for new players */
		SET_BIT( ch->act, PLR_AUTOEXIT );
		SET_BIT( ch->act, PLR_AUTOLOOT );
		SET_BIT( ch->act, PLR_AUTOSAC );
		SET_BIT( ch->act, PLR_TELNET_GA );
		ch->stance[19] = -1;
		ch->stance[20] = -1;
		ch->stance[21] = -1;
		ch->stance[22] = -1;
		ch->stance[23] = -1;
		ch->exp = 0;
		ch->hit = ch->max_hit;
		ch->mana = ch->max_mana;
		ch->move = ch->max_move;
		ch->special = 0;
		set_switchname( ch, ch->name );
		ch->home = ROOM_VNUM_SCHOOL; /* Set recall point to school entrance */
		/* Starting room based on experience level */
		if ( ch->pcdata->explevel == 0 )
			char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL_SEC1 ) );
		else if ( ch->pcdata->explevel == 1 )
			char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL_SEC2 ) );
		else
			char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
		do_newbiepack( ch, "" );
		clear_stats( ch );

		/* Announce entry */
		players_logged++;
		sprintf( buf, "#7A #Rnew player#7 named #2%s #7enters #R%s.#n", ch->name, game_config.game_name );
		enter_info( buf );
		act( "$n has entered the game.", ch, NULL, NULL, TO_ROOM );

		/* Show board, then room */
		do_board( ch, "" );
		do_look( ch, "auto" );

		/* Silently equip newbie gear (shows effect messages in nice spot) */
		{
			OBJ_DATA *nobj, *nobj_next;
			bool equipped;
			/* Loop until nothing more can be equipped (handles dual slots like rings) */
			do {
				equipped = FALSE;
				LIST_FOR_EACH_SAFE( nobj, nobj_next, &ch->carrying, OBJ_DATA, content_node ) {
					if ( nobj->wear_loc == WEAR_NONE && can_see_obj( ch, nobj ) ) {
						/* Check finger slots */
						if ( CAN_WEAR( nobj, ITEM_WEAR_FINGER ) ) {
							if ( get_eq_char( ch, WEAR_FINGER_L ) == NULL ) {
								equip_char( ch, nobj, WEAR_FINGER_L );
								equipped = TRUE;
								continue;
							} else if ( get_eq_char( ch, WEAR_FINGER_R ) == NULL ) {
								equip_char( ch, nobj, WEAR_FINGER_R );
								equipped = TRUE;
								continue;
							}
						}
						/* Check neck slots */
						if ( CAN_WEAR( nobj, ITEM_WEAR_NECK ) ) {
							if ( get_eq_char( ch, WEAR_NECK_1 ) == NULL ) {
								equip_char( ch, nobj, WEAR_NECK_1 );
								equipped = TRUE;
								continue;
							} else if ( get_eq_char( ch, WEAR_NECK_2 ) == NULL ) {
								equip_char( ch, nobj, WEAR_NECK_2 );
								equipped = TRUE;
								continue;
							}
						}
						/* Check wrist slots */
						if ( CAN_WEAR( nobj, ITEM_WEAR_WRIST ) ) {
							if ( get_eq_char( ch, WEAR_WRIST_L ) == NULL ) {
								equip_char( ch, nobj, WEAR_WRIST_L );
								equipped = TRUE;
								continue;
							} else if ( get_eq_char( ch, WEAR_WRIST_R ) == NULL ) {
								equip_char( ch, nobj, WEAR_WRIST_R );
								equipped = TRUE;
								continue;
							}
						}
						/* Check hand slots (wield/hold) */
						if ( CAN_WEAR( nobj, ITEM_WIELD ) || CAN_WEAR( nobj, ITEM_HOLD ) ) {
							if ( get_eq_char( ch, WEAR_WIELD ) == NULL ) {
								equip_char( ch, nobj, WEAR_WIELD );
								equipped = TRUE;
								continue;
							} else if ( get_eq_char( ch, WEAR_HOLD ) == NULL ) {
								equip_char( ch, nobj, WEAR_HOLD );
								equipped = TRUE;
								continue;
							}
						}
						/* Single slots */
						if ( CAN_WEAR( nobj, ITEM_WEAR_HEAD ) && get_eq_char( ch, WEAR_HEAD ) == NULL ) {
							equip_char( ch, nobj, WEAR_HEAD );
							equipped = TRUE;
							continue;
						}
						if ( CAN_WEAR( nobj, ITEM_WEAR_BODY ) && get_eq_char( ch, WEAR_BODY ) == NULL ) {
							equip_char( ch, nobj, WEAR_BODY );
							equipped = TRUE;
							continue;
						}
						if ( CAN_WEAR( nobj, ITEM_WEAR_LEGS ) && get_eq_char( ch, WEAR_LEGS ) == NULL ) {
							equip_char( ch, nobj, WEAR_LEGS );
							equipped = TRUE;
							continue;
						}
						if ( CAN_WEAR( nobj, ITEM_WEAR_FEET ) && get_eq_char( ch, WEAR_FEET ) == NULL ) {
							equip_char( ch, nobj, WEAR_FEET );
							equipped = TRUE;
							continue;
						}
						if ( CAN_WEAR( nobj, ITEM_WEAR_HANDS ) && get_eq_char( ch, WEAR_HANDS ) == NULL ) {
							equip_char( ch, nobj, WEAR_HANDS );
							equipped = TRUE;
							continue;
						}
						if ( CAN_WEAR( nobj, ITEM_WEAR_ARMS ) && get_eq_char( ch, WEAR_ARMS ) == NULL ) {
							equip_char( ch, nobj, WEAR_ARMS );
							equipped = TRUE;
							continue;
						}
						if ( CAN_WEAR( nobj, ITEM_WEAR_ABOUT ) && get_eq_char( ch, WEAR_ABOUT ) == NULL ) {
							equip_char( ch, nobj, WEAR_ABOUT );
							equipped = TRUE;
							continue;
						}
						if ( CAN_WEAR( nobj, ITEM_WEAR_WAIST ) && get_eq_char( ch, WEAR_WAIST ) == NULL ) {
							equip_char( ch, nobj, WEAR_WAIST );
							equipped = TRUE;
							continue;
						}
						if ( nobj->item_type == ITEM_LIGHT && get_eq_char( ch, WEAR_LIGHT ) == NULL ) {
							equip_char( ch, nobj, WEAR_LIGHT );
							equipped = TRUE;
							continue;
						}
						if ( CAN_WEAR( nobj, ITEM_WEAR_FACE ) && get_eq_char( ch, WEAR_FACE ) == NULL ) {
							equip_char( ch, nobj, WEAR_FACE );
							equipped = TRUE;
							continue;
						}
					}
				}
			} while ( equipped );
		}

		/* Room program greeting */
		room_text( ch, ">ENTER<" );

		/* Experience-appropriate welcome hint */
		if ( ch->pcdata->explevel == 0 )
			send_to_char( "\n\r#GTip:#n Type '#Rhelp tutorial#n' for a beginner's guide!\n\r", ch );
		else if ( ch->pcdata->explevel == 1 )
			send_to_char( "\n\r#GTip:#n Type '#Rhelp dystopiaplus#n' for Dystopia-specific features!\n\r", ch );

		/* Send initial GMCP data */
		gmcp_send_char_data( ch );
		mcmp_ui_event( ch, "login" );
		return;
	} else if ( !IS_NPC( ch ) && ch->pcdata->obj_vnum != 0 ) {
		if ( ch->in_room != NULL )
			char_to_room( ch, ch->in_room );
		else
			char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
		bind_char( ch );
		return;
	} else if ( ch->in_room != NULL ) {
		char_to_room( ch, ch->in_room );
		do_look( ch, "auto" );
	} else if ( IS_IMMORTAL( ch ) ) {
		char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
		do_look( ch, "auto" );
	} else {
		char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
		do_look( ch, "auto" );
	}
	do_board( ch, "" );

	/* Send initial GMCP data now that character is fully loaded */
	gmcp_send_char_data( ch );

	ch->fight_timer = 0;
	ch->pcdata->revision = CURRENT_REVISION;
	act( "$n has entered the game.", ch, NULL, NULL, TO_ROOM );
	clear_stats( ch );

	remove_illegal_items( ch, TRUE );
	room_text( ch, ">ENTER<" );

	for ( i = 0; i < MAX_DISCIPLINES; i++ ) {
		if ( ch->power[i] > 10 )
			ch->power[i] = 10;
	}

	clear_foreign_disciplines( ch );

	ch->embraced = NULL;
	ch->embracing = NULL;
	multicheck( ch );
	/* Send initial GMCP data if enabled */
	if ( d->gmcp_enabled )
		gmcp_send_char_data( ch );
	mcmp_ui_event( ch, "login" );
}

/*
 * Deal with sockets that haven't logged in yet.
 * Dispatches to per-state handler functions above.
 */
void nanny( DESCRIPTOR_DATA *d, char *argument ) {

	if ( d->connected != CON_NOTE_TEXT )
		while ( isspace( *argument ) )
			argument++;

	switch ( d->connected ) {

	default:
		bug( "Nanny: bad d->connected %d.", d->connected );
		close_socket( d );
		return;

	case CON_NOT_PLAYING:
		break;

	case CON_DETECT_CAPS:
		break; /* Handled in game_loop() via intro_check_ready(), not here */

	case CON_GET_NAME:             nanny_get_name( d, argument );             break;
	case CON_GET_OLD_PASSWORD:     nanny_get_old_password( d, argument );     break;
	case CON_CONFIRM_NEW_NAME:     nanny_confirm_new_name( d, argument );     break;
	case CON_GET_NEW_PASSWORD:     nanny_get_new_password( d, argument );     break;
	case CON_CONFIRM_NEW_PASSWORD: nanny_confirm_new_password( d, argument ); break;
	case CON_GET_NEW_SEX:          nanny_get_new_sex( d, argument );          break;
	case CON_GET_NEW_EXPLEVEL:     nanny_get_new_explevel( d, argument );     break;
	case CON_GET_NEW_ANSI:         nanny_get_new_ansi( d, argument );         break;
	case CON_READ_MOTD:            nanny_read_motd( d, argument );            break;

	/* Note system states, (c)1995-96 erwin@pip.dknet.dk */
	case CON_NOTE_TO:              handle_con_note_to( d, argument );         break;
	case CON_NOTE_SUBJECT:         handle_con_note_subject( d, argument );    break;
	case CON_NOTE_EXPIRE:          handle_con_note_expire( d, argument );     break;
	case CON_NOTE_TEXT:             handle_con_note_text( d, argument );       break;
	case CON_NOTE_FINISH:          handle_con_note_finish( d, argument );     break;
	}
}

/*
 * Parse a name for acceptability.
 *
 * Validation flow (database-driven with UTF-8 confusable detection):
 *   1. UTF-8 validity
 *   2. Length: 3-12 codepoints, max 48 bytes
 *   3. Character validation: each codepoint must pass utf8_is_name_char()
 *   4. Mixed-script rejection: all chars must be from the same script group
 *   5. Forbidden names: reserved words, protected prefixes, exact blocks
 *   6. Profanity: skeletonize name, check against profanity_filters
 *   7. I/L twit check: only when all characters are ASCII
 *   8. Mob name collision: unchanged
 */
bool check_parse_name( char *name ) {
	int byte_len, cp_len;
	int primary_script = 0;
	bool all_ascii = TRUE;

	if ( name == NULL || name[0] == '\0' )
		return FALSE;

	byte_len = (int) strlen( name );

	/*
	 * 1. UTF-8 validity.
	 */
	if ( !utf8_is_valid( name, byte_len ) )
		return FALSE;

	/*
	 * 2. Length restrictions: 3-12 codepoints, max 48 bytes.
	 */
	if ( byte_len > 48 )
		return FALSE;

	cp_len = utf8_strlen( name );
	if ( cp_len < 3 || cp_len > 12 )
		return FALSE;

	/*
	 * 3. Character validation + 4. Mixed-script detection.
	 * Walk every codepoint: must be a valid name character.
	 * Determine primary script from first non-ASCII character.
	 * Reject if a different script appears later.
	 */
	{
		const char *p = name;

		while ( *p != '\0' ) {
			unsigned int cp = utf8_decode( &p );
			int script;

			if ( !utf8_is_name_char( cp ) )
				return FALSE;

			if ( cp > 0x7F )
				all_ascii = FALSE;

			script = utf8_char_script( cp );

			/* Only check mixing for non-Latin scripts vs Latin, or between
			 * non-Latin scripts. Latin (script 1) is always allowed as it
			 * serves as the base script. */
			if ( script > 1 ) {
				if ( primary_script == 0 )
					primary_script = script;
				else if ( script != primary_script )
					return FALSE;
			}
		}
	}

	/*
	 * 5. Forbidden names (database-driven).
	 */
	{
		FORBIDDEN_NAME *fn;

		for ( fn = forbidden_name_list; fn; fn = fn->next ) {
			switch ( fn->type ) {
			case NAMETYPE_RESERVED:
			case NAMETYPE_BLOCKED:
				/* Exact match (case-insensitive) */
				if ( !str_cmp( name, fn->name ) )
					return FALSE;
				break;
			case NAMETYPE_PROTECTED:
				/* Contains but not exact match */
				if ( is_contained( fn->name, name )
					&& str_cmp( name, fn->name ) )
					return FALSE;
				break;
			}
		}
	}

	/*
	 * 6. Profanity check with confusable-character detection.
	 * Skeletonize the name (Unicode lookalikes -> ASCII canonical)
	 * then check against profanity filter patterns.
	 */
	{
		char skeleton[256];
		PROFANITY_FILTER *pf;

		utf8_skeletonize( name, skeleton, sizeof( skeleton ) );

		for ( pf = profanity_filter_list; pf; pf = pf->next ) {
			if ( is_contained( pf->pattern, skeleton ) )
				return FALSE;
		}
	}

	/*
	 * 7. Lock out IllIll twits (ASCII names only).
	 */
	if ( all_ascii ) {
		char *pc;
		bool fIll = TRUE;

		for ( pc = name; *pc != '\0'; pc++ ) {
			if ( LOWER( *pc ) != 'i' && LOWER( *pc ) != 'l' )
				fIll = FALSE;
		}

		if ( fIll )
			return FALSE;
	}

	/* Special override */
	if ( !str_cmp( name, "kip" ) ) return TRUE;

	/*
	 * 8. Prevent players from naming themselves after mobs.
	 */
	{
		extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
		MOB_INDEX_DATA *pMobIndex;
		int iHash;

		for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
			for ( pMobIndex = mob_index_hash[iHash];
				pMobIndex != NULL;
				pMobIndex = pMobIndex->next ) {
				if ( is_name( name, pMobIndex->player_name ) )
					return FALSE;
			}
		}
	}

	return TRUE;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn ) {
	CHAR_DATA *ch;

	LIST_FOR_EACH( ch, &g_characters, CHAR_DATA, char_node ) {
		if ( !IS_NPC( ch ) && !IS_EXTRA( ch, EXTRA_SWITCH ) && ( !fConn || ch->desc == NULL ) && !str_cmp( GET_PC_NAME( d->character ), GET_PC_NAME( ch ) ) ) {
			if ( fConn == FALSE ) {
				free(d->character->pcdata->pwd);
				d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
			} else {
				free_char( d->character );
				d->character = ch;
				ch->desc = d;
				ch->timer = 0;
				send_to_char( "Reconnecting.\n\r", ch );
				if ( IS_NPC( ch ) || ch->pcdata->obj_vnum == 0 )
					act( "$n has reconnected.", ch, NULL, NULL, TO_ROOM );
				sprintf( log_buf, "%s@%s reconnected.", ch->name, ch->lasthost );
				log_string( log_buf );
				d->connected = CON_PLAYING;
				/* Send GMCP data on reconnect */
				if ( d->gmcp_enabled )
					gmcp_send_char_data( ch );
				/* MTTS auto-upgrade on reconnect */
				ttype_apply_mtts( d, ch );
				/* Inform the character of a note in progress and the possbility of continuation! */
				if ( ch->pcdata->in_progress )
					send_to_char( "You have a note in progress. Type NWRITE to continue it.\n\r", ch );
			}
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * Kick off old connection.  KaVir.
 */
bool check_kickoff( DESCRIPTOR_DATA *d, char *name, bool fConn ) {
	CHAR_DATA *ch;

	LIST_FOR_EACH( ch, &g_characters, CHAR_DATA, char_node ) {
		if ( !IS_NPC( ch ) && ( !fConn || ch->desc == NULL ) && !str_cmp( GET_PC_NAME( d->character ), GET_PC_NAME( ch ) ) ) {
			if ( fConn == FALSE ) {
				free(d->character->pcdata->pwd);
				d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
			} else {
				free_char( d->character );
				d->character = ch;
				ch->desc = d;
				ch->timer = 0;
				send_to_char( "You take over your body, which was already in use.\n\r", ch );
				act( "...$n's body has been taken over by another spirit!", ch, NULL, NULL, TO_ROOM );
				sprintf( log_buf, "%s@%s kicking off old link.", ch->name, ch->lasthost );
				log_string( log_buf );
				d->connected = CON_PLAYING;
				/* MTTS auto-upgrade on kickoff */
				ttype_apply_mtts( d, ch );
			}
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * Check if someone is editing this player via pedit.
 * If so, save their work and close the edit session so the player can log in.
 */
void check_pedit_editing( const char *name ) {
	CHAR_DATA *ch;
	char buf[MAX_STRING_LENGTH];

	LIST_FOR_EACH( ch, &g_characters, CHAR_DATA, char_node ) {
		if ( IS_NPC( ch ) || ch->pcdata == NULL )
			continue;
		if ( ch->pcdata->pfile == NULL )
			continue;
		if ( str_cmp( name, ch->pcdata->pfile->name ) )
			continue;

		/* Found an immortal editing this player - save and close */
		sprintf( buf, "%s is logging in - saving and closing your edit session.\n\r", name );
		send_to_char( buf, ch );
		pedit_save_offline( ch );
		if ( ch->desc && ch->desc->connected == CON_PFILE )
			ch->desc->connected = CON_PLAYING;
	}
}

/*
 * Check if already playing - KaVir.
 * Using kickoff code from Malice, as mine is v. dodgy.
 */
bool check_playing( DESCRIPTOR_DATA *d, char *name ) {
	DESCRIPTOR_DATA *dold;

	LIST_FOR_EACH( dold, &g_descriptors, DESCRIPTOR_DATA, node ) {
		if ( dold != d && dold->character != NULL && dold->connected != CON_GET_NAME && dold->connected != CON_GET_OLD_PASSWORD && !str_cmp( name, dold->original ? GET_PC_NAME( dold->original ) : GET_PC_NAME( dold->character ) ) ) {
			char buf[MAX_STRING_LENGTH];
			if ( d->character != NULL ) {
				free_char( d->character );
				d->character = NULL;
			}
			send_to_char( "This body has been taken over!\n\r", dold->character );
			d->connected = CON_PLAYING;
			d->character = dold->character;
			d->character->desc = d;
			send_to_char( "You take over your body, which was already in use.\n\r", d->character );
			act( "$n doubles over in agony and $s eyes roll up into $s head.", d->character, NULL, NULL, TO_ROOM );
			act( "...$n's body has been taken over by another spirit!", d->character, NULL, NULL, TO_ROOM );
			dold->character = NULL;

			sprintf( buf, "Kicking off old connection %s@%s", d->character->name, d->host );
			log_string( buf );
			close_socket( dold ); /*Slam the old connection into the ether*/
			return TRUE;
		}
	}

	return FALSE;
}

void stop_idling( CHAR_DATA *ch ) {
	if ( ch == NULL || ch->desc == NULL || ( ch->desc->connected != CON_PLAYING && ch->desc->connected != CON_EDITING ) || ch->was_in_room == NULL || ch->in_room != get_room_index( ROOM_VNUM_LIMBO ) )
		return;

	ch->timer = 0;
	char_from_room( ch );
	char_to_room( ch, ch->was_in_room );
	ch->was_in_room = NULL;
	act( "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
	return;
}
