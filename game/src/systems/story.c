/*
 * story.c — "Echoes of the Sundering" story quest system.
 *
 * A hub-and-spoke narrative quest separate from the main quest system.
 * Players follow NPC questgivers through 16 areas, completing local
 * tasks (kill, fetch, examine) at each hub before being sent onward.
 *
 * Progress is tracked via:
 *   pcdata->story_node      — current hub (0 = not started, 1-16 = active)
 *   pcdata->story_clue      — last breadcrumb text, recallable via 'story'
 *   pcdata->story_kills     — kill counter for current hub's objective
 *   pcdata->story_progress  — bitfield: which sub-tasks are complete
 *
 * NPC dialogue and task completion are handled by Lua scripts.
 */

#include "../core/merc.h"

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

	if ( ch->pcdata->story_clue != NULL && ch->pcdata->story_clue[0] != '\0' ) {
		send_to_char( ch->pcdata->story_clue, ch );
		send_to_char( "\n\r", ch );
	} else {
		send_to_char( "A whisper echoes in your mind: #C\"Seek the one who stands "
			"in judgment at the Temple of Midgaard. Ask about the darkness "
			"that spreads through these lands.\"#n\n\r", ch );
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
