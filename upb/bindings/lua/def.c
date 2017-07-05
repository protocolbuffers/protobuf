
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "lauxlib.h"
#include "upb/bindings/lua/upb.h"
#include "upb/def.h"
#include "upb/pb/glue.h"

#define LUPB_ENUMDEF "lupb.enumdef"
#define LUPB_FIELDDEF "lupb.fielddef"
#define LUPB_FILEDEF "lupb.filedef"
#define LUPB_MSGDEF "lupb.msgdef"
#define LUPB_ONEOFDEF "lupb.oneof"
#define LUPB_SYMTAB "lupb.symtab"
#define LUPB_OBJCACHE "lupb.objcache"

#define CHK(pred) do { \
    upb_status status = UPB_STATUS_INIT; \
    pred; \
    lupb_checkstatus(L, &status); \
  } while (0)


const char *lupb_checkname(lua_State *L, int narg) {
  size_t len;
  const char *name = luaL_checklstring(L, narg, &len);
  if (strlen(name) != len)
    luaL_error(L, "names cannot have embedded NULLs");
  return name;
}

upb_fieldtype_t lupb_checkfieldtype(lua_State *L, int narg) {
  int type = luaL_checkint(L, narg);
  if (!upb_fielddef_checktype(type))
    luaL_argerror(L, narg, "invalid field type");
  return type;
}


/* lupb_refcounted ************************************************************/

/* All upb objects that use upb_refcounted have a userdata that begins with a
 * pointer to that object.  Each type has its own metatable.  Objects are cached
 * in a weak table indexed by the C pointer of the object they are caching.
 *
 * Note that we consistently use memcpy() to read to/from the object.  This
 * allows the userdata to use its own struct without violating aliasing, as
 * long as it begins with a pointer. */

/* Checks type; if it matches, pulls the pointer out of the wrapper. */
void *lupb_refcounted_check(lua_State *L, int narg, const char *type) {
  void *ud = lua_touserdata(L, narg);
  void *ret;

  if (!ud) {
    luaL_typerror(L, narg, "refcounted");
  }

  memcpy(&ret, ud, sizeof(ret));
  if (!ret) {
    luaL_error(L, "called into dead object");
  }

  luaL_checkudata(L, narg, type);
  return ret;
}

bool lupb_refcounted_pushwrapper(lua_State *L, const upb_refcounted *obj,
                                 const char *type, const void *ref_donor,
                                 size_t size) {
  bool create;
  void *ud;

  if (obj == NULL) {
    lua_pushnil(L);
    return false;
  }

  /* Lookup our cache in the registry (we don't put our objects in the registry
   * directly because we need our cache to be a weak table). */
  lua_getfield(L, LUA_REGISTRYINDEX, LUPB_OBJCACHE);
  UPB_ASSERT(!lua_isnil(L, -1));  /* Should have been created by luaopen_upb. */
  lua_pushlightuserdata(L, (void*)obj);
  lua_rawget(L, -2);
  /* Stack is now: objcache, cached value. */

  create = false;

  if (lua_isnil(L, -1)) {
    create = true;
  } else {
    void *ud = lua_touserdata(L, -1);
    void *ud_obj;
    lupb_assert(L, ud);
    memcpy(&ud_obj, ud, sizeof(void*));

    /* A corner case: it is possible for the value to be GC'd
     * already, in which case we should evict this entry and create
     * a new one. */
    if (ud_obj == NULL) {
      create = true;
    }
  }

  ud = NULL;

  if (create) {
    /* Remove bad cached value and push new value. */
    lua_pop(L, 1);

    /* All of our userdata begin with a pointer to the obj. */
    ud = lua_newuserdata(L, size);
    memcpy(ud, &obj, sizeof(void*));
    upb_refcounted_donateref(obj, ref_donor, ud);

    luaL_getmetatable(L, type);
    /* Should have been created by luaopen_upb. */
    lupb_assert(L, !lua_isnil(L, -1));
    lua_setmetatable(L, -2);

    /* Set it in the cache. */
    lua_pushlightuserdata(L, (void*)obj);
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
  } else {
    /* Existing wrapper obj already has a ref. */
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
  UPB_ASSERT(created == true);
}

int lupb_refcounted_gc(lua_State *L) {
  void *ud = lua_touserdata(L, 1);
  void *nullp;
  upb_refcounted *obj;
  memcpy(&obj, ud, sizeof(obj));
  upb_refcounted_unref(obj, ud);

  /* Zero out pointer so we can detect a call into a GC'd object. */
  nullp = NULL;
  memcpy(ud, &nullp, sizeof(nullp));

  return 0;
}


/* lupb_def *******************************************************************/

static const upb_def *lupb_def_check(lua_State *L, int narg) {
  upb_def *ret;
  void *ud, *ud2;

  ud = lua_touserdata(L, narg);
  if (!ud) {
    luaL_typerror(L, narg, "upb def");
  }

  memcpy(&ret, ud, sizeof(ret));
  if (!ret) {
    luaL_error(L, "called into dead object");
  }

  ud2 = luaL_testudata(L, narg, LUPB_MSGDEF);
  if (!ud2) ud2 = luaL_testudata(L, narg, LUPB_ENUMDEF);
  if (!ud2) ud2 = luaL_testudata(L, narg, LUPB_FIELDDEF);
  if (!ud2) luaL_typerror(L, narg, "upb def");

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
  const char *type = NULL;
  bool created;

  if (def == NULL) {
    lua_pushnil(L);
    return false;
  }

  switch (upb_def_type(def)) {
    case UPB_DEF_MSG:
      type = LUPB_MSGDEF;
      break;
    case UPB_DEF_FIELD:
      type = LUPB_FIELDDEF;
      break;
    case UPB_DEF_ENUM:
      type = LUPB_ENUMDEF;
      break;
    default:
      printf("Def type: %d\n", (int)upb_def_type(def));
      UPB_UNREACHABLE();
  }

  created = lupb_refcounted_pushwrapper(L, upb_def_upcast(def), type, ref_donor,
                                        sizeof(void *));
  return created;
}

void lupb_def_pushnewrapper(lua_State *L, const upb_def *def,
                            const void *ref_donor) {
  bool created = lupb_def_pushwrapper(L, def, ref_donor);
  UPB_ASSERT(created == true);
}

static int lupb_def_type(lua_State *L) {
  const upb_def *def = lupb_def_check(L, 1);
  lua_pushinteger(L, upb_def_type(def));
  return 1;
}

void lupb_filedef_pushwrapper(lua_State *L, const upb_filedef *f,
                              const void *ref_donor);

static int lupb_def_file(lua_State *L) {
  const upb_def *def = lupb_def_check(L, 1);
  lupb_filedef_pushwrapper(L, upb_def_file(def), NULL);
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

static int lupb_def_name(lua_State *L) {
  const upb_def *def = lupb_def_check(L, 1);
  lua_pushstring(L, upb_def_name(def));
  return 1;
}

static int lupb_def_setfullname(lua_State *L) {
  const char *name = lupb_checkname(L, 2);
  CHK(upb_def_setfullname(lupb_def_checkmutable(L, 1), name, &status));
  return 0;
}

#define LUPB_COMMON_DEF_METHODS \
  {"def_type", lupb_def_type},  \
  {"file", lupb_def_file}, \
  {"full_name", lupb_def_fullname}, \
  {"freeze", lupb_def_freeze}, \
  {"is_frozen", lupb_def_isfrozen}, \
  {"name", lupb_def_name}, \
  {"set_full_name", lupb_def_setfullname}, \


/* lupb_fielddef **************************************************************/

void lupb_fielddef_pushwrapper(lua_State *L, const upb_fielddef *f,
                               const void *ref_donor) {
  lupb_def_pushwrapper(L, upb_fielddef_upcast(f), ref_donor);
}

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
  lupb_def_pushnewrapper(L, upb_fielddef_upcast(f), &f);
  return 1;
}

/* Getters */

void lupb_oneofdef_pushwrapper(lua_State *L, const upb_oneofdef *o,
                               const void *ref_donor);

static int lupb_fielddef_containingoneof(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_oneofdef_pushwrapper(L, upb_fielddef_containingoneof(f), NULL);
  return 1;
}

static int lupb_fielddef_containingtype(lua_State *L) {
  const upb_fielddef *f = lupb_fielddef_check(L, 1);
  lupb_msgdef_pushwrapper(L, upb_fielddef_containingtype(f), NULL);
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
  const upb_def *def;

  if (!upb_fielddef_hassubdef(f))
    luaL_error(L, "Tried to get subdef of non-message field");
  def = upb_fielddef_subdef(f);
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

/* Setters */

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
      /* Else fall through and set string default. */
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

static const struct luaL_Reg lupb_fielddef_mm[] = {
  {"__gc", lupb_refcounted_gc},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_fielddef_m[] = {
  LUPB_COMMON_DEF_METHODS

  {"containing_oneof", lupb_fielddef_containingoneof},
  {"containing_type", lupb_fielddef_containingtype},
  {"containing_type_name", lupb_fielddef_containingtypename},
  {"default", lupb_fielddef_default},
  {"descriptor_type", lupb_fielddef_descriptortype},
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

  {NULL, NULL}
};


/* lupb_oneofdef **************************************************************/

void lupb_oneofdef_pushwrapper(lua_State *L, const upb_oneofdef *o,
                               const void *ref_donor) {
  lupb_refcounted_pushwrapper(L, upb_oneofdef_upcast(o), LUPB_ONEOFDEF,
                              ref_donor, sizeof(void *));
}

const upb_oneofdef *lupb_oneofdef_check(lua_State *L, int narg) {
  return lupb_refcounted_check(L, narg, LUPB_ONEOFDEF);
}

static upb_oneofdef *lupb_oneofdef_checkmutable(lua_State *L, int narg) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, narg);
  if (upb_oneofdef_isfrozen(o))
    luaL_error(L, "not allowed on frozen value");
  return (upb_oneofdef*)o;
}

static int lupb_oneofdef_new(lua_State *L) {
  upb_oneofdef *o = upb_oneofdef_new(&o);
  lupb_refcounted_pushnewrapper(L, upb_oneofdef_upcast(o), LUPB_ONEOFDEF, &o);
  return 1;
}

/* Getters */

static int lupb_oneofdef_containingtype(lua_State *L) {
  const upb_oneofdef *o = lupb_oneofdef_check(L, 1);
  lupb_def_pushwrapper(L, upb_msgdef_upcast(upb_oneofdef_containingtype(o)),
                       NULL);
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

  lupb_def_pushwrapper(L, upb_fielddef_upcast(f), NULL);
  return 1;
}

static int lupb_oneofiter_next(lua_State *L) {
  upb_oneof_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_oneof_done(i)) return 0;
  lupb_fielddef_pushwrapper(L, upb_oneof_iter_field(i), NULL);
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

/* Setters */

static int lupb_oneofdef_add(lua_State *L) {
  upb_oneofdef *o = lupb_oneofdef_checkmutable(L, 1);
  upb_fielddef *f = lupb_fielddef_checkmutable(L, 2);
  CHK(upb_oneofdef_addfield(o, f, NULL, &status));
  return 0;
}

static int lupb_oneofdef_setname(lua_State *L) {
  upb_oneofdef *o = lupb_oneofdef_checkmutable(L, 1);
  CHK(upb_oneofdef_setname(o, lupb_checkname(L, 2), &status));
  return 0;
}

static const struct luaL_Reg lupb_oneofdef_m[] = {
  {"containing_type", lupb_oneofdef_containingtype},
  {"field", lupb_oneofdef_field},
  {"fields", lupb_oneofdef_fields},
  {"name", lupb_oneofdef_name},

  {"add", lupb_oneofdef_add},
  {"set_name", lupb_oneofdef_setname},

  {NULL, NULL}
};

static const struct luaL_Reg lupb_oneofdef_mm[] = {
  {"__gc", lupb_refcounted_gc},
  {"__len", lupb_oneofdef_len},
  {NULL, NULL}
};


/* lupb_msgdef ****************************************************************/

typedef struct {
  const upb_msgdef *md;
} lupb_msgdef;

void lupb_msgdef_pushwrapper(lua_State *L, const upb_msgdef *m,
                             const void *ref_donor) {
  lupb_def_pushwrapper(L, upb_msgdef_upcast(m), ref_donor);
}

const upb_msgdef *lupb_msgdef_check(lua_State *L, int narg) {
  return lupb_refcounted_check(L, narg, LUPB_MSGDEF);
}

static upb_msgdef *lupb_msgdef_checkmutable(lua_State *L, int narg) {
  const upb_msgdef *m = lupb_msgdef_check(L, narg);
  if (upb_msgdef_isfrozen(m))
    luaL_error(L, "not allowed on frozen value");
  return (upb_msgdef*)m;
}

static int lupb_msgdef_new(lua_State *L) {
  upb_msgdef *md = upb_msgdef_new(&md);
  lupb_def_pushnewrapper(L, upb_msgdef_upcast(md), &md);
  return 1;
}

static int lupb_msgdef_add(lua_State *L) {
  upb_msgdef *m = lupb_msgdef_checkmutable(L, 1);

  /* Both oneofs and fields can be added. */
  if (luaL_testudata(L, 2, LUPB_FIELDDEF)) {
    upb_fielddef *f = lupb_fielddef_checkmutable(L, 2);
    CHK(upb_msgdef_addfield(m, f, NULL, &status));
  } else if (luaL_testudata(L, 2, LUPB_ONEOFDEF)) {
    upb_oneofdef *o = lupb_oneofdef_checkmutable(L, 2);
    CHK(upb_msgdef_addoneof(m, o, NULL, &status));
  }
  return 0;
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

  lupb_def_pushwrapper(L, upb_fielddef_upcast(f), NULL);
  return 1;
}

static int lupb_msgdef_lookupname(lua_State *L) {
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  const upb_fielddef *f;
  const upb_oneofdef *o;
  if (!upb_msgdef_lookupnamez(m, lua_tostring(L, 2), &f, &o)) {
    lua_pushnil(L);
  } else if (o) {
    lupb_oneofdef_pushwrapper(L, o, NULL);
  } else {
    lupb_fielddef_pushwrapper(L, f, NULL);
  }
  return 1;
}

static int lupb_msgfielditer_next(lua_State *L) {
  upb_msg_field_iter *i = lua_touserdata(L, lua_upvalueindex(1));
  if (upb_msg_field_done(i)) return 0;
  lupb_def_pushwrapper(L, upb_fielddef_upcast(upb_msg_iter_field(i)), NULL);
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
  lupb_oneofdef_pushwrapper(L, upb_msg_iter_oneof(i), NULL);
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
  {"__gc", lupb_refcounted_gc},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_msgdef_m[] = {
  LUPB_COMMON_DEF_METHODS
  {"add", lupb_msgdef_add},
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
  lupb_def_pushnewrapper(L, upb_enumdef_upcast(e), &e);
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
  {"__gc", lupb_refcounted_gc},
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


/* lupb_filedef ***************************************************************/

void lupb_filedef_pushwrapper(lua_State *L, const upb_filedef *f,
                              const void *ref_donor) {
  lupb_refcounted_pushwrapper(L, upb_filedef_upcast(f), LUPB_FILEDEF, ref_donor,
                              sizeof(void *));
}

void lupb_filedef_pushnewrapper(lua_State *L, const upb_filedef *f,
                                const void *ref_donor) {
  lupb_refcounted_pushnewrapper(L, upb_filedef_upcast(f), LUPB_FILEDEF,
                                ref_donor);
}

const upb_filedef *lupb_filedef_check(lua_State *L, int narg) {
  return lupb_refcounted_check(L, narg, LUPB_FILEDEF);
}

static upb_filedef *lupb_filedef_checkmutable(lua_State *L, int narg) {
  const upb_filedef *f = lupb_filedef_check(L, narg);
  if (upb_filedef_isfrozen(f))
    luaL_error(L, "not allowed on frozen value");
  return (upb_filedef*)f;
}

static int lupb_filedef_new(lua_State *L) {
  upb_filedef *f = upb_filedef_new(&f);
  lupb_filedef_pushnewrapper(L, f, &f);
  return 1;
}

static int lupb_filedef_def(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  int index = luaL_checkint(L, 2);
  const upb_def *def = upb_filedef_def(f, index);

  if (!def) {
    return luaL_error(L, "index out of range");
  }

  lupb_def_pushwrapper(L, def, NULL);
  return 1;
}

static int lupb_filedefdepiter_next(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, lua_upvalueindex(1));
  size_t i = lua_tointeger(L, lua_upvalueindex(2));

  if (i >= upb_filedef_depcount(f)) {
    return 0;
  }

  lupb_filedef_pushwrapper(L, upb_filedef_dep(f, i), NULL);
  lua_pushinteger(L, i + 1);
  lua_replace(L, lua_upvalueindex(2));
  return 1;
}


static int lupb_filedef_dependencies(lua_State *L) {
  lupb_filedef_check(L, 1);
  lua_pushvalue(L, 1);
  lua_pushnumber(L, 0);      /* Index, starts at zero. */
  lua_pushcclosure(L, &lupb_filedefdepiter_next, 2);
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

static int lupb_filedef_len(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, 1);
  lua_pushinteger(L, upb_filedef_defcount(f));
  return 1;
}

static int lupb_filedef_setname(lua_State *L) {
  upb_filedef *f = lupb_filedef_checkmutable(L, 1);
  CHK(upb_filedef_setname(f, lupb_checkname(L, 2), &status));
  return 0;
}

static int lupb_filedef_setpackage(lua_State *L) {
  upb_filedef *f = lupb_filedef_checkmutable(L, 1);
  CHK(upb_filedef_setpackage(f, lupb_checkname(L, 2), &status));
  return 0;
}

static int lupb_filedefiter_next(lua_State *L) {
  const upb_filedef *f = lupb_filedef_check(L, lua_upvalueindex(1));
  int type = lua_tointeger(L, lua_upvalueindex(2));
  size_t i = lua_tointeger(L, lua_upvalueindex(3));
  size_t n = upb_filedef_defcount(f);

  for (; i < n; i++) {
    const upb_def *def;

    def = upb_filedef_def(f, i);
    UPB_ASSERT(def);

    if (type == UPB_DEF_ANY || upb_def_type(def) == type) {
      lua_pushinteger(L, i + 1);
      lua_replace(L, lua_upvalueindex(3));
      lupb_def_pushwrapper(L, def, NULL);
      return 1;
    }
  }

  return 0;
}

static int lupb_filedef_defs(lua_State *L) {
  lupb_filedef_check(L, 1);
  luaL_checkinteger(L, 2);   /* Def type.  Could make this optional. */
  lua_pushnumber(L, 0);      /* Index, starts at zero. */
  lua_pushcclosure(L, &lupb_filedefiter_next, 3);
  return 1;
}

static const struct luaL_Reg lupb_filedef_mm[] = {
  {"__gc", lupb_refcounted_gc},
  {"__len", lupb_filedef_len},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_filedef_m[] = {
  {"def", lupb_filedef_def},
  {"defs", lupb_filedef_defs},
  {"dependencies", lupb_filedef_dependencies},
  {"name", lupb_filedef_name},
  {"package", lupb_filedef_package},
  {"syntax", lupb_filedef_syntax},

  {"set_name", lupb_filedef_setname},
  {"set_package", lupb_filedef_setpackage},

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

static int lupb_symtab_add(lua_State *L) {
  upb_symtab *s = lupb_symtab_check(L, 1);
  int n;
  upb_def **defs;

  luaL_checktype(L, 2, LUA_TTABLE);
  /* Iterate over table twice.  First iteration to count entries and
   * check constraints. */
  n = 0;
  for (lua_pushnil(L); lua_next(L, 2); lua_pop(L, 1)) {
    lupb_def_checkmutable(L, -1);
    ++n;
  }

  /* Second iteration to build deflist.
   * Allocate list with lua_newuserdata() so it is anchored as a GC root in
   * case any Lua functions longjmp(). */
  defs = lua_newuserdata(L, n * sizeof(*defs));
  n = 0;
  for (lua_pushnil(L); lua_next(L, 2); lua_pop(L, 1)) {
    upb_def *def = lupb_def_checkmutable(L, -1);
    defs[n++] = def;
  }

  CHK(upb_symtab_add(s, defs, n, NULL, &status));
  return 0;
}

static int lupb_symtab_addfile(lua_State *L) {
  upb_symtab *s = lupb_symtab_check(L, 1);
  upb_filedef *f = lupb_filedef_checkmutable(L, 2);
  CHK(upb_symtab_addfile(s, f, &status));
  return 0;
}

static int lupb_symtab_lookup(lua_State *L) {
  const upb_symtab *s = lupb_symtab_check(L, 1);
  int i;
  for (i = 2; i <= lua_gettop(L); i++) {
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
  /* Need to guarantee that the symtab outlives the iter. */
  lua_pushvalue(L, 1);
  lua_pushcclosure(L, &lupb_symtabiter_next, 2);
  return 1;
}

static const struct luaL_Reg lupb_symtab_m[] = {
  {"add", lupb_symtab_add},
  {"add_file", lupb_symtab_addfile},
  {"defs", lupb_symtab_defs},
  {"lookup", lupb_symtab_lookup},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_symtab_mm[] = {
  {"__gc", lupb_symtab_gc},
  {NULL, NULL}
};

/* lupb toplevel **************************************************************/

static int lupb_freeze(lua_State *L) {
  int n = lua_gettop(L);
  int i;
  /* Scratch memory; lua_newuserdata() anchors it as a GC root in case any Lua
   * functions fail. */
  upb_def **defs = lua_newuserdata(L, n * sizeof(upb_def*));

  for (i = 0; i < n; i++) {
    /* Could allow an array of defs here also. */
    defs[i] = lupb_def_checkmutable(L, i + 1);
  }
  CHK(upb_def_freeze(defs, n, &status));
  return 0;
}

/* This is a *temporary* API that will be removed once pending refactorings are
 * complete (it does not belong here in core because it depends on both
 * the descriptor.proto schema and the protobuf binary format. */
static int lupb_loaddescriptor(lua_State *L) {
  size_t len;
  const char *str = luaL_checklstring(L, 1, &len);
  size_t i;
  upb_filedef **files = NULL;
  CHK(files = upb_loaddescriptor(str, len, &files, &status));

  lua_newtable(L);
  for (i = 1; *files; i++, files++) {
    lupb_filedef_pushnewrapper(L, *files, &files);
    lua_rawseti(L, -2, i);
  }

  return 1;
}

static void lupb_setfieldi(lua_State *L, const char *field, int i) {
  lua_pushinteger(L, i);
  lua_setfield(L, -2, field);
}

static const struct luaL_Reg lupbdef_toplevel_m[] = {
  {"EnumDef", lupb_enumdef_new},
  {"FieldDef", lupb_fielddef_new},
  {"FileDef", lupb_filedef_new},
  {"MessageDef", lupb_msgdef_new},
  {"OneofDef", lupb_oneofdef_new},
  {"SymbolTable", lupb_symtab_new},
  {"freeze", lupb_freeze},
  {"load_descriptor", lupb_loaddescriptor},

  {NULL, NULL}
};

void lupb_def_registertypes(lua_State *L) {
  lupb_setfuncs(L, lupbdef_toplevel_m);

  /* Refcounted types. */
  lupb_register_type(L, LUPB_ENUMDEF,  lupb_enumdef_m,  lupb_enumdef_mm);
  lupb_register_type(L, LUPB_FIELDDEF, lupb_fielddef_m, lupb_fielddef_mm);
  lupb_register_type(L, LUPB_FILEDEF,  lupb_filedef_m,  lupb_filedef_mm);
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

  lupb_setfieldi(L, "SYNTAX_PROTO2",  UPB_SYNTAX_PROTO2);
  lupb_setfieldi(L, "SYNTAX_PROTO3",  UPB_SYNTAX_PROTO3);
}
