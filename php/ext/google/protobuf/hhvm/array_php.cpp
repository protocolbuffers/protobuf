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

static void RepeatedField_init_handlers(zend_object_handlers* handlers) {
}

static void RepeatedField_init_type(zend_class_entry* klass) {
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

PHP_METHOD(RepeatedField, __construct);
PHP_METHOD(RepeatedField, append);
PHP_METHOD(RepeatedField, offsetExists);
PHP_METHOD(RepeatedField, offsetGet);
PHP_METHOD(RepeatedField, offsetSet);
PHP_METHOD(RepeatedField, offsetUnset);
PHP_METHOD(RepeatedField, count);
PHP_METHOD(RepeatedField, getIterator);

static zend_function_entry RepeatedField_methods[] = {
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

PROTO_DEFINE_CLASS(RepeatedField,
                   "Google\\Protobuf\\Internal\\RepeatedField");

static void RepeatedFieldIter_init_handlers(zend_object_handlers* handlers) {
}

static void RepeatedFieldIter_init_type(zend_class_entry* klass) {
  TSRMLS_FETCH();
  zend_class_implements(klass TSRMLS_CC, 1, zend_ce_iterator);
}

PHP_METHOD(RepeatedFieldIter, rewind);
PHP_METHOD(RepeatedFieldIter, current);
PHP_METHOD(RepeatedFieldIter, key);
PHP_METHOD(RepeatedFieldIter, next);
PHP_METHOD(RepeatedFieldIter, valid);

static zend_function_entry RepeatedFieldIter_methods[] = {
  PHP_ME(RepeatedFieldIter, rewind,      arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedFieldIter, current,     arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedFieldIter, key,         arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedFieldIter, next,        arginfo_void, ZEND_ACC_PUBLIC)
  PHP_ME(RepeatedFieldIter, valid,       arginfo_void, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

PROTO_DEFINE_CLASS(RepeatedFieldIter,
                   "Google\\Protobuf\\Internal\\RepeatedFieldIter");


// -----------------------------------------------------------------------------
// Define PHP methods
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
  RepeatedField___construct(
      intern, static_cast<upb_descriptortype_t>(type), klass);
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

  RepeatedField *intern = UNBOX(RepeatedField, getThis());
  upb_msgval val = tomsgval(value, upb_array_type(intern->array), NULL);
  size_t size = upb_array_size(intern->array);
  upb_array_set(intern->array, size, val);
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
  RETURN_BOOL(index >= 0 && index < upb_array_size(intern->array));
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
  upb_msgval value = upb_array_get(intern->array, index);
  tophpval(value, upb_array_type(intern->array),
           static_cast<zend_class_entry*>(intern->klass), return_value);
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

  RepeatedField *intern = UNBOX(RepeatedField, getThis());
  upb_msgval val = tomsgval(value, upb_array_type(intern->array), NULL);
  if (!index || Z_TYPE_P(index) == IS_NULL) {
    size_t size = upb_array_size(intern->array);
    upb_array_set(intern->array, size, val);
  } else {
    upb_array_set(intern->array, Z_LVAL_P(index), val);
  }
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
      index != (upb_array_size(intern->array) - 1)) {
    zend_error(E_USER_ERROR, "Cannot remove element at %ld.\n", index);
    return;
  }
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

  RETURN_LONG(upb_array_size(intern->array));
}

/**
 * Return the beginning iterator.
 * This will also be called for: foreach($arr)
 * @return object Beginning iterator.
 */
PHP_METHOD(RepeatedField, getIterator) {
  ZVAL_OBJ(return_value, RepeatedFieldIter_type->create_object(
      RepeatedFieldIter_type TSRMLS_CC));
  RepeatedField *intern = UNBOX(RepeatedField, getThis());
  RepeatedFieldIter *iter = UNBOX(RepeatedFieldIter, return_value);
  iter->repeated_field = intern;
  iter->position = 0;
}

// -----------------------------------------------------------------------------
// RepeatedFieldIter
// -----------------------------------------------------------------------------

PHP_METHOD(RepeatedFieldIter, rewind) {
  RepeatedFieldIter *intern = UNBOX(RepeatedFieldIter, getThis());
  intern->position = 0;
}

PHP_METHOD(RepeatedFieldIter, current) {
  RepeatedFieldIter *intern = UNBOX(RepeatedFieldIter, getThis());

  upb_msgval value = upb_array_get(
      intern->repeated_field->array, intern->position);
  tophpval(value, upb_array_type(intern->repeated_field->array),
           static_cast<zend_class_entry*>(intern->repeated_field->klass),
           return_value);
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
  RETURN_BOOL(upb_array_size(intern->repeated_field->array) > intern->position);
}

// -----------------------------------------------------------------------------
// GPBType
// -----------------------------------------------------------------------------

zend_class_entry* Type_type;

static zend_function_entry Type_methods[] = {
  ZEND_FE_END
};

void Type_init(TSRMLS_D) {
  zend_class_entry class_type;
  INIT_CLASS_ENTRY(class_type, "Google\\Protobuf\\Internal\\GPBType",
                   Type_methods);
  Type_type = zend_register_internal_class(&class_type TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("DOUBLE"),  1 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("FLOAT"),   2 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("INT64"),   3 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("UINT64"),  4 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("INT32"),   5 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("FIXED64"), 6 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("FIXED32"), 7 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("BOOL"),    8 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("STRING"),  9 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("GROUP"),   10 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("MESSAGE"), 11 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("BYTES"),   12 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("UINT32"),  13 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("ENUM"),    14 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("SFIXED32"),
                                   15 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("SFIXED64"),
                                   16 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("SINT32"), 17 TSRMLS_CC);
  zend_declare_class_constant_long(Type_type, STR("SINT64"), 18 TSRMLS_CC);
}
