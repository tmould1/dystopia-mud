/*
 * script_api.c — C functions exposed to Lua scripts.
 *
 * Three metatables are registered for full userdata (pointer boxes):
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
 *
 * We use full userdata (a pointer-sized box) rather than lightuserdata
 * because Lua 5.4 lightuserdata all share a single metatable — setting
 * one type's metatable overwrites the others.  Full userdata each have
 * their own metatable, so Char/Room/Obj can coexist on the same stack.
 * ================================================================ */

static CHAR_DATA *check_char( lua_State *L, int idx ) {
	void **ud = (void **) luaL_checkudata( L, idx, "Char" );
	if ( *ud == NULL ) {
		luaL_error( L, "NULL character at argument %d", idx );
		return NULL;
	}
	return (CHAR_DATA *) *ud;
}

static ROOM_INDEX_DATA *check_room( lua_State *L, int idx ) {
	void **ud = (void **) luaL_checkudata( L, idx, "Room" );
	if ( *ud == NULL ) {
		luaL_error( L, "NULL room at argument %d", idx );
		return NULL;
	}
	return (ROOM_INDEX_DATA *) *ud;
}

static OBJ_DATA *check_obj( lua_State *L, int idx ) {
	void **ud = (void **) luaL_checkudata( L, idx, "Obj" );
	if ( *ud == NULL ) {
		luaL_error( L, "NULL object at argument %d", idx );
		return NULL;
	}
	return (OBJ_DATA *) *ud;
}


/* Helper: push a CHAR_DATA* as full userdata with Char metatable */
static void push_char( lua_State *L, CHAR_DATA *ch ) {
	void **ud = (void **) lua_newuserdatauv( L, sizeof( void * ), 0 );
	*ud = ch;
	luaL_getmetatable( L, "Char" );
	lua_setmetatable( L, -2 );
}

/* Helper: push a ROOM_INDEX_DATA* as full userdata with Room metatable */
static void push_room( lua_State *L, ROOM_INDEX_DATA *room ) {
	void **ud = (void **) lua_newuserdatauv( L, sizeof( void * ), 0 );
	*ud = room;
	luaL_getmetatable( L, "Room" );
	lua_setmetatable( L, -2 );
}

/* Helper: push an OBJ_DATA* as full userdata with Obj metatable */
static void push_obj( lua_State *L, OBJ_DATA *obj ) {
	void **ud = (void **) lua_newuserdatauv( L, sizeof( void * ), 0 );
	*ud = obj;
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


/* ch:fighting() — returns the character's fighting target, or nil */
static int api_char_fighting( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );

	if ( ch->fighting == NULL ) {
		lua_pushnil( L );
		return 1;
	}

	push_char( L, ch->fighting );
	return 1;
}

/* ch:is_awake() — returns true if the character is awake */
static int api_char_is_awake( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushboolean( L, IS_AWAKE( ch ) );
	return 1;
}

/* ch:position() — returns the character's position (POS_*) */
static int api_char_position( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushinteger( L, ch->position );
	return 1;
}

/* ch:class() — returns the character's class bitmask */
static int api_char_class( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushinteger( L, ch->class );
	return 1;
}

/* ch:is_class(class_bit) — returns true if the character is that class */
static int api_char_is_class( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	int cls = (int) luaL_checkinteger( L, 2 );
	lua_pushboolean( L, IS_CLASS( ch, cls ) );
	return 1;
}

/* ch:alignment() — returns the character's alignment value */
static int api_char_alignment( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushinteger( L, ch->alignment );
	return 1;
}

/* ch:gold() — returns the character's gold */
static int api_char_gold( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	lua_pushinteger( L, ch->gold );
	return 1;
}

/* ch:set_gold(n) — set the character's gold */
static int api_char_set_gold( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	int n = (int) luaL_checkinteger( L, 2 );
	ch->gold = n;
	return 0;
}

/* ch:attack(victim) — initiate combat (multi_hit) */
static int api_char_attack( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	CHAR_DATA *victim = check_char( L, 2 );
	multi_hit( ch, victim, TYPE_UNDEFINED );
	return 0;
}

/* ch:do_command(cmd_with_args) — execute a game command */
static int api_char_do_command( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	const char *cmd = luaL_checkstring( L, 2 );
	char buf[MAX_INPUT_LENGTH];

	snprintf( buf, sizeof( buf ), "%s", cmd );
	interpret( ch, buf );
	return 0;
}

/* ch:can_see(target) — returns true if ch can see target */
static int api_char_can_see( lua_State *L ) {
	CHAR_DATA *ch = check_char( L, 1 );
	CHAR_DATA *target = check_char( L, 2 );
	lua_pushboolean( L, can_see( ch, target ) );
	return 1;
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
	{ "fighting",     api_char_fighting },
	{ "is_awake",     api_char_is_awake },
	{ "position",     api_char_position },
	{ "class",        api_char_class },
	{ "is_class",     api_char_is_class },
	{ "alignment",    api_char_alignment },
	{ "gold",         api_char_gold },
	{ "set_gold",     api_char_set_gold },
	{ "attack",       api_char_attack },
	{ "do_command",   api_char_do_command },
	{ "can_see",      api_char_can_see },
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


/* room:find_fighting() — find a random character in the room who is fighting */
static int api_room_find_fighting( lua_State *L ) {
	ROOM_INDEX_DATA *room = check_room( L, 1 );
	CHAR_DATA *vch;

	LIST_FOR_EACH( vch, &room->characters, CHAR_DATA, room_node ) {
		if ( vch->fighting != NULL && number_bits( 1 ) == 0 ) {
			push_char( L, vch );
			return 1;
		}
	}

	lua_pushnil( L );
	return 1;
}

/* room:find_corpse() — find the first NPC corpse in the room */
static int api_room_find_corpse( lua_State *L ) {
	ROOM_INDEX_DATA *room = check_room( L, 1 );
	OBJ_DATA *obj;

	LIST_FOR_EACH( obj, &room->objects, OBJ_DATA, room_node ) {
		if ( obj->item_type == ITEM_CORPSE_NPC ) {
			push_obj( L, obj );
			return 1;
		}
	}

	lua_pushnil( L );
	return 1;
}

/* room:find_trash() — find the first trash/cheap item in the room */
static int api_room_find_trash( lua_State *L ) {
	ROOM_INDEX_DATA *room = check_room( L, 1 );
	OBJ_DATA *obj;

	LIST_FOR_EACH( obj, &room->objects, OBJ_DATA, room_node ) {
		if ( !IS_SET( obj->wear_flags, ITEM_TAKE ) )
			continue;
		if ( obj->item_type == ITEM_DRINK_CON
			|| obj->item_type == ITEM_TRASH
			|| obj->cost < 10 ) {
			push_obj( L, obj );
			return 1;
		}
	}

	lua_pushnil( L );
	return 1;
}


/* room:has_mob() — returns true if any living NPC is in the room */
static int api_room_has_mob( lua_State *L ) {
	ROOM_INDEX_DATA *room = check_room( L, 1 );
	CHAR_DATA *vch;

	LIST_FOR_EACH( vch, &room->characters, CHAR_DATA, room_node ) {
		if ( IS_NPC( vch ) && vch->position != POS_DEAD ) {
			lua_pushboolean( L, 1 );
			return 1;
		}
	}

	lua_pushboolean( L, 0 );
	return 1;
}

/* room:first_mob_reset_vnum() — vnum of first 'M' reset, or nil */
static int api_room_first_mob_reset_vnum( lua_State *L ) {
	ROOM_INDEX_DATA *room = check_room( L, 1 );
	RESET_DATA *pReset;

	LIST_FOR_EACH( pReset, &room->resets, RESET_DATA, node ) {
		if ( pReset->command == 'M' ) {
			lua_pushinteger( L, pReset->arg1 );
			return 1;
		}
	}

	lua_pushnil( L );
	return 1;
}


static const luaL_Reg room_methods[] = {
	{ "vnum",                 api_room_vnum },
	{ "find_mob",             api_room_find_mob },
	{ "echo",                 api_room_echo },
	{ "find_fighting",        api_room_find_fighting },
	{ "find_corpse",          api_room_find_corpse },
	{ "find_trash",           api_room_find_trash },
	{ "has_mob",              api_room_has_mob },
	{ "first_mob_reset_vnum", api_room_first_mob_reset_vnum },
	{ NULL,                   NULL }
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


/* obj:item_type() — returns the object's item type */
static int api_obj_item_type( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	lua_pushinteger( L, obj->item_type );
	return 1;
}

/* obj:cost() — returns the object's cost */
static int api_obj_cost( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	lua_pushinteger( L, obj->cost );
	return 1;
}

/* obj:extract() — remove the object from the game */
static int api_obj_extract( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	extract_obj( obj );
	return 0;
}

/* obj:contents_to_room(room) — move all contents of a container to a room */
static int api_obj_contents_to_room( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	ROOM_INDEX_DATA *room = check_room( L, 2 );
	OBJ_DATA *item;
	OBJ_DATA *item_next;

	LIST_FOR_EACH_SAFE( item, item_next, &obj->contents, OBJ_DATA, content_node ) {
		obj_from_obj( item );
		obj_to_room( item, room );
	}

	return 0;
}


/* obj:carried_by() — returns the character carrying/wearing this object */
static int api_obj_carried_by( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );

	if ( obj->carried_by == NULL ) {
		lua_pushnil( L );
		return 1;
	}

	push_char( L, obj->carried_by );
	return 1;
}

/* obj:is_worn() — returns true if the object is currently equipped */
static int api_obj_is_worn( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	lua_pushboolean( L, obj->wear_loc != WEAR_NONE );
	return 1;
}

/* obj:wear_loc() — returns the current wear location (-1 if not worn) */
static int api_obj_wear_loc( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	lua_pushinteger( L, obj->wear_loc );
	return 1;
}

/* obj:vnum() — returns the object template vnum */
static int api_obj_vnum( lua_State *L ) {
	OBJ_DATA *obj = check_obj( L, 1 );
	lua_pushinteger( L, obj->pIndexData ? obj->pIndexData->vnum : 0 );
	return 1;
}


static const luaL_Reg obj_methods[] = {
	{ "name",             api_obj_name },
	{ "to_char",          api_obj_to_char },
	{ "to_room",          api_obj_to_room },
	{ "set_timer",        api_obj_set_timer },
	{ "item_type",        api_obj_item_type },
	{ "cost",             api_obj_cost },
	{ "extract",          api_obj_extract },
	{ "contents_to_room", api_obj_contents_to_room },
	{ "carried_by",       api_obj_carried_by },
	{ "is_worn",          api_obj_is_worn },
	{ "wear_loc",         api_obj_wear_loc },
	{ "vnum",             api_obj_vnum },
	{ NULL,               NULL }
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

/*
 * game.cast_spell(spell, level, caster [, target])
 *
 * spell  — skill number (int) or spell name (string)
 * level  — caster level for the spell
 * caster — the character casting
 * target — optional target; defaults to caster (self-cast)
 */
static int api_game_cast_spell( lua_State *L ) {
	int sn;
	int level;
	CHAR_DATA *ch;
	CHAR_DATA *target;

	/* Arg 1: spell number or name */
	if ( lua_isinteger( L, 1 ) ) {
		sn = (int) lua_tointeger( L, 1 );
	} else {
		const char *name = luaL_checkstring( L, 1 );
		sn = skill_lookup( name );
		if ( sn < 0 ) {
			luaL_error( L, "cast_spell: unknown spell '%s'", name );
			return 0;
		}
	}

	level = (int) luaL_checkinteger( L, 2 );
	ch = check_char( L, 3 );
	target = lua_isuserdata( L, 4 ) ? check_char( L, 4 ) : ch;

	if ( sn < 0 || sn >= MAX_SKILL ) {
		luaL_error( L, "cast_spell: invalid skill number %d", sn );
		return 0;
	}

	if ( skill_table[sn].spell_fun == NULL ) {
		luaL_error( L, "cast_spell: skill %d has no spell function", sn );
		return 0;
	}

	( *skill_table[sn].spell_fun )( sn, level, ch, target );
	return 0;
}

/* game.get_room(vnum) — look up a room by vnum */
static int api_game_get_room( lua_State *L ) {
	int vnum = (int) luaL_checkinteger( L, 1 );
	ROOM_INDEX_DATA *room = get_room_index( vnum );

	if ( room == NULL ) {
		lua_pushnil( L );
		return 1;
	}

	push_room( L, room );
	return 1;
}

/* game.create_mobile(vnum, room) — spawn a mobile and place in room */
static int api_game_create_mobile( lua_State *L ) {
	int vnum = (int) luaL_checkinteger( L, 1 );
	ROOM_INDEX_DATA *room = check_room( L, 2 );
	MOB_INDEX_DATA *pMobIndex;
	CHAR_DATA *mob;

	pMobIndex = get_mob_index( vnum );
	if ( pMobIndex == NULL ) {
		luaL_error( L, "create_mobile: invalid vnum %d", vnum );
		return 0;
	}

	mob = create_mobile( pMobIndex );
	if ( room_is_dark( room ) )
		SET_BIT( mob->affected_by, AFF_INFRARED );
	char_to_room( mob, room );

	push_char( L, mob );
	return 1;
}

/* game.random(min, max) — returns a random number in [min, max] */
static int api_game_random( lua_State *L ) {
	int lo = (int) luaL_checkinteger( L, 1 );
	int hi = (int) luaL_checkinteger( L, 2 );
	lua_pushinteger( L, number_range( lo, hi ) );
	return 1;
}

/* game.hour() — returns the current game hour (0-23) */
static int api_game_hour( lua_State *L ) {
	lua_pushinteger( L, time_info.hour );
	return 1;
}


static const luaL_Reg game_funcs[] = {
	{ "create_object",  api_game_create_object },
	{ "create_portal",  api_game_create_portal },
	{ "create_mobile",  api_game_create_mobile },
	{ "get_room",       api_game_get_room },
	{ "cast_spell",     api_game_cast_spell },
	{ "random",         api_game_random },
	{ "hour",           api_game_hour },
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
