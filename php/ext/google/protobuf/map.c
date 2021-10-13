// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "map.h"

#include <Zend/zend_API.h>
#include <Zend/zend_interfaces.h>

#include <ext/spl/spl_iterators.h>

#include "arena.h"
#include "convert.h"
#include "message.h"
#include "php-upb.h"
#include "protobuf.h"

static void MapFieldIter_make(zval *val, zval *map_field);

// -----------------------------------------------------------------------------
// MapField
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  zval arena;
  upb_map *map;
  MapField_Type type;
} MapField;

zend_class_entry *MapField_class_entry;
static zend_object_handlers MapField_object_handlers;

static bool MapType_Eq(MapField_Type a, MapField_Type b) {
  return a.key_type == b.key_type && TypeInfo_Eq(a.val_type, b.val_type);
}

static TypeInfo KeyType(MapField_Type type) {
  TypeInfo ret = {type.key_type};
  return ret;
}

MapField_Type MapType_Get(const upb_fielddef *f) {
  const upb_msgdef *ent = upb_fielddef_msgsubdef(f);
  const upb_fielddef *key_f = upb_msgdef_itof(ent, 1);
  const upb_fielddef *val_f = upb_msgdef_itof(ent, 2);
  MapField_Type type = {
      upb_fielddef_type(key_f),
      {upb_fielddef_type(val_f), Descriptor_GetFromFieldDef(val_f)}};
  return type;
}

// PHP Object Handlers /////////////////////////////////////////////////////////

/**
 * MapField_create()
 *
 * PHP class entry function to allocate and initialize a new MapField
 * object.
 */
static zend_object* MapField_create(zend_class_entry *class_type) {
  MapField *intern = emalloc(sizeof(MapField));
  zend_object_std_init(&intern->std, class_type);
  intern->std.handlers = &MapField_object_handlers;
  Arena_Init(&intern->arena);
  intern->map = NULL;
  // Skip object_properties_init(), we don't allow derived classes.
  return &intern->std;
}

/**
 * MapField_dtor()
 *
 * Object handler to destroy a MapField. This releases all resources
 * associated with the message. Note that it is possible to access a destroyed
 * object from PHP in rare cases.
 */
static void MapField_destructor(zend_object* obj) {
  MapField* intern = (MapField*)obj;
  ObjCache_Delete(intern->map);
  zval_ptr_dtor(&intern->arena);
  zend_object_std_dtor(&intern->std);
}

/**
 * MapField_compare_objects()
 *
 * Object handler for comparing two repeated field objects. Called whenever PHP
 * code does:
 *
 *   $map1 == $map2
 */
static int MapField_compare_objects(zval *map1, zval *map2) {
  MapField* intern1 = (MapField*)Z_OBJ_P(map1);
  MapField* intern2 = (MapField*)Z_OBJ_P(map2);

  return MapType_Eq(intern1->type, intern2->type) &&
                 MapEq(intern1->map, intern2->map, intern1->type)
             ? 0
             : 1;
}

/**
 * MapField_clone_obj()
 *
 * Object handler for cloning an object in PHP. Called when PHP code does:
 *
 *   $map2 = clone $map1;
 */
static zend_object *MapField_clone_obj(PROTO_VAL *object) {
  MapField* intern = PROTO_VAL_P(object);
  upb_arena *arena = Arena_Get(&intern->arena);
  upb_map *clone =
      upb_map_new(arena, intern->type.key_type, intern->type.val_type.type);
  size_t iter = UPB_MAP_BEGIN;

  while (upb_mapiter_next(intern->map, &iter)) {
    upb_msgval key = upb_mapiter_key(intern->map, iter);
    upb_msgval val = upb_mapiter_value(intern->map, iter);
    upb_map_set(clone, key, val, arena);
  }

  zval ret;
  MapField_GetPhpWrapper(&ret, clone, intern->type, &intern->arena);
  return Z_OBJ_P(&ret);
}

static zval *Map_GetPropertyPtrPtr(PROTO_VAL *object, PROTO_STR *member,
                                   int type, void **cache_slot) {
  return NULL;  // We don't offer direct references to our properties.
}

static HashTable *Map_GetProperties(PROTO_VAL *object) {
  return NULL;  // We do not have a properties table.
}

// C Functions from map.h //////////////////////////////////////////////////////

// These are documented in the header file.

void MapField_GetPhpWrapper(zval *val, upb_map *map, MapField_Type type,
                            zval *arena) {
  if (!map) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(map, val)) {
    MapField *intern = emalloc(sizeof(MapField));
    zend_object_std_init(&intern->std, MapField_class_entry);
    intern->std.handlers = &MapField_object_handlers;
    ZVAL_COPY(&intern->arena, arena);
    intern->map = map;
    intern->type = type;
    // Skip object_properties_init(), we don't allow derived classes.
    ObjCache_Add(intern->map, &intern->std);
    ZVAL_OBJ(val, &intern->std);
  }
}

upb_map *MapField_GetUpbMap(zval *val, MapField_Type type, upb_arena *arena) {
  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }

  if (Z_TYPE_P(val) == IS_ARRAY) {
    upb_map *map = upb_map_new(arena, type.key_type, type.val_type.type);
    HashTable *table = HASH_OF(val);
    HashPosition pos;

    zend_hash_internal_pointer_reset_ex(table, &pos);

    while (true) {
      zval php_key;
      zval *php_val;
      upb_msgval upb_key;
      upb_msgval upb_val;

      zend_hash_get_current_key_zval_ex(table, &php_key, &pos);
      php_val = zend_hash_get_current_data_ex(table, &pos);

      if (!php_val) return map;

      if (!Convert_PhpToUpb(&php_key, &upb_key, KeyType(type), arena) ||
          !Convert_PhpToUpbAutoWrap(php_val, &upb_val, type.val_type, arena)) {
        return NULL;
      }

      upb_map_set(map, upb_key, upb_val, arena);
      zend_hash_move_forward_ex(table, &pos);
      zval_dtor(&php_key);
    }
  } else if (Z_TYPE_P(val) == IS_OBJECT &&
             Z_OBJCE_P(val) == MapField_class_entry) {
    MapField *intern = (MapField*)Z_OBJ_P(val);

    if (!MapType_Eq(intern->type, type)) {
      php_error_docref(NULL, E_USER_ERROR, "Wrong type for this map field.");
      return NULL;
    }

    upb_arena_fuse(arena, Arena_Get(&intern->arena));
    return intern->map;
  } else {
    php_error_docref(NULL, E_USER_ERROR, "Must be a map");
    return NULL;
  }
}

bool MapEq(const upb_map *m1, const upb_map *m2, MapField_Type type) {
  size_t iter = UPB_MAP_BEGIN;

  if ((m1 == NULL) != (m2 == NULL)) return false;
  if (m1 == NULL) return true;
  if (upb_map_size(m1) != upb_map_size(m2)) return false;

  while (upb_mapiter_next(m1, &iter)) {
    upb_msgval key = upb_mapiter_key(m1, iter);
    upb_msgval val1 = upb_mapiter_value(m1, iter);
    upb_msgval val2;

    if (!upb_map_get(m2, key, &val2)) return false;
    if (!ValueEq(val1, val2, type.val_type)) return false;
  }

  return true;
}


// MapField PHP methods ////////////////////////////////////////////////////////

/**
 * MapField::__construct()
 *
 * Constructs an instance of MapField.
 * @param long Key type.
 * @param long Value type.
 * @param string Message/Enum class (message/enum value types only).
 */
PHP_METHOD(MapField, __construct) {
  MapField *intern = (MapField*)Z_OBJ_P(getThis());
  upb_arena *arena = Arena_Get(&intern->arena);
  zend_long key_type, val_type;
  zend_class_entry* klass = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll|C", &key_type, &val_type,
                            &klass) != SUCCESS) {
    return;
  }

  intern->type.key_type = pbphp_dtype_to_type(key_type);
  intern->type.val_type.type = pbphp_dtype_to_type(val_type);
  intern->type.val_type.desc = Descriptor_GetFromClassEntry(klass);

  // Check that the key type is an allowed type.
  switch (intern->type.key_type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_BOOL:
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      // These are OK.
      break;
    default:
      zend_error(E_USER_ERROR, "Invalid key type for map.");
  }

  if (intern->type.val_type.type == UPB_TYPE_MESSAGE && klass == NULL) {
    php_error_docref(NULL, E_USER_ERROR,
                     "Message/enum type must have concrete class.");
    return;
  }

  intern->map =
      upb_map_new(arena, intern->type.key_type, intern->type.val_type.type);
  ObjCache_Add(intern->map, &intern->std);
}

/**
 * MapField::offsetExists()
 *
 * Implements the ArrayAccess interface. Invoked when PHP code calls:
 *
 *   isset($map[$idx]);
 *   empty($map[$idx]);
 *
 * @param long The index to be checked.
 * @return bool True if the element at the given index exists.
 */
PHP_METHOD(MapField, offsetExists) {
  MapField *intern = (MapField*)Z_OBJ_P(getThis());
  zval *key;
  upb_msgval upb_key;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) != SUCCESS ||
      !Convert_PhpToUpb(key, &upb_key, KeyType(intern->type), NULL)) {
    return;
  }

  RETURN_BOOL(upb_map_get(intern->map, upb_key, NULL));
}

/**
 * MapField::offsetGet()
 *
 * Implements the ArrayAccess interface. Invoked when PHP code calls:
 *
 *   $x = $map[$idx];
 *
 * @param long The index of the element to be fetched.
 * @return object The stored element at given index.
 * @exception Invalid type for index.
 * @exception Non-existing index.
 */
PHP_METHOD(MapField, offsetGet) {
  MapField *intern = (MapField*)Z_OBJ_P(getThis());
  zval *key;
  zval ret;
  upb_msgval upb_key, upb_val;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) != SUCCESS ||
      !Convert_PhpToUpb(key, &upb_key, KeyType(intern->type), NULL)) {
    return;
  }

  if (!upb_map_get(intern->map, upb_key, &upb_val)) {
    zend_error(E_USER_ERROR, "Given key doesn't exist.");
    return;
  }

  Convert_UpbToPhp(upb_val, &ret, intern->type.val_type, &intern->arena);
  RETURN_COPY_VALUE(&ret);
}

/**
 * MapField::offsetSet()
 *
 * Implements the ArrayAccess interface. Invoked when PHP code calls:
 *
 *   $map[$idx] = $x;
 *
 * @param long The index of the element to be assigned.
 * @param object The element to be assigned.
 * @exception Invalid type for index.
 * @exception Non-existing index.
 * @exception Incorrect type of the element.
 */
PHP_METHOD(MapField, offsetSet) {
  MapField *intern = (MapField*)Z_OBJ_P(getThis());
  upb_arena *arena = Arena_Get(&intern->arena);
  zval *key, *val;
  upb_msgval upb_key, upb_val;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &key, &val) != SUCCESS ||
      !Convert_PhpToUpb(key, &upb_key, KeyType(intern->type), NULL) ||
      !Convert_PhpToUpb(val, &upb_val, intern->type.val_type, arena)) {
    return;
  }

  upb_map_set(intern->map, upb_key, upb_val, arena);
}

/**
 * MapField::offsetUnset()
 *
 * Implements the ArrayAccess interface. Invoked when PHP code calls:
 *
 *   unset($map[$idx]);
 *
 * @param long The index of the element to be removed.
 * @exception Invalid type for index.
 * @exception The element to be removed is not at the end of the MapField.
 */
PHP_METHOD(MapField, offsetUnset) {
  MapField *intern = (MapField*)Z_OBJ_P(getThis());
  zval *key;
  upb_msgval upb_key;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) != SUCCESS ||
      !Convert_PhpToUpb(key, &upb_key, KeyType(intern->type), NULL)) {
    return;
  }

  upb_map_delete(intern->map, upb_key);
}

/**
 * MapField::count()
 *
 * Implements the Countable interface. Invoked when PHP code calls:
 *
 *   $len = count($map);
 * Return the number of stored elements.
 * This will also be called for: count($map)
 * @return long The number of stored elements.
 */
PHP_METHOD(MapField, count) {
  MapField *intern = (MapField*)Z_OBJ_P(getThis());

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  RETURN_LONG(upb_map_size(intern->map));
}

/**
 * MapField::getIterator()
 *
 * Implements the IteratorAggregate interface. Invoked when PHP code calls:
 *
 *   foreach ($arr) {}
 *
 * @return object Beginning iterator.
 */
PHP_METHOD(MapField, getIterator) {
  zval ret;
  MapFieldIter_make(&ret, getThis());
  RETURN_COPY_VALUE(&ret);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_construct, 0, 0, 2)
  ZEND_ARG_INFO(0, key_type)
  ZEND_ARG_INFO(0, value_type)
  ZEND_ARG_INFO(0, value_class)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetGet, 0, 0, 1)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetSet, 0, 0, 2)
  ZEND_ARG_INFO(0, index)
  ZEND_ARG_INFO(0, newval)
ZEND_END_ARG_INFO()

static zend_function_entry MapField_methods[] = {
  PHP_ME(MapField, __construct,  arginfo_construct, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetExists, arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetGet,    arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetSet,    arginfo_offsetSet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetUnset,  arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, count,        arginfo_void,      ZEND_ACC_PUBLIC)
  PHP_ME(MapField, getIterator,  arginfo_void,      ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// MapFieldIter
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  zval map_field;
  size_t position;
} MapFieldIter;

zend_class_entry *MapFieldIter_class_entry;
static zend_object_handlers MapFieldIter_object_handlers;

/**
 * MapFieldIter_create()
 *
 * PHP class entry function to allocate and initialize a new MapFieldIter
 * object.
 */
zend_object* MapFieldIter_create(zend_class_entry *class_type) {
  MapFieldIter *intern = emalloc(sizeof(MapFieldIter));
  zend_object_std_init(&intern->std, class_type);
  intern->std.handlers = &MapFieldIter_object_handlers;
  ZVAL_NULL(&intern->map_field);
  intern->position = 0;
  // Skip object_properties_init(), we don't allow derived classes.
  return &intern->std;
}

/**
 * MapFieldIter_dtor()
 *
 * Object handler to destroy a MapFieldIter. This releases all resources
 * associated with the message. Note that it is possible to access a destroyed
 * object from PHP in rare cases.
 */
static void map_field_iter_dtor(zend_object* obj) {
  MapFieldIter* intern = (MapFieldIter*)obj;
  zval_ptr_dtor(&intern->map_field);
  zend_object_std_dtor(&intern->std);
}

/**
 * MapFieldIter_make()
 *
 * Function to create a MapFieldIter directly from C.
 */
static void MapFieldIter_make(zval *val, zval *map_field) {
  MapFieldIter *iter;
  ZVAL_OBJ(val,
           MapFieldIter_class_entry->create_object(MapFieldIter_class_entry));
  iter = (MapFieldIter*)Z_OBJ_P(val);
  ZVAL_COPY(&iter->map_field, map_field);
}

// -----------------------------------------------------------------------------
// PHP MapFieldIter Methods
// -----------------------------------------------------------------------------

/*
 * When a user writes:
 *
 *   foreach($arr as $key => $val) {}
 *
 * PHP translates this into:
 *
 *   $iter = $arr->getIterator();
 *   for ($iter->rewind(); $iter->valid(); $iter->next()) {
 *     $key = $iter->key();
 *     $val = $iter->current();
 *   }
 */

/**
 * MapFieldIter::rewind()
 *
 * Implements the Iterator interface. Sets the iterator to the first element.
 */
PHP_METHOD(MapFieldIter, rewind) {
  MapFieldIter *intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField *map_field = (MapField*)Z_OBJ_P(&intern->map_field);
  intern->position = UPB_MAP_BEGIN;
  upb_mapiter_next(map_field->map, &intern->position);
}

/**
 * MapFieldIter::current()
 *
 * Implements the Iterator interface. Returns the current value.
 */
PHP_METHOD(MapFieldIter, current) {
  MapFieldIter *intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField *field = (MapField*)Z_OBJ_P(&intern->map_field);
  upb_msgval upb_val = upb_mapiter_value(field->map, intern->position);
  zval ret;
  Convert_UpbToPhp(upb_val, &ret, field->type.val_type, &field->arena);
  RETURN_COPY_VALUE(&ret);
}

/**
 * MapFieldIter::key()
 *
 * Implements the Iterator interface. Returns the current key.
 */
PHP_METHOD(MapFieldIter, key) {
  MapFieldIter *intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField *field = (MapField*)Z_OBJ_P(&intern->map_field);
  upb_msgval upb_key = upb_mapiter_key(field->map, intern->position);
  zval ret;
  Convert_UpbToPhp(upb_key, &ret, KeyType(field->type), NULL);
  RETURN_COPY_VALUE(&ret);
}

/**
 * MapFieldIter::next()
 *
 * Implements the Iterator interface. Advances to the next element.
 */
PHP_METHOD(MapFieldIter, next) {
  MapFieldIter *intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField *field = (MapField*)Z_OBJ_P(&intern->map_field);
  upb_mapiter_next(field->map, &intern->position);
}

/**
 * MapFieldIter::valid()
 *
 * Implements the Iterator interface. Returns true if this is a valid element.
 */
PHP_METHOD(MapFieldIter, valid) {
  MapFieldIter *intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField *field = (MapField*)Z_OBJ_P(&intern->map_field);
  bool done = upb_mapiter_done(field->map, intern->position);
  RETURN_BOOL(!done);
}

static zend_function_entry map_field_iter_methods[] = {
  PHP_ME(MapFieldIter, rewind,      arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, current,     arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, key,         arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, next,        arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, valid,       arginfo_void, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// Module init.
// -----------------------------------------------------------------------------

/**
 * Map_ModuleInit()
 *
 * Called when the C extension is loaded to register all types.
 */

void Map_ModuleInit() {
  zend_class_entry tmp_ce;
  zend_object_handlers *h;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Internal\\MapField",
                   MapField_methods);

  MapField_class_entry = zend_register_internal_class(&tmp_ce);
  zend_class_implements(MapField_class_entry, 3, zend_ce_arrayaccess,
                        zend_ce_aggregate, zend_ce_countable);
  MapField_class_entry->ce_flags |= ZEND_ACC_FINAL;
  MapField_class_entry->create_object = MapField_create;

  h = &MapField_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = MapField_destructor;
#if PHP_VERSION_ID < 80000
  h->compare_objects = MapField_compare_objects;
#else
  h->compare = MapField_compare_objects;
#endif
  h->clone_obj = MapField_clone_obj;
  h->get_properties = Map_GetProperties;
  h->get_property_ptr_ptr = Map_GetPropertyPtrPtr;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Internal\\MapFieldIter",
                   map_field_iter_methods);

  MapFieldIter_class_entry = zend_register_internal_class(&tmp_ce);
  zend_class_implements(MapFieldIter_class_entry, 1, zend_ce_iterator);
  MapFieldIter_class_entry->ce_flags |= ZEND_ACC_FINAL;
  MapFieldIter_class_entry->ce_flags |= ZEND_ACC_FINAL;
  MapFieldIter_class_entry->create_object = MapFieldIter_create;

  h = &MapFieldIter_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = map_field_iter_dtor;
}
