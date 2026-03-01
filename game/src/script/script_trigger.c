/*
 * script_trigger.c — Trigger dispatch for Lua scripts.
 *
 * Each trigger function is called from a specific game hook point.
 * It iterates NPCs or room scripts, checks trigger type and conditions,
 * then calls script_run() to execute the Lua callback.
 */

#include "merc.h"
#include "script.h"


/* Defined in script_lua.c */
void script_run( SCRIPT_DATA *script, const char *func,
	CHAR_DATA *mob, CHAR_DATA *ch, const char *text );
void script_run_room( SCRIPT_DATA *script, const char *func,
	CHAR_DATA *ch, ROOM_INDEX_DATA *room, const char *text );
void script_run_obj( SCRIPT_DATA *script, const char *func,
	OBJ_DATA *obj, CHAR_DATA *ch, CHAR_DATA *victim );
bool script_run_tick( SCRIPT_DATA *script, CHAR_DATA *mob );


/*
 * Check if a script's chance roll succeeds.
 * chance == 0 means always fire.
 */
static bool script_chance_check( SCRIPT_DATA *script ) {
	if ( script->chance <= 0 )
		return TRUE;
	return ( number_range( 1, 100 ) <= script->chance );
}


/*
 * Check if text matches a script's pattern.
 * NULL or empty pattern matches anything.
 * Uses case-insensitive substring search.
 */
static bool script_pattern_match( SCRIPT_DATA *script, const char *text ) {
	if ( script->pattern == NULL || script->pattern[0] == '\0' )
		return TRUE;
	if ( text == NULL || text[0] == '\0' )
		return FALSE;
	return ( str_infix( script->pattern, text ) == 0 );
}


/*
 * TRIG_GREET — fired when a player enters a room.
 * Iterates all NPCs in the room and fires their TRIG_GREET scripts.
 *
 * Called from act_move.c and nanny.c after char_to_room().
 */
void script_trigger_greet( CHAR_DATA *ch, ROOM_INDEX_DATA *room ) {
	CHAR_DATA *mob;
	CHAR_DATA *mob_next;
	SCRIPT_DATA *script;

	if ( ch == NULL || room == NULL )
		return;

	/* Only player entry triggers greet */
	if ( IS_NPC( ch ) )
		return;

	LIST_FOR_EACH_SAFE( mob, mob_next, &room->characters, CHAR_DATA, room_node ) {
		if ( !IS_NPC( mob ) )
			continue;
		if ( mob == ch )
			continue;
		if ( mob->pIndexData == NULL )
			continue;

		LIST_FOR_EACH( script, &mob->pIndexData->scripts, SCRIPT_DATA, node ) {
			if ( !IS_SET( script->trigger, TRIG_GREET ) )
				continue;
			if ( !script_chance_check( script ) )
				continue;

			script_run( script, "on_greet", mob, ch, NULL );
		}
	}
}


/*
 * TRIG_SPEECH — fired when a player says something.
 * Iterates all NPCs in the room and fires matching TRIG_SPEECH scripts.
 *
 * Called from act_comm.c in do_say().
 */
void script_trigger_speech( CHAR_DATA *ch, const char *text ) {
	CHAR_DATA *mob;
	CHAR_DATA *mob_next;
	SCRIPT_DATA *script;
	ROOM_INDEX_DATA *room;

	if ( ch == NULL || text == NULL || text[0] == '\0' )
		return;

	/* Only player speech triggers scripts */
	if ( IS_NPC( ch ) )
		return;

	room = ch->in_room;
	if ( room == NULL )
		return;

	LIST_FOR_EACH_SAFE( mob, mob_next, &room->characters, CHAR_DATA, room_node ) {
		if ( !IS_NPC( mob ) )
			continue;
		if ( mob == ch )
			continue;
		if ( mob->pIndexData == NULL )
			continue;

		LIST_FOR_EACH( script, &mob->pIndexData->scripts, SCRIPT_DATA, node ) {
			if ( !IS_SET( script->trigger, TRIG_SPEECH ) )
				continue;
			if ( !script_pattern_match( script, text ) )
				continue;
			if ( !script_chance_check( script ) )
				continue;

			script_run( script, "on_speech", mob, ch, text );
		}
	}
}


/*
 * TRIG_TICK — fired once per game tick for each NPC.
 * Iterates the mob's template scripts and fires TRIG_TICK scripts.
 * Returns TRUE if any script returned true (skip remaining mob AI).
 *
 * Lua callback: on_tick(mob) → true/false
 *
 * Called from mobile_update() in update.c.
 */
bool script_trigger_tick( CHAR_DATA *ch ) {
	SCRIPT_DATA *script;

	if ( ch == NULL || ch->pIndexData == NULL )
		return FALSE;

	LIST_FOR_EACH( script, &ch->pIndexData->scripts, SCRIPT_DATA, node ) {
		if ( !IS_SET( script->trigger, TRIG_TICK ) )
			continue;
		if ( !script_chance_check( script ) )
			continue;

		if ( script_run_tick( script, ch ) )
			return TRUE;
	}

	return FALSE;
}


/*
 * TRIG_GREET on rooms — fired when a player enters a room.
 * Iterates the room's own scripts (not mob scripts).
 *
 * Lua callback: on_enter(ch, room)
 */
void script_trigger_room_enter( CHAR_DATA *ch, ROOM_INDEX_DATA *room ) {
	SCRIPT_DATA *script;

	if ( ch == NULL || room == NULL )
		return;

	if ( IS_NPC( ch ) )
		return;

	LIST_FOR_EACH( script, &room->scripts, SCRIPT_DATA, node ) {
		if ( !IS_SET( script->trigger, TRIG_GREET ) )
			continue;
		if ( !script_chance_check( script ) )
			continue;

		script_run_room( script, "on_enter", ch, room, NULL );
	}
}


/*
 * TRIG_SPEECH on rooms — fired when a player says something.
 * Iterates the room's own scripts (not mob scripts).
 *
 * Lua callback: on_say(ch, room, text)
 */
void script_trigger_room_speech( CHAR_DATA *ch, const char *text ) {
	SCRIPT_DATA *script;
	ROOM_INDEX_DATA *room;

	if ( ch == NULL || text == NULL || text[0] == '\0' )
		return;

	if ( IS_NPC( ch ) )
		return;

	room = ch->in_room;
	if ( room == NULL )
		return;

	LIST_FOR_EACH( script, &room->scripts, SCRIPT_DATA, node ) {
		if ( !IS_SET( script->trigger, TRIG_SPEECH ) )
			continue;
		if ( !script_pattern_match( script, text ) )
			continue;
		if ( !script_chance_check( script ) )
			continue;

		script_run_room( script, "on_say", ch, room, text );
	}
}


/*
 * TRIG_TICK on objects — fired periodically for carried/worn objects.
 * Iterates all objects in the game, fires TRIG_TICK scripts on objects
 * that are carried by a character.
 *
 * Lua callback: on_tick(obj)
 *
 * Called from update_handler() on the pulse_point cycle.
 */
void script_trigger_obj_tick( void ) {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	SCRIPT_DATA *script;
	CHAR_DATA *carrier;

	LIST_FOR_EACH_SAFE( obj, obj_next, &g_objects, OBJ_DATA, obj_node ) {
		if ( obj->pIndexData == NULL )
			continue;
		if ( list_empty( &obj->pIndexData->scripts ) )
			continue;

		carrier = obj->carried_by;
		if ( carrier == NULL )
			continue;

		LIST_FOR_EACH( script, &obj->pIndexData->scripts, SCRIPT_DATA, node ) {
			if ( !IS_SET( script->trigger, TRIG_TICK ) )
				continue;
			if ( !script_chance_check( script ) )
				continue;

			script_run_obj( script, "on_tick", obj, carrier, NULL );
		}
	}
}


/*
 * TRIG_KILL on objects — fired when the carrier kills an NPC.
 * Iterates all objects carried/worn by the killer and fires TRIG_KILL
 * scripts.
 *
 * Lua callback: on_kill(obj, killer, victim)
 *
 * Called from fight.c when a player kills an NPC.
 */
void script_trigger_obj_kill( CHAR_DATA *ch, CHAR_DATA *victim ) {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	SCRIPT_DATA *script;

	if ( ch == NULL || victim == NULL )
		return;

	LIST_FOR_EACH_SAFE( obj, obj_next, &ch->carrying, OBJ_DATA, content_node ) {
		if ( obj->pIndexData == NULL )
			continue;
		if ( list_empty( &obj->pIndexData->scripts ) )
			continue;

		LIST_FOR_EACH( script, &obj->pIndexData->scripts, SCRIPT_DATA, node ) {
			if ( !IS_SET( script->trigger, TRIG_KILL ) )
				continue;
			if ( !script_chance_check( script ) )
				continue;

			script_run_obj( script, "on_kill", obj, ch, victim );
		}
	}
}
