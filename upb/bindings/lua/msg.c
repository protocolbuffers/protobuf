/*
** lupb_msg -- Message/Array/Map objects in Lua/C that wrap upb/msg.h
*/

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "upb/bindings/lua/upb.h"
#include "upb/handlers.h"
#include "upb/legacy_msg_reflection.h"
#include "upb/msg.h"

#include "upb/port_def.inc"

/*
 * Message/Array/Map objects can be constructed in one of two ways:
 *
 * 1. To point to existing msg/array/map data inside an arena.
 * 2. To create and uniquely own some brand new data.
 *
 * Case (1) is for when we've parsed some data into an arena (which is faster
 * than parsing directly into Lua objects) or when we're pointing at some
 * read-only data (like custom options in a def).
 *
 * Case (2) is for when a user creates the object directly in Lua.
 *
 * We use the userval of container objects (Message/Array/Map) to store
 * references to sub-objects (Strings/Messages/Arrays/Maps).  But we need to
 * keep the userval in sync with the underlying upb_msg/upb_array/upb_map.
 * We populate the userval lazily from the underlying data.
 *
 * This means that no one may remove/replace any String/Message/Array/Map
 * field/entry in the underlying upb_{msg,array,map} behind our back.  It's ok
 * for entries to be added or for primitives to be modified, but *replacing*
 * sub-containers is not.
 *
 * Luckily parse/merge follow this rule.  However clear does not, so it's not
 * safe to clear behind our back.
 */

#define LUPB_ARENA "lupb.arena"

#define LUPB_MSGCLASS "lupb.msgclass"
#define LUPB_MSGFACTORY "lupb.msgfactory"

#define LUPB_ARRAY "lupb.array"
#define LUPB_MAP "lupb.map"
#define LUPB_MSG "lupb.msg"
#define LUPB_STRING "lupb.string"

static int lupb_msg_pushnew(lua_State *L, int narg);

/* Lazily creates the uservalue if it doesn't exist. */
static void lupb_getuservalue(lua_State *L, int index) {
  lua_getuservalue(L, index);
  if (lua_isnil(L, -1)) {
    /* Lazily create and set userval. */
    lua_pop(L, 1);  /* nil. */
    lua_pushvalue(L, index); /* userdata copy. */
    lua_newtable(L);
    lua_setuservalue(L, -2);
    lua_pop(L, 1);  /* userdata copy. */
    lua_getuservalue(L, index);
  }
  assert(!lua_isnil(L, -1));
}

static void lupb_uservalseti(lua_State *L, int userdata, int index, int val) {
  lupb_getuservalue(L, userdata);
  lua_pushvalue(L, val);
  lua_rawseti(L, -2, index);
  lua_pop(L, 1);  /* Uservalue. */
}

static void lupb_uservalgeti(lua_State *L, int userdata, int index) {
  lupb_getuservalue(L, userdata);
  lua_rawgeti(L, -1, index);
  lua_insert(L, -2);
  lua_pop(L, 1);  /* Uservalue. */
}

/* Pushes a new userdata with the given metatable. */
static void *lupb_newuserdata(lua_State *L, size_t size, const char *type) {
  void *ret = lua_newuserdata(L, size);

  /* Set metatable. */
  luaL_getmetatable(L, type);
  UPB_ASSERT(!lua_isnil(L, -1));  /* Should have been created by luaopen_upb. */
  lua_setmetatable(L, -2);

  /* We don't set a uservalue here -- we lazily create it later if necessary. */

  return ret;
}


/* lupb_arena *****************************************************************/

/* lupb_arena only exists to wrap a upb_arena.  It is never exposed to users;
 * it is an internal memory management detail.  Other objects refer to this
 * object from their userdata to keep the arena-owned data alive. */

typedef struct {
  upb_arena *arena;
} lupb_arena;

upb_arena *lupb_arena_check(lua_State *L, int narg) {
  lupb_arena *a = luaL_checkudata(L, narg, LUPB_ARENA);
  return a ? a->arena : NULL;
}

int lupb_arena_new(lua_State *L) {
  lupb_arena *a = lupb_newuserdata(L, sizeof(lupb_arena), LUPB_ARENA);

  /* TODO(haberman): use Lua alloc func as block allocator?  Would need to
   * verify that all cases of upb_malloc in msg/table are longjmp-safe. */
  a->arena = upb_arena_new();

  return 1;
}

char lupb_arena_cache_key;

/* Returns the global lupb_arena func that was created in our luaopen().
 * Callers can be guaranteed that it will be alive as long as |L| is.
 * TODO(haberman): we shouldn't use a global arena!  We should have
 * one arena for a parse, or per independently-created message. */
upb_arena *lupb_arena_get(lua_State *L) {
  upb_arena *arena;

  lua_pushlightuserdata(L, &lupb_arena_cache_key);
  lua_gettable(L, LUA_REGISTRYINDEX);
  arena = lua_touserdata(L, -1);
  UPB_ASSERT(arena);
  lua_pop(L, 1);

  return arena;
}

static void lupb_arena_initsingleton(lua_State *L) {
  lua_pushlightuserdata(L, &lupb_arena_cache_key);
  lupb_arena_new(L);
  lua_settable(L, LUA_REGISTRYINDEX);
}

static int lupb_arena_gc(lua_State *L) {
  upb_arena *a = lupb_arena_check(L, 1);
  upb_arena_free(a);
  return 0;
}

static const struct luaL_Reg lupb_arena_mm[] = {
  {"__gc", lupb_arena_gc},
  {NULL, NULL}
};


/* lupb_msgfactory ************************************************************/

/* Userval contains a map of:
 *   [1] -> SymbolTable (to keep GC-reachable)
 *   [const upb_msgdef*] -> [lupb_msgclass userdata]
 */

#define LUPB_MSGFACTORY_SYMTAB 1

typedef struct lupb_msgfactory {
  upb_msgfactory *factory;
} lupb_msgfactory;

static int lupb_msgclass_pushnew(lua_State *L, int factory,
                                 const upb_msgdef *md);

/* lupb_msgfactory helpers. */

static lupb_msgfactory *lupb_msgfactory_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_MSGFACTORY);
}

static void lupb_msgfactory_pushmsgclass(lua_State *L, int narg,
                                         const upb_msgdef *md) {
  lupb_getuservalue(L, narg);
  lua_pushlightuserdata(L, (void*)md);
  lua_rawget(L, -2);

  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    /* TODO: verify md is in symtab? */
    lupb_msgclass_pushnew(L, narg, md);

    /* Set in userval. */
    lua_pushlightuserdata(L, (void*)md);
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
  }
}

static int lupb_msgfactory_gc(lua_State *L) {
  lupb_msgfactory *lfactory = lupb_msgfactory_check(L, 1);

  if (lfactory->factory) {
    upb_msgfactory_free(lfactory->factory);
    lfactory->factory = NULL;
  }

  return 0;
}

/* lupb_msgfactory Public API. */

/**
 * lupb_msgfactory_new()
 *
 * Handles:
 *   msgfactory = upb.MessageFactory(symtab)
 *
 * Creates a new, empty MessageFactory for the given SymbolTable.
 * Message classes will be created on demand when the user calls
 * msgfactory.get_message_class().
 */
static int lupb_msgfactory_new(lua_State *L) {
  const upb_symtab *symtab = lupb_symtab_check(L, 1);

  lupb_msgfactory *lmsgfactory =
      lupb_newuserdata(L, sizeof(lupb_msgfactory), LUPB_MSGFACTORY);
  lmsgfactory->factory = upb_msgfactory_new(symtab);
  lupb_uservalseti(L, -1, LUPB_MSGFACTORY_SYMTAB, 1);

  return 1;
}

/**
 * lupb_msgfactory_getmsgclass()
 *
 * Handles:
 *   MessageClass = factory.get_message_class(message_name)
 */
static int lupb_msgfactory_getmsgclass(lua_State *L) {
  lupb_msgfactory *lfactory = lupb_msgfactory_check(L, 1);
  const upb_symtab *symtab = upb_msgfactory_symtab(lfactory->factory);
  const upb_msgdef *m = upb_symtab_lookupmsg(symtab, luaL_checkstring(L, 2));

  if (!m) {
    luaL_error(L, "No such message type: %s\n", lua_tostring(L, 2));
  }

  lupb_msgfactory_pushmsgclass(L, 1, m);

  return 1;
}

static const struct luaL_Reg lupb_msgfactory_m[] = {
  {"get_message_class", lupb_msgfactory_getmsgclass},
  {NULL, NULL}
};

static const struct luaL_Reg lupb_msgfactory_mm[] = {
  {"__gc", lupb_msgfactory_gc},
  {NULL, NULL}
};


/* lupb_msgclass **************************************************************/

/* Userval contains a map of:
 *   [1] -> MessageFactory (to keep GC-reachable)
 *   [const upb_msgdef*] -> [lupb_msgclass userdata]
 */

#define LUPB_MSGCLASS_FACTORY 1

struct lupb_msgclass {
  const upb_msglayout *layout;
  const upb_msgdef *msgdef;
  const lupb_msgfactory *lfactory;
};

/* Type-checks for assigning to a message field. */
static upb_msgval lupb_array_typecheck(lua_State *L, int narg, int msg,
                                       const upb_fielddef *f);
static upb_msgval lupb_map_typecheck(lua_State *L, int narg, int msg,
                                     const upb_fielddef *f);
static const lupb_msgclass *lupb_msg_getsubmsgclass(lua_State *L, int narg,
                                                    const upb_fielddef *f);
static const lupb_msgclass *lupb_msg_msgclassfor(lua_State *L, int narg,
                                                 const upb_msgdef *md);

const lupb_msgclass *lupb_msgclass_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_MSGCLASS);
}

const upb_msglayout *lupb_msgclass_getlayout(lua_State *L, int narg) {
  return lupb_msgclass_check(L, narg)->layout;
}

const upb_msgdef *lupb_msgclass_getmsgdef(const lupb_msgclass *lmsgclass) {
  return lmsgclass->msgdef;
}

upb_msgfactory *lupb_msgclass_getfactory(const lupb_msgclass *lmsgclass) {
  return lmsgclass->lfactory->factory;
}

/**
 * lupb_msgclass_typecheck()
 *
 * Verifies that the expected msgclass matches the actual.  If not, raises a Lua
 * error.
 */
static void lupb_msgclass_typecheck(lua_State *L, const lupb_msgclass *expected,
                                    const lupb_msgclass *actual) {
  if (expected != actual) {
    luaL_error(L, "Message had incorrect type, expected '%s', got '%s'",
               upb_msgdef_fullname(expected->msgdef),
               upb_msgdef_fullname(actual->msgdef));
  }
}

static const lupb_msgclass *lupb_msgclass_msgclassfor(lua_State *L, int narg,
                                                      const upb_msgdef *md) {
  lupb_uservalgeti(L, narg, LUPB_MSGCLASS_FACTORY);
  lupb_msgfactory_pushmsgclass(L, -1, md);
  return lupb_msgclass_check(L, -1);
}

/**
 * lupb_msgclass_getsubmsgclass()
 *
 * Given a MessageClass at index |narg| and the submessage field |f|, returns
 * the message class for this field.
 *
 * Currently we do a hash table lookup for this.  If we wanted we could try to
 * optimize this by caching these pointers in our msgclass, in an array indexed
 * by field index.  We would still need to fall back to calling msgclassfor(),
 * unless we wanted to eagerly create message classes for all submessages.  But
 * for big schemas that might be a lot of things to build, and we might end up
 * not using most of them. */
static const lupb_msgclass *lupb_msgclass_getsubmsgclass(lua_State *L, int narg,
                                                         const upb_fielddef *f) {
  if (upb_fielddef_type(f) != UPB_TYPE_MESSAGE) {
    return NULL;
  }

  return lupb_msgclass_msgclassfor(L, narg, upb_fielddef_msgsubdef(f));
}

static int lupb_msgclass_pushnew(lua_State *L, int factory,
                                 const upb_msgdef *md) {
  const lupb_msgfactory *lfactory = lupb_msgfactory_check(L, factory);
  lupb_msgclass *lmc = lupb_newuserdata(L, sizeof(*lmc), LUPB_MSGCLASS);

  lupb_uservalseti(L, -1, LUPB_MSGCLASS_FACTORY, factory);
  lmc->layout = upb_msgfactory_getlayout(lfactory->factory, md);
  lmc->lfactory = lfactory;
  lmc->msgdef = md;

  return 1;
}

/* MessageClass Public API. */

/**
 * lupb_msgclass_call()
 *
 * Handles:
 *   msg = MessageClass()
 *
 * Creates a new message from the given MessageClass.
 */
static int lupb_msgclass_call(lua_State *L) {
  lupb_msg_pushnew(L, 1);
  return 1;
}

static const struct luaL_Reg lupb_msgclass_mm[] = {
  {"__call", lupb_msgclass_call},
  {NULL, NULL}
};


/* upb <-> Lua type conversion ************************************************/

static bool lupb_istypewrapped(upb_fieldtype_t type) {
  return type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES ||
         type == UPB_TYPE_MESSAGE;
}

static upb_msgval lupb_tomsgval(lua_State *L, upb_fieldtype_t type, int narg,
                                const lupb_msgclass *lmsgclass) {
  switch (type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      return upb_msgval_int32(lupb_checkint32(L, narg));
    case UPB_TYPE_INT64:
      return upb_msgval_int64(lupb_checkint64(L, narg));
    case UPB_TYPE_UINT32:
      return upb_msgval_uint32(lupb_checkuint32(L, narg));
    case UPB_TYPE_UINT64:
      return upb_msgval_uint64(lupb_checkuint64(L, narg));
    case UPB_TYPE_DOUBLE:
      return upb_msgval_double(lupb_checkdouble(L, narg));
    case UPB_TYPE_FLOAT:
      return upb_msgval_float(lupb_checkfloat(L, narg));
    case UPB_TYPE_BOOL:
      return upb_msgval_bool(lupb_checkbool(L, narg));
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      size_t len;
      const char *ptr = lupb_checkstring(L, narg, &len);
      return upb_msgval_makestr(ptr, len);
    }
    case UPB_TYPE_MESSAGE:
      UPB_ASSERT(lmsgclass);
      return upb_msgval_msg(lupb_msg_checkmsg(L, narg, lmsgclass));
  }
  UPB_UNREACHABLE();
}

static void lupb_pushmsgval(lua_State *L, upb_fieldtype_t type,
                            upb_msgval val) {
  switch (type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      lupb_pushint32(L, upb_msgval_getint32(val));
      return;
    case UPB_TYPE_INT64:
      lupb_pushint64(L, upb_msgval_getint64(val));
      return;
    case UPB_TYPE_UINT32:
      lupb_pushuint32(L, upb_msgval_getuint32(val));
      return;
    case UPB_TYPE_UINT64:
      lupb_pushuint64(L, upb_msgval_getuint64(val));
      return;
    case UPB_TYPE_DOUBLE:
      lupb_pushdouble(L, upb_msgval_getdouble(val));
      return;
    case UPB_TYPE_FLOAT:
      lupb_pushfloat(L, upb_msgval_getfloat(val));
      return;
    case UPB_TYPE_BOOL:
      lua_pushboolean(L, upb_msgval_getbool(val));
      return;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      break;  /* Shouldn't call this function. */
  }
  UPB_UNREACHABLE();
}


/* lupb_array *****************************************************************/

/* A strongly typed array.  Implemented by wrapping upb_array.
 *
 * - we only allow integer indices.
 * - all entries must have the correct type.
 * - we do not allow "holes" in the array; you can only assign to an existing
 *   index or one past the end (which will grow the array by one).
 *
 * For string/submessage entries we keep in the userval:
 *
 *   [number index] -> [lupb_string/lupb_msg userdata]
 */

typedef struct {
  /* Only needed for array of message.  This wastes space in the non-message
   * case but simplifies the code.  Could optimize away if desired. */
  const lupb_msgclass *lmsgclass;
  upb_array *arr;
  upb_fieldtype_t type;
} lupb_array;

#define ARRAY_MSGCLASS_INDEX 0

static lupb_array *lupb_array_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_ARRAY);
}

/**
 * lupb_array_typecheck()
 *
 * Verifies that the lupb_array object at index |narg| can be safely assigned
 * to the field |f| of the lupb_msg object at index |msg|.  If this is safe,
 * returns a upb_msgval representing the array.  Otherwise, throws a Lua error.
 */
static upb_msgval lupb_array_typecheck(lua_State *L, int narg, int msg,
                                       const upb_fielddef *f) {
  lupb_array *larray = lupb_array_check(L, narg);

  if (upb_array_type(larray->arr) != upb_fielddef_type(f) ||
      lupb_msg_getsubmsgclass(L, msg, f) != larray->lmsgclass) {
    luaL_error(L, "Array had incorrect type (expected: %d, got: %d)",
               (int)upb_fielddef_type(f), (int)upb_array_type(larray->arr));
  }

  if (upb_array_type(larray->arr) == UPB_TYPE_MESSAGE) {
    lupb_msgclass_typecheck(L, lupb_msg_getsubmsgclass(L, msg, f),
                            larray->lmsgclass);
  }

  return upb_msgval_arr(larray->arr);
}

/**
 * lupb_array_checkindex()
 *
 * Checks the array index at Lua stack index |narg| to verify that it is an
 * integer between 1 and |max|, inclusively.  Also corrects it to be zero-based
 * for C.
 *
 * We use "int" because of lua_rawseti/lua_rawgeti -- can re-evaluate if we want
 * arrays bigger than 2^31.
 */
static int lupb_array_checkindex(lua_State *L, int narg, uint32_t max) {
  uint32_t n = lupb_checkuint32(L, narg);
  if (n == 0 || n > max || n > INT_MAX) {
    luaL_error(L, "Invalid array index: expected between 1 and %d", (int)max);
  }
  return n - 1;  /* Lua uses 1-based indexing. :( */
}

/* lupb_array Public API */

static int lupb_array_new(lua_State *L) {
  lupb_array *larray;
  upb_fieldtype_t type;
  const lupb_msgclass *lmsgclass = NULL;

  if (lua_type(L, 1) == LUA_TNUMBER) {
    type = lupb_checkfieldtype(L, 1);
  } else {
    type = UPB_TYPE_MESSAGE;
    lmsgclass = lupb_msgclass_check(L, 1);
    lupb_uservalseti(L, -1, ARRAY_MSGCLASS_INDEX, 1);  /* GC-root lmsgclass. */
  }

  larray = lupb_newuserdata(L, sizeof(*larray), LUPB_ARRAY);
  larray->type = type;
  larray->lmsgclass = lmsgclass;
  larray->arr = upb_array_new(lupb_arena_get(L));

  return 1;
}

static int lupb_array_newindex(lua_State *L) {
  lupb_array *larray = lupb_array_check(L, 1);
  upb_fieldtype_t type = upb_array_type(larray->arr);
  uint32_t n = lupb_array_checkindex(L, 2, upb_array_size(larray->arr) + 1);
  upb_msgval msgval = lupb_tomsgval(L, type, 3, larray->lmsgclass);

  upb_array_set(larray->arr, larray->type, n, msgval, lupb_arena_get(L));

  if (lupb_istypewrapped(type)) {
    lupb_uservalseti(L, 1, n, 3);
  }

  return 0;  /* 1 for chained assignments? */
}

static int lupb_array_index(lua_State *L) {
  lupb_array *larray = lupb_array_check(L, 1);
  upb_array *array = larray->arr;
  uint32_t n = lupb_array_checkindex(L, 2, upb_array_size(array));
  upb_fieldtype_t type = upb_array_type(array);

  if (lupb_istypewrapped(type)) {
    lupb_uservalgeti(L, 1, n);
  } else {
    lupb_pushmsgval(L, upb_array_type(array),
                    upb_array_get(array, larray->type, n));
  }

  return 1;
}

static int lupb_array_len(lua_State *L) {
  lupb_array *larray = lupb_array_check(L, 1);
  lua_pushnumber(L, upb_array_size(larray->arr));
  return 1;
}

static const struct luaL_Reg lupb_array_mm[] = {
  {"__index", lupb_array_index},
  {"__len", lupb_array_len},
  {"__newindex", lupb_array_newindex},
  {NULL, NULL}
};


/* lupb_map *******************************************************************/

/* A map object.  Implemented by wrapping upb_map.
 *
 * When the value type is string/bytes/message, the userval consists of:
 *
 *   [Lua number/string] -> [lupb_string/lupb_msg userdata]
 *
 * For other value types we don't use the userdata.
 */

typedef struct {
  const lupb_msgclass *value_lmsgclass;
  upb_map *map;
} lupb_map;

#define MAP_MSGCLASS_INDEX 0

/* lupb_map internal functions */

static lupb_map *lupb_map_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_ARRAY);
}

/**
 * lupb_map_typecheck()
 *
 * Checks that the lupb_map at index |narg| can be safely assigned to the
 * field |f| of the message at index |msg|.  If so, returns a upb_msgval for
 * this map.  Otherwise, raises a Lua error.
 */
static upb_msgval lupb_map_typecheck(lua_State *L, int narg, int msg,
                                     const upb_fielddef *f) {
  lupb_map *lmap = lupb_map_check(L, narg);
  upb_map *map = lmap->map;
  const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
  const upb_fielddef *key_field = upb_msgdef_itof(entry, UPB_MAPENTRY_KEY);
  const upb_fielddef *value_field = upb_msgdef_itof(entry, UPB_MAPENTRY_VALUE);

  UPB_ASSERT(entry && key_field && value_field);

  if (upb_map_keytype(map) != upb_fielddef_type(key_field)) {
    luaL_error(L, "Map key type invalid");
  }

  if (upb_map_valuetype(map) != upb_fielddef_type(value_field)) {
    luaL_error(L, "Map had incorrect value type (expected: %s, got: %s)",
               upb_fielddef_type(value_field), upb_map_valuetype(map));
  }

  if (upb_map_valuetype(map) == UPB_TYPE_MESSAGE) {
    lupb_msgclass_typecheck(
        L, lupb_msg_msgclassfor(L, msg, upb_fielddef_msgsubdef(value_field)),
        lmap->value_lmsgclass);
  }

  return upb_msgval_map(map);
}

/* lupb_map Public API */

/**
 * lupb_map_new
 *
 * Handles:
 *   new_map = upb.Map(key_type, value_type)
 */
static int lupb_map_new(lua_State *L) {
  lupb_map *lmap;
  upb_fieldtype_t key_type = lupb_checkfieldtype(L, 1);
  upb_fieldtype_t value_type;
  const lupb_msgclass *value_lmsgclass = NULL;

  if (lua_type(L, 2) == LUA_TNUMBER) {
    value_type = lupb_checkfieldtype(L, 2);
  } else {
    value_type = UPB_TYPE_MESSAGE;
  }

  lmap = lupb_newuserdata(L, sizeof(*lmap), LUPB_MAP);

  if (value_type == UPB_TYPE_MESSAGE) {
    value_lmsgclass = lupb_msgclass_check(L, 2);
    lupb_uservalseti(L, -1, MAP_MSGCLASS_INDEX, 2);  /* GC-root lmsgclass. */
  }

  lmap->value_lmsgclass = value_lmsgclass;
  lmap->map = upb_map_new(key_type, value_type, lupb_arena_get(L));

  return 1;
}

/**
 * lupb_map_index
 *
 * Handles:
 *   map[key]
 */
static int lupb_map_index(lua_State *L) {
  lupb_map *lmap = lupb_map_check(L, 1);
  upb_map *map = lmap->map;
  upb_fieldtype_t valtype = upb_map_valuetype(map);
  /* We don't always use "key", but this call checks the key type. */
  upb_msgval key = lupb_tomsgval(L, upb_map_keytype(map), 2, NULL);

  if (lupb_istypewrapped(valtype)) {
    /* Userval contains the full map, lookup there by key. */
    lupb_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);

    if (lua_isnil(L, -1)) {
      /* TODO: lazy read from upb_map */
    }
  } else {
    /* Lookup in upb_map. */
    upb_msgval val;
    if (upb_map_get(map, key, &val)) {
      lupb_pushmsgval(L, upb_map_valuetype(map), val);
    } else {
      lua_pushnil(L);
    }
  }

  return 1;
}

/**
 * lupb_map_len
 *
 * Handles:
 *   map_len = #map
 */
static int lupb_map_len(lua_State *L) {
  lupb_map *lmap = lupb_map_check(L, 1);
  lua_pushnumber(L, upb_map_size(lmap->map));
  return 1;
}

/**
 * lupb_map_newindex
 *
 * Handles:
 *   map[key] = val
 *   map[key] = nil  # to remove from map
 */
static int lupb_map_newindex(lua_State *L) {
  lupb_map *lmap = lupb_map_check(L, 1);
  upb_map *map = lmap->map;
  upb_msgval key = lupb_tomsgval(L, upb_map_keytype(map), 2, NULL);

  if (lua_isnil(L, 3)) {
    /* Delete from map. */
    upb_map_del(map, key);

    if (lupb_istypewrapped(upb_map_valuetype(map))) {
      /* Delete in userval. */
      lupb_getuservalue(L, 1);
      lua_pushvalue(L, 2);
      lua_pushnil(L);
      lua_rawset(L, -3);
      lua_pop(L, 1);
    }
  } else {
    /* Set in map. */
    upb_msgval val =
        lupb_tomsgval(L, upb_map_valuetype(map), 3, lmap->value_lmsgclass);

    upb_map_set(map, key, val, NULL);

    if (lupb_istypewrapped(upb_map_valuetype(map))) {
      /* Set in userval. */
      lupb_getuservalue(L, 1);
      lua_pushvalue(L, 2);
      lua_pushvalue(L, 3);
      lua_rawset(L, -3);
      lua_pop(L, 1);
    }
  }

  return 0;
}

/* upb_mapiter [[[ */

static int lupb_mapiter_next(lua_State *L) {
  upb_mapiter *i = lua_touserdata(L, lua_upvalueindex(1));
  lupb_map *lmap = lupb_map_check(L, 1);
  upb_map *map = lmap->map;

  if (upb_mapiter_done(i)) {
    return 0;
  }

  lupb_pushmsgval(L, upb_map_keytype(map), upb_mapiter_key(i));
  lupb_pushmsgval(L, upb_map_valuetype(map), upb_mapiter_value(i));
  upb_mapiter_next(i);

  return 2;
}

static int lupb_map_pairs(lua_State *L) {
  lupb_map *lmap = lupb_map_check(L, 1);

  if (lupb_istypewrapped(upb_map_keytype(lmap->map)) ||
      lupb_istypewrapped(upb_map_valuetype(lmap->map))) {
    /* Complex key or value type.
     * Sync upb_map to userval if necessary, then iterate over userval. */

    /* TODO: Lua tables don't know how many entries they have, gah!. */
    return 1;
  } else {
    /* Simple key and value type, iterate over the upb_map directly. */
    upb_mapiter *i = lua_newuserdata(L, upb_mapiter_sizeof());

    upb_mapiter_begin(i, lmap->map);
    lua_pushvalue(L, 1);

    /* Upvalues are [upb_mapiter, lupb_map]. */
    lua_pushcclosure(L, &lupb_mapiter_next, 2);

    return 1;
  }
}

/* upb_mapiter ]]] */

static const struct luaL_Reg lupb_map_mm[] = {
  {"__index", lupb_map_index},
  {"__len", lupb_map_len},
  {"__newindex", lupb_map_newindex},
  {"__pairs", lupb_map_pairs},
  {NULL, NULL}
};


/* lupb_msg *******************************************************************/

/* A message object.  Implemented by wrapping upb_msg.
 *
 * Our userval contains:
 *
 * - [0] -> our message class
 * - [lupb_fieldindex(f)] -> [lupb_{string,array,map,msg} userdata]
 *
 * Fields with scalar number/bool types don't go in the userval.
 */

#define LUPB_MSG_MSGCLASSINDEX 0
#define LUPB_MSG_ARENA -1

int lupb_fieldindex(const upb_fielddef *f) {
  return upb_fielddef_index(f) + 1;  /* 1-based Lua arrays. */
}


typedef struct {
  const lupb_msgclass *lmsgclass;
  upb_msg *msg;
} lupb_msg;

/* lupb_msg helpers */

static bool in_userval(const upb_fielddef *f) {
  return lupb_istypewrapped(upb_fielddef_type(f)) || upb_fielddef_isseq(f) ||
         upb_fielddef_ismap(f);
}

lupb_msg *lupb_msg_check(lua_State *L, int narg) {
  lupb_msg *msg = luaL_checkudata(L, narg, LUPB_MSG);
  if (!msg->lmsgclass) luaL_error(L, "called into dead msg");
  return msg;
}

const upb_msg *lupb_msg_checkmsg(lua_State *L, int narg,
                                 const lupb_msgclass *lmsgclass) {
  lupb_msg *lmsg = lupb_msg_check(L, narg);
  lupb_msgclass_typecheck(L, lmsgclass, lmsg->lmsgclass);
  return lmsg->msg;
}

upb_msg *lupb_msg_checkmsg2(lua_State *L, int narg,
                            const upb_msglayout **layout) {
  lupb_msg *lmsg = lupb_msg_check(L, narg);
  *layout = lmsg->lmsgclass->layout;
  return lmsg->msg;
}

const upb_msgdef *lupb_msg_checkdef(lua_State *L, int narg) {
  return lupb_msg_check(L, narg)->lmsgclass->msgdef;
}

static const upb_fielddef *lupb_msg_checkfield(lua_State *L,
                                               const lupb_msg *msg,
                                               int fieldarg) {
  size_t len;
  const char *fieldname = luaL_checklstring(L, fieldarg, &len);
  const upb_msgdef *msgdef = msg->lmsgclass->msgdef;
  const upb_fielddef *f = upb_msgdef_ntof(msgdef, fieldname, len);

  if (!f) {
    const char *msg = lua_pushfstring(L, "no such field: %s", fieldname);
    luaL_argerror(L, fieldarg, msg);
    return NULL;  /* Never reached. */
  }

  return f;
}

static const lupb_msgclass *lupb_msg_msgclassfor(lua_State *L, int narg,
                                                 const upb_msgdef *md) {
  lupb_uservalgeti(L, narg, LUPB_MSG_MSGCLASSINDEX);
  return lupb_msgclass_msgclassfor(L, -1, md);
}

static const lupb_msgclass *lupb_msg_getsubmsgclass(lua_State *L, int narg,
                                                    const upb_fielddef *f) {
  lupb_uservalgeti(L, narg, LUPB_MSG_MSGCLASSINDEX);
  return lupb_msgclass_getsubmsgclass(L, -1, f);
}

int lupb_msg_pushref(lua_State *L, int msgclass, upb_msg *msg) {
  const lupb_msgclass *lmsgclass = lupb_msgclass_check(L, msgclass);
  lupb_msg *lmsg = lupb_newuserdata(L, sizeof(lupb_msg), LUPB_MSG);

  lmsg->lmsgclass = lmsgclass;
  lmsg->msg = msg;

  lupb_uservalseti(L, -1, LUPB_MSG_MSGCLASSINDEX, msgclass);
  lupb_uservalseti(L, -1, LUPB_MSG_ARENA, -2);

  return 1;
}

/* lupb_msg Public API */

/**
 * lupb_msg_pushnew
 *
 * Handles:
 *   new_msg = MessageClass()
 */
static int lupb_msg_pushnew(lua_State *L, int narg) {
  const lupb_msgclass *lmsgclass = lupb_msgclass_check(L, narg);
  lupb_msg *lmsg = lupb_newuserdata(L, sizeof(lupb_msg), LUPB_MSG);

  lmsg->lmsgclass = lmsgclass;
  lmsg->msg = upb_msg_new(lmsgclass->layout, lupb_arena_get(L));

  lupb_uservalseti(L, -1, LUPB_MSG_MSGCLASSINDEX, narg);

  return 1;
}

/**
 * lupb_msg_index
 *
 * Handles:
 *   msg.foo
 *   msg["foo"]
 *   msg[field_descriptor]  # (for extensions) (TODO)
 */
static int lupb_msg_index(lua_State *L) {
  lupb_msg *lmsg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, lmsg, 2);
  const upb_msglayout *l = lmsg->lmsgclass->layout;
  int field_index = upb_fielddef_index(f);

  if (in_userval(f)) {
    lupb_uservalgeti(L, 1, lupb_fieldindex(f));

    if (lua_isnil(L, -1)) {
      /* Check if we need to lazily create wrapper. */
      if (upb_fielddef_isseq(f)) {
        /* TODO(haberman) */
      } else if (upb_fielddef_issubmsg(f)) {
        /* TODO(haberman) */
      } else {
        UPB_ASSERT(upb_fielddef_isstring(f));
        if (upb_msg_has(lmsg->msg, field_index, l)) {
          upb_msgval val = upb_msg_get(lmsg->msg, field_index, l);
          lua_pop(L, 1);
          lua_pushlstring(L, val.str.data, val.str.size);
          lupb_uservalseti(L, 1, lupb_fieldindex(f), -1);
        }
      }
    }
  } else {
    upb_msgval val = upb_msg_get(lmsg->msg, field_index, l);
    lupb_pushmsgval(L, upb_fielddef_type(f), val);
  }

  return 1;
}

/**
 * lupb_msg_newindex()
 *
 * Handles:
 *   msg.foo = bar
 *   msg["foo"] = bar
 *   msg[field_descriptor] = bar  # (for extensions) (TODO)
 */
static int lupb_msg_newindex(lua_State *L) {
  lupb_msg *lmsg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, lmsg, 2);
  upb_fieldtype_t type = upb_fielddef_type(f);
  int field_index = upb_fielddef_index(f);
  upb_msgval msgval;

  /* Typecheck and get msgval. */

  if (upb_fielddef_isseq(f)) {
    msgval = lupb_array_typecheck(L, 3, 1, f);
  } else if (upb_fielddef_ismap(f)) {
    msgval = lupb_map_typecheck(L, 3, 1, f);
  } else {
    const lupb_msgclass *lmsgclass = NULL;

    if (type == UPB_TYPE_MESSAGE) {
      lmsgclass = lupb_msg_getsubmsgclass(L, 1, f);
    }

    msgval = lupb_tomsgval(L, type, 3, lmsgclass);
  }

  /* Set in upb_msg and userval (if necessary). */

  upb_msg_set(lmsg->msg, field_index, msgval, lmsg->lmsgclass->layout);

  if (in_userval(f)) {
    lupb_uservalseti(L, 1, lupb_fieldindex(f), 3);
  }

  return 0;  /* 1 for chained assignments? */
}

static const struct luaL_Reg lupb_msg_mm[] = {
  {"__index", lupb_msg_index},
  {"__newindex", lupb_msg_newindex},
  {NULL, NULL}
};


/* lupb_msg toplevel **********************************************************/

static const struct luaL_Reg lupb_msg_toplevel_m[] = {
  {"Array", lupb_array_new},
  {"Map", lupb_map_new},
  {"MessageFactory", lupb_msgfactory_new},
  {NULL, NULL}
};

void lupb_msg_registertypes(lua_State *L) {
  lupb_setfuncs(L, lupb_msg_toplevel_m);

  lupb_register_type(L, LUPB_ARENA,      NULL,              lupb_arena_mm);
  lupb_register_type(L, LUPB_MSGCLASS,   NULL,              lupb_msgclass_mm);
  lupb_register_type(L, LUPB_MSGFACTORY, lupb_msgfactory_m, lupb_msgfactory_mm);
  lupb_register_type(L, LUPB_ARRAY,      NULL,              lupb_array_mm);
  lupb_register_type(L, LUPB_MAP,        NULL,              lupb_map_mm);
  lupb_register_type(L, LUPB_MSG,        NULL,              lupb_msg_mm);

  lupb_arena_initsingleton(L);
}
