
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

static void lupb_symtab_pushwrapper(lua_State *L, int narg, const void *def,
                                    const char *type);

/* lupb_wrapper ***************************************************************/

/* Wrappers around upb def objects.  The userval contains a reference to the
 * symtab. */

#define LUPB_SYMTAB_INDEX 1

typedef struct {
  const void* def;   /* upb_msgdef, upb_enumdef, upb_oneofdef, etc. */
} lupb_wrapper;

static const void *lupb_wrapper_check(lua_State *L, int narg,
                                      const char *type) {
  lupb_wrapper *w = luaL_checkudata(L, narg, type);
  return w->def;
}

static void lupb_wrapper_pushsymtab(lua_State *L, int narg) {
  lua_getiuservalue(L, narg, LUPB_SYMTAB_INDEX);
}

/* lupb_wrapper_pushwrapper()
 *
 * For a given def wrapper at index |narg|, pushes a wrapper for the given |def|
 * and the given |type|.  The new wrapper will be part of the same symtab. */
static void lupb_wrapper_pushwrapper(lua_State *L, int narg, const void *def,
                                     const char *type) {
  lupb_wrapper_pushsymtab(L, narg);
  lupb_symtab_pushwrapper(L, -1, def, type);
  lua_replace(L, -2);  /* Remove symtab from stack. */
}

/* lupb_msgdef_pushsubmsgdef()
 *
 * Pops the msgdef wrapper at the top of the stack and replaces it with a msgdef
 * wrapper for field |f| of this msgdef (submsg may not be direct, for example it
 * may be the submessage of the map value).
 */
void lupb_msgdef_pushsubmsgdef(lua_State *L, const upb_fielddef *f) {
  const upb_msgdef *m = upb_fielddef_msgsubdef(f);
  assert(m);
  lupb_wrapper_pushwrapper(L, -1, m, LUPB_MSGDEF);
  lua_replace(L, -2);  /* Replace msgdef with submsgdef. */
}

/* lupb_fielddef **************************************************************/

const upb_fielddef *lupb_fielddef_check(lua_State *L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_FIELDDEF);
}

static int lupb_fielddef_containingoneof(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  const upb_oneofdef *o = upb_fielddef_containingoneof(f);
  lupb_wrapper_pushwrapper(L, 1, o, LUPB_ONEOFDEF);
  return 1;
}

static int lupb_fielddef_containingtype(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  const upb_msgdef *m = upb_fielddef_containingtype(f);
  lupb_wrapper_pushwrapper(L, 1, m, LUPB_MSGDEF);
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
  if (num) {
    lua_pushinteger(L, num);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int lupb_fielddef_packed(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushboolean(L, upb_fielddef_packed(f));
  return 1;
}

static int lupb_fielddef_msgsubdef(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  const upb_msgdef *m = upb_fielddef_msgsubdef(f);
  lupb_wrapper_pushwrapper(L, 1, m, LUPB_MSGDEF);
  return 1;
}

static int lupb_fielddef_enumsubdef(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  const upb_enumdef *e = upb_fielddef_enumsubdef(f);
  lupb_wrapper_pushwrapper(L, 1, e, LUPB_ENUMDEF);
  return 1;
}

static int lupb_fielddef_type(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lua_pushinteger(L, upb_fielddef_type(f));
  return 1;
}

static const struct luaL_Reg lupb_fielddef_m[] = {
  {"containing_oneof", lupb_fielddef_containingoneof},
  {"containing_type", lupb_fielddef_containingtype},
  {"default", lupb_fielddef_default},
  {"descriptor_type", lupb_fielddef_descriptortype},
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

const upb_oneofdef *lupb_oneofdef_check(lua_State *L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_ONEOFDEF);
}

static int lupb_oneofdef_containingtype(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  const upb_msgdef *m = upb_oneofdef_containingtype(o);
  lupb_wrapper_pushwrapper(L, 1, m, LUPB_MSGDEF);
  return 1;
}

static int lupb_oneofdef_field(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  int32_t idx = lupb_checkint32(L, 2);
  int count = upb_oneofdef_fieldcount(o);

  if (idx < 0 || idx >= count) {
    const char *msg = lua_pushfstring(L, "index %d exceeds field count %d",
                                      idx, count);
    return luaL_argerror(L, 2, msg);
  }

  lupb_wrapper_pushwrapper(L, 1, upb_oneofdef_field(o, idx), LUPB_FIELDDEF);
  return 1;
}

static int lupb_oneofiter_next(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, lua_upvalueindex(1));
  int *index = lua_touserdata(L, lua_upvalueindex(2));
  const upb_fielddef *f;
  if (*index == upb_oneofdef_fieldcount(o)) return 0;
  f = upb_oneofdef_field(o, (*index)++);
  lupb_wrapper_pushwrapper(L, lua_upvalueindex(1), f, LUPB_FIELDDEF);
  return 1;
}

static int lupb_oneofdef_fields(lua_State *L) {
  int *index = lua_newuserdata(L, sizeof(int));
  lupb_oneofdef_check(L, 1);
  *index = 0;

  /* Closure upvalues are: oneofdef, index. */
  lua_pushcclosure(L, &lupb_oneofiter_next, 2);
  return 1;
}

static int lupb_oneofdef_len(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  lua_pushinteger(L, upb_oneofdef_fieldcount(o));
  return 1;
}

/* lupb_oneofdef_lookupfield()
 *
 * Handles:
 *   oneof.lookup_field(field_number)
 *   oneof.lookup_field(field_name)
 */
static int lupb_oneofdef_lookupfield(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  const upb_fielddef *f;

  switch (lua_type(L, 2)) {
    case LUA_TNUMBER:
      f = upb_oneofdef_itof(o, lua_tointeger(L, 2));
      break;
    case LUA_TSTRING:
      f = upb_oneofdef_ntofz(o, lua_tostring(L, 2));
      break;
    default: {
      const char *msg = lua_pushfstring(L, "number or string expected, got %s",
                                        luaL_typename(L, 2));
      return luaL_argerror(L, 2, msg);
    }
  }

  lupb_wrapper_pushwrapper(L, 1, f, LUPB_FIELDDEF);
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
  {"lookup_field", lupb_oneofdef_lookupfield},
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

const upb_msgdef *lupb_msgdef_check(lua_State *L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_MSGDEF);
}

static int lupb_msgdef_fieldcount(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushinteger(L, upb_msgdef_fieldcount(m));
  return 1;
}

static int lupb_msgdef_oneofcount(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushinteger(L, upb_msgdef_oneofcount(m));
  return 1;
}

static bool lupb_msgdef_pushnested(lua_State *L, int msgdef, int name) {
  const upb_msgdef *m = lupb_msgdef_check(L, msgdef);
  lupb_wrapper_pushsymtab(L, msgdef);
  upb_symtab *symtab = lupb_symtab_check(L, -1);
  lua_pop(L, 1);

  /* Construct full package.Message.SubMessage name. */
  lua_pushstring(L, upb_msgdef_fullname(m));
  lua_pushstring(L, ".");
  lua_pushvalue(L, name);
  lua_concat(L, 3);
  const char *nested_name = lua_tostring(L, -1);

  /* Try lookup. */
  const upb_msgdef *nested = upb_symtab_lookupmsg(symtab, nested_name);
  if (!nested) return false;
  lupb_wrapper_pushwrapper(L, msgdef, nested, LUPB_MSGDEF);
  return true;
}

/* lupb_msgdef_field()
 *
 * Handles:
 *   msg.field(field_number) -> fielddef
 *   msg.field(field_name) -> fielddef
 */
static int lupb_msgdef_field(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  const upb_fielddef *f;

  switch (lua_type(L, 2)) {
    case LUA_TNUMBER:
      f = upb_msgdef_itof(m, lua_tointeger(L, 2));
      break;
    case LUA_TSTRING:
      f = upb_msgdef_ntofz(m, lua_tostring(L, 2));
      break;
    default: {
      const char *msg = lua_pushfstring(L, "number or string expected, got %s",
                                        luaL_typename(L, 2));
      return luaL_argerror(L, 2, msg);
    }
  }

  lupb_wrapper_pushwrapper(L, 1, f, LUPB_FIELDDEF);
  return 1;
}

/* lupb_msgdef_lookupname()
 *
 * Handles:
 *   msg.lookup_name(name) -> fielddef or oneofdef
 */
static int lupb_msgdef_lookupname(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  const upb_fielddef *f;
  const upb_oneofdef *o;

  if (!upb_msgdef_lookupnamez(m, lua_tostring(L, 2), &f, &o)) {
    lua_pushnil(L);
  } else if (o) {
    lupb_wrapper_pushwrapper(L, 1, o, LUPB_ONEOFDEF);
  } else {
    lupb_wrapper_pushwrapper(L, 1, f, LUPB_FIELDDEF);
  }

  return 1;
}

/* lupb_msgdef_name()
 *
 * Handles:
 *   msg.name() -> string
 */
static int lupb_msgdef_name(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushstring(L, upb_msgdef_name(m));
  return 1;
}

static int lupb_msgfielditer_next(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, lua_upvalueindex(1));
  int *index = lua_touserdata(L, lua_upvalueindex(2));
  const upb_fielddef *f;
  if (*index == upb_msgdef_fieldcount(m)) return 0;
  f = upb_msgdef_field(m, (*index)++);
  lupb_wrapper_pushwrapper(L, lua_upvalueindex(1), f, LUPB_FIELDDEF);
  return 1;
}

static int lupb_msgdef_fields(lua_State *L) {
  int *index = lua_newuserdata(L, sizeof(int));
  lupb_msgdef_check(L, 1);
  *index = 0;

  /* Closure upvalues are: msgdef, index. */
  lua_pushcclosure(L, &lupb_msgfielditer_next, 2);
  return 1;
}

static int lupb_msgdef_file(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  const upb_filedef *file = upb_msgdef_file(m);
  lupb_wrapper_pushwrapper(L, 1, file, LUPB_FILEDEF);
  return 1;
}

static int lupb_msgdef_fullname(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushstring(L, upb_msgdef_fullname(m));
  return 1;
}

static int lupb_msgdef_index(lua_State *L) {
  if (!lupb_msgdef_pushnested(L, 1, 2)) {
    luaL_error(L, "No such nested message");
  }
  return 1;
}

static int lupb_msgoneofiter_next(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, lua_upvalueindex(1));
  int *index = lua_touserdata(L, lua_upvalueindex(2));
  const upb_oneofdef *o;
  if (*index == upb_msgdef_oneofcount(m)) return 0;
  o = upb_msgdef_oneof(m, (*index)++);
  lupb_wrapper_pushwrapper(L, lua_upvalueindex(1), o, LUPB_ONEOFDEF);
  return 1;
}

static int lupb_msgdef_oneofs(lua_State *L) {
  int *index = lua_newuserdata(L, sizeof(int));
  lupb_msgdef_check(L, 1);
  *index = 0;

  /* Closure upvalues are: msgdef, index. */
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

static int lupb_msgdef_tostring(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  lua_pushfstring(L, "<upb.MessageDef name=%s, field_count=%d>",
                  upb_msgdef_fullname(m), (int)upb_msgdef_numfields(m));
  return 1;
}

static const struct luaL_Reg lupb_msgdef_mm[] = {
  {"__call", lupb_msgdef_call},
  {"__index", lupb_msgdef_index},
  {"__len", lupb_msgdef_fieldcount},
  {"__tostring", lupb_msgdef_tostring},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_msgdef_m[] = {
  {"field", lupb_msgdef_field},
  {"fields", lupb_msgdef_fields},
  {"field_count", lupb_msgdef_fieldcount},
  {"file", lupb_msgdef_file},
  {"full_name", lupb_msgdef_fullname},
  {"lookup_name", lupb_msgdef_lookupname},
  {"name", lupb_msgdef_name},
  {"oneof_count", lupb_msgdef_oneofcount},
  {"oneofs", lupb_msgdef_oneofs},
  {"syntax", lupb_msgdef_syntax},
  {"_map_entry", lupb_msgdef_mapentry},
  {NULL, NULL}
};


/* lupb_enumdef ***************************************************************/

const upb_enumdef *lupb_enumdef_check(lua_State *L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_ENUMDEF);
}

static int lupb_enumdef_len(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);
  lua_pushinteger(L, upb_enumdef_numvals(e));
  return 1;
}

static int lupb_enumdef_file(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);
  const upb_filedef *file = upb_enumdef_file(e);
  lupb_wrapper_pushwrapper(L, 1, file, LUPB_FILEDEF);
  return 1;
}

/* lupb_enumdef_value()
 *
 * Handles:
 *   enum.value(number) -> name
 *   enum.value(name) -> number
 */
static int lupb_enumdef_value(lua_State *L) {
  const upb_enumdef *e = lupb_enumdef_check(L, 1);

  switch (lua_type(L, 2)) {
    case LUA_TNUMBER: {
      int32_t key = lupb_checkint32(L, 2);
      /* Pushes "nil" for a NULL pointer. */
      lua_pushstring(L, upb_enumdef_iton(e, key));
      break;
    }
    case LUA_TSTRING: {
      const char *key = lua_tostring(L, 2);
      int32_t num;
      if (upb_enumdef_ntoiz(e, key, &num)) {
        lua_pushinteger(L, num);
      } else {
        lua_pushnil(L);
      }
      break;
    }
    default: {
      const char *msg = lua_pushfstring(L, "number or string expected, got %s",
                                        luaL_typename(L, 2));
      return luaL_argerror(L, 2, msg);
    }
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
  lupb_wrapper_pushsymtab(L, 1);
  upb_enum_begin(i, e);

  /* Closure upvalues are: iter, symtab. */
  lua_pushcclosure(L, &lupb_enumiter_next, 2);
  return 1;
}

static const struct luaL_Reg lupb_enumdef_mm[] = {
  {"__len", lupb_enumdef_len},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_enumdef_m[] = {
  {"file", lupb_enumdef_file},
  {"value", lupb_enumdef_value},
  {"values", lupb_enumdef_values},
  {NULL, NULL}
};


/* lupb_filedef ***************************************************************/

const upb_filedef *lupb_filedef_check(lua_State *L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_FILEDEF);
}

static int lupb_filedef_dep(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  int index = luaL_checkint(L, 2);
  const upb_filedef *dep = upb_filedef_dep(f, index);
  lupb_wrapper_pushwrapper(L, 1, dep, LUPB_FILEDEF);
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
  const upb_enumdef *e = upb_filedef_enum(f, index);
  lupb_wrapper_pushwrapper(L, 1, e, LUPB_ENUMDEF);
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
  const upb_msgdef *m = upb_filedef_msg(f, index);
  lupb_wrapper_pushwrapper(L, 1, m, LUPB_MSGDEF);
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

static int lupb_filedef_symtab(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  const upb_symtab *symtab = upb_filedef_symtab(f);
  lupb_wrapper_pushwrapper(L, 1, symtab, LUPB_SYMTAB);
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
  {"symtab", lupb_filedef_symtab},
  {"syntax", lupb_filedef_syntax},
  {NULL, NULL}
};


/* lupb_symtab ****************************************************************/

/* The symtab owns all defs.  Thus GC-rooting the symtab ensures that all
 * underlying defs stay alive.
 *
 * The symtab's userval is a cache of def* -> object. */

#define LUPB_CACHE_INDEX 1

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

void lupb_symtab_pushwrapper(lua_State *L, int narg, const void *def,
                             const char *type) {
  narg = lua_absindex(L, narg);
  assert(luaL_testudata(L, narg, LUPB_SYMTAB));

  if (def == NULL) {
    lua_pushnil(L);
    return;
  }

  lua_getiuservalue(L, narg, LUPB_CACHE_INDEX);  /* Get cache. */

  /* Index by "def" pointer. */
  lua_rawgetp(L, -1, def);

  /* Stack is now: cache, cached value. */
  if (lua_isnil(L, -1)) {
    /* Create new wrapper. */
    lupb_wrapper *w = lupb_newuserdata(L, sizeof(*w), 1, type);
    w->def = def;
    lua_replace(L, -2);  /* Replace nil */

    /* Set symtab as userval. */
    lua_pushvalue(L, narg);
    lua_setiuservalue(L, -2, LUPB_SYMTAB_INDEX);

    /* Add wrapper to the the cache. */
    lua_pushvalue(L, -1);
    lua_rawsetp(L, -3, def);
  }

  lua_replace(L, -2);  /* Remove cache, leaving only the wrapper. */
}

/* upb_symtab_new()
 *
 * Handles:
 *   upb.SymbolTable() -> <new instance>
 */
static int lupb_symtab_new(lua_State *L) {
  lupb_symtab *lsymtab = lupb_newuserdata(L, sizeof(*lsymtab), 1, LUPB_SYMTAB);
  lsymtab->symtab = upb_symtab_new();

  /* Create our object cache. */
  lua_newtable(L);

  /* Cache metatable: specifies that values are weak. */
  lua_createtable(L, 0, 1);
  lua_pushstring(L, "v");
  lua_setfield(L, -2, "__mode");
  lua_setmetatable(L, -2);

  /* Put the symtab itself in the cache metatable. */
  lua_pushvalue(L, -2);
  lua_rawsetp(L, -2, lsymtab->symtab);

  /* Set the cache as our userval. */
  lua_setiuservalue(L, -2, LUPB_CACHE_INDEX);

  return 1;
}

static int lupb_symtab_gc(lua_State *L) {
  lupb_symtab *lsymtab = luaL_checkudata(L, 1, LUPB_SYMTAB);
  upb_symtab_free(lsymtab->symtab);
  lsymtab->symtab = NULL;
  return 0;
}

static int lupb_symtab_addfile(lua_State *L) {
  size_t len;
  upb_symtab *s = lupb_symtab_check(L, 1);
  const char *str = luaL_checklstring(L, 2, &len);
  upb_arena *arena = lupb_arena_pushnew(L);
  const google_protobuf_FileDescriptorProto *file;
  const upb_filedef *file_def;
  upb_status status;

  upb_status_clear(&status);
  file = google_protobuf_FileDescriptorProto_parse(str, len, arena);

  if (!file) {
    luaL_argerror(L, 2, "failed to parse descriptor");
  }

  file_def = upb_symtab_addfile(s, file, &status);
  lupb_checkstatus(L, &status);

  lupb_symtab_pushwrapper(L, 1, file_def, LUPB_FILEDEF);

  return 1;
}

static int lupb_symtab_addset(lua_State *L) {
  size_t i, n, len;
  const google_protobuf_FileDescriptorProto *const *files;
  google_protobuf_FileDescriptorSet *set;
  upb_symtab *s = lupb_symtab_check(L, 1);
  const char *str = luaL_checklstring(L, 2, &len);
  upb_arena *arena = lupb_arena_pushnew(L);
  upb_status status;

  upb_status_clear(&status);
  set = google_protobuf_FileDescriptorSet_parse(str, len, arena);

  if (!set) {
    luaL_argerror(L, 2, "failed to parse descriptor");
  }

  files = google_protobuf_FileDescriptorSet_file(set, &n);
  for (i = 0; i < n; i++) {
    upb_symtab_addfile(s, files[i], &status);
    lupb_checkstatus(L, &status);
  }

  return 0;
}

static int lupb_symtab_lookupmsg(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  const upb_msgdef *m = upb_symtab_lookupmsg(s, luaL_checkstring(L, 2));
  lupb_symtab_pushwrapper(L, 1, m, LUPB_MSGDEF);
  return 1;
}

static int lupb_symtab_lookupenum(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  const upb_enumdef *e = upb_symtab_lookupenum(s, luaL_checkstring(L, 2));
  lupb_symtab_pushwrapper(L, 1, e, LUPB_ENUMDEF);
  return 1;
}

static int lupb_symtab_tostring(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  lua_pushfstring(L, "<upb.SymbolTable file_count=%d>",
                  (int)upb_symtab_filecount(s));
  return 1;
}

static const struct luaL_Reg lupb_symtab_m[] = {
  {"add_file", lupb_symtab_addfile},
  {"add_set", lupb_symtab_addset},
  {"lookup_msg", lupb_symtab_lookupmsg},
  {"lookup_enum", lupb_symtab_lookupenum},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_symtab_mm[] = {
  {"__gc", lupb_symtab_gc},
  {"__tostring", lupb_symtab_tostring},
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

  /* Register types. */
  lupb_register_type(L, LUPB_ENUMDEF,  lupb_enumdef_m,  lupb_enumdef_mm);
  lupb_register_type(L, LUPB_FIELDDEF, lupb_fielddef_m, NULL);
  lupb_register_type(L, LUPB_FILEDEF,  lupb_filedef_m,  NULL);
  lupb_register_type(L, LUPB_MSGDEF,   lupb_msgdef_m,   lupb_msgdef_mm);
  lupb_register_type(L, LUPB_ONEOFDEF, lupb_oneofdef_m, lupb_oneofdef_mm);
  lupb_register_type(L, LUPB_SYMTAB,   lupb_symtab_m,   lupb_symtab_mm);

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

  lupb_setfieldi(L, "SYNTAX_PROTO2",  UPB_SYNTAX_PROTO2);
  lupb_setfieldi(L, "SYNTAX_PROTO3",  UPB_SYNTAX_PROTO3);
}
