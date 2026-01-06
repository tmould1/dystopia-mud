#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

GAMECONFIG_DATA game_config;

void load_gameconfig()
{
	FILE *fp;
	char line[MSL];

	game_config.base_xp = 5000;
	game_config.max_xp_per_kill = 1000000000;
	game_config.game_name = str_dup("Dystopia+ Spin-Off");
	game_config.gui_url = str_dup("");
	game_config.gui_version = str_dup("1");
	game_config.banner_left = str_dup("#0<>#n");
	game_config.banner_right = str_dup("#0<>#n");
	game_config.banner_fill = str_dup("#0==#n");

	if ((fp = fopen(mud_path(mud_txt_dir, "gameconfig.txt"), "r")) == NULL)
	{
		// No gameconfig.txt found, create one with defaults.
		save_gameconfig();
		return;
	}

	while (fgets(line, MSL, fp) != NULL)
	{
		if (line[0] == '\0')
			continue;

		// Format will be <variable>: <value>
		char *variable = strtok(line, ":");
		char *value = strtok(NULL, ":");

		// The value could have spaces in front of it, so, we need to remove them.
		while (value[0] == ' ')
		{
			value++;
		}
		
		// If we are processing a string, we need to remove any newline and/or carriage return characters at the end.
		if (value != NULL)
		{
			value[strcspn(value, "\r\n")] = 0;
		}
		
		if (variable == NULL || value == NULL)
		{
			log_string("Error reading gameconfig.txt");
			continue;
		}

		if (!str_cmp(variable, "base_xp"))
		{
			game_config.base_xp = atoi(value);
		}
		else if (!str_cmp(variable, "max_xp_per_kill"))
		{
			game_config.max_xp_per_kill = atoi(value);
		}
		else if (!str_cmp(variable, "game_name"))
		{
			game_config.game_name = str_dup(value);
		}
		else if (!str_cmp(variable, "gui_url"))
		{
			free_string(game_config.gui_url);
			game_config.gui_url = str_dup(value);
		}
		else if (!str_cmp(variable, "gui_version"))
		{
			free_string(game_config.gui_version);
			game_config.gui_version = str_dup(value);
		}
		else if (!str_cmp(variable, "banner_left"))
		{
			free_string(game_config.banner_left);
			game_config.banner_left = str_dup(value);
		}
		else if (!str_cmp(variable, "banner_right"))
		{
			free_string(game_config.banner_right);
			game_config.banner_right = str_dup(value);
		}
		else if (!str_cmp(variable, "banner_fill"))
		{
			free_string(game_config.banner_fill);
			game_config.banner_fill = str_dup(value);
		}
		else
		{
			log_string("Unknown variable in gameconfig.txt");
		}
	}
	fclose(fp);
}

void save_gameconfig()
{
	FILE *fp;

	if ((fp = fopen(mud_path(mud_txt_dir, "gameconfig.txt"), "w")) == NULL)
	{
		log_string("Error writing to gameconfig.txt");
		return;
	}

	fprintf(fp, "base_xp: %d\n", game_config.base_xp);
	fprintf(fp, "max_xp_per_kill: %d\n", game_config.max_xp_per_kill);
	fprintf(fp, "game_name: %s\n", game_config.game_name);
	fprintf(fp, "gui_url: %s\n", game_config.gui_url);
	fprintf(fp, "gui_version: %s\n", game_config.gui_version);
	fprintf(fp, "banner_left: %s\n", game_config.banner_left);
	fprintf(fp, "banner_right: %s\n", game_config.banner_right);
	fprintf(fp, "banner_fill: %s\n", game_config.banner_fill);

	fclose(fp);
}

void do_gameconfig(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);	// Arg1 is a switch
	one_argument_case(argument, arg2);		// Arg2 is the value, respect case

	if (arg[0] == '\0')
	{
		char buf[MAX_STRING_LENGTH];

		send_to_char("Syntax: gameconfig <field> <value>\n\r", ch);
		send_to_char("Field - Description - Current Value:\n\r", ch);

		sprintf(buf, "base_xp - Base XP: %d\n\r", game_config.base_xp);
		send_to_char(buf, ch);
		sprintf(buf, "max_xp_per_kill - Max XP per kill: %d\n\r", game_config.max_xp_per_kill);
		send_to_char(buf, ch);
		sprintf(buf, "game_name - Game Name: \"%s\"\n\r", game_config.game_name);
		send_to_char(buf, ch);
		sprintf(buf, "gui_url - Mudlet GUI Package URL: \"%s\"\n\r", game_config.gui_url);
		send_to_char(buf, ch);
		sprintf(buf, "gui_version - Mudlet GUI Package Version: \"%s\"\n\r", game_config.gui_version);
		send_to_char(buf, ch);
		sprintf(buf, "banner_left - Who Banner Left Endcap: \"%s\"\n\r", game_config.banner_left);
		send_to_char(buf, ch);
		sprintf(buf, "banner_right - Who Banner Right Endcap: \"%s\"\n\r", game_config.banner_right);
		send_to_char(buf, ch);
		sprintf(buf, "banner_fill - Who Banner Fill: \"%s\"\n\r", game_config.banner_fill);
		send_to_char(buf, ch);
		return;
	}

	char modified_field[MAX_INPUT_LENGTH];
	char old_value[MAX_INPUT_LENGTH];
	char new_value[MAX_INPUT_LENGTH];
	// Reading base_xp
	if (!str_cmp(arg, "base_xp"))
	{
		// if arg2 is blank, report the value
		if (arg2[0] == '\0')
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "Base XP: %d\n\r", game_config.base_xp);
			send_to_char(buf, ch);
			return;
		}

		sprintf(modified_field, "Base XP");
		sprintf(old_value, "%d", game_config.base_xp);
		sprintf(new_value, arg2);
		game_config.base_xp = atoi(new_value);
	}
	else if (!str_cmp(arg, "max_xp_per_kill"))
	{
		// if arg2 is blank, report the value
		if (arg2[0] == '\0')
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "Max XP per kill: %d\n\r", game_config.max_xp_per_kill);
			send_to_char(buf, ch);
			return;
		}

		sprintf(modified_field, "Max XP per kill");
		sprintf(old_value, "%d", game_config.max_xp_per_kill);
		sprintf(new_value, arg2);
		game_config.max_xp_per_kill = atoi(new_value);
	}
	else if (!str_cmp(arg, "game_name"))
	{
		// if arg2 is blank, report the value
		if (arg2[0] == '\0')
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "Game Name: \"%s\"\n\r", game_config.game_name);
			send_to_char(buf, ch);
			return;
		}

		sprintf(modified_field, "Game Name");
		sprintf(old_value, "%s", game_config.game_name);
		sprintf(new_value, arg2);
		free_string(game_config.game_name);
		game_config.game_name = str_dup(new_value);
	}
	else if (!str_cmp(arg, "gui_url"))
	{
		// if arg2 is blank, report the value
		if (arg2[0] == '\0')
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "GUI URL: \"%s\"\n\r", game_config.gui_url);
			send_to_char(buf, ch);
			return;
		}

		sprintf(modified_field, "GUI URL");
		sprintf(old_value, "%s", game_config.gui_url);
		sprintf(new_value, arg2);
		free_string(game_config.gui_url);
		game_config.gui_url = str_dup(new_value);
	}
	else if (!str_cmp(arg, "gui_version"))
	{
		// if arg2 is blank, report the value
		if (arg2[0] == '\0')
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "GUI Version: \"%s\"\n\r", game_config.gui_version);
			send_to_char(buf, ch);
			return;
		}

		sprintf(modified_field, "GUI Version");
		sprintf(old_value, "%s", game_config.gui_version);
		sprintf(new_value, arg2);
		free_string(game_config.gui_version);
		game_config.gui_version = str_dup(new_value);
	}
	else if (!str_cmp(arg, "banner_left"))
	{
		// if arg2 is blank, report the value
		if (arg2[0] == '\0')
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "Banner Left Endcap: \"%s\"\n\r", game_config.banner_left);
			send_to_char(buf, ch);
			return;
		}

		sprintf(modified_field, "Banner Left Endcap");
		sprintf(old_value, "%s", game_config.banner_left);
		sprintf(new_value, arg2);
		free_string(game_config.banner_left);
		game_config.banner_left = str_dup(new_value);
	}
	else if (!str_cmp(arg, "banner_right"))
	{
		// if arg2 is blank, report the value
		if (arg2[0] == '\0')
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "Banner Right Endcap: \"%s\"\n\r", game_config.banner_right);
			send_to_char(buf, ch);
			return;
		}

		sprintf(modified_field, "Banner Right Endcap");
		sprintf(old_value, "%s", game_config.banner_right);
		sprintf(new_value, arg2);
		free_string(game_config.banner_right);
		game_config.banner_right = str_dup(new_value);
	}
	else if (!str_cmp(arg, "banner_fill"))
	{
		// if arg2 is blank, report the value
		if (arg2[0] == '\0')
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "Banner Fill: \"%s\"\n\r", game_config.banner_fill);
			send_to_char(buf, ch);
			return;
		}

		sprintf(modified_field, "Banner Fill");
		sprintf(old_value, "%s", game_config.banner_fill);
		sprintf(new_value, arg2);
		free_string(game_config.banner_fill);
		game_config.banner_fill = str_dup(new_value);
	}
	// arg1 not recognized, show prompt
	else
	{
		stc("Invalid field option.", ch);
		do_gameconfig(ch, "");
		return;
	}

	// Final reporting
	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "Done!  %s changed from %s to %s\n\r", modified_field, old_value, new_value);
	send_to_char(buf, ch);
	save_gameconfig();
	return;
}