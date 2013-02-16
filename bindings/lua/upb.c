/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A Lua extension for upb.  Exposes only the core library
 * (sub-libraries are exposed in other extensions).
 */

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "lauxlib.h"
#include "bindings/lua/upb.h"
#include "upb/bytestream.h"
#include "upb/pb/glue.h"

// Lua metatable types.
#define LUPB_MSGDEF "lupb.msgdef"
#define LUPB_ENUMDEF "lupb.enumdef"
#define LUPB_FIELDDEF "lupb.fielddef"
#define LUPB_SYMTAB "lupb.symtab"

// Other table constants.
#define LUPB_OBJCACHE "lupb.objcache"

#if LUA_VERSION_NUM == 501

// Taken from Lua 5.2's source.
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

#elif LUA_VERSION_NUM == 502

int luaL_typerror(lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}

#else
#error Only Lua 5.1 and 5.2 are supported
#endif

const char *lupb_checkname(lua_State *L, int narg) {
  size_t len;
  const char *name = luaL_checklstring(L, narg, &len);
  if (strlen(name) != len)
    luaL_error(L, "names cannot have embedded NULLs");
  return name;
}

static bool streql(const char *a, const char *b) { return strcmp(a, b) == 0; }

static uint32_t lupb_checkint32(lua_State *L, int narg, const char *name) {
  lua_Number n = lua_tonumber(L, narg);
  if (n > INT32_MAX || n < INT32_MIN || rint(n) != n)
    luaL_error(L, "Invalid %s", name);
  return n;
}

// Converts a number or bool from Lua -> upb_value.
static upb_value lupb_getvalue(lua_State *L, int narg, upb_fieldtype_t type) {
  upb_value val;
  if (type == UPB_TYPE(BOOL)) {
    if (!lua_isboolean(L, narg))
      luaL_error(L, "Must explicitly pass true or false for boolean fields");
    upb_value_setbool(&val, lua_toboolean(L, narg));
  } else {
    // Numeric type.
    lua_Number num = luaL_checknumber(L, narg);
    switch (type) {
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
      default: luaL_error(L, "invalid type");
    }
  }
  return val;
}

// Converts a upb_value -> Lua value.
static void lupb_pushvalue(lua_State *L, upb_value val, upb_fieldtype_t type) {
  switch (type) {
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
      const upb_byteregion *r = upb_value_getbyteregion(val);
      size_t len;
      const char *str = upb_byteregion_getptr(r, 0, &len);
      lua_pushlstring(L, str, len);
    }
    default: luaL_error(L, "internal error");
  }
}

void lupb_checkstatus(lua_State *L, upb_status *s) {
  if (!upb_ok(s)) {
    lua_pushstring(L, upb_status_getstr(s));
    upb_status_uninit(s);
    lua_error(L);
  } else {
    upb_status_uninit(s);
  }
}


/* refcounted *****************************************************************/

// All upb objects that use upb_refcounted share a common Lua userdata
// representation and a common scheme for caching Lua wrapper object.  They do
// however have different metatables.  Objects are cached in a weak table
// indexed by the C pointer of the object they are caching.

typedef union {
  const upb_refcounted *refcounted;
  const upb_def *def;
  upb_symtab *symtab;
} lupb_refcounted;

static bool lupb_refcounted_pushwrapper(lua_State *L, const upb_refcounted *obj,
                                        const char *type, const void *owner) {
  if (obj == NULL) {
    lua_pushnil(L);
    return false;
  }

  // Lookup our cache in the registry (we don't put our objects in the registry
  // directly because we need our cache to be a weak table).
  lupb_refcounted *ud = NULL;
  lua_getfield(L, LUA_REGISTRYINDEX, LUPB_OBJCACHE);
  assert(!lua_isnil(L, -1));  // Should have been created by luaopen_upb.
  lua_pushlightuserdata(L, (void*)obj);
  lua_rawget(L, -2);
  // Stack: objcache, cached value.
  bool create = lua_isnil(L, -1) ||
                // A corner case: it is possible for the value to be GC'd
                // already, in which case we should evict this entry and create
                // a new one.
                ((lupb_refcounted*)lua_touserdata(L, -1))->refcounted == NULL;
  if (create) {
    // Remove bad cached value and push new value.
    lua_pop(L, 1);

    // We take advantage of the fact that all of our objects are currently a
    // single pointer, and thus have the same layout.
    // TODO: this probably violates aliasing.
    ud = lua_newuserdata(L, sizeof(lupb_refcounted));
    ud->refcounted = obj;
    upb_refcounted_donateref(obj, owner, ud);

    luaL_getmetatable(L, type);
    assert(!lua_isnil(L, -1));  // Should have been created by luaopen_upb.
    lua_setmetatable(L, -2);

    // Set it in the cache.
    lua_pushlightuserdata(L, (void*)obj);
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
  } else {
    // Existing wrapper obj already has a ref.
    ud = lua_touserdata(L, -1);
    upb_refcounted_checkref(obj, ud);
    if (owner)
      upb_refcounted_unref(obj, owner);
  }
  lua_insert(L, -2);
  lua_pop(L, 1);
  return create;
}

static void lupb_refcounted_pushnewrapper(lua_State *L, upb_refcounted *obj,
                                          const char *type, const void *owner) {
  bool created = lupb_refcounted_pushwrapper(L, obj, type, owner);
  UPB_ASSERT_VAR(created, created == true);
}


/* lupb_def *******************************************************************/

static const upb_def *lupb_def_check(lua_State *L, int narg) {
  lupb_refcounted *r = luaL_testudata(L, narg, LUPB_MSGDEF);
  if (!r) r = luaL_testudata(L, narg, LUPB_ENUMDEF);
  if (!r) r = luaL_testudata(L, narg, LUPB_FIELDDEF);
  if (!r) luaL_typerror(L, narg, "upb def");
  if (!r->refcounted) luaL_error(L, "called into dead def");
  return r->def;
}

static upb_def *lupb_def_checkmutable(lua_State *L, int narg) {
  const upb_def *def = lupb_def_check(L, narg);
  if (upb_def_isfrozen(def))
    luaL_typerror(L, narg, "not allowed on frozen value");
  return (upb_def*)def;
}

bool lupb_def_pushwrapper(lua_State *L, const upb_def *def, const void *owner) {
  if (def == NULL) {
    lua_pushnil(L);
    return false;
  }

  const char *type = NULL;
  switch (def->type) {
    case UPB_DEF_MSG: type = LUPB_MSGDEF; break;
    case UPB_DEF_ENUM: type = LUPB_ENUMDEF; break;
    case UPB_DEF_FIELD: type = LUPB_FIELDDEF; break;
    default: luaL_error(L, "unknown deftype %d", def->type);
  }
  return lupb_refcounted_pushwrapper(L, upb_upcast(def), type, owner);
}

void lupb_def_pushnewrapper(lua_State *L, const upb_def *def,
                            const void *owner) {
  bool created = lupb_def_pushwrapper(L, def, owner);
  UPB_ASSERT_VAR(created, created == true);
}

static int lupb_def_type(lua_State *L) {
  const upb_def *def = lupb_def_check(L, 1);
  lua_pushnumber(L, upb_def_type(def));
  return 1;
}

static int lupb_def_isfrozen(lua_State *L) {
  const upb_def *def = lupb_def_check(L, 1);
  lua_pushboolean(L, upb_def_isfrozen(def));
  return 1;
}

static int lupb_def_fullname(lua_State *L) {
  const upb_def *def = lupb_def_check(L, 1);
  lua_pushstring(L, upb_def_fullname(def));
  return 1;
}

static int lupb_def_setfullname(lua_State *L) {
  upb_def *def = lupb_def_checkmutable(L, 1);
  const char *name = lupb_checkname(L, 2);
  upb_def_setfullname(def, name);
  return 0;
}

#define LUPB_COMMON_DEF_METHODS \
  {"def_type", lupb_def_type},  \
  {"full_name", lupb_def_fullname}, \
  {"is_frozen", lupb_def_isfrozen}, \
  {"set_full_name", lupb_def_setfullname}, \


/* lupb_fielddef **************************************************************/

static const upb_fielddef *lupb_fielddef_check(lua_State *L, int narg) {
  lupb_refcounted *r = luaL_checkudata(L, narg, LUPB_FIELDDEF);
  if (!r) luaL_typerror(L, narg, "upb fielddef");
  if (!r->refcounted) luaL_error(L, "called into dead fielddef");
  return upb_downcast_fielddef(r->def);
}

static upb_fielddef *lupb_fielddef_checkmutable(lua_State *L, int narg) {
  const upb_fielddef *f = lupb_fielddef_check(L, narg);
  if (upb_fielddef_isfrozen(f))
    luaL_typerror(L, narg, "not allowed on frozen value");
  return (upb_fielddef*)f;
}

// Setter functions; these are called by both the constructor and the individual
// setter API calls like field:set_type().

static void lupb_fielddef_dosetdefault(lua_State *L, upb_fielddef *f,
                                       int narg) {
  int type = lua_type(L, narg);
  upb_fieldtype_t upbtype = upb_fielddef_type(f);
  if (type == LUA_TSTRING) {
    if (!upb_fielddef_isstring(f) && upbtype != UPB_TYPE(ENUM))
      luaL_argerror(L, narg, "field does not expect a string default");
    size_t len;
    const char *str = lua_tolstring(L, narg, &len);
    if (!upb_fielddef_setdefaultstr(f, str, len))
      luaL_argerror(L, narg, "invalid default string for enum");
  } else {
    upb_fielddef_setdefault(f, lupb_getvalue(L, narg, upbtype));
  }
}

static void lupb_fielddef_dosetlabel(lua_State *L, upb_fielddef *f, int narg) {
  upb_label_t label = luaL_checknumber(L, narg);
  if (!upb_fielddef_setlabel(f, label))
    luaL_argerror(L, narg, "invalid field label");
}

static void lupb_fielddef_dosetnumber(lua_State *L, upb_fielddef *f, int narg) {
  int32_t n = luaL_checknumber(L, narg);
  if (!upb_fielddef_setnumber(f, n))
    luaL_argerror(L, narg, "invalid field number");
}

static void lupb_fielddef_dosetsubdef(lua_State *L, upb_fielddef *f, int narg) {
  const upb_def *def = NULL;
  if (!lua_isnil(L, narg))
    def = lupb_def_check(L, narg);
  if (!upb_fielddef_setsubdef(f, def))
    luaL_argerror(L, narg, "invalid subdef for this field");
}

static void lupb_fielddef_dosetsubdefname(lua_State *L, upb_fielddef *f,
                                          int narg) {
  const char *name = NULL;
  if (!lua_isnil(L, narg))
    name = lupb_checkname(L, narg);
  if (!upb_fielddef_setsubdefname(f, name))
    luaL_argerror(L, narg, "field type does not expect a subdef");
}

static void lupb_fielddef_dosettype(lua_State *L, upb_fielddef *f, int narg) {
  int32_t type = luaL_checknumber(L, narg);
  if (!upb_fielddef_settype(f, type))
    luaL_argerror(L, narg, "invalid field type");
}

// Setter API calls.  These use the setter functions above.

static int lupb_fielddef_setdefault(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  lupb_fielddef_dosetdefault(L, f, 2);
  return 0;
}

static int lupb_fielddef_setlabel(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  lupb_fielddef_dosetlabel(L, f, 2);
  return 0;
}

static int lupb_fielddef_setnumber(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  lupb_fielddef_dosetnumber(L, f, 2);
  return 0;
}

static int lupb_fielddef_setsubdef(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  lupb_fielddef_dosetsubdef(L, f, 2);
  return 0;
}

static int lupb_fielddef_setsubdefname(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  lupb_fielddef_dosetsubdefname(L, f, 2);
  return 0;
}

static int lupb_fielddef_settype(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  lupb_fielddef_dosettype(L, f, 2);
  return 0;
}

// Constructor and other methods.

static int lupb_fielddef_new(lua_State *L) {
  upb_fielddef *f = upb_fielddef_new(&f);
  int narg = lua_gettop(L);

  lupb_def_pushnewrapper(L, upb_upcast(f), &f);

  if (narg == 0) return 1;

  // User can specify initialization values like so:
  //   upb.FieldDef{label=upb.LABEL_REQUIRED, name="my_field", number=5,
  //                type=upb.TYPE_INT32, default_value=12, type_name="Foo"}
  luaL_checktype(L, 1, LUA_TTABLE);
  for (lua_pushnil(L); lua_next(L, 1); lua_pop(L, 1)) {
    luaL_checktype(L, -2, LUA_TSTRING);
    const char *key = lua_tostring(L, -2);
    int v = -1;
    if (streql(key, "name")) upb_fielddef_setname(f, lupb_checkname(L, v));
    else if (streql(key, "number")) lupb_fielddef_dosetnumber(L, f, v);
    else if (streql(key, "type")) lupb_fielddef_dosettype(L, f, v);
    else if (streql(key, "label")) lupb_fielddef_dosetlabel(L, f, v);
    else if (streql(key, "default_value")) ;  // Defer to second pass.
    else if (streql(key, "subdef")) ;         // Defer to second pass.
    else if (streql(key, "subdef_name")) ;    // Defer to second pass.
    else luaL_error(L, "Cannot set fielddef member '%s'", key);
  }

  // Have to do these in a second pass because these depend on the type, so we
  // have to make sure the type is set if the user specified one.
  for (lua_pushnil(L); lua_next(L, 1); lua_pop(L, 1)) {
    const char *key = lua_tostring(L, -2);
    int v = -1;
    if (streql(key, "default_value")) lupb_fielddef_dosetdefault(L, f, v);
    else if (streql(key, "subdef")) lupb_fielddef_dosetsubdef(L, f, v);
    else if (streql(key, "subdef_name")) lupb_fielddef_dosetsubdefname(L, f, v);
  }

  return 1;
}

static int lupb_fielddef_default(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  upb_fieldtype_t type = upb_fielddef_type(f);
  if (upb_fielddef_default_is_symbolic(f))
    type = UPB_TYPE(STRING);
  lupb_pushvalue(L, upb_fielddef_default(f), type);
  return 1;
}

static int lupb_fielddef_label(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushnumber(L, upb_fielddef_label(f));
  return 1;
}

static int lupb_fielddef_number(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  int32_t num = upb_fielddef_number(f);
  if (num)
    lua_pushnumber(L, num);
  else
    lua_pushnil(L);
  return 1;
}

static int lupb_fielddef_selectorbase(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  if (!upb_fielddef_isfrozen(f))
    luaL_error(L, "_selectorbase is only defined for frozen fielddefs");
  lua_pushnumber(L, f->selector_base);
  return 1;
}

static int lupb_fielddef_hassubdef(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushboolean(L, upb_fielddef_hassubdef(f));
  return 1;
}

static int lupb_fielddef_msgdef(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_def_pushwrapper(L, upb_upcast(upb_fielddef_msgdef(f)), NULL);
  return 1;
}

static int lupb_fielddef_subdef(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  if (!upb_fielddef_hassubdef(f))
    luaL_error(L, "Tried to get subdef of non-message field");
  const upb_def *def = upb_fielddef_subdef(f);
  lupb_def_pushwrapper(L, def, NULL);
  return 1;
}

static int lupb_fielddef_subdefname(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  if (!upb_fielddef_hassubdef(f))
    luaL_error(L, "Tried to get subdef name of non-message field");
  lua_pushstring(L, upb_fielddef_subdefname(f));
  return 1;
}

static int lupb_fielddef_type(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushnumber(L, upb_fielddef_type(f));
  return 1;
}

static int lupb_fielddef_gc(lua_State *L) {
  lupb_refcounted *r = luaL_checkudata(L, 1, LUPB_FIELDDEF);
  upb_def_unref(r->def, r);
  r->refcounted = NULL;
  return 0;
}

static const struct luaL_Reg lupb_fielddef_m[] = {
  LUPB_COMMON_DEF_METHODS

  {"default", lupb_fielddef_default},
  {"has_subdef", lupb_fielddef_hassubdef},
  {"label", lupb_fielddef_label},
  {"msgdef", lupb_fielddef_msgdef},
  {"name", lupb_def_fullname},  // name() is just an alias for fullname()
  {"number", lupb_fielddef_number},
  {"subdef", lupb_fielddef_subdef},
  {"subdef_name", lupb_fielddef_subdefname},
  {"type", lupb_fielddef_type},

  {"set_default", lupb_fielddef_setdefault},
  {"set_label", lupb_fielddef_setlabel},
  {"set_name", lupb_def_setfullname},  // name() is just an alias for fullname()
  {"set_number", lupb_fielddef_setnumber},
  {"set_subdef", lupb_fielddef_setsubdef},
  {"set_subdef_name", lupb_fielddef_setsubdefname},
  {"set_type", lupb_fielddef_settype},

  // Internal-only.
  {"_selector_base", lupb_fielddef_selectorbase},

  {NULL, NULL}
};

static const struct luaL_Reg lupb_fielddef_mm[] = {
  {"__gc", lupb_fielddef_gc},
  {NULL, NULL}
};


/* lupb_msgdef ****************************************************************/

const upb_msgdef *lupb_msgdef_check(lua_State *L, int narg) {
  lupb_refcounted *r = luaL_checkudata(L, narg, LUPB_MSGDEF);
  if (!r) luaL_typerror(L, narg, LUPB_MSGDEF);
  if (!r->refcounted) luaL_error(L, "called into dead msgdef");
  return upb_downcast_msgdef(r->def);
}

static upb_msgdef *lupb_msgdef_checkmutable(lua_State *L, int narg) {
  const upb_msgdef *m = lupb_msgdef_check(L, narg);
  if (upb_msgdef_isfrozen(m))
    luaL_typerror(L, narg, "not allowed on frozen value");
  return (upb_msgdef*)m;
}

static int lupb_msgdef_gc(lua_State *L) {
  lupb_refcounted *r = luaL_checkudata(L, 1, LUPB_MSGDEF);
  upb_def_unref(r->def, r);
  r->refcounted = NULL;
  return 0;
}

static int lupb_msgdef_new(lua_State *L) {
  int narg = lua_gettop(L);
  upb_msgdef *md = upb_msgdef_new(&md);
  lupb_def_pushnewrapper(L, upb_upcast(md), &md);

  if (narg == 0) return 1;

  // User can specify initialization values like so:
  //   upb.MessageDef{full_name="MyMessage", extstart=8000, fields={...}}
  luaL_checktype(L, 1, LUA_TTABLE);
  for (lua_pushnil(L); lua_next(L, 1); lua_pop(L, 1)) {
    luaL_checktype(L, -2, LUA_TSTRING);
    const char *key = lua_tostring(L, -2);

    if (streql(key, "full_name")) {  // full_name="MyMessage"
      const char *fqname = lua_tostring(L, -1);
      if (!fqname || !upb_def_setfullname(upb_upcast(md), fqname))
        luaL_error(L, "Invalid full_name");
    } else if (streql(key, "fields")) {  // fields={...}
      // Iterate over the list of fields.
      luaL_checktype(L, -1, LUA_TTABLE);
      for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1)) {
        upb_fielddef *f = lupb_fielddef_checkmutable(L, -1);
        if (!upb_msgdef_addfield(md, f, NULL)) {
          // TODO: more specific error.
          luaL_error(L, "Could not add field.");
        }
      }
    } else {
      // TODO: extrange=
      luaL_error(L, "Unknown initializer key '%s'", key);
    }
  }
  return 1;
}

static int lupb_msgdef_add(lua_State *L) {
  upb_msgdef *m = lupb_msgdef_checkmutable(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  int n = lua_rawlen(L, 2);
  // TODO: add upb interface that lets us avoid this malloc/free.
  upb_fielddef **fields = malloc(n * sizeof(upb_fielddef*));
  for (int i = 0; i < n; i++) {
    lua_rawgeti(L, -1, i + 1);
    fields[i] = lupb_fielddef_checkmutable(L, -1);
    lua_pop(L, 1);
  }

  bool success = upb_msgdef_addfields(m, fields, n, NULL);
  free(fields);
  if (!success) luaL_error(L, "fields could not be added");
  return 0;
}

static int lupb_msgdef_len(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushinteger(L, upb_msgdef_numfields(m));
  return 1;
}

static int lupb_msgdef_selectorcount(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushinteger(L, m->selector_count);
  return 1;
}

static int lupb_msgdef_field(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  int type = lua_type(L, 2);
  const upb_fielddef *f;
  if (type == LUA_TNUMBER) {
    f = upb_msgdef_itof(m, lua_tointeger(L, 2));
  } else if (type == LUA_TSTRING) {
    f = upb_msgdef_ntof(m, lua_tostring(L, 2));
  } else {
    const char *msg = lua_pushfstring(L, "number or string expected, got %s",
                                      luaL_typename(L, 2));
    return luaL_argerror(L, 2, msg);
  }

  lupb_def_pushwrapper(L, upb_upcast(f), NULL);
  return 1;
}

static int lupb_msgiter_next(lua_State *L) {
  upb_msg_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_msg_done(i)) return 0;
  lupb_def_pushwrapper(L, upb_upcast(upb_msg_iter_field(i)), NULL);
  upb_msg_next(i);
  return 1;
}

static int lupb_msgdef_fields(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  upb_msg_iter *i = lua_newuserdata(L, sizeof(upb_msg_iter));
  upb_msg_begin(i, m);
  lua_pushcclosure(L, &lupb_msgiter_next, 1);
  return 1;
}

static const struct luaL_Reg lupb_msgdef_mm[] = {
  {"__gc", lupb_msgdef_gc},
  {"__len", lupb_msgdef_len},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_msgdef_m[] = {
  LUPB_COMMON_DEF_METHODS
  {"add", lupb_msgdef_add},
  {"field", lupb_msgdef_field},
  {"fields", lupb_msgdef_fields},

  // Internal-only.
  {"_selector_count", lupb_msgdef_selectorcount},

  {NULL, NULL}
};


/* lupb_enumdef ***************************************************************/

const upb_enumdef *lupb_enumdef_check(lua_State *L, int narg) {
  lupb_refcounted *r = luaL_checkudata(L, narg, LUPB_ENUMDEF);
  if (!r) luaL_typerror(L, narg, LUPB_ENUMDEF);
  if (!r->refcounted) luaL_error(L, "called into dead enumdef");
  return upb_downcast_enumdef(r->def);
}

static upb_enumdef *lupb_enumdef_checkmutable(lua_State *L, int narg) {
  const upb_enumdef *f = lupb_enumdef_check(L, narg);
  if (upb_enumdef_isfrozen(f))
    luaL_typerror(L, narg, "not allowed on frozen value");
  return (upb_enumdef*)f;
}

static int lupb_enumdef_gc(lua_State *L) {
  lupb_refcounted *r = luaL_checkudata(L, 1, LUPB_ENUMDEF);
  upb_def_unref(r->def, r);
  r->refcounted = NULL;
  return 0;
}

static int lupb_enumdef_new(lua_State *L) {
  int narg = lua_gettop(L);
  upb_enumdef *e = upb_enumdef_new(&e);
  lupb_def_pushnewrapper(L, upb_upcast(e), &e);

  if (narg == 0) return 1;

  // User can specify initialization values like so:
  //   upb.EnumDef{full_name="MyEnum",
  //     values={
  //       {"FOO_VALUE_1", 1},
  //       {"FOO_VALUE_2", 2}
  //     }
  //   }
  luaL_checktype(L, 1, LUA_TTABLE);
  for (lua_pushnil(L); lua_next(L, 1); lua_pop(L, 1)) {
    luaL_checktype(L, -2, LUA_TSTRING);
    const char *key = lua_tostring(L, -2);
    if (streql(key, "values")) {
      for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1)) {
        lua_rawgeti(L, -1, 1);
        luaL_checktype(L, -1, LUA_TSTRING);
        const char *name = lua_tostring(L, -1);
        lua_rawgeti(L, -2, 2);
        int32_t num = lupb_checkint32(L, -1, "value");
        upb_status status = UPB_STATUS_INIT;
        upb_enumdef_addval(e, name, num, &status);
        lupb_checkstatus(L, &status);
        lua_pop(L, 2);  // The key/val we got from lua_rawgeti()
      }
    } else if (streql(key, "full_name")) {
      const char *fullname = lua_tostring(L, -1);
      if (!fullname || !upb_def_setfullname(upb_upcast(e), fullname))
        luaL_error(L, "Invalid full_name");
    } else {
      luaL_error(L, "Unknown initializer key '%s'", key);
    }
  }
  return 1;
}

static int lupb_enumdef_add(lua_State *L) {
  upb_enumdef *e = lupb_enumdef_checkmutable(L, 1);
  const char *name = lupb_checkname(L, 2);
  int32_t num = lupb_checkint32(L, 3, "value");
  upb_status status = UPB_STATUS_INIT;
  upb_enumdef_addval(e, name, num, &status);
  lupb_checkstatus(L, &status);
  return 0;
}

static int lupb_enumdef_len(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);
  lua_pushinteger(L, upb_enumdef_numvals(e));
  return 1;
}

static int lupb_enumdef_value(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);
  int type = lua_type(L, 2);
  if (type == LUA_TNUMBER) {
    // Pushes "nil" for a NULL pointer.
    lua_pushstring(L, upb_enumdef_iton(e, lupb_checkint32(L, 2, "value")));
  } else if (type == LUA_TSTRING) {
    int32_t num;
    if (upb_enumdef_ntoi(e, lua_tostring(L, 2), &num)) {
      lua_pushnumber(L, num);
    } else {
      lua_pushnil(L);
    }
  } else {
    const char *msg = lua_pushfstring(L, "number or string expected, got %s",
                                      luaL_typename(L, 2));
    return luaL_argerror(L, 2, msg);
  }
  return 1;
}

static int lupb_enumiter_next(lua_State *L) {
  upb_enum_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_enum_done(i)) return 0;
  lua_pushstring(L, upb_enum_iter_name(i));
  lua_pushnumber(L, upb_enum_iter_number(i));
  upb_enum_next(i);
  return 2;
}

static int lupb_enumdef_values(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);
  upb_enum_iter *i = lua_newuserdata(L, sizeof(upb_enum_iter));
  upb_enum_begin(i, e);
  lua_pushcclosure(L, &lupb_enumiter_next, 1);
  return 1;
}

static const struct luaL_Reg lupb_enumdef_mm[] = {
  {"__gc", lupb_enumdef_gc},
  {"__len", lupb_enumdef_len},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_enumdef_m[] = {
  LUPB_COMMON_DEF_METHODS
  {"add", lupb_enumdef_add},
  {"value", lupb_enumdef_value},
  {"values", lupb_enumdef_values},
  {NULL, NULL}
};


/* lupb_symtab ****************************************************************/

// Inherits a ref on the symtab.
// Checks that narg is a proper lupb_symtab object.  If it is, leaves its
// metatable on the stack for cache lookups/updates.
upb_symtab *lupb_symtab_check(lua_State *L, int narg) {
  lupb_refcounted *r = luaL_checkudata(L, narg, LUPB_SYMTAB);
  if (!r) luaL_typerror(L, narg, LUPB_SYMTAB);
  if (!r->refcounted) luaL_error(L, "called into dead symtab");
  return r->symtab;
}

// narg is a lua table containing a list of defs to add.
void lupb_symtab_doadd(lua_State *L, upb_symtab *s, int narg) {
  luaL_checktype(L, narg, LUA_TTABLE);
  // Iterate over table twice.  First iteration to count entries and
  // check constraints.
  int n = 0;
  for (lua_pushnil(L); lua_next(L, narg); lua_pop(L, 1)) {
    lupb_def_check(L, -1);
    ++n;
  }

  // Second iteration to build deflist and layout.
  upb_def **defs = malloc(n * sizeof(*defs));
  n = 0;
  for (lua_pushnil(L); lua_next(L, narg); lua_pop(L, 1)) {
    upb_def *def = lupb_def_checkmutable(L, -1);
    defs[n++] = def;
  }

  upb_status status = UPB_STATUS_INIT;
  upb_symtab_add(s, defs, n, NULL, &status);
  free(defs);
  lupb_checkstatus(L, &status);
}

static int lupb_symtab_new(lua_State *L) {
  int narg = lua_gettop(L);
  upb_symtab *s = upb_symtab_new(&s);
  lupb_refcounted_pushnewrapper(L, upb_upcast(s), LUPB_SYMTAB, &s);
  if (narg > 0) lupb_symtab_doadd(L, s, 1);
  return 1;
}

static int lupb_symtab_add(lua_State *L) {
  lupb_symtab_doadd(L, lupb_symtab_check(L, 1), 2);
  return 0;
}

static int lupb_symtab_gc(lua_State *L) {
  lupb_refcounted *r = luaL_checkudata(L, 1, LUPB_SYMTAB);
  upb_symtab_unref(r->symtab, r);
  r->refcounted = NULL;
  return 0;
}

static int lupb_symtab_lookup(lua_State *L) {
  upb_symtab *s = lupb_symtab_check(L, 1);
  for (int i = 2; i <= lua_gettop(L); i++) {
    const upb_def *def =
        upb_symtab_lookup(s, luaL_checkstring(L, i), &def);
    lupb_def_pushwrapper(L, def, &def);
    lua_replace(L, i);
  }
  return lua_gettop(L) - 1;
}

static int lupb_symtab_getdefs(lua_State *L) {
  upb_symtab *s = lupb_symtab_check(L, 1);
  upb_deftype_t type = luaL_checkint(L, 2);
  int count;
  const upb_def **defs = upb_symtab_getdefs(s, type, &defs, &count);

  // Create the table in which we will return the defs.
  lua_createtable(L, count, 0);
  for (int i = 0; i < count; i++) {
    const upb_def *def = defs[i];
    lupb_def_pushwrapper(L, def, &defs);
    lua_rawseti(L, -2, i + 1);
  }
  free(defs);
  return 1;
}

// This is a *temporary* API that will be removed once pending refactorings are
// complete (it does not belong here in core because it depends on both
// the descriptor.proto schema and the protobuf binary format.
static int lupb_symtab_load_descriptor(lua_State *L) {
  size_t len;
  upb_symtab *s = lupb_symtab_check(L, 1);
  const char *str = luaL_checklstring(L, 2, &len);
  upb_status status = UPB_STATUS_INIT;
  upb_load_descriptor_into_symtab(s, str, len, &status);
  lupb_checkstatus(L, &status);
  return 0;
}

static const struct luaL_Reg lupb_symtab_m[] = {
  {"add", lupb_symtab_add},
  {"getdefs", lupb_symtab_getdefs},
  {"lookup", lupb_symtab_lookup},
  {"load_descriptor", lupb_symtab_load_descriptor},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_symtab_mm[] = {
  {"__gc", lupb_symtab_gc},
  {NULL, NULL}
};


/* lupb toplevel **************************************************************/

static int lupb_def_freeze(lua_State *L) {
  int n = lua_gettop(L);
  upb_def **defs = malloc(n * sizeof(upb_def*));
  for (int i = 0; i < n; i++) {
    // Could allow an array of defs here also.
    defs[i] = lupb_def_checkmutable(L, i + 1);
  }
  upb_status s = UPB_STATUS_INIT;
  upb_def_freeze(defs, n, &s);
  free(defs);
  lupb_checkstatus(L, &s);
  return 0;
}

static const struct luaL_Reg lupb_toplevel_m[] = {
  {"EnumDef", lupb_enumdef_new},
  {"FieldDef", lupb_fielddef_new},
  {"MessageDef", lupb_msgdef_new},
  {"SymbolTable", lupb_symtab_new},
  {"freeze", lupb_def_freeze},

  {NULL, NULL}
};

// Register the given type with the given methods and metamethods.
static void lupb_register_type(lua_State *L, const char *name,
                               const luaL_Reg *m, const luaL_Reg *mm) {
  luaL_newmetatable(L, name);
  lupb_setfuncs(L, mm);  // Register all mm in the metatable.
  lua_createtable(L, 0, 0);
  // Methods go in the mt's __index method.  This implies that you can't
  // implement __index.
  lupb_setfuncs(L, m);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);  // The mt.
}

static void lupb_setfieldi(lua_State *L, const char *field, int i) {
  lua_pushnumber(L, i);
  lua_setfield(L, -2, field);
}

int luaopen_upb(lua_State *L) {
  lupb_register_type(L, LUPB_MSGDEF,   lupb_msgdef_m,   lupb_msgdef_mm);
  lupb_register_type(L, LUPB_ENUMDEF,  lupb_enumdef_m,  lupb_enumdef_mm);
  lupb_register_type(L, LUPB_FIELDDEF, lupb_fielddef_m, lupb_fielddef_mm);
  lupb_register_type(L, LUPB_SYMTAB,   lupb_symtab_m,   lupb_symtab_mm);

  // Create our object cache.
  lua_newtable(L);
  lua_createtable(L, 0, 1);  // Cache metatable.
  lua_pushstring(L, "v");    // Values are weak.
  lua_setfield(L, -2, "__mode");
  lua_setmetatable(L, -2);
  lua_setfield(L, LUA_REGISTRYINDEX, LUPB_OBJCACHE);

  lupb_newlib(L, "upb", lupb_toplevel_m);

  // Define a couple functions as Lua source (kept here instead of a separate
  // Lua file so that upb.so is self-contained)
  const char *lua_source =
    "return function(upb)\n"
    "  upb.build_defs = function(defs)\n"
    "    local symtab = upb.SymbolTable(defs)\n"
    "    return symtab:getdefs(upb.DEF_ANY)\n"
    "  end\n"
    "end";

  if (luaL_dostring(L, lua_source) != 0)
    lua_error(L);

  // Call the chunk that will define the extra functions on upb, passing our
  // package dictionary as the argument.
  lua_pushvalue(L, -2);
  lua_call(L, 1, 0);

  // Register constants.
  lupb_setfieldi(L, "LABEL_OPTIONAL", UPB_LABEL(OPTIONAL));
  lupb_setfieldi(L, "LABEL_REQUIRED", UPB_LABEL(REQUIRED));
  lupb_setfieldi(L, "LABEL_REPEATED", UPB_LABEL(REPEATED));

  lupb_setfieldi(L, "TYPE_DOUBLE",    UPB_TYPE(DOUBLE));
  lupb_setfieldi(L, "TYPE_FLOAT",     UPB_TYPE(FLOAT));
  lupb_setfieldi(L, "TYPE_INT64",     UPB_TYPE(INT64));
  lupb_setfieldi(L, "TYPE_UINT64",    UPB_TYPE(UINT64));
  lupb_setfieldi(L, "TYPE_INT32",     UPB_TYPE(INT32));
  lupb_setfieldi(L, "TYPE_FIXED64",   UPB_TYPE(FIXED64));
  lupb_setfieldi(L, "TYPE_FIXED32",   UPB_TYPE(FIXED32));
  lupb_setfieldi(L, "TYPE_BOOL",      UPB_TYPE(BOOL));
  lupb_setfieldi(L, "TYPE_STRING",    UPB_TYPE(STRING));
  lupb_setfieldi(L, "TYPE_GROUP",     UPB_TYPE(GROUP));
  lupb_setfieldi(L, "TYPE_MESSAGE",   UPB_TYPE(MESSAGE));
  lupb_setfieldi(L, "TYPE_BYTES",     UPB_TYPE(BYTES));
  lupb_setfieldi(L, "TYPE_UINT32",    UPB_TYPE(UINT32));
  lupb_setfieldi(L, "TYPE_ENUM",      UPB_TYPE(ENUM));
  lupb_setfieldi(L, "TYPE_SFIXED32",  UPB_TYPE(SFIXED32));
  lupb_setfieldi(L, "TYPE_SFIXED64",  UPB_TYPE(SFIXED64));
  lupb_setfieldi(L, "TYPE_SINT32",    UPB_TYPE(SINT32));
  lupb_setfieldi(L, "TYPE_SINT64",    UPB_TYPE(SINT64));

  lupb_setfieldi(L, "DEF_MSG",      UPB_DEF_MSG);
  lupb_setfieldi(L, "DEF_FIELD",    UPB_DEF_FIELD);
  lupb_setfieldi(L, "DEF_ENUM",     UPB_DEF_ENUM);
  lupb_setfieldi(L, "DEF_SERVICE",  UPB_DEF_SERVICE);
  lupb_setfieldi(L, "DEF_ANY",      UPB_DEF_ANY);

  return 1;  // Return package table.
}

// Alternate names so that the library can be loaded as upb5_1 etc.
int LUPB_OPENFUNC(upb)(lua_State *L) { return luaopen_upb(L); }
