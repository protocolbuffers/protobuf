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
  ZEND_FE_END
};

// Forward declare static functions.

static zend_object_value repeated_field_create(zend_class_entry *ce TSRMLS_DC);
static void repeated_field_free(void *object TSRMLS_DC);
static int repeated_field_array_init(zval *array, upb_fieldtype_t type,
                                     uint size ZEND_FILE_LINE_DC);
static void repeated_field_free_element(void *object);
static void repeated_field_write_dimension(zval *object, zval *offset,
                                           zval *value TSRMLS_DC);
static int repeated_field_has_dimension(zval *object, zval *offset TSRMLS_DC);
static HashTable *repeated_field_get_gc(zval *object, zval ***table,
                                        int *n TSRMLS_DC);

// -----------------------------------------------------------------------------
// RepeatedField creation/desctruction
// -----------------------------------------------------------------------------

zend_class_entry* repeated_field_type;
zend_object_handlers* repeated_field_handlers;

void repeated_field_init(TSRMLS_D) {
  zend_class_entry class_type;
  const char* class_name = "Google\\Protobuf\\Internal\\RepeatedField";
  INIT_CLASS_ENTRY_EX(class_type, class_name, strlen(class_name),
                      repeated_field_methods);

  repeated_field_type = zend_register_internal_class(&class_type TSRMLS_CC);
  repeated_field_type->create_object = repeated_field_create;

  zend_class_implements(repeated_field_type TSRMLS_CC, 2, spl_ce_ArrayAccess,
                        spl_ce_Countable);

  repeated_field_handlers = PEMALLOC(zend_object_handlers);
  memcpy(repeated_field_handlers, zend_get_std_object_handlers(),
         sizeof(zend_object_handlers));
  repeated_field_handlers->get_gc = repeated_field_get_gc;
}

static zend_object_value repeated_field_create(zend_class_entry *ce TSRMLS_DC) {
  zend_object_value retval = {0};
  RepeatedField *intern;

  intern = emalloc(sizeof(RepeatedField));
  memset(intern, 0, sizeof(RepeatedField));

  zend_object_std_init(&intern->std, ce TSRMLS_CC);
  object_properties_init(&intern->std, ce);

  intern->array = NULL;
  intern->type = 0;
  intern->msg_ce = NULL;

  retval.handle = zend_objects_store_put(
      intern, (zend_objects_store_dtor_t)zend_objects_destroy_object,
      (zend_objects_free_object_storage_t)repeated_field_free, NULL TSRMLS_CC);
  retval.handlers = repeated_field_handlers;

  return retval;
}

static void repeated_field_free(void *object TSRMLS_DC) {
  RepeatedField *intern = object;
  zend_object_std_dtor(&intern->std TSRMLS_CC);
  zval_ptr_dtor(&intern->array);
  efree(object);
}

static int repeated_field_array_init(zval *array, upb_fieldtype_t type,
                                     uint size ZEND_FILE_LINE_DC) {
  ALLOC_HASHTABLE(Z_ARRVAL_P(array));

  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      zend_hash_init(Z_ARRVAL_P(array), size, NULL, ZVAL_PTR_DTOR, 0);
      break;
    default:
      zend_hash_init(Z_ARRVAL_P(array), size, NULL, repeated_field_free_element,
                     0);
  }
  Z_TYPE_P(array) = IS_ARRAY;
  return SUCCESS;
}

static void repeated_field_free_element(void *object) {
}

// -----------------------------------------------------------------------------
// RepeatedField Handlers
// -----------------------------------------------------------------------------

static void repeated_field_write_dimension(zval *object, zval *offset,
                                           zval *value TSRMLS_DC) {
  uint64_t index;

  RepeatedField *intern = zend_object_store_get_object(object TSRMLS_CC);
  HashTable *ht = HASH_OF(intern->array);
  int size = native_slot_size(intern->type);

  unsigned char memory[NATIVE_SLOT_MAX_SIZE];
  memset(memory, 0, NATIVE_SLOT_MAX_SIZE);

  if (!native_slot_set(intern->type, intern->msg_ce, memory, value)) {
    return;
  }

  if (!offset || Z_TYPE_P(offset) == IS_NULL) {
    index = zend_hash_num_elements(HASH_OF(intern->array));
  } else {
    if (protobuf_convert_to_uint64(offset, &index)) {
      if (!zend_hash_index_exists(ht, index)) {
        zend_error(E_USER_ERROR, "Element at %d doesn't exist.\n", index);
        return;
      }
    } else {
      return;
    }
  }

  zend_hash_index_update(ht, index, memory, size, NULL);
}

static HashTable *repeated_field_get_gc(zval *object, zval ***table,
                                        int *n TSRMLS_DC) {
  *table = NULL;
  *n = 0;
  RepeatedField *intern = zend_object_store_get_object(object TSRMLS_CC);
  return HASH_OF(intern->array);
}

// -----------------------------------------------------------------------------
// C RepeatedField Utilities
// -----------------------------------------------------------------------------

void *repeated_field_index_native(RepeatedField *intern, int index) {
  HashTable *ht = HASH_OF(intern->array);
  void *value;

  if (zend_hash_index_find(ht, index, (void **)&value) == FAILURE) {
    zend_error(E_USER_ERROR, "Element at %d doesn't exist.\n", index);
    return NULL;
  }

  return value;
}

void repeated_field_push_native(RepeatedField *intern, void *value TSRMLS_DC) {
  HashTable *ht = HASH_OF(intern->array);
  int size = native_slot_size(intern->type);
  zend_hash_next_index_insert(ht, (void **)value, size, NULL);
}

void repeated_field_create_with_type(zend_class_entry *ce,
                                     const upb_fielddef *field,
                                     zval **repeated_field TSRMLS_DC) {
  MAKE_STD_ZVAL(*repeated_field);
  Z_TYPE_PP(repeated_field) = IS_OBJECT;
  Z_OBJVAL_PP(repeated_field) =
      repeated_field_type->create_object(repeated_field_type TSRMLS_CC);

  RepeatedField *intern =
      zend_object_store_get_object(*repeated_field TSRMLS_CC);
  intern->type = upb_fielddef_type(field);
  if (intern->type == UPB_TYPE_MESSAGE) {
    upb_msgdef *msg = upb_fielddef_msgsubdef(field);
    zval *desc_php = get_def_obj(msg);
    Descriptor *desc = zend_object_store_get_object(desc_php TSRMLS_CC);
    intern->msg_ce = desc->klass;
  }
  MAKE_STD_ZVAL(intern->array);
  repeated_field_array_init(intern->array, intern->type, 0 ZEND_FILE_LINE_CC);

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

  RepeatedField *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
  intern->type = to_fieldtype(type);
  intern->msg_ce = klass;

  MAKE_STD_ZVAL(intern->array);
  repeated_field_array_init(intern->array, intern->type, 0 ZEND_FILE_LINE_CC);

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

  RepeatedField *intern = zend_object_store_get_object(getThis() TSRMLS_CC);

  RETURN_BOOL(index >= 0 &&
              index < zend_hash_num_elements(HASH_OF(intern->array)));
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

  RepeatedField *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
  HashTable *table = HASH_OF(intern->array);

  if (zend_hash_index_find(table, index, (void **)&memory) == FAILURE) {
    zend_error(E_USER_ERROR, "Element at %d doesn't exist.\n", index);
    return;
  }

  native_slot_get(intern->type, memory, return_value_ptr TSRMLS_CC);
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

  RepeatedField *intern = zend_object_store_get_object(getThis() TSRMLS_CC);

  // Only the element at the end of the array can be removed.
  if (index == -1 ||
      index != (zend_hash_num_elements(HASH_OF(intern->array)) - 1)) {
    zend_error(E_USER_ERROR, "Cannot remove element at %d.\n", index);
    return;
  }

  zend_hash_index_del(HASH_OF(intern->array), index);
}

/**
 * Return the number of stored elements.
 * This will also be called for: count($arr)
 * @return long The number of stored elements.
 */
PHP_METHOD(RepeatedField, count) {
  RepeatedField *intern = zend_object_store_get_object(getThis() TSRMLS_CC);

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  RETURN_LONG(zend_hash_num_elements(HASH_OF(intern->array)));
}
