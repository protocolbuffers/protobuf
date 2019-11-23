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
#include "upb/reflection.h"

#include "upb/port_def.inc"

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
 *                    arena
 *                 +->group
 *                 |
 *                 V        +-----+
 *            lupb_arena    |cache|-weak-+
 *                 |  ^     +-----+      |
 *                 |  |                  V
 * Lua level       |  +------------lupb_msg
 * ----------------|-----------------|-------------------------------------------
 * upb level       |                 |
 *                 |            +----V------------------------------+
 *                 +->upb_arena | upb_msg  ...(empty arena storage) |
 *                              +-----------------------------------+
 *
 * If the user creates a reference between two objects that have different
 * arenas, we need to merge the arenas into a single, bigger arena group.  The
 * arena group will reference both arenas, and will inherit the longest lifetime
 * of anything in the arena.
 *
 *                                              arena
 *                 +--------------------------->group<-----------------+
 *                 |                                                   |
 *                 V                           +-----+                 V
 *            lupb_arena                +-weak-|cache|-weak-+     lupb_arena
 *                 |  ^                 |      +-----+      |        ^   |
 *                 |  |                 V                   V        |   |
 * Lua level       |  +------------lupb_msg              lupb_msg----+   |
 * ----------------|-----------------|-------------------------|---------|-------
 * upb level       |                 |                         |         |
 *                 |            +----V----+               +----V----+    V
 *                 +->upb_arena | upb_msg |               | upb_msg | upb_arena
 *                              +------|--+               +--^------+
 *                                     +---------------------+
 * Key invariants:
 *   1. every wrapper references the arena that contains it.
 *   2. every arena group references all arenas that own upb objects reachable
 *      from that arena.  In other words, when a wrapper references an arena,
 *      this is sufficient to ensure that any upb object reachable from that
 *      wrapper will stay alive.
 *
 * Additionally, every message object contains a strong reference to the
 * corresponding Descriptor object.  Likewise, array/map objects reference a
 * Descriptor object if they are typed to store message values.
 *
 * (The object cache could be per-arena-group.  This would keep individual cache
 * tables smaller, and when an arena group is freed the entire cache table could
 * be collected in one fell swoop.  However this makes merging another arena
 * into the group an O(n) operation, since all entries would need to be copied
 * from the existing cache table.)
 */

#define LUPB_ARENA "lupb.arena"
#define LUPB_ARRAY "lupb.array"
#define LUPB_MAP "lupb.map"
#define LUPB_MSG "lupb.msg"

static int lupb_msg_pushnew(lua_State *L, int narg);

static void lupb_msg_typecheck(lua_State *L, const upb_msgdef *expected,
                                  const upb_msgdef *actual) {
  if (expected != actual) {
    luaL_error(L, "Message had incorrect type, expected '%s', got '%s'",
               upb_msgdef_fullname(expected), upb_msgdef_fullname(actual));
  }
}


/* lupb_arena *****************************************************************/

/* lupb_arena only exists to wrap a upb_arena.  It is never exposed to users; it
 * is an internal memory management detail.  Other wrapper objects refer to this
 * object from their userdata to keep the arena-owned data alive.
 *
 * The arena contains a table that caches wrapper objects. */

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
  assert(arena);
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


/* upb <-> Lua type conversion ************************************************/

static bool lupb_istypewrapped(upb_fieldtype_t type) {
  return type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES ||
         type == UPB_TYPE_MESSAGE;
}

static upb_msgval lupb_tomsgval(lua_State *L, upb_fieldtype_t type, int narg,
                                const upb_msgdef *msgdef) {
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
      ret.str_val = upb_strview_make(ptr, len);
      break;
    }
    case UPB_TYPE_MESSAGE:
      assert(msgdef);
      ret.msg_val = lupb_msg_checkmsg(L, narg, msgdef);
      break;
  }
  return ret;
}

static void lupb_pushmsgval(lua_State *L, upb_fieldtype_t type,
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
      lupb_pushdouble(L, val.double_val);
      return;
    case UPB_TYPE_FLOAT:
      lupb_pushfloat(L, val.float_val);
      return;
    case UPB_TYPE_BOOL:
      lua_pushboolean(L, val.bool_val);
      return;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      break;  /* Shouldn't call this function. */
  }
  LUPB_UNREACHABLE();
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
 *
 * Additionally, for message-typed arrays we keep:
 *
 *   [ARRAY_MSGDEF_INDEX] -> [SymbolTable] (to keep msgdef GC-reachable)
 */

typedef struct {
  upb_array *arr;
  upb_fieldtype_t type;
} lupb_array;

#define ARRAY_MSGDEF_INDEX 1

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
  upb_msgval val;

  if (larray->type != upb_fielddef_type(f)) {
    luaL_error(L, "Array had incorrect type (expected: %d, got: %d)",
               (int)upb_fielddef_type(f), (int)larray->type);
  }

  if (larray->type == UPB_TYPE_MESSAGE) {
    lupb_msgdef_typecheck(L, upb_fielddef_msgsubdef(f), larray->msgdef);
  }

  val.array_val = larray->arr;
  return val;
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

static void lupb_array_push(lua_State *L, upb_array *a, const upb_fielddef *f) {

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
  upb_fieldtype_t type;
  int n = 1;

  lupb_arena_new();  /* Userval 1. */

  if (lua_type(L, 1) == LUA_TNUMBER) {
    type = lupb_checkfieldtype(L, 1);
  } else {
    lupb_msgdef_check(L, 1);
    type = UPB_TYPE_MESSAGE;
    n = 2;
    lua_push(L, 1);  /* Userval 2. */
  }

  larray = lupb_newuserdata(L, sizeof(*larray), n, LUPB_ARRAY);
  larray->type = type;
  larray->arr = upb_array_new(lupb_arena_get(L), type);

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
  upb_fieldtype_t type = larray->type;
  uint32_t n = lupb_array_checkindex(L, 2, size + 1);
  upb_msgval msgval = lupb_tomsgval(L, type, 3, larray->msgdef);

  if (n == size) {
    upb_array_append(larray->arr, msgval, lupb_arena_get(L));
  } else {
    upb_array_set(larray->arr, n, msgval);
  }

  if (lupb_istypewrapped(type)) {
    lupb_uservalseti(L, 1, n, 3);
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
  upb_fieldtype_t type = larray->type;

  if (lupb_istypewrapped(type)) {
    lupb_uservalgeti(L, 1, n);
  } else {
    upb_msgval val = upb_array_get(larray->arr, n);
    lupb_pushmsgval(L, type, val);
  }

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

/* A map object.  Implemented by wrapping upb_map.
 *
 * When the value type is string/bytes/message, the userval consists of:
 *
 *   [Lua number/string] -> [lupb_string/lupb_msg userdata]
 *
 * Additionally when the map is message, we store:
 *
 *   [MAP_MSGDEF_INDEX] -> MessageDef
 *
 * For other value types we don't use the userdata.
 */

typedef struct {
  upb_map *map;
  upb_fieldtype_t key_type;
  upb_fieldtype_t value_type;
} lupb_map;

#define MAP_MSGDEF_INDEX 1

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
  const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
  const upb_fielddef *key_field = upb_msgdef_itof(entry, UPB_MAPENTRY_KEY);
  const upb_fielddef *value_field = upb_msgdef_itof(entry, UPB_MAPENTRY_VALUE);
  upb_msgval val;

  assert(entry && key_field && value_field);

  if (lmap->key_type != upb_fielddef_type(key_field)) {
    luaL_error(L, "Map had incorrect field type (expected: %s, got: %s)",
               upb_fielddef_type(key_field), lmap->key_type);
  }

  if (lmap->value_type != upb_fielddef_type(value_field)) {
    luaL_error(L, "Map had incorrect value type (expected: %s, got: %s)",
               upb_fielddef_type(value_field), lmap->value_type);
  }

  if (lmap->value_type == UPB_TYPE_MESSAGE) {
    lupb_uservalgeti(L, narg, MAP_MSGDEF_INDEX);
    lupb_wrapper_pushwrapper(L

    lupb_msgclass_typecheck(
        L, lupb_msg_msgclassfor(L, msg, upb_fielddef_msgsubdef(value_field)),
        lmap->value_lmsgclass);
  }

  val.map_val = lmap->map;
  return val;
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
  lupb_map *lmap;
  upb_fieldtype_t key_type = lupb_checkfieldtype(L, 1);
  upb_fieldtype_t value_type;

  if (lua_type(L, 2) == LUA_TNUMBER) {
    value_type = lupb_checkfieldtype(L, 2);
  } else {
    value_type = UPB_TYPE_MESSAGE;
  }

  lmap = lupb_newuserdata(L, sizeof(*lmap), LUPB_MAP);

  if (value_type == UPB_TYPE_MESSAGE) {
    value_lmsgclass = lupb_msgclass_check(L, 2);
    lupb_uservalseti(L, -1, MAP_MSGDEF_INDEX, 2);  /* GC-root lmsgclass. */
  }

  lmap->key_type = key_type;
  lmap->value_type = value_type;
  lmap->map = upb_map_new(lupb_arena_get(L), key_type, value_type);

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
  upb_fieldtype_t valtype = lmap->value_type;
  /* We don't always use "key", but this call checks the key type. */
  upb_msgval key = lupb_tomsgval(L, lmap->key_type, 2, NULL);

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
      lupb_pushmsgval(L, lmap->value_type, val);
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
  upb_msgval key = lupb_tomsgval(L, lmap->key_type, 2, NULL);

  if (lua_isnil(L, 3)) {
    /* Delete from map. */
    upb_map_delete(map, key);

    if (lupb_istypewrapped(lmap->value_type)) {
      /* Delete in userval. */
      lupb_getuservalue(L, 1);
      lua_pushvalue(L, 2);
      lua_pushnil(L);
      lua_rawset(L, -3);
      lua_pop(L, 1);
    }
  } else {
    /* Set in map. */
    upb_msgval val = lupb_tomsgval(L, lmap->value_type, 3, lmap->msgdef);

    upb_map_set(map, key, val, lupb_arena_get(L));

    if (lupb_istypewrapped(lmap->value_type)) {
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

  if (upb_mapiter_done(i)) {
    return 0;
  }

  lupb_pushmsgval(L, lmap->key_type, upb_mapiter_key(i));
  lupb_pushmsgval(L, lmap->value_type, upb_mapiter_value(i));
  upb_mapiter_next(i);

  return 2;
}

static int lupb_map_pairs(lua_State *L) {
  lupb_map *lmap = lupb_map_check(L, 1);

  if (lupb_istypewrapped(lmap->key_type) ||
      lupb_istypewrapped(lmap->value_type)) {
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
 * - [MSG_MSGDEF_INDEX] -> our message class
 *
 * Fields with scalar number/bool types don't go in the userval.
 */

#define MSG_MSGDEF_INDEX 1

int lupb_fieldindex(const upb_fielddef *f) {
  return upb_fielddef_index(f) + 1;  /* 1-based Lua arrays. */
}

typedef struct {
  upb_msg *msg;
} lupb_msg;

/* lupb_msg helpers */

static bool in_userval(const upb_fielddef *f) {
  return lupb_istypewrapped(upb_fielddef_type(f)) || upb_fielddef_isseq(f) ||
         upb_fielddef_ismap(f);
}

upb_msg *lupb_msg_check(lua_State *L, int narg) {
  lupb_msg *msg = luaL_checkudata(L, narg, LUPB_MSG);
  if (!msg->msg) luaL_error(L, "called into dead msg");
  return msg->msg;
}

const upb_msg *lupb_msg_checkmsg(lua_State *L, int narg,
                                 const upb_msgdef *msgdef) {
  lupb_msg_check(L, narg);
  lupb_uservalgeti
  lupb_msgclass_typecheck(L, lmsgclass, lmsg->lmsgclass);
  return lmsg->msg;
}

upb_msg *lupb_msg_checkmsg2(lua_State *L, int narg,
                            const upb_msglayout **layout) {
  lupb_msg *lmsg = lupb_msg_check(L, narg);
  *layout = lmsg->lmsgclass->layout;
  return lmsg->msg;
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
  upb_msg *msg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, lmsg, 2);

  if (in_userval(f)) {
    lupb_uservalgeti(L, 1, lupb_fieldindex(f));

    if (lua_isnil(L, -1)) {
      /* Check if we need to lazily create wrapper. */
      if (upb_fielddef_isseq(f)) {
        /* TODO(haberman) */
      } else if (upb_fielddef_issubmsg(f)) {
        /* TODO(haberman) */
      } else {
        assert(upb_fielddef_isstring(f));
        if (upb_msg_has(msg, f)) {
          upb_msgval val = upb_msg_get(msg, f);
          lua_pop(L, 1);
          lua_pushlstring(L, val.str_val.data, val.str_val.size);
          lupb_uservalseti(L, 1, lupb_fieldindex(f), -1);
        }
      }
    }
  } else {
    upb_msgval val = upb_msg_get(msg, f);
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
  upb_msg *msg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, lmsg, 2);
  upb_fieldtype_t type = upb_fielddef_type(f);
  upb_arena *arena = lupb_arena_get(L);
  upb_msgval msgval;

  /* Typecheck and get msgval. */

  if (upb_fielddef_isseq(f)) {
    msgval = lupb_array_typecheck(L, 3, 1, f);
  } else if (upb_fielddef_ismap(f)) {
    msgval = lupb_map_typecheck(L, 3, 1, f);
  } else {
    const upb_msgdef *msgdef =
        type == UPB_TYPE_MESSAGE ? upb_fielddef_msgsubdef(f) : NULL;
    msgval = lupb_tomsgval(L, type, 3, msgdef);
  }

  /* Set in upb_msg and userval (if necessary). */

  upb_msg_set(msg, f, msgval, arena);

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

static int lupb_decode(lua_State *L) {
  size_t len;
  const upb_msglayout *layout;
  upb_msg *msg = lupb_msg_checkmsg2(L, 1, &layout);
  const char *pb = lua_tolstring(L, 2, &len);

  upb_decode(pb, len, msg, layout, lupb_arena_get(L));
  /* TODO(haberman): check for error. */

  return 0;
}

static int lupb_encode(lua_State *L) {
  const upb_msglayout *layout;
  const upb_msg *msg = lupb_msg_checkmsg2(L, 1, &layout);
  upb_arena *arena = upb_arena_new();
  size_t size;
  char *result;

  result = upb_encode(msg, (const void*)layout, arena, &size);

  /* Free resources before we potentially bail on error. */
  lua_pushlstring(L, result, size);
  upb_arena_free(arena);
  /* TODO(haberman): check for error. */

  return 1;
}

static const struct luaL_Reg lupb_msg_toplevel_m[] = {
  {"Array", lupb_array_new},
  {"Map", lupb_map_new},
  {"MessageFactory", lupb_msgfactory_new},
  {"decode", lupb_decode},
  {"encode", lupb_encode},
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
