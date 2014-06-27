/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A Lua extension for upb/descriptor.
 */

#include "upb/bindings/lua/upb.h"

static const struct luaL_Reg toplevel_m[] = {
  {NULL, NULL}
};

int luaopen_upb_descriptor(lua_State *L) {
  lupb_newlib(L, "upb.descriptor", toplevel_m);

  return 1;  // Return package table.
}
