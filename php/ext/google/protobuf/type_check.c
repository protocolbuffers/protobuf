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
static const char int64_min_digits[] = "9223372036854775808";

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

ZEND_BEGIN_ARG_INFO_EX(arg_check_map, 0, 0, 3)
  ZEND_ARG_INFO(1, val)
  ZEND_ARG_INFO(0, key_type)
  ZEND_ARG_INFO(0, value_type)
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
  PHP_ME(Util, checkMapField,    arg_check_map, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
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

// This is modified from is_numeric_string in zend_operators.h. The behavior of 
// this function is the same as is_numeric_string, except that this takes
// int64_t as input instead of long.
static zend_uchar convert_numeric_string(
    const char *str, int length, int64_t *lval, double *dval) {
  const char *ptr;
  int base = 10, digits = 0, dp_or_e = 0;
  double local_dval = 0.0;
  zend_uchar type;

  if (length == 0) {
    return IS_NULL;
  }

  while (*str == ' ' || *str == '\t' || *str == '\n' || 
         *str == '\r' || *str == '\v' || *str == '\f') {
    str++;
    length--;
  }
  ptr = str;

  if (*ptr == '-' || *ptr == '+') {
    ptr++;
  }

  if (ZEND_IS_DIGIT(*ptr)) {
    // Handle hex numbers
    // str is used instead of ptr to disallow signs and keep old behavior.
    if (length > 2 && *str == '0' && (str[1] == 'x' || str[1] == 'X')) {
      base = 16;
      ptr += 2;
    }

    // Skip any leading 0s.
    while (*ptr == '0') {
      ptr++;
    }

    // Count the number of digits. If a decimal point/exponent is found,
    // it's a double. Otherwise, if there's a dval or no need to check for
    // a full match, stop when there are too many digits for a int64 */
    for (type = IS_LONG;
        !(digits >= MAX_LENGTH_OF_INT64 && dval);
        digits++, ptr++) {
check_digits:
      if (ZEND_IS_DIGIT(*ptr) || (base == 16 && ZEND_IS_XDIGIT(*ptr))) {
        continue;
      } else if (base == 10) {
        if (*ptr == '.' && dp_or_e < 1) {
          goto process_double;
        } else if ((*ptr == 'e' || *ptr == 'E') && dp_or_e < 2) {
          const char *e = ptr + 1;

          if (*e == '-' || *e == '+') {
            ptr = e++;
          }
          if (ZEND_IS_DIGIT(*e)) {
            goto process_double;
          }
        }
      }
      break;
    }

    if (base == 10) {
      if (digits >= MAX_LENGTH_OF_INT64) {
        dp_or_e = -1;
        goto process_double;
      }
    } else if (!(digits < SIZEOF_INT64 * 2 ||
               (digits == SIZEOF_INT64 * 2 && ptr[-digits] <= '7'))) {
      if (dval) {
        local_dval = zend_hex_strtod(str, &ptr);
      }
      type = IS_DOUBLE;
    }
  } else if (*ptr == '.' && ZEND_IS_DIGIT(ptr[1])) {
process_double:
    type = IS_DOUBLE;

    // If there's a dval, do the conversion; else continue checking
    // the digits if we need to check for a full match.
    if (dval) {
      local_dval = zend_strtod(str, &ptr);
    } else if (dp_or_e != -1) {
      dp_or_e = (*ptr++ == '.') ? 1 : 2;
      goto check_digits;
    }
  } else {
    return IS_NULL;
  }
  if (ptr != str + length) {
    zend_error(E_NOTICE, "A non well formed numeric value encountered");
    return 0;
  }

  if (type == IS_LONG) {
    if (digits == MAX_LENGTH_OF_INT64 - 1) {
      int cmp = strcmp(&ptr[-digits], int64_min_digits);

      if (!(cmp < 0 || (cmp == 0 && *str == '-'))) {
        if (dval) {
          *dval = zend_strtod(str, NULL);
        }

	return IS_DOUBLE;
      }
    }
    if (lval) {
      *lval = strtoll(str, NULL, base);
    }
    return IS_LONG;
  } else {
    if (dval) {
      *dval = local_dval;
    }
    return IS_DOUBLE;
  }
}

#define CONVERT_TO_INTEGER(type)                                             \
  static bool convert_int64_to_##type(int64_t val, type##_t* type##_value) { \
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
    int64_t lval;                                                            \
    double dval;                                                             \
                                                                             \
    switch (convert_numeric_string(val, len, &lval, &dval)) {                \
      case IS_DOUBLE: {                                                      \
        return convert_double_to_##type(dval, type##_value);                 \
      }                                                                      \
      case IS_LONG: {                                                        \
        return convert_int64_to_##type(lval, type##_value);                  \
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
        return convert_int64_to_##type(Z_LVAL_P(from), to);                  \
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
  static bool convert_int64_to_##type(int64_t val, type* type##_value) {    \
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
    int64_t lval;                                                           \
    double dval;                                                            \
                                                                            \
    switch (convert_numeric_string(val, len, &lval, &dval)) {               \
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
        return convert_int64_to_##type(Z_LVAL_P(from), to);                 \
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
#if PHP_MAJOR_VERSION < 7
    case IS_BOOL:
      *to = (int8_t)Z_BVAL_P(from);
      break;
#else
    case IS_TRUE:
      *to = 1;
      break;
    case IS_FALSE:
      *to = 0;
      break;
#endif
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
#if PHP_MAJOR_VERSION < 7
    case IS_BOOL:
#else
    case IS_TRUE:
    case IS_FALSE:
#endif
    case IS_LONG:
    case IS_DOUBLE: {
      zval tmp;
      php_proto_zend_make_printable_zval(from, &tmp);
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

void check_repeated_field(const zend_class_entry* klass, PHP_PROTO_LONG type,
                          zval* val, zval* return_value) {
#if PHP_MAJOR_VERSION >= 7
  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }
#endif

  TSRMLS_FETCH();
  if (Z_TYPE_P(val) == IS_ARRAY) {
    HashTable* table = HASH_OF(val);
    HashPosition pointer;
    void* memory;

#if PHP_MAJOR_VERSION < 7
    zval* repeated_field;
    MAKE_STD_ZVAL(repeated_field);
#else
    zval repeated_field;
#endif

    repeated_field_create_with_type(repeated_field_type, to_fieldtype(type),
                                    klass, &repeated_field TSRMLS_CC);

    for (zend_hash_internal_pointer_reset_ex(table, &pointer);
         php_proto_zend_hash_get_current_data_ex(table, (void**)&memory,
                                                 &pointer) == SUCCESS;
         zend_hash_move_forward_ex(table, &pointer)) {
      repeated_field_handlers->write_dimension(
          CACHED_TO_ZVAL_PTR(repeated_field), NULL,
          CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory) TSRMLS_CC);
    }

    Z_DELREF_P(CACHED_TO_ZVAL_PTR(repeated_field));
    RETURN_ZVAL(CACHED_TO_ZVAL_PTR(repeated_field), 1, 0);

  } else if (Z_TYPE_P(val) == IS_OBJECT) {
    if (!instanceof_function(Z_OBJCE_P(val), repeated_field_type TSRMLS_CC)) {
      zend_error(E_USER_ERROR, "Given value is not an instance of %s.",
                 repeated_field_type->name);
      return;
    }
    RepeatedField* intern = UNBOX(RepeatedField, val);
    if (to_fieldtype(type) != intern->type) {
      zend_error(E_USER_ERROR, "Incorrect repeated field type.");
      return;
    }
    if (klass != NULL && intern->msg_ce != klass) {
      zend_error(E_USER_ERROR,
                 "Expect a repeated field of %s, but %s is given.", klass->name,
                 intern->msg_ce->name);
      return;
    }
    RETURN_ZVAL(val, 1, 0);
  } else {
    zend_error(E_USER_ERROR, "Incorrect repeated field type.");
    return;
  }
}

PHP_METHOD(Util, checkRepeatedField) {
  zval* val;
  PHP_PROTO_LONG type;
  const zend_class_entry* klass = NULL;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zl|C", &val, &type,
                            &klass) == FAILURE) {
    return;
  }
  RETURN_ZVAL(val, 1, 0);
}

void check_map_field(const zend_class_entry* klass, PHP_PROTO_LONG key_type,
                     PHP_PROTO_LONG value_type, zval* val, zval* return_value) {
#if PHP_MAJOR_VERSION >= 7
  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }
#endif

  TSRMLS_FETCH();
  if (Z_TYPE_P(val) == IS_ARRAY) {
    HashTable* table = Z_ARRVAL_P(val);
    HashPosition pointer;
    zval key;
    void* value;

#if PHP_MAJOR_VERSION < 7
    zval* map_field;
    MAKE_STD_ZVAL(map_field);
#else
    zval map_field;
#endif

    map_field_create_with_type(map_field_type, to_fieldtype(key_type),
                               to_fieldtype(value_type), klass,
                               &map_field TSRMLS_CC);

    for (zend_hash_internal_pointer_reset_ex(table, &pointer);
         php_proto_zend_hash_get_current_data_ex(table, (void**)&value,
                                                 &pointer) == SUCCESS;
         zend_hash_move_forward_ex(table, &pointer)) {
      zend_hash_get_current_key_zval_ex(table, &key, &pointer);
      map_field_handlers->write_dimension(
          CACHED_TO_ZVAL_PTR(map_field), &key,
          CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)value) TSRMLS_CC);
    }

    Z_DELREF_P(CACHED_TO_ZVAL_PTR(map_field));
    RETURN_ZVAL(CACHED_TO_ZVAL_PTR(map_field), 1, 0);
  } else if (Z_TYPE_P(val) == IS_OBJECT) {
    if (!instanceof_function(Z_OBJCE_P(val), map_field_type TSRMLS_CC)) {
      zend_error(E_USER_ERROR, "Given value is not an instance of %s.",
                 map_field_type->name);
      return;
    }
    Map* intern = UNBOX(Map, val);
    if (to_fieldtype(key_type) != intern->key_type) {
      zend_error(E_USER_ERROR, "Incorrect map field key type.");
      return;
    }
    if (to_fieldtype(value_type) != intern->value_type) {
      zend_error(E_USER_ERROR, "Incorrect map field value type.");
      return;
    }
    if (klass != NULL && intern->msg_ce != klass) {
      zend_error(E_USER_ERROR, "Expect a map field of %s, but %s is given.",
                 klass->name, intern->msg_ce->name);
      return;
    }
    RETURN_ZVAL(val, 1, 0);
  } else {
    zend_error(E_USER_ERROR, "Incorrect map field type.");
    return;
  }
}

PHP_METHOD(Util, checkMapField) {
  zval* val;
  PHP_PROTO_LONG key_type, value_type;
  const zend_class_entry* klass = NULL;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zll|C", &val, &key_type,
                            &value_type, &klass) == FAILURE) {
    return;
  }
  RETURN_ZVAL(val, 1, 0);
}
