/*
 * Lua 5.4 amalgamation — compile all of Lua as a single compilation unit.
 * Follows the same pattern as sqlite3.c in game/src/db/.
 *
 * Excludes lua.c (standalone interpreter) and luac.c (standalone compiler)
 * since they define their own main() functions.
 *
 * Build with warnings suppressed (-w on GCC, TurnOffAllWarnings on MSVC).
 */

/* Core */
#include "../../lib/lua/lapi.c"
#include "../../lib/lua/lctype.c"
#include "../../lib/lua/ldebug.c"
#include "../../lib/lua/ldo.c"
#include "../../lib/lua/ldump.c"
#include "../../lib/lua/lfunc.c"
#include "../../lib/lua/lgc.c"
#include "../../lib/lua/llex.c"
#include "../../lib/lua/lmem.c"
#include "../../lib/lua/lobject.c"
#include "../../lib/lua/lopcodes.c"
#include "../../lib/lua/lparser.c"
#include "../../lib/lua/lstate.c"
#include "../../lib/lua/lstring.c"
#include "../../lib/lua/ltable.c"
#include "../../lib/lua/ltm.c"
#include "../../lib/lua/lundump.c"
#include "../../lib/lua/lvm.c"
#include "../../lib/lua/lzio.c"
#include "../../lib/lua/lcode.c"

/* Auxiliary library */
#include "../../lib/lua/lauxlib.c"

/* Standard libraries */
#include "../../lib/lua/lbaselib.c"
#include "../../lib/lua/lcorolib.c"
#include "../../lib/lua/ldblib.c"
#include "../../lib/lua/linit.c"
#include "../../lib/lua/liolib.c"
#include "../../lib/lua/lmathlib.c"
#include "../../lib/lua/loadlib.c"
#include "../../lib/lua/loslib.c"
#include "../../lib/lua/lstrlib.c"
#include "../../lib/lua/ltablib.c"
#include "../../lib/lua/lutf8lib.c"
