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

#include "protobuf_php.h"

// -----------------------------------------------------------------------------
// Define static methods
// -----------------------------------------------------------------------------

static void MapField_init_handlers(zend_object_handlers* handlers) {
}

static void MapField_init_type(zend_class_entry* klass) {
  TSRMLS_FETCH();
  zend_class_implements(klass TSRMLS_CC, 3, spl_ce_ArrayAccess,
                        zend_ce_aggregate, spl_ce_Countable);
}

// -----------------------------------------------------------------------------
// Define PHP class
// -----------------------------------------------------------------------------

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetGet, 0, 0, 1)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetSet, 0, 0, 2)
  ZEND_ARG_INFO(0, index)
  ZEND_ARG_INFO(0, newval)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_void, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(MapField, __construct);
PHP_METHOD(MapField, offsetExists);
PHP_METHOD(MapField, offsetGet);
PHP_METHOD(MapField, offsetSet);
PHP_METHOD(MapField, offsetUnset);
PHP_METHOD(MapField, count);
PHP_METHOD(MapField, getIterator);

static zend_function_entry MapField_methods[] = {
  PHP_ME(MapField, __construct,  NULL,              ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetExists, arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetGet,    arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetSet,    arginfo_offsetSet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetUnset,  arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, count,        arginfo_void,      ZEND_ACC_PUBLIC)
  PHP_ME(MapField, getIterator,  arginfo_void,      ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

PROTO_DEFINE_CLASS(MapField,
                   "Google\\Protobuf\\Internal\\MapField");

// -----------------------------------------------------------------------------
// Define PHP methods
// -----------------------------------------------------------------------------

/**
 * Constructs an instance of RepeatedField.
 * @param long Type of the stored key.
 * @param long Type of the stored value.
 * @param string Message/Enum class name (message/enum fields only).
 */
PHP_METHOD(MapField, __construct) {
  long key_type, value_type;
  zend_class_entry* klass = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|C", &key_type,
                            &value_type, &klass) == FAILURE) {
    return;
  }

  // Check that the key type is an allowed type.
  switch (to_fieldtype(static_cast<upb_descriptortype_t>(key_type))) {
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

  MapField *intern = UNBOX(MapField, getThis());
  intern->klass = klass;
  MapField___construct(
      intern, static_cast<upb_descriptortype_t>(key_type),
      static_cast<upb_descriptortype_t>(value_type), klass);
}

/**
 * Check whether the element at given key exists.
 * @param mixed The key to be checked.
 * @return bool True if the element at the given key exists.
 */
PHP_METHOD(MapField, offsetExists) {
  zval *key;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &key) ==
      FAILURE) {
    return;
  }

  MapField *intern = UNBOX(MapField, getThis());
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map));
  upb_msgval v;
  RETURN_BOOL(upb_map_get(intern->map, k, &v));
}

/**
 * Return the element at the given key.
 * This will also be called for: $ele = $map[$key]
 * @param mixed The key of the element to be fetched.
 * @return object The stored element at given key.
 * @exception Invalid type for key.
 * @exception Non-existing key.
 */
PHP_METHOD(MapField, offsetGet) {
  zval *key;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &key) ==
      FAILURE) {
    return;
  }

  MapField *intern = UNBOX(MapField, getThis());
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map));
  upb_msgval v;
  if(!upb_map_get(intern->map, k, &v)) {
    RETURN_NULL();
  } else {
    tophpval(v, upb_map_valuetype(intern->map),
             static_cast<zend_class_entry*>(intern->klass), return_value);
    return;
  }
}

/**
 * Assign the element at the given key.
 * This will also be called for: $map[$key] = $ele
 * @param mixed The key of the element to be assigned.
 * @param object The element to be assigned.
 * @exception Invalid type for key.
 * @exception Non-existing key.
 * @exception Incorrect type of the element.
 */
PHP_METHOD(MapField, offsetSet) {
  zval *key, *value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &key, &value) ==
      FAILURE) {
    return;
  }

  MapField *intern = UNBOX(MapField, getThis());
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map));
  upb_msgval v = tomsgval(value, upb_map_valuetype(intern->map));
  upb_map_set(intern->map, k, v, NULL);
}

/**
 * Remove the element at the given key.
 * This will also be called for: unset($key)
 * @param mixed The key of the element to be removed.
 * @exception Invalid type for key.
 * @exception Key does not exist.
 */
PHP_METHOD(MapField, offsetUnset) {
  zval *key;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &key) ==
      FAILURE) {
    return;
  }
}

/**
 * Return the number of stored elements.
 * This will also be called for: count($arr)
 * @return long The number of stored elements.
 */
PHP_METHOD(MapField, count) {
  MapField *intern = UNBOX(MapField, getThis());

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  RETURN_LONG(upb_map_size(intern->map));
}

/**
 * Return the beginning iterator.
 * This will also be called for: foreach($arr)
 * @return object Beginning iterator.
 */
PHP_METHOD(MapField, getIterator) {
//   CREATE_OBJ_ON_ALLOCATED_ZVAL_PTR(return_value,
//                                    repeated_field_iter_type);
// 
//   RepeatedField *intern = UNBOX(RepeatedField, getThis());
//   RepeatedFieldIter *iter = UNBOX(RepeatedFieldIter, return_value);
//   iter->repeated_field = intern;
//   iter->position = 0;
}
