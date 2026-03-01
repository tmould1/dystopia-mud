/*
 * script_lua.c — Lua state management and sandbox for MUD scripting.
 *
 * Manages a single global lua_State used by all scripts. Provides
 * sandbox setup (remove dangerous libraries), instruction-count
 * timeout protection, and the core script execution function.
 */

#include "merc.h"
#include "script.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

/* Maximum Lua instructions before a script is killed */
#define SCRIPT_MAX_INSTRUCTIONS  100000

/* Forward declarations for script_api.c */
void script_register_char_api( lua_State *L );
void script_register_room_api( lua_State *L );
void script_register_obj_api( lua_State *L );

/* Global Lua state */
static lua_State *g_lua = NULL;


/*
 * Instruction count hook — kills runaway scripts.
 */
static void script_timeout_hook( lua_State *L, lua_Debug *ar ) {
	(void) ar;
	luaL_error( L, "script exceeded instruction limit (%d)",
		SCRIPT_MAX_INSTRUCTIONS );
}


/*
 * Remove dangerous globals from the Lua environment.
 */
static void script_sandbox( lua_State *L ) {
	const char *blocked[] = {
		"io", "os", "debug", "loadfile", "dofile", "require",
		"collectgarbage", "rawset", "rawget", "rawequal", "rawlen",
		NULL
	};
	int i;

	for ( i = 0; blocked[i] != NULL; i++ ) {
		lua_pushnil( L );
		lua_setglobal( L, blocked[i] );
	}

	/* Set instruction count hook */
	lua_sethook( L, script_timeout_hook, LUA_MASKCOUNT,
		SCRIPT_MAX_INSTRUCTIONS );
}


/*
 * Initialize the Lua scripting engine.
 * Called from boot_db() during server startup.
 */
void script_init( void ) {
	if ( g_lua != NULL )
		return;

	g_lua = luaL_newstate();
	if ( g_lua == NULL ) {
		bug( "script_init: failed to create Lua state", 0 );
		return;
	}

	/* Open safe standard libraries */
	luaL_requiref( g_lua, "_G",        luaopen_base,      1 );  lua_pop( g_lua, 1 );
	luaL_requiref( g_lua, "string",    luaopen_string,    1 );  lua_pop( g_lua, 1 );
	luaL_requiref( g_lua, "table",     luaopen_table,     1 );  lua_pop( g_lua, 1 );
	luaL_requiref( g_lua, "math",      luaopen_math,      1 );  lua_pop( g_lua, 1 );
	luaL_requiref( g_lua, "utf8",      luaopen_utf8,      1 );  lua_pop( g_lua, 1 );

	/* Lock down the environment */
	script_sandbox( g_lua );

	/* Register C API for Lua scripts */
	script_register_char_api( g_lua );
	script_register_room_api( g_lua );
	script_register_obj_api( g_lua );

	log_string( "Lua scripting engine initialized (Lua 5.4)." );
}


/*
 * Shut down the Lua scripting engine.
 */
void script_shutdown( void ) {
	if ( g_lua != NULL ) {
		lua_close( g_lua );
		g_lua = NULL;
		log_string( "Lua scripting engine shut down." );
	}
}


/*
 * Execute a script's Lua code, calling the named function with arguments.
 *
 * The script's code is loaded (defining functions), then the specified
 * callback function is called.  Any errors are logged and swallowed —
 * a broken script must never crash the game.
 *
 * Parameters:
 *   script   - The script data (contains Lua source code)
 *   func     - Name of the Lua function to call (e.g. "on_greet")
 *   mob      - The NPC that owns the script (pushed as first arg)
 *   ch       - The player that triggered the event (second arg)
 *   text     - Optional text argument for speech triggers (NULL if none)
 */
void script_run( SCRIPT_DATA *script, const char *func,
	CHAR_DATA *mob, CHAR_DATA *ch, const char *text ) {
	char buf[MAX_STRING_LENGTH];
	int nargs;
	int top;

	if ( g_lua == NULL || script == NULL || script->code == NULL )
		return;

	top = lua_gettop( g_lua );

	/* Load and execute the script code to define its functions */
	if ( luaL_dostring( g_lua, script->code ) != LUA_OK ) {
		const char *err = lua_tostring( g_lua, -1 );
		snprintf( buf, sizeof( buf ),
			"script_run: load error in '%s'", script->name );
		bug( buf, 0 );
		if ( err )
			log_string( err );
		lua_settop( g_lua, top );
		return;
	}

	/* Look up the callback function */
	lua_getglobal( g_lua, func );
	if ( !lua_isfunction( g_lua, -1 ) ) {
		/* Function not defined — not an error, script may only handle
		 * some trigger types */
		lua_settop( g_lua, top );
		return;
	}

	/* Push arguments: mob, ch, [text] */
	lua_pushlightuserdata( g_lua, mob );
	luaL_getmetatable( g_lua, "Char" );
	lua_setmetatable( g_lua, -2 );

	lua_pushlightuserdata( g_lua, ch );
	luaL_getmetatable( g_lua, "Char" );
	lua_setmetatable( g_lua, -2 );

	nargs = 2;

	if ( text != NULL ) {
		lua_pushstring( g_lua, text );
		nargs = 3;
	}

	/* Call the function with error handling */
	if ( lua_pcall( g_lua, nargs, 0, 0 ) != LUA_OK ) {
		const char *err = lua_tostring( g_lua, -1 );
		snprintf( buf, sizeof( buf ),
			"script_run: runtime error in '%s.%s'", func, script->name );
		bug( buf, 0 );
		if ( err )
			log_string( err );
	}

	/* Clean up the stack */
	lua_settop( g_lua, top );
}


/*
 * Execute a room script's Lua code.
 * Pushes (ch, room, [text]) instead of (mob, ch, [text]).
 */
void script_run_room( SCRIPT_DATA *script, const char *func,
	CHAR_DATA *ch, ROOM_INDEX_DATA *room, const char *text ) {
	char buf[MAX_STRING_LENGTH];
	int nargs;
	int top;

	if ( g_lua == NULL || script == NULL || script->code == NULL )
		return;

	top = lua_gettop( g_lua );

	if ( luaL_dostring( g_lua, script->code ) != LUA_OK ) {
		const char *err = lua_tostring( g_lua, -1 );
		snprintf( buf, sizeof( buf ),
			"script_run_room: load error in '%s'", script->name );
		bug( buf, 0 );
		if ( err )
			log_string( err );
		lua_settop( g_lua, top );
		return;
	}

	lua_getglobal( g_lua, func );
	if ( !lua_isfunction( g_lua, -1 ) ) {
		lua_settop( g_lua, top );
		return;
	}

	/* Push arguments: ch, room, [text] */
	lua_pushlightuserdata( g_lua, ch );
	luaL_getmetatable( g_lua, "Char" );
	lua_setmetatable( g_lua, -2 );

	lua_pushlightuserdata( g_lua, room );
	luaL_getmetatable( g_lua, "Room" );
	lua_setmetatable( g_lua, -2 );

	nargs = 2;

	if ( text != NULL ) {
		lua_pushstring( g_lua, text );
		nargs = 3;
	}

	if ( lua_pcall( g_lua, nargs, 0, 0 ) != LUA_OK ) {
		const char *err = lua_tostring( g_lua, -1 );
		snprintf( buf, sizeof( buf ),
			"script_run_room: runtime error in '%s.%s'", func, script->name );
		bug( buf, 0 );
		if ( err )
			log_string( err );
	}

	lua_settop( g_lua, top );
}
