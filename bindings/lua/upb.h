/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Shared definitions for upb Lua modules.
 */

#ifndef UPB_LUA_UPB_H_
#define UPB_LUA_UPB_H_

#include "upb/def.h"

// Lua 5.1/5.2 compatibility code.
#if LUA_VERSION_NUM == 501

#define lua_rawlen lua_objlen
#define lupb_newlib(L, name, l) luaL_register(L, name, l)
#define lupb_setfuncs(L, l) luaL_register(L, NULL, l)
#define LUPB_OPENFUNC(mod) luaopen_ ## mod ## upb5_1

void *luaL_testudata(lua_State *L, int ud, const char *tname);

#elif LUA_VERSION_NUM == 502

// Lua 5.2 modules are not expected to set a global variable, so "name" is
// unused.
#define lupb_newlib(L, name, l) luaL_newlib(L, l)
#define lupb_setfuncs(L, l) luaL_setfuncs(L, l, 0)
int luaL_typerror(lua_State *L, int narg, const char *tname);
#define LUPB_OPENFUNC(mod) luaopen_ ## mod ## upb5_2

#else
#error Only Lua 5.1 and 5.2 are supported
#endif

const upb_msgdef *lupb_msgdef_check(lua_State *L, int narg);
const upb_enumdef *lupb_enumdef_check(lua_State *L, int narg);
const char *lupb_checkname(lua_State *L, int narg);
bool lupb_def_pushwrapper(lua_State *L, const upb_def *def, const void *owner);
void lupb_def_pushnewrapper(lua_State *L, const upb_def *def,
                            const void *owner);

#endif  // UPB_LUA_UPB_H_
