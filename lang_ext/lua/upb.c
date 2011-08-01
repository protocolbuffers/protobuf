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

static bool streql(const char *a, const char *b) { return strcmp(a, b) == 0; }

static uint8_t lupb_touint8(lua_State *L, int narg, const char *name) {
  lua_Number n = lua_tonumber(L, narg);
  if (n > UINT8_MAX || n < 0 || rint(n) != n)
    luaL_error(L, "Invalid %s", name);
  return n;
}

static uint32_t lupb_touint32(lua_State *L, int narg, const char *name) {
  lua_Number n = lua_tonumber(L, narg);
  if (n > UINT32_MAX || n < 0 || rint(n) != n)
    luaL_error(L, "Invalid %s", name);
  return n;
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
    default: luaL_error(L, "internal error");
  }
}

// Returns a scalar value (ie. not a submessage) as a upb_value.
static upb_value lupb_getvalue(lua_State *L, int narg, upb_fielddef *f,
                               upb_strref *ref) {
  assert(!upb_issubmsg(f));
  upb_value val;
  if (upb_fielddef_type(f) == UPB_TYPE(BOOL)) {
    if (!lua_isboolean(L, narg))
      luaL_error(L, "Must explicitly pass true or false for boolean fields");
    upb_value_setbool(&val, lua_toboolean(L, narg));
  } else if (upb_fielddef_type(f) == UPB_TYPE(STRING)) {
    size_t len;
    ref->ptr = luaL_checklstring(L, narg, &len);
    ref->len = len;
    upb_value_setstrref(&val, ref);
  } else {
    // Numeric type.
    lua_Number num = 0;
    num = luaL_checknumber(L, narg);
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE(INT32):
      case UPB_TYPE(SINT32):
      case UPB_TYPE(SFIXED32):
      case UPB_TYPE(ENUM):
        if (num > INT32_MAX || num < INT32_MIN || num != rint(num))
          luaL_error(L, "Cannot convert %f to 32-bit integer", num);
        upb_value_setint32(&val, num);
        break;
      case UPB_TYPE(INT64):
      case UPB_TYPE(SINT64):
      case UPB_TYPE(SFIXED64):
        if (num > INT64_MAX || num < INT64_MIN || num != rint(num))
          luaL_error(L, "Cannot convert %f to 64-bit integer", num);
        upb_value_setint64(&val, num);
        break;
      case UPB_TYPE(UINT32):
      case UPB_TYPE(FIXED32):
        if (num > UINT32_MAX || num < 0 || num != rint(num))
          luaL_error(L, "Cannot convert %f to unsigned 32-bit integer", num);
        upb_value_setuint32(&val, num);
        break;
      case UPB_TYPE(UINT64):
      case UPB_TYPE(FIXED64):
        if (num > UINT64_MAX || num < 0 || num != rint(num))
          luaL_error(L, "Cannot convert %f to unsigned 64-bit integer", num);
        upb_value_setuint64(&val, num);
        break;
      case UPB_TYPE(DOUBLE):
        if (num > DBL_MAX || num < -DBL_MAX) {
          // This could happen if lua_Number was long double.
          luaL_error(L, "Cannot convert %f to double", num);
        }
        upb_value_setdouble(&val, num);
        break;
      case UPB_TYPE(FLOAT):
        if (num > FLT_MAX || num < -FLT_MAX)
          luaL_error(L, "Cannot convert %f to float", num);
        upb_value_setfloat(&val, num);
        break;
    }
  }
  return val;
}

//static void lupb_msg_getorcreate(lua_State *L, upb_msg *msg, upb_msgdef *md);
static void lupb_fielddef_getorcreate(lua_State *L, upb_fielddef *f);
static upb_msgdef *lupb_msgdef_check(lua_State *L, int narg);
static void lupb_msg_pushnew(lua_State *L, upb_msgdef *md);

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

static void lupb_cache_create(lua_State *L, void *cobj, const char *type) {
  bool created =
      lupb_cache_getorcreate_size(L, cobj, type, sizeof(void*)) != NULL;
  (void)created;  // For NDEBUG
  assert(created);
}


/* lupb_def *******************************************************************/

// All the def types share the same C layout, even though they are different Lua
// types with different metatables.
typedef struct {
  upb_def *def;
} lupb_def;

static lupb_def *lupb_def_check(lua_State *L, int narg) {
  void *ldef = luaL_checkudata(L, narg, "upb.msgdef");
  if (!ldef) ldef = luaL_checkudata(L, narg, "upb.enumdef");
  luaL_argcheck(L, ldef != NULL, narg, "upb def expected");
  return ldef;
}

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

static lupb_fielddef *lupb_fielddef_check(lua_State *L, int narg) {
  lupb_fielddef *f = luaL_checkudata(L, narg, "upb.fielddef");
  luaL_argcheck(L, f != NULL, narg, "upb fielddef expected");
  return f;
}

#if 0
static const upb_accessor lupb_accessor = {
  upb_startfield_handler *appendseq;     // Repeated fields only.
  upb_startfield_handler *appendsubmsg;  // Submsg fields (repeated or no).
  upb_value_handler      *set;           // Scalar fields (repeated or no).

  // Readers.
  upb_has_reader         *has;
  upb_value_reader       *get;
  upb_seqbegin_handler   *seqbegin;
  upb_seqnext_handler    *seqnext;
  upb_seqget_handler     *seqget;
};
#endif

static upb_accessor_vtbl *lupb_accessor(upb_fielddef *f) {
  return upb_stdmsg_accessor(f);
}

static int lupb_fielddef_index(lua_State *L) {
  lupb_fielddef *f = lupb_fielddef_check(L, 1);
  const char *str = luaL_checkstring(L, 2);
  if (streql(str, "name")) {
    lua_pushstring(L, upb_fielddef_name(f->field));
  } else if (streql(str, "number")) {
    lua_pushinteger(L, upb_fielddef_number(f->field));
  } else if (streql(str, "type")) {
    lua_pushinteger(L, upb_fielddef_type(f->field));
  } else if (streql(str, "label")) {
    lua_pushinteger(L, upb_fielddef_label(f->field));
  } else if (streql(str, "subdef")) {
    lupb_def_getorcreate(L, upb_fielddef_subdef(f->field), false);
  } else if (streql(str, "msgdef")) {
    lupb_def_getorcreate(L, UPB_UPCAST(upb_fielddef_msgdef(f->field)), false);
  } else {
    luaL_error(L, "Invalid fielddef member '%s'", str);
  }
  return 1;
}

static void lupb_fielddef_set(lua_State *L, upb_fielddef *f,
                              const char *field, int narg) {
  if (!upb_fielddef_ismutable(f)) luaL_error(L, "fielddef is not mutable.");
  if (streql(field, "name")) {
    const char *name = lua_tostring(L, narg);
    if (!name || !upb_fielddef_setname(f, name))
      luaL_error(L, "Invalid name");
  } else if (streql(field, "number")) {
    if (!upb_fielddef_setnumber(f, lupb_touint32(L, narg, "number")))
      luaL_error(L, "Invalid number");
  } else if (streql(field, "type")) {
    if (!upb_fielddef_settype(f, lupb_touint8(L, narg, "type")))
      luaL_error(L, "Invalid type");
  } else if (streql(field, "label")) {
    if (!upb_fielddef_setlabel(f, lupb_touint8(L, narg, "label")))
      luaL_error(L, "Invalid label");
  } else if (streql(field, "type_name")) {
    const char *name = lua_tostring(L, narg);
    if (!name || !upb_fielddef_settypename(f, name))
      luaL_error(L, "Invalid type_name");
  } else if (streql(field, "default_value")) {
    if (!upb_fielddef_type(f))
      luaL_error(L, "Must set type before setting default_value");
    upb_strref ref;
    upb_fielddef_setdefault(f, lupb_getvalue(L, narg, f, &ref));
  } else {
    luaL_error(L, "Cannot set fielddef member '%s'", field);
  }
}

static int lupb_fielddef_new(lua_State *L) {
  upb_fielddef *f = upb_fielddef_new();
  lupb_cache_create(L, f, "upb.fielddef");

  if (lua_gettop(L) == 0) return 1;

  // User can specify initialization values like so:
  //   upb.FieldDef{label=upb.LABEL_REQUIRED, name="my_field", number=5,
  //                type=upb.TYPE_INT32, default_value=12, type_name="Foo"}
  luaL_checktype(L, 1, LUA_TTABLE);
  // Iterate over table.
  lua_pushnil(L);  // first key
  while (lua_next(L, 1)) {
    luaL_checktype(L, -2, LUA_TSTRING);
    const char *key = lua_tostring(L, -2);
    lupb_fielddef_set(L, f, key, -1);
    lua_pop(L, 1);
  }
  return 1;
}

static void lupb_fielddef_getorcreate(lua_State *L, upb_fielddef *f) {
  bool created = lupb_cache_getorcreate(L, f, "upb.fielddef");
  if (created) {
    // Need to obtain a ref on this field's msgdef (fielddefs themselves aren't
    // refcounted, but they're kept alive by their owning msgdef).
    upb_def_ref(UPB_UPCAST(f->msgdef));
  }
}

static int lupb_fielddef_newindex(lua_State *L) {
  lupb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_fielddef_set(L, f->field, luaL_checkstring(L, 2), 3);
  return 0;
}

static int lupb_fielddef_gc(lua_State *L) {
  lupb_fielddef *lfielddef = lupb_fielddef_check(L, 1);
  upb_fielddef_unref(lfielddef->field);
  return 0;
}

static const struct luaL_Reg lupb_fielddef_mm[] = {
  {"__gc", lupb_fielddef_gc},
  {"__index", lupb_fielddef_index},
  {"__newindex", lupb_fielddef_newindex},
  {NULL, NULL}
};


/* lupb_msgdef ****************************************************************/

static upb_msgdef *lupb_msgdef_check(lua_State *L, int narg) {
  lupb_def *ldef = luaL_checkudata(L, narg, "upb.msgdef");
  luaL_argcheck(L, ldef != NULL, narg, "upb msgdef expected");
  return upb_downcast_msgdef(ldef->def);
}

static int lupb_msgdef_gc(lua_State *L) {
  lupb_def *ldef = luaL_checkudata(L, 1, "upb.msgdef");
  upb_def_unref(ldef->def);
  return 0;
}

static int lupb_msgdef_call(lua_State *L) {
  upb_msgdef *md = lupb_msgdef_check(L, 1);
  lupb_msg_pushnew(L, md);
  return 1;
}

static int lupb_msgdef_new(lua_State *L) {
  upb_msgdef *md = upb_msgdef_new();
  lupb_cache_create(L, md, "upb.msgdef");

  if (lua_gettop(L) == 0) return 1;

  // User can specify initialization values like so:
  //   upb.MessageDef{fqname="MyMessage", extstart=8000, fields={...}}
  luaL_checktype(L, 1, LUA_TTABLE);
  // Iterate over table.
  lua_pushnil(L);  // first key
  while (lua_next(L, 1)) {
    luaL_checktype(L, -2, LUA_TSTRING);
    const char *key = lua_tostring(L, -2);

    if (streql(key, "fqname")) {  // fqname="MyMessage"
      const char *fqname = lua_tostring(L, -1);
      if (!fqname || !upb_def_setfqname(UPB_UPCAST(md), fqname))
        luaL_error(L, "Invalid fqname");
    } else if (streql(key, "fields")) {  // fields={...}
      // Iterate over the list of fields.
      lua_pushnil(L);
      luaL_checktype(L, -2, LUA_TTABLE);
      while (lua_next(L, -2)) {
        lupb_fielddef *f = lupb_fielddef_check(L, -1);
        if (!upb_msgdef_addfield(md, f->field)) {
          // TODO: more specific error.
          luaL_error(L, "Could not add field.");
        }
        lua_pop(L, 1);
      }
    } else {
      // TODO: extrange=
      luaL_error(L, "Unknown initializer key '%s'", key);
    }
    lua_pop(L, 1);
  }
  return 1;
}

static int lupb_msgdef_fqname(lua_State *L) {
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
  {"__call", lupb_msgdef_call},
  {"__gc", lupb_msgdef_gc},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_msgdef_m[] = {
  {"fieldbyname", lupb_msgdef_fieldbyname},
  {"fieldbynum", lupb_msgdef_fieldbynum},
  {"fqname", lupb_msgdef_fqname},
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

// narg is a lua table containing a list of defs to add.
void lupb_symtab_doadd(lua_State *L, upb_symtab *s, int narg) {
  luaL_checktype(L, narg, LUA_TTABLE);
  // Iterate over table twice.  First iteration to count entries and
  // check constraints.
  int n = 0;
  lua_pushnil(L);  // first key
  while (lua_next(L, narg)) {
    lupb_def_check(L, -1);
    ++n;
    lua_pop(L, 1);
  }

  // Second iteration to build deflist and layout.
  upb_def **defs = malloc(n * sizeof(*defs));
  n = 0;
  lua_pushnil(L);  // first key
  while (lua_next(L, 1)) {
    upb_def *def = lupb_def_check(L, -1)->def;
    defs[n++] = def;
    upb_msgdef *md = upb_dyncast_msgdef(def);
    if (md) {
      upb_msg_iter i;
      for(i = upb_msg_begin(md); !upb_msg_done(i); i = upb_msg_next(md, i)) {
        upb_fielddef *f = upb_msg_iter_field(i);
        upb_fielddef_setaccessor(f, lupb_accessor(f));
      }
      upb_msgdef_layout(md);
    }
    lua_pop(L, 1);
  }

  upb_status status = UPB_STATUS_INIT;
  upb_symtab_add(s, defs, n, &status);
  free(defs);
  lupb_checkstatus(L, &status);
}

static int lupb_symtab_new(lua_State *L) {
  upb_symtab *s = upb_symtab_new();
  lupb_cache_create(L, s, "upb.symtab");
  if (lua_gettop(L) == 0) return 1;
  lupb_symtab_doadd(L, s, 1);
  return 1;
}

static int lupb_symtab_add(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  lupb_symtab_doadd(L, s->symtab, 2);
  return 0;
}

static int lupb_symtab_gc(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  upb_symtab_unref(s->symtab);
  return 0;
}

static int lupb_symtab_lookup(lua_State *L) {
  lupb_symtab *s = lupb_symtab_check(L, 1);
  for (int i = 2; i <= lua_gettop(L); i++) {
    upb_def *def = upb_symtab_lookup(s->symtab, luaL_checkstring(L, i));
    if (def) {
      lupb_def_getorcreate(L, def, true);
    } else {
      lua_pushnil(L);
    }
    lua_replace(L, i);
  }
  return lua_gettop(L) - 1;
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

static const struct luaL_Reg lupb_symtab_m[] = {
  {"add", lupb_symtab_add},
  {"getdefs", lupb_symtab_getdefs},
  {"lookup", lupb_symtab_lookup},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_symtab_mm[] = {
  {"__gc", lupb_symtab_gc},
  {NULL, NULL}
};


/* lupb_msg********************************************************************/

// Messages are userdata where we store primitive values (numbers and bools)
// right in the userdata.  We also use integer entries in the environment table
// like so:
//   {msgdef, <string, submessage, or array fields>}

static void *lupb_msg_check(lua_State *L, int narg, upb_msgdef **md) {
  void *msg = luaL_checkudata(L, narg, "upb.msg");
  luaL_argcheck(L, msg != NULL, narg, "msg expected");
  // If going all the way to the environment table for the msgdef is an
  // efficiency issue, we could put the pointer right in the userdata.
  lua_getfenv(L, narg);
  lua_rawgeti(L, -1, 1);
  // Shouldn't have to check msgdef userdata validity, environment table can't
  // be accessed from Lua.
  lupb_def *lmd = lua_touserdata(L, -1);
  *md = upb_downcast_msgdef(lmd->def);
  return msg;
}

static void lupb_msg_pushnew(lua_State *L, upb_msgdef *md) {
  void *msg = lua_newuserdata(L, upb_msgdef_size(md));
  luaL_getmetatable(L, "upb.msg");
  assert(!lua_isnil(L, -1));  // Should have been created by luaopen_upb.
  lua_setmetatable(L, -2);
  upb_msg_clear(msg, md);
  lua_getfenv(L, -1);
  lupb_cache_getorcreate(L, md, "upb.msgdef");
  lua_rawseti(L, -2, 1);
  lua_pop(L, 1);  // Pop the fenv.
}

static int lupb_msg_new(lua_State *L) {
  upb_msgdef *md = lupb_msgdef_check(L, 1);
  lupb_msg_pushnew(L, md);
  return 1;
}

static int lupb_msg_index(lua_State *L) {
  assert(lua_gettop(L) == 2);  // __index should always be called with 2 args.
  upb_msgdef *md;
  void *m = lupb_msg_check(L, 1, &md);
  const char *name = luaL_checkstring(L, 2);
  upb_fielddef *f = upb_msgdef_ntof(md, name);
  if (!f) luaL_error(L, "%s is not a field name", name);
  if (upb_isseq(f)) luaL_error(L, "NYI: access of repeated fields");
  upb_value val =
      upb_msg_has(m, f) ? upb_msg_get(m, f) : upb_fielddef_default(f);
  lupb_pushvalue(L, val, f);
  return 1;
}

static int lupb_msg_newindex(lua_State *L) {
  assert(lua_gettop(L) == 3);  // __newindex should always be called with 3 args.
  upb_msgdef *md;
  void *m = lupb_msg_check(L, 1, &md);
  const char *name = luaL_checkstring(L, 2);
  upb_fielddef *f = upb_msgdef_ntof(md, name);
  if (!f) luaL_error(L, "%s is not a field name", name);
  upb_msg_set(m, f, lupb_getvalue(L, 3, f, NULL));
  return 0;
}

#if 0
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
#endif

static const struct luaL_Reg lupb_msg_mm[] = {
  {"__index", lupb_msg_index},
  {"__newindex", lupb_msg_newindex},
  {NULL, NULL}
};

// Functions that operate on msgdefs but do not live in the msgdef namespace.
static int lupb_clear(lua_State *L) {
  upb_msgdef *md;
  void *m = lupb_msg_check(L, 1, &md);
  upb_msg_clear(m, md);
  return 0;
}

static int lupb_has(lua_State *L) {
  upb_msgdef *md;
  void *m = lupb_msg_check(L, 1, &md);
  const char *name = luaL_checkstring(L, 2);
  upb_fielddef *f = upb_msgdef_ntof(md, name);
  if (!f) luaL_error(L, "%s is not a field name", name);
  lua_pushboolean(L, upb_msg_has(m, f));
  return 1;
}

static int lupb_msgdef(lua_State *L) {
  upb_msgdef *md;
  lupb_msg_check(L, 1, &md);
  lupb_def_getorcreate(L, UPB_UPCAST(md), false);
  return 1;
}


/* lupb toplevel **************************************************************/

static const struct luaL_Reg lupb_toplevel_m[] = {
  {"SymbolTable", lupb_symtab_new},
  {"MessageDef", lupb_msgdef_new},
  {"FieldDef", lupb_fielddef_new},

  {"Message", lupb_msg_new},
  //{"Array", lupb_array_new},

  {"clear", lupb_clear},
  {"msgdef", lupb_msgdef},
  {"has", lupb_has},

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

static void lupb_setfieldi(lua_State *L, const char *field, int i) {
  lua_pushnumber(L, i);
  lua_setfield(L, -2, field);
}

int luaopen_upb(lua_State *L) {
  lupb_register_type(L, "upb.msgdef", lupb_msgdef_m, lupb_msgdef_mm);
  lupb_register_type(L, "upb.enumdef", lupb_enumdef_m, lupb_enumdef_mm);
  lupb_register_type(L, "upb.fielddef", NULL, lupb_fielddef_mm);
  lupb_register_type(L, "upb.symtab", lupb_symtab_m, lupb_symtab_mm);

  lupb_register_type(L, "upb.msg", NULL, lupb_msg_mm);
  lupb_register_type(L, "upb.array", NULL, lupb_msg_mm);

  // Create our object cache.
  lua_createtable(L, 0, 0);
  lua_createtable(L, 0, 1);  // Cache metatable.
  lua_pushstring(L, "v");    // Values are weak.
  lua_setfield(L, -2, "__mode");
  lua_setfield(L, LUA_REGISTRYINDEX, "upb.objcache");

  luaL_register(L, "upb", lupb_toplevel_m);

  // Register constants.
  lupb_setfieldi(L, "LABEL_OPTIONAL", UPB_LABEL(OPTIONAL));
  lupb_setfieldi(L, "LABEL_REQUIRED", UPB_LABEL(REQUIRED));
  lupb_setfieldi(L, "LABEL_REPEATED", UPB_LABEL(REPEATED));

  lupb_setfieldi(L, "TYPE_DOUBLE", UPB_TYPE(DOUBLE));
  lupb_setfieldi(L, "TYPE_FLOAT", UPB_TYPE(FLOAT));
  lupb_setfieldi(L, "TYPE_INT64", UPB_TYPE(INT64));
  lupb_setfieldi(L, "TYPE_UINT64", UPB_TYPE(UINT64));
  lupb_setfieldi(L, "TYPE_INT32", UPB_TYPE(INT32));
  lupb_setfieldi(L, "TYPE_FIXED64", UPB_TYPE(FIXED64));
  lupb_setfieldi(L, "TYPE_FIXED32", UPB_TYPE(FIXED32));
  lupb_setfieldi(L, "TYPE_BOOL", UPB_TYPE(BOOL));
  lupb_setfieldi(L, "TYPE_STRING", UPB_TYPE(STRING));
  lupb_setfieldi(L, "TYPE_GROUP", UPB_TYPE(GROUP));
  lupb_setfieldi(L, "TYPE_MESSAGE", UPB_TYPE(MESSAGE));
  lupb_setfieldi(L, "TYPE_BYTES", UPB_TYPE(BYTES));
  lupb_setfieldi(L, "TYPE_UINT32", UPB_TYPE(UINT32));
  lupb_setfieldi(L, "TYPE_ENUM", UPB_TYPE(ENUM));
  lupb_setfieldi(L, "TYPE_SFIXED32", UPB_TYPE(SFIXED32));
  lupb_setfieldi(L, "TYPE_SFIXED64", UPB_TYPE(SFIXED64));
  lupb_setfieldi(L, "TYPE_SINT32", UPB_TYPE(SINT32));
  lupb_setfieldi(L, "TYPE_SINT64", UPB_TYPE(SINT64));

  return 1;  // Return package table.
}
