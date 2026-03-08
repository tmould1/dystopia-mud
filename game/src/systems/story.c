/*
 * story.c — "Echoes of the Sundering" story quest system.
 *
 * A hub-and-spoke narrative quest separate from the main quest system.
 * Players follow NPC questgivers through 16 areas, completing local
 * tasks (kill, fetch, examine) at each hub before being sent onward.
 *
 * Progress is tracked via:
 *   pcdata->story_node      — current hub (0 = not started, 1-16 = active)
 *   pcdata->story_kills     — kill counter for current hub's objective
 *   pcdata->story_progress  — bitfield: which sub-tasks are complete
 *
 * Clue text is looked up from the central story_clues table in quest.db,
 * keyed by (story_node, stage).  stage = 0 (travel) when progress == 0,
 * stage = 1 (tasks) when progress > 0.
 *
 * NPC dialogue and task completion are handled by Lua scripts.
 */

#include "../core/merc.h"
#include "../db/db_quest.h"

/* Total number of story hubs */
#define STORY_NODE_MAX 16

/*
 * do_story — recall story quest status and sub-task progress.
 *
 * Syntax: story
 */
void do_story( CHAR_DATA *ch, char *argument ) {
	if ( IS_NPC( ch ) )
		return;

	if ( ch->pcdata->story_node == 0 ) {
		send_to_char( "You have no recollection of any particular quest for knowledge.\n\r", ch );
		return;
	}

	if ( ch->pcdata->story_node > STORY_NODE_MAX ) {
		send_to_char( "#CYou reflect on your journey through the shattered world. "
			"The Sundering broke it — or made it. The Unraveling continues — "
			"or always has. But the stories remain. The worlds remain. "
			"And as long as someone walks through them and listens, "
			"they are not forgotten.#n\n\r", ch );
		return;
	}

	{
		int stage = ( ch->pcdata->story_progress > 0 ) ? 1 : 0;
		const char *clue = story_clue_lookup( ch->pcdata->story_node, stage );

		if ( clue[0] != '\0' ) {
			send_to_char( clue, ch );
			send_to_char( "\n\r", ch );
		} else {
			send_to_char( "A whisper echoes in your mind: #C\"Seek the one who stands "
				"in judgment at the Temple of Midgaard. Ask about the darkness "
				"that spreads through these lands.\"#n\n\r", ch );
		}
	}

	/* Show sub-task progress if any tasks are active */
	if ( ch->pcdata->story_progress != 0 || ch->pcdata->story_kills > 0 ) {
		char buf[MAX_STRING_LENGTH];
		snprintf( buf, sizeof( buf ),
			"#w[Tasks: %s%s%s | Kills: %d]#n\n\r",
			( ch->pcdata->story_progress & 0x01 ) ? "#G*#w" : "#R-#w",
			( ch->pcdata->story_progress & 0x02 ) ? "#G*#w" : "#R-#w",
			( ch->pcdata->story_progress & 0x04 ) ? "#G*#w" : "#R-#w",
			ch->pcdata->story_kills );
		send_to_char( buf, ch );
	}
}


/*
 * do_storyadmin — immortal command to inspect and modify story quest state.
 *
 * Syntax:
 *   storyadmin                           Show usage
 *   storyadmin <player> [info]           Show story state
 *   storyadmin <player> set node <n>     Set story node (0-17)
 *   storyadmin <player> set kills <n>    Set kill counter
 *   storyadmin <player> set progress <n> Set progress bitfield
 *   storyadmin <player> reset            Reset all story fields
 *   storyadmin <player> advance          Move to next hub
 */
void do_storyadmin( CHAR_DATA *ch, char *argument ) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument( argument, arg1 );

	if ( arg1[0] == '\0' ) {
		send_to_char( "\n\r #tFFD700\xe2\x9c\xa6 Story Quest Administration \xe2\x9c\xa6#n\n\r\n\r", ch );
		send_to_char( "  #Cstoryadmin#n <player> [#Cinfo#n]              Show story state\n\r", ch );
		send_to_char( "  #Cstoryadmin#n <player> #Cset node#n <0-17>     Set story node\n\r", ch );
		send_to_char( "  #Cstoryadmin#n <player> #Cset kills#n <n>       Set kill counter\n\r", ch );
		send_to_char( "  #Cstoryadmin#n <player> #Cset progress#n <n>    Set progress bitfield\n\r", ch );
		send_to_char( "  #Cstoryadmin#n <player> #Creset#n               Reset all story fields\n\r", ch );
		send_to_char( "  #Cstoryadmin#n <player> #Cadvance#n             Move to next hub\n\r", ch );
		send_to_char( "\n\r  Nodes: 0=not started, 1-16=hub, 17+=completed\n\r\n\r", ch );
		return;
	}

	victim = get_char_world( ch, arg1 );
	if ( !victim ) {
		send_to_char( "Player not found.\n\r", ch );
		return;
	}
	if ( IS_NPC( victim ) ) {
		send_to_char( "Not on NPCs.\n\r", ch );
		return;
	}

	argument = one_argument( argument, arg2 );

	/*
	 * Info (default when no subcommand given)
	 */
	if ( arg2[0] == '\0' || !str_cmp( arg2, "info" ) ) {
		const char *node_label;
		int stage;
		const char *clue;

		if ( victim->pcdata->story_node == 0 )
			node_label = "not started";
		else if ( victim->pcdata->story_node > STORY_NODE_MAX )
			node_label = "completed";
		else
			node_label = "active";

		stage = ( victim->pcdata->story_progress > 0 ) ? 1 : 0;
		clue = story_clue_lookup( victim->pcdata->story_node, stage );

		snprintf( buf, sizeof( buf ),
			"\n\r #tFFD700\xe2\x9c\xa6 Story State: #C%s#n\n\r\n\r"
			"  Node:     #C%d#n (%s)\n\r"
			"  Kills:    #C%d#n\n\r"
			"  Progress: #C0x%02X#n  [%s%s%s]\n\r"
			"  Clue:     (node %d, stage %d) %s\n\r\n\r",
			victim->name,
			victim->pcdata->story_node, node_label,
			victim->pcdata->story_kills,
			victim->pcdata->story_progress,
			( victim->pcdata->story_progress & 0x01 ) ? "#G*#w" : "#R-#w",
			( victim->pcdata->story_progress & 0x02 ) ? "#G*#w" : "#R-#w",
			( victim->pcdata->story_progress & 0x04 ) ? "#G*#w" : "#R-#w",
			victim->pcdata->story_node, stage,
			clue[0] ? clue : "#x243(none)#n" );
		send_to_char( buf, ch );
		return;
	}

	/*
	 * Set <field> <value>
	 */
	if ( !str_cmp( arg2, "set" ) ) {
		char arg3[MAX_INPUT_LENGTH];
		argument = one_argument( argument, arg3 );

		if ( arg3[0] == '\0' ) {
			send_to_char( "Set what? (node, kills, progress)\n\r", ch );
			return;
		}

		if ( !str_cmp( arg3, "node" ) ) {
			char arg4[MAX_INPUT_LENGTH];
			int val;
			one_argument( argument, arg4 );
			if ( arg4[0] == '\0' ) {
				send_to_char( "Set node to what value? (0-17)\n\r", ch );
				return;
			}
			val = atoi( arg4 );
			if ( val < 0 || val > STORY_NODE_MAX + 1 ) {
				send_to_char( "Node must be 0-17.\n\r", ch );
				return;
			}
			victim->pcdata->story_node = val;
			save_char_obj( victim );
			snprintf( buf, sizeof( buf ),
				"Set %s story_node to %d.\n\r", victim->name, val );
			send_to_char( buf, ch );
			return;
		}

		if ( !str_cmp( arg3, "kills" ) ) {
			char arg4[MAX_INPUT_LENGTH];
			int val;
			one_argument( argument, arg4 );
			if ( arg4[0] == '\0' ) {
				send_to_char( "Set kills to what value?\n\r", ch );
				return;
			}
			val = atoi( arg4 );
			if ( val < 0 ) val = 0;
			victim->pcdata->story_kills = val;
			save_char_obj( victim );
			snprintf( buf, sizeof( buf ),
				"Set %s story_kills to %d.\n\r", victim->name, val );
			send_to_char( buf, ch );
			return;
		}

		if ( !str_cmp( arg3, "progress" ) ) {
			char arg4[MAX_INPUT_LENGTH];
			int val;
			one_argument( argument, arg4 );
			if ( arg4[0] == '\0' ) {
				send_to_char( "Set progress to what value?\n\r", ch );
				return;
			}
			val = atoi( arg4 );
			if ( val < 0 ) val = 0;
			victim->pcdata->story_progress = (uint32_t) val;
			save_char_obj( victim );
			snprintf( buf, sizeof( buf ),
				"Set %s story_progress to 0x%02X.\n\r",
				victim->name, victim->pcdata->story_progress );
			send_to_char( buf, ch );
			return;
		}

		send_to_char( "Unknown field. Use: node, kills, progress\n\r", ch );
		return;
	}

	/*
	 * Reset — clear all story fields
	 */
	if ( !str_cmp( arg2, "reset" ) ) {
		victim->pcdata->story_node = 0;
		victim->pcdata->story_kills = 0;
		victim->pcdata->story_progress = 0;
		save_char_obj( victim );
		snprintf( buf, sizeof( buf ),
			"Reset all story fields for %s.\n\r", victim->name );
		send_to_char( buf, ch );
		send_to_char( "Your story quest progress has been reset.\n\r", victim );
		return;
	}

	/*
	 * Advance — move to next hub, clear transient state
	 */
	if ( !str_cmp( arg2, "advance" ) ) {
		if ( victim->pcdata->story_node == 0 )
			victim->pcdata->story_node = 1;
		else if ( victim->pcdata->story_node > STORY_NODE_MAX )
			;  /* already completed, no change */
		else
			victim->pcdata->story_node++;

		victim->pcdata->story_kills = 0;
		victim->pcdata->story_progress = 0;
		save_char_obj( victim );
		snprintf( buf, sizeof( buf ),
			"Advanced %s to story node %d.\n\r",
			victim->name, victim->pcdata->story_node );
		send_to_char( buf, ch );
		return;
	}

	send_to_char( "Unknown subcommand. Type 'storyadmin' for help.\n\r", ch );
}
