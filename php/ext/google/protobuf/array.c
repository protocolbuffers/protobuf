// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "array.h"

#include <Zend/zend_API.h>
#include <Zend/zend_interfaces.h>

#include <ext/spl/spl_iterators.h>

// This is not self-contained: it must be after other Zend includes.
#include <Zend/zend_exceptions.h>

#include "arena.h"
#include "convert.h"
#include "def.h"
#include "message.h"
#include "php-upb.h"
#include "protobuf.h"

static void RepeatedFieldIter_make(zval* val, zval* repeated_field);

// -----------------------------------------------------------------------------
// RepeatedField
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  zval arena;
  upb_Array* array;
  TypeInfo type;
} RepeatedField;

zend_class_entry* RepeatedField_class_entry;
static zend_object_handlers RepeatedField_object_handlers;

// PHP Object Handlers /////////////////////////////////////////////////////////

/**
 * RepeatedField_create()
 *
 * PHP class entry function to allocate and initialize a new RepeatedField
 * object.
 */
static zend_object* RepeatedField_create(zend_class_entry* class_type) {
  RepeatedField* intern = emalloc(sizeof(RepeatedField));
  zend_object_std_init(&intern->std, class_type);
  intern->std.handlers = &RepeatedField_object_handlers;
  Arena_Init(&intern->arena);
  intern->array = NULL;
  // Skip object_properties_init(), we don't allow derived classes.
  return &intern->std;
}

/**
 * RepeatedField_dtor()
 *
 * Object handler to destroy a RepeatedField. This releases all resources
 * associated with the message. Note that it is possible to access a destroyed
 * object from PHP in rare cases.
 */
static void RepeatedField_destructor(zend_object* obj) {
  RepeatedField* intern = (RepeatedField*)obj;
  ObjCache_Delete(intern->array);
  zval_ptr_dtor(&intern->arena);
  zend_object_std_dtor(&intern->std);
}

/**
 * RepeatedField_compare_objects()
 *
 * Object handler for comparing two repeated field objects. Called whenever PHP
 * code does:
 *
 *   $rf1 == $rf2
 */
static int RepeatedField_compare_objects(zval* rf1, zval* rf2) {
  RepeatedField* intern1 = (RepeatedField*)Z_OBJ_P(rf1);
  RepeatedField* intern2 = (RepeatedField*)Z_OBJ_P(rf2);

  return TypeInfo_Eq(intern1->type, intern2->type) &&
                 ArrayEq(intern1->array, intern2->array, intern1->type)
             ? 0
             : 1;
}

/**
 * RepeatedField_clone_obj()
 *
 * Object handler for cloning an object in PHP. Called when PHP code does:
 *
 *   $rf2 = clone $rf1;
 */
static zend_object* RepeatedField_clone_obj(zend_object* object) {
  RepeatedField* intern = (RepeatedField*)object;
  upb_Arena* arena = Arena_Get(&intern->arena);
  upb_Array* clone = upb_Array_New(arena, intern->type.type);
  size_t n = upb_Array_Size(intern->array);
  size_t i;

  for (i = 0; i < n; i++) {
    upb_MessageValue msgval = upb_Array_Get(intern->array, i);
    upb_Array_Append(clone, msgval, arena);
  }

  zval ret;
  RepeatedField_GetPhpWrapper(&ret, clone, intern->type, &intern->arena);
  return Z_OBJ_P(&ret);
}

static HashTable* RepeatedField_GetProperties(zend_object* object) {
  return NULL;  // We do not have a properties table.
}

static zval* RepeatedField_GetPropertyPtrPtr(zend_object* object,
                                             zend_string* member, int type,
                                             void** cache_slot) {
  return NULL;  // We don't offer direct references to our properties.
}

// C Functions from array.h ////////////////////////////////////////////////////

// These are documented in the header file.

void RepeatedField_GetPhpWrapper(zval* val, upb_Array* arr, TypeInfo type,
                                 zval* arena) {
  if (!arr) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(arr, val)) {
    RepeatedField* intern = emalloc(sizeof(RepeatedField));
    zend_object_std_init(&intern->std, RepeatedField_class_entry);
    intern->std.handlers = &RepeatedField_object_handlers;
    ZVAL_COPY(&intern->arena, arena);
    intern->array = arr;
    intern->type = type;
    // Skip object_properties_init(), we don't allow derived classes.
    ObjCache_Add(intern->array, &intern->std);
    ZVAL_OBJ(val, &intern->std);
  }
}

upb_Array* RepeatedField_GetUpbArray(zval* val, TypeInfo type,
                                     upb_Arena* arena) {
  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }

  if (Z_TYPE_P(val) == IS_ARRAY) {
    // Auto-construct, eg. [1, 2, 3] -> upb_Array([1, 2, 3]).
    upb_Array* arr = upb_Array_New(arena, type.type);
    HashTable* table = HASH_OF(val);
    HashPosition pos;

    zend_hash_internal_pointer_reset_ex(table, &pos);

    while (true) {
      zval* zv = zend_hash_get_current_data_ex(table, &pos);
      upb_MessageValue val;

      if (!zv) return arr;

      if (!Convert_PhpToUpbAutoWrap(zv, &val, type, arena)) {
        return NULL;
      }

      upb_Array_Append(arr, val, arena);
      zend_hash_move_forward_ex(table, &pos);
    }
  } else if (Z_TYPE_P(val) == IS_OBJECT &&
             Z_OBJCE_P(val) == RepeatedField_class_entry) {
    // Unwrap existing RepeatedField object to get the upb_Array* inside.
    RepeatedField* intern = (RepeatedField*)Z_OBJ_P(val);

    if (!TypeInfo_Eq(intern->type, type)) {
      php_error_docref(NULL, E_USER_ERROR,
                       "Wrong type for this repeated field.");
    }

    upb_Arena_Fuse(arena, Arena_Get(&intern->arena));
    return intern->array;
  } else {
    php_error_docref(NULL, E_USER_ERROR, "Must be a repeated field");
    return NULL;
  }
}

bool ArrayEq(const upb_Array* a1, const upb_Array* a2, TypeInfo type) {
  size_t i;
  size_t n;

  if ((a1 == NULL) != (a2 == NULL)) return false;
  if (a1 == NULL) return true;

  n = upb_Array_Size(a1);
  if (n != upb_Array_Size(a2)) return false;

  for (i = 0; i < n; i++) {
    upb_MessageValue val1 = upb_Array_Get(a1, i);
    upb_MessageValue val2 = upb_Array_Get(a2, i);
    if (!ValueEq(val1, val2, type)) return false;
  }

  return true;
}

// RepeatedField PHP methods ///////////////////////////////////////////////////

/**
 * RepeatedField::__construct()
 *
 * Constructs an instance of RepeatedField.
 * @param long Type of the stored element.
 * @param string Message/Enum class.
 */
PHP_METHOD(RepeatedField, __construct) {
  RepeatedField* intern = (RepeatedField*)Z_OBJ_P(getThis());
  upb_Arena* arena = Arena_Get(&intern->arena);
  zend_long type;
  zend_class_entry* klass = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|C", &type, &klass) != SUCCESS) {
    return;
  }

  intern->type.type = pbphp_dtype_to_type(type);
  intern->type.desc = Descriptor_GetFromClassEntry(klass);

  if (intern->type.type == kUpb_CType_Message && klass == NULL) {
    php_error_docref(NULL, E_USER_ERROR,
                     "Message/enum type must have concrete class.");
    return;
  }

  intern->array = upb_Array_New(arena, intern->type.type);
  ObjCache_Add(intern->array, &intern->std);
}

/**
 * RepeatedField::append()
 *
 * Append element to the end of the repeated field.
 * @param object The element to be added.
 */
PHP_METHOD(RepeatedField, append) {
  RepeatedField* intern = (RepeatedField*)Z_OBJ_P(getThis());
  upb_Arena* arena = Arena_Get(&intern->arena);
  zval* php_val;
  upb_MessageValue msgval;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &php_val) != SUCCESS ||
      !Convert_PhpToUpb(php_val, &msgval, intern->type, arena)) {
    return;
  }

  upb_Array_Append(intern->array, msgval, arena);
}

/**
 * RepeatedField::offsetExists(): bool
 *
 * Implements the ArrayAccess interface. Invoked when PHP code calls:
 *
 *   isset($arr[$idx]);
 *   empty($arr[$idx]);
 *
 * @param long The index to be checked.
 * @return bool True if the element at the given index exists.
 */
PHP_METHOD(RepeatedField, offsetExists) {
  RepeatedField* intern = (RepeatedField*)Z_OBJ_P(getThis());
  zend_long index;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) == FAILURE) {
    return;
  }

  RETURN_BOOL(index >= 0 && index < upb_Array_Size(intern->array));
}

/**
 * RepeatedField::offsetGet(): mixed
 *
 * Implements the ArrayAccess interface. Invoked when PHP code calls:
 *
 *   $x = $arr[$idx];
 *
 * @param long The index of the element to be fetched.
 * @return object The stored element at given index.
 * @exception Invalid type for index.
 * @exception Non-existing index.
 */
PHP_METHOD(RepeatedField, offsetGet) {
  RepeatedField* intern = (RepeatedField*)Z_OBJ_P(getThis());
  zend_long index;
  upb_MessageValue msgval;
  zval ret;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) == FAILURE) {
    return;
  }

  if (index < 0 || index >= upb_Array_Size(intern->array)) {
    zend_error(E_USER_ERROR, "Element at %ld doesn't exist.\n", index);
    return;
  }

  msgval = upb_Array_Get(intern->array, index);
  Convert_UpbToPhp(msgval, &ret, intern->type, &intern->arena);
  RETURN_COPY_VALUE(&ret);
}

/**
 * RepeatedField::offsetSet(): void
 *
 * Implements the ArrayAccess interface. Invoked when PHP code calls:
 *
 *   $arr[$idx] = $x;
 *   $arr []= $x;  // Append
 *
 * @param long The index of the element to be assigned.
 * @param object The element to be assigned.
 * @exception Invalid type for index.
 * @exception Non-existing index.
 * @exception Incorrect type of the element.
 */
PHP_METHOD(RepeatedField, offsetSet) {
  RepeatedField* intern = (RepeatedField*)Z_OBJ_P(getThis());
  upb_Arena* arena = Arena_Get(&intern->arena);
  size_t size = upb_Array_Size(intern->array);
  zval *offset, *val;
  int64_t index;
  upb_MessageValue msgval;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &offset, &val) != SUCCESS) {
    return;
  }

  if (Z_TYPE_P(offset) == IS_NULL) {
    index = size;
  } else if (!Convert_PhpToInt64(offset, &index)) {
    return;
  }

  if (!Convert_PhpToUpb(val, &msgval, intern->type, arena)) {
    return;
  }

  if (index > size) {
    zend_error(E_USER_ERROR, "Element at index %ld doesn't exist.\n", index);
  } else if (index == size) {
    upb_Array_Append(intern->array, msgval, Arena_Get(&intern->arena));
  } else {
    upb_Array_Set(intern->array, index, msgval);
  }
}

/**
 * RepeatedField::offsetUnset(): void
 *
 * Implements the ArrayAccess interface. Invoked when PHP code calls:
 *
 *   unset($arr[$idx]);
 *
 * @param long The index of the element to be removed.
 * @exception Invalid type for index.
 * @exception The element to be removed is not at the end of the RepeatedField.
 */
PHP_METHOD(RepeatedField, offsetUnset) {
  RepeatedField* intern = (RepeatedField*)Z_OBJ_P(getThis());
  zend_long index;
  zend_long size = upb_Array_Size(intern->array);

  // Only the element at the end of the array can be removed.
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) != SUCCESS) {
    return;
  }

  if (size == 0 || index < 0 || index >= size) {
    php_error_docref(NULL, E_USER_ERROR, "Cannot remove element at %ld.\n",
                     index);
    return;
  }

  upb_Array_Delete(intern->array, index, 1);
}

/**
 * RepeatedField::count(): int
 *
 * Implements the Countable interface. Invoked when PHP code calls:
 *
 *   $len = count($arr);
 * Return the number of stored elements.
 * This will also be called for: count($arr)
 * @return long The number of stored elements.
 */
PHP_METHOD(RepeatedField, count) {
  RepeatedField* intern = (RepeatedField*)Z_OBJ_P(getThis());

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  RETURN_LONG(upb_Array_Size(intern->array));
}

/**
 * RepeatedField::getIterator(): Traversable
 *
 * Implements the IteratorAggregate interface. Invoked when PHP code calls:
 *
 *   foreach ($arr) {}
 *
 * @return object Beginning iterator.
 */
PHP_METHOD(RepeatedField, getIterator) {
  zval ret;
  RepeatedFieldIter_make(&ret, getThis());
  RETURN_COPY_VALUE(&ret);
}

// clang-format off
ZEND_BEGIN_ARG_INFO_EX(arginfo_construct, 0, 0, 1)
  ZEND_ARG_INFO(0, type)
  ZEND_ARG_INFO(0, class)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_append, 0, 0, 1)
  ZEND_ARG_INFO(0, newval)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_offsetExists, 0, 0, _IS_BOOL, 0)
  ZEND_ARG_INFO(0, index)
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

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_count, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_getIterator, 0, 0, Traversable, 0)
ZEND_END_ARG_INFO()

static zend_function_entry repeated_field_methods[] = {
  PHP_ME(RepeatedField, __construct,  arginfo_construct,    ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, append,       arginfo_append,       ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, offsetExists, arginfo_offsetExists, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, offsetGet,    arginfo_offsetGet,    ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, offsetSet,    arginfo_offsetSet,    ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, offsetUnset,  arginfo_offsetUnset,  ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, count,        arginfo_count,        ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, getIterator,  arginfo_getIterator,  ZEND_ACC_PUBLIC)
  ZEND_FE_END
};
// clang-format on

// -----------------------------------------------------------------------------
// PHP RepeatedFieldIter
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  zval repeated_field;
  zend_long position;
} RepeatedFieldIter;

zend_class_entry* RepeatedFieldIter_class_entry;
static zend_object_handlers repeated_field_iter_object_handlers;

/**
 * RepeatedFieldIter_create()
 *
 * PHP class entry function to allocate and initialize a new RepeatedFieldIter
 * object.
 */
zend_object* RepeatedFieldIter_create(zend_class_entry* class_type) {
  RepeatedFieldIter* intern = emalloc(sizeof(RepeatedFieldIter));
  zend_object_std_init(&intern->std, class_type);
  intern->std.handlers = &repeated_field_iter_object_handlers;
  ZVAL_NULL(&intern->repeated_field);
  intern->position = 0;
  // Skip object_properties_init(), we don't allow derived classes.
  return &intern->std;
}

/**
 * RepeatedFieldIter_dtor()
 *
 * Object handler to destroy a RepeatedFieldIter. This releases all resources
 * associated with the message. Note that it is possible to access a destroyed
 * object from PHP in rare cases.
 */
static void RepeatedFieldIter_dtor(zend_object* obj) {
  RepeatedFieldIter* intern = (RepeatedFieldIter*)obj;
  zval_ptr_dtor(&intern->repeated_field);
  zend_object_std_dtor(&intern->std);
}

/**
 * RepeatedFieldIter_make()
 *
 * C function to create a RepeatedFieldIter.
 */
static void RepeatedFieldIter_make(zval* val, zval* repeated_field) {
  RepeatedFieldIter* iter;
  ZVAL_OBJ(val, RepeatedFieldIter_class_entry->create_object(
                    RepeatedFieldIter_class_entry));
  iter = (RepeatedFieldIter*)Z_OBJ_P(val);
  ZVAL_COPY(&iter->repeated_field, repeated_field);
}

/*
 * When a user writes:
 *
 *   foreach($arr as $key => $val) {}
 *
 * PHP's iterator protocol is:
 *
 *   $iter = $arr->getIterator();
 *   for ($iter->rewind(); $iter->valid(); $iter->next()) {
 *     $key = $iter->key();
 *     $val = $iter->current();
 *   }
 */

/**
 * RepeatedFieldIter::rewind(): void
 *
 * Implements the Iterator interface. Sets the iterator to the first element.
 */
PHP_METHOD(RepeatedFieldIter, rewind) {
  RepeatedFieldIter* intern = (RepeatedFieldIter*)Z_OBJ_P(getThis());
  intern->position = 0;
}

/**
 * RepeatedFieldIter::current(): mixed
 *
 * Implements the Iterator interface. Returns the current value.
 */
PHP_METHOD(RepeatedFieldIter, current) {
  RepeatedFieldIter* intern = (RepeatedFieldIter*)Z_OBJ_P(getThis());
  RepeatedField* field = (RepeatedField*)Z_OBJ_P(&intern->repeated_field);
  upb_Array* array = field->array;
  zend_long index = intern->position;
  upb_MessageValue msgval;
  zval ret;

  if (index < 0 || index >= upb_Array_Size(array)) {
    zend_error(E_USER_ERROR, "Element at %ld doesn't exist.\n", index);
  }

  msgval = upb_Array_Get(array, index);

  Convert_UpbToPhp(msgval, &ret, field->type, &field->arena);
  RETURN_COPY_VALUE(&ret);
}

/**
 * RepeatedFieldIter::key(): mixed
 *
 * Implements the Iterator interface. Returns the current key.
 */
PHP_METHOD(RepeatedFieldIter, key) {
  RepeatedFieldIter* intern = (RepeatedFieldIter*)Z_OBJ_P(getThis());
  RETURN_LONG(intern->position);
}

/**
 * RepeatedFieldIter::next(): void
 *
 * Implements the Iterator interface. Advances to the next element.
 */
PHP_METHOD(RepeatedFieldIter, next) {
  RepeatedFieldIter* intern = (RepeatedFieldIter*)Z_OBJ_P(getThis());
  ++intern->position;
}

/**
 * RepeatedFieldIter::valid(): bool
 *
 * Implements the Iterator interface. Returns true if this is a valid element.
 */
PHP_METHOD(RepeatedFieldIter, valid) {
  RepeatedFieldIter* intern = (RepeatedFieldIter*)Z_OBJ_P(getThis());
  RepeatedField* field = (RepeatedField*)Z_OBJ_P(&intern->repeated_field);
  RETURN_BOOL(intern->position < upb_Array_Size(field->array));
}

// clang-format: off
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_current, 0, 0,
                                                  IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_key, 0, 0, IS_MIXED,
                                                  0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_next, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_valid, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rewind, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static zend_function_entry repeated_field_iter_methods[] = {
    PHP_ME(RepeatedFieldIter, rewind, arginfo_rewind, ZEND_ACC_PUBLIC)
        PHP_ME(RepeatedFieldIter, current, arginfo_current, ZEND_ACC_PUBLIC)
            PHP_ME(RepeatedFieldIter, key, arginfo_key, ZEND_ACC_PUBLIC) PHP_ME(
                RepeatedFieldIter, next, arginfo_next, ZEND_ACC_PUBLIC)
                PHP_ME(RepeatedFieldIter, valid, arginfo_valid, ZEND_ACC_PUBLIC)
                    ZEND_FE_END};
// clang-format: on

// -----------------------------------------------------------------------------
// Module init.
// -----------------------------------------------------------------------------

/**
 * Array_ModuleInit()
 *
 * Called when the C extension is loaded to register all types.
 */
void Array_ModuleInit() {
  zend_class_entry tmp_ce;
  zend_object_handlers* h;

  // RepeatedField.
  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Internal\\RepeatedField",
                   repeated_field_methods);

  RepeatedField_class_entry = zend_register_internal_class(&tmp_ce);
  zend_class_implements(RepeatedField_class_entry, 3, zend_ce_arrayaccess,
                        zend_ce_aggregate, zend_ce_countable);
  RepeatedField_class_entry->ce_flags |= ZEND_ACC_FINAL;
  RepeatedField_class_entry->create_object = RepeatedField_create;

  h = &RepeatedField_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = RepeatedField_destructor;
  h->compare = RepeatedField_compare_objects;
  h->clone_obj = RepeatedField_clone_obj;
  h->get_properties = RepeatedField_GetProperties;
  h->get_property_ptr_ptr = RepeatedField_GetPropertyPtrPtr;

  // RepeatedFieldIter
  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Internal\\RepeatedFieldIter",
                   repeated_field_iter_methods);

  RepeatedFieldIter_class_entry = zend_register_internal_class(&tmp_ce);
  zend_class_implements(RepeatedFieldIter_class_entry, 1, zend_ce_iterator);
  RepeatedFieldIter_class_entry->ce_flags |= ZEND_ACC_FINAL;
  RepeatedFieldIter_class_entry->create_object = RepeatedFieldIter_create;

  h = &repeated_field_iter_object_handlers;
  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = RepeatedFieldIter_dtor;
}
