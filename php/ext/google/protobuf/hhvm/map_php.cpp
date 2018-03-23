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

static void MapFieldIter_init_handlers(zend_object_handlers* handlers) {
}

static void MapFieldIter_init_type(zend_class_entry* klass) {
  TSRMLS_FETCH();
  zend_class_implements(klass TSRMLS_CC, 1, zend_ce_iterator);
}

PHP_METHOD(MapFieldIter, rewind);
PHP_METHOD(MapFieldIter, current);
PHP_METHOD(MapFieldIter, key);
PHP_METHOD(MapFieldIter, next);
PHP_METHOD(MapFieldIter, valid);

static zend_function_entry MapFieldIter_methods[] = {
  PHP_ME(MapFieldIter, rewind,      arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, current,     arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, key,         arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, next,        arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(MapFieldIter, valid,       arginfo_void, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

PROTO_DEFINE_CLASS(MapFieldIter,
                   "Google\\Protobuf\\Internal\\MapFieldIter");

// -----------------------------------------------------------------------------
// Define PHP methods
// -----------------------------------------------------------------------------

/**
 * Constructs an instance of MapField.
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
      static_cast<upb_descriptortype_t>(value_type), intern->arena, klass);
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
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map), NULL);
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
  Arena *cpparena = UNBOX_ARENA(intern->arena);
  upb_alloc *alloc = upb_arena_alloc(cpparena->arena);
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map), alloc);
  upb_msgval v;
  if(!upb_map_get(intern->map, k, &v)) {
    RETURN_NULL();
  } else {
    if (upb_map_valuetype(intern->map) == UPB_TYPE_MESSAGE) {
      RETURN_PHP_OBJECT((*(intern->wrappers))[(void*)upb_msgval_getmsg(v)]);
    } else {
      tophpval(v, upb_map_valuetype(intern->map),
               static_cast<zend_class_entry*>(intern->klass),
               return_value);
    }
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
  Arena *cpparena = UNBOX_ARENA(intern->arena);
  upb_alloc *alloc = upb_arena_alloc(cpparena->arena);
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map), alloc);
  upb_msgval v = tomsgval(value, upb_map_valuetype(intern->map), alloc);


  upb_msgval oldv = {0};
  upb_map_set(intern->map, k, v, &oldv);

  if (upb_map_valuetype(intern->map) == UPB_TYPE_MESSAGE) {
    const upb_msg *old_msg = upb_msgval_getmsg(oldv);
    if (old_msg != NULL) {
      PHP_OBJECT_DELREF((*(intern->wrappers))[(void*)old_msg]);
    }
    PHP_OBJECT cached = ZVAL_PTR_TO_PHP_OBJECT(value);
    PHP_OBJECT_ADDREF(cached);
    (*(intern->wrappers))[(void*)upb_msgval_getmsg(v)] = cached;
  }
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

  MapField *intern = UNBOX(MapField, getThis());
  upb_msgval k = tomsgval(key, upb_map_keytype(intern->map), NULL);
  upb_map_del(intern->map, k);
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
  ZVAL_OBJ(return_value, MapFieldIter_type->create_object(
      MapFieldIter_type TSRMLS_CC));
  MapField *intern = UNBOX(MapField, getThis());
  MapFieldIter *iter = UNBOX(MapFieldIter, return_value);
  iter->map_field = intern;
  iter->iter = upb_mapiter_new(intern->map, upb_map_getalloc(intern->map));
}

// -----------------------------------------------------------------------------
// MapFieldIter
// -----------------------------------------------------------------------------

PHP_METHOD(MapFieldIter, rewind) {
  MapFieldIter *intern = UNBOX(MapFieldIter, getThis());
  upb_alloc *a = upb_map_getalloc(intern->map_field->map);
  upb_mapiter_free(intern->iter, a);
  intern->iter = upb_mapiter_new(intern->map_field->map, a);
}

PHP_METHOD(MapFieldIter, current) {
  MapFieldIter *intern = UNBOX(MapFieldIter, getThis());

  upb_msgval value = upb_mapiter_value(intern->iter);
  tophpval(value, upb_map_valuetype(intern->map_field->map),
           static_cast<zend_class_entry*>(intern->map_field->klass),
           return_value);
}

PHP_METHOD(MapFieldIter, key) {
  MapFieldIter *intern = UNBOX(MapFieldIter, getThis());
  upb_msgval key = upb_mapiter_key(intern->iter);
  return tophpval(key, upb_map_keytype(intern->map_field->map),
                  static_cast<zend_class_entry*>(intern->map_field->klass),
                  return_value);
}

PHP_METHOD(MapFieldIter, next) {
  MapFieldIter *intern = UNBOX(MapFieldIter, getThis());
  upb_mapiter_next(intern->iter);
}

PHP_METHOD(MapFieldIter, valid) {
  MapFieldIter *intern = UNBOX(MapFieldIter, getThis());
  RETURN_BOOL(!upb_mapiter_done(intern->iter));
}
