/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * A Lua extension for upb.
 */

#include "lauxlib.h"
#include "upb_def.h"

/* lupb_def *******************************************************************/

// All the def types share the same C layout, even though they are differen Lua
// types with different metatables.
typedef struct {
  upb_def *def;
} lupb_def;

static void lupb_pushnewdef(lua_State *L, upb_def *def) {
  lupb_def *ldef = lua_newuserdata(L, sizeof(lupb_def));
  ldef->def = def;
  const char *type_name;
  switch(def->type) {
    case UPB_DEF_MSG:
      type_name = "upb.msgdef";
      break;
    case UPB_DEF_ENUM:
      type_name = "upb.enumdef";
      break;
    default:
      luaL_error(L, "unknown deftype %d", def->type);
  }
  luaL_getmetatable(L, type_name);
  lua_setmetatable(L, -2);
}

static lupb_def *lupb_msgdef_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, "upb.msgdef");
}

static lupb_def *lupb_enumdef_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, "upb.enumdef");
}

static int lupb_msgdef_gc(lua_State *L) {
  lupb_def *ldef = lupb_msgdef_check(L, 1);
  upb_def_unref(ldef->def);
  return 0;
}

static int lupb_enumdef_gc(lua_State *L) {
  lupb_def *ldef = lupb_enumdef_check(L, 1);
  upb_def_unref(ldef->def);
  return 0;
}

static const struct luaL_Reg lupb_msgdef_methods[] = {
  {"__gc", lupb_msgdef_gc},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_enumdef_methods[] = {
  {"__gc", lupb_enumdef_gc},
  {NULL, NULL}
};


/* lupb_symtab ****************************************************************/

// lupb_symtab caches the Lua objects it vends (defs) via lookup or resolve.
// It does this (instead of creating a new Lua object every time) for two
// reasons:
// * it uses less memory, because we can reuse existing objects.
// * it gives the expected equality semantics, eg. symtab[sym] == symtab[sym].
//
// The downside is a bit of complexity.  We need a place to store these
// cached defs; the only good answer is in the metatable.  This means we need
// a new metatable for every symtab instance (instead of one shared by all
// instances).  Since this is different than the regular pattern, we can't
// use luaL_checkudata(), we have to implement it ourselves.
typedef struct {
  upb_symtab *symtab;
} lupb_symtab;

static int lupb_symtab_gc(lua_State *L);

// Inherits a ref on the symtab.
static void lupb_pushnewsymtab(lua_State *L, upb_symtab *symtab) {
  lupb_symtab *lsymtab = lua_newuserdata(L, sizeof(lupb_symtab));
  lsymtab->symtab = symtab;
  // Create its metatable (see note above about mt-per-object).
  lua_createtable(L, 0, 1);
  luaL_getmetatable(L, "upb.symtab");
  lua_setfield(L, -2, "__index");  // Uses the type metatable to find methods.
  lua_pushcfunction(L, lupb_symtab_gc);
  lua_setfield(L, -2, "__gc");

  // Put this metatable in the registry so we can find it for type validation.
  lua_pushlightuserdata(L, lsymtab);
  lua_pushvalue(L, -2);
  lua_rawset(L, LUA_REGISTRYINDEX);

  // Set the symtab's metatable.
  lua_setmetatable(L, -2);
}

// Checks that narg is a proper lupb_symtab object.  If it is, leaves its
// metatable on the stack for cache lookups/updates.
lupb_symtab *lupb_symtab_check(lua_State *L, int narg) {
  lupb_symtab *symtab = lua_touserdata(L, narg);
  if (symtab != NULL) {
    if (lua_getmetatable(L, narg)) {
      // We use a metatable-per-object to support memoization of defs.
      lua_pushlightuserdata(L, symtab);
      lua_rawget(L, LUA_REGISTRYINDEX);
      if (lua_rawequal(L, -1, -2)) {  // Does it have the correct mt?
        lua_pop(L, 1);  // Remove one copy of the mt, keep the other.
        return symtab;
      }
    }
  }
  luaL_typerror(L, narg, "upb.symtab");
  return NULL;  // Placate the compiler; luaL_typerror will longjmp out of here.
}

static int lupb_symtab_gc(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  upb_symtab_unref(s->symtab);

  // Remove its metatable from the registry.
  lua_pushlightuserdata(L, s);
  lua_pushnil(L);
  lua_rawset(L, LUA_REGISTRYINDEX);
  return 0;
}

// "mt" is the index of the metatable, -1 is the fqname of this def.
// Leaves the Lua object for the def at the top of the stack.
// Inherits a ref on "def".
static void lupb_symtab_getorcreate(lua_State *L, upb_def *def, int mt) {
  // We may have this def cached, in which case we should return the same Lua
  // object (as long as the value in the underlying symtab has not changed.
  lua_rawget(L, mt);
  if (!lua_isnil(L, -1)) {
    // Def is cached, make sure it hasn't changed.
    lupb_def *ldef = lua_touserdata(L, -1);
    if (!ldef) luaL_error(L, "upb's internal cache is corrupt.");
    if (ldef->def == def) {
      // Cache is good, we can just return the cached value.
      upb_def_unref(def);
      return;
    }
  }
  // Cached entry didn't exist or wasn't good.
  lua_pop(L, 1);  // Remove bad cached value.
  lupb_pushnewdef(L, def);

  // Set it in the cache.
  lua_pushvalue(L, 2);  // push name (arg to this function).
  lua_pushvalue(L, -2); // push the new def.
  lua_rawset(L, mt);    // set in the cache (the mt).
}

static int lupb_symtab_lookup(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  size_t len;
  const char *name = luaL_checklstring(L, 2, &len);
  upb_string namestr = UPB_STACK_STRING_LEN(name, len);
  upb_def *def = upb_symtab_lookup(s->symtab, &namestr);
  if (!def) {
    // There shouldn't be a value in our cache either because the symtab
    // currently provides no API for deleting syms from a table.  In case
    // this changes in the future, we explicitly delete from the cache here.
    lua_pushvalue(L, 2);  // push name (arg to this function).
    lua_pushnil(L);
    lua_rawset(L, -3);  // lupb_symtab_check() left our mt on the stack.

    // Return nil because the symbol was not found.
    lua_pushnil(L);
    return 1;
  } else {
    lua_pushvalue(L, 2);
    lupb_symtab_getorcreate(L, def, 3);
    return 1;
  }
}

static int lupb_symtab_getdefs(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  upb_deftype_t type = luaL_checkint(L, 2);
  int count;
  upb_def **defs = upb_symtab_getdefs(s->symtab, &count, type);

  // Create the table in which we will return the defs.
  lua_createtable(L, 0, count);
  int ret = lua_gettop(L);

  for (int i = 0; i < count; i++) {
    upb_def *def = defs[i];
    // Look it up in the cache by name.
    upb_string *name = def->fqname;
    lua_pushlstring(L, upb_string_getrobuf(name), upb_string_len(name));
    lua_pushvalue(L, -1);  // Push it again since the getorcreate consumes one.
    lupb_symtab_getorcreate(L, def, 3);

    // Add it to our return table.
    lua_settable(L, ret);
  }
  return 1;
}

static int lupb_symtab_add_descriptorproto(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  upb_symtab_add_descriptorproto(s->symtab);
  return 0;  // No args to return.
}

static const struct luaL_Reg lupb_symtab_methods[] = {
  {"add_descriptorproto", lupb_symtab_add_descriptorproto},
  //{"addfds", lupb_symtab_addfds},
  {"getdefs", lupb_symtab_getdefs},
  {"lookup", lupb_symtab_lookup},
  //{"resolve", lupb_symtab_resolve},
  {NULL, NULL}
};


/* lupb toplevel **************************************************************/

static int lupb_symtab_new(lua_State *L) {
  upb_symtab *s = upb_symtab_new();
  lupb_pushnewsymtab(L, s);
  return 1;
}

static const struct luaL_Reg lupb_toplevel_methods[] = {
  {"symtab", lupb_symtab_new},
  {NULL, NULL}
};

int luaopen_upb(lua_State *L) {
  luaL_newmetatable(L, "upb.msgdef");
  luaL_register(L, NULL, lupb_msgdef_methods);

  luaL_newmetatable(L, "upb.enumdef");
  luaL_register(L, NULL, lupb_enumdef_methods);

  luaL_newmetatable(L, "upb.symtab");
  luaL_register(L, NULL, lupb_symtab_methods);

  luaL_register(L, "upb", lupb_toplevel_methods);
  return 1;  // Return package table.
}
