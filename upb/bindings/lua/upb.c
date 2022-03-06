/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * require("lua") -- A Lua extension for upb.
 *
 * Exposes only the core library
 * (sub-libraries are exposed in other extensions).
 *
 * 64-bit woes: Lua can only represent numbers of type lua_Number (which is
 * double unless the user specifically overrides this).  Doubles can represent
 * the entire range of 64-bit integers, but lose precision once the integers are
 * greater than 2^53.
 *
 * Lua 5.3 is adding support for integers, which will allow for 64-bit
 * integers (which can be interpreted as signed or unsigned).
 *
 * LuaJIT supports 64-bit signed and unsigned boxed representations
 * through its "cdata" mechanism, but this is not portable to regular Lua.
 *
 * Hopefully Lua 5.3 will come soon enough that we can either use Lua 5.3
 * integer support or LuaJIT 64-bit cdata for users that need the entire
 * domain of [u]int64 values.
 */

#include "upb/bindings/lua/upb.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "upb/msg.h"

/* Lua compatibility code *****************************************************/

/* Shims for upcoming Lua 5.3 functionality. */
static bool lua_isinteger(lua_State* L, int argn) {
  LUPB_UNUSED(L);
  LUPB_UNUSED(argn);
  return false;
}

/* Utility functions **********************************************************/

void lupb_checkstatus(lua_State* L, upb_Status* s) {
  if (!upb_Status_IsOk(s)) {
    lua_pushstring(L, upb_Status_ErrorMessage(s));
    lua_error(L);
  }
}

/* Pushes a new userdata with the given metatable. */
void* lupb_newuserdata(lua_State* L, size_t size, int n, const char* type) {
#if LUA_VERSION_NUM >= 504
  void* ret = lua_newuserdatauv(L, size, n);
#else
  void* ret = lua_newuserdata(L, size);
  lua_createtable(L, 0, n);
  lua_setuservalue(L, -2);
#endif

  /* Set metatable. */
  luaL_getmetatable(L, type);
  assert(!lua_isnil(L, -1)); /* Should have been created by luaopen_upb. */
  lua_setmetatable(L, -2);

  return ret;
}

#if LUA_VERSION_NUM < 504
int lua_setiuservalue(lua_State* L, int index, int n) {
  lua_getuservalue(L, index);
  lua_insert(L, -2);
  lua_rawseti(L, -2, n);
  lua_pop(L, 1);
  return 1;
}

int lua_getiuservalue(lua_State* L, int index, int n) {
  lua_getuservalue(L, index);
  lua_rawgeti(L, -1, n);
  lua_replace(L, -2);
  return 1;
}
#endif

/* We use this function as the __index metamethod when a type has both methods
 * and an __index metamethod. */
int lupb_indexmm(lua_State* L) {
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

void lupb_register_type(lua_State* L, const char* name, const luaL_Reg* m,
                        const luaL_Reg* mm) {
  luaL_newmetatable(L, name);

  if (mm) {
    lupb_setfuncs(L, mm);
  }

  if (m) {
    lua_createtable(L, 0, 0); /* __index table */
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

  lua_pop(L, 1); /* The mt. */
}

/* Scalar type mapping ********************************************************/

/* Functions that convert scalar/primitive values (numbers, strings, bool)
 * between Lua and C/upb.  Handles type/range checking. */

bool lupb_checkbool(lua_State* L, int narg) {
  if (!lua_isboolean(L, narg)) {
    luaL_error(L, "must be true or false");
  }
  return lua_toboolean(L, narg);
}

/* Unlike luaL_checkstring(), this does not allow implicit conversion to
 * string. */
const char* lupb_checkstring(lua_State* L, int narg, size_t* len) {
  if (lua_type(L, narg) != LUA_TSTRING) {
    luaL_error(L, "Expected string");
  }

  return lua_tolstring(L, narg, len);
}

/* Unlike luaL_checkinteger, these do not implicitly convert from string or
 * round an existing double value.  We allow floating-point input, but only if
 * the actual value is integral. */
#define INTCHECK(type, ctype, min, max)                                        \
  ctype lupb_check##type(lua_State* L, int narg) {                             \
    double n;                                                                  \
    if (lua_isinteger(L, narg)) {                                              \
      return lua_tointeger(L, narg);                                           \
    }                                                                          \
                                                                               \
    /* Prevent implicit conversion from string. */                             \
    luaL_checktype(L, narg, LUA_TNUMBER);                                      \
    n = lua_tonumber(L, narg);                                                 \
                                                                               \
    /* Check this double has no fractional part and remains in bounds.         \
     * Consider INT64_MIN and INT64_MAX:                                       \
     * 1. INT64_MIN -(2^63) is a power of 2, so this converts to a double.     \
     * 2. INT64_MAX (2^63 - 1) is not a power of 2, and conversion of          \
     * out-of-range integer values to a double can lead to undefined behavior. \
     * On some compilers, this conversion can return 0, but it also can return \
     * the max value. To deal with this, we can first divide by 2 to prevent   \
     * the overflow, multiply it back, and add 1 to find the true limit. */    \
    double i;                                                                  \
    double max_value = (((double)max / 2) * 2) + 1;                            \
    if ((modf(n, &i) != 0.0) || n < min || n >= max_value) {                   \
      luaL_error(L, "number %f was not an integer or out of range for " #type, \
                 n);                                                           \
    }                                                                          \
    return (ctype)n;                                                           \
  }                                                                            \
  void lupb_push##type(lua_State* L, ctype val) {                              \
    /* TODO: push integer for Lua >= 5.3, 64-bit cdata for LuaJIT. */          \
    /* This is lossy for some [u]int64 values, which isn't great, but */       \
    /* crashing when we encounter these values seems worse. */                 \
    lua_pushnumber(L, val);                                                    \
  }

INTCHECK(int64, int64_t, INT64_MIN, INT64_MAX)
INTCHECK(int32, int32_t, INT32_MIN, INT32_MAX)
INTCHECK(uint64, uint64_t, 0, UINT64_MAX)
INTCHECK(uint32, uint32_t, 0, UINT32_MAX)

double lupb_checkdouble(lua_State* L, int narg) {
  /* If we were being really hard-nosed here, we'd check whether the input was
   * an integer that has no precise double representation.  But doubles aren't
   * generally expected to be exact like integers are, and worse this could
   * cause data-dependent runtime errors: one run of the program could work fine
   * because the integer calculations happened to be exactly representable in
   * double, while the next could crash because of subtly different input. */

  luaL_checktype(L, narg, LUA_TNUMBER); /* lua_tonumber() auto-converts. */
  return lua_tonumber(L, narg);
}

float lupb_checkfloat(lua_State* L, int narg) {
  /* We don't worry about checking whether the input can be exactly converted to
   * float -- see above. */

  luaL_checktype(L, narg, LUA_TNUMBER); /* lua_tonumber() auto-converts. */
  return lua_tonumber(L, narg);
}

void lupb_pushdouble(lua_State* L, double d) { lua_pushnumber(L, d); }

void lupb_pushfloat(lua_State* L, float d) { lua_pushnumber(L, d); }

/* Library entry point ********************************************************/

int luaopen_lupb(lua_State* L) {
#if LUA_VERSION_NUM == 501
  const struct luaL_Reg funcs[] = {{NULL, NULL}};
  luaL_register(L, "upb_c", funcs);
#else
  lua_createtable(L, 0, 8);
#endif
  lupb_def_registertypes(L);
  lupb_msg_registertypes(L);
  return 1; /* Return package table. */
}
