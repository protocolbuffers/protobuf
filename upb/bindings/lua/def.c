
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "lauxlib.h"
#include "upb/bindings/lua/upb.h"
#include "upb/def.h"

#define LUPB_ENUMDEF "lupb.enumdef"
#define LUPB_FIELDDEF "lupb.fielddef"
#define LUPB_FILEDEF "lupb.filedef"
#define LUPB_MSGDEF "lupb.msgdef"
#define LUPB_ONEOFDEF "lupb.oneof"
#define LUPB_SYMTAB "lupb.symtab"
#define LUPB_OBJCACHE "lupb.objcache"

#define CHK(pred)                 \
  do {                            \
    upb_status status;            \
    upb_status_clear(&status);    \
    pred;                         \
    lupb_checkstatus(L, &status); \
  } while (0)

/* lupb_wrapper ***************************************************************/

/* Wrappers around upb objects. */

/* Checks type; if it matches, pulls the pointer out of the wrapper. */
void *lupb_checkwrapper(lua_State *L, int narg, const char *type) {
  void *ud = lua_touserdata(L, narg);
  void *ret;

  if (!ud) {
    luaL_typerror(L, narg, "upb wrapper");
  }

  memcpy(&ret, ud, sizeof(ret));
  if (!ret) {
    luaL_error(L, "called into dead object");
  }

  luaL_checkudata(L, narg, type);
  return ret;
}

void lupb_pushwrapper(lua_State *L, const void *obj, const char *type) {
  void *ud;

  if (obj == NULL) {
    lua_pushnil(L);
    return;
  }

  /* Lookup our cache in the registry (we don't put our objects in the registry
   * directly because we need our cache to be a weak table). */
  lua_getfield(L, LUA_REGISTRYINDEX, LUPB_OBJCACHE);
  UPB_ASSERT(!lua_isnil(L, -1));  /* Should have been created by luaopen_upb. */
  lua_pushlightuserdata(L, (void*)obj);
  lua_rawget(L, -2);
  /* Stack is now: objcache, cached value. */

  if (lua_isnil(L, -1)) {
    /* Remove bad cached value and push new value. */
    lua_pop(L, 1);
    ud = lua_newuserdata(L, sizeof(*ud));
    memcpy(ud, &obj, sizeof(*ud));

    luaL_getmetatable(L, type);
    /* Should have been created by luaopen_upb. */
    lupb_assert(L, !lua_isnil(L, -1));
    lua_setmetatable(L, -2);

    /* Set it in the cache. */
    lua_pushlightuserdata(L, (void*)obj);
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
  }

  lua_insert(L, -2);
  lua_pop(L, 1);
}

void lupb_msgdef_pushwrapper(lua_State *L, const upb_msgdef *m);
void lupb_oneofdef_pushwrapper(lua_State *L, const upb_oneofdef *o);
static void lupb_enumdef_pushwrapper(lua_State *L, const upb_enumdef *e);


/* lupb_fielddef **************************************************************/

void lupb_fielddef_pushwrapper(lua_State *L, const upb_fielddef *f) {
  lupb_pushwrapper(L, f, LUPB_FIELDDEF);
}

const upb_fielddef *lupb_fielddef_check(lua_State *L, int narg) {
  return lupb_checkwrapper(L, narg, LUPB_FIELDDEF);
}

static int lupb_fielddef_containingoneof(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_oneofdef_pushwrapper(L, upb_fielddef_containingoneof(f));
  return 1;
}

static int lupb_fielddef_containingtype(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_msgdef_pushwrapper(L, upb_fielddef_containingtype(f));
  return 1;
}

static int lupb_fielddef_default(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
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
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
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

static int lupb_fielddef_descriptortype(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushnumber(L, upb_fielddef_descriptortype(f));
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

static int lupb_fielddef_isextension(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushboolean(L, upb_fielddef_isextension(f));
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

static int lupb_fielddef_msgsubdef(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_msgdef_pushwrapper(L, upb_fielddef_msgsubdef(f));
  return 1;
}

static int lupb_fielddef_enumsubdef(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_enumdef_pushwrapper(L, upb_fielddef_enumsubdef(f));
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

static const struct luaL_Reg lupb_fielddef_m[] = {
  {"containing_oneof", lupb_fielddef_containingoneof},
  {"containing_type", lupb_fielddef_containingtype},
  {"default", lupb_fielddef_default},
  {"descriptor_type", lupb_fielddef_descriptortype},
  {"getsel", lupb_fielddef_getsel},
  {"has_subdef", lupb_fielddef_hassubdef},
  {"index", lupb_fielddef_index},
  {"is_extension", lupb_fielddef_isextension},
  {"label", lupb_fielddef_label},
  {"lazy", lupb_fielddef_lazy},
  {"name", lupb_fielddef_name},
  {"number", lupb_fielddef_number},
  {"packed", lupb_fielddef_packed},
  {"msgsubdef", lupb_fielddef_msgsubdef},
  {"enumsubdef", lupb_fielddef_enumsubdef},
  {"type", lupb_fielddef_type},
  {NULL, NULL}
};


/* lupb_oneofdef **************************************************************/

void lupb_oneofdef_pushwrapper(lua_State *L, const upb_oneofdef *o) {
  lupb_pushwrapper(L, o, LUPB_ONEOFDEF);
}

const upb_oneofdef *lupb_oneofdef_check(lua_State *L, int narg) {
  return lupb_checkwrapper(L, narg, LUPB_ONEOFDEF);
}

static int lupb_oneofdef_containingtype(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  lupb_msgdef_pushwrapper(L, upb_oneofdef_containingtype(o));
  return 1;
}

static int lupb_oneofdef_field(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  int type = lua_type(L, 2);
  const upb_fielddef *f;
  if (type == LUA_TNUMBER) {
    f = upb_oneofdef_itof(o, lua_tointeger(L, 2));
  } else if (type == LUA_TSTRING) {
    f = upb_oneofdef_ntofz(o, lua_tostring(L, 2));
  } else {
    const char *msg = lua_pushfstring(L, "number or string expected, got %s",
                                      luaL_typename(L, 2));
    return luaL_argerror(L, 2, msg);
  }

  lupb_fielddef_pushwrapper(L, f);
  return 1;
}

static int lupb_oneofiter_next(lua_State *L) {
  upb_oneof_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_oneof_done(i)) return 0;
  lupb_fielddef_pushwrapper(L, upb_oneof_iter_field(i));
  upb_oneof_next(i);
  return 1;
}

static int lupb_oneofdef_fields(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  upb_oneof_iter *i = lua_newuserdata(L, sizeof(upb_oneof_iter));
  upb_oneof_begin(i, o);
  /* Need to guarantee that the msgdef outlives the iter. */
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, &lupb_oneofiter_next, 2);
  return 1;
}

static int lupb_oneofdef_len(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  lua_pushinteger(L, upb_oneofdef_numfields(o));
  return 1;
}

static int lupb_oneofdef_name(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  lua_pushstring(L, upb_oneofdef_name(o));
  return 1;
}

static const struct luaL_Reg lupb_oneofdef_m[] = {
  {"containing_type", lupb_oneofdef_containingtype},
  {"field", lupb_oneofdef_field},
  {"fields", lupb_oneofdef_fields},
  {"name", lupb_oneofdef_name},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_oneofdef_mm[] = {
  {"__len", lupb_oneofdef_len},
  {NULL, NULL}
};


/* lupb_msgdef ****************************************************************/

typedef struct {
  const upb_msgdef *md;
} lupb_msgdef;

void lupb_msgdef_pushwrapper(lua_State *L, const upb_msgdef *m) {
  lupb_pushwrapper(L, m, LUPB_MSGDEF);
}

const upb_msgdef *lupb_msgdef_check(lua_State *L, int narg) {
  return lupb_checkwrapper(L, narg, LUPB_MSGDEF);
}

static int lupb_msgdef_len(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushinteger(L, upb_msgdef_numfields(m));
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

  lupb_fielddef_pushwrapper(L, f);
  return 1;
}

static int lupb_msgdef_lookupname(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  const upb_fielddef *f;
  const upb_oneofdef *o;
  if (!upb_msgdef_lookupnamez(m, lua_tostring(L, 2), &f, &o)) {
    lua_pushnil(L);
  } else if (o) {
    lupb_oneofdef_pushwrapper(L, o);
  } else {
    lupb_fielddef_pushwrapper(L, f);
  }
  return 1;
}

static int lupb_msgfielditer_next(lua_State *L) {
  upb_msg_field_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_msg_field_done(i)) return 0;
  lupb_fielddef_pushwrapper(L, upb_msg_iter_field(i));
  upb_msg_field_next(i);
  return 1;
}

static int lupb_msgdef_fields(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  upb_msg_field_iter *i = lua_newuserdata(L, sizeof(upb_msg_field_iter));
  upb_msg_field_begin(i, m);
  /* Need to guarantee that the msgdef outlives the iter. */
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, &lupb_msgfielditer_next, 2);
  return 1;
}

static int lupb_msgoneofiter_next(lua_State *L) {
  upb_msg_oneof_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_msg_oneof_done(i)) return 0;
  lupb_oneofdef_pushwrapper(L, upb_msg_iter_oneof(i));
  upb_msg_oneof_next(i);
  return 1;
}

static int lupb_msgdef_oneofs(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  upb_msg_oneof_iter *i = lua_newuserdata(L, sizeof(upb_msg_oneof_iter));
  upb_msg_oneof_begin(i, m);
  /* Need to guarantee that the msgdef outlives the iter. */
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, &lupb_msgoneofiter_next, 2);
  return 1;
}

static int lupb_msgdef_mapentry(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushboolean(L, upb_msgdef_mapentry(m));
  return 1;
}

static int lupb_msgdef_syntax(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushinteger(L, upb_msgdef_syntax(m));
  return 1;
}

static const struct luaL_Reg lupb_msgdef_mm[] = {
  {"__len", lupb_msgdef_len},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_msgdef_m[] = {
  {"field", lupb_msgdef_field},
  {"fields", lupb_msgdef_fields},
  {"lookup_name", lupb_msgdef_lookupname},
  {"oneofs", lupb_msgdef_oneofs},
  {"syntax", lupb_msgdef_syntax},
  {"_map_entry", lupb_msgdef_mapentry},
  {NULL, NULL}
};


/* lupb_enumdef ***************************************************************/

const upb_enumdef *lupb_enumdef_check(lua_State *L, int narg) {
  return lupb_checkwrapper(L, narg, LUPB_ENUMDEF);
}

static void lupb_enumdef_pushwrapper(lua_State *L, const upb_enumdef *e) {
  lupb_pushwrapper(L, e, LUPB_ENUMDEF);
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
    /* Pushes "nil" for a NULL pointer. */
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
  /* Need to guarantee that the enumdef outlives the iter. */
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, &lupb_enumiter_next, 2);
  return 1;
}

static const struct luaL_Reg lupb_enumdef_mm[] = {
  {"__len", lupb_enumdef_len},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_enumdef_m[] = {
  {"value", lupb_enumdef_value},
  {"values", lupb_enumdef_values},
  {NULL, NULL}
};


/* lupb_filedef ***************************************************************/

void lupb_filedef_pushwrapper(lua_State *L, const upb_filedef *f) {
  lupb_pushwrapper(L, f, LUPB_FILEDEF);
}

const upb_filedef *lupb_filedef_check(lua_State *L, int narg) {
  return lupb_checkwrapper(L, narg, LUPB_FILEDEF);
}

static int lupb_filedef_dep(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  int index = luaL_checkint(L, 2);
  lupb_filedef_pushwrapper(L, upb_filedef_dep(f, index));
  return 1;
}

static int lupb_filedef_depcount(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  lua_pushnumber(L, upb_filedef_depcount(f));
  return 1;
}

static int lupb_filedef_enum(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  int index = luaL_checkint(L, 2);
  lupb_enumdef_pushwrapper(L, upb_filedef_enum(f, index));
  return 1;
}

static int lupb_filedef_enumcount(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  lua_pushnumber(L, upb_filedef_enumcount(f));
  return 1;
}

static int lupb_filedef_msg(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  int index = luaL_checkint(L, 2);
  lupb_msgdef_pushwrapper(L, upb_filedef_msg(f, index));
  return 1;
}

static int lupb_filedef_msgcount(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  lua_pushnumber(L, upb_filedef_msgcount(f));
  return 1;
}

static int lupb_filedef_name(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  lua_pushstring(L, upb_filedef_name(f));
  return 1;
}

static int lupb_filedef_package(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  lua_pushstring(L, upb_filedef_package(f));
  return 1;
}

static int lupb_filedef_syntax(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  lua_pushnumber(L, upb_filedef_syntax(f));
  return 1;
}

static const struct luaL_Reg lupb_filedef_m[] = {
  {"dep", lupb_filedef_dep},
  {"depcount", lupb_filedef_depcount},
  {"enum", lupb_filedef_enum},
  {"enumcount", lupb_filedef_enumcount},
  {"msg", lupb_filedef_msg},
  {"msgcount", lupb_filedef_msgcount},
  {"name", lupb_filedef_name},
  {"package", lupb_filedef_package},
  {"syntax", lupb_filedef_syntax},
  {NULL, NULL}
};


/* lupb_symtab ****************************************************************/

typedef struct {
  upb_symtab *symtab;
} lupb_symtab;

upb_symtab *lupb_symtab_check(lua_State *L, int narg) {
  lupb_symtab *lsymtab = luaL_checkudata(L, narg, LUPB_SYMTAB);
  if (!lsymtab->symtab) {
    luaL_error(L, "called into dead object");
  }
  return lsymtab->symtab;
}

static int lupb_symtab_new(lua_State *L) {
  lupb_symtab *lsymtab = lua_newuserdata(L, sizeof(*lsymtab));
  lsymtab->symtab = upb_symtab_new();
  luaL_getmetatable(L, LUPB_SYMTAB);
  lua_setmetatable(L, -2);
  return 1;
}

static int lupb_symtab_gc(lua_State *L) {
  lupb_symtab *lsymtab = luaL_checkudata(L, 1, LUPB_SYMTAB);
  upb_symtab_free(lsymtab->symtab);
  lsymtab->symtab = NULL;
  return 0;
}

/* TODO(haberman): perhaps this should take a message object instead of a
 * serialized string once we have a good story for vending compiled-in
 * messages. */
static int lupb_symtab_add(lua_State *L) {
  upb_arena *arena;
  size_t i, n, len;
  const google_protobuf_FileDescriptorProto *const *files;
  google_protobuf_FileDescriptorSet *set;
  upb_symtab *s = lupb_symtab_check(L, 1);
  const char *str = luaL_checklstring(L, 2, &len);

  lupb_arena_new(L);
  arena = lupb_arena_check(L, -1);

  set = google_protobuf_FileDescriptorSet_parse(str, len, arena);

  if (!set) {
    luaL_argerror(L, 2, "failed to parse descriptor");
  }

  files = google_protobuf_FileDescriptorSet_file(set, &n);
  for (i = 0; i < n; i++) {
    CHK(upb_symtab_addfile(s, files[i], &status));
  }

  return 0;
}

static int lupb_symtab_lookupmsg(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  const upb_msgdef *m = upb_symtab_lookupmsg(s, luaL_checkstring(L, 2));
  lupb_msgdef_pushwrapper(L, m);
  return 1;
}

static int lupb_symtab_lookupenum(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  const upb_enumdef *e = upb_symtab_lookupenum(s, luaL_checkstring(L, 2));
  lupb_enumdef_pushwrapper(L, e);
  return 1;
}

static const struct luaL_Reg lupb_symtab_m[] = {
  {"add", lupb_symtab_add},
  {"lookup_msg", lupb_symtab_lookupmsg},
  {"lookup_enum", lupb_symtab_lookupenum},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_symtab_mm[] = {
  {"__gc", lupb_symtab_gc},
  {NULL, NULL}
};

/* lupb toplevel **************************************************************/

static void lupb_setfieldi(lua_State *L, const char *field, int i) {
  lua_pushinteger(L, i);
  lua_setfield(L, -2, field);
}

static const struct luaL_Reg lupbdef_toplevel_m[] = {
  {"SymbolTable", lupb_symtab_new},
  {NULL, NULL}
};

void lupb_def_registertypes(lua_State *L) {
  lupb_setfuncs(L, lupbdef_toplevel_m);

  /* Refcounted types. */
  lupb_register_type(L, LUPB_ENUMDEF,  lupb_enumdef_m,  lupb_enumdef_mm);
  lupb_register_type(L, LUPB_FIELDDEF, lupb_fielddef_m, NULL);
  lupb_register_type(L, LUPB_FILEDEF,  lupb_filedef_m,  NULL);
  lupb_register_type(L, LUPB_MSGDEF,   lupb_msgdef_m,   lupb_msgdef_mm);
  lupb_register_type(L, LUPB_ONEOFDEF, lupb_oneofdef_m, lupb_oneofdef_mm);
  lupb_register_type(L, LUPB_SYMTAB,   lupb_symtab_m,   lupb_symtab_mm);

  /* Create our object cache. */
  lua_newtable(L);
  lua_createtable(L, 0, 1);  /* Cache metatable. */
  lua_pushstring(L, "v");    /* Values are weak. */
  lua_setfield(L, -2, "__mode");
  lua_setmetatable(L, -2);
  lua_setfield(L, LUA_REGISTRYINDEX, LUPB_OBJCACHE);

  /* Register constants. */
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

  lupb_setfieldi(L, "SYNTAX_PROTO2",  UPB_SYNTAX_PROTO2);
  lupb_setfieldi(L, "SYNTAX_PROTO3",  UPB_SYNTAX_PROTO3);
}
