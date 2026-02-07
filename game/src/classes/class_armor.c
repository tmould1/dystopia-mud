/***************************************************************************
 *  class_armor.c - Generic class armor creation system
 *
 *  Provides database-driven armor creation for all classes.
 *  Configuration stored in game.db tables: class_armor_config, class_armor_pieces
 ***************************************************************************/

#include "merc.h"
#include "../db/db_game.h"
#include "../core/cfg.h"

/*
 * Generic class armor creation function.
 * Called by class-specific armor commands with the class_id.
 */
void do_classarmor_generic( CHAR_DATA *ch, char *argument, int class_id ) {
	const CLASS_ARMOR_CONFIG *config;
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;
	char arg[MAX_INPUT_LENGTH];
	int vnum;
	int cost;

	argument = one_argument( argument, arg );

	if ( IS_NPC( ch ) )
		return;

	/* Get armor config for this class */
	config = db_class_get_armor_config( class_id );
	if ( config == NULL ) {
		send_to_char( "Your class does not have armor configuration.\n\r", ch );
		return;
	}

	/* Check class (allow immortals to bypass) */
	if ( !IS_IMMORTAL( ch ) && !IS_CLASS( ch, class_id ) ) {
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	/* Show usage if no argument */
	if ( arg[0] == '\0' ) {
		send_to_char( config->usage_message, ch );
		send_to_char( "\n\r", ch );
		return;
	}

	/* Check primal cost - convert string key from DB to cfg enum */
	{
		char full_key[128];
		cfg_key_t cost_key;

		/* DB stores keys like "vampire.vampirearmor.practice_cost", prepend "ability." */
		snprintf( full_key, sizeof( full_key ), "ability.%s", config->acfg_cost_key );
		cost_key = cfg_key_from_string( full_key );

		if ( cost_key == CFG_COUNT ) {
			log_string( "class_armor: invalid cfg key in database" );
			cost = 60;  /* Fallback default */
		} else {
			cost = cfg( cost_key );
		}
	}
	if ( !IS_IMMORTAL( ch ) && ch->practice < cost ) {
		char buf[256];
		snprintf( buf, sizeof( buf ), "It costs %d points of primal to create equipment.\n\r", cost );
		send_to_char( buf, ch );
		return;
	}

	/* Look up the vnum for this piece */
	vnum = db_class_get_armor_vnum( class_id, arg );
	if ( vnum == 0 ) {
		/* Invalid piece - show usage */
		send_to_char( config->usage_message, ch );
		send_to_char( "\n\r", ch );
		return;
	}

	/* Get the object template */
	pObjIndex = get_obj_index( vnum );
	if ( pObjIndex == NULL ) {
		send_to_char( "Missing object, please inform a God.\n\r", ch );
		return;
	}

	/* Deduct cost and create object */
	if ( !IS_IMMORTAL( ch ) )
		ch->practice -= cost;

	obj = create_object( pObjIndex, 50 );
	obj->questowner = str_dup( ch->pcdata->switchname );
	obj_to_char( obj, ch );

	act( config->act_to_char, ch, obj, NULL, TO_CHAR );
	act( config->act_to_room, ch, obj, NULL, TO_ROOM );
}
