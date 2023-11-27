// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "map.h"

#include <Zend/zend_API.h>
#include <Zend/zend_interfaces.h>

#include <ext/spl/spl_iterators.h>

#include "arena.h"
#include "convert.h"
#include "message.h"
#include "php-upb.h"
#include "protobuf.h"

static void MapFieldIter_make(zval* val, zval* map_field);

// -----------------------------------------------------------------------------
// MapField
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  zval arena;
  upb_Map* map;
  MapField_Type type;
} MapField;

zend_class_entry* MapField_class_entry;
static zend_object_handlers MapField_object_handlers;

static bool MapType_Eq(MapField_Type a, MapField_Type b) {
  return a.key_type == b.key_type && TypeInfo_Eq(a.val_type, b.val_type);
}

static TypeInfo KeyType(MapField_Type type) {
  TypeInfo ret = {type.key_type};
  return ret;
}

MapField_Type MapType_Get(const upb_FieldDef* f) {
  const upb_MessageDef* ent = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_FindFieldByNumber(ent, 1);
  const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(ent, 2);
  MapField_Type type = {
      upb_FieldDef_CType(key_f),
      {upb_FieldDef_CType(val_f), Descriptor_GetFromFieldDef(val_f)}};
  return type;
}

// PHP Object Handlers /////////////////////////////////////////////////////////

/**
 * MapField_create()
 *
 * PHP class entry function to allocate and initialize a new MapField
 * object.
 */
static zend_object* MapField_create(zend_class_entry* class_type) {
  MapField* intern = emalloc(sizeof(MapField));
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
static int MapField_compare_objects(zval* map1, zval* map2) {
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
static zend_object* MapField_clone_obj(zend_object* object) {
  MapField* intern = (MapField*)object;
  upb_Arena* arena = Arena_Get(&intern->arena);
  upb_Map* clone =
      upb_Map_New(arena, intern->type.key_type, intern->type.val_type.type);
  size_t iter = kUpb_Map_Begin;

  while (upb_MapIterator_Next(intern->map, &iter)) {
    upb_MessageValue key = upb_MapIterator_Key(intern->map, iter);
    upb_MessageValue val = upb_MapIterator_Value(intern->map, iter);
    upb_Map_Set(clone, key, val, arena);
  }

  zval ret;
  MapField_GetPhpWrapper(&ret, clone, intern->type, &intern->arena);
  return Z_OBJ_P(&ret);
}

static zval* Map_GetPropertyPtrPtr(zend_object* object, zend_string* member,
                                   int type, void** cache_slot) {
  return NULL;  // We don't offer direct references to our properties.
}

static HashTable* Map_GetProperties(zend_object* object) {
  return NULL;  // We do not have a properties table.
}

// C Functions from map.h //////////////////////////////////////////////////////

// These are documented in the header file.

void MapField_GetPhpWrapper(zval* val, upb_Map* map, MapField_Type type,
                            zval* arena) {
  if (!map) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(map, val)) {
    MapField* intern = emalloc(sizeof(MapField));
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

upb_Map* MapField_GetUpbMap(zval* val, MapField_Type type, upb_Arena* arena) {
  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }

  if (Z_TYPE_P(val) == IS_ARRAY) {
    upb_Map* map = upb_Map_New(arena, type.key_type, type.val_type.type);
    HashTable* table = HASH_OF(val);
    HashPosition pos;

    zend_hash_internal_pointer_reset_ex(table, &pos);

    while (true) {
      zval php_key;
      zval* php_val;
      upb_MessageValue upb_key;
      upb_MessageValue upb_val;

      zend_hash_get_current_key_zval_ex(table, &php_key, &pos);
      php_val = zend_hash_get_current_data_ex(table, &pos);

      if (!php_val) return map;

      if (!Convert_PhpToUpb(&php_key, &upb_key, KeyType(type), arena) ||
          !Convert_PhpToUpbAutoWrap(php_val, &upb_val, type.val_type, arena)) {
        return NULL;
      }

      upb_Map_Set(map, upb_key, upb_val, arena);
      zend_hash_move_forward_ex(table, &pos);
      zval_dtor(&php_key);
    }
  } else if (Z_TYPE_P(val) == IS_OBJECT &&
             Z_OBJCE_P(val) == MapField_class_entry) {
    MapField* intern = (MapField*)Z_OBJ_P(val);

    if (!MapType_Eq(intern->type, type)) {
      php_error_docref(NULL, E_USER_ERROR, "Wrong type for this map field.");
      return NULL;
    }

    upb_Arena_Fuse(arena, Arena_Get(&intern->arena));
    return intern->map;
  } else {
    php_error_docref(NULL, E_USER_ERROR, "Must be a map");
    return NULL;
  }
}

bool MapEq(const upb_Map* m1, const upb_Map* m2, MapField_Type type) {
  size_t iter = kUpb_Map_Begin;

  if ((m1 == NULL) != (m2 == NULL)) return false;
  if (m1 == NULL) return true;
  if (upb_Map_Size(m1) != upb_Map_Size(m2)) return false;

  while (upb_MapIterator_Next(m1, &iter)) {
    upb_MessageValue key = upb_MapIterator_Key(m1, iter);
    upb_MessageValue val1 = upb_MapIterator_Value(m1, iter);
    upb_MessageValue val2;

    if (!upb_Map_Get(m2, key, &val2)) return false;
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
  MapField* intern = (MapField*)Z_OBJ_P(getThis());
  upb_Arena* arena = Arena_Get(&intern->arena);
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
    case kUpb_CType_Int32:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt32:
    case kUpb_CType_UInt64:
    case kUpb_CType_Bool:
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      // These are OK.
      break;
    default:
      zend_error(E_USER_ERROR, "Invalid key type for map.");
  }

  if (intern->type.val_type.type == kUpb_CType_Message && klass == NULL) {
    php_error_docref(NULL, E_USER_ERROR,
                     "Message/enum type must have concrete class.");
    return;
  }

  intern->map =
      upb_Map_New(arena, intern->type.key_type, intern->type.val_type.type);
  ObjCache_Add(intern->map, &intern->std);
}

/**
 * MapField::offsetExists(): bool
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
  MapField* intern = (MapField*)Z_OBJ_P(getThis());
  zval* key;
  upb_MessageValue upb_key;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) != SUCCESS ||
      !Convert_PhpToUpb(key, &upb_key, KeyType(intern->type), NULL)) {
    return;
  }

  RETURN_BOOL(upb_Map_Get(intern->map, upb_key, NULL));
}

/**
 * MapField::offsetGet(): mixed
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
  MapField* intern = (MapField*)Z_OBJ_P(getThis());
  zval* key;
  zval ret;
  upb_MessageValue upb_key, upb_val;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) != SUCCESS ||
      !Convert_PhpToUpb(key, &upb_key, KeyType(intern->type), NULL)) {
    return;
  }

  if (!upb_Map_Get(intern->map, upb_key, &upb_val)) {
    zend_error(E_USER_ERROR, "Given key doesn't exist.");
    return;
  }

  Convert_UpbToPhp(upb_val, &ret, intern->type.val_type, &intern->arena);
  RETURN_COPY_VALUE(&ret);
}

/**
 * MapField::offsetSet(): void
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
  MapField* intern = (MapField*)Z_OBJ_P(getThis());
  upb_Arena* arena = Arena_Get(&intern->arena);
  zval *key, *val;
  upb_MessageValue upb_key, upb_val;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &key, &val) != SUCCESS ||
      !Convert_PhpToUpb(key, &upb_key, KeyType(intern->type), NULL) ||
      !Convert_PhpToUpb(val, &upb_val, intern->type.val_type, arena)) {
    return;
  }

  upb_Map_Set(intern->map, upb_key, upb_val, arena);
}

/**
 * MapField::offsetUnset(): void
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
  MapField* intern = (MapField*)Z_OBJ_P(getThis());
  zval* key;
  upb_MessageValue upb_key;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) != SUCCESS ||
      !Convert_PhpToUpb(key, &upb_key, KeyType(intern->type), NULL)) {
    return;
  }

  upb_Map_Delete(intern->map, upb_key, NULL);
}

/**
 * MapField::count(): int
 *
 * Implements the Countable interface. Invoked when PHP code calls:
 *
 *   $len = count($map);
 * Return the number of stored elements.
 * This will also be called for: count($map)
 * @return long The number of stored elements.
 */
PHP_METHOD(MapField, count) {
  MapField* intern = (MapField*)Z_OBJ_P(getThis());

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  RETURN_LONG(upb_Map_Size(intern->map));
}

/**
 * MapField::getIterator(): Traversable
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

// clang-format off
ZEND_BEGIN_ARG_INFO_EX(arginfo_construct, 0, 0, 2)
  ZEND_ARG_INFO(0, key_type)
  ZEND_ARG_INFO(0, value_type)
  ZEND_ARG_INFO(0, value_class)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_offsetGet, 0, 0, IS_MIXED, 1)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_offsetSet, 0, 2, IS_VOID, 0)
  ZEND_ARG_INFO(0, index)
  ZEND_ARG_INFO(0, newval)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_offsetUnset, 0, 0, IS_VOID, 0)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_offsetExists, 0, 0, _IS_BOOL, 0)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_getIterator, 0, 0, Traversable, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_count, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

static zend_function_entry MapField_methods[] = {
  PHP_ME(MapField, __construct,  arginfo_construct,    ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetExists, arginfo_offsetExists, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetGet,    arginfo_offsetGet,    ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetSet,    arginfo_offsetSet,    ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetUnset,  arginfo_offsetUnset,  ZEND_ACC_PUBLIC)
  PHP_ME(MapField, count,        arginfo_count,        ZEND_ACC_PUBLIC)
  PHP_ME(MapField, getIterator,  arginfo_getIterator,  ZEND_ACC_PUBLIC)
  ZEND_FE_END
};
// clang-format on

// -----------------------------------------------------------------------------
// MapFieldIter
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  zval map_field;
  size_t position;
} MapFieldIter;

zend_class_entry* MapFieldIter_class_entry;
static zend_object_handlers MapFieldIter_object_handlers;

/**
 * MapFieldIter_create()
 *
 * PHP class entry function to allocate and initialize a new MapFieldIter
 * object.
 */
zend_object* MapFieldIter_create(zend_class_entry* class_type) {
  MapFieldIter* intern = emalloc(sizeof(MapFieldIter));
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
static void MapFieldIter_make(zval* val, zval* map_field) {
  MapFieldIter* iter;
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
 * MapFieldIter::rewind(): void
 *
 * Implements the Iterator interface. Sets the iterator to the first element.
 */
PHP_METHOD(MapFieldIter, rewind) {
  MapFieldIter* intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField* map_field = (MapField*)Z_OBJ_P(&intern->map_field);
  intern->position = kUpb_Map_Begin;
  upb_MapIterator_Next(map_field->map, &intern->position);
}

/**
 * MapFieldIter::current(): mixed
 *
 * Implements the Iterator interface. Returns the current value.
 */
PHP_METHOD(MapFieldIter, current) {
  MapFieldIter* intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField* field = (MapField*)Z_OBJ_P(&intern->map_field);
  upb_MessageValue upb_val =
      upb_MapIterator_Value(field->map, intern->position);
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
  MapFieldIter* intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField* field = (MapField*)Z_OBJ_P(&intern->map_field);
  upb_MessageValue upb_key = upb_MapIterator_Key(field->map, intern->position);
  zval ret;
  Convert_UpbToPhp(upb_key, &ret, KeyType(field->type), NULL);
  RETURN_COPY_VALUE(&ret);
}

/**
 * MapFieldIter::next(): void
 *
 * Implements the Iterator interface. Advances to the next element.
 */
PHP_METHOD(MapFieldIter, next) {
  MapFieldIter* intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField* field = (MapField*)Z_OBJ_P(&intern->map_field);
  upb_MapIterator_Next(field->map, &intern->position);
}

/**
 * MapFieldIter::valid(): bool
 *
 * Implements the Iterator interface. Returns true if this is a valid element.
 */
PHP_METHOD(MapFieldIter, valid) {
  MapFieldIter* intern = (MapFieldIter*)Z_OBJ_P(getThis());
  MapField* field = (MapField*)Z_OBJ_P(&intern->map_field);
  bool done = upb_MapIterator_Done(field->map, intern->position);
  RETURN_BOOL(!done);
}

// clang-format off
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rewind, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_current, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_key, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_next, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_valid, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

static zend_function_entry map_field_iter_methods[] = {
  PHP_ME(MapFieldIter, rewind,      arginfo_rewind,  ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, current,     arginfo_current, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, key,         arginfo_key,     ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, next,        arginfo_next,    ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, valid,       arginfo_valid,   ZEND_ACC_PUBLIC)
  ZEND_FE_END
};
// clang-format on

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
  zend_object_handlers* h;

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
  h->compare = MapField_compare_objects;
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
