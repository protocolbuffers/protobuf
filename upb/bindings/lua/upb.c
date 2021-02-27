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

#include "upb/bindings/lua/upb.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "upb/msg.h"

/* Lua compatibility code *****************************************************/

/* Shims for upcoming Lua 5.3 functionality. */
static bool lua_isinteger(lua_State *L, int argn) {
  LUPB_UNUSED(L);
  LUPB_UNUSED(argn);
  return false;
}


/* Utility functions **********************************************************/

void lupb_checkstatus(lua_State *L, upb_status *s) {
  if (!upb_ok(s)) {
    lua_pushstring(L, upb_status_errmsg(s));
    lua_error(L);
  }
}

/* Pushes a new userdata with the given metatable. */
void *lupb_newuserdata(lua_State *L, size_t size, int n, const char *type) {
#if LUA_VERSION_NUM >= 504
  void *ret = lua_newuserdatauv(L, size, n);
#else
  void *ret = lua_newuserdata(L, size);
  lua_createtable(L, 0, n);
  lua_setuservalue(L, -2);
#endif

  /* Set metatable. */
  luaL_getmetatable(L, type);
  assert(!lua_isnil(L, -1));  /* Should have been created by luaopen_upb. */
  lua_setmetatable(L, -2);

  return ret;
}

#if LUA_VERSION_NUM < 504
int lua_setiuservalue(lua_State *L, int index, int n) {
  lua_getuservalue(L, index);
  lua_insert(L, -2);
  lua_rawseti(L, -2, n);
  lua_pop(L, 1);
  return 1;
}

int lua_getiuservalue(lua_State *L, int index, int n) {
  lua_getuservalue(L, index);
  lua_rawgeti(L, -1, n);
  lua_replace(L, -2);
  return 1;
}
#endif

/* We use this function as the __index metamethod when a type has both methods
 * and an __index metamethod. */
int lupb_indexmm(lua_State *L) {
  /* Look up in __index table (which is a closure param). */
  lua_pushvalue(L, 2);
  lua_rawget(L, lua_upvalueindex(1));
  if (!lua_isnil(L, -1)) {
    return 1;
  }

  /* Not found, chain to user __index metamethod. */
  lua_pushvalue(L, lua_upvalueindex(2));
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_call(L, 2, 1);
  return 1;
}

void lupb_register_type(lua_State *L, const char *name, const luaL_Reg *m,
                        const luaL_Reg *mm) {
  luaL_newmetatable(L, name);

  if (mm) {
    lupb_setfuncs(L, mm);
  }

  if (m) {
    lua_createtable(L, 0, 0);  /* __index table */
    lupb_setfuncs(L, m);

    /* Methods go in the mt's __index slot.  If the user also specified an
     * __index metamethod, use our custom lupb_indexmm() that can check both. */
    lua_getfield(L, -2, "__index");
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
    } else {
      lua_pushcclosure(L, &lupb_indexmm, 2);
    }
    lua_setfield(L, -2, "__index");
  }

  lua_pop(L, 1);  /* The mt. */
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

/* Library entry point ********************************************************/

int luaopen_lupb(lua_State *L) {
#if LUA_VERSION_NUM == 501
  const struct luaL_Reg funcs[] = {{NULL, NULL}};
  luaL_register(L, "upb_c", funcs);
#else
  lua_createtable(L, 0, 8);
#endif
  lupb_def_registertypes(L);
  lupb_msg_registertypes(L);
  return 1;  /* Return package table. */
}
