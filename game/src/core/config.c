#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "../db/db_game.h"
#include "../systems/mcmp.h"

GAMECONFIG_DATA game_config;

void load_gameconfig() {
	/* Set defaults first */
	game_config.base_xp = 5000;
	game_config.max_xp_per_kill = 1000000000;
	game_config.game_name = str_dup( "Dystopia+ Spin-Off" );
	game_config.gui_url = str_dup( "" );
	game_config.gui_version = str_dup( "1" );
	game_config.banner_left = str_dup( "#0<>#n" );
	game_config.banner_right = str_dup( "#0<>#n" );
	game_config.banner_fill = str_dup( "#0==#n" );
	game_config.audio_url = str_dup( MCMP_DEFAULT_URL );

	/* Load from game.db (overwrites defaults for any keys present) */
	db_game_load_gameconfig();

	/* Load audio config from game.db */
	db_game_load_audio_config();

	/* Load immortal pretitles from game.db */
	db_game_load_pretitles();
}

void save_gameconfig() {
	db_game_save_gameconfig();
}

void do_gameconfig( CHAR_DATA *ch, char *argument ) {
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument( argument, arg ); // Arg1 is a switch
	one_argument_case( argument, arg2 );	  // Arg2 is the value, respect case

	if ( arg[0] == '\0' ) {
		char buf[MAX_STRING_LENGTH];

		send_to_char( "Syntax: gameconfig <field> <value>\n\r\n\r", ch );

		snprintf( buf, sizeof( buf ), "  %-16s %d\n\r", "base_xp", game_config.base_xp );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  %-16s %d\n\r", "max_xp_per_kill", game_config.max_xp_per_kill );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  %-16s \"%s\"\n\r", "game_name", game_config.game_name );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  %-16s \"%s\"\n\r", "gui_url", game_config.gui_url );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  %-16s \"%s\"\n\r", "gui_version", game_config.gui_version );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  %-16s \"%s\"\n\r", "banner_left", game_config.banner_left );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  %-16s \"%s\"\n\r", "banner_right", game_config.banner_right );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  %-16s \"%s\"\n\r", "banner_fill", game_config.banner_fill );
		send_to_char( buf, ch );
		snprintf( buf, sizeof( buf ), "  %-16s \"%s\"\n\r", "audio_url", game_config.audio_url );
		send_to_char( buf, ch );
		return;
	}

	char modified_field[MAX_INPUT_LENGTH];
	char old_value[MAX_INPUT_LENGTH];
	char new_value[MAX_INPUT_LENGTH];
	// Reading base_xp
	if ( !str_cmp( arg, "base_xp" ) ) {
		// if arg2 is blank, report the value
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "Base XP: %d\n\r", game_config.base_xp );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "Base XP" );
		snprintf( old_value, sizeof( old_value ), "%d", game_config.base_xp );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		game_config.base_xp = atoi( new_value );
	} else if ( !str_cmp( arg, "max_xp_per_kill" ) ) {
		// if arg2 is blank, report the value
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "Max XP per kill: %d\n\r", game_config.max_xp_per_kill );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "Max XP per kill" );
		snprintf( old_value, sizeof( old_value ), "%d", game_config.max_xp_per_kill );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		game_config.max_xp_per_kill = atoi( new_value );
	} else if ( !str_cmp( arg, "game_name" ) ) {
		// if arg2 is blank, report the value
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "Game Name: \"%s\"\n\r", game_config.game_name );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "Game Name" );
		snprintf( old_value, sizeof( old_value ), "%s", game_config.game_name );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		free(game_config.game_name);
		game_config.game_name = str_dup( new_value );
	} else if ( !str_cmp( arg, "gui_url" ) ) {
		// if arg2 is blank, report the value
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "GUI URL: \"%s\"\n\r", game_config.gui_url );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "GUI URL" );
		snprintf( old_value, sizeof( old_value ), "%s", game_config.gui_url );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		free(game_config.gui_url);
		game_config.gui_url = str_dup( new_value );
	} else if ( !str_cmp( arg, "gui_version" ) ) {
		// if arg2 is blank, report the value
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "GUI Version: \"%s\"\n\r", game_config.gui_version );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "GUI Version" );
		snprintf( old_value, sizeof( old_value ), "%s", game_config.gui_version );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		free(game_config.gui_version);
		game_config.gui_version = str_dup( new_value );
	} else if ( !str_cmp( arg, "banner_left" ) ) {
		// if arg2 is blank, report the value
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "Banner Left Endcap: \"%s\"\n\r", game_config.banner_left );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "Banner Left Endcap" );
		snprintf( old_value, sizeof( old_value ), "%s", game_config.banner_left );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		free(game_config.banner_left);
		game_config.banner_left = str_dup( new_value );
	} else if ( !str_cmp( arg, "banner_right" ) ) {
		// if arg2 is blank, report the value
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "Banner Right Endcap: \"%s\"\n\r", game_config.banner_right );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "Banner Right Endcap" );
		snprintf( old_value, sizeof( old_value ), "%s", game_config.banner_right );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		free(game_config.banner_right);
		game_config.banner_right = str_dup( new_value );
	} else if ( !str_cmp( arg, "banner_fill" ) ) {
		// if arg2 is blank, report the value
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "Banner Fill: \"%s\"\n\r", game_config.banner_fill );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "Banner Fill" );
		snprintf( old_value, sizeof( old_value ), "%s", game_config.banner_fill );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		free(game_config.banner_fill);
		game_config.banner_fill = str_dup( new_value );
	} else if ( !str_cmp( arg, "audio_url" ) ) {
		if ( arg2[0] == '\0' ) {
			char buf[MAX_STRING_LENGTH];
			snprintf( buf, sizeof( buf ), "MCMP Audio Base URL: \"%s\"\n\r", game_config.audio_url );
			send_to_char( buf, ch );
			return;
		}

		snprintf( modified_field, sizeof( modified_field ), "MCMP Audio Base URL" );
		snprintf( old_value, sizeof( old_value ), "%s", game_config.audio_url );
		snprintf( new_value, sizeof( new_value ), "%s", arg2 );
		free(game_config.audio_url);
		game_config.audio_url = str_dup( new_value );
	}
	// arg1 not recognized, show prompt
	else {
		stc( "Invalid field option.", ch );
		do_gameconfig( ch, "" );
		return;
	}

	// Final reporting
	char buf[MAX_STRING_LENGTH];
	snprintf( buf, sizeof( buf ), "Done!  %s changed from %s to %s\n\r", modified_field, old_value, new_value );
	send_to_char( buf, ch );
	save_gameconfig();
	return;
}