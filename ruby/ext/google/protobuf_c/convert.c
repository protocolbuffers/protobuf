
#include "protobuf.h"

static upb_strview Convert_StrDup(VALUE str, upb_arena *arena) {
  upb_strview ret;
  char *ptr = upb_arena_malloc(arena, RSTRING_LEN(str));
  memcpy(ptr, RSTRING_PTR(str), RSTRING_LEN(str));
  ret.data = ptr;
  ret.size = RSTRING_LEN(str);
  return ret;
}

static bool is_ruby_num(VALUE value) {
  return (TYPE(value) == T_FLOAT ||
          TYPE(value) == T_FIXNUM ||
          TYPE(value) == T_BIGNUM);
}

static void Convert_CheckInt(const char* name, upb_fieldtype_t type,
                             VALUE val) {
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
  if (type == UPB_TYPE_UINT32 || type == UPB_TYPE_UINT64) {
    if (NUM2DBL(val) < 0) {
      rb_raise(
          rb_eRangeError,
          "Assigning negative value to unsigned integer field '%s' (given %s).",
          name, rb_class2name(CLASS_OF(val)));
    }
  }
}

static int32_t Convert_ToEnum(VALUE value, const char* name,
                              const upb_enumdef* e) {
  int32_t val;

  switch (TYPE(value)) {
    case T_FLOAT:
    case T_FIXNUM:
    case T_BIGNUM:
      Convert_CheckInt(name, UPB_TYPE_INT32, value);
      val = NUM2INT(value);
      break;
    case T_STRING:
      if (!upb_enumdef_ntoi(e, RSTRING_PTR(value), RSTRING_LEN(value), &val)) {
        goto unknownval;
      }
      break;
    case T_SYMBOL:
      if (!upb_enumdef_ntoiz(e, rb_id2name(SYM2ID(value)), &val)) {
        goto unknownval;
      }
      break;
    default:
      rb_raise(cTypeError,
               "Expected number or symbol type for enum field '%s'.", name);
  }

  return val;

unknownval:
  rb_raise(rb_eRangeError, "Unknown symbol value for enum field '%s'.", name);
}

upb_msgval Convert_RubyToUpb(VALUE value, const char* name,
                             upb_fieldtype_t type, TypeInfo info,
                             upb_arena* arena) {
  upb_msgval ret;

  switch (type) {
    case UPB_TYPE_FLOAT:
      if (!is_ruby_num(value)) {
        rb_raise(cTypeError, "Expected number type for float field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }
      ret.float_val = NUM2DBL(value);
      break;
    case UPB_TYPE_DOUBLE:
      if (!is_ruby_num(value)) {
        rb_raise(cTypeError, "Expected number type for double field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }
      ret.double_val = NUM2DBL(value);
      break;
    case UPB_TYPE_BOOL: {
      if (value == Qtrue) {
        ret.bool_val = 1;
      } else if (value == Qfalse) {
        ret.bool_val = 0;
      } else {
        rb_raise(cTypeError, "Invalid argument for boolean field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }
      break;
    }
    case UPB_TYPE_STRING: {
      VALUE utf8 = rb_enc_from_encoding(rb_utf8_encoding());
      if (CLASS_OF(value) == rb_cSymbol) {
        value = rb_funcall(value, rb_intern("to_s"), 0);
      } else if (CLASS_OF(value) != rb_cString) {
        rb_raise(cTypeError, "Invalid argument for string field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }

      if (rb_obj_encoding(value) != utf8) {
        // Note: this will not duplicate underlying string data unless necessary.
        value = rb_str_encode(value, utf8, 0, Qnil);

        if (rb_enc_str_coderange(value) == ENC_CODERANGE_BROKEN) {
          rb_raise(rb_eEncodingError, "String is invalid UTF-8");
        }
      }

      ret.str_val = Convert_StrDup(value, arena);
      break;
    }
    case UPB_TYPE_BYTES: {
      VALUE bytes = rb_enc_from_encoding(rb_ascii8bit_encoding());
      if (CLASS_OF(value) != rb_cString) {
        rb_raise(cTypeError, "Invalid argument for bytes field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }

      if (rb_obj_encoding(value) != bytes) {
        // Note: this will not duplicate underlying string data unless necessary.
        // TODO(haberman): is this really necessary to get raw bytes?
        value = rb_str_encode(value, bytes, 0, Qnil);
      }

      ret.str_val = Convert_StrDup(value, arena);
      break;
    }
    case UPB_TYPE_MESSAGE:
      ret.msg_val =
          Message_GetUpbMessage(value, info.message_type, name, arena);
      break;
    case UPB_TYPE_ENUM:
      ret.int32_val = Convert_ToEnum(value, name, info.enum_type);
      break;
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      Convert_CheckInt(name, type, value);
      switch (type) {
      case UPB_TYPE_INT32:
        ret.int32_val = NUM2INT(value);
        break;
      case UPB_TYPE_INT64:
        ret.int64_val = NUM2INT(value);
        break;
      case UPB_TYPE_UINT32:
        ret.uint32_val = NUM2UINT(value);
        break;
      case UPB_TYPE_UINT64:
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

VALUE Convert_UpbToRuby(upb_msgval upb_val, upb_fieldtype_t type,
                        TypeInfo type_info, VALUE arena) {
  switch (type) {
    case UPB_TYPE_FLOAT:
      return DBL2NUM(upb_val.float_val);
    case UPB_TYPE_DOUBLE:
      return DBL2NUM(upb_val.double_val);
    case UPB_TYPE_BOOL:
      return upb_val.bool_val ? Qtrue : Qfalse;
    case UPB_TYPE_INT32:
      return INT2NUM(upb_val.int32_val);
    case UPB_TYPE_INT64:
      return LL2NUM(upb_val.int64_val);
    case UPB_TYPE_UINT32:
      return UINT2NUM(upb_val.uint32_val);
    case UPB_TYPE_UINT64:
      return ULL2NUM(upb_val.int64_val);
    case UPB_TYPE_ENUM: {
      const char* name =
          upb_enumdef_iton(type_info.enum_type, upb_val.int32_val);
      if (name) {
        return ID2SYM(rb_intern(name));
      } else {
        return INT2NUM(upb_val.int32_val);
      }
    }
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      return rb_str_new(upb_val.str_val.data, upb_val.str_val.size);
    case UPB_TYPE_MESSAGE: {
      VALUE val = DEREF(memory, VALUE);

      // Lazily expand wrapper type if necessary.
      int type = TYPE(val);
      if (type != T_DATA && type != T_NIL) {
        // This must be a wrapper type.
        val = ruby_wrapper_type(type_class, val);
        DEREF(memory, VALUE) = val;
      }

      return val;
    }
    default:
      rb_raise(rb_eRuntimeError, "Convert_UpbToRuby(): Unexpected type %d",
               (int)type);
  }
}
