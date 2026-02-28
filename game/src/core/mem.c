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

/***************************************************************************
 *  File: mem.c                                                            *
 *                                                                         *
 *  OLC object allocation and deallocation.                                *
 *  Uses standard calloc/free (free lists removed).                        *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

/*
 * Globals — allocation counters for statistics
 */
extern int top_reset;
extern int top_area;
extern int top_exit;
extern int top_ed;
extern int top_room;

/*****************************************************************************
 Name:		new_reset_data
 Purpose:	Creates and clears a reset structure.
 ****************************************************************************/
RESET_DATA *new_reset_data( void ) {
	RESET_DATA *pReset;

	pReset = calloc( 1, sizeof( *pReset ) );
	if ( !pReset ) {
		bug( "new_reset_data: calloc failed", 0 );
		exit( 1 );
	}
	top_reset++;

	pReset->command = 'X';

	return pReset;
}

void free_reset_data( RESET_DATA *pReset ) {
	if ( !pReset ) return;
	free( pReset );
}

/*****************************************************************************
 Name:		new_area
 Purpose:	Creates and clears a new area structure.
 ****************************************************************************/
AREA_DATA *new_area( void ) {
	AREA_DATA *pArea;
	char buf[MAX_INPUT_LENGTH];

	pArea = calloc( 1, sizeof( *pArea ) );
	if ( !pArea ) {
		bug( "new_area: calloc failed", 0 );
		exit( 1 );
	}
	top_area++;

	pArea->name = str_dup( "New area" );
	pArea->recall = ROOM_VNUM_TEMPLE;
	pArea->area_flags = AREA_ADDED;
	pArea->security = 1;
	pArea->builders = str_dup( "None" );
	pArea->vnum = top_area - 1;
	snprintf( buf, sizeof( buf ), "area%d.are", pArea->vnum );
	pArea->filename = str_dup( buf );

	return pArea;
}

void free_area( AREA_DATA *pArea ) {
	if ( !pArea ) return;
	free(pArea->name);
	free(pArea->filename);
	free(pArea->builders);
	free( pArea );
}

EXIT_DATA *new_exit( void ) {
	EXIT_DATA *pExit;

	pExit = calloc( 1, sizeof( *pExit ) );
	if ( !pExit ) {
		bug( "new_exit: calloc failed", 0 );
		exit( 1 );
	}
	top_exit++;

	pExit->keyword = str_dup( "" );
	pExit->description = str_dup( "" );

	return pExit;
}

void free_exit( EXIT_DATA *pExit ) {
	if ( !pExit ) return;
	free(pExit->keyword);
	free(pExit->description);
	free( pExit );
}

EXTRA_DESCR_DATA *new_extra_descr( void ) {
	EXTRA_DESCR_DATA *pExtra;

	pExtra = calloc( 1, sizeof( *pExtra ) );
	if ( !pExtra ) {
		bug( "new_extra_descr: calloc failed", 0 );
		exit( 1 );
	}
	top_ed++;

	return pExtra;
}

void free_extra_descr( EXTRA_DESCR_DATA *pExtra ) {
	if ( !pExtra ) return;
	free(pExtra->keyword);
	free(pExtra->description);
	free( pExtra );
}

ROOM_INDEX_DATA *new_room_index( void ) {
	ROOM_INDEX_DATA *pRoom;

	pRoom = calloc( 1, sizeof( *pRoom ) );
	if ( !pRoom ) {
		bug( "new_room_index: calloc failed", 0 );
		exit( 1 );
	}
	top_room++;

	list_init( &pRoom->characters );
	list_init( &pRoom->objects );
	pRoom->name = str_dup( "" );
	pRoom->description = str_dup( "" );

	return pRoom;
}

void free_room_index( ROOM_INDEX_DATA *pRoom ) {
	int door;
	EXTRA_DESCR_DATA *pExtra;
	EXTRA_DESCR_DATA *pExtra_next;
	RESET_DATA *pReset;
	RESET_DATA *pReset_next;

	if ( !pRoom ) return;

	free(pRoom->name);
	free(pRoom->description);

	for ( door = 0; door < MAX_DIR; door++ ) {
		if ( pRoom->exit[door] )
			free_exit( pRoom->exit[door] );
	}

	for ( pExtra = pRoom->extra_descr; pExtra; pExtra = pExtra_next ) {
		pExtra_next = pExtra->next;
		free_extra_descr( pExtra );
	}

	for ( pReset = pRoom->reset_first; pReset; pReset = pReset_next ) {
		pReset_next = pReset->next;
		free_reset_data( pReset );
	}

	free( pRoom );
}

AFFECT_DATA *new_affect( void ) {
	AFFECT_DATA *pAf;

	pAf = calloc( 1, sizeof( *pAf ) );
	if ( !pAf ) {
		bug( "new_affect: calloc failed", 0 );
		exit( 1 );
	}
	top_affect++;

	return pAf;
}

void free_affect( AFFECT_DATA *pAf ) {
	if ( !pAf ) return;
	free( pAf );
}

SHOP_DATA *new_shop( void ) {
	SHOP_DATA *pShop;

	pShop = calloc( 1, sizeof( *pShop ) );
	if ( !pShop ) {
		bug( "new_shop: calloc failed", 0 );
		exit( 1 );
	}
	top_shop++;

	pShop->profit_buy = 100;
	pShop->profit_sell = 100;
	pShop->close_hour = 23;

	return pShop;
}

void free_shop( SHOP_DATA *pShop ) {
	if ( !pShop ) return;
	free( pShop );
}

OBJ_INDEX_DATA *new_obj_index( void ) {
	OBJ_INDEX_DATA *pObj;

	pObj = calloc( 1, sizeof( *pObj ) );
	if ( !pObj ) {
		bug( "new_obj_index: calloc failed", 0 );
		exit( 1 );
	}
	top_obj_index++;

	list_init( &pObj->affects );
	pObj->name = str_dup( "no name" );
	pObj->short_descr = str_dup( "(no short description)" );
	pObj->description = str_dup( "(no description)" );
	pObj->item_type = ITEM_TRASH;

	return pObj;
}

void free_obj_index( OBJ_INDEX_DATA *pObj ) {
	EXTRA_DESCR_DATA *pExtra;
	EXTRA_DESCR_DATA *pExtra_next;
	AFFECT_DATA *pAf;
	AFFECT_DATA *pAf_next;

	if ( !pObj ) return;

	free(pObj->name);
	free(pObj->short_descr);
	free(pObj->description);

	LIST_FOR_EACH_SAFE( pAf, pAf_next, &pObj->affects, AFFECT_DATA, node ) {
		list_remove( &pObj->affects, &pAf->node );
		free_affect( pAf );
	}

	for ( pExtra = pObj->extra_descr; pExtra; pExtra = pExtra_next ) {
		pExtra_next = pExtra->next;
		free_extra_descr( pExtra );
	}

	free( pObj );
}

MOB_INDEX_DATA *new_mob_index( void ) {
	MOB_INDEX_DATA *pMob;

	pMob = calloc( 1, sizeof( *pMob ) );
	if ( !pMob ) {
		bug( "new_mob_index: calloc failed", 0 );
		exit( 1 );
	}
	top_mob_index++;

	pMob->player_name = str_dup( "no name" );
	pMob->short_descr = str_dup( "(no short description)" );
	pMob->long_descr = str_dup( "(no long description)\n\r" );
	pMob->description = str_dup( "" );
	pMob->act = ACT_IS_NPC;

	return pMob;
}

void free_mob_index( MOB_INDEX_DATA *pMob ) {
	if ( !pMob ) return;

	free(pMob->player_name);
	free(pMob->short_descr);
	free(pMob->long_descr);
	free(pMob->description);

	if ( pMob->pShop )
		free_shop( pMob->pShop );

	free( pMob );
}
