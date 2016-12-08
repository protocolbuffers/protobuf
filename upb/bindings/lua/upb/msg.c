
/* lupb_msgfactory **************************************************************/

/* Userval contains a map of:
 *   [1] = SymbolTable (to keep GC-reachable)
 *   const upb_msgdef* -> lupb_msgclass
 */

#define LUPB_MSGFACTORY_SYMTAB 1

typedef struct lupb_msgfactory {
  upb_msgfactory *factory;
} lupb_msgfactory;

static int lupb_msgclass_pushnew(lua_State *L, int factory, const upb_msglayout *l);

static lupb_msgfactory *lupb_msgfactory_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_MSGFACTORY);
}

static int lupb_msgfactory_new(lua_State *L) {
  const upb_symtab *symtab = lupb_symtab_check(L, 1);

  lupb_msgfactory *lmsgfactory =
      lupb_newuserdata(L, sizeof(lupb_msgfactory), LUPB_MSGFACTORY);
  lmsgfactory->factory = upb_msgfactory_new(symtab);
  lupb_uservalseti(L, -1, LUPB_MSGFACTORY_SYMTAB, 1);

  return 1;
}

static int lupb_msgfactory_gc(lua_State *L) {
  lupb_msgfactory *lfactory = lupb_msgfactory_check(L, 1);

  if (lfactory->factory) {
    upb_msgfactory_free(lfactory->factory);
    lfactory->factory = NULL;
  }

  return 0;
}

static void lupb_msgfactory_pushmsgclass(lua_State *L, int narg,
                                         const upb_msgdef *md) {
  const lupb_msgfactory *lfactory = lupb_msgfactory_check(L, narg);

  lua_getuservalue(L, narg);
  lua_pushlightuserdata(L, (void*)md);
  lua_rawget(L, -2);

  if (lua_isnil(L, -1)) {
    /* TODO: verify md is in symtab? */
    lupb_msgclass_pushnew(L, narg,
                          upb_msgfactory_getlayout(lfactory->factory, md));

    /* Set in userval. */
    lua_pushlightuserdata(L, (void*)md);
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
  }
}

static int lupb_msgfactory_getmsgclass(lua_State *L) {
  lupb_msgfactory_pushmsgclass(L, 1, lupb_msgdef_check(L, 2));
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

#define LUPB_MSGCLASS_FACTORY 1
#define LUPB_MSGCLASS_MSGDEF 2

typedef struct lupb_msgclass {
  const upb_msglayout *layout;
  const lupb_msgfactory *lfactory;
} lupb_msgclass;

/* Checks that the given object is a lupb_msg with the given lmsgclass. */
static upb_msgval lupb_msg_typecheck(lua_State *L, int narg,
                                     const lupb_msgclass *lmsgclass);

/* Type-checks for assigning to a message field. */
static upb_msgval lupb_array_typecheck(lua_State *L, int narg, int msg,
                                       const upb_fielddef *f);
static upb_msgval lupb_map_typecheck(lua_State *L, int narg, int msg,
                                     const upb_fielddef *f);
static const lupb_msgclass *lupb_msg_getsubmsgclass(lua_State *L, int narg,
                                                    const upb_fielddef *f);
static const lupb_msgclass *lupb_msg_msgclassfor(lua_State *L, int narg,
                                                 const upb_msgdef *md);

static lupb_msgclass *lupb_msgclass_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_MSGCLASS);
}

static void lupb_msgclass_typecheck(lua_State *L, const lupb_msgclass *expected,
                                    const lupb_msgclass *actual) {
  if (expected != actual) {
    const upb_msgdef *msgdef = upb_msglayout_msgdef(expected->layout);
    /* TODO: better error message. */
    luaL_typerror(L, 3, upb_msgdef_fullname(msgdef));
  }
}

static const lupb_msgclass *lupb_msgclass_msgclassfor(lua_State *L, int narg,
                                                      const upb_msgdef *md) {
  lupb_uservalgeti(L, narg, LUPB_MSGCLASS_FACTORY);
  lupb_msgfactory_pushmsgclass(L, -1, md);
  return lupb_msgclass_check(L, -1);
}

static const lupb_msgclass *lupb_msgclass_getsubmsgclass(lua_State *L, int narg,
                                                         const upb_fielddef *f) {
  /* If we wanted we could try to optimize this by caching these pointers in our
   * msgclass, in an array indexed by field index.  We would still need to fall
   * back to calling msgclassfor(), unless we wanted to eagerly create
   * message classes for all submessages.  But for big schemas that might be a
   * lot of things to build, and we might end up not using most of them. */
  return lupb_msgclass_msgclassfor(L, narg, upb_fielddef_msgsubdef(f));
}

static int lupb_msgclass_pushnew(lua_State *L, int factory, const upb_msglayout *l) {
  const lupb_msgfactory *lfactory = lupb_msgfactory_check(L, factory);
  lupb_msgclass *lmc = lupb_newuserdata(L, sizeof(*lmc), LUPB_MSGCLASS);

  lupb_uservalseti(L, -1, LUPB_MSGCLASS_FACTORY, factory);
  lmc->layout = l;
  lmc->lfactory = lfactory;

  return 1;
}

static int lupb_msgclass_call(lua_State *L) {
  lupb_msg_pushnew(L, 1);
  return 1;
}

static const struct luaL_Reg lupb_msgclass_mm[] = {
  {"__call", lupb_msgclass_call},
  {NULL, NULL}
};




/* lupb_string ****************************************************************/

/* A wrapper around a Lua string.
 *
 * This type is NOT exposed to users.  Users deal with plain Lua strings.
 *
 * This type exists for two reasons:
 *
 * 1. To provide storage for a upb_string, which is required for interoperating
 *    with upb_msg.  It allows upb to visit string data structures without
 *    calling into Lua.
 * 2. To cache a string's UTF-8 validity.  We want to validate that a string is
 *    valid UTF-8 before allowing it to be assigned to a string field.  However
 *    if a string is assigned from one message to another, or assigned to
 *    multiple message fields, we don't want to force the UTF-8 check again.  We
 *    cache inside this object if the UTF-8 check has been performed.
 *
 * TODO(haberman): is this slightly too clever?  If we just exposed this object
 * directly to Lua we could get rid of the cache.  But then the object we expose
 * to users wouldn't be a true string, so expressions like this would fail:
 *
 *   if msg.string_field == "abc" then
 *     -- ...
 *   end
 *
 * Instead users would have to say this, which seems like a drag:
 *
 *   if tostring(msg.string_field) == "abc" then
 *     -- ...
 *   end
 */

typedef struct {
  enum ValidUtf8 {
    UTF8_UNCHECKED = 0,
    UTF8_VALID = 1,
    UTF8_INVALID = 2
  } utf8_validity;  /* Possibly move this into upb_string at some point. */
  /* upb_string follows. */
} lupb_string;

#define LUPB_STRING_INDEX 1  /* The index where we reference the Lua string. */

static upb_string *lupb_string_upbstr(lupb_string *lstring) {
  return lupb_structafter(&lstring[1]);
}

static size_t lupb_string_sizeof() {
  return lupb_sizewithstruct(sizeof(lupb_string), upb_string_sizeof());
}

/* The cache maps char* (lightuserdata) -> lupb_string userdata.  The char* is
 * the string data from a Lua string object.  In practice Lua string objects
 * have a stable char* for the actual string data, so we can safely key by this.
 * See: http://lua-users.org/lists/lua-l/2011-06/msg00401.html
 *
 * The cache's values are weak, so cache entries can be collected if this string
 * is no longer a member of any message, array, or map.  Keeping real Lua
 * strings as weak keys is not possible, because Lua does make strings subject
 * to weak collection, so this would prevent these strings from ever being
 * collected. */
static void lupb_string_pushcache(lua_State *L) {
  static char key;
  lua_pushlightuserdata(L, &key);
  lua_rawget(L, LUA_REGISTRYINDEX);

  /* Lazily create. */
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);  /* nil. */
    lua_newtable(L);
    lua_createtable(L, 0, 1);  /* Cache metatable. */
    lua_pushstring(L, "v");    /* Values are weak. */
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    lua_pushlightuserdata(L, &key);
    lua_pushvalue(L, -2);  /* Cache. */
    lua_rawset(L, LUA_REGISTRYINDEX);
  }
}

static lupb_string *lupb_string_pushwrapper(lua_State *L, int narg) {
  const char *str;
  size_t len;
  lupb_string *lstring;

  lupb_checkstring(L, narg);
  str = lua_tolstring(L, narg, &len);
  lupb_string_pushcache(L);
  lua_pushlightuserdata(L, (void*)str);
  lua_rawget(L, -2);

  if (lua_isnil(L, -1)) {
    /* String wasn't in cache, need to create it. */
    lua_pop(L, 1);  /* nil. */
    lstring = lupb_newuserdata(L, lupb_string_sizeof(), LUPB_STRING);
    lstring->utf8_validity = UTF8_UNCHECKED;
    upb_string_set(lupb_string_upbstr(lstring), str, len);
    lua_pushlightuserdata(L, (void*)str);
    lua_pushvalue(L, -2);
    /* Stack is [cache, lupb_string, str, lupb_string]. */
    lua_rawset(L, -4);

    /* Need to create a reference to the underlying string object, so
     * lupb_string keeps it alive. */
    lupb_uservalseti(L, -1, LUPB_STRING_INDEX, narg);
  } else {
    lstring = lua_touserdata(L, -1);
  }

  lua_remove(L, -2);  /* cache. */
  return lstring;
}

/* The value at narg should be a Lua string object.  This will push a wrapper
 * object (which may be from the cache).  Returns a upb_string* that is valid
 * for as long as the pushed object is alive.
 *
 * This object should only be used internally, and not exposed to users! */
static upb_msgval lupb_string_pushbyteswrapper(lua_State *L, int narg) {
  lupb_string *lstring = lupb_string_pushwrapper(L, narg);
  return upb_msgval_str(lupb_string_upbstr(lstring));
}

/* Like lupb_string_pushbyteswrapper(), except it also validates that the string
 * is valid UTF-8 (if we haven't already) and throws an error if not. */
static upb_msgval lupb_string_pushstringwrapper(lua_State *L, int narg) {
  lupb_string *lstring = lupb_string_pushwrapper(L, narg);

  if (lstring->utf8_validity == UTF8_UNCHECKED) {
    if (true  /* TODO: check UTF-8 */) {
      lstring->utf8_validity = UTF8_VALID;
    } else {
      lstring->utf8_validity = UTF8_INVALID;
    }
  }

  if (lstring->utf8_validity != UTF8_VALID) {
    luaL_error(L, "String is not valid UTF-8");
  }

  return upb_msgval_str(lupb_string_upbstr(lstring));
}

/* Given a previously pushed wrapper object, unwraps it and pushes the plain
 * string object underneath.  This is the only object we should expose to users.
 */
static void lupb_string_unwrap(lua_State *L, int arg) {
  lupb_uservalgeti(L, arg, LUPB_STRING_INDEX);
}


/* upb <-> Lua type conversion ************************************************/

static bool lupb_isstring(upb_fieldtype_t type) {
  return type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES;
}

static bool lupb_istypewrapped(upb_fieldtype_t type) {
  return type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES ||
         type == UPB_TYPE_MESSAGE;
}

static upb_msgval lupb_tomsgval(lua_State *L, upb_fieldtype_t type, int narg,
                                const lupb_msgclass *lmsgclass,
                                bool *pushed_luaobj) {
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
      /* For map lookup by key, we might want a lighter-weight way of creating a
       * temporary string. */
      *pushed_luaobj = true;
      return lupb_string_pushstringwrapper(L, narg);
    case UPB_TYPE_BYTES:
      *pushed_luaobj = true;
      return lupb_string_pushbyteswrapper(L, narg);
    case UPB_TYPE_MESSAGE:
      UPB_ASSERT(lmsgclass);
      *pushed_luaobj = true;
      lua_pushvalue(L, narg);
      return lupb_msg_typecheck(L, narg, lmsgclass);
  }
}

static void lupb_pushmsgval(lua_State *L, upb_fieldtype_t type,
                            upb_msgval val) {
  switch (type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      lupb_pushint32(L, upb_msgval_getint32(val));
      break;
    case UPB_TYPE_INT64:
      lupb_pushint64(L, upb_msgval_getint64(val));
      break;
    case UPB_TYPE_UINT32:
      lupb_pushuint32(L, upb_msgval_getuint32(val));
      break;
    case UPB_TYPE_UINT64:
      lupb_pushuint64(L, upb_msgval_getuint64(val));
      break;
    case UPB_TYPE_DOUBLE:
      lupb_pushdouble(L, upb_msgval_getdouble(val));
      break;
    case UPB_TYPE_FLOAT:
      lupb_pushfloat(L, upb_msgval_getdouble(val));
      break;
    case UPB_TYPE_BOOL:
      lupb_pushbool(L, upb_msgval_getbool(val));
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      lupb_assert(L, false);
  }
}


/* lupb_array *****************************************************************/

/* A strongly typed array.  Implemented by wrapping upb_array.
 *
 * - we only allow integer indices.
 * - all entries must have the correct type.
 * - we do not allow "holes" in the array; you can only assign to an existing
 *   index or one past the end (which will grow the array by one).
 */

typedef struct {
  /* Only needed for array of message.  This wastes space in the non-message
   * case but simplifies the code.  Could optimize away if desired. */
  lupb_msgclass *lmsgclass;

  /* upb_array follows. */
} lupb_array;

static size_t lupb_array_sizeof(upb_fieldtype_t type) {
  return lupb_sizewithstruct(sizeof(lupb_array), upb_array_sizeof(type));
}

static upb_array *lupb_array_upbarr(lupb_array *arr) {
  return lupb_structafter(&arr[1]);
}

static lupb_array *lupb_array_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_ARRAY);
}

static upb_array *lupb_array_check2(lua_State *L, int narg) {
  return lupb_array_upbarr(lupb_array_check(L, narg));
}

static upb_msgval lupb_array_typecheck(lua_State *L, int narg, int msg,
                                       const upb_fielddef *f) {
  lupb_array *larray = lupb_array_check(L, narg);
  upb_array *array = lupb_array_upbarr(larray);

  if (upb_array_type(array) != upb_fielddef_type(f) ||
      lupb_msg_getsubmsgclass(L, msg, f) != larray->lmsgclass) {
    luaL_error(L, "Array had incorrect type (expected: %s, got: %s)",
               upb_fielddef_type(f), upb_array_type(array));
  }

  if (upb_array_type(array) == UPB_TYPE_MESSAGE) {
    lupb_msgclass_typecheck(L, lupb_msg_getsubmsgclass(L, msg, f),
                            larray->lmsgclass);
  }

  return upb_msgval_arr(array);
}

/* We use "int" because of lua_rawseti/lua_rawgeti -- can re-evaluate if we want
 * arrays bigger than 2^31. */
static int lupb_array_checkindex(lua_State *L, int narg, uint32_t max) {
  uint32_t n = lupb_checkuint32(L, narg);
  if (n == 0 || n > max || n > INT_MAX) {  /* Lua uses 1-based indexing. :( */
    luaL_error(L, "Invalid array index.");
  }
  return n;
}

static int lupb_array_new(lua_State *L) {
  lupb_array *larray;
  upb_fieldtype_t type;
  lupb_msgclass *lmsgclass = NULL;

  if (lua_type(L, 1) == LUA_TNUMBER) {
    type = lupb_checkfieldtype(L, 1);
  } else {
    type = UPB_TYPE_MESSAGE;
    lmsgclass = lupb_msgclass_check(L, 1);
    lupb_uservalseti(L, -1, MSGCLASS_INDEX, 1);  /* GC-root lmsgclass. */
  }

  larray = lupb_newuserdata(L, lupb_array_sizeof(type), LUPB_ARRAY);
  larray->lmsgclass = lmsgclass;
  upb_array_init(lupb_array_upbarr(larray), type);

  return 1;
}

static int lupb_array_gc(lua_State *L) {
  upb_array *array = lupb_array_check2(L, 1);
  WITH_ALLOC(upb_array_uninit(array, alloc));
  return 0;
}

static int lupb_array_newindex(lua_State *L) {
  lupb_array *larray = lupb_array_check(L, 1);
  upb_array *array = lupb_array_upbarr(larray);
  upb_fieldtype_t type = upb_array_type(array);
  bool hasuserval = false;
  uint32_t n = lupb_array_checkindex(L, 2, upb_array_size(array) + 1);
  upb_msgval msgval = lupb_tomsgval(L, type, 3, larray->lmsgclass, &hasuserval);

  WITH_ALLOC(upb_array_set(array, n, msgval, alloc));

  if (hasuserval) {
    lupb_uservalseti(L, 1, n, -1);
  }

  return 0;  /* 1 for chained assignments? */
}

static int lupb_array_index(lua_State *L) {
  lupb_array *larray = lupb_array_check(L, 1);
  upb_array *array = lupb_array_upbarr(larray);
  uint32_t n = lupb_array_checkindex(L, 2, upb_array_size(array));
  upb_fieldtype_t type = upb_array_type(array);

  if (lupb_istypewrapped(type)) {
    lupb_uservalgeti(L, 1, n);
    if (lupb_isstring(type)) {
      lupb_string_unwrap(L, -1);
    }
  } else {
    lupb_pushmsgval(L, upb_array_type(array), upb_array_get(array, n));
  }

  return 1;
}

static int lupb_array_len(lua_State *L) {
  upb_array *array = lupb_array_check2(L, 1);
  lua_pushnumber(L, upb_array_size(array));
  return 1;
}

static const struct luaL_Reg lupb_array_mm[] = {
  {"__gc", lupb_array_gc},
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
 * We always keep this synced to the underlying upb_map.  For other value types
 * we don't use the userdata, and we just read/write the underlying upb_map.
 *
 * 
 */

typedef struct {
  const lupb_msgclass *value_lmsgclass;
  /* upb_map follows */
} lupb_map;

/* lupb_map internal functions */

static size_t lupb_map_sizeof(upb_fieldtype_t ktype, upb_fieldtype_t vtype) {
  return lupb_sizewithstruct(sizeof(lupb_map), upb_map_sizeof(ktype, vtype));
}

static upb_map *lupb_map_upbmap(lupb_map *lmap) {
  return lupb_structafter(&lmap[1]);
}

static lupb_map *lupb_map_check(lua_State *L, int narg) {
  return luaL_checkudata(L, narg, LUPB_ARRAY);
}

static upb_map *lupb_map_check2(lua_State *L, int narg) {
  return lupb_map_upbmap(lupb_map_check(L, narg));
}

static upb_msgval lupb_map_typecheck(lua_State *L, int narg, int msg,
                                     const upb_fielddef *f) {
  lupb_map *lmap = lupb_map_check(L, narg);
  upb_map *map = lupb_map_upbmap(lmap);
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

static void lupb_map_lazy

static int lupb_map_gc(lua_State *L) {
  upb_map *map = lupb_map_check2(L, 1);
  WITH_ALLOC(upb_map_uninit(map, alloc));
  return 0;
}

/* lupb_map Public API */

static int lupb_map_new(lua_State *L) {
  lupb_map *lmap;
  upb_map *map;
  upb_fieldtype_t key_type = lupb_checkfieldtype(L, 1);
  upb_fieldtype_t value_type;
  lupb_msgclass *value_lmsgclass = NULL;

  if (lua_type(L, 2) == LUA_TNUMBER) {
    value_type = lupb_checkfieldtype(L, 2);
  } else {
    value_type = UPB_TYPE_MESSAGE;
  }

  lmap = lupb_newuserdata(L, lupb_map_sizeof(key_type, value_type), LUPB_MAP);
  map = lupb_map_upbmap(lmap);

  if (value_type == UPB_TYPE_MESSAGE) {
    value_lmsgclass = lupb_msgclass_check(L, 2);
    lupb_uservalseti(L, -1, MSGCLASS_INDEX, 2);  /* GC-root lmsgclass. */
  }

  lmap->value_lmsgclass = value_lmsgclass;
  WITH_ALLOC(upb_map_init(map, key_type, value_type, alloc));

  return 1;
}

static int lupb_map_index(lua_State *L) {
  lupb_map *lmap = lupb_map_check(L, 1);
  upb_map *map = lupb_map_upbmap(lmap);
  upb_fieldtype_t valtype = upb_map_valuetype(map);
  bool pushedobj;
  /* We don't always use "key", but this call checks the key type. */
  upb_msgval key = lupb_map_tokeymsgval(L, upb_map_keytype(map), 2);

  if (lupb_istypewrapped(valtype)) {
    /* Userval contains the full map, lookup there by key. */
    lupb_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
  } else {
    /* Lookup in upb_map. */
    upb_msgval val;
    if (upb_map_get(map, key, &val)) {
      lupb_map_pushmsgval(L, upb_map_valuetype(map), val);
    } else {
      lua_pushnil(L);
    }
  }

  return 1;
}

static int lupb_map_len(lua_State *L) {
  upb_map *map = lupb_map_check2(L, 1);
  lua_pushnumber(L, upb_map_size(map));
  return 1;
}

static int lupb_map_newindex(lua_State *L) {
  lupb_map *lmap = lupb_map_check(L, 1);
  upb_map *map = lupb_map_upbmap(lmap);
  bool keyobj = false;
  upb_msgval key = lupb_tomsgval(L, upb_map_keytype(map), 2, NULL, &keyobj);

  if (lua_isnil(L, 3)) {
    /* Delete from map. */
    WITH_ALLOC(upb_map_del(map, key, alloc));

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
    bool valobj = false;
    upb_msgval val = lupb_tomsgval(L, upb_map_valuetype(map), 3,
                                   lmap->value_lmsgclass, &valobj);

    WITH_ALLOC(upb_map_set(map, key, val, NULL, alloc));

    if (valobj) {
      /* Set in userval. */
      lupb_getuservalue(L, 1);
      lua_pushvalue(L, 2);
      lua_pushvalue(L, -3);
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
  upb_map *map = lupb_map_upbmap(lmap);
  upb_string *str = malloc(upb_string_sizeof());

  if (upb_mapiter_done(i)) {
    return 0;
  }

  lupb_map_pushmsgval(L, upb_map_keytype(map), upb_mapiter_key(i));
  lupb_map_pushmsgval(L, upb_map_valuetype(map), upb_mapiter_value(i));
  upb_mapiter_next(i);

  free(str);

  return 2;
}

static int lupb_map_pairs(lua_State *L) {
  lupb_map *lmap = lupb_map_check(L, 1);
  upb_map *map = lupb_map_upbmap(lmap);
  upb_mapiter *i = lua_newuserdata(L, upb_mapiter_sizeof());

  upb_mapiter_begin(i, map);
  lua_pushvalue(L, 1);

  /* Upvalues are [upb_mapiter, lupb_map]. */
  lua_pushcclosure(L, &lupb_mapiter_next, 2);

  return 1;
}

/* upb_mapiter ]]] */

static const struct luaL_Reg lupb_map_mm[] = {
  {"__gc", lupb_map_gc},
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
 * - [0] = our message class
 * - [upb_fielddef_index(f)] = any submessage/string/map/repeated obj.
 */

#define LUPB_MSG_MSGCLASSINDEX 0

typedef struct {
  const lupb_msgclass *lmsgclass;
  /* Data follows, in a flat buffer. */
} lupb_msg;

/* lupb_msg helpers */

static bool in_userval(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) || upb_fielddef_issubmsg(f) ||
         upb_fielddef_isstring(f) || upb_fielddef_ismap(f);
}

lupb_msg *lupb_msg_check(lua_State *L, int narg) {
  lupb_msg *msg = luaL_checkudata(L, narg, LUPB_MSG);
  if (!msg->lmsgclass) luaL_error(L, "called into dead msg");
  return msg;
}

void *lupb_msg_upbmsg(lupb_msg *lmsg) {
  return lupb_structafter(&lmsg[1]);
}

static upb_msgval lupb_msg_typecheck(lua_State *L, int narg,
                                     const lupb_msgclass *lmsgclass) {
  lupb_msg *msg = lupb_msg_check(L, narg);
  lupb_msgclass_typecheck(L, msg->lmsgclass, lmsgclass);
  return upb_msgval_msg(msg);
}

const upb_msgdef *lupb_msg_checkdef(lua_State *L, int narg) {
  return upb_msglayout_msgdef(lupb_msg_check(L, narg)->lmsgclass->layout);
}

static const upb_fielddef *lupb_msg_checkfield(lua_State *L,
                                               const lupb_msg *msg,
                                               int fieldarg) {
  size_t len;
  const char *fieldname = luaL_checklstring(L, fieldarg, &len);
  const upb_msgdef *msgdef = upb_msglayout_msgdef(msg->lmsgclass->layout);
  const upb_fielddef *f = upb_msgdef_ntof(msgdef, fieldname, len);

  if (!f) {
    const char *msg = lua_pushfstring(L, "no such field: %s", fieldname);
    luaL_argerror(L, fieldarg, msg);
    return NULL;  /* Never reached. */
  }

  return f;
}

static int lupb_msg_pushnew(lua_State *L, int narg) {
  lupb_msgclass *lmsgclass = lupb_msgclass_check(L, narg);
  size_t size = upb_msg_sizeof(lmsgclass->layout);
  lupb_msg *msg = lupb_newuserdata(L, size, LUPB_MSG);

  msg->lmsgclass = lmsgclass;
  lupb_uservalseti(L, -1, LUPB_MSG_MSGCLASSINDEX, narg);

  return 1;
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

/* lupb_msg Public API */

static int lupb_msg_index(lua_State *L) {
  lupb_msg *msg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, msg, 2);

  if (in_userval(f)) {
    lupb_uservalgeti(L, 1, upb_fielddef_index(f));
    if (upb_fielddef_isseq(f) && lua_isnil(L, -1)) {
      /* TODO(haberman): default-construct empty array. */
    }
  } else {
    lupb_pushmsgval(L, upb_fielddef_type(f),
                    upb_msg_get(msg, f, msg->lmsgclass->layout));
  }

  return 1;
}

static int lupb_msg_newindex(lua_State *L) {
  lupb_msg *lmsg = lupb_msg_check(L, 1);
  const upb_fielddef *f = lupb_msg_checkfield(L, lmsg, 2);
  bool luaobj = false;
  upb_msgval msgval;

  /* Typecheck and get msgval. */

  if (upb_fielddef_isseq(f)) {
    msgval = lupb_array_typecheck(L, 3, 1, f);
    luaobj = true;
  } else if (upb_fielddef_ismap(f)) {
    msgval = lupb_map_typecheck(L, 3, 1, f);
    luaobj = true;
  } else {
    const lupb_msgclass *lmsgclass = NULL;
    upb_fieldtype_t type = upb_fielddef_type(f);

    if (type == UPB_TYPE_MESSAGE) {
      lmsgclass = lupb_msg_getsubmsgclass(L, 1, f);
    }

    msgval = lupb_tomsgval(L, upb_fielddef_type(f), 3, lmsgclass, &luaobj);
  }

  /* Set in upb_msg and userval (if necessary). */

  WITH_ALLOC(upb_msg_set(lmsg, f, msgval, lmsg->lmsgclass->layout, alloc));

  if (luaobj) {
    lupb_uservalseti(L, 1, upb_fielddef_index(f), -1);
  }

  return 0;  /* 1 for chained assignments? */
}

static const struct luaL_Reg lupb_msg_mm[] = {
  {"__index", lupb_msg_index},
  {"__newindex", lupb_msg_newindex},
  {NULL, NULL}
};


