/*
 * script_api.c — C functions exposed to Lua scripts.
 *
 * Three metatables are registered for lightuserdata:
 *   "Char" — CHAR_DATA methods (name, say, send, etc.)
 *   "Room" — ROOM_INDEX_DATA methods (vnum, find_mob, echo)
 *   "Obj"  — OBJ_DATA methods (name, to_char, to_room)
 *
 * A "game" global table provides factory functions:
 *   game.create_object(vnum)
 *   game.create_portal(dest_vnum, room)
 *   game.cast_spell(sn, level, ch)
 */

#include "merc.h"
#include "script.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


/* ================================================================
 * Helpers — extract typed pointers from Lua arguments
 * ================================================================ */

static CHAR_DATA *check_char( lua_State *L, int idx ) {
	CHAR_DATA *ch;

	if ( !lua_isuserdata( L, idx ) ) {
		luaL_error( L, "expected character userdata at argument %d", idx );
		return NULL;
	}

	ch = (CHAR_DATA *) lua_touserdata( L, idx );
	if ( ch == NULL ) {
		luaL_error( L, "NULL character at argument %d", idx );
		return NULL;
	}

	return ch;
}

static ROOM_INDEX_DATA *check_room( lua_State *L, int idx ) {
	ROOM_INDEX_DATA *room;

	if ( !lua_isuserdata( L, idx ) ) {
		luaL_error( L, "expected room userdata at argument %d", idx );
		return NULL;
	}

	room = (ROOM_INDEX_DATA *) lua_touserdata( L, idx );
	if ( room == NULL ) {
		luaL_error( L, "NULL room at argument %d", idx );
		return NULL;
	}

	return room;
}

static OBJ_DATA *check_obj( lua_State *L, int idx ) {
	OBJ_DATA *obj;

	if ( !lua_isuserdata( L, idx ) ) {
		luaL_error( L, "expected object userdata at argument %d", idx );
		return NULL;
	}

	obj = (OBJ_DATA *) lua_touserdata( L, idx );
	if ( obj == NULL ) {
		luaL_error( L, "NULL object at argument %d", idx );
		return NULL;
	}

	return obj;
}


/* Helper: push a CHAR_DATA* as lightuserdata with Char metatable */
static void push_char( lua_State *L, CHAR_DATA *ch ) {
	lua_pushlightuserdata( L, ch );
	luaL_getmetatable( L, "Char" );
	lua_setmetatable( L, -2 );
}

/* Helper: push a ROOM_INDEX_DATA* as lightuserdata with Room metatable */
static void push_room( lua_State *L, ROOM_INDEX_DATA *room ) {
	lua_pushlightuserdata( L, room );
	luaL_getmetatable( L, "Room" );
	lua_setmetatable( L, -2 );
}

/* Helper: push an OBJ_DATA* as lightuserdata with Obj metatable */
static void push_obj( lua_State *L, OBJ_DATA *obj ) {
	lua_pushlightuserdata( L, obj );
	luaL_getmetatable( L, "Obj" );
	lua_setmetatable( L, -2 );
}


/* ================================================================
 * "Char" metatable methods
 * ================================================================ */

/* ch:name() — returns the character's visible name */
static int api_char_name( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );

	if ( IS_NPC( ch ) )
		lua_pushstring( L, ch->short_descr );
	else
		lua_pushstring( L, ch->name );

	return 1;
}

/* ch:level() — returns the character's level */
static int api_char_level( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushinteger( L, ch->level );
	return 1;
}

/* ch:is_npc() — returns true if the character is an NPC */
static int api_char_is_npc( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushboolean( L, IS_NPC( ch ) );
	return 1;
}

/* ch:is_immortal() — returns true if the character is an immortal */
static int api_char_is_immortal( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushboolean( L, ch->level >= LEVEL_IMMORTAL );
	return 1;
}

/* ch:hp() — returns current hit points */
static int api_char_hp( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushinteger( L, ch->hit );
	return 1;
}

/* ch:max_hp() — returns maximum hit points */
static int api_char_max_hp( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushinteger( L, ch->max_hit );
	return 1;
}

/* ch:send(text) — send a message to the character */
static int api_char_send( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	const char *text = luaL_checkstring( L, 2 );
	send_to_char( text, ch );
	return 0;
}

/* mob:say(text) — NPC says something (uses do_say) */
static int api_char_say( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	const char *text = luaL_checkstring( L, 2 );
	char buf[MAX_STRING_LENGTH];

	if ( !IS_NPC( ch ) ) {
		luaL_error( L, "say() can only be called on NPCs" );
		return 0;
	}

	snprintf( buf, sizeof( buf ), "%s", text );
	do_say( ch, buf );
	return 0;
}

/* mob:emote(text) — NPC emotes (uses do_emote) */
static int api_char_emote( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	const char *text = luaL_checkstring( L, 2 );
	char buf[MAX_STRING_LENGTH];

	if ( !IS_NPC( ch ) ) {
		luaL_error( L, "emote() can only be called on NPCs" );
		return 0;
	}

	snprintf( buf, sizeof( buf ), "%s", text );
	do_emote( ch, buf );
	return 0;
}

/* ch:room() — returns the character's current room as Room userdata */
static int api_char_room( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );

	if ( ch->in_room == NULL ) {
		lua_pushnil( L );
		return 1;
	}

	push_room( L, ch->in_room );
	return 1;
}

/* ch:teleport(vnum) — move character to a room by vnum */
static int api_char_teleport( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	int vnum = (int) luaL_checkinteger( L, 2 );
	ROOM_INDEX_DATA *dest;
	char buf[MAX_INPUT_LENGTH];

	dest = get_room_index( vnum );
	if ( dest == NULL ) {
		luaL_error( L, "teleport: invalid room vnum %d", vnum );
		return 0;
	}

	if ( ch->in_room != NULL )
		char_from_room( ch );
	char_to_room( ch, dest );
	act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
	snprintf( buf, sizeof( buf ), "auto" );
	do_look( ch, buf );
	return 0;
}


static const luaL_Reg char_methods[] = {
	{ "name",         api_char_name },
	{ "level",        api_char_level },
	{ "is_npc",       api_char_is_npc },
	{ "is_immortal",  api_char_is_immortal },
	{ "hp",           api_char_hp },
	{ "max_hp",       api_char_max_hp },
	{ "send",         api_char_send },
	{ "say",          api_char_say },
	{ "emote",        api_char_emote },
	{ "room",         api_char_room },
	{ "teleport",     api_char_teleport },
	{ NULL,           NULL }
};


/* ================================================================
 * "Room" metatable methods
 * ================================================================ */

/* room:vnum() — returns the room's vnum */
static int api_room_vnum( lua_State *L ) {
	ROOM_INDEX_DATA *room = check_room( L, 1 );
	lua_pushinteger( L, room->vnum );
	return 1;
}

/* room:find_mob(vnum) — find an NPC in the room by template vnum */
static int api_room_find_mob( lua_State *L ) {
	ROOM_INDEX_DATA *room = check_room( L, 1 );
	int vnum = (int) luaL_checkinteger( L, 2 );
	CHAR_DATA *vch;

	LIST_FOR_EACH( vch, &room->characters, CHAR_DATA, room_node ) {
		if ( IS_NPC( vch ) && vch->pIndexData != NULL
			&& vch->pIndexData->vnum == vnum ) {
			push_char( L, vch );
			return 1;
		}
	}

	lua_pushnil( L );
	return 1;
}

/* room:echo(text) — send a message to everyone in the room */
static int api_room_echo( lua_State *L ) {
	ROOM_INDEX_DATA *room = check_room( L, 1 );
	const char *text = luaL_checkstring( L, 2 );
	CHAR_DATA *vch;

	LIST_FOR_EACH( vch, &room->characters, CHAR_DATA, room_node ) {
		send_to_char( text, vch );
	}

	return 0;
}


static const luaL_Reg room_methods[] = {
	{ "vnum",         api_room_vnum },
	{ "find_mob",     api_room_find_mob },
	{ "echo",         api_room_echo },
	{ NULL,           NULL }
};


/* ================================================================
 * "Obj" metatable methods
 * ================================================================ */

/* obj:name() — returns the object's short description */
static int api_obj_name( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	lua_pushstring( L, obj->short_descr );
	return 1;
}

/* obj:to_char(ch) — give the object to a character */
static int api_obj_to_char( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	CHAR_DATA *ch = check_char( L, 2 );
	obj_to_char( obj, ch );
	return 0;
}

/* obj:to_room(room) — place the object in a room */
static int api_obj_to_room( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	ROOM_INDEX_DATA *room = check_room( L, 2 );
	obj_to_room( obj, room );
	return 0;
}

/* obj:set_timer(n) — set the object's decay timer */
static int api_obj_set_timer( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	int n = (int) luaL_checkinteger( L, 2 );
	obj->timer = n;
	return 0;
}


static const luaL_Reg obj_methods[] = {
	{ "name",         api_obj_name },
	{ "to_char",      api_obj_to_char },
	{ "to_room",      api_obj_to_room },
	{ "set_timer",    api_obj_set_timer },
	{ NULL,           NULL }
};


/* ================================================================
 * "game" global table — factory functions
 * ================================================================ */

/* game.create_object(vnum [, level]) — create an object from a template */
static int api_game_create_object( lua_State *L ) {
	int vnum = (int) luaL_checkinteger( L, 1 );
	int level = (int) luaL_optinteger( L, 2, 1 );
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;

	pObjIndex = get_obj_index( vnum );
	if ( pObjIndex == NULL ) {
		luaL_error( L, "create_object: invalid vnum %d", vnum );
		return 0;
	}

	obj = create_object( pObjIndex, level );
	push_obj( L, obj );
	return 1;
}

/* game.create_portal(dest_vnum, room [, timer]) — create a portal in a room */
static int api_game_create_portal( lua_State *L ) {
	int dest = (int) luaL_checkinteger( L, 1 );
	ROOM_INDEX_DATA *room = check_room( L, 2 );
	int timer = (int) luaL_optinteger( L, 3, 1 );
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;

	if ( get_room_index( dest ) == NULL ) {
		luaL_error( L, "create_portal: invalid destination vnum %d", dest );
		return 0;
	}

	pObjIndex = get_obj_index( OBJ_VNUM_PORTAL );
	if ( pObjIndex == NULL ) {
		luaL_error( L, "create_portal: portal template (vnum %d) not found",
			OBJ_VNUM_PORTAL );
		return 0;
	}

	obj = create_object( pObjIndex, 0 );
	obj->timer = timer;
	obj->value[0] = dest;
	obj->value[1] = 1;
	obj_to_room( obj, room );

	push_obj( L, obj );
	return 1;
}

/* game.cast_spell(sn, level, ch) — cast a spell on a character */
static int api_game_cast_spell( lua_State *L ) {
	int sn = (int) luaL_checkinteger( L, 1 );
	int level = (int) luaL_checkinteger( L, 2 );
	CHAR_DATA *ch = check_char( L, 3 );

	if ( sn < 0 || sn >= MAX_SKILL ) {
		luaL_error( L, "cast_spell: invalid skill number %d", sn );
		return 0;
	}

	if ( skill_table[sn].spell_fun == NULL ) {
		luaL_error( L, "cast_spell: skill %d has no spell function", sn );
		return 0;
	}

	( *skill_table[sn].spell_fun )( sn, level, ch, ch );
	return 0;
}


static const luaL_Reg game_funcs[] = {
	{ "create_object",  api_game_create_object },
	{ "create_portal",  api_game_create_portal },
	{ "cast_spell",     api_game_cast_spell },
	{ NULL,             NULL }
};


/* ================================================================
 * Registration — called from script_init()
 * ================================================================ */

/* Helper: register a metatable with methods and locked __metatable */
static void register_metatable( lua_State *L, const char *name,
	const luaL_Reg *methods ) {
	luaL_newmetatable( L, name );

	lua_newtable( L );
	luaL_setfuncs( L, methods, 0 );
	lua_setfield( L, -2, "__index" );

	lua_pushboolean( L, 0 );
	lua_setfield( L, -2, "__metatable" );

	lua_pop( L, 1 );
}

void script_register_char_api( lua_State *L ) {
	register_metatable( L, "Char", char_methods );
}

void script_register_room_api( lua_State *L ) {
	register_metatable( L, "Room", room_methods );
}

void script_register_obj_api( lua_State *L ) {
	register_metatable( L, "Obj", obj_methods );

	/* Register the "game" global table */
	lua_newtable( L );
	luaL_setfuncs( L, game_funcs, 0 );
	lua_setglobal( L, "game" );
}
