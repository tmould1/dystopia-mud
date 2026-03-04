/*
 * story.c — "Echoes of the Sundering" story quest system.
 *
 * A breadcrumb-driven narrative quest separate from the main quest system.
 * Players follow NPC questgivers through the world, piecing together the
 * story of the Sundering — the event that fractured reality and created
 * the patchwork world they inhabit.
 *
 * Progress is tracked via pcdata->story_node (current node, 0 = not started)
 * and pcdata->story_clue (last breadcrumb text, recallable via 'story').
 * NPC dialogue is delivered by Lua TRIG_SPEECH scripts.
 */

#include "../core/merc.h"

/* Total number of story nodes (including branch endpoints) */
#define STORY_NODE_MAX 18

/*
 * do_story — recall the last story quest breadcrumb.
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
}
