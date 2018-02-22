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

#include "protobuf_php.h"

PHP_METHOD(Util, checkInt32);
PHP_METHOD(Util, checkUint32);
PHP_METHOD(Util, checkInt64);
PHP_METHOD(Util, checkUint64);
PHP_METHOD(Util, checkEnum);
PHP_METHOD(Util, checkFloat);
PHP_METHOD(Util, checkDouble);
PHP_METHOD(Util, checkBool);
PHP_METHOD(Util, checkString);
PHP_METHOD(Util, checkBytes);
PHP_METHOD(Util, checkMessage);
PHP_METHOD(Util, checkMapField);
PHP_METHOD(Util, checkRepeatedField);

PROTO_REGISTER_CLASS_METHODS_START(Util)
  PHP_ME(Util, checkInt32,  NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkUint32, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkInt64,  NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkUint64, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkEnum,   NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkFloat,  NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkDouble, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkBool,   NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkString, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkBytes,  NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkMessage, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkMapField, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkRepeatedField, NULL,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
PROTO_REGISTER_CLASS_METHODS_END

static zend_class_entry* util_type;

void Util_init(TSRMLS_D) {
  zend_class_entry class_type;
  INIT_CLASS_ENTRY(class_type, "Google\\Protobuf\\Internal\\GPBUtil",
                   Util_methods);
  util_type = zend_register_internal_class(&class_type TSRMLS_CC);
}

// -----------------------------------------------------------------------------
// PHP Functions.
// -----------------------------------------------------------------------------

// The implementation of type checking for primitive fields is empty. This is
// because type checking is done when direct assigning message fields (e.g.,
// foo->a = 1). Functions defined here are place holders in generated code for
// pure PHP implementation (c extension and pure PHP share the same generated
// code).
#define PHP_TYPE_CHECK(type) \
  PHP_METHOD(Util, check##type) {}

PHP_TYPE_CHECK(Int32)
PHP_TYPE_CHECK(Uint32)
PHP_TYPE_CHECK(Int64)
PHP_TYPE_CHECK(Uint64)
PHP_TYPE_CHECK(Enum)
PHP_TYPE_CHECK(Float)
PHP_TYPE_CHECK(Double)
PHP_TYPE_CHECK(Bool)
PHP_TYPE_CHECK(String)
PHP_TYPE_CHECK(Bytes)

#undef PHP_TYPE_CHECK

PHP_METHOD(Util, checkMessage) {
//    zval* val;
//    zend_class_entry* klass = NULL;
//    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o!C", &val, &klass) ==
//        FAILURE) {
//      return;
//    }
//    if (val == NULL) {
//      RETURN_NULL();
//    }
//    if (!instanceof_function(Z_OBJCE_P(val), klass TSRMLS_CC)) {
//      zend_error(E_USER_ERROR, "Given value is not an instance of %s.",
//                 klass->name);
//      return;
//    }
//    RETURN_ZVAL(val, 1, 0);
}
 
//  void check_repeated_field(const zend_class_entry* klass, PHP_PROTO_LONG type,
//                            zval* val, zval* return_value) {
//  #if PHP_MAJOR_VERSION >= 7
//    if (Z_ISREF_P(val)) {
//      ZVAL_DEREF(val);
//    }
//  #endif
//  
//    TSRMLS_FETCH();
//    if (Z_TYPE_P(val) == IS_ARRAY) {
//      HashTable* table = HASH_OF(val);
//      HashPosition pointer;
//      void* memory;
//  
//  #if PHP_MAJOR_VERSION < 7
//      zval* repeated_field;
//      MAKE_STD_ZVAL(repeated_field);
//  #else
//      zval repeated_field;
//  #endif
//  
//      repeated_field_create_with_type(repeated_field_type, to_fieldtype(type),
//                                      klass, &repeated_field TSRMLS_CC);
//  
//      for (zend_hash_internal_pointer_reset_ex(table, &pointer);
//           php_proto_zend_hash_get_current_data_ex(table, (void**)&memory,
//                                                   &pointer) == SUCCESS;
//           zend_hash_move_forward_ex(table, &pointer)) {
//        repeated_field_handlers->write_dimension(
//            CACHED_TO_ZVAL_PTR(repeated_field), NULL,
//            CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory) TSRMLS_CC);
//      }
//  
//      Z_DELREF_P(CACHED_TO_ZVAL_PTR(repeated_field));
//      RETURN_ZVAL(CACHED_TO_ZVAL_PTR(repeated_field), 1, 0);
//  
//    } else if (Z_TYPE_P(val) == IS_OBJECT) {
//      if (!instanceof_function(Z_OBJCE_P(val), repeated_field_type TSRMLS_CC)) {
//        zend_error(E_USER_ERROR, "Given value is not an instance of %s.",
//                   repeated_field_type->name);
//        return;
//      }
//      RepeatedField* intern = UNBOX(RepeatedField, val);
//      if (to_fieldtype(type) != intern->type) {
//        zend_error(E_USER_ERROR, "Incorrect repeated field type.");
//        return;
//      }
//      if (klass != NULL && intern->msg_ce != klass) {
//        zend_error(E_USER_ERROR,
//                   "Expect a repeated field of %s, but %s is given.", klass->name,
//                   intern->msg_ce->name);
//        return;
//      }
//      RETURN_ZVAL(val, 1, 0);
//    } else {
//      zend_error(E_USER_ERROR, "Incorrect repeated field type.");
//      return;
//    }
//  }
 
PHP_METHOD(Util, checkRepeatedField) {
  zval* val;
  PROTO_SIZE type;
  const zend_class_entry* klass = NULL;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zl|C", &val, &type,
                            &klass) == FAILURE) {
    return;
  }
  RETURN_ZVAL(val, 1, 0);
}
 
//  void check_map_field(const zend_class_entry* klass, PHP_PROTO_LONG key_type,
//                       PHP_PROTO_LONG value_type, zval* val, zval* return_value) {
//  #if PHP_MAJOR_VERSION >= 7
//    if (Z_ISREF_P(val)) {
//      ZVAL_DEREF(val);
//    }
//  #endif
//  
//    TSRMLS_FETCH();
//    if (Z_TYPE_P(val) == IS_ARRAY) {
//      HashTable* table = Z_ARRVAL_P(val);
//      HashPosition pointer;
//      zval key;
//      void* value;
//  
//  #if PHP_MAJOR_VERSION < 7
//      zval* map_field;
//      MAKE_STD_ZVAL(map_field);
//  #else
//      zval map_field;
//  #endif
//  
//      map_field_create_with_type(map_field_type, to_fieldtype(key_type),
//                                 to_fieldtype(value_type), klass,
//                                 &map_field TSRMLS_CC);
//  
//      for (zend_hash_internal_pointer_reset_ex(table, &pointer);
//           php_proto_zend_hash_get_current_data_ex(table, (void**)&value,
//                                                   &pointer) == SUCCESS;
//           zend_hash_move_forward_ex(table, &pointer)) {
//        zend_hash_get_current_key_zval_ex(table, &key, &pointer);
//        map_field_handlers->write_dimension(
//            CACHED_TO_ZVAL_PTR(map_field), &key,
//            CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)value) TSRMLS_CC);
//      }
//  
//      Z_DELREF_P(CACHED_TO_ZVAL_PTR(map_field));
//      RETURN_ZVAL(CACHED_TO_ZVAL_PTR(map_field), 1, 0);
//    } else if (Z_TYPE_P(val) == IS_OBJECT) {
//      if (!instanceof_function(Z_OBJCE_P(val), map_field_type TSRMLS_CC)) {
//        zend_error(E_USER_ERROR, "Given value is not an instance of %s.",
//                   map_field_type->name);
//        return;
//      }
//      Map* intern = UNBOX(Map, val);
//      if (to_fieldtype(key_type) != intern->key_type) {
//        zend_error(E_USER_ERROR, "Incorrect map field key type.");
//        return;
//      }
//      if (to_fieldtype(value_type) != intern->value_type) {
//        zend_error(E_USER_ERROR, "Incorrect map field value type.");
//        return;
//      }
//      if (klass != NULL && intern->msg_ce != klass) {
//        zend_error(E_USER_ERROR, "Expect a map field of %s, but %s is given.",
//                   klass->name, intern->msg_ce->name);
//        return;
//      }
//      RETURN_ZVAL(val, 1, 0);
//    } else {
//      zend_error(E_USER_ERROR, "Incorrect map field type.");
//      return;
//    }
//  }
 
PHP_METHOD(Util, checkMapField) {
  zval* val;
  PROTO_SIZE key_type, value_type;
  const zend_class_entry* klass = NULL;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zll|C", &val, &key_type,
                            &value_type, &klass) == FAILURE) {
    return;
  }
  RETURN_ZVAL(val, 1, 0);
}
