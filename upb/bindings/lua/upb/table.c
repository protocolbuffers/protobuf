/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Lua extension that provides access to upb_table.  This is an internal-only
 * interface and exists for the sole purpose of writing a C code generator in
 * Lua that can dump a upb_table as static C initializers.  This lets us use
 * Lua for convenient string manipulation while saving us from re-implementing
 * the upb_table hash function and hash table layout / collision strategy in
 * Lua.
 *
 * Since this is used only as part of the toolchain (and not part of the
 * runtime) we do not hold this module to the same stringent requirements as
 * the main Lua modules (for example that misbehaving Lua programs cannot
 * crash the interpreter).
 */

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "upb/bindings/lua/upb.h"
#include "upb/def.h"
#include "upb/symtab.h"

static void lupbtable_setnum(lua_State *L, int tab, const char *key,
                             lua_Number val) {
  lua_pushnumber(L, val);
  lua_setfield(L, tab - 1, key);
}

static void lupbtable_pushval(lua_State *L, _upb_value val, upb_ctype_t ctype) {
  switch (ctype) {
    case UPB_CTYPE_INT32:
      lua_pushnumber(L, val.int32);
      break;
    case UPB_CTYPE_PTR:
      lupb_def_pushwrapper(L, val.ptr, NULL);
      break;
    case UPB_CTYPE_CSTR:
      lua_pushstring(L, val.cstr);
      break;
    default:
      luaL_error(L, "Unexpected type: %d", ctype);
  }
}

// Sets a few fields common to both hash table entries and arrays.
static void lupbtable_setmetafields(lua_State *L, int ctype, const void *ptr) {
  // We tack this onto every entry so we know it even if the entries
  // don't stay with the table.
  lua_pushnumber(L, ctype);
  lua_setfield(L, -2, "valtype");

  // Set this to facilitate linking.
  lua_pushlightuserdata(L, (void*)ptr);
  lua_setfield(L, -2, "ptr");
}

static void lupbtable_pushent(lua_State *L, const upb_tabent *e,
                              bool inttab, int ctype) {
  lua_newtable(L);
  if (!upb_tabent_isempty(e)) {
    if (inttab) {
      lua_pushnumber(L, e->key.num);
    } else {
      lua_pushlstring(L, e->key.s.str, e->key.s.length);
    }
    lua_setfield(L, -2, "key");
    lupbtable_pushval(L, e->val, ctype);
    lua_setfield(L, -2, "value");
  }
  lua_pushlightuserdata(L, (void*)e->next);
  lua_setfield(L, -2, "next");
  lupbtable_setmetafields(L, ctype, e);
}

// Dumps the shared part of upb_table into a Lua table.
static void lupbtable_pushtable(lua_State *L, const upb_table *t, bool inttab) {
  lua_newtable(L);
  lupbtable_setnum(L, -1, "count", t->count);
  lupbtable_setnum(L, -1, "mask",  t->mask);
  lupbtable_setnum(L, -1, "ctype", t->ctype);
  lupbtable_setnum(L, -1, "size_lg2",  t->size_lg2);

  lua_newtable(L);
  for (size_t i = 0; i < upb_table_size(t); i++) {
    lupbtable_pushent(L, &t->entries[i], inttab, t->ctype);
    lua_rawseti(L, -2, i + 1);
  }
  lua_setfield(L, -2, "entries");
}

// Dumps a upb_inttable to a Lua table.
static void lupbtable_pushinttable(lua_State *L, const upb_inttable *t) {
  lupbtable_pushtable(L, &t->t, true);
  lupbtable_setnum(L, -1, "array_size", t->array_size);
  lupbtable_setnum(L, -1, "array_count", t->array_count);

  lua_newtable(L);
  for (size_t i = 0; i < t->array_size; i++) {
    lua_newtable(L);
    if (upb_arrhas(t->array[i])) {
      lupbtable_pushval(L, t->array[i], t->t.ctype);
      lua_setfield(L, -2, "val");
    }
    lupbtable_setmetafields(L, t->t.ctype, &t->array[i]);
    lua_rawseti(L, -2, i + 1);
  }
  lua_setfield(L, -2, "array");
}

static void lupbtable_pushstrtable(lua_State *L, const upb_strtable *t) {
  lupbtable_pushtable(L, &t->t, false);
}

static int lupbtable_msgdef_itof(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lupbtable_pushinttable(L, &m->itof);
  return 1;
}

static int lupbtable_msgdef_ntof(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lupbtable_pushstrtable(L, &m->ntof);
  return 1;
}

static int lupbtable_enumdef_iton(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);
  lupbtable_pushinttable(L, &e->iton);
  return 1;
}

static int lupbtable_enumdef_ntoi(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);
  lupbtable_pushstrtable(L, &e->ntoi);
  return 1;
}

static int lupbtable_symtab_symtab(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  lupbtable_pushstrtable(L, &s->symtab);
  return 1;
}

static void lupbtable_setfieldi(lua_State *L, const char *field, int i) {
  lua_pushnumber(L, i);
  lua_setfield(L, -2, field);
}

static const struct luaL_Reg lupbtable_toplevel_m[] = {
  {"msgdef_itof", lupbtable_msgdef_itof},
  {"msgdef_ntof", lupbtable_msgdef_ntof},
  {"enumdef_iton", lupbtable_enumdef_iton},
  {"enumdef_ntoi", lupbtable_enumdef_ntoi},
  {"symtab_symtab", lupbtable_symtab_symtab},
  {NULL, NULL}
};

int luaopen_upb_table_c(lua_State *L) {
  static char module_key;
  if (lupb_openlib(L, &module_key, "upb.table", lupbtable_toplevel_m)) {
    return 1;
  }

  // We define these here because they are not public.
  lupbtable_setfieldi(L, "CTYPE_PTR",   UPB_CTYPE_PTR);
  lupbtable_setfieldi(L, "CTYPE_CSTR",  UPB_CTYPE_CSTR);
  lupbtable_setfieldi(L, "CTYPE_INT32", UPB_CTYPE_INT32);

  lua_pushlightuserdata(L, NULL);
  lua_setfield(L, -2, "NULL");

  return 1;  // Return a single Lua value, the package table created above.
}
