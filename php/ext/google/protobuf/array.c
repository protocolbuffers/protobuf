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

#include <ext/spl/spl_iterators.h>
#include <Zend/zend_API.h>
#include <Zend/zend_interfaces.h>

#include "protobuf.h"

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetGet, 0, 0, 1)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetSet, 0, 0, 2)
  ZEND_ARG_INFO(0, index)
  ZEND_ARG_INFO(0, newval)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_void, 0)
ZEND_END_ARG_INFO()

static zend_function_entry repeated_field_methods[] = {
  PHP_ME(RepeatedField, __construct,  NULL,              ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, append,       NULL,              ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, offsetExists, arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, offsetGet,    arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, offsetSet,    arginfo_offsetSet, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, offsetUnset,  arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, count,        arginfo_void,      ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedField, getIterator,  arginfo_void,      ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

static zend_function_entry repeated_field_iter_methods[] = {
  PHP_ME(RepeatedFieldIter, rewind,      arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedFieldIter, current,     arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedFieldIter, key,         arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedFieldIter, next,        arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedFieldIter, valid,       arginfo_void, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// Forward declare static functions.

static int repeated_field_array_init(zval *array, upb_fieldtype_t type,
                                     uint size ZEND_FILE_LINE_DC);
static void repeated_field_write_dimension(zval *object, zval *offset,
                                           zval *value TSRMLS_DC);
static int repeated_field_has_dimension(zval *object, zval *offset TSRMLS_DC);
static HashTable *repeated_field_get_gc(zval *object, CACHED_VALUE **table,
                                        int *n TSRMLS_DC);
#if PHP_MAJOR_VERSION < 7
static zend_object_value repeated_field_create(zend_class_entry *ce TSRMLS_DC);
static zend_object_value repeated_field_iter_create(zend_class_entry *ce TSRMLS_DC);
#else
static zend_object *repeated_field_create(zend_class_entry *ce TSRMLS_DC);
static zend_object *repeated_field_iter_create(zend_class_entry *ce TSRMLS_DC);
#endif

// -----------------------------------------------------------------------------
// RepeatedField creation/desctruction
// -----------------------------------------------------------------------------

zend_class_entry* repeated_field_type;
zend_class_entry* repeated_field_iter_type;
zend_object_handlers* repeated_field_handlers;
zend_object_handlers* repeated_field_iter_handlers;

// Define object free method.
PHP_PROTO_OBJECT_FREE_START(RepeatedField, repeated_field)
#if PHP_MAJOR_VERSION < 7
php_proto_zval_ptr_dtor(intern->array);
#else
php_proto_zval_ptr_dtor(&intern->array);
#endif
PHP_PROTO_OBJECT_FREE_END

PHP_PROTO_OBJECT_DTOR_START(RepeatedField, repeated_field)
PHP_PROTO_OBJECT_DTOR_END

// Define object create method.
PHP_PROTO_OBJECT_CREATE_START(RepeatedField, repeated_field)
#if PHP_MAJOR_VERSION < 7
intern->array = NULL;
#endif
intern->type = 0;
intern->msg_ce = NULL;
PHP_PROTO_OBJECT_CREATE_END(RepeatedField, repeated_field)

// Init class entry.
PHP_PROTO_INIT_CLASS_START("Google\\Protobuf\\Internal\\RepeatedField",
                           RepeatedField, repeated_field)
zend_class_implements(repeated_field_type TSRMLS_CC, 3, spl_ce_ArrayAccess,
                      zend_ce_aggregate, spl_ce_Countable);
repeated_field_handlers->write_dimension = repeated_field_write_dimension;
repeated_field_handlers->get_gc = repeated_field_get_gc;
PHP_PROTO_INIT_CLASS_END

// Define array element free function.
#if PHP_MAJOR_VERSION < 7
static inline void php_proto_array_string_release(void *value) {
  zval_ptr_dtor(value);
}

static inline void php_proto_array_object_release(void *value) {
  zval_ptr_dtor(value);
}
static inline void php_proto_array_default_release(void *value) {
}
#else
static inline void php_proto_array_string_release(zval *value) {
  void* ptr = Z_PTR_P(value);
  zend_string* object = *(zend_string**)ptr;
  zend_string_release(object);
  efree(ptr);
}
static inline void php_proto_array_object_release(zval *value) {
  zval_ptr_dtor(value);
}
static void php_proto_array_default_release(zval* value) {
  void* ptr = Z_PTR_P(value);
  efree(ptr);
}
#endif

static int repeated_field_array_init(zval *array, upb_fieldtype_t type,
                                     uint size ZEND_FILE_LINE_DC) {
  PHP_PROTO_ALLOC_ARRAY(array);

  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      zend_hash_init(Z_ARRVAL_P(array), size, NULL,
                     php_proto_array_string_release, 0);
      break;
    case UPB_TYPE_MESSAGE:
      zend_hash_init(Z_ARRVAL_P(array), size, NULL,
                     php_proto_array_object_release, 0);
      break;
    default:
      zend_hash_init(Z_ARRVAL_P(array), size, NULL,
                     php_proto_array_default_release, 0);
  }
  return SUCCESS;
}

// -----------------------------------------------------------------------------
// RepeatedField Handlers
// -----------------------------------------------------------------------------

static void repeated_field_write_dimension(zval *object, zval *offset,
                                           zval *value TSRMLS_DC) {
  uint64_t index;

  RepeatedField *intern = UNBOX(RepeatedField, object);
  HashTable *ht = PHP_PROTO_HASH_OF(intern->array);
  int size = native_slot_size(intern->type);

  unsigned char memory[NATIVE_SLOT_MAX_SIZE];
  memset(memory, 0, NATIVE_SLOT_MAX_SIZE);

  if (!native_slot_set_by_array(intern->type, intern->msg_ce, memory,
                                value TSRMLS_CC)) {
    return;
  }

  if (!offset || Z_TYPE_P(offset) == IS_NULL) {
    index = zend_hash_num_elements(PHP_PROTO_HASH_OF(intern->array));
  } else {
    if (protobuf_convert_to_uint64(offset, &index)) {
      if (!zend_hash_index_exists(ht, index)) {
        zend_error(E_USER_ERROR, "Element at %llu doesn't exist.\n",
                   (long long unsigned int)index);
        return;
      }
    } else {
      return;
    }
  }

  if (intern->type == UPB_TYPE_MESSAGE) {
    php_proto_zend_hash_index_update_zval(ht, index, *(zval**)memory);
  } else {
    php_proto_zend_hash_index_update_mem(ht, index, memory, size, NULL);
  }
}

#if PHP_MAJOR_VERSION < 7
static HashTable *repeated_field_get_gc(zval *object, zval ***table,
                                        int *n TSRMLS_DC) {
#else
static HashTable *repeated_field_get_gc(zval *object, zval **table, int *n) {
#endif
  *table = NULL;
  *n = 0;
  RepeatedField *intern = UNBOX(RepeatedField, object);
  return PHP_PROTO_HASH_OF(intern->array);
}

// -----------------------------------------------------------------------------
// C RepeatedField Utilities
// -----------------------------------------------------------------------------

void *repeated_field_index_native(RepeatedField *intern, int index TSRMLS_DC) {
  HashTable *ht = PHP_PROTO_HASH_OF(intern->array);
  void *value;

  if (intern->type == UPB_TYPE_MESSAGE) {
    if (php_proto_zend_hash_index_find_zval(ht, index, (void **)&value) ==
        FAILURE) {
      zend_error(E_USER_ERROR, "Element at %d doesn't exist.\n", index);
      return NULL;
    }
  } else {
    if (php_proto_zend_hash_index_find_mem(ht, index, (void **)&value) ==
        FAILURE) {
      zend_error(E_USER_ERROR, "Element at %d doesn't exist.\n", index);
      return NULL;
    }
  }

  return value;
}

void repeated_field_push_native(RepeatedField *intern, void *value) {
  HashTable *ht = PHP_PROTO_HASH_OF(intern->array);
  int size = native_slot_size(intern->type);
  if (intern->type == UPB_TYPE_MESSAGE) {
    php_proto_zend_hash_next_index_insert_zval(ht, value);
  } else {
    php_proto_zend_hash_next_index_insert_mem(ht, (void **)value, size, NULL);
  }
}

void repeated_field_ensure_created(
    const upb_fielddef *field,
    CACHED_VALUE *repeated_field PHP_PROTO_TSRMLS_DC) {
  if (ZVAL_IS_NULL(CACHED_PTR_TO_ZVAL_PTR(repeated_field))) {
    zval_ptr_dtor(repeated_field);
#if PHP_MAJOR_VERSION < 7
    MAKE_STD_ZVAL(CACHED_PTR_TO_ZVAL_PTR(repeated_field));
#endif
    repeated_field_create_with_field(repeated_field_type, field,
                                     repeated_field PHP_PROTO_TSRMLS_CC);
  }
}

void repeated_field_create_with_field(
    zend_class_entry *ce, const upb_fielddef *field,
    CACHED_VALUE *repeated_field PHP_PROTO_TSRMLS_DC) {
  upb_fieldtype_t type = upb_fielddef_type(field);
  const zend_class_entry *msg_ce = field_type_class(field PHP_PROTO_TSRMLS_CC);
  repeated_field_create_with_type(ce, type, msg_ce,
                                  repeated_field PHP_PROTO_TSRMLS_CC);
}

void repeated_field_create_with_type(
    zend_class_entry *ce, upb_fieldtype_t type, const zend_class_entry *msg_ce,
    CACHED_VALUE *repeated_field PHP_PROTO_TSRMLS_DC) {
  CREATE_OBJ_ON_ALLOCATED_ZVAL_PTR(CACHED_PTR_TO_ZVAL_PTR(repeated_field),
                                   repeated_field_type);

  RepeatedField *intern =
      UNBOX(RepeatedField, CACHED_TO_ZVAL_PTR(*repeated_field));
  intern->type = type;
  intern->msg_ce = msg_ce;
#if PHP_MAJOR_VERSION < 7
  MAKE_STD_ZVAL(intern->array);
  repeated_field_array_init(intern->array, intern->type, 0 ZEND_FILE_LINE_CC);
#else
  repeated_field_array_init(&intern->array, intern->type, 0 ZEND_FILE_LINE_CC);
#endif

  // TODO(teboring): Link class entry for message and enum
}


// -----------------------------------------------------------------------------
// PHP RepeatedField Methods
// -----------------------------------------------------------------------------

/**
 * Constructs an instance of RepeatedField.
 * @param long Type of the stored element.
 * @param string Message/Enum class name (message/enum fields only).
 */
PHP_METHOD(RepeatedField, __construct) {
  long type;
  zend_class_entry* klass = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|C", &type, &klass) ==
      FAILURE) {
    return;
  }

  RepeatedField *intern = UNBOX(RepeatedField, getThis());
  intern->type = to_fieldtype(type);
  intern->msg_ce = klass;

#if PHP_MAJOR_VERSION < 7
  MAKE_STD_ZVAL(intern->array);
  repeated_field_array_init(intern->array, intern->type, 0 ZEND_FILE_LINE_CC);
#else
  repeated_field_array_init(&intern->array, intern->type, 0 ZEND_FILE_LINE_CC);
#endif

  if (intern->type == UPB_TYPE_MESSAGE && klass == NULL) {
    zend_error(E_USER_ERROR, "Message type must have concrete class.");
    return;
  }

  // TODO(teboring): Consider enum.
}

/**
 * Append element to the end of the repeated field.
 * @param object The element to be added.
 */
PHP_METHOD(RepeatedField, append) {
  zval *value;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) ==
      FAILURE) {
    return;
  }
  repeated_field_write_dimension(getThis(), NULL, value TSRMLS_CC);
}

/**
 * Check whether the element at given index exists.
 * @param long The index to be checked.
 * @return bool True if the element at the given index exists.
 */
PHP_METHOD(RepeatedField, offsetExists) {
  long index;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    return;
  }

  RepeatedField *intern = UNBOX(RepeatedField, getThis());

  RETURN_BOOL(index >= 0 &&
              index < zend_hash_num_elements(PHP_PROTO_HASH_OF(intern->array)));
}

/**
 * Return the element at the given index.
 * This will also be called for: $ele = $arr[0]
 * @param long The index of the element to be fetched.
 * @return object The stored element at given index.
 * @exception Invalid type for index.
 * @exception Non-existing index.
 */
PHP_METHOD(RepeatedField, offsetGet) {
  long index;
  void *memory;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    return;
  }

  RepeatedField *intern = UNBOX(RepeatedField, getThis());
  HashTable *table = PHP_PROTO_HASH_OF(intern->array);

  if (intern->type == UPB_TYPE_MESSAGE) {
    if (php_proto_zend_hash_index_find_zval(table, index, (void **)&memory) ==
        FAILURE) {
      zend_error(E_USER_ERROR, "Element at %ld doesn't exist.\n", index);
      return;
    }
  } else {
    if (php_proto_zend_hash_index_find_mem(table, index, (void **)&memory) ==
        FAILURE) {
      zend_error(E_USER_ERROR, "Element at %ld doesn't exist.\n", index);
      return;
    }
  }
  native_slot_get_by_array(intern->type, memory,
                           ZVAL_PTR_TO_CACHED_PTR(return_value) TSRMLS_CC);
}

/**
 * Assign the element at the given index.
 * This will also be called for: $arr []= $ele and $arr[0] = ele
 * @param long The index of the element to be assigned.
 * @param object The element to be assigned.
 * @exception Invalid type for index.
 * @exception Non-existing index.
 * @exception Incorrect type of the element.
 */
PHP_METHOD(RepeatedField, offsetSet) {
  zval *index, *value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &index, &value) ==
      FAILURE) {
    return;
  }
  repeated_field_write_dimension(getThis(), index, value TSRMLS_CC);
}

/**
 * Remove the element at the given index.
 * This will also be called for: unset($arr)
 * @param long The index of the element to be removed.
 * @exception Invalid type for index.
 * @exception The element to be removed is not at the end of the RepeatedField.
 */
PHP_METHOD(RepeatedField, offsetUnset) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    return;
  }

  RepeatedField *intern = UNBOX(RepeatedField, getThis());

  // Only the element at the end of the array can be removed.
  if (index == -1 ||
      index != (zend_hash_num_elements(PHP_PROTO_HASH_OF(intern->array)) - 1)) {
    zend_error(E_USER_ERROR, "Cannot remove element at %ld.\n", index);
    return;
  }

  zend_hash_index_del(PHP_PROTO_HASH_OF(intern->array), index);
}

/**
 * Return the number of stored elements.
 * This will also be called for: count($arr)
 * @return long The number of stored elements.
 */
PHP_METHOD(RepeatedField, count) {
  RepeatedField *intern = UNBOX(RepeatedField, getThis());

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  RETURN_LONG(zend_hash_num_elements(PHP_PROTO_HASH_OF(intern->array)));
}

/**
 * Return the beginning iterator.
 * This will also be called for: foreach($arr)
 * @return object Beginning iterator.
 */
PHP_METHOD(RepeatedField, getIterator) {
  CREATE_OBJ_ON_ALLOCATED_ZVAL_PTR(return_value,
                                   repeated_field_iter_type);

  RepeatedField *intern = UNBOX(RepeatedField, getThis());
  RepeatedFieldIter *iter = UNBOX(RepeatedFieldIter, return_value);
  iter->repeated_field = intern;
  iter->position = 0;
}

// -----------------------------------------------------------------------------
// RepeatedFieldIter creation/desctruction
// -----------------------------------------------------------------------------

// Define object free method.
PHP_PROTO_OBJECT_FREE_START(RepeatedFieldIter, repeated_field_iter)
PHP_PROTO_OBJECT_FREE_END

PHP_PROTO_OBJECT_DTOR_START(RepeatedFieldIter, repeated_field_iter)
PHP_PROTO_OBJECT_DTOR_END

// Define object create method.
PHP_PROTO_OBJECT_CREATE_START(RepeatedFieldIter, repeated_field_iter)
intern->repeated_field = NULL;
intern->position = 0;
PHP_PROTO_OBJECT_CREATE_END(RepeatedFieldIter, repeated_field_iter)

// Init class entry.
PHP_PROTO_INIT_CLASS_START("Google\\Protobuf\\Internal\\RepeatedFieldIter",
                           RepeatedFieldIter, repeated_field_iter)
zend_class_implements(repeated_field_iter_type TSRMLS_CC, 1, zend_ce_iterator);
PHP_PROTO_INIT_CLASS_END

// -----------------------------------------------------------------------------
// PHP RepeatedFieldIter Methods
// -----------------------------------------------------------------------------

PHP_METHOD(RepeatedFieldIter, rewind) {
  RepeatedFieldIter *intern = UNBOX(RepeatedFieldIter, getThis());
  intern->position = 0;
}

PHP_METHOD(RepeatedFieldIter, current) {
  RepeatedFieldIter *intern = UNBOX(RepeatedFieldIter, getThis());
  RepeatedField *repeated_field = intern->repeated_field;

  long index;
  void *memory;

  HashTable *table = PHP_PROTO_HASH_OF(repeated_field->array);

  if (repeated_field->type == UPB_TYPE_MESSAGE) {
    if (php_proto_zend_hash_index_find_zval(table, intern->position,
                                            (void **)&memory) == FAILURE) {
      zend_error(E_USER_ERROR, "Element at %d doesn't exist.\n", index);
      return;
    }
  } else {
    if (php_proto_zend_hash_index_find_mem(table, intern->position,
                                           (void **)&memory) == FAILURE) {
      zend_error(E_USER_ERROR, "Element at %d doesn't exist.\n", index);
      return;
    }
  }
  native_slot_get_by_array(repeated_field->type, memory,
                           ZVAL_PTR_TO_CACHED_PTR(return_value) TSRMLS_CC);
}

PHP_METHOD(RepeatedFieldIter, key) {
  RepeatedFieldIter *intern = UNBOX(RepeatedFieldIter, getThis());
  RETURN_LONG(intern->position);
}

PHP_METHOD(RepeatedFieldIter, next) {
  RepeatedFieldIter *intern = UNBOX(RepeatedFieldIter, getThis());
  ++intern->position;
}

PHP_METHOD(RepeatedFieldIter, valid) {
  RepeatedFieldIter *intern = UNBOX(RepeatedFieldIter, getThis());
  RETURN_BOOL(zend_hash_num_elements(PHP_PROTO_HASH_OF(
                  intern->repeated_field->array)) > intern->position);
}
