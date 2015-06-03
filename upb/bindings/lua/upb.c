/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A Lua extension for upb.  Exposes only the core library
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

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "lauxlib.h"
#include "upb/bindings/lua/upb.h"
#include "upb/handlers.h"
#include "upb/pb/glue.h"
#include "upb/shim/shim.h"

static const char upb_lua[] = {
#include "upb/bindings/lua/upb.lua.h"
};

// Lua metatable types.
#define LUPB_MSG "lupb.msg"
#define LUPB_ARRAY "lupb.array"
#define LUPB_MSGDEF "lupb.msgdef"
#define LUPB_ENUMDEF "lupb.enumdef"
#define LUPB_FIELDDEF "lupb.fielddef"
#define LUPB_SYMTAB "lupb.symtab"

// Other table constants.
#define LUPB_OBJCACHE "lupb.objcache"

static void lupb_msgdef_init(lua_State *L);
static size_t lupb_msgdef_sizeof();

/* Lua compatibility code *****************************************************/

// Lua 5.1 and Lua 5.2 have slightly incompatible APIs.  A little bit of
// compatibility code can help hide the difference.  Not too many people still
// use Lua 5.1 but LuaJIT uses the Lua 5.1 API in some ways.

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

static void lupb_newlib(lua_State *L, const char *name, const luaL_Reg *funcs) {
  luaL_register(L, name, funcs);
}

#define lupb_setfuncs(L, l) luaL_register(L, NULL, l)

#elif LUA_VERSION_NUM == 502

int luaL_typerror(lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}

static void lupb_newlib(lua_State *L, const char *name, const luaL_Reg *funcs) {
  // Lua 5.2 modules are not expected to set a global variable, so "name" is
  // unused.
  UPB_UNUSED(name);

  // Can't use luaL_newlib(), because funcs is not the actual array.
  // Could (micro-)optimize this a bit to count funcs for initial table size.
  lua_createtable(L, 0, 8);
  luaL_setfuncs(L, funcs, 0);
}

#define lupb_setfuncs(L, l) luaL_setfuncs(L, l, 0)

#else
#error Only Lua 5.1 and 5.2 are supported
#endif

// Shims for upcoming Lua 5.3 functionality.
bool lua_isinteger(lua_State *L, int argn) {
  UPB_UNUSED(L);
  UPB_UNUSED(argn);
  return false;
}


/* Utility functions **********************************************************/

// We store our module table in the registry, keyed by ptr.
// For more info about the motivation/rationale, see this thread:
//   http://thread.gmane.org/gmane.comp.lang.lua.general/110632
bool lupb_openlib(lua_State *L, void *ptr, const char *name,
                  const luaL_Reg *funcs) {
  // Lookup cached module table.
  lua_pushlightuserdata(L, ptr);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (!lua_isnil(L, -1)) {
    return true;
  }

  lupb_newlib(L, name, funcs);

  // Save module table in cache.
  lua_pushlightuserdata(L, ptr);
  lua_pushvalue(L, -2);
  lua_rawset(L, LUA_REGISTRYINDEX);

  return false;
}

// Pushes a new userdata with the given metatable and ensures that it has a
// uservalue.
static void *newudata_with_userval(lua_State *L, size_t size,
                                   const char *type) {
  void *ret = lua_newuserdata(L, size);

  // Set metatable.
  luaL_getmetatable(L, type);
  assert(!lua_isnil(L, -1));  // Should have been created by luaopen_upb.
  lua_setmetatable(L, -2);

  lua_newtable(L);
  lua_setuservalue(L, -2);

  return ret;
}

const char *lupb_checkname(lua_State *L, int narg) {
  size_t len;
  const char *name = luaL_checklstring(L, narg, &len);
  if (strlen(name) != len)
    luaL_error(L, "names cannot have embedded NULLs");
  return name;
}

bool lupb_checkbool(lua_State *L, int narg) {
  if (!lua_isboolean(L, narg)) {
    luaL_error(L, "must be true or false");
  }
  return lua_toboolean(L, narg);
}

// Unlike luaL_checkstring(), this does not allow implicit conversion to string.
void lupb_checkstring(lua_State *L, int narg) {
  if (lua_type(L, narg) != LUA_TSTRING)
    luaL_error(L, "Expected string");
}

// Unlike luaL_checkinteger, these do not implicitly convert from string or
// round an existing double value.  We allow floating-point input, but only if
// the actual value is integral.
#define INTCHECK(type, ctype)                                                  \
  ctype lupb_check##type(lua_State *L, int narg) {                             \
    if (lua_isinteger(L, narg)) {                                              \
      return lua_tointeger(L, narg);                                           \
    }                                                                          \
                                                                               \
    /* Prevent implicit conversion from string. */                             \
    luaL_checktype(L, narg, LUA_TNUMBER);                                      \
    double n = lua_tonumber(L, narg);                                          \
                                                                               \
    ctype i = (ctype)n;                                                        \
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

INTCHECK(int64,  int64_t);
INTCHECK(int32,  int32_t);
INTCHECK(uint64, uint64_t);
INTCHECK(uint32, uint32_t);

double lupb_checkdouble(lua_State *L, int narg) {
  // If we were being really hard-nosed here, we'd check whether the input was
  // an integer that has no precise double representation.  But doubles aren't
  // generally expected to be exact like integers are, and worse this could
  // cause data-dependent runtime errors: one run of the program could work fine
  // because the integer calculations happened to be exactly representable in
  // double, while the next could crash because of subtly different input.

  luaL_checktype(L, narg, LUA_TNUMBER);  // lua_tonumber() implicitly converts.
  return lua_tonumber(L, narg);
}

float lupb_checkfloat(lua_State *L, int narg) {
  // We don't worry about checking whether the input can be exactly converted to
  // float -- see above.

  luaL_checktype(L, narg, LUA_TNUMBER);  // lua_tonumber() implicitly converts.
  return lua_tonumber(L, narg);
}

void lupb_pushdouble(lua_State *L, double d) {
  lua_pushnumber(L, d);
}

void lupb_pushfloat(lua_State *L, float d) {
  lua_pushnumber(L, d);
}

static void lupb_checkval(lua_State *L, int narg, upb_fieldtype_t type) {
  switch(type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      lupb_checkint32(L, narg);
      break;
    case UPB_TYPE_INT64:
      lupb_checkint64(L, narg);
      break;
    case UPB_TYPE_UINT32:
      lupb_checkuint32(L, narg);
      break;
    case UPB_TYPE_UINT64:
      lupb_checkuint64(L, narg);
      break;
    case UPB_TYPE_DOUBLE:
      lupb_checkdouble(L, narg);
      break;
    case UPB_TYPE_FLOAT:
      lupb_checkfloat(L, narg);
      break;
    case UPB_TYPE_BOOL:
      lupb_checkbool(L, narg);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      lupb_checkstring(L, narg);
      break;
    case UPB_TYPE_MESSAGE:
      lupb_assert(L, false);
  }
}

void lupb_checkstatus(lua_State *L, upb_status *s) {
  if (!upb_ok(s)) {
    lua_pushstring(L, upb_status_errmsg(s));
    lua_error(L);
  }
}

static upb_fieldtype_t lupb_checkfieldtype(lua_State *L, int narg) {
  int type = luaL_checkint(L, narg);
  if (!upb_fielddef_checktype(type))
    luaL_argerror(L, narg, "invalid field type");
  return type;
}

#define CHK(pred) do { \
    upb_status status = UPB_STATUS_INIT; \
    pred; \
    lupb_checkstatus(L, &status); \
  } while (0)


/* lupb_refcounted ************************************************************/

// All upb objects that use upb_refcounted have a userdata that begins with a
// pointer to that object.  Each type has its own metatable.  Objects are cached
// in a weak table indexed by the C pointer of the object they are caching.
//
// Note that we consistently use memcpy() to read to/from the object.  This
// allows the userdata to use its own struct without violating aliasing, as
// long as it begins with a pointer.

// Checks type; if it matches, pulls the pointer out of the wrapper.
void *lupb_refcounted_check(lua_State *L, int narg, const char *type) {
  void *ud = luaL_checkudata(L, narg, type);
  void *ret;
  memcpy(&ret, ud, sizeof ret);
  if (!ret) luaL_error(L, "called into dead object");
  return ret;
}

bool lupb_refcounted_pushwrapper(lua_State *L, const upb_refcounted *obj,
                                 const char *type, const void *ref_donor,
                                 size_t size) {
  if (obj == NULL) {
    lua_pushnil(L);
    return false;
  }

  // Lookup our cache in the registry (we don't put our objects in the registry
  // directly because we need our cache to be a weak table).
  lua_getfield(L, LUA_REGISTRYINDEX, LUPB_OBJCACHE);
  assert(!lua_isnil(L, -1));  // Should have been created by luaopen_upb.
  lua_pushlightuserdata(L, (void*)obj);
  lua_rawget(L, -2);
  // Stack is now: objcache, cached value.

  bool create = false;

  if (lua_isnil(L, -1)) {
    create = true;
  } else {
    void *ud = lua_touserdata(L, -1);
    lupb_assert(L, ud);
    void *ud_obj;
    memcpy(&ud_obj, ud, sizeof(void*));

    // A corner case: it is possible for the value to be GC'd
    // already, in which case we should evict this entry and create
    // a new one.
    if (ud_obj == NULL) {
      create = true;
    }
  }

  void *ud = NULL;

  if (create) {
    // Remove bad cached value and push new value.
    lua_pop(L, 1);

    // All of our userdata begin with a pointer to the obj.
    ud = lua_newuserdata(L, size);
    memcpy(ud, &obj, sizeof(void*));
    upb_refcounted_donateref(obj, ref_donor, ud);

    luaL_getmetatable(L, type);
    // Should have been created by luaopen_upb.
    lupb_assert(L, !lua_isnil(L, -1));
    lua_setmetatable(L, -2);

    // Set it in the cache.
    lua_pushlightuserdata(L, (void*)obj);
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
  } else {
    // Existing wrapper obj already has a ref.
    ud = lua_touserdata(L, -1);
    upb_refcounted_checkref(obj, ud);
    if (ref_donor)
      upb_refcounted_unref(obj, ref_donor);
  }

  lua_insert(L, -2);
  lua_pop(L, 1);
  return create;
}

void lupb_refcounted_pushnewrapper(lua_State *L, const upb_refcounted *obj,
                                   const char *type, const void *ref_donor) {
  bool created =
      lupb_refcounted_pushwrapper(L, obj, type, ref_donor, sizeof(void *));
  UPB_ASSERT_VAR(created, created == true);
}

static int lupb_refcounted_gc(lua_State *L) {
  void *ud = lua_touserdata(L, 1);
  upb_refcounted *obj;
  memcpy(&obj, ud, sizeof obj);
  upb_refcounted_unref(obj, ud);

  // Zero out pointer so we can detect a call into a GC'd object.
  void *nullp = NULL;
  memcpy(ud, &nullp, sizeof nullp);

  return 0;
}

static const struct luaL_Reg lupb_refcounted_mm[] = {
  {"__gc", lupb_refcounted_gc},
  {NULL, NULL}
};


/* lupb_def *******************************************************************/

static const upb_def *lupb_def_check(lua_State *L, int narg) {
  void *ud = luaL_testudata(L, narg, LUPB_MSGDEF);
  if (!ud) ud = luaL_testudata(L, narg, LUPB_ENUMDEF);
  if (!ud) ud = luaL_testudata(L, narg, LUPB_FIELDDEF);
  if (!ud) luaL_typerror(L, narg, "upb def");

  upb_def *ret;
  memcpy(&ret, ud, sizeof ret);
  if (!ret) luaL_error(L, "called into dead object");
  return ret;
}

static upb_def *lupb_def_checkmutable(lua_State *L, int narg) {
  const upb_def *def = lupb_def_check(L, narg);
  if (upb_def_isfrozen(def))
    luaL_error(L, "not allowed on frozen value");
  return (upb_def*)def;
}

bool lupb_def_pushwrapper(lua_State *L, const upb_def *def,
                          const void *ref_donor) {
  if (def == NULL) {
    lua_pushnil(L);
    return false;
  }

  const char *type = NULL;
  size_t size = sizeof(void*);

  switch (upb_def_type(def)) {
    case UPB_DEF_MSG: {
      type = LUPB_MSGDEF;
      size = lupb_msgdef_sizeof();
      break;
    }
    case UPB_DEF_ENUM: type = LUPB_ENUMDEF; break;
    case UPB_DEF_FIELD: type = LUPB_FIELDDEF; break;
    default: luaL_error(L, "unknown deftype %d", def->type);
  }

  bool created =
      lupb_refcounted_pushwrapper(L, UPB_UPCAST(def), type, ref_donor, size);

  if (created && upb_def_type(def) == UPB_DEF_MSG) {
    lupb_msgdef_init(L);
  }

  return created;
}

void lupb_def_pushnewrapper(lua_State *L, const upb_def *def,
                            const void *ref_donor) {
  bool created = lupb_def_pushwrapper(L, def, ref_donor);
  UPB_ASSERT_VAR(created, created == true);
}

static int lupb_def_type(lua_State *L) {
  const upb_def *def = lupb_def_check(L, 1);
  lua_pushinteger(L, upb_def_type(def));
  return 1;
}

static int lupb_def_freeze(lua_State *L) {
  upb_def *def = lupb_def_checkmutable(L, 1);
  CHK(upb_def_freeze(&def, 1, &status));
  return 0;
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
  const char *name = lupb_checkname(L, 2);
  CHK(upb_def_setfullname(lupb_def_checkmutable(L, 1), name, &status));
  return 0;
}

#define LUPB_COMMON_DEF_METHODS \
  {"def_type", lupb_def_type},  \
  {"full_name", lupb_def_fullname}, \
  {"freeze", lupb_def_freeze}, \
  {"is_frozen", lupb_def_isfrozen}, \
  {"set_full_name", lupb_def_setfullname}, \


/* lupb_fielddef **************************************************************/

const upb_fielddef *lupb_fielddef_check(lua_State *L, int narg) {
  return lupb_refcounted_check(L, narg, LUPB_FIELDDEF);
}

static upb_fielddef *lupb_fielddef_checkmutable(lua_State *L, int narg) {
  const upb_fielddef *f = lupb_fielddef_check(L, narg);
  if (upb_fielddef_isfrozen(f))
    luaL_error(L, "not allowed on frozen value");
  return (upb_fielddef*)f;
}

static int lupb_fielddef_new(lua_State *L) {
  upb_fielddef *f = upb_fielddef_new(&f);
  lupb_def_pushnewrapper(L, UPB_UPCAST(f), &f);
  return 1;
}

// Getters

static int lupb_fielddef_containingtype(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_def_pushwrapper(L, UPB_UPCAST(upb_fielddef_containingtype(f)), NULL);
  return 1;
}

static int lupb_fielddef_containingtypename(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  lua_pushstring(L, upb_fielddef_containingtypename(f));
  return 1;
}

static int lupb_fielddef_default(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    int32:
      lupb_pushint32(L, upb_fielddef_defaultint32(f)); break;
    case UPB_TYPE_INT64:
      lupb_pushint64(L, upb_fielddef_defaultint64(f)); break;
    case UPB_TYPE_UINT32:
      lupb_pushuint32(L, upb_fielddef_defaultuint32(f)); break;
    case UPB_TYPE_UINT64:
      lupb_pushuint64(L, upb_fielddef_defaultuint64(f)); break;
    case UPB_TYPE_DOUBLE:
      lua_pushnumber(L, upb_fielddef_defaultdouble(f)); break;
    case UPB_TYPE_FLOAT:
      lua_pushnumber(L, upb_fielddef_defaultfloat(f)); break;
    case UPB_TYPE_BOOL:
      lua_pushboolean(L, upb_fielddef_defaultbool(f)); break;
    case UPB_TYPE_ENUM:
      if (upb_fielddef_enumhasdefaultstr(f)) {
        goto str;
      } else if (upb_fielddef_enumhasdefaultint32(f)) {
        goto int32;
      } else {
        lua_pushnil(L);
      }
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    str: {
      size_t len;
      const char *data = upb_fielddef_defaultstr(f, &len);
      lua_pushlstring(L, data, len);
      break;
    }
    case UPB_TYPE_MESSAGE:
      return luaL_error(L, "Message fields do not have explicit defaults.");
  }
  return 1;
}

static int lupb_fielddef_getsel(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  upb_selector_t sel;
  if (upb_handlers_getselector(f, luaL_checknumber(L, 2), &sel)) {
    lua_pushinteger(L, sel);
    return 1;
  } else {
    return 0;
  }
}

static int lupb_fielddef_hassubdef(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushboolean(L, upb_fielddef_hassubdef(f));
  return 1;
}

static int lupb_fielddef_index(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushinteger(L, upb_fielddef_index(f));
  return 1;
}

static int lupb_fielddef_intfmt(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushinteger(L, upb_fielddef_intfmt(f));
  return 1;
}

static int lupb_fielddef_isextension(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushboolean(L, upb_fielddef_isextension(f));
  return 1;
}

static int lupb_fielddef_istagdelim(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushboolean(L, upb_fielddef_istagdelim(f));
  return 1;
}

static int lupb_fielddef_label(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushinteger(L, upb_fielddef_label(f));
  return 1;
}

static int lupb_fielddef_lazy(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushboolean(L, upb_fielddef_lazy(f));
  return 1;
}

static int lupb_fielddef_name(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushstring(L, upb_fielddef_name(f));
  return 1;
}

static int lupb_fielddef_number(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  int32_t num = upb_fielddef_number(f);
  if (num)
    lua_pushinteger(L, num);
  else
    lua_pushnil(L);
  return 1;
}

static int lupb_fielddef_packed(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushboolean(L, upb_fielddef_packed(f));
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
  if (upb_fielddef_typeisset(f))
    lua_pushinteger(L, upb_fielddef_type(f));
  else
    lua_pushnil(L);
  return 1;
}

// Setters

static int lupb_fielddef_setcontainingtypename(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  const char *name = NULL;
  if (!lua_isnil(L, 2))
    name = lupb_checkname(L, 2);
  CHK(upb_fielddef_setcontainingtypename(f, name, &status));
  return 0;
}

static int lupb_fielddef_setdefault(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
      upb_fielddef_setdefaultint32(f, lupb_checkint32(L, 2));
      break;
    case UPB_TYPE_INT64:
      upb_fielddef_setdefaultint64(f, lupb_checkint64(L, 2));
      break;
    case UPB_TYPE_UINT32:
      upb_fielddef_setdefaultuint32(f, lupb_checkuint32(L, 2));
      break;
    case UPB_TYPE_UINT64:
      upb_fielddef_setdefaultuint64(f, lupb_checkuint64(L, 2));
      break;
    case UPB_TYPE_DOUBLE:
      upb_fielddef_setdefaultdouble(f, lupb_checkdouble(L, 2));
      break;
    case UPB_TYPE_FLOAT:
      upb_fielddef_setdefaultfloat(f, lupb_checkfloat(L, 2));
      break;
    case UPB_TYPE_BOOL:
      upb_fielddef_setdefaultbool(f, lupb_checkbool(L, 2));
      break;
    case UPB_TYPE_MESSAGE:
      return luaL_error(L, "Message types cannot have defaults.");
    case UPB_TYPE_ENUM:
      if (lua_type(L, 2) != LUA_TSTRING) {
        upb_fielddef_setdefaultint32(f, lupb_checkint32(L, 2));
        break;
      }
      // Else fall through and set string default.
    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING: {
      size_t len;
      const char *str = lua_tolstring(L, 2, &len);
      CHK(upb_fielddef_setdefaultstr(f, str, len, &status));
    }
  }
  return 0;
}

static int lupb_fielddef_setisextension(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  CHK(upb_fielddef_setisextension(f, lupb_checkbool(L, 2)));
  return 0;
}

static int lupb_fielddef_setlabel(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  int label = luaL_checkint(L, 2);
  if (!upb_fielddef_checklabel(label))
    luaL_argerror(L, 2, "invalid field label");
  upb_fielddef_setlabel(f, label);
  return 0;
}

static int lupb_fielddef_setlazy(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  upb_fielddef_setlazy(f, lupb_checkbool(L, 2));
  return 0;
}

static int lupb_fielddef_setname(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  CHK(upb_fielddef_setname(f, lupb_checkname(L, 2), &status));
  return 0;
}

static int lupb_fielddef_setnumber(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  CHK(upb_fielddef_setnumber(f, luaL_checkint(L, 2), &status));
  return 0;
}

static int lupb_fielddef_setpacked(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  upb_fielddef_setpacked(f, lupb_checkbool(L, 2));
  return 0;
}

static int lupb_fielddef_setsubdef(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  const upb_def *def = NULL;
  if (!lua_isnil(L, 2))
    def = lupb_def_check(L, 2);
  CHK(upb_fielddef_setsubdef(f, def, &status));
  return 0;
}

static int lupb_fielddef_setsubdefname(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  const char *name = NULL;
  if (!lua_isnil(L, 2))
    name = lupb_checkname(L, 2);
  CHK(upb_fielddef_setsubdefname(f, name, &status));
  return 0;
}

static int lupb_fielddef_settype(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  upb_fielddef_settype(f, lupb_checkfieldtype(L, 2));
  return 0;
}

static int lupb_fielddef_setintfmt(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  int32_t intfmt = luaL_checknumber(L, 2);
  if (!upb_fielddef_checkintfmt(intfmt))
    luaL_argerror(L, 2, "invalid intfmt");
  upb_fielddef_setintfmt(f, intfmt);
  return 0;
}

static int lupb_fielddef_settagdelim(lua_State *L) {
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 1);
  bool is_tag_delim = lupb_checkbool(L, 2);
  CHK(upb_fielddef_settagdelim(f, is_tag_delim));
  return 0;
}

static int lupb_fielddef_selectorbase(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  if (!upb_fielddef_isfrozen(f))
    luaL_error(L, "_selectorbase is only defined for frozen fielddefs");
  lua_pushinteger(L, f->selector_base);
  return 1;
}

static const struct luaL_Reg lupb_fielddef_m[] = {
  LUPB_COMMON_DEF_METHODS

  {"containing_type", lupb_fielddef_containingtype},
  {"containing_type_name", lupb_fielddef_containingtypename},
  {"default", lupb_fielddef_default},
  {"getsel", lupb_fielddef_getsel},
  {"has_subdef", lupb_fielddef_hassubdef},
  {"index", lupb_fielddef_index},
  {"intfmt", lupb_fielddef_intfmt},
  {"is_extension", lupb_fielddef_isextension},
  {"istagdelim", lupb_fielddef_istagdelim},
  {"label", lupb_fielddef_label},
  {"lazy", lupb_fielddef_lazy},
  {"name", lupb_fielddef_name},
  {"number", lupb_fielddef_number},
  {"packed", lupb_fielddef_packed},
  {"subdef", lupb_fielddef_subdef},
  {"subdef_name", lupb_fielddef_subdefname},
  {"type", lupb_fielddef_type},

  {"set_containing_type_name", lupb_fielddef_setcontainingtypename},
  {"set_default", lupb_fielddef_setdefault},
  {"set_is_extension", lupb_fielddef_setisextension},
  {"set_label", lupb_fielddef_setlabel},
  {"set_lazy", lupb_fielddef_setlazy},
  {"set_name", lupb_fielddef_setname},
  {"set_number", lupb_fielddef_setnumber},
  {"set_packed", lupb_fielddef_setpacked},
  {"set_subdef", lupb_fielddef_setsubdef},
  {"set_subdef_name", lupb_fielddef_setsubdefname},
  {"set_type", lupb_fielddef_settype},
  {"set_intfmt", lupb_fielddef_setintfmt},
  {"set_tagdelim", lupb_fielddef_settagdelim},

  // Internal-only.
  {"_selector_base", lupb_fielddef_selectorbase},

  {NULL, NULL}
};


/* lupb_msgdef ****************************************************************/

typedef struct {
  const upb_msgdef *md;

  // These members are initialized lazily the first time a message is created
  // for this def.
  uint16_t *field_offsets;
  size_t msg_size;
  size_t hasbits_size;
  lua_State *L;
} lupb_msgdef;

static size_t lupb_msgdef_sizeof() {
  return sizeof(lupb_msgdef);
}

const upb_msgdef *lupb_msgdef_check(lua_State *L, int narg) {
  return lupb_refcounted_check(L, narg, LUPB_MSGDEF);
}

lupb_msgdef *lupb_msgdef_check2(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_MSGDEF);
}

static upb_msgdef *lupb_msgdef_checkmutable(lua_State *L, int narg) {
  const upb_msgdef *m = lupb_msgdef_check(L, narg);
  if (upb_msgdef_isfrozen(m))
    luaL_error(L, "not allowed on frozen value");
  return (upb_msgdef*)m;
}

static int lupb_msgdef_new(lua_State *L) {
  upb_msgdef *md = upb_msgdef_new(&md);
  lupb_def_pushnewrapper(L, UPB_UPCAST(md), &md);
  return 1;
}

// Unlike other refcounted types we need a custom __gc so that we free our field
// offsets.
static int lupb_msgdef_gc(lua_State *L) {
  lupb_refcounted_gc(L);
  lupb_msgdef *lmd = luaL_checkudata(L, -1, LUPB_MSGDEF);
  free(lmd->field_offsets);
  return 0;
}

static void lupb_msgdef_init(lua_State *L) {
  lupb_msgdef *lmd = luaL_checkudata(L, -1, LUPB_MSGDEF);
  lmd->L = L;
  lmd->field_offsets = NULL;
}

static int lupb_msgdef_add(lua_State *L) {
  upb_msgdef *m = lupb_msgdef_checkmutable(L, 1);
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 2);
  CHK(upb_msgdef_addfield(m, f, NULL, &status));
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

static int lupb_msgdef_submsgfieldcount(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushinteger(L, m->submsg_field_count);
  return 1;
}

static int lupb_msgdef_field(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  int type = lua_type(L, 2);
  const upb_fielddef *f;
  if (type == LUA_TNUMBER) {
    f = upb_msgdef_itof(m, lua_tointeger(L, 2));
  } else if (type == LUA_TSTRING) {
    f = upb_msgdef_ntofz(m, lua_tostring(L, 2));
  } else {
    const char *msg = lua_pushfstring(L, "number or string expected, got %s",
                                      luaL_typename(L, 2));
    return luaL_argerror(L, 2, msg);
  }

  lupb_def_pushwrapper(L, UPB_UPCAST(f), NULL);
  return 1;
}

static int lupb_msgiter_next(lua_State *L) {
  upb_msg_field_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_msg_field_done(i)) return 0;
  lupb_def_pushwrapper(L, UPB_UPCAST(upb_msg_iter_field(i)), NULL);
  upb_msg_field_next(i);
  return 1;
}

static int lupb_msgdef_fields(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  upb_msg_field_iter *i = lua_newuserdata(L, sizeof(upb_msg_field_iter));
  upb_msg_field_begin(i, m);
  // Need to guarantee that the msgdef outlives the iter.
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, &lupb_msgiter_next, 2);
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
  {"_submsg_field_count", lupb_msgdef_submsgfieldcount},

  {NULL, NULL}
};


/* lupb_enumdef ***************************************************************/

const upb_enumdef *lupb_enumdef_check(lua_State *L, int narg) {
  return lupb_refcounted_check(L, narg, LUPB_ENUMDEF);
}

static upb_enumdef *lupb_enumdef_checkmutable(lua_State *L, int narg) {
  const upb_enumdef *f = lupb_enumdef_check(L, narg);
  if (upb_enumdef_isfrozen(f))
    luaL_error(L, "not allowed on frozen value");
  return (upb_enumdef*)f;
}

static int lupb_enumdef_new(lua_State *L) {
  upb_enumdef *e = upb_enumdef_new(&e);
  lupb_def_pushnewrapper(L, UPB_UPCAST(e), &e);
  return 1;
}

static int lupb_enumdef_add(lua_State *L) {
  upb_enumdef *e = lupb_enumdef_checkmutable(L, 1);
  const char *name = lupb_checkname(L, 2);
  int32_t val = lupb_checkint32(L, 3);
  CHK(upb_enumdef_addval(e, name, val, &status));
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
    int32_t key = lupb_checkint32(L, 2);
    lua_pushstring(L, upb_enumdef_iton(e, key));
  } else if (type == LUA_TSTRING) {
    const char *key = lua_tostring(L, 2);
    int32_t num;
    if (upb_enumdef_ntoiz(e, key, &num)) {
      lua_pushinteger(L, num);
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
  lua_pushinteger(L, upb_enum_iter_number(i));
  upb_enum_next(i);
  return 2;
}

static int lupb_enumdef_values(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);
  upb_enum_iter *i = lua_newuserdata(L, sizeof(upb_enum_iter));
  upb_enum_begin(i, e);
  // Need to guarantee that the enumdef outlives the iter.
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, &lupb_enumiter_next, 2);
  return 1;
}

static const struct luaL_Reg lupb_enumdef_mm[] = {
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
const upb_symtab *lupb_symtab_check(lua_State *L, int narg) {
  return lupb_refcounted_check(L, narg, LUPB_SYMTAB);
}

static upb_symtab *lupb_symtab_checkmutable(lua_State *L, int narg) {
  const upb_symtab *s = lupb_symtab_check(L, narg);
  if (upb_symtab_isfrozen(s))
    luaL_error(L, "not allowed on frozen value");
  return (upb_symtab*)s;
}

void lupb_symtab_pushwrapper(lua_State *L, const upb_symtab *s,
                             const void *ref_donor) {
  lupb_refcounted_pushwrapper(L, UPB_UPCAST(s), LUPB_SYMTAB, ref_donor,
                              sizeof(void *));
}

void lupb_symtab_pushnewrapper(lua_State *L, const upb_symtab *s,
                               const void *ref_donor) {
  lupb_refcounted_pushnewrapper(L, UPB_UPCAST(s), LUPB_SYMTAB, ref_donor);
}

static int lupb_symtab_new(lua_State *L) {
  upb_symtab *s = upb_symtab_new(&s);
  lupb_symtab_pushnewrapper(L, s, &s);
  return 1;
}

static int lupb_symtab_freeze(lua_State *L) {
  upb_symtab_freeze(lupb_symtab_checkmutable(L, 1));
  return 0;
}

static int lupb_symtab_isfrozen(lua_State *L) {
  lua_pushboolean(L, upb_symtab_isfrozen(lupb_symtab_check(L, 1)));
  return 1;
}

static int lupb_symtab_add(lua_State *L) {
  upb_symtab *s = lupb_symtab_checkmutable(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  // Iterate over table twice.  First iteration to count entries and
  // check constraints.
  int n = 0;
  for (lua_pushnil(L); lua_next(L, 2); lua_pop(L, 1)) {
    lupb_def_checkmutable(L, -1);
    ++n;
  }

  // Second iteration to build deflist.
  // Allocate list with lua_newuserdata() so it is anchored as a GC root in
  // case any Lua functions longjmp().
  upb_def **defs = lua_newuserdata(L, n * sizeof(*defs));
  n = 0;
  for (lua_pushnil(L); lua_next(L, 2); lua_pop(L, 1)) {
    upb_def *def = lupb_def_checkmutable(L, -1);
    defs[n++] = def;
  }

  CHK(upb_symtab_add(s, defs, n, NULL, &status));
  return 0;
}

static int lupb_symtab_lookup(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  for (int i = 2; i <= lua_gettop(L); i++) {
    const upb_def *def = upb_symtab_lookup(s, luaL_checkstring(L, i));
    lupb_def_pushwrapper(L, def, NULL);
    lua_replace(L, i);
  }
  return lua_gettop(L) - 1;
}

static int lupb_symtabiter_next(lua_State *L) {
  upb_symtab_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_symtab_done(i)) return 0;
  lupb_def_pushwrapper(L, upb_symtab_iter_def(i), NULL);
  upb_symtab_next(i);
  return 1;
}

static int lupb_symtab_defs(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  upb_deftype_t type = lua_gettop(L) > 1 ? luaL_checkint(L, 2) : UPB_DEF_ANY;
  upb_symtab_iter *i = lua_newuserdata(L, sizeof(upb_symtab_iter));
  upb_symtab_begin(i, s, type);
  // Need to guarantee that the symtab outlives the iter.
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, &lupb_symtabiter_next, 2);
  return 1;
}

// This is a *temporary* API that will be removed once pending refactorings are
// complete (it does not belong here in core because it depends on both
// the descriptor.proto schema and the protobuf binary format.
static int lupb_symtab_load_descriptor(lua_State *L) {
  size_t len;
  upb_symtab *s = lupb_symtab_checkmutable(L, 1);
  const char *str = luaL_checklstring(L, 2, &len);
  CHK(upb_load_descriptor_into_symtab(s, str, len, &status));
  return 0;
}

static const struct luaL_Reg lupb_symtab_m[] = {
  {"add", lupb_symtab_add},
  {"defs", lupb_symtab_defs},
  {"freeze", lupb_symtab_freeze},
  {"is_frozen", lupb_symtab_isfrozen},
  {"lookup", lupb_symtab_lookup},
  {"load_descriptor", lupb_symtab_load_descriptor},
  {NULL, NULL}
};


/* lupb_array *****************************************************************/

// A lupb_array provides a strongly-typed array.
//
// For the moment we store all values in the userdata's environment table /
// userval, for simplicity.  Later we may wish to move the data into raw
// memory as both a space and time optimization.
//
// Compared to regular Lua tables:
//
// - we only allow integer indices.
// - all entries must match the type of the table.
// - we do not allow "holes" in the array; you can only assign to an existing
//   index or one past the end (which will grow the array by one).

typedef struct {
  uint32_t size;
  upb_fieldtype_t type;
  const upb_msgdef *msgdef;  // Only when type == UPB_TYPE_MESSAGE
} lupb_array;

static lupb_array *lupb_array_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_ARRAY);
}

static uint32_t lupb_array_checkindex(lua_State *L, int narg, uint32_t max) {
  uint32_t n = lupb_checkuint32(L, narg);
  if (n == 0 || n > max) {  // Lua uses 1-based indexing. :(
    luaL_error(L, "Invalid array index.");
  }
  return n;
}

static int lupb_array_new(lua_State *L) {
  lupb_array *array = newudata_with_userval(L, sizeof(*array), LUPB_ARRAY);
  array->size = 0;

  if (lua_type(L, 1) == LUA_TNUMBER) {
    array->type = lupb_checkfieldtype(L, 1);
    if (array->type == UPB_TYPE_MESSAGE) {
      return luaL_error(
          L, "For message arrays construct with the specific message type.");
    }
  } else {
    array->type = UPB_TYPE_MESSAGE;
    array->msgdef = lupb_msgdef_check(L, 1);

    // Store a reference to this msgdef in the environment table to ensure it
    // outlives this array.
    lua_getuservalue(L, -1);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 0);
    lua_pop(L, 1);  // Pop userval.
  }

  return 1;
}

static int lupb_array_newindex(lua_State *L) {
  lupb_array *array = lupb_array_check(L, 1);
  uint32_t n = lupb_array_checkindex(L, 2, array->size + 1);

  if (n == array->size + 1) {
    array->size++;
  }

  if (array->type == UPB_TYPE_MESSAGE) {
    if (array->msgdef != lupb_msg_checkdef(L, 3)) {
      return luaL_error(L, "Tried to assign wrong message type.");
    }
  } else {
    lupb_checkval(L, 3, array->type);
  }

  // Write value to userval table.
  lua_getuservalue(L, 1);
  lua_pushvalue(L, 3);
  lua_rawseti(L, -2, n);

  return 0;  // 1 for chained assignments?
}

static int lupb_array_index(lua_State *L) {
  lupb_array *array = lupb_array_check(L, 1);
  uint32_t n = lupb_array_checkindex(L, 2, array->size);

  lua_getuservalue(L, 1);
  lua_rawgeti(L, -1, n);
  return 1;
}

static int lupb_array_len(lua_State *L) {
  lupb_array *array = lupb_array_check(L, 1);
  lua_pushnumber(L, array->size);
  return 1;
}

static const struct luaL_Reg lupb_array_mm[] = {
  {"__index", lupb_array_index},
  {"__len", lupb_array_len},
  {"__newindex", lupb_array_newindex},
  {NULL, NULL}
};

/* lupb_msg **************************************************************/

// A lupb_msg is a userdata where:
//
// - the userdata's memory contains hasbits and primitive fields.
// - the userdata's environment table / uservalue contains references to string
//   fields, submessage fields, and array fields.

typedef struct {
  const lupb_msgdef *lmd;
  char data[];
} lupb_msg;

#define MSGDEF_INDEX 0

static bool in_userval(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) || upb_fielddef_issubmsg(f) ||
         upb_fielddef_isstring(f);
}

static size_t lupb_sizeof(lua_State *L, const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      return 1;
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_ENUM:
    case UPB_TYPE_FLOAT:
      return 4;
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_DOUBLE:
      return 8;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      break;
  }
  lupb_assert(L, false);
  return 0;
}

static int div_round_up(size_t n, size_t d) {
  int ret = n / d;
  // If there was a positive remainder, then the result was rounded down and we
  // need to compensate by adding one.
  if (n % d > 0) ++ret;
  return ret;
}

static size_t align_up(size_t val, size_t align) {
  return val % align == 0 ? val : val + align - (val % align);
}

// If we always read/write as a consistent type to each value, this shouldn't
// violate aliasing.
//
// Note that the slightly prettier option of:
//
//   *(type*)(&msg->data[ofs])
//
// ...is potentially more questionable wrt the C standard and aliasing.
// Does the expression &msg->data[ofs] "access the stored value"?  If so,
// this would violate aliasing.  So instead we use the expression:
//
//   (char*)msg + sizeof(lupb_msg) + ofs
//
// ...which unambigiously is doing nothing but calculating a pointer address.
#define DEREF(msg, ofs, type) *(type*)((char*)msg + sizeof(lupb_msg) + ofs)

lupb_msg *lupb_msg_check(lua_State *L, int narg) {
  lupb_msg *msg = luaL_checkudata(L, narg, LUPB_MSG);
  if (!msg->lmd) luaL_error(L, "called into dead msg");
  return msg;
}

const upb_msgdef *lupb_msg_checkdef(lua_State *L, int narg) {
  return lupb_msg_check(L, narg)->lmd->md;
}

static const upb_fielddef *lupb_msg_checkfield(lua_State *L,
                                               const lupb_msgdef *lmd,
                                               int fieldarg) {
  size_t len;
  const char *fieldname = luaL_checklstring(L, fieldarg, &len);
  const upb_fielddef *f = upb_msgdef_ntof(lmd->md, fieldname, len);

  if (!f) {
    const char *msg = lua_pushfstring(L, "no such field: %s", fieldname);
    luaL_argerror(L, fieldarg, msg);
    return NULL;  // Never reached.
  }

  return f;
}

// Assigns offsets for storing data in instances of messages for this type, if
// they have not already been assigned.  "narg" should be the stack location of
// a Lua msgdef object.  It should be frozen (if it is not, we will throw an
// error).  It should not throw errors in any other case, since we may have
// values on our stack that would leak if we longjmp'd across them.
//
// TODO(haberman): (if we want to avoid this and be robust against even lua
// errors due to OOM, we should stop using upb_handlers_newfrozen() and
// implement it ourselves with a Lua table as cache, since that would get
// cleaned up properly on error).
static lupb_msgdef *lupb_msg_assignoffsets(lua_State *L, int narg) {
  lupb_msgdef *lmd = lupb_msgdef_check2(L, narg);
  if (!upb_msgdef_isfrozen(lmd->md))
    luaL_error(L, "msgdef must be frozen");

  if (lmd->field_offsets) {
    // Already assigned.
    return lmd;
  }

  int n = upb_msgdef_numfields(lmd->md);
  uint16_t *offsets = malloc(sizeof(*offsets) * n);

  // Offset with the raw data part; starts with hasbits.
  size_t hasbits_size = div_round_up(n, 8);
  size_t data_ofs = hasbits_size;
  // Index within the userval.
  // Starts at one to not collide with MSGDEF_INDEX.
  size_t userval_idx = 1;

  // Assign offsets.
  upb_msg_field_iter i;
  for (upb_msg_field_begin(&i, lmd->md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    if (in_userval(f)) {
      offsets[upb_fielddef_index(f)] = userval_idx++;
    } else {
      size_t size = lupb_sizeof(L, f);
      data_ofs = align_up(data_ofs, size);
      offsets[upb_fielddef_index(f)] = data_ofs;
      data_ofs += size;
    }
  }

  lmd->field_offsets = offsets;
  lmd->msg_size = sizeof(lupb_msg) + data_ofs;
  lmd->hasbits_size = hasbits_size;

  // Now recursively assign offsets for all submessages, and also add them to
  // the uservalue to ensure that all the lupb_msgdef objects for our
  // submessages outlive us.  This is particularly important if/when we build
  // handlers to populate this msgdef.

  lua_pushvalue(L, narg);
  lua_newtable(L);  // This will be our userval.

  int idx = 1;
  for (upb_msg_field_begin(&i, lmd->md);
       !upb_msg_field_done(&i);
       upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE) {
      bool created = lupb_def_pushwrapper(L, upb_fielddef_subdef(f), NULL);
      UPB_ASSERT_VAR(created, !created);
      lupb_msg_assignoffsets(L, -1);
      lua_rawseti(L, -2, idx++);  // Append to uservalue.
    }
  }

  lua_setuservalue(L, -2);
  lua_pop(L, 1);  // copy of msgdef

  return lmd;
}

void lupb_msg_pushnew(lua_State *L, int narg) {
  lupb_msgdef *lmd = lupb_msg_assignoffsets(L, narg);

  // Add passed-in MessageDef to a table which will become the msg's userval.
  lua_pushvalue(L, narg);
  lua_newtable(L);
  lua_pushvalue(L, narg);
  lua_rawseti(L, -2, MSGDEF_INDEX);

  lupb_msg *msg = newudata_with_userval(L, lmd->msg_size, LUPB_MSG);
  memset(msg, 0, lmd->msg_size);

  // Create a msg->msgdef reference, both:
  // 1. a pointer in the userdata itself (for easy access) and
  msg->lmd = lmd;

  // 2. a reference in Lua-space from the msg's uservalue to the messagedef
  //    wrapper object (so the msgdef wrapper object will always outlive us,
  //    GC-wise).
  lua_pushvalue(L, -2);     // Push the table from before.
  lua_setuservalue(L, -2);  // Pop table, now msg is at top again.
  lua_remove(L, -2);        // Remove table, so new message is only new value.
}

static int lupb_msg_new(lua_State *L) {
  lupb_msg_pushnew(L, 1);
  return 1;
}

static bool lupb_msg_has(const lupb_msg *msg, const upb_fielddef *f) {
  uint16_t idx = upb_fielddef_index(f);
  return msg->data[idx / 8] & (1 << (idx % 8));
}

static void lupb_msg_set(lupb_msg *msg, const upb_fielddef *f) {
  uint16_t idx = upb_fielddef_index(f);
  msg->data[idx / 8] |= (1 << (idx % 8));
}

static int lupb_msg_index(lua_State *L) {
  lupb_msg *msg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, msg->lmd, 2);

  if (!upb_fielddef_isseq(f) && !lupb_msg_has(msg, f)) {
    lua_pushnil(L);
    return 1;
  }

  int ofs = msg->lmd->field_offsets[upb_fielddef_index(f)];

  if (in_userval(f)) {
    lua_getuservalue(L, 1);
    lua_pushinteger(L, ofs);
    lua_rawget(L, -2);
  } else {
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_FLOAT:
        lupb_pushfloat(L, DEREF(msg, ofs, float));
        break;
      case UPB_TYPE_DOUBLE:
        lupb_pushdouble(L, DEREF(msg, ofs, double));
        break;
      case UPB_TYPE_BOOL:
        lua_pushboolean(L, DEREF(msg, ofs, bool));
        break;
      case UPB_TYPE_ENUM:
      case UPB_TYPE_INT32:
        lupb_pushint32(L, DEREF(msg, ofs, int32_t));
        break;
      case UPB_TYPE_UINT32:
        lupb_pushuint32(L, DEREF(msg, ofs, uint32_t));
        break;
      case UPB_TYPE_INT64:
        if (LUA_VERSION_NUM < 503) {
          // Check value? Lua < 5.3.0 has no native integer support, lua_Number
          // is probably double which can't exactly represent large int64s.
        }
        lupb_pushint64(L, DEREF(msg, ofs, int64_t));
        break;
      case UPB_TYPE_UINT64:
        if (LUA_VERSION_NUM < 503) {
          // Check value? Lua < 5.3.0 has no native integer support, lua_Number
          // is probably double which can't exactly represent large uint64s.
        }
        lupb_pushuint64(L, DEREF(msg, ofs, uint64_t));
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
      case UPB_TYPE_MESSAGE:
        lupb_assert(L, false);
        break;
    }
  }

  return 1;
}

int lupb_msg_newindex(lua_State *L) {
  lupb_msg *msg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, msg->lmd, 2);

  lupb_msg_set(msg, f);

  int ofs = msg->lmd->field_offsets[upb_fielddef_index(f)];

  if (in_userval(f)) {
    // Type-check and then store in the userval.
    if (upb_fielddef_isseq(f)) {
      lupb_array *array = lupb_array_check(L, 3);
      if (array->type != upb_fielddef_type(f) ||
          (array->type == UPB_TYPE_MESSAGE &&
           array->msgdef != upb_fielddef_msgsubdef(f))) {
        return luaL_error(L, "Array type mismatch");
      }
    } else if (upb_fielddef_isstring(f)) {
      lupb_checkstring(L, 3);
    } else {
      if (lupb_msg_checkdef(L, 3) != upb_fielddef_msgsubdef(f)) {
        return luaL_error(L, "Message type mismatch");
      }
    }
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 3);
    lua_rawseti(L, -2, ofs);
  } else {
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_FLOAT:
        DEREF(msg, ofs, float) = lupb_checkfloat(L, 3);
        break;
      case UPB_TYPE_DOUBLE:
        DEREF(msg, ofs, double) = lupb_checkdouble(L, 3);
        break;
      case UPB_TYPE_ENUM:
      case UPB_TYPE_INT32:
        DEREF(msg, ofs, int32_t) = lupb_checkint32(L, 3);
        break;
      case UPB_TYPE_UINT32:
        DEREF(msg, ofs, uint32_t) = lupb_checkuint32(L, 3);
        break;
      case UPB_TYPE_INT64:
        DEREF(msg, ofs, int64_t) = lupb_checkint64(L, 3);
        break;
      case UPB_TYPE_UINT64:
        DEREF(msg, ofs, uint64_t) = lupb_checkuint64(L, 3);
        break;
      case UPB_TYPE_BOOL:
        DEREF(msg, ofs, bool) = lupb_checkbool(L, 3);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
      case UPB_TYPE_MESSAGE:
        lupb_assert(L, false);
    }
  }

  return 0;  // 1 for chained assignments?
}

static const struct luaL_Reg lupb_msg_mm[] = {
  {"__index", lupb_msg_index},
  {"__newindex", lupb_msg_newindex},
  {NULL, NULL}
};


/* lupb_msg populating handlers ***********************************************/

// NOTE: doesn't support repeated or submessage fields yet.  Coming soon.

typedef struct {
  uint32_t ofs;
  uint32_t hasbit;
} lupb_handlerdata;

static void lupb_sethasbit(lupb_msg *msg, uint32_t hasbit) {
  msg->data[hasbit / 8] |= 1 << (hasbit % 8);
}

static size_t strhandler(void *closure, const void *hd, const char *str,
                         size_t len, const upb_bufhandle *handle) {
  UPB_UNUSED(handle);
  lupb_msg *msg = closure;
  const lupb_handlerdata *data = hd;
  lua_State *L = msg->lmd->L;
  lua_pushlstring(L, str, len);
  lua_rawseti(L, -2, data->ofs);
  lupb_sethasbit(msg, data->hasbit);
  return len;
}

const void *newhandlerdata(upb_handlers *h, uint32_t ofs, uint32_t hasbit) {
  lupb_handlerdata *data = malloc(sizeof(*data));
  data->ofs = ofs;
  data->hasbit = hasbit;
  upb_handlers_addcleanup(h, data, free);
  return data;
}

void callback(const void *closure, upb_handlers *h) {
  lua_State *L = (lua_State*)closure;
  lupb_def_pushwrapper(L, UPB_UPCAST(upb_handlers_msgdef(h)), NULL);
  lupb_msgdef *lmd = lupb_msg_assignoffsets(L, -1);
  upb_msg_field_iter i;
  upb_msg_field_begin(&i, upb_handlers_msgdef(h));
  for (; !upb_msg_field_done(&i); upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    int hasbit = upb_fielddef_index(f);
    uint16_t ofs = lmd->field_offsets[upb_fielddef_index(f)];
    if (upb_fielddef_isseq(f)) {
      luaL_error(L, "Doesn't support repeated fields yet.");
    } else {
      switch (upb_fielddef_type(f)) {
        case UPB_TYPE_BOOL:
        case UPB_TYPE_INT32:
        case UPB_TYPE_UINT32:
        case UPB_TYPE_ENUM:
        case UPB_TYPE_FLOAT:
        case UPB_TYPE_INT64:
        case UPB_TYPE_UINT64:
        case UPB_TYPE_DOUBLE:
          hasbit += offsetof(lupb_msg, data) * 8;
          ofs += offsetof(lupb_msg, data);
          upb_shim_set(h, f, ofs, hasbit);
          break;
        case UPB_TYPE_STRING:
        case UPB_TYPE_BYTES: {
          upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
          upb_handlerattr_sethandlerdata(&attr, newhandlerdata(h, ofs, hasbit));
          // XXX: does't currently handle split buffers.
          upb_handlers_setstring(h, f, strhandler, &attr);
          upb_handlerattr_uninit(&attr);
          break;
        }
        case UPB_TYPE_MESSAGE:
          luaL_error(L, "Doesn't support submessages yet.");
          break;
      }
    }
  }
  lua_pop(L, 1);  // msgdef wrapper
}

const upb_handlers *lupb_msg_newwritehandlers(lua_State *L, int narg,
                                              const void *owner) {
  lupb_msgdef *lmd = lupb_msg_assignoffsets(L, narg);
  return upb_handlers_newfrozen(lmd->md, owner, callback, L);
}


/* lupb toplevel **************************************************************/

static int lupb_freeze(lua_State *L) {
  int n = lua_gettop(L);
  // Scratch memory; lua_newuserdata() anchors it as a GC root in case any Lua
  // functions fail.
  upb_def **defs = lua_newuserdata(L, n * sizeof(upb_def*));
  for (int i = 0; i < n; i++) {
    // Could allow an array of defs here also.
    defs[i] = lupb_def_checkmutable(L, i + 1);
  }
  CHK(upb_def_freeze(defs, n, &status));
  return 0;
}

static const struct luaL_Reg lupb_toplevel_m[] = {
  {"Array", lupb_array_new},
  {"EnumDef", lupb_enumdef_new},
  {"FieldDef", lupb_fielddef_new},
  {"Message", lupb_msg_new},
  {"MessageDef", lupb_msgdef_new},
  {"SymbolTable", lupb_symtab_new},
  {"freeze", lupb_freeze},

  {NULL, NULL}
};

void lupb_register_type(lua_State *L, const char *name, const luaL_Reg *m,
                        const luaL_Reg *mm, bool refcount_gc) {
  luaL_newmetatable(L, name);

  if (mm) {
    lupb_setfuncs(L, mm);
  }

  if (refcount_gc) {
    lupb_setfuncs(L, lupb_refcounted_mm);
  }

  if (m) {
    // Methods go in the mt's __index method.  This implies that you can't
    // implement __index and also have methods.
    lua_getfield(L, -1, "__index");
    lupb_assert(L, lua_isnil(L, -1));
    lua_pop(L, 1);

    lua_createtable(L, 0, 0);
    lupb_setfuncs(L, m);
    lua_setfield(L, -2, "__index");
  }

  lua_pop(L, 1);  // The mt.
}

static void lupb_setfieldi(lua_State *L, const char *field, int i) {
  lua_pushinteger(L, i);
  lua_setfield(L, -2, field);
}

int luaopen_upb(lua_State *L) {
  static char module_key;
  if (lupb_openlib(L, &module_key, "upb", lupb_toplevel_m)) {
    return 1;
  }

  // Non-refcounted types.
  lupb_register_type(L, LUPB_ARRAY,    NULL,            lupb_array_mm,   false);
  lupb_register_type(L, LUPB_MSG,      NULL,            lupb_msg_mm,     false);

  // Refcounted types.
  lupb_register_type(L, LUPB_ENUMDEF,  lupb_enumdef_m,  lupb_enumdef_mm,  true);
  lupb_register_type(L, LUPB_FIELDDEF, lupb_fielddef_m, NULL,             true);
  lupb_register_type(L, LUPB_SYMTAB,   lupb_symtab_m,   NULL,             true);

  // Refcounted but with custom __gc.
  lupb_register_type(L, LUPB_MSGDEF,   lupb_msgdef_m,   lupb_msgdef_mm,  false);

  // Create our object cache.
  lua_newtable(L);
  lua_createtable(L, 0, 1);  // Cache metatable.
  lua_pushstring(L, "v");    // Values are weak.
  lua_setfield(L, -2, "__mode");
  lua_setmetatable(L, -2);
  lua_setfield(L, LUA_REGISTRYINDEX, LUPB_OBJCACHE);

  // Register constants.
  lupb_setfieldi(L, "LABEL_OPTIONAL", UPB_LABEL_OPTIONAL);
  lupb_setfieldi(L, "LABEL_REQUIRED", UPB_LABEL_REQUIRED);
  lupb_setfieldi(L, "LABEL_REPEATED", UPB_LABEL_REPEATED);

  lupb_setfieldi(L, "TYPE_DOUBLE",    UPB_TYPE_DOUBLE);
  lupb_setfieldi(L, "TYPE_FLOAT",     UPB_TYPE_FLOAT);
  lupb_setfieldi(L, "TYPE_INT64",     UPB_TYPE_INT64);
  lupb_setfieldi(L, "TYPE_UINT64",    UPB_TYPE_UINT64);
  lupb_setfieldi(L, "TYPE_INT32",     UPB_TYPE_INT32);
  lupb_setfieldi(L, "TYPE_BOOL",      UPB_TYPE_BOOL);
  lupb_setfieldi(L, "TYPE_STRING",    UPB_TYPE_STRING);
  lupb_setfieldi(L, "TYPE_MESSAGE",   UPB_TYPE_MESSAGE);
  lupb_setfieldi(L, "TYPE_BYTES",     UPB_TYPE_BYTES);
  lupb_setfieldi(L, "TYPE_UINT32",    UPB_TYPE_UINT32);
  lupb_setfieldi(L, "TYPE_ENUM",      UPB_TYPE_ENUM);

  lupb_setfieldi(L, "INTFMT_VARIABLE",  UPB_INTFMT_VARIABLE);
  lupb_setfieldi(L, "INTFMT_FIXED",     UPB_INTFMT_FIXED);
  lupb_setfieldi(L, "INTFMT_ZIGZAG",    UPB_INTFMT_ZIGZAG);

  lupb_setfieldi(L, "DESCRIPTOR_TYPE_DOUBLE",    UPB_DESCRIPTOR_TYPE_DOUBLE);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_FLOAT",     UPB_DESCRIPTOR_TYPE_FLOAT);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_INT64",     UPB_DESCRIPTOR_TYPE_INT64);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_UINT64",    UPB_DESCRIPTOR_TYPE_UINT64);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_INT32",     UPB_DESCRIPTOR_TYPE_INT32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_FIXED64",   UPB_DESCRIPTOR_TYPE_FIXED64);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_FIXED32",   UPB_DESCRIPTOR_TYPE_FIXED32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_BOOL",      UPB_DESCRIPTOR_TYPE_BOOL);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_STRING",    UPB_DESCRIPTOR_TYPE_STRING);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_GROUP",     UPB_DESCRIPTOR_TYPE_GROUP);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_MESSAGE",   UPB_DESCRIPTOR_TYPE_MESSAGE);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_BYTES",     UPB_DESCRIPTOR_TYPE_BYTES);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_UINT32",    UPB_DESCRIPTOR_TYPE_UINT32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_ENUM",      UPB_DESCRIPTOR_TYPE_ENUM);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_SFIXED32",  UPB_DESCRIPTOR_TYPE_SFIXED32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_SFIXED64",  UPB_DESCRIPTOR_TYPE_SFIXED64);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_SINT32",    UPB_DESCRIPTOR_TYPE_SINT32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_SINT64",    UPB_DESCRIPTOR_TYPE_SINT64);

  lupb_setfieldi(L, "DEF_MSG",      UPB_DEF_MSG);
  lupb_setfieldi(L, "DEF_FIELD",    UPB_DEF_FIELD);
  lupb_setfieldi(L, "DEF_ENUM",     UPB_DEF_ENUM);
  lupb_setfieldi(L, "DEF_SERVICE",  UPB_DEF_SERVICE);
  lupb_setfieldi(L, "DEF_ANY",      UPB_DEF_ANY);

  lupb_setfieldi(L, "HANDLER_INT32",       UPB_HANDLER_INT32);
  lupb_setfieldi(L, "HANDLER_INT64",       UPB_HANDLER_INT64);
  lupb_setfieldi(L, "HANDLER_UINT32",      UPB_HANDLER_UINT32);
  lupb_setfieldi(L, "HANDLER_UINT64",      UPB_HANDLER_UINT64);
  lupb_setfieldi(L, "HANDLER_FLOAT",       UPB_HANDLER_FLOAT);
  lupb_setfieldi(L, "HANDLER_DOUBLE",      UPB_HANDLER_DOUBLE);
  lupb_setfieldi(L, "HANDLER_BOOL",        UPB_HANDLER_BOOL);
  lupb_setfieldi(L, "HANDLER_STARTSTR",    UPB_HANDLER_STARTSTR);
  lupb_setfieldi(L, "HANDLER_STRING",      UPB_HANDLER_STRING);
  lupb_setfieldi(L, "HANDLER_ENDSTR",      UPB_HANDLER_ENDSTR);
  lupb_setfieldi(L, "HANDLER_STARTSUBMSG", UPB_HANDLER_STARTSUBMSG);
  lupb_setfieldi(L, "HANDLER_ENDSUBMSG",   UPB_HANDLER_ENDSUBMSG);
  lupb_setfieldi(L, "HANDLER_STARTSEQ",    UPB_HANDLER_STARTSEQ);
  lupb_setfieldi(L, "HANDLER_ENDSEQ",      UPB_HANDLER_ENDSEQ);

  // Call the chunk that will define the extra functions on upb, passing our
  // package dictionary as the argument.
  if (luaL_loadbuffer(L, upb_lua, sizeof(upb_lua), "upb.lua") ||
      lua_pcall(L, 0, LUA_MULTRET, 0)) {
    lua_error(L);
  }
  lua_pushvalue(L, -2);
  lua_call(L, 1, 0);

  return 1;  // Return package table.
}
