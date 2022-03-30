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

// -----------------------------------------------------------------------------
// Ruby <-> upb data conversion functions.
//
// This file Also contains a few other assorted algorithms on upb_MessageValue.
//
// None of the algorithms in this file require any access to the internal
// representation of Ruby or upb objects.
// -----------------------------------------------------------------------------

#include "convert.h"

#include "message.h"
#include "protobuf.h"

static upb_StringView Convert_StringData(VALUE str, upb_Arena* arena) {
  upb_StringView ret;
  if (arena) {
    char* ptr = upb_Arena_Malloc(arena, RSTRING_LEN(str));
    memcpy(ptr, RSTRING_PTR(str), RSTRING_LEN(str));
    ret.data = ptr;
  } else {
    // Data is only needed temporarily (within map lookup).
    ret.data = RSTRING_PTR(str);
  }
  ret.size = RSTRING_LEN(str);
  return ret;
}

static bool is_ruby_num(VALUE value) {
  return (TYPE(value) == T_FLOAT || TYPE(value) == T_FIXNUM ||
          TYPE(value) == T_BIGNUM);
}

static void Convert_CheckInt(const char* name, upb_CType type, VALUE val) {
  if (!is_ruby_num(val)) {
    rb_raise(cTypeError,
             "Expected number type for integral field '%s' (given %s).", name,
             rb_class2name(CLASS_OF(val)));
  }

  // NUM2{INT,UINT,LL,ULL} macros do the appropriate range checks on upper
  // bound; we just need to do precision checks (i.e., disallow rounding) and
  // check for < 0 on unsigned types.
  if (TYPE(val) == T_FLOAT) {
    double dbl_val = NUM2DBL(val);
    if (floor(dbl_val) != dbl_val) {
      rb_raise(rb_eRangeError,
               "Non-integral floating point value assigned to integer field "
               "'%s' (given %s).",
               name, rb_class2name(CLASS_OF(val)));
    }
  }
  if (type == kUpb_CType_UInt32 || type == kUpb_CType_UInt64) {
    if (NUM2DBL(val) < 0) {
      rb_raise(
          rb_eRangeError,
          "Assigning negative value to unsigned integer field '%s' (given %s).",
          name, rb_class2name(CLASS_OF(val)));
    }
  }
}

static int32_t Convert_ToEnum(VALUE value, const char* name,
                              const upb_EnumDef* e) {
  int32_t val;

  switch (TYPE(value)) {
    case T_FLOAT:
    case T_FIXNUM:
    case T_BIGNUM:
      Convert_CheckInt(name, kUpb_CType_Int32, value);
      val = NUM2INT(value);
      break;
    case T_STRING: {
      const upb_EnumValueDef* ev = upb_EnumDef_FindValueByNameWithSize(
          e, RSTRING_PTR(value), RSTRING_LEN(value));
      if (!ev) goto unknownval;
      val = upb_EnumValueDef_Number(ev);
      break;
    }
    case T_SYMBOL: {
      const upb_EnumValueDef* ev =
          upb_EnumDef_FindValueByName(e, rb_id2name(SYM2ID(value)));
      if (!ev)
        goto unknownval;
      val = upb_EnumValueDef_Number(ev);
      break;
    }
    default:
      rb_raise(cTypeError,
               "Expected number or symbol type for enum field '%s'.", name);
  }

  return val;

unknownval:
  rb_raise(rb_eRangeError, "Unknown symbol value for enum field '%s'.", name);
}

upb_MessageValue Convert_RubyToUpb(VALUE value, const char* name,
                                   TypeInfo type_info, upb_Arena* arena) {
  upb_MessageValue ret;

  switch (type_info.type) {
    case kUpb_CType_Float:
      if (!is_ruby_num(value)) {
        rb_raise(cTypeError,
                 "Expected number type for float field '%s' (given %s).", name,
                 rb_class2name(CLASS_OF(value)));
      }
      ret.float_val = NUM2DBL(value);
      break;
    case kUpb_CType_Double:
      if (!is_ruby_num(value)) {
        rb_raise(cTypeError,
                 "Expected number type for double field '%s' (given %s).", name,
                 rb_class2name(CLASS_OF(value)));
      }
      ret.double_val = NUM2DBL(value);
      break;
    case kUpb_CType_Bool: {
      if (value == Qtrue) {
        ret.bool_val = 1;
      } else if (value == Qfalse) {
        ret.bool_val = 0;
      } else {
        rb_raise(cTypeError,
                 "Invalid argument for boolean field '%s' (given %s).", name,
                 rb_class2name(CLASS_OF(value)));
      }
      break;
    }
    case kUpb_CType_String: {
      VALUE utf8 = rb_enc_from_encoding(rb_utf8_encoding());
      if (rb_obj_class(value) == rb_cSymbol) {
        value = rb_funcall(value, rb_intern("to_s"), 0);
      } else if (rb_obj_class(value) != rb_cString) {
        rb_raise(cTypeError,
                 "Invalid argument for string field '%s' (given %s).", name,
                 rb_class2name(CLASS_OF(value)));
      }

      if (rb_obj_encoding(value) != utf8) {
        // Note: this will not duplicate underlying string data unless
        // necessary.
        value = rb_str_encode(value, utf8, 0, Qnil);

        if (rb_enc_str_coderange(value) == ENC_CODERANGE_BROKEN) {
          rb_raise(rb_eEncodingError, "String is invalid UTF-8");
        }
      }

      ret.str_val = Convert_StringData(value, arena);
      break;
    }
    case kUpb_CType_Bytes: {
      VALUE bytes = rb_enc_from_encoding(rb_ascii8bit_encoding());
      if (rb_obj_class(value) != rb_cString) {
        rb_raise(cTypeError,
                 "Invalid argument for bytes field '%s' (given %s).", name,
                 rb_class2name(CLASS_OF(value)));
      }

      if (rb_obj_encoding(value) != bytes) {
        // Note: this will not duplicate underlying string data unless
        // necessary.
        // TODO(haberman): is this really necessary to get raw bytes?
        value = rb_str_encode(value, bytes, 0, Qnil);
      }

      ret.str_val = Convert_StringData(value, arena);
      break;
    }
    case kUpb_CType_Message:
      ret.msg_val =
          Message_GetUpbMessage(value, type_info.def.msgdef, name, arena);
      break;
    case kUpb_CType_Enum:
      ret.int32_val = Convert_ToEnum(value, name, type_info.def.enumdef);
      break;
    case kUpb_CType_Int32:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt32:
    case kUpb_CType_UInt64:
      Convert_CheckInt(name, type_info.type, value);
      switch (type_info.type) {
        case kUpb_CType_Int32:
          ret.int32_val = NUM2INT(value);
          break;
        case kUpb_CType_Int64:
          ret.int64_val = NUM2LL(value);
          break;
        case kUpb_CType_UInt32:
          ret.uint32_val = NUM2UINT(value);
          break;
        case kUpb_CType_UInt64:
          ret.uint64_val = NUM2ULL(value);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  return ret;
}

VALUE Convert_UpbToRuby(upb_MessageValue upb_val, TypeInfo type_info,
                        VALUE arena) {
  switch (type_info.type) {
    case kUpb_CType_Float:
      return DBL2NUM(upb_val.float_val);
    case kUpb_CType_Double:
      return DBL2NUM(upb_val.double_val);
    case kUpb_CType_Bool:
      return upb_val.bool_val ? Qtrue : Qfalse;
    case kUpb_CType_Int32:
      return INT2NUM(upb_val.int32_val);
    case kUpb_CType_Int64:
      return LL2NUM(upb_val.int64_val);
    case kUpb_CType_UInt32:
      return UINT2NUM(upb_val.uint32_val);
    case kUpb_CType_UInt64:
      return ULL2NUM(upb_val.int64_val);
    case kUpb_CType_Enum: {
      const upb_EnumValueDef *ev = upb_EnumDef_FindValueByNumber(
          type_info.def.enumdef, upb_val.int32_val);
      if (ev) {
        return ID2SYM(rb_intern(upb_EnumValueDef_Name(ev)));
      } else {
        return INT2NUM(upb_val.int32_val);
      }
    }
    case kUpb_CType_String: {
      VALUE str_rb = rb_str_new(upb_val.str_val.data, upb_val.str_val.size);
      rb_enc_associate(str_rb, rb_utf8_encoding());
      rb_obj_freeze(str_rb);
      return str_rb;
    }
    case kUpb_CType_Bytes: {
      VALUE str_rb = rb_str_new(upb_val.str_val.data, upb_val.str_val.size);
      rb_enc_associate(str_rb, rb_ascii8bit_encoding());
      rb_obj_freeze(str_rb);
      return str_rb;
    }
    case kUpb_CType_Message:
      return Message_GetRubyWrapper((upb_Message*)upb_val.msg_val,
                                    type_info.def.msgdef, arena);
    default:
      rb_raise(rb_eRuntimeError, "Convert_UpbToRuby(): Unexpected type %d",
               (int)type_info.type);
  }
}

upb_MessageValue Msgval_DeepCopy(upb_MessageValue msgval, TypeInfo type_info,
                                 upb_Arena* arena) {
  upb_MessageValue new_msgval;

  switch (type_info.type) {
    default:
      memcpy(&new_msgval, &msgval, sizeof(msgval));
      break;
    case kUpb_CType_String:
    case kUpb_CType_Bytes: {
      size_t n = msgval.str_val.size;
      char* mem = upb_Arena_Malloc(arena, n);
      new_msgval.str_val.data = mem;
      new_msgval.str_val.size = n;
      memcpy(mem, msgval.str_val.data, n);
      break;
    }
    case kUpb_CType_Message:
      new_msgval.msg_val =
          Message_deep_copy(msgval.msg_val, type_info.def.msgdef, arena);
      break;
  }

  return new_msgval;
}

bool Msgval_IsEqual(upb_MessageValue val1, upb_MessageValue val2,
                    TypeInfo type_info) {
  switch (type_info.type) {
    case kUpb_CType_Bool:
      return memcmp(&val1, &val2, 1) == 0;
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return memcmp(&val1, &val2, 4) == 0;
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return memcmp(&val1, &val2, 8) == 0;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return val1.str_val.size == val2.str_val.size &&
             memcmp(val1.str_val.data, val2.str_val.data, val1.str_val.size) ==
                 0;
    case kUpb_CType_Message:
      return Message_Equal(val1.msg_val, val2.msg_val, type_info.def.msgdef);
    default:
      rb_raise(rb_eRuntimeError, "Internal error, unexpected type");
  }
}

uint64_t Msgval_GetHash(upb_MessageValue val, TypeInfo type_info,
                        uint64_t seed) {
  switch (type_info.type) {
    case kUpb_CType_Bool:
      return _upb_Hash(&val, 1, seed);
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return _upb_Hash(&val, 4, seed);
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return _upb_Hash(&val, 8, seed);
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return _upb_Hash(val.str_val.data, val.str_val.size, seed);
    case kUpb_CType_Message:
      return Message_Hash(val.msg_val, type_info.def.msgdef, seed);
    default:
      rb_raise(rb_eRuntimeError, "Internal error, unexpected type");
  }
}
