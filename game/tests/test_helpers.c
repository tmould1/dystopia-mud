/*
 * Test helpers for Dystopia MUD unit tests
 */

#include "merc.h"
#include "test_helpers.h"
#include <stdlib.h>
#include <string.h>

CHAR_DATA *make_test_player( void ) {
	CHAR_DATA *ch;
	PC_DATA *pcdata;

	ch = (CHAR_DATA *) calloc( 1, sizeof( CHAR_DATA ) );
	pcdata = (PC_DATA *) calloc( 1, sizeof( PC_DATA ) );

	ch->pcdata = pcdata;
	ch->desc = NULL;
	ch->name = "TestPlayer";
	ch->level = 1;
	ch->trust = 0;
	ch->act = 0; /* Not NPC */
	ch->class = 0;

	/* Default stats: 13 across the board */
	pcdata->perm_str = 13;
	pcdata->perm_int = 13;
	pcdata->perm_wis = 13;
	pcdata->perm_dex = 13;
	pcdata->perm_con = 13;
	pcdata->mod_str = 0;
	pcdata->mod_int = 0;
	pcdata->mod_wis = 0;
	pcdata->mod_dex = 0;
	pcdata->mod_con = 0;

	return ch;
}

CHAR_DATA *make_test_npc( void ) {
	CHAR_DATA *ch;

	ch = (CHAR_DATA *) calloc( 1, sizeof( CHAR_DATA ) );

	ch->pcdata = NULL;
	ch->desc = NULL;
	ch->name = "TestMob";
	ch->level = 5;
	ch->trust = 0;
	ch->act = ACT_IS_NPC;

	return ch;
}

void free_test_char( CHAR_DATA *ch ) {
	if ( ch == NULL )
		return;

	if ( ch->pcdata != NULL )
		free( ch->pcdata );

	free( ch );
}

void seed_rng( int seed ) {
	current_time = (time_t) seed;
	init_mm();
}
