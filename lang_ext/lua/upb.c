/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A Lua extension for upb.
 */

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "lauxlib.h"
#include "upb/def.h"
#include "upb/msg.h"
#include "upb/pb/glue.h"

//static void lupb_msg_getorcreate(lua_State *L, upb_msg *msg, upb_msgdef *md);

// All the def types share the same C layout, even though they are different Lua
// types with different metatables.
typedef struct {
  upb_def *def;
} lupb_def;

void lupb_checkstatus(lua_State *L, upb_status *s) {
  if (!upb_ok(s)) {
    upb_status_print(s, stderr);
    // Need to copy the string to the stack, so we can free it and not leak
    // it (since luaL_error() does not return).
    char buf[strlen(s->str)+1];
    strcpy(buf, s->str);
    upb_status_uninit(s);
    luaL_error(L, "%s", buf);
  }
  upb_status_uninit(s);
}


/* object cache ***************************************************************/

// We cache all the lua objects (userdata) we vend in a weak table, indexed by
// the C pointer of the object they are caching.

static void *lupb_cache_getorcreate_size(
    lua_State *L, void *cobj, const char *type, size_t size) {
  // Lookup our cache in the registry (we don't put our objects in the registry
  // directly because we need our cache to be a weak table).
  void **obj = NULL;
  lua_getfield(L, LUA_REGISTRYINDEX, "upb.objcache");
  assert(!lua_isnil(L, -1));  // Should have been created by luaopen_upb.
  lua_pushlightuserdata(L, cobj);
  lua_rawget(L, -2);
  // Stack: objcache, cached value.
  if (lua_isnil(L, -1)) {
    // Remove bad cached value and push new value.
    lua_pop(L, 1);
    // We take advantage of the fact that all of our objects are currently a
    // single pointer, and thus have the same layout.
    obj = lua_newuserdata(L, size);
    *obj = cobj;
    luaL_getmetatable(L, type);
    assert(!lua_isnil(L, -1));  // Should have been created by luaopen_upb.
    lua_setmetatable(L, -2);

    // Set it in the cache.
    lua_pushlightuserdata(L, cobj);
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
  }
  lua_insert(L, -2);
  lua_pop(L, 1);
  return obj;
}

// Most types are just 1 pointer and can use this helper.
static bool lupb_cache_getorcreate(lua_State *L, void *cobj, const char *type) {
  return lupb_cache_getorcreate_size(L, cobj, type, sizeof(void*)) != NULL;
}


/* lupb_msg********************************************************************/

#if 0
// We prefer field access syntax (foo.bar, foo.bar = 5) over method syntax
// (foo:bar(), foo:set_bar(5)) to make messages behave more like regular tables.
// However, there are methods also, like foo:CopyFrom(other_foo) or foo:Clear().

typedef struct {
  upb_msg *msg;
  upb_msgdef *msgdef;
} lupb_msg;

static lupb_msg *lupb_msg_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, "upb.msg");
}

static void lupb_msg_pushnew(lua_State *L, upb_msgdef *md) {
  upb_msg *msg = upb_msg_new(md);
  lupb_msg *m = lupb_cache_getorcreate_size(L, msg, "upb.msg", sizeof(lupb_msg));
  assert(m);
  m->msgdef = md;
  // We need to ensure that the msgdef outlives the msg.  This performs an
  // atomic ref, if this turns out to be too expensive there are other
  // possible approaches, like creating a separate metatable for every
  // msgdef that references the msgdef.
  upb_msgdef_ref(md);
}

// Caller does *not* pass a ref.
static void lupb_msg_getorcreate(lua_State *L, upb_msg *msg, upb_msgdef *md) {
  lupb_msg *m = lupb_cache_getorcreate_size(L, msg, "upb.msg", sizeof(lupb_msg));
  if (m) {
    // New Lua object, we need to ref the message.
    m->msg = upb_msg_getref(msg);
    m->msgdef = md;
    // See comment above.
    upb_msgdef_ref(md);
  }
}

static int lupb_msg_gc(lua_State *L) {
  lupb_msg *m = lupb_msg_check(L, 1);
  upb_msg_unref(m->msg, m->msgdef);
  upb_msgdef_unref(m->msgdef);
  return 0;
}

static void lupb_pushvalue(lua_State *L, upb_value val, upb_fielddef *f) {
  switch (f->type) {
    case UPB_TYPE(INT32):
    case UPB_TYPE(SINT32):
    case UPB_TYPE(SFIXED32):
    case UPB_TYPE(ENUM):
      lua_pushnumber(L, upb_value_getint32(val)); break;
    case UPB_TYPE(INT64):
    case UPB_TYPE(SINT64):
    case UPB_TYPE(SFIXED64):
      lua_pushnumber(L, upb_value_getint64(val)); break;
    case UPB_TYPE(UINT32):
    case UPB_TYPE(FIXED32):
      lua_pushnumber(L, upb_value_getuint32(val)); break;
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64):
      lua_pushnumber(L, upb_value_getuint64(val)); break;
    case UPB_TYPE(DOUBLE):
      lua_pushnumber(L, upb_value_getdouble(val)); break;
    case UPB_TYPE(FLOAT):
      lua_pushnumber(L, upb_value_getfloat(val)); break;
    case UPB_TYPE(BOOL):
      lua_pushboolean(L, upb_value_getbool(val)); break;
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES): {
      lupb_pushstring(L, upb_value_getstr(val)); break;
    }
    case UPB_TYPE(MESSAGE):
    case UPB_TYPE(GROUP): {
      upb_msg *msg = upb_value_getmsg(val);
      assert(msg);
      lupb_msg_getorcreate(L, msg, upb_downcast_msgdef(f->def));
    }
  }
}

static upb_value lupb_getvalue(lua_State *L, int narg, upb_fielddef *f) {
  upb_value val;
  lua_Number num = 0;
  if (!upb_issubmsg(f) && !upb_isstring(f) && f->type != UPB_TYPE(BOOL)) {
    num = luaL_checknumber(L, narg);
    if (f->type != UPB_TYPE(DOUBLE) && f->type != UPB_TYPE(FLOAT) &&
        num != rint(num)) {
      luaL_error(L, "Cannot assign non-integer number %f to integer field", num);
    }
  }
  switch (f->type) {
    case UPB_TYPE(INT32):
    case UPB_TYPE(SINT32):
    case UPB_TYPE(SFIXED32):
    case UPB_TYPE(ENUM):
      if (num > INT32_MAX || num < INT32_MIN)
        luaL_error(L, "Number %f is out-of-range for 32-bit integer field.", num);
      upb_value_setint32(&val, num);
      break;
    case UPB_TYPE(INT64):
    case UPB_TYPE(SINT64):
    case UPB_TYPE(SFIXED64):
      if (num > INT64_MAX || num < INT64_MIN)
        luaL_error(L, "Number %f is out-of-range for 64-bit integer field.", num);
      upb_value_setint64(&val, num);
      break;
    case UPB_TYPE(UINT32):
    case UPB_TYPE(FIXED32):
      if (num > UINT32_MAX || num < 0)
        luaL_error(L, "Number %f is out-of-range for unsigned 32-bit integer field.", num);
      upb_value_setuint32(&val, num);
      break;
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64):
      if (num > UINT64_MAX || num < 0)
        luaL_error(L, "Number %f is out-of-range for unsigned 64-bit integer field.", num);
      upb_value_setuint64(&val, num);
      break;
    case UPB_TYPE(DOUBLE):
      if (num > DBL_MAX || num < -DBL_MAX) {
        // This could happen if lua_Number was long double.
        luaL_error(L, "Number %f is out-of-range for double field.", num);
      }
      upb_value_setdouble(&val, num);
      break;
    case UPB_TYPE(FLOAT):
      if (num > FLT_MAX || num < -FLT_MAX)
        luaL_error(L, "Number %f is out-of-range for float field.", num);
      upb_value_setfloat(&val, num);
      break;
    case UPB_TYPE(BOOL):
      if (!lua_isboolean(L, narg))
        luaL_error(L, "Must explicitly pass true or false for boolean fields");
      upb_value_setbool(&val, lua_toboolean(L, narg));
      break;
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES): {
      // TODO: is there any reasonable way to avoid a copy here?
      size_t len;
      const char *str = luaL_checklstring(L, narg, &len);
      upb_value_setstr(&val, upb_strduplen(str, len));
      break;
    }
    case UPB_TYPE(MESSAGE):
    case UPB_TYPE(GROUP): {
      lupb_msg *m = lupb_msg_check(L, narg);
      if (m->msgdef != upb_downcast_msgdef(f->def))
        luaL_error(L, "Tried to assign a message of the wrong type.");
      upb_value_setmsg(&val, m->msg);
      break;
    }
  }
  return val;
}


static int lupb_msg_index(lua_State *L) {
  assert(lua_gettop(L) == 2);  // __index should always be called with 2 args.
  lupb_msg *m = lupb_msg_check(L, 1);
  size_t len;
  const char *name = luaL_checklstring(L, 2, &len);
  upb_string namestr = UPB_STACK_STRING_LEN(name, len);
  upb_fielddef *f = upb_msgdef_ntof(m->msgdef, &namestr);
  if (f) {
    if (upb_isarray(f)) {
      luaL_error(L, "Access of repeated fields not yet implemented.");
    }
    lupb_pushvalue(L, upb_msg_get(m->msg, f), f);
  } else {
    // It wasn't a field, perhaps it's a method?
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {
      luaL_error(L, "%s is not a field name or a method name", name);
    }
  }
  return 1;
}

static int lupb_msg_newindex(lua_State *L) {
  assert(lua_gettop(L) == 3);  // __newindex should always be called with 3 args.
  lupb_msg *m = lupb_msg_check(L, 1);
  size_t len;
  const char *name = luaL_checklstring(L, 2, &len);
  upb_string namestr = UPB_STACK_STRING_LEN(name, len);
  upb_fielddef *f = upb_msgdef_ntof(m->msgdef, &namestr);
  if (f) {
    upb_value val = lupb_getvalue(L, 3, f);
    upb_msg_set(m->msg, f, val);
    if (upb_isstring(f)) {
      upb_string_unref(upb_value_getstr(val));
    }
  } else {
    luaL_error(L, "%s is not a field name", name);
  }
  return 0;
}

static int lupb_msg_clear(lua_State *L) {
  lupb_msg *m = lupb_msg_check(L, 1);
  upb_msg_clear(m->msg, m->msgdef);
  return 0;
}

static int lupb_msg_parse(lua_State *L) {
  lupb_msg *m = lupb_msg_check(L, 1);
  size_t len;
  const char *strbuf = luaL_checklstring(L, 2, &len);
  upb_string str = UPB_STACK_STRING_LEN(strbuf, len);
  upb_status status = UPB_STATUS_INIT;
  upb_strtomsg(&str, m->msg, m->msgdef, &status);
  lupb_checkstatus(L, &status);
  return 0;
}

static int lupb_msg_totext(lua_State *L) {
  lupb_msg *m = lupb_msg_check(L, 1);
  upb_string *str = upb_string_new();
  upb_msgtotext(str, m->msg, m->msgdef, false);
  lupb_pushstring(L, str);
  upb_string_unref(str);
  return 1;
}

static const struct luaL_Reg lupb_msg_mm[] = {
  {"__gc", lupb_msg_gc},
  {"__index", lupb_msg_index},
  {"__newindex", lupb_msg_newindex},
  // Our __index mm will look up methods if the index isn't a field name.
  {"Clear", lupb_msg_clear},
  {"Parse", lupb_msg_parse},
  {"ToText", lupb_msg_totext},
  {NULL, NULL}
};
#endif


/* lupb_msgdef ****************************************************************/

static upb_msgdef *lupb_msgdef_check(lua_State *L, int narg) {
  lupb_def *ldef = luaL_checkudata(L, narg, "upb.msgdef");
  return upb_downcast_msgdef(ldef->def);
}

static int lupb_msgdef_gc(lua_State *L) {
  lupb_def *ldef = luaL_checkudata(L, 1, "upb.msgdef");
  upb_def_unref(ldef->def);
  return 0;
}

#if 0
static int lupb_msgdef_call(lua_State *L) {
  upb_msgdef *md = lupb_msgdef_check(L, 1);
  lupb_msg_pushnew(L, md);
  return 1;
}
#endif

static void lupb_fielddef_getorcreate(lua_State *L, upb_fielddef *f);

static int lupb_msgdef_name(lua_State *L) {
  upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushstring(L, m->base.fqname);
  return 1;
}

static int lupb_msgdef_fieldbyname(lua_State *L) {
  upb_msgdef *m = lupb_msgdef_check(L, 1);
  upb_fielddef *f = upb_msgdef_ntof(m, luaL_checkstring(L, 2));
  if (f) {
    lupb_fielddef_getorcreate(L, f);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int lupb_msgdef_fieldbynum(lua_State *L) {
  upb_msgdef *m = lupb_msgdef_check(L, 1);
  int num = luaL_checkint(L, 2);
  upb_fielddef *f = upb_msgdef_itof(m, num);
  if (f) {
    lupb_fielddef_getorcreate(L, f);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static const struct luaL_Reg lupb_msgdef_mm[] = {
  //{"__call", lupb_msgdef_call},
  {"__gc", lupb_msgdef_gc},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_msgdef_m[] = {
  {"fieldbyname", lupb_msgdef_fieldbyname},
  {"fieldbynum", lupb_msgdef_fieldbynum},
  {"name", lupb_msgdef_name},
  {NULL, NULL}
};


/* lupb_enumdef ***************************************************************/

static upb_enumdef *lupb_enumdef_check(lua_State *L, int narg) {
  lupb_def *ldef = luaL_checkudata(L, narg, "upb.enumdef");
  return upb_downcast_enumdef(ldef->def);
}

static int lupb_enumdef_gc(lua_State *L) {
  upb_enumdef *e = lupb_enumdef_check(L, 1);
  upb_def_unref(UPB_UPCAST(e));
  return 0;
}

static int lupb_enumdef_name(lua_State *L) {
  upb_enumdef *e = lupb_enumdef_check(L, 1);
  lua_pushstring(L, e->base.fqname);
  return 1;
}

static const struct luaL_Reg lupb_enumdef_mm[] = {
  {"__gc", lupb_enumdef_gc},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_enumdef_m[] = {
  {"name", lupb_enumdef_name},
  {NULL, NULL}
};


/* lupb_def *******************************************************************/

static void lupb_def_getorcreate(lua_State *L, upb_def *def, int owned) {
  bool created = false;
  switch(def->type) {
    case UPB_DEF_MSG:
      created = lupb_cache_getorcreate(L, def, "upb.msgdef");
      break;
    case UPB_DEF_ENUM:
      created = lupb_cache_getorcreate(L, def, "upb.enumdef");
      break;
    default:
      luaL_error(L, "unknown deftype %d", def->type);
  }
  if (!owned && created) {
    upb_def_ref(def);
  } else if (owned && !created) {
    upb_def_unref(def);
  }
}


/* lupb_fielddef **************************************************************/

typedef struct {
  upb_fielddef *field;
} lupb_fielddef;

static void lupb_fielddef_getorcreate(lua_State *L, upb_fielddef *f) {
  bool created = lupb_cache_getorcreate(L, f, "upb.fielddef");
  if (created) {
    // Need to obtain a ref on this field's msgdef (fielddefs themselves aren't
    // refcounted, but they're kept alive by their owning msgdef).
    upb_def_ref(UPB_UPCAST(f->msgdef));
  }
}

static lupb_fielddef *lupb_fielddef_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, "upb.fielddef");
}

static int lupb_fielddef_index(lua_State *L) {
  lupb_fielddef *f = lupb_fielddef_check(L, 1);
  const char *str = luaL_checkstring(L, 2);
  if (strcmp(str, "name") == 0) {
    lua_pushstring(L, upb_fielddef_name(f->field));
  } else if (strcmp(str, "number") == 0) {
    lua_pushinteger(L, upb_fielddef_number(f->field));
  } else if (strcmp(str, "type") == 0) {
    lua_pushinteger(L, upb_fielddef_type(f->field));
  } else if (strcmp(str, "label") == 0) {
    lua_pushinteger(L, upb_fielddef_label(f->field));
  } else if (strcmp(str, "subdef") == 0) {
    lupb_def_getorcreate(L, upb_fielddef_subdef(f->field), false);
  } else if (strcmp(str, "msgdef") == 0) {
    lupb_def_getorcreate(L, UPB_UPCAST(upb_fielddef_msgdef(f->field)), false);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int lupb_fielddef_gc(lua_State *L) {
  lupb_fielddef *lfielddef = lupb_fielddef_check(L, 1);
  upb_def_unref(UPB_UPCAST(lfielddef->field->msgdef));
  return 0;
}

static const struct luaL_Reg lupb_fielddef_mm[] = {
  {"__gc", lupb_fielddef_gc},
  {"__index", lupb_fielddef_index},
  {NULL, NULL}
};


/* lupb_symtab ****************************************************************/

typedef struct {
  upb_symtab *symtab;
} lupb_symtab;

// Inherits a ref on the symtab.
// Checks that narg is a proper lupb_symtab object.  If it is, leaves its
// metatable on the stack for cache lookups/updates.
lupb_symtab *lupb_symtab_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, "upb.symtab");
}

static int lupb_symtab_gc(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  upb_symtab_unref(s->symtab);
  return 0;
}

static int lupb_symtab_lookup(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  upb_def *def = upb_symtab_lookup(s->symtab, luaL_checkstring(L, 2));
  if (def) {
    lupb_def_getorcreate(L, def, true);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int lupb_symtab_getdefs(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  upb_deftype_t type = luaL_checkint(L, 2);
  int count;
  upb_def **defs = upb_symtab_getdefs(s->symtab, &count, type);

  // Create the table in which we will return the defs.
  lua_createtable(L, count, 0);
  for (int i = 0; i < count; i++) {
    upb_def *def = defs[i];
    lua_pushnumber(L, i + 1);  // 1-based array.
    lupb_def_getorcreate(L, def, true);
    // Add it to our return table.
    lua_settable(L, -3);
  }
  free(defs);
  return 1;
}

static int lupb_symtab_parsedesc(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  size_t len;
  const char *str = luaL_checklstring(L, 2, &len);
  upb_status status = UPB_STATUS_INIT;
  upb_read_descriptor(s->symtab, str, len, &status);
  lupb_checkstatus(L, &status);
  return 0;
}

static const struct luaL_Reg lupb_symtab_m[] = {
  //{"addfds", lupb_symtab_addfds},
  {"getdefs", lupb_symtab_getdefs},
  {"lookup", lupb_symtab_lookup},
  {"parsedesc", lupb_symtab_parsedesc},
  //{"resolve", lupb_symtab_resolve},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_symtab_mm[] = {
  {"__gc", lupb_symtab_gc},
  {NULL, NULL}
};


/* lupb toplevel **************************************************************/

static int lupb_symtab_new(lua_State *L) {
  upb_symtab *s = upb_symtab_new();
  bool created = lupb_cache_getorcreate(L, s, "upb.symtab");
  (void)created;  // For NDEBUG
  assert(created);  // It's new, there shouldn't be an obj for it already.
  return 1;
}

static const struct luaL_Reg lupb_toplevel_m[] = {
  {"symtab", lupb_symtab_new},
  {NULL, NULL}
};

// Register the given type with the given methods and metamethods.
static void lupb_register_type(lua_State *L, const char *name,
                               const luaL_Reg *m, const luaL_Reg *mm) {
  luaL_newmetatable(L, name);
  luaL_register(L, NULL, mm);  // Register all mm in the metatable.
  lua_createtable(L, 0, 0);
  if (m) {
    // Methods go in the mt's __index method.  This implies that you can't
    // implement __index and also set methods yourself.
    luaL_register(L, NULL, m);
    lua_setfield(L, -2, "__index");
  }
  lua_pop(L, 1);  // The mt.
}

int luaopen_upb(lua_State *L) {
  lupb_register_type(L, "upb.msgdef", lupb_msgdef_m, lupb_msgdef_mm);
  lupb_register_type(L, "upb.enumdef", lupb_enumdef_m, lupb_enumdef_mm);
  lupb_register_type(L, "upb.fielddef", NULL, lupb_fielddef_mm);
  lupb_register_type(L, "upb.symtab", lupb_symtab_m, lupb_symtab_mm);
  //lupb_register_type(L, "upb.msg", NULL, lupb_msg_mm);

  // Create our object cache.
  lua_createtable(L, 0, 0);
  lua_createtable(L, 0, 1);  // Cache metatable.
  lua_pushstring(L, "v");    // Values are weak.
  lua_setfield(L, -2, "__mode");
  lua_setfield(L, LUA_REGISTRYINDEX, "upb.objcache");

  luaL_register(L, "upb", lupb_toplevel_m);
  return 1;  // Return package table.
}
