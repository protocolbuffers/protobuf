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

#include "convert.h"

#include <php.h>

// This is not self-contained: it must be after other Zend includes.
#include <Zend/zend_exceptions.h>

#include "array.h"
#include "map.h"
#include "message.h"
#include "php-upb.h"
#include "protobuf.h"

// -----------------------------------------------------------------------------
// GPBUtil
// -----------------------------------------------------------------------------

static zend_class_entry* GPBUtil_class_entry;

// The implementation of type checking for primitive fields is empty. This is
// because type checking is done when direct assigning message fields (e.g.,
// foo->a = 1). Functions defined here are place holders in generated code for
// pure PHP implementation (c extension and pure PHP share the same generated
// code).

PHP_METHOD(Util, checkInt32) {}
PHP_METHOD(Util, checkUint32) {}
PHP_METHOD(Util, checkInt64) {}
PHP_METHOD(Util, checkUint64) {}
PHP_METHOD(Util, checkEnum) {}
PHP_METHOD(Util, checkFloat) {}
PHP_METHOD(Util, checkDouble) {}
PHP_METHOD(Util, checkBool) {}
PHP_METHOD(Util, checkString) {}
PHP_METHOD(Util, checkBytes) {}
PHP_METHOD(Util, checkMessage) {}

// The result of checkMapField() is assigned, so we need to return the first
// param:
//   $arr = GPBUtil::checkMapField($var,
//                                 \Google\Protobuf\Internal\GPBType::INT64,
//                                 \Google\Protobuf\Internal\GPBType::INT32);
PHP_METHOD(Util, checkMapField) {
  zval *val, *key_type, *val_type, *klass;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zzz|z", &val, &key_type,
                            &val_type, &klass) == FAILURE) {
    return;
  }
  RETURN_COPY(val);
}

// The result of checkRepeatedField() is assigned, so we need to return the
// first param:
// $arr = GPBUtil::checkRepeatedField(
//     $var, \Google\Protobuf\Internal\GPBType::STRING);
PHP_METHOD(Util, checkRepeatedField) {
  zval *val, *type, *klass;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz|z", &val, &type, &klass) ==
      FAILURE) {
    return;
  }
  RETURN_COPY(val);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_checkPrimitive, 0, 0, 1)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_checkString, 0, 0, 1)
  ZEND_ARG_INFO(0, value)
  ZEND_ARG_INFO(0, check_utf8)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_checkMessage, 0, 0, 2)
  ZEND_ARG_INFO(0, value)
  ZEND_ARG_INFO(0, class)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_checkMapField, 0, 0, 3)
  ZEND_ARG_INFO(0, value)
  ZEND_ARG_INFO(0, key_type)
  ZEND_ARG_INFO(0, value_type)
  ZEND_ARG_INFO(0, value_class)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_checkRepeatedField, 0, 0, 2)
  ZEND_ARG_INFO(0, value)
  ZEND_ARG_INFO(0, type)
  ZEND_ARG_INFO(0, class)
ZEND_END_ARG_INFO()

static zend_function_entry util_methods[] = {
  PHP_ME(Util, checkInt32,  arginfo_checkPrimitive,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkUint32, arginfo_checkPrimitive,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkInt64,  arginfo_checkPrimitive,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkUint64, arginfo_checkPrimitive,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkEnum,   arginfo_checkMessage,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkFloat,  arginfo_checkPrimitive,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkDouble, arginfo_checkPrimitive,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkBool,   arginfo_checkPrimitive,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkString, arginfo_checkString,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkBytes,  arginfo_checkPrimitive,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkMessage, arginfo_checkMessage,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkMapField, arginfo_checkMapField,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Util, checkRepeatedField, arginfo_checkRepeatedField,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// Conversion functions used from C
// -----------------------------------------------------------------------------

upb_CType pbphp_dtype_to_type(upb_FieldType type) {
  switch (type) {
#define CASE(descriptor_type, type)           \
  case kUpb_FieldType_##descriptor_type: \
    return kUpb_CType_##type;

  CASE(Float,    Float);
  CASE(Double,   Double);
  CASE(Bool,     Bool);
  CASE(String,   String);
  CASE(Bytes,    Bytes);
  CASE(Message,  Message);
  CASE(Group,    Message);
  CASE(Enum,     Enum);
  CASE(Int32,    Int32);
  CASE(Int64,    Int64);
  CASE(UInt32,   Int32);
  CASE(UInt64,   UInt64);
  CASE(SInt32,   Int32);
  CASE(SInt64,   Int64);
  CASE(Fixed32,  UInt32);
  CASE(Fixed64,  UInt64);
  CASE(SFixed32, Int32);
  CASE(SFixed64, Int64);

#undef CASE

  }

  zend_error(E_ERROR, "Unknown field type.");
  return 0;
}

static bool buftouint64(const char *ptr, const char *end, uint64_t *val) {
  uint64_t u64 = 0;
  while (ptr < end) {
    unsigned ch = (unsigned)(*ptr - '0');
    if (ch >= 10) break;
    if (u64 > UINT64_MAX / 10 || u64 * 10 > UINT64_MAX - ch) {
      return false;
    }
    u64 *= 10;
    u64 += ch;
    ptr++;
  }

  if (ptr != end) {
    // In PHP tradition, we allow truncation: "1.1" -> 1.
    // But we don't allow 'e', eg. '1.1e2' or any other non-numeric chars.
    if (*ptr++ != '.') return false;

    for (;ptr < end; ptr++) {
      if (*ptr < '0' || *ptr > '9') {
        return false;
      }
    }
  }

  *val = u64;
  return true;
}

static bool buftoint64(const char *ptr, const char *end, int64_t *val) {
  bool neg = false;
  uint64_t u64;

  if (ptr != end && *ptr == '-') {
    ptr++;
    neg = true;
  }

  if (!buftouint64(ptr, end, &u64) ||
      u64 > (uint64_t)INT64_MAX + neg) {
    return false;
  }

  *val = neg ? -u64 : u64;
  return true;
}

static void throw_conversion_exception(const char *to, const zval *zv) {
  zval tmp;
  ZVAL_COPY(&tmp, zv);
  convert_to_string(&tmp);

  zend_throw_exception_ex(NULL, 0, "Cannot convert '%s' to %s",
                          Z_STRVAL_P(&tmp), to);

  zval_ptr_dtor(&tmp);
}

bool Convert_PhpToInt64(const zval *php_val, int64_t *i64) {
  switch (Z_TYPE_P(php_val)) {
    case IS_LONG:
      *i64 = Z_LVAL_P(php_val);
      return true;
    case IS_DOUBLE: {
      double dbl = Z_DVAL_P(php_val);
      if (dbl > 9223372036854774784.0 || dbl < -9223372036854775808.0) {
        zend_throw_exception_ex(NULL, 0, "Out of range");
        return false;
      }
      *i64 = dbl; /* must be guarded, overflow here is UB */
      return true;
    }
    case IS_STRING: {
      const char *buf = Z_STRVAL_P(php_val);
      // PHP would accept scientific notation here, but we're going to be a
      // little more discerning and only accept pure integers.
      bool ok = buftoint64(buf, buf + Z_STRLEN_P(php_val), i64);
      if (!ok) {
        throw_conversion_exception("integer", php_val);
      }
      return ok;
    }
    default:
      throw_conversion_exception("integer", php_val);
      return false;
  }
}

static bool to_double(zval *php_val, double *dbl) {
  switch (Z_TYPE_P(php_val)) {
    case IS_LONG:
      *dbl = Z_LVAL_P(php_val);
      return true;
    case IS_DOUBLE:
      *dbl = Z_DVAL_P(php_val);
      return true;
    case IS_STRING: {
      zend_long lval;
      switch (is_numeric_string(Z_STRVAL_P(php_val), Z_STRLEN_P(php_val), &lval,
                                dbl, false)) {
        case IS_LONG:
          *dbl = lval;
          return true;
        case IS_DOUBLE:
          return true;
        default:
          goto fail;
      }
    }
    default:
     fail:
      throw_conversion_exception("double", php_val);
      return false;
  }
}

static bool to_bool(zval* from, bool* to) {
  switch (Z_TYPE_P(from)) {
    case IS_TRUE:
      *to = true;
      return true;
    case IS_FALSE:
      *to = false;
      return true;
    case IS_LONG:
      *to = (Z_LVAL_P(from) != 0);
      return true;
    case IS_DOUBLE:
      *to = (Z_LVAL_P(from) != 0);
      return true;
    case IS_STRING:
      if (Z_STRLEN_P(from) == 0 ||
          (Z_STRLEN_P(from) == 1 && Z_STRVAL_P(from)[0] == '0')) {
        *to = false;
      } else {
        *to = true;
      }
      return true;
    default:
      throw_conversion_exception("bool", from);
      return false;
  }
}

static bool to_string(zval* from) {
  if (Z_ISREF_P(from)) {
    ZVAL_DEREF(from);
  }

  switch (Z_TYPE_P(from)) {
    case IS_STRING:
      return true;
    case IS_TRUE:
    case IS_FALSE:
    case IS_LONG:
    case IS_DOUBLE: {
      zval tmp;
      zend_make_printable_zval(from, &tmp);
      ZVAL_COPY_VALUE(from, &tmp);
      return true;
    }
    default:
      throw_conversion_exception("string", from);
      return false;
  }
}

bool Convert_PhpToUpb(zval *php_val, upb_MessageValue *upb_val, TypeInfo type,
                      upb_Arena *arena) {
  int64_t i64;

  if (Z_ISREF_P(php_val)) {
    ZVAL_DEREF(php_val);
  }

  switch (type.type) {
    case kUpb_CType_Int64:
      return Convert_PhpToInt64(php_val, &upb_val->int64_val);
    case kUpb_CType_Int32:
    case kUpb_CType_Enum:
      if (!Convert_PhpToInt64(php_val, &i64)) {
        return false;
      }
      upb_val->int32_val = i64;
      return true;
    case kUpb_CType_UInt64:
      if (!Convert_PhpToInt64(php_val, &i64)) {
        return false;
      }
      upb_val->uint64_val = i64;
      return true;
    case kUpb_CType_UInt32:
      if (!Convert_PhpToInt64(php_val, &i64)) {
        return false;
      }
      upb_val->uint32_val = i64;
      return true;
    case kUpb_CType_Double:
      return to_double(php_val, &upb_val->double_val);
    case kUpb_CType_Float:
      if (!to_double(php_val, &upb_val->double_val)) return false;
      upb_val->float_val = upb_val->double_val;
      return true;
    case kUpb_CType_Bool:
      return to_bool(php_val, &upb_val->bool_val);
    case kUpb_CType_String:
    case kUpb_CType_Bytes: {
      char *ptr;
      size_t size;

      if (!to_string(php_val)) return false;

      size = Z_STRLEN_P(php_val);

      // If arena is NULL we reference the input zval.
      // The resulting upb_StringView will only be value while the zval is alive.
      if (arena) {
        ptr = upb_Arena_Malloc(arena, size);
        memcpy(ptr, Z_STRVAL_P(php_val), size);
      } else {
        ptr = Z_STRVAL_P(php_val);
      }

      upb_val->str_val = upb_StringView_FromDataAndSize(ptr, size);
      return true;
    }
    case kUpb_CType_Message:
      PBPHP_ASSERT(type.desc);
      return Message_GetUpbMessage(php_val, type.desc, arena,
                                   (upb_Message **)&upb_val->msg_val);
  }

  return false;
}

void Convert_UpbToPhp(upb_MessageValue upb_val, zval *php_val, TypeInfo type,
                      zval *arena) {
  switch (type.type) {
    case kUpb_CType_Int64:
#if SIZEOF_ZEND_LONG == 8
      ZVAL_LONG(php_val, upb_val.int64_val);
#else
      {
        char buf[20];
        int size = sprintf(buf, "%lld", upb_val.int64_val);
        ZVAL_NEW_STR(php_val, zend_string_init(buf, size, 0));
      }
#endif
      break;
    case kUpb_CType_UInt64:
#if SIZEOF_ZEND_LONG == 8
      ZVAL_LONG(php_val, upb_val.uint64_val);
#else
      {
        char buf[20];
        int size = sprintf(buf, "%lld", (int64_t)upb_val.uint64_val);
        ZVAL_NEW_STR(php_val, zend_string_init(buf, size, 0));
      }
#endif
      break;
    case kUpb_CType_Int32:
    case kUpb_CType_Enum:
      ZVAL_LONG(php_val, upb_val.int32_val);
      break;
    case kUpb_CType_UInt32: {
      // Sign-extend for consistency between 32/64-bit builds.
      zend_long val = (int32_t)upb_val.uint32_val;
      ZVAL_LONG(php_val, val);
      break;
    }
    case kUpb_CType_Double:
      ZVAL_DOUBLE(php_val, upb_val.double_val);
      break;
    case kUpb_CType_Float:
      ZVAL_DOUBLE(php_val, upb_val.float_val);
      break;
    case kUpb_CType_Bool:
      ZVAL_BOOL(php_val, upb_val.bool_val);
      break;
    case kUpb_CType_String:
    case kUpb_CType_Bytes: {
      upb_StringView str = upb_val.str_val;
      ZVAL_NEW_STR(php_val, zend_string_init(str.data, str.size, 0));
      break;
    }
    case kUpb_CType_Message:
      PBPHP_ASSERT(type.desc);
      Message_GetPhpWrapper(php_val, type.desc, (upb_Message *)upb_val.msg_val,
                            arena);
      break;
  }
}

// Check if the field is a well known wrapper type
static bool IsWrapper(const upb_MessageDef* m) {
  if (!m) return false;
  switch (upb_MessageDef_WellKnownType(m)) {
    case kUpb_WellKnown_DoubleValue:
    case kUpb_WellKnown_FloatValue:
    case kUpb_WellKnown_Int64Value:
    case kUpb_WellKnown_UInt64Value:
    case kUpb_WellKnown_Int32Value:
    case kUpb_WellKnown_UInt32Value:
    case kUpb_WellKnown_StringValue:
    case kUpb_WellKnown_BytesValue:
    case kUpb_WellKnown_BoolValue:
      return true;
    default:
      return false;
  }
}

bool Convert_PhpToUpbAutoWrap(zval *val, upb_MessageValue *upb_val, TypeInfo type,
                              upb_Arena *arena) {
  const upb_MessageDef *subm = type.desc ? type.desc->msgdef : NULL;
  if (subm && IsWrapper(subm) && Z_TYPE_P(val) != IS_OBJECT) {
    // Assigning a scalar to a wrapper-typed value. We will automatically wrap
    // the value, so the user doesn't need to create a FooWrapper(['value': X])
    // message manually.
    upb_Message *wrapper = upb_Message_New(subm, arena);
    const upb_FieldDef *val_f = upb_MessageDef_FindFieldByNumber(subm, 1);
    upb_MessageValue msgval;
    if (!Convert_PhpToUpb(val, &msgval, TypeInfo_Get(val_f), arena)) return false;
    upb_Message_Set(wrapper, val_f, msgval, arena);
    upb_val->msg_val = wrapper;
    return true;
  } else {
    // Convert_PhpToUpb doesn't auto-construct messages. This means that we only
    // allow:
    //   ['foo_submsg': new Foo(['a' => 1])]
    // not:
    //   ['foo_submsg': ['a' => 1]]
    return Convert_PhpToUpb(val, upb_val, type, arena);
  }
}

void Convert_ModuleInit(void) {
  const char *prefix_name = "TYPE_URL_PREFIX";
  zend_class_entry class_type;

  INIT_CLASS_ENTRY(class_type, "Google\\Protobuf\\Internal\\GPBUtil",
                   util_methods);
  GPBUtil_class_entry = zend_register_internal_class(&class_type);

  zend_declare_class_constant_string(GPBUtil_class_entry, prefix_name,
                                     strlen(prefix_name),
                                     "type.googleapis.com/");
}
