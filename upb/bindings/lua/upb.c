/*
** require("lua") -- A Lua extension for upb.
**
** Exposes only the core library
** (sub-libraries are exposed in other extensions).
**
** 64-bit woes: Lua can only represent numbers of type lua_Number (which is
** double unless the user specifically overrides this).  Doubles can represent
** the entire range of 64-bit integers, but lose precision once the integers are
** greater than 2^53.
**
** Lua 5.3 is adding support for integers, which will allow for 64-bit
** integers (which can be interpreted as signed or unsigned).
**
** LuaJIT supports 64-bit signed and unsigned boxed representations
** through its "cdata" mechanism, but this is not portable to regular Lua.
**
** Hopefully Lua 5.3 will come soon enough that we can either use Lua 5.3
** integer support or LuaJIT 64-bit cdata for users that need the entire
** domain of [u]int64 values.
*/

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "lauxlib.h"
#include "upb/bindings/lua/upb.h"
#include "upb/handlers.h"
#include "upb/msg.h"


/* Lua compatibility code *****************************************************/

/* Lua 5.1 and Lua 5.2 have slightly incompatible APIs.  A little bit of
 * compatibility code can help hide the difference.  Not too many people still
 * use Lua 5.1 but LuaJIT uses the Lua 5.1 API in some ways. */

#if LUA_VERSION_NUM == 501

/* taken from lua 5.2's source. */
void *luaL_testudata(lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      luaL_getmetatable(L, tname);  /* get correct metatable */
      if (!lua_rawequal(L, -1, -2))  /* not the same? */
        p = NULL;  /* value is a userdata with wrong metatable */
      lua_pop(L, 2);  /* remove both metatables */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}

static void lupb_newlib(lua_State *L, const char *name, const luaL_Reg *funcs) {
  luaL_register(L, name, funcs);
}

#elif LUA_VERSION_NUM == 502

int luaL_typerror(lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}

static void lupb_newlib(lua_State *L, const char *name, const luaL_Reg *funcs) {
  /* Lua 5.2 modules are not expected to set a global variable, so "name" is
   * unused. */
  UPB_UNUSED(name);

  /* Can't use luaL_newlib(), because funcs is not the actual array.
   * Could (micro-)optimize this a bit to count funcs for initial table size. */
  lua_createtable(L, 0, 8);
  luaL_setfuncs(L, funcs, 0);
}

#else
#error Only Lua 5.1 and 5.2 are supported
#endif

/* Shims for upcoming Lua 5.3 functionality. */
bool lua_isinteger(lua_State *L, int argn) {
  UPB_UNUSED(L);
  UPB_UNUSED(argn);
  return false;
}


/* Utility functions **********************************************************/

/* We store our module table in the registry, keyed by ptr.
 * For more info about the motivation/rationale, see this thread:
 *   http://thread.gmane.org/gmane.comp.lang.lua.general/110632 */
bool lupb_openlib(lua_State *L, void *ptr, const char *name,
                  const luaL_Reg *funcs) {
  /* Lookup cached module table. */
  lua_pushlightuserdata(L, ptr);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (!lua_isnil(L, -1)) {
    return true;
  }

  lupb_newlib(L, name, funcs);

  /* Save module table in cache. */
  lua_pushlightuserdata(L, ptr);
  lua_pushvalue(L, -2);
  lua_rawset(L, LUA_REGISTRYINDEX);

  return false;
}

void lupb_checkstatus(lua_State *L, upb_status *s) {
  if (!upb_ok(s)) {
    lua_pushstring(L, upb_status_errmsg(s));
    lua_error(L);
  }
}

/* Scalar type mapping ********************************************************/

/* Functions that convert scalar/primitive values (numbers, strings, bool)
 * between Lua and C/upb.  Handles type/range checking. */

bool lupb_checkbool(lua_State *L, int narg) {
  if (!lua_isboolean(L, narg)) {
    luaL_error(L, "must be true or false");
  }
  return lua_toboolean(L, narg);
}

/* Unlike luaL_checkstring(), this does not allow implicit conversion to
 * string. */
const char *lupb_checkstring(lua_State *L, int narg, size_t *len) {
  if (lua_type(L, narg) != LUA_TSTRING) {
    luaL_error(L, "Expected string");
  }

  return lua_tolstring(L, narg, len);
}

/* Unlike luaL_checkinteger, these do not implicitly convert from string or
 * round an existing double value.  We allow floating-point input, but only if
 * the actual value is integral. */
#define INTCHECK(type, ctype)                                                  \
  ctype lupb_check##type(lua_State *L, int narg) {                             \
    double n;                                                                  \
    ctype i;                                                                   \
    if (lua_isinteger(L, narg)) {                                              \
      return lua_tointeger(L, narg);                                           \
    }                                                                          \
                                                                               \
    /* Prevent implicit conversion from string. */                             \
    luaL_checktype(L, narg, LUA_TNUMBER);                                      \
    n = lua_tonumber(L, narg);                                                 \
                                                                               \
    i = (ctype)n;                                                              \
    if ((double)i != n) {                                                      \
      /* double -> ctype truncated or rounded. */                              \
      luaL_error(L, "number %f was not an integer or out of range for " #type, \
                 n);                                                           \
    }                                                                          \
    return i;                                                                  \
  }                                                                            \
  void lupb_push##type(lua_State *L, ctype val) {                              \
    /* TODO: push integer for Lua >= 5.3, 64-bit cdata for LuaJIT. */          \
    /* This is lossy for some [u]int64 values, which isn't great, but */       \
    /* crashing when we encounter these values seems worse. */                 \
    lua_pushnumber(L, val);                                                    \
  }

INTCHECK(int64,  int64_t)
INTCHECK(int32,  int32_t)
INTCHECK(uint64, uint64_t)
INTCHECK(uint32, uint32_t)

double lupb_checkdouble(lua_State *L, int narg) {
  /* If we were being really hard-nosed here, we'd check whether the input was
   * an integer that has no precise double representation.  But doubles aren't
   * generally expected to be exact like integers are, and worse this could
   * cause data-dependent runtime errors: one run of the program could work fine
   * because the integer calculations happened to be exactly representable in
   * double, while the next could crash because of subtly different input. */

  luaL_checktype(L, narg, LUA_TNUMBER);  /* lua_tonumber() auto-converts. */
  return lua_tonumber(L, narg);
}

float lupb_checkfloat(lua_State *L, int narg) {
  /* We don't worry about checking whether the input can be exactly converted to
   * float -- see above. */

  luaL_checktype(L, narg, LUA_TNUMBER);  /* lua_tonumber() auto-converts. */
  return lua_tonumber(L, narg);
}

void lupb_pushdouble(lua_State *L, double d) {
  lua_pushnumber(L, d);
}

void lupb_pushfloat(lua_State *L, float d) {
  lua_pushnumber(L, d);
}


static const struct luaL_Reg lupb_toplevel_m[] = {
  {NULL, NULL}
};

void lupb_register_type(lua_State *L, const char *name, const luaL_Reg *m,
                        const luaL_Reg *mm) {
  luaL_newmetatable(L, name);

  if (mm) {
    lupb_setfuncs(L, mm);
  }

  if (m) {
    /* Methods go in the mt's __index method.  This implies that you can'
     * implement __index and also have methods. */
    lua_getfield(L, -1, "__index");
    lupb_assert(L, lua_isnil(L, -1));
    lua_pop(L, 1);

    lua_createtable(L, 0, 0);
    lupb_setfuncs(L, m);
    lua_setfield(L, -2, "__index");
  }

  lua_pop(L, 1);  /* The mt. */
}

int luaopen_upb_c(lua_State *L) {
  static char module_key;
  if (lupb_openlib(L, &module_key, "upb_c", lupb_toplevel_m)) {
    return 1;
  }

  lupb_def_registertypes(L);
  lupb_msg_registertypes(L);

  return 1;  /* Return package table. */
}
