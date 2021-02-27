/*
** lupb_msg -- Message/Array/Map objects in Lua/C that wrap upb/msg.h
*/

#include "upb/msg.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "upb/bindings/lua/upb.h"
#include "upb/json_decode.h"
#include "upb/json_encode.h"
#include "upb/port_def.inc"
#include "upb/reflection.h"
#include "upb/text_encode.h"

/*
 * Message/Map/Array objects.  These objects form a directed graph: a message
 * can contain submessages, arrays, and maps, which can then point to other
 * messages.  This graph can technically be cyclic, though this is an error and
 * a cyclic graph cannot be serialized.  So it's better to think of this as a
 * tree of objects.
 *
 * The actual data exists at the upb level (upb_msg, upb_map, upb_array),
 * independently of Lua.  The upb objects contain all the canonical data and
 * edges between objects.  Lua wrapper objects expose the upb objects to Lua,
 * but ultimately they are just wrappers.  They pass through all reads and
 * writes to the underlying upb objects.
 *
 * Each upb object lives in a upb arena.  We have a Lua object to wrap the upb
 * arena, but arenas are never exposed to the user.  The Lua arena object just
 * serves to own the upb arena and free it at the proper time, once the Lua GC
 * has determined that there are no more references to anything that lives in
 * that arena.  All wrapper objects strongly reference the arena to which they
 * belong.
 *
 * A global object cache stores a mapping of C pointer (upb_msg*, upb_array*,
 * upb_map*) to a corresponding Lua wrapper.  These references are weak so that
 * the wrappers can be collected if they are no longer needed.  A new wrapper
 * object can always be recreated later.
 *
 *                          +-----+
 *            lupb_arena    |cache|-weak-+
 *                 |  ^     +-----+      |
 *                 |  |                  V
 * Lua level       |  +------------lupb_msg
 * ----------------|-----------------|------------------------------------------
 * upb level       |                 |
 *                 |            +----V------------------------------+
 *                 +->upb_arena | upb_msg  ...(empty arena storage) |
 *                              +-----------------------------------+
 *
 * If the user creates a reference between two objects that have different
 * arenas, we need to fuse the two arenas together, so that the blocks will
 * outlive both arenas.
 *
 *                 +-------------------------->(fused)<----------------+
 *                 |                                                   |
 *                 V                           +-----+                 V
 *            lupb_arena                +-weak-|cache|-weak-+     lupb_arena
 *                 |  ^                 |      +-----+      |        ^   |
 *                 |  |                 V                   V        |   |
 * Lua level       |  +------------lupb_msg              lupb_msg----+   |
 * ----------------|-----------------|-------------------------|---------|------
 * upb level       |                 |                         |         |
 *                 |            +----V----+               +----V----+    V
 *                 +->upb_arena | upb_msg |               | upb_msg | upb_arena
 *                              +------|--+               +--^------+
 *                                     +---------------------+
 * Key invariants:
 *   1. every wrapper references the arena that contains it.
 *   2. every fused arena includes all arenas that own upb objects reachable
 *      from that arena.  In other words, when a wrapper references an arena,
 *      this is sufficient to ensure that any upb object reachable from that
 *      wrapper will stay alive.
 *
 * Additionally, every message object contains a strong reference to the
 * corresponding Descriptor object.  Likewise, array/map objects reference a
 * Descriptor object if they are typed to store message values.
 */

#define LUPB_ARENA "lupb.arena"
#define LUPB_ARRAY "lupb.array"
#define LUPB_MAP "lupb.map"
#define LUPB_MSG "lupb.msg"

#define LUPB_ARENA_INDEX 1
#define LUPB_MSGDEF_INDEX 2  /* For msg, and map/array that store msg */

static void lupb_msg_newmsgwrapper(lua_State *L, int narg, upb_msgval val);
static upb_msg *lupb_msg_check(lua_State *L, int narg);

static upb_fieldtype_t lupb_checkfieldtype(lua_State *L, int narg) {
  uint32_t n = lupb_checkuint32(L, narg);
  bool ok = n >= UPB_TYPE_BOOL && n <= UPB_TYPE_BYTES;
  luaL_argcheck(L, ok, narg, "invalid field type");
  return n;
}

char cache_key;

/* lupb_cacheinit()
 *
 * Creates the global cache used by lupb_cacheget() and lupb_cacheset().
 */
static void lupb_cacheinit(lua_State *L) {
  /* Create our object cache. */
  lua_newtable(L);

  /* Cache metatable gives the cache weak values */
  lua_createtable(L, 0, 1);
  lua_pushstring(L, "v");
  lua_setfield(L, -2, "__mode");
  lua_setmetatable(L, -2);

  /* Set cache in the registry. */
  lua_rawsetp(L, LUA_REGISTRYINDEX, &cache_key);
}

/* lupb_cacheget()
 *
 * Pushes cache[key] and returns true if this key is present in the cache.
 * Otherwise returns false and leaves nothing on the stack.
 */
static bool lupb_cacheget(lua_State *L, const void *key) {
  if (key == NULL) {
    lua_pushnil(L);
    return true;
  }

  lua_rawgetp(L, LUA_REGISTRYINDEX, &cache_key);
  lua_rawgetp(L, -1, key);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 2);  /* Pop table, nil. */
    return false;
  } else {
    lua_replace(L, -2);  /* Replace cache table. */
    return true;
  }
}

/* lupb_cacheset()
 *
 * Sets cache[key] = val, where "val" is the value at the top of the stack.
 * Does not pop the value.
 */
static void lupb_cacheset(lua_State *L, const void *key) {
  lua_rawgetp(L, LUA_REGISTRYINDEX, &cache_key);
  lua_pushvalue(L, -2);
  lua_rawsetp(L, -2, key);
  lua_pop(L, 1);  /* Pop table. */
}

/* lupb_arena *****************************************************************/

/* lupb_arena only exists to wrap a upb_arena.  It is never exposed to users; it
 * is an internal memory management detail.  Other wrapper objects refer to this
 * object from their userdata to keep the arena-owned data alive.
 */

typedef struct {
  upb_arena *arena;
} lupb_arena;

static upb_arena *lupb_arena_check(lua_State *L, int narg) {
  lupb_arena *a = luaL_checkudata(L, narg, LUPB_ARENA);
  return a->arena;
}

upb_arena *lupb_arena_pushnew(lua_State *L) {
  lupb_arena *a = lupb_newuserdata(L, sizeof(lupb_arena), 1, LUPB_ARENA);
  a->arena = upb_arena_new();
  return a->arena;
}

/**
 * lupb_arena_fuse()
 *
 * Merges |from| into |to| so that there is a single arena group that contains
 * both, and both arenas will point at this new table. */
static void lupb_arena_fuse(lua_State *L, int to, int from) {
  upb_arena *to_arena = lupb_arena_check(L, to);
  upb_arena *from_arena = lupb_arena_check(L, from);
  upb_arena_fuse(to_arena, from_arena);
}

static void lupb_arena_fuseobjs(lua_State *L, int to, int from) {
  lua_getiuservalue(L, to, LUPB_ARENA_INDEX);
  lua_getiuservalue(L, from, LUPB_ARENA_INDEX);
  lupb_arena_fuse(L, lua_absindex(L, -2), lua_absindex(L, -1));
  lua_pop(L, 2);
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

/* lupb_arenaget()
 *
 * Returns the arena from the given message, array, or map object.
 */
static upb_arena *lupb_arenaget(lua_State *L, int narg) {
  upb_arena *arena;
  lua_getiuservalue(L, narg, LUPB_ARENA_INDEX);
  arena = lupb_arena_check(L, -1);
  lua_pop(L, 1);
  return arena;
}

/* upb <-> Lua type conversion ************************************************/

/* Whether string data should be copied into the containing arena.  We can
 * avoid a copy if the string data is only needed temporarily (like for a map
 * lookup).
 */
typedef enum {
  LUPB_COPY,  /* Copy string data into the arena. */
  LUPB_REF    /* Reference the Lua copy of the string data. */
} lupb_copy_t;

/**
 * lupb_tomsgval()
 *
 * Converts the given Lua value |narg| to a upb_msgval.
 */
static upb_msgval lupb_tomsgval(lua_State *L, upb_fieldtype_t type, int narg,
                                int container, lupb_copy_t copy) {
  upb_msgval ret;
  switch (type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      ret.int32_val = lupb_checkint32(L, narg);
      break;
    case UPB_TYPE_INT64:
      ret.int64_val = lupb_checkint64(L, narg);
      break;
    case UPB_TYPE_UINT32:
      ret.uint32_val = lupb_checkuint32(L, narg);
      break;
    case UPB_TYPE_UINT64:
      ret.uint64_val = lupb_checkuint64(L, narg);
      break;
    case UPB_TYPE_DOUBLE:
      ret.double_val = lupb_checkdouble(L, narg);
      break;
    case UPB_TYPE_FLOAT:
      ret.float_val = lupb_checkfloat(L, narg);
      break;
    case UPB_TYPE_BOOL:
      ret.bool_val = lupb_checkbool(L, narg);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      size_t len;
      const char *ptr = lupb_checkstring(L, narg, &len);
      switch (copy) {
        case LUPB_COPY: {
          upb_arena *arena = lupb_arenaget(L, container);
          char *data = upb_arena_malloc(arena, len);
          memcpy(data, ptr, len);
          ret.str_val = upb_strview_make(data, len);
          break;
        }
        case LUPB_REF:
          ret.str_val = upb_strview_make(ptr, len);
          break;
      }
      break;
    }
    case UPB_TYPE_MESSAGE:
      ret.msg_val = lupb_msg_check(L, narg);
      /* Typecheck message. */
      lua_getiuservalue(L, container, LUPB_MSGDEF_INDEX);
      lua_getiuservalue(L, narg, LUPB_MSGDEF_INDEX);
      luaL_argcheck(L, lua_rawequal(L, -1, -2), narg, "message type mismatch");
      lua_pop(L, 2);
      break;
  }
  return ret;
}

static void lupb_pushmsgval(lua_State *L, int container, upb_fieldtype_t type,
                            upb_msgval val) {
  switch (type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      lupb_pushint32(L, val.int32_val);
      return;
    case UPB_TYPE_INT64:
      lupb_pushint64(L, val.int64_val);
      return;
    case UPB_TYPE_UINT32:
      lupb_pushuint32(L, val.uint32_val);
      return;
    case UPB_TYPE_UINT64:
      lupb_pushuint64(L, val.uint64_val);
      return;
    case UPB_TYPE_DOUBLE:
      lua_pushnumber(L, val.double_val);
      return;
    case UPB_TYPE_FLOAT:
      lua_pushnumber(L, val.float_val);
      return;
    case UPB_TYPE_BOOL:
      lua_pushboolean(L, val.bool_val);
      return;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      lua_pushlstring(L, val.str_val.data, val.str_val.size);
      return;
    case UPB_TYPE_MESSAGE:
      assert(container);
      if (!lupb_cacheget(L, val.msg_val)) {
        lupb_msg_newmsgwrapper(L, container, val);
      }
      return;
  }
  LUPB_UNREACHABLE();
}


/* lupb_array *****************************************************************/

typedef struct {
  upb_array *arr;
  upb_fieldtype_t type;
} lupb_array;

static lupb_array *lupb_array_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_ARRAY);
}

/**
 * lupb_array_checkindex()
 *
 * Checks the array index at Lua stack index |narg| to verify that it is an
 * integer between 1 and |max|, inclusively.  Also corrects it to be zero-based
 * for C.
 */
static int lupb_array_checkindex(lua_State *L, int narg, uint32_t max) {
  uint32_t n = lupb_checkuint32(L, narg);
  luaL_argcheck(L, n != 0 && n <= max, narg, "invalid array index");
  return n - 1;  /* Lua uses 1-based indexing. */
}

/* lupb_array Public API */

/* lupb_array_new():
 *
 * Handles:
 *   Array(upb.TYPE_INT32)
 *   Array(message_type)
 */
static int lupb_array_new(lua_State *L) {
  lupb_array *larray;
  upb_arena *arena;

  if (lua_type(L, 1) == LUA_TNUMBER) {
    upb_fieldtype_t type = lupb_checkfieldtype(L, 1);
    larray = lupb_newuserdata(L, sizeof(*larray), 1, LUPB_ARRAY);
    larray->type = type;
  } else {
    lupb_msgdef_check(L, 1);
    larray = lupb_newuserdata(L, sizeof(*larray), 2, LUPB_ARRAY);
    larray->type = UPB_TYPE_MESSAGE;
    lua_pushvalue(L, 1);
    lua_setiuservalue(L, -2, LUPB_MSGDEF_INDEX);
  }

  arena = lupb_arena_pushnew(L);
  lua_setiuservalue(L, -2, LUPB_ARENA_INDEX);

  larray->arr = upb_array_new(arena, larray->type);
  lupb_cacheset(L, larray->arr);

  return 1;
}

/* lupb_array_newindex():
 *
 * Handles:
 *   array[idx] = val
 *
 * idx can be within the array or one past the end to extend.
 */
static int lupb_array_newindex(lua_State *L) {
  lupb_array *larray = lupb_array_check(L, 1);
  size_t size = upb_array_size(larray->arr);
  uint32_t n = lupb_array_checkindex(L, 2, size + 1);
  upb_msgval msgval = lupb_tomsgval(L, larray->type, 3, 1, LUPB_COPY);

  if (n == size) {
    upb_array_append(larray->arr, msgval, lupb_arenaget(L, 1));
  } else {
    upb_array_set(larray->arr, n, msgval);
  }

  if (larray->type == UPB_TYPE_MESSAGE) {
    lupb_arena_fuseobjs(L, 1, 3);
  }

  return 0;  /* 1 for chained assignments? */
}

/* lupb_array_index():
 *
 * Handles:
 *   array[idx] -> val
 *
 * idx must be within the array.
 */
static int lupb_array_index(lua_State *L) {
  lupb_array *larray = lupb_array_check(L, 1);
  size_t size = upb_array_size(larray->arr);
  uint32_t n = lupb_array_checkindex(L, 2, size);
  upb_msgval val = upb_array_get(larray->arr, n);

  lupb_pushmsgval(L, 1, larray->type, val);

  return 1;
}

/* lupb_array_len():
 *
 * Handles:
 *   #array -> len
 */
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

typedef struct {
  upb_map *map;
  upb_fieldtype_t key_type;
  upb_fieldtype_t value_type;
} lupb_map;

#define MAP_MSGDEF_INDEX 1

static lupb_map *lupb_map_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_MAP);
}

/* lupb_map Public API */

/**
 * lupb_map_new
 *
 * Handles:
 *   new_map = upb.Map(key_type, value_type)
 *   new_map = upb.Map(key_type, value_msgdef)
 */
static int lupb_map_new(lua_State *L) {
  upb_arena *arena;
  lupb_map *lmap;

  if (lua_type(L, 2) == LUA_TNUMBER) {
    lmap = lupb_newuserdata(L, sizeof(*lmap), 1, LUPB_MAP);
    lmap->value_type = lupb_checkfieldtype(L, 2);
  } else {
    lupb_msgdef_check(L, 2);
    lmap = lupb_newuserdata(L, sizeof(*lmap), 2, LUPB_MAP);
    lmap->value_type = UPB_TYPE_MESSAGE;
    lua_pushvalue(L, 2);
    lua_setiuservalue(L, -2, MAP_MSGDEF_INDEX);
  }

  arena = lupb_arena_pushnew(L);
  lua_setiuservalue(L, -2, LUPB_ARENA_INDEX);

  lmap->key_type = lupb_checkfieldtype(L, 1);
  lmap->map = upb_map_new(arena, lmap->key_type, lmap->value_type);
  lupb_cacheset(L, lmap->map);

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
  upb_msgval key = lupb_tomsgval(L, lmap->key_type, 2, 1, LUPB_REF);
  upb_msgval val;

  if (upb_map_get(lmap->map, key, &val)) {
    lupb_pushmsgval(L, 1, lmap->value_type, val);
  } else {
    lua_pushnil(L);
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
  upb_msgval key = lupb_tomsgval(L, lmap->key_type, 2, 1, LUPB_REF);

  if (lua_isnil(L, 3)) {
    upb_map_delete(map, key);
  } else {
    upb_msgval val = lupb_tomsgval(L, lmap->value_type, 3, 1, LUPB_COPY);
    upb_map_set(map, key, val, lupb_arenaget(L, 1));
    if (lmap->value_type == UPB_TYPE_MESSAGE) {
      lupb_arena_fuseobjs(L, 1, 3);
    }
  }

  return 0;
}

static int lupb_mapiter_next(lua_State *L) {
  int map = lua_upvalueindex(2);
  size_t *iter = lua_touserdata(L, lua_upvalueindex(1));
  lupb_map *lmap = lupb_map_check(L, map);

  if (upb_mapiter_next(lmap->map, iter)) {
    upb_msgval key = upb_mapiter_key(lmap->map, *iter);
    upb_msgval val = upb_mapiter_value(lmap->map, *iter);
    lupb_pushmsgval(L, map, lmap->key_type, key);
    lupb_pushmsgval(L, map, lmap->value_type, val);
    return 2;
  } else {
    return 0;
  }

}

/**
 * lupb_map_pairs()
 *
 * Handles:
 *   pairs(map)
 */
static int lupb_map_pairs(lua_State *L) {
  size_t *iter = lua_newuserdata(L, sizeof(*iter));
  lupb_map_check(L, 1);

  *iter = UPB_MAP_BEGIN;
  lua_pushvalue(L, 1);

  /* Upvalues are [iter, lupb_map]. */
  lua_pushcclosure(L, &lupb_mapiter_next, 2);

  return 1;
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

typedef struct {
  upb_msg *msg;
} lupb_msg;

/* lupb_msg helpers */

static upb_msg *lupb_msg_check(lua_State *L, int narg) {
  lupb_msg *msg = luaL_checkudata(L, narg, LUPB_MSG);
  return msg->msg;
}

static const upb_msgdef *lupb_msg_getmsgdef(lua_State *L, int msg) {
  lua_getiuservalue(L, msg, LUPB_MSGDEF_INDEX);
  const upb_msgdef *m = lupb_msgdef_check(L, -1);
  lua_pop(L, 1);
  return m;
}

static const upb_fielddef *lupb_msg_tofield(lua_State *L, int msg, int field) {
  size_t len;
  const char *fieldname = luaL_checklstring(L, field, &len);
  const upb_msgdef *m = lupb_msg_getmsgdef(L, msg);
  return upb_msgdef_ntof(m, fieldname, len);
}

static const upb_fielddef *lupb_msg_checkfield(lua_State *L, int msg,
                                               int field) {
  const upb_fielddef *f = lupb_msg_tofield(L, msg, field);
  if (f == NULL) {
    luaL_error(L, "no such field '%s'", lua_tostring(L, field));
  }
  return f;
}

upb_msg *lupb_msg_pushnew(lua_State *L, int narg) {
  const upb_msgdef *m = lupb_msgdef_check(L, narg);
  lupb_msg *lmsg = lupb_newuserdata(L, sizeof(lupb_msg), 2, LUPB_MSG);
  upb_arena *arena = lupb_arena_pushnew(L);

  lua_setiuservalue(L, -2, LUPB_ARENA_INDEX);
  lua_pushvalue(L, 1);
  lua_setiuservalue(L, -2, LUPB_MSGDEF_INDEX);

  lmsg->msg = upb_msg_new(m, arena);
  lupb_cacheset(L, lmsg->msg);
  return lmsg->msg;
}

/**
 * lupb_msg_newmsgwrapper()
 *
 * Creates a new wrapper for a message, copying the arena and msgdef references
 * from |narg| (which should be an array or map).
 */
static void lupb_msg_newmsgwrapper(lua_State *L, int narg, upb_msgval val) {
  lupb_msg *lmsg = lupb_newuserdata(L, sizeof(*lmsg), 2, LUPB_MSG);
  lmsg->msg = (upb_msg*)val.msg_val;  /* XXX: cast isn't great. */
  lupb_cacheset(L, lmsg->msg);

  /* Copy both arena and msgdef into the wrapper. */
  lua_getiuservalue(L, narg, LUPB_ARENA_INDEX);
  lua_setiuservalue(L, -2, LUPB_ARENA_INDEX);
  lua_getiuservalue(L, narg, LUPB_MSGDEF_INDEX);
  lua_setiuservalue(L, -2, LUPB_MSGDEF_INDEX);
}

/**
 * lupb_msg_newud()
 *
 * Creates the Lua userdata for a new wrapper object, adding a reference to
 * the msgdef if necessary.
 */
static void *lupb_msg_newud(lua_State *L, int narg, size_t size,
                            const char *type, const upb_fielddef *f) {
  if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE) {
    /* Wrapper needs a reference to the msgdef. */
    void* ud = lupb_newuserdata(L, size, 2, type);
    lua_getiuservalue(L, narg, LUPB_MSGDEF_INDEX);
    lupb_msgdef_pushsubmsgdef(L, f);
    lua_setiuservalue(L, -2, LUPB_MSGDEF_INDEX);
    return ud;
  } else {
    return lupb_newuserdata(L, size, 1, type);
  }
}

/**
 * lupb_msg_newwrapper()
 *
 * Creates a new Lua wrapper object to wrap the given array, map, or message.
 */
static void lupb_msg_newwrapper(lua_State *L, int narg, const upb_fielddef *f,
                                upb_mutmsgval val) {
  if (upb_fielddef_ismap(f)) {
    const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
    const upb_fielddef *key_f = upb_msgdef_itof(entry, UPB_MAPENTRY_KEY);
    const upb_fielddef *val_f = upb_msgdef_itof(entry, UPB_MAPENTRY_VALUE);
    lupb_map *lmap = lupb_msg_newud(L, narg, sizeof(*lmap), LUPB_MAP, val_f);
    lmap->key_type = upb_fielddef_type(key_f);
    lmap->value_type = upb_fielddef_type(val_f);
    lmap->map = val.map;
  } else if (upb_fielddef_isseq(f)) {
    lupb_array *larr = lupb_msg_newud(L, narg, sizeof(*larr), LUPB_ARRAY, f);
    larr->type = upb_fielddef_type(f);
    larr->arr = val.array;
  } else {
    lupb_msg *lmsg = lupb_msg_newud(L, narg, sizeof(*lmsg), LUPB_MSG, f);
    lmsg->msg = val.msg;
  }

  /* Copy arena ref to new wrapper.  This may be a different arena than the
   * underlying data was originally constructed from, but if so both arenas
   * must be in the same group. */
  lua_getiuservalue(L, narg, LUPB_ARENA_INDEX);
  lua_setiuservalue(L, -2, LUPB_ARENA_INDEX);

  lupb_cacheset(L, val.msg);
}

/**
 * lupb_msg_typechecksubmsg()
 *
 * Typechecks the given array, map, or msg against this upb_fielddef.
 */
static void lupb_msg_typechecksubmsg(lua_State *L, int narg, int msgarg,
                                     const upb_fielddef *f) {
  /* Typecheck this map's msgdef against this message field. */
  lua_getiuservalue(L, narg, LUPB_MSGDEF_INDEX);
  lua_getiuservalue(L, msgarg, LUPB_MSGDEF_INDEX);
  lupb_msgdef_pushsubmsgdef(L, f);
  luaL_argcheck(L, lua_rawequal(L, -1, -2), narg, "message type mismatch");
  lua_pop(L, 2);
}

/* lupb_msg Public API */

/**
 * lupb_msgdef_call
 *
 * Handles:
 *   new_msg = MessageClass()
 *   new_msg = MessageClass{foo = "bar", baz = 3, quux = {foo = 3}}
 */
int lupb_msgdef_call(lua_State *L) {
  int arg_count = lua_gettop(L);
  lupb_msg_pushnew(L, 1);

  if (arg_count > 1) {
    /* Set initial fields from table. */
    int msg = arg_count + 1;
    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
      lua_pushvalue(L, -2);  /* now stack is key, val, key */
      lua_insert(L, -3);  /* now stack is key, key, val */
      lua_settable(L, msg);
    }
  }

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
  upb_msg *msg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, 1, 2);

  if (upb_fielddef_isseq(f) || upb_fielddef_issubmsg(f)) {
    /* Wrapped type; get or create wrapper. */
    upb_arena *arena = upb_fielddef_isseq(f) ? lupb_arenaget(L, 1) : NULL;
    upb_mutmsgval val = upb_msg_mutable(msg, f, arena);
    if (!lupb_cacheget(L, val.msg)) {
      lupb_msg_newwrapper(L, 1, f, val);
    }
  } else {
    /* Value type, just push value and return .*/
    upb_msgval val = upb_msg_get(msg, f);
    lupb_pushmsgval(L, 0, upb_fielddef_type(f), val);
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
  upb_msg *msg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, 1, 2);
  upb_msgval msgval;
  bool merge_arenas = true;

  if (upb_fielddef_ismap(f)) {
    lupb_map *lmap = lupb_map_check(L, 3);
    const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
    const upb_fielddef *key_f = upb_msgdef_itof(entry, UPB_MAPENTRY_KEY);
    const upb_fielddef *val_f = upb_msgdef_itof(entry, UPB_MAPENTRY_VALUE);
    upb_fieldtype_t key_type = upb_fielddef_type(key_f);
    upb_fieldtype_t value_type = upb_fielddef_type(val_f);
    luaL_argcheck(L, lmap->key_type == key_type, 3, "key type mismatch");
    luaL_argcheck(L, lmap->value_type == value_type, 3, "value type mismatch");
    if (value_type == UPB_TYPE_MESSAGE) {
      lupb_msg_typechecksubmsg(L, 3, 1, val_f);
    }
    msgval.map_val = lmap->map;
  } else if (upb_fielddef_isseq(f)) {
    lupb_array *larr = lupb_array_check(L, 3);
    upb_fieldtype_t type = upb_fielddef_type(f);
    luaL_argcheck(L, larr->type == type, 3, "array type mismatch");
    if (type == UPB_TYPE_MESSAGE) {
      lupb_msg_typechecksubmsg(L, 3, 1, f);
    }
    msgval.array_val = larr->arr;
  } else if (upb_fielddef_issubmsg(f)) {
    upb_msg *msg = lupb_msg_check(L, 3);
    lupb_msg_typechecksubmsg(L, 3, 1, f);
    msgval.msg_val = msg;
  } else {
    msgval = lupb_tomsgval(L, upb_fielddef_type(f), 3, 1, LUPB_COPY);
    merge_arenas = false;
  }

  if (merge_arenas) {
    lupb_arena_fuseobjs(L, 1, 3);
  }

  upb_msg_set(msg, f, msgval, lupb_arenaget(L, 1));

  /* Return the new value for chained assignments. */
  lua_pushvalue(L, 3);
  return 1;
}

/**
 * lupb_msg_tostring()
 *
 * Handles:
 *   tostring(msg)
 *   print(msg)
 *   etc.
 */
static int lupb_msg_tostring(lua_State *L) {
  upb_msg *msg = lupb_msg_check(L, 1);
  const upb_msgdef *m;
  char buf[1024];
  size_t size;

  lua_getiuservalue(L, 1, LUPB_MSGDEF_INDEX);
  m = lupb_msgdef_check(L, -1);

  size = upb_text_encode(msg, m, NULL, 0, buf, sizeof(buf));

  if (size < sizeof(buf)) {
    lua_pushlstring(L, buf, size);
  } else {
    char *ptr = malloc(size + 1);
    upb_text_encode(msg, m, NULL, 0, ptr, size + 1);
    lua_pushlstring(L, ptr, size);
    free(ptr);
  }

  return 1;
}

static const struct luaL_Reg lupb_msg_mm[] = {
  {"__index", lupb_msg_index},
  {"__newindex", lupb_msg_newindex},
  {"__tostring", lupb_msg_tostring},
  {NULL, NULL}
};


/* lupb_msg toplevel **********************************************************/

static int lupb_getoptions(lua_State *L, int narg) {
  int options = 0;
  if (lua_gettop(L) >= narg) {
    size_t len = lua_rawlen(L, narg);
    for (size_t i = 1; i <= len; i++) {
      lua_rawgeti(L, narg, i);
      options |= lupb_checkuint32(L, -1);
      lua_pop(L, 1);
    }
  }
  return options;
}

/**
 * lupb_decode()
 *
 * Handles:
 *   msg = upb.decode(MessageClass, bin_string)
 */
static int lupb_decode(lua_State *L) {
  size_t len;
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  const char *pb = lua_tolstring(L, 2, &len);
  const upb_msglayout *layout = upb_msgdef_layout(m);
  upb_msg *msg = lupb_msg_pushnew(L, 1);
  upb_arena *arena = lupb_arenaget(L, -1);
  char *buf;
  bool ok;

  /* Copy input data to arena, message will reference it. */
  buf = upb_arena_malloc(arena, len);
  memcpy(buf, pb, len);

  ok = _upb_decode(buf, len, msg, layout, arena, UPB_DECODE_ALIAS);

  if (!ok) {
    lua_pushstring(L, "Error decoding protobuf.");
    return lua_error(L);
  }

  return 1;
}

/**
 * lupb_encode()
 *
 * Handles:
 *   bin_string = upb.encode(msg)
 */
static int lupb_encode(lua_State *L) {
  const upb_msg *msg = lupb_msg_check(L, 1);
  const upb_msgdef *m = lupb_msg_getmsgdef(L, 1);
  const upb_msglayout *layout = upb_msgdef_layout(m);
  int options = lupb_getoptions(L, 2);
  upb_arena *arena;
  size_t size;
  char *result;

  arena = lupb_arena_pushnew(L);
  result = upb_encode_ex(msg, (const void*)layout, options, arena, &size);

  if (!result) {
    lua_pushstring(L, "Error encoding protobuf.");
    return lua_error(L);
  }

  lua_pushlstring(L, result, size);

  return 1;
}

/**
 * lupb_jsondecode()
 *
 * Handles:
 *   text_string = upb.json_decode(MessageClass, json_str, {upb.JSONDEC_IGNOREUNKNOWN})
 */
static int lupb_jsondecode(lua_State *L) {
  size_t len;
  const upb_msgdef *m = lupb_msgdef_check(L, 1);
  const char *json = lua_tolstring(L, 2, &len);
  int options = lupb_getoptions(L, 3);
  upb_msg *msg;
  upb_arena *arena;
  upb_status status;

  msg = lupb_msg_pushnew(L, 1);
  arena = lupb_arenaget(L, -1);
  upb_status_clear(&status);
  upb_json_decode(json, len, msg, m, NULL, options, arena, &status);
  lupb_checkstatus(L, &status);

  return 1;
}

/**
 * lupb_jsonencode()
 *
 * Handles:
 *   text_string = upb.json_encode(msg, {upb.JSONENC_EMITDEFAULTS})
 */
static int lupb_jsonencode(lua_State *L) {
  upb_msg *msg = lupb_msg_check(L, 1);
  const upb_msgdef *m = lupb_msg_getmsgdef(L, 1);
  int options = lupb_getoptions(L, 2);
  char buf[1024];
  size_t size;
  upb_status status;

  upb_status_clear(&status);
  size = upb_json_encode(msg, m, NULL, options, buf, sizeof(buf), &status);
  lupb_checkstatus(L, &status);

  if (size < sizeof(buf)) {
    lua_pushlstring(L, buf, size);
  } else {
    char *ptr = malloc(size + 1);
    upb_json_encode(msg, m, NULL, options, ptr, size + 1, &status);
    lupb_checkstatus(L, &status);
    lua_pushlstring(L, ptr, size);
    free(ptr);
  }

  return 1;
}

/**
 * lupb_textencode()
 *
 * Handles:
 *   text_string = upb.text_encode(msg, {upb.TXTENC_SINGLELINE})
 */
static int lupb_textencode(lua_State *L) {
  upb_msg *msg = lupb_msg_check(L, 1);
  const upb_msgdef *m = lupb_msg_getmsgdef(L, 1);
  int options = lupb_getoptions(L, 2);
  char buf[1024];
  size_t size;

  size = upb_text_encode(msg, m, NULL, options, buf, sizeof(buf));

  if (size < sizeof(buf)) {
    lua_pushlstring(L, buf, size);
  } else {
    char *ptr = malloc(size + 1);
    upb_text_encode(msg, m, NULL, options, ptr, size + 1);
    lua_pushlstring(L, ptr, size);
    free(ptr);
  }

  return 1;
}

static void lupb_setfieldi(lua_State *L, const char *field, int i) {
  lua_pushinteger(L, i);
  lua_setfield(L, -2, field);
}

static const struct luaL_Reg lupb_msg_toplevel_m[] = {
  {"Array", lupb_array_new},
  {"Map", lupb_map_new},
  {"decode", lupb_decode},
  {"encode", lupb_encode},
  {"json_decode", lupb_jsondecode},
  {"json_encode", lupb_jsonencode},
  {"text_encode", lupb_textencode},
  {NULL, NULL}
};

void lupb_msg_registertypes(lua_State *L) {
  lupb_setfuncs(L, lupb_msg_toplevel_m);

  lupb_register_type(L, LUPB_ARENA, NULL, lupb_arena_mm);
  lupb_register_type(L, LUPB_ARRAY, NULL, lupb_array_mm);
  lupb_register_type(L, LUPB_MAP,   NULL, lupb_map_mm);
  lupb_register_type(L, LUPB_MSG,   NULL, lupb_msg_mm);

  lupb_setfieldi(L, "TXTENC_SINGLELINE", UPB_TXTENC_SINGLELINE);
  lupb_setfieldi(L, "TXTENC_SKIPUNKNOWN", UPB_TXTENC_SKIPUNKNOWN);
  lupb_setfieldi(L, "TXTENC_NOSORT", UPB_TXTENC_NOSORT);

  lupb_setfieldi(L, "ENCODE_DETERMINISTIC", UPB_ENCODE_DETERMINISTIC);
  lupb_setfieldi(L, "ENCODE_SKIPUNKNOWN", UPB_ENCODE_SKIPUNKNOWN);

  lupb_setfieldi(L, "JSONENC_EMITDEFAULTS", UPB_JSONENC_EMITDEFAULTS);
  lupb_setfieldi(L, "JSONENC_PROTONAMES", UPB_JSONENC_PROTONAMES);

  lupb_setfieldi(L, "JSONDEC_IGNOREUNKNOWN", UPB_JSONDEC_IGNOREUNKNOWN);

  lupb_cacheinit(L);
}
