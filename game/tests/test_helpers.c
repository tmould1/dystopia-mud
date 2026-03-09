/*
 * Test helpers for Dystopia MUD unit tests
 */

#include "merc.h"
#include "test_helpers.h"
#include <stdlib.h>
#include <string.h>

extern const struct cmd_type cmd_table[];

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

	/* Initialize list nodes and heads */
	list_node_init( &ch->char_node );
	list_node_init( &ch->room_node );
	list_node_init( &ch->extracted_node );
	list_init( &ch->affects );
	list_init( &ch->carrying );

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

	/* Initialize list nodes and heads */
	list_node_init( &ch->char_node );
	list_node_init( &ch->room_node );
	list_node_init( &ch->extracted_node );
	list_init( &ch->affects );
	list_init( &ch->carrying );

	return ch;
}

CHAR_DATA *make_full_test_npc( void ) {
	CHAR_DATA *ch;

	ch = (CHAR_DATA *) calloc( 1, sizeof( CHAR_DATA ) );
	clear_char( ch );
	ch->act = ACT_IS_NPC;
	free( ch->name );
	ch->name = str_dup( "TestMob" );

	return ch;
}

void free_test_char( CHAR_DATA *ch ) {
	if ( ch == NULL )
		return;

	/* Strip any remaining affects */
	if ( ch->affects.sentinel.next != NULL ) {
		AFFECT_DATA *paf, *paf_next;
		LIST_FOR_EACH_SAFE( paf, paf_next, &ch->affects, AFFECT_DATA, node ) {
			list_remove( &ch->affects, &paf->node );
			free( paf );
		}
	}

	if ( ch->pcdata != NULL )
		free( ch->pcdata );

	free( ch );
}

void seed_rng( int seed ) {
	current_time = (time_t) seed;
	init_mm();
}

static bool booted = FALSE;

bool ensure_booted( void ) {
	if ( booted )
		return TRUE;

	/* boot_headless derives paths from the exe location.
	 * The test exe runs from game/tests/, gamedata is at ../../gamedata/ */
	boot_headless( "../../gamedata/dystopia.exe" );
	booted = TRUE;
	return TRUE;
}

AFFECT_DATA make_test_affect( int type, int duration, int location,
	int modifier, int bitvector ) {
	AFFECT_DATA af;
	memset( &af, 0, sizeof( af ) );
	af.type = type;
	af.duration = duration;
	af.location = location;
	af.modifier = modifier;
	af.bitvector = bitvector;
	list_node_init( &af.node );
	return af;
}

int find_cmd_index( const char *name ) {
	int cmd;
	for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ ) {
		if ( !str_cmp( name, cmd_table[cmd].name ) )
			return cmd;
	}
	return -1;
}
