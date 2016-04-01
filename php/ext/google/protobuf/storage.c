#include <stdint.h>
#include <protobuf.h>

// -----------------------------------------------------------------------------
// PHP <-> native slot management.
// -----------------------------------------------------------------------------

static zval* int32_to_zval(int32_t value) {
  zval* tmp;
  MAKE_STD_ZVAL(tmp);
  ZVAL_LONG(tmp, value);
  php_printf("int32 to zval\n");
  // ZVAL_LONG(tmp, 1);
  return tmp;
}

#define DEREF(memory, type) *(type*)(memory)

size_t native_slot_size(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_FLOAT: return 4;
    case UPB_TYPE_DOUBLE: return 8;
    case UPB_TYPE_BOOL: return 1;
    case UPB_TYPE_STRING: return sizeof(zval*);
    case UPB_TYPE_BYTES: return sizeof(zval*);
    case UPB_TYPE_MESSAGE: return sizeof(zval*);
    case UPB_TYPE_ENUM: return 4;
    case UPB_TYPE_INT32: return 4;
    case UPB_TYPE_INT64: return 8;
    case UPB_TYPE_UINT32: return 4;
    case UPB_TYPE_UINT64: return 8;
    default: return 0;
  }
}

static bool is_php_num(zval* value) {
  // Is numerial string also valid?
  return (Z_TYPE_P(value) == IS_LONG ||
          Z_TYPE_P(value) == IS_DOUBLE);
}

void native_slot_check_int_range_precision(upb_fieldtype_t type, zval* val) {
  // TODO(teboring): Add it back.
  // if (!is_php_num(val)) {
  //   zend_error(E_ERROR, "Expected number type for integral field.");
  // }

  // if (Z_TYPE_P(val) == IS_DOUBLE) {
  //   double dbl_val = NUM2DBL(val);
  //   if (floor(dbl_val) != dbl_val) {
  //     zend_error(E_ERROR,
  //              "Non-integral floating point value assigned to integer field.");
  //   }
  // }
  // if (type == UPB_TYPE_UINT32 || type == UPB_TYPE_UINT64) {
  //   if (NUM2DBL(val) < 0) {
  //     zend_error(E_ERROR,
  //              "Assigning negative value to unsigned integer field.");
  //   }
  // }
}

zval* native_slot_get(upb_fieldtype_t type, /*VALUE type_class,*/
                      const void* memory TSRMLS_DC) {
  zval* retval = NULL;
  switch (type) {
    // TODO(teboring): Add it back.
    // case UPB_TYPE_FLOAT:
    //   return DBL2NUM(DEREF(memory, float));
    // case UPB_TYPE_DOUBLE:
    //   return DBL2NUM(DEREF(memory, double));
    // case UPB_TYPE_BOOL:
    //   return DEREF(memory, int8_t) ? Qtrue : Qfalse;
    // case UPB_TYPE_STRING:
    // case UPB_TYPE_BYTES:
    // case UPB_TYPE_MESSAGE:
    //   return DEREF(memory, VALUE);
    // case UPB_TYPE_ENUM: {
    //   int32_t val = DEREF(memory, int32_t);
    //   VALUE symbol = enum_lookup(type_class, INT2NUM(val));
    //   if (symbol == Qnil) {
    //     return INT2NUM(val);
    //   } else {
    //     return symbol;
    //   }
    // }
    case UPB_TYPE_INT32:
      return int32_to_zval(DEREF(memory, int32_t));
    // TODO(teboring): Add it back.
    // case UPB_TYPE_INT64:
    //   return LL2NUM(DEREF(memory, int64_t));
    // case UPB_TYPE_UINT32:
    //   return UINT2NUM(DEREF(memory, uint32_t));
    // case UPB_TYPE_UINT64:
    //   return ULL2NUM(DEREF(memory, uint64_t));
    default:
      return EG(uninitialized_zval_ptr);
  }
}

void native_slot_init(upb_fieldtype_t type, void* memory) {
  switch (type) {
    case UPB_TYPE_FLOAT:
      DEREF(memory, float) = 0.0;
      break;
    case UPB_TYPE_DOUBLE:
      DEREF(memory, double) = 0.0;
      break;
    case UPB_TYPE_BOOL:
      DEREF(memory, int8_t) = 0;
      break;
    // TODO(teboring): Add it back.
    // case UPB_TYPE_STRING:
    // case UPB_TYPE_BYTES:
    //   DEREF(memory, VALUE) = php_str_new2("");
    //   php_enc_associate(DEREF(memory, VALUE), (type == UPB_TYPE_BYTES)
    //                                              ? kRubyString8bitEncoding
    //                                              : kRubyStringUtf8Encoding);
    //   break;
    // case UPB_TYPE_MESSAGE:
    //   DEREF(memory, VALUE) = Qnil;
    //   break;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:
      DEREF(memory, int32_t) = 0;
      break;
    case UPB_TYPE_INT64:
      DEREF(memory, int64_t) = 0;
      break;
    case UPB_TYPE_UINT32:
      DEREF(memory, uint32_t) = 0;
      break;
    case UPB_TYPE_UINT64:
      DEREF(memory, uint64_t) = 0;
      break;
    default:
      break;
  }
}

void native_slot_set(upb_fieldtype_t type, /*VALUE type_class,*/ void* memory,
                     zval* value) {
  native_slot_set_value_and_case(type, /*type_class,*/ memory, value, NULL, 0);
}

void native_slot_set_value_and_case(upb_fieldtype_t type, /*VALUE type_class,*/
                                    void* memory, zval* value,
                                    uint32_t* case_memory,
                                    uint32_t case_number) {
  switch (type) {
    case UPB_TYPE_FLOAT:
      if (!Z_TYPE_P(value) == IS_LONG) {
        zend_error(E_ERROR, "Expected number type for float field.");
      }
      DEREF(memory, float) = Z_DVAL_P(value);
      break;
    case UPB_TYPE_DOUBLE:
      // TODO(teboring): Add it back.
      // if (!is_php_num(value)) {
      //   zend_error(E_ERROR, "Expected number type for double field.");
      // }
      // DEREF(memory, double) = Z_DVAL_P(value);
      break;
    case UPB_TYPE_BOOL: {
      int8_t val = -1;
      if (zval_is_true(value)) {
        val = 1;
      } else {
        val = 0;
      }
      // TODO(teboring): Add it back.
      // else if (value == Qfalse) {
      //   val = 0;
      // }
      // else {
      //   php_raise(php_eTypeError, "Invalid argument for boolean field.");
      // }
      DEREF(memory, int8_t) = val;
      break;
    }
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      // TODO(teboring): Add it back.
      // if (Z_TYPE_P(value) != IS_STRING) {
      //   zend_error(E_ERROR, "Invalid argument for string field.");
      // }
      // native_slot_validate_string_encoding(type, value);
      // DEREF(memory, zval*) = value;
      break;
    }
    case UPB_TYPE_MESSAGE: {
      // TODO(teboring): Add it back.
      // if (CLASS_OF(value) == CLASS_OF(Qnil)) {
      //   value = Qnil;
      // } else if (CLASS_OF(value) != type_class) {
      //   php_raise(php_eTypeError,
      //            "Invalid type %s to assign to submessage field.",
      //            php_class2name(CLASS_OF(value)));
      // }
      // DEREF(memory, VALUE) = value;
      break;
    }
    case UPB_TYPE_ENUM: {
      // TODO(teboring): Add it back.
      // int32_t int_val = 0;
      // if (!is_php_num(value) && TYPE(value) != T_SYMBOL) {
      //   php_raise(php_eTypeError,
      //            "Expected number or symbol type for enum field.");
      // }
      // if (TYPE(value) == T_SYMBOL) {
      //   // Ensure that the given symbol exists in the enum module.
      //   VALUE lookup = php_funcall(type_class, php_intern("resolve"), 1, value);
      //   if (lookup == Qnil) {
      //     php_raise(php_eRangeError, "Unknown symbol value for enum field.");
      //   } else {
      //     int_val = NUM2INT(lookup);
      //   }
      // } else {
      //   native_slot_check_int_range_precision(UPB_TYPE_INT32, value);
      //   int_val = NUM2INT(value);
      // }
      // DEREF(memory, int32_t) = int_val;
      // break;
    }
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      native_slot_check_int_range_precision(type, value);
      switch (type) {
        case UPB_TYPE_INT32:
          php_printf("Setting INT32 field\n");
          DEREF(memory, int32_t) = Z_LVAL_P(value);
          break;
        case UPB_TYPE_INT64:
          // TODO(teboring): Add it back.
          // DEREF(memory, int64_t) = NUM2LL(value);
          break;
        case UPB_TYPE_UINT32:
          // TODO(teboring): Add it back.
          // DEREF(memory, uint32_t) = NUM2UINT(value);
          break;
        case UPB_TYPE_UINT64:
          // TODO(teboring): Add it back.
          // DEREF(memory, uint64_t) = NUM2ULL(value);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  if (case_memory != NULL) {
    *case_memory = case_number;
  }
}

// -----------------------------------------------------------------------------
// Map field utilities.
// ----------------------------------------------------------------------------

const upb_msgdef* tryget_map_entry_msgdef(const upb_fielddef* field) {
  const upb_msgdef* subdef;
  if (upb_fielddef_label(field) != UPB_LABEL_REPEATED ||
      upb_fielddef_type(field) != UPB_TYPE_MESSAGE) {
    return NULL;
  }
  subdef = upb_fielddef_msgsubdef(field);
  return upb_msgdef_mapentry(subdef) ? subdef : NULL;
}

const upb_msgdef* map_entry_msgdef(const upb_fielddef* field) {
  const upb_msgdef* subdef = tryget_map_entry_msgdef(field);
  assert(subdef);
  return subdef;
}

bool is_map_field(const upb_fielddef* field) {
  return tryget_map_entry_msgdef(field) != NULL;
}

// -----------------------------------------------------------------------------
// Memory layout management.
// -----------------------------------------------------------------------------

static size_t align_up_to(size_t offset, size_t granularity) {
  // Granularity must be a power of two.
  return (offset + granularity - 1) & ~(granularity - 1);
}

MessageLayout* create_layout(const upb_msgdef* msgdef) {
  MessageLayout* layout = ALLOC(MessageLayout);
  int nfields = upb_msgdef_numfields(msgdef);
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  size_t off = 0;

  layout->fields = ALLOC_N(MessageField, nfields);

  for (upb_msg_field_begin(&it, msgdef); !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    size_t field_size;

    if (upb_fielddef_containingoneof(field)) {
      // Oneofs are handled separately below.
      continue;
    }

    // Allocate |field_size| bytes for this field in the layout.
    field_size = 0;
    if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      field_size = sizeof(zval*);
    } else {
      field_size = native_slot_size(upb_fielddef_type(field));
    }

    // Align current offset up to | size | granularity.
    off = align_up_to(off, field_size);
    layout->fields[upb_fielddef_index(field)].offset = off;
    layout->fields[upb_fielddef_index(field)].case_offset =
        MESSAGE_FIELD_NO_CASE;
    off += field_size;
  }

  // Handle oneofs now -- we iterate over oneofs specifically and allocate only
  // one slot per oneof.
  //
  // We assign all value slots first, then pack the 'case' fields at the end,
  // since in the common case (modern 64-bit platform) these are 8 bytes and 4
  // bytes respectively and we want to avoid alignment overhead.
  //
  // Note that we reserve 4 bytes (a uint32) per 'case' slot because the value
  // space for oneof cases is conceptually as wide as field tag numbers.  In
  // practice, it's unlikely that a oneof would have more than e.g.  256 or 64K
  // members (8 or 16 bits respectively), so conceivably we could assign
  // consecutive case numbers and then pick a smaller oneof case slot size, but
  // the complexity to implement this indirection is probably not worthwhile.
  for (upb_msg_oneof_begin(&oit, msgdef); !upb_msg_oneof_done(&oit);
       upb_msg_oneof_next(&oit)) {
    const upb_oneofdef* oneof = upb_msg_iter_oneof(&oit);
    upb_oneof_iter fit;

    // Always allocate NATIVE_SLOT_MAX_SIZE bytes, but share the slot between
    // all fields.
    size_t field_size = NATIVE_SLOT_MAX_SIZE;
    // Align the offset .
    off = align_up_to( off, field_size);
    // Assign all fields in the oneof this same offset.
    for (upb_oneof_begin(&fit, oneof); !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* field = upb_oneof_iter_field(&fit);
      layout->fields[upb_fielddef_index(field)].offset = off;
    }
    off += field_size;
  }

  // Now the case fields.
  for (upb_msg_oneof_begin(&oit, msgdef); !upb_msg_oneof_done(&oit);
       upb_msg_oneof_next(&oit)) {
    const upb_oneofdef* oneof = upb_msg_iter_oneof(&oit);
    upb_oneof_iter fit;

    size_t field_size = sizeof(uint32_t);
    // Align the offset .
    off = (off + field_size - 1) & ~(field_size - 1);
    // Assign all fields in the oneof this same offset.
    for (upb_oneof_begin(&fit, oneof); !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* field = upb_oneof_iter_field(&fit);
      layout->fields[upb_fielddef_index(field)].case_offset = off;
    }
    off += field_size;
  }

  layout->size = off;

  layout->msgdef = msgdef;
  upb_msgdef_ref(layout->msgdef, &layout->msgdef);

  return layout;
}

void free_layout(MessageLayout* layout) {
  FREE(layout->fields);
  upb_msgdef_unref(layout->msgdef, &layout->msgdef);
  FREE(layout);
}

// TODO(teboring): Add it back.
// VALUE field_type_class(const upb_fielddef* field) {
//   VALUE type_class = Qnil;
//   if (upb_fielddef_type(field) == UPB_TYPE_MESSAGE) {
//     VALUE submsgdesc = get_def_obj(upb_fielddef_subdef(field));
//     type_class = Descriptor_msgclass(submsgdesc);
//   } else if (upb_fielddef_type(field) == UPB_TYPE_ENUM) {
//     VALUE subenumdesc = get_def_obj(upb_fielddef_subdef(field));
//     type_class = EnumDescriptor_enummodule(subenumdesc);
//   }
//   return type_class;
// }

static void* slot_memory(MessageLayout* layout, const void* storage,
                         const upb_fielddef* field) {
  return ((uint8_t*)storage) + layout->fields[upb_fielddef_index(field)].offset;
}

static uint32_t* slot_oneof_case(MessageLayout* layout, const void* storage,
                                 const upb_fielddef* field) {
  return (uint32_t*)(((uint8_t*)storage) +
                     layout->fields[upb_fielddef_index(field)].case_offset);
}

void layout_set(MessageLayout* layout, void* storage, const upb_fielddef* field,
                zval* val) {
  void* memory = slot_memory(layout, storage, field);
  uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

  if (upb_fielddef_containingoneof(field)) {
    if (Z_TYPE_P(val) == IS_NULL) {
      // Assigning nil to a oneof field clears the oneof completely.
      *oneof_case = ONEOF_CASE_NONE;
      memset(memory, 0, NATIVE_SLOT_MAX_SIZE);
    } else {
      // The transition between field types for a single oneof (union) slot is
      // somewhat complex because we need to ensure that a GC triggered at any
      // point by a call into the Ruby VM sees a valid state for this field and
      // does not either go off into the weeds (following what it thinks is a
      // VALUE but is actually a different field type) or miss an object (seeing
      // what it thinks is a primitive field but is actually a VALUE for the new
      // field type).
      //
      // In order for the transition to be safe, the oneof case slot must be in
      // sync with the value slot whenever the Ruby VM has been called. Thus, we
      // use native_slot_set_value_and_case(), which ensures that both the value
      // and case number are altered atomically (w.r.t. the Ruby VM).
      native_slot_set_value_and_case(upb_fielddef_type(field),
                                     /*field_type_class(field),*/ memory, val,
                                     oneof_case, upb_fielddef_number(field));
    }
  } else if (is_map_field(field)) {
    // TODO(teboring): Add it back.
    // check_map_field_type(val, field);
    // DEREF(memory, zval*) = val;
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    // TODO(teboring): Add it back.
    // check_repeated_field_type(val, field);
    // DEREF(memory, zval*) = val;
  } else {
    native_slot_set(upb_fielddef_type(field), /*field_type_class(field),*/ memory,
                    val);
  }
}

void layout_init(MessageLayout* layout, void* storage) {
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, layout->msgdef); !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    void* memory = slot_memory(layout, storage, field);
    uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

    if (upb_fielddef_containingoneof(field)) {
      // TODO(teboring): Add it back.
      // memset(memory, 0, NATIVE_SLOT_MAX_SIZE);
      // *oneof_case = ONEOF_CASE_NONE;
    } else if (is_map_field(field)) {
      // TODO(teboring): Add it back.
      // VALUE map = Qnil;

      // const upb_fielddef* key_field = map_field_key(field);
      // const upb_fielddef* value_field = map_field_value(field);
      // VALUE type_class = field_type_class(value_field);

      // if (type_class != Qnil) {
      //   VALUE args[3] = {
      //       fieldtype_to_php(upb_fielddef_type(key_field)),
      //       fieldtype_to_php(upb_fielddef_type(value_field)), type_class,
      //   };
      //   map = php_class_new_instance(3, args, cMap);
      // } else {
      //   VALUE args[2] = {
      //       fieldtype_to_php(upb_fielddef_type(key_field)),
      //       fieldtype_to_php(upb_fielddef_type(value_field)),
      //   };
      //   map = php_class_new_instance(2, args, cMap);
      // }

      // DEREF(memory, VALUE) = map;
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      // TODO(teboring): Add it back.
      // VALUE ary = Qnil;

      // VALUE type_class = field_type_class(field);

      // if (type_class != Qnil) {
      //   VALUE args[2] = {
      //       fieldtype_to_php(upb_fielddef_type(field)), type_class,
      //   };
      //   ary = php_class_new_instance(2, args, cRepeatedField);
      // } else {
      //   VALUE args[1] = {fieldtype_to_php(upb_fielddef_type(field))};
      //   ary = php_class_new_instance(1, args, cRepeatedField);
      // }

      // DEREF(memory, VALUE) = ary;
    } else {
      native_slot_init(upb_fielddef_type(field), memory);
    }
  }
}

zval* layout_get(MessageLayout* layout, const void* storage,
                 const upb_fielddef* field TSRMLS_DC) {
  void* memory = slot_memory(layout, storage, field);
  uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

  if (upb_fielddef_containingoneof(field)) {
    if (*oneof_case != upb_fielddef_number(field)) {
      return NULL;
      // TODO(teboring): Add it back.
      // return Qnil;
    }
      return NULL;
    // TODO(teboring): Add it back.
    // return native_slot_get(upb_fielddef_type(field), field_type_class(field),
    //                        memory);
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      return NULL;
    // TODO(teboring): Add it back.
    // return *((VALUE*)memory);
  } else {
    return native_slot_get(
        upb_fielddef_type(field), /*field_type_class(field), */
        memory TSRMLS_CC);
  }
}
