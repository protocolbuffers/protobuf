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

#include <Zend/zend_operators.h>

#include "protobuf.h"
#include "utf8.h"

static zend_class_entry* util_type;

ZEND_BEGIN_ARG_INFO_EX(arg_check_optional, 0, 0, 1)
  ZEND_ARG_INFO(1, val)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arg_check_message, 0, 0, 2)
  ZEND_ARG_INFO(1, val)
  ZEND_ARG_INFO(0, klass)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arg_check_repeated, 0, 0, 2)
  ZEND_ARG_INFO(1, val)
  ZEND_ARG_INFO(0, type)
  ZEND_ARG_INFO(0, klass)
ZEND_END_ARG_INFO()

static zend_function_entry util_methods[] = {
  PHP_ME(Util, checkInt32,  arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkUint32, arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkInt64,  arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkUint64, arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkEnum,   arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkFloat,  arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkDouble, arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkBool,   arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkString, arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkBytes,  arg_check_optional, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkMessage, arg_check_message, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkRepeatedField, arg_check_repeated,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  ZEND_FE_END
};

void util_init(TSRMLS_D) {
  zend_class_entry class_type;
  INIT_CLASS_ENTRY(class_type, "Google\\Protobuf\\Internal\\GPBUtil",
                   util_methods);
  util_type = zend_register_internal_class(&class_type TSRMLS_CC);
}

// -----------------------------------------------------------------------------
// Type checking/conversion.
// -----------------------------------------------------------------------------

#define CONVERT_TO_INTEGER(type)                                             \
  static bool convert_long_to_##type(long val, type##_t* type##_value) {     \
    *type##_value = (type##_t)val;                                           \
    return true;                                                             \
  }                                                                          \
                                                                             \
  static bool convert_double_to_##type(double val, type##_t* type##_value) { \
    *type##_value = (type##_t)zend_dval_to_lval(val);                        \
    return true;                                                             \
  }                                                                          \
                                                                             \
  static bool convert_string_to_##type(const char* val, int len,             \
                                       type##_t* type##_value) {             \
    long lval;                                                               \
    double dval;                                                             \
                                                                             \
    switch (is_numeric_string(val, len, &lval, &dval, 0)) {                  \
      case IS_DOUBLE: {                                                      \
        return convert_double_to_##type(dval, type##_value);                 \
      }                                                                      \
      case IS_LONG: {                                                        \
        return convert_long_to_##type(lval, type##_value);                   \
      }                                                                      \
      default:                                                               \
        zend_error(E_USER_ERROR,                                             \
                   "Given string value cannot be converted to integer.");    \
        return false;                                                        \
    }                                                                        \
  }                                                                          \
                                                                             \
  bool protobuf_convert_to_##type(zval* from, type##_t* to) {                \
    switch (Z_TYPE_P(from)) {                                                \
      case IS_LONG: {                                                        \
        return convert_long_to_##type(Z_LVAL_P(from), to);                   \
      }                                                                      \
      case IS_DOUBLE: {                                                      \
        return convert_double_to_##type(Z_DVAL_P(from), to);                 \
      }                                                                      \
      case IS_STRING: {                                                      \
        return convert_string_to_##type(Z_STRVAL_P(from), Z_STRLEN_P(from),  \
                                        to);                                 \
      }                                                                      \
      default: {                                                             \
        zend_error(E_USER_ERROR,                                             \
                   "Given value cannot be converted to integer.");           \
        return false;                                                        \
      }                                                                      \
    }                                                                        \
    return false;                                                            \
  }

CONVERT_TO_INTEGER(int32);
CONVERT_TO_INTEGER(uint32);
CONVERT_TO_INTEGER(int64);
CONVERT_TO_INTEGER(uint64);

#undef CONVERT_TO_INTEGER

#define CONVERT_TO_FLOAT(type)                                              \
  static bool convert_long_to_##type(long val, type* type##_value) {        \
    *type##_value = (type)val;                                              \
    return true;                                                            \
  }                                                                         \
                                                                            \
  static bool convert_double_to_##type(double val, type* type##_value) {    \
    *type##_value = (type)val;                                              \
    return true;                                                            \
  }                                                                         \
                                                                            \
  static bool convert_string_to_##type(const char* val, int len,            \
                                       type* type##_value) {                \
    long lval;                                                              \
    double dval;                                                            \
                                                                            \
    switch (is_numeric_string(val, len, &lval, &dval, 0)) {                 \
      case IS_DOUBLE: {                                                     \
        *type##_value = (type)dval;                                         \
        return true;                                                        \
      }                                                                     \
      case IS_LONG: {                                                       \
        *type##_value = (type)lval;                                         \
        return true;                                                        \
      }                                                                     \
      default:                                                              \
        zend_error(E_USER_ERROR,                                            \
                   "Given string value cannot be converted to integer.");   \
        return false;                                                       \
    }                                                                       \
  }                                                                         \
                                                                            \
  bool protobuf_convert_to_##type(zval* from, type* to) {                   \
    switch (Z_TYPE_P(from)) {                                               \
      case IS_LONG: {                                                       \
        return convert_long_to_##type(Z_LVAL_P(from), to);                  \
      }                                                                     \
      case IS_DOUBLE: {                                                     \
        return convert_double_to_##type(Z_DVAL_P(from), to);                \
      }                                                                     \
      case IS_STRING: {                                                     \
        return convert_string_to_##type(Z_STRVAL_P(from), Z_STRLEN_P(from), \
                                        to);                                \
      }                                                                     \
      default: {                                                            \
        zend_error(E_USER_ERROR,                                            \
                   "Given value cannot be converted to integer.");          \
        return false;                                                       \
      }                                                                     \
    }                                                                       \
    return false;                                                           \
  }

CONVERT_TO_FLOAT(float);
CONVERT_TO_FLOAT(double);

#undef CONVERT_TO_FLOAT

bool protobuf_convert_to_bool(zval* from, int8_t* to) {
  switch (Z_TYPE_P(from)) {
    case IS_BOOL:
      *to = (int8_t)Z_BVAL_P(from);
      break;
    case IS_LONG:
      *to = (int8_t)(Z_LVAL_P(from) != 0);
      break;
    case IS_DOUBLE:
      *to = (int8_t)(Z_LVAL_P(from) != 0);
      break;
    case IS_STRING: {
      char* strval = Z_STRVAL_P(from);

      if (Z_STRLEN_P(from) == 0 ||
          (Z_STRLEN_P(from) == 1 && Z_STRVAL_P(from)[0] == '0')) {
        *to = 0;
      } else {
        *to = 1;
      }
      STR_FREE(strval);
    } break;
    default: {
      zend_error(E_USER_ERROR, "Given value cannot be converted to bool.");
      return false;
    }
  }
  return true;
}

bool protobuf_convert_to_string(zval* from) {
  switch (Z_TYPE_P(from)) {
    case IS_STRING: {
      return true;
    }
    case IS_BOOL:
    case IS_LONG:
    case IS_DOUBLE: {
      int use_copy;
      zval tmp;
      zend_make_printable_zval(from, &tmp, &use_copy);
      ZVAL_COPY_VALUE(from, &tmp);
      return true;
    }
    default:
      zend_error(E_USER_ERROR, "Given value cannot be converted to string.");
      return false;
  }
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
  zval* val;
  zend_class_entry* klass = NULL;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o!C", &val, &klass) ==
      FAILURE) {
    return;
  }
  if (val == NULL) {
    RETURN_NULL();
  }
  if (!instanceof_function(Z_OBJCE_P(val), klass TSRMLS_CC)) {
    zend_error(E_USER_ERROR, "Given value is not an instance of %s.",
               klass->name);
    return;
  }
  RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(Util, checkRepeatedField) {
  zval* val;
  long type;
  const zend_class_entry* klass = NULL;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ol|C", &val,
                            repeated_field_type, &type, &klass) == FAILURE) {
    return;
  }

  RepeatedField *intern =
      (RepeatedField *)zend_object_store_get_object(val TSRMLS_CC);
  if (to_fieldtype(type) != intern->type) {
    zend_error(E_USER_ERROR, "Incorrect repeated field type.");
    return;
  }
  if (klass != NULL && intern->msg_ce != klass) {
    zend_error(E_USER_ERROR, "Expect a repeated field of %s, but %s is given.",
               klass->name, intern->msg_ce->name);
    return;
  }
}
