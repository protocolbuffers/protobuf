// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
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

#include "protobuf.h"

#include <math.h>

#include <ruby/encoding.h>

// -----------------------------------------------------------------------------
// Ruby <-> native slot management.
// -----------------------------------------------------------------------------

#define CHARPTR_AT(msg, ofs) ((char*)msg + ofs)
#define DEREF_OFFSET(msg, ofs, type) *(type*)CHARPTR_AT(msg, ofs)
#define DEREF(memory, type) *(type*)(memory)

size_t native_slot_size(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_FLOAT:   return 4;
    case UPB_TYPE_DOUBLE:  return 8;
    case UPB_TYPE_BOOL:    return 1;
    case UPB_TYPE_STRING:  return sizeof(VALUE);
    case UPB_TYPE_BYTES:   return sizeof(VALUE);
    case UPB_TYPE_MESSAGE: return sizeof(VALUE);
    case UPB_TYPE_ENUM:    return 4;
    case UPB_TYPE_INT32:   return 4;
    case UPB_TYPE_INT64:   return 8;
    case UPB_TYPE_UINT32:  return 4;
    case UPB_TYPE_UINT64:  return 8;
    default: return 0;
  }
}

static bool is_ruby_num(VALUE value) {
  return (TYPE(value) == T_FLOAT ||
          TYPE(value) == T_FIXNUM ||
          TYPE(value) == T_BIGNUM);
}

void native_slot_check_int_range_precision(const char* name, upb_fieldtype_t type, VALUE val) {
  if (!is_ruby_num(val)) {
    rb_raise(cTypeError, "Expected number type for integral field '%s' (given %s).",
             name, rb_class2name(CLASS_OF(val)));
  }

  // NUM2{INT,UINT,LL,ULL} macros do the appropriate range checks on upper
  // bound; we just need to do precision checks (i.e., disallow rounding) and
  // check for < 0 on unsigned types.
  if (TYPE(val) == T_FLOAT) {
    double dbl_val = NUM2DBL(val);
    if (floor(dbl_val) != dbl_val) {
      rb_raise(rb_eRangeError,
               "Non-integral floating point value assigned to integer field '%s' (given %s).",
               name, rb_class2name(CLASS_OF(val)));
    }
  }
  if (type == UPB_TYPE_UINT32 || type == UPB_TYPE_UINT64) {
    if (NUM2DBL(val) < 0) {
      rb_raise(rb_eRangeError,
               "Assigning negative value to unsigned integer field '%s' (given %s).",
               name, rb_class2name(CLASS_OF(val)));
    }
  }
}

VALUE native_slot_encode_and_freeze_string(upb_fieldtype_t type, VALUE value) {
  rb_encoding* desired_encoding = (type == UPB_TYPE_STRING) ?
      kRubyStringUtf8Encoding : kRubyString8bitEncoding;
  VALUE desired_encoding_value = rb_enc_from_encoding(desired_encoding);

  // Note: this will not duplicate underlying string data unless necessary.
  value = rb_str_encode(value, desired_encoding_value, 0, Qnil);

  if (type == UPB_TYPE_STRING &&
      rb_enc_str_coderange(value) == ENC_CODERANGE_BROKEN) {
    rb_raise(rb_eEncodingError, "String is invalid UTF-8");
  }

  // Ensure the data remains valid.  Since we called #encode a moment ago,
  // this does not freeze the string the user assigned.
  rb_obj_freeze(value);

  return value;
}

void native_slot_set(const char* name,
                     upb_fieldtype_t type, VALUE type_class,
                     void* memory, VALUE value) {
  native_slot_set_value_and_case(name, type, type_class, memory, value, NULL, 0);
}

void native_slot_set_value_and_case(const char* name,
                                    upb_fieldtype_t type, VALUE type_class,
                                    void* memory, VALUE value,
                                    uint32_t* case_memory,
                                    uint32_t case_number) {
  // Note that in order to atomically change the value in memory and the case
  // value (w.r.t. Ruby VM calls), we must set the value at |memory| only after
  // all Ruby VM calls are complete. The case is then set at the bottom of this
  // function.
  switch (type) {
    case UPB_TYPE_FLOAT:
      if (!is_ruby_num(value)) {
        rb_raise(cTypeError, "Expected number type for float field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }
      DEREF(memory, float) = NUM2DBL(value);
      break;
    case UPB_TYPE_DOUBLE:
      if (!is_ruby_num(value)) {
        rb_raise(cTypeError, "Expected number type for double field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }
      DEREF(memory, double) = NUM2DBL(value);
      break;
    case UPB_TYPE_BOOL: {
      int8_t val = -1;
      if (value == Qtrue) {
        val = 1;
      } else if (value == Qfalse) {
        val = 0;
      } else {
        rb_raise(cTypeError, "Invalid argument for boolean field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }
      DEREF(memory, int8_t) = val;
      break;
    }
    case UPB_TYPE_STRING:
      if (CLASS_OF(value) == rb_cSymbol) {
        value = rb_funcall(value, rb_intern("to_s"), 0);
      } else if (CLASS_OF(value) != rb_cString) {
        rb_raise(cTypeError, "Invalid argument for string field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }

      DEREF(memory, VALUE) = native_slot_encode_and_freeze_string(type, value);
      break;

    case UPB_TYPE_BYTES: {
      if (CLASS_OF(value) != rb_cString) {
        rb_raise(cTypeError, "Invalid argument for bytes field '%s' (given %s).",
                 name, rb_class2name(CLASS_OF(value)));
      }

      DEREF(memory, VALUE) = native_slot_encode_and_freeze_string(type, value);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      if (CLASS_OF(value) == CLASS_OF(Qnil)) {
        value = Qnil;
      } else if (CLASS_OF(value) != type_class) {
        // check for possible implicit conversions
        VALUE converted_value = NULL;
        char* field_type_name = rb_class2name(type_class);

        if (strcmp(field_type_name, "Google::Protobuf::Timestamp") == 0 &&
            rb_obj_is_kind_of(value, rb_cTime)) {
          // Time -> Google::Protobuf::Timestamp
          VALUE hash = rb_hash_new();
          rb_hash_aset(hash, rb_str_new2("seconds"), rb_funcall(value, rb_intern("to_i"), 0));
          rb_hash_aset(hash, rb_str_new2("nanos"), rb_funcall(value, rb_intern("nsec"), 0));
          VALUE args[1] = { hash };
          converted_value = rb_class_new_instance(1, args, type_class);
        } else if (strcmp(field_type_name, "Google::Protobuf::Duration") == 0 &&
                   rb_obj_is_kind_of(value, rb_cNumeric)) {
          // Numeric -> Google::Protobuf::Duration
          VALUE hash = rb_hash_new();
          rb_hash_aset(hash, rb_str_new2("seconds"), rb_funcall(value, rb_intern("to_i"), 0));
          VALUE n_value = rb_funcall(value, rb_intern("remainder"), 1, INT2NUM(1));
          n_value = rb_funcall(n_value, rb_intern("*"), 1, INT2NUM(1000000000));
          n_value = rb_funcall(n_value, rb_intern("round"), 0);
          rb_hash_aset(hash, rb_str_new2("nanos"), n_value);
          VALUE args[1] = { hash };
          converted_value = rb_class_new_instance(1, args, type_class);
        }

        // raise if no suitable conversaion could be found
        if (converted_value == NULL) {
          rb_raise(cTypeError,
                   "Invalid type %s to assign to submessage field '%s'.",
                  rb_class2name(CLASS_OF(value)), name);
        } else {
          value = converted_value;
        }
      }
      DEREF(memory, VALUE) = value;
      break;
    }
    case UPB_TYPE_ENUM: {
      int32_t int_val = 0;
      if (TYPE(value) == T_STRING) {
        value = rb_funcall(value, rb_intern("to_sym"), 0);
      } else if (!is_ruby_num(value) && TYPE(value) != T_SYMBOL) {
        rb_raise(cTypeError,
                 "Expected number or symbol type for enum field '%s'.", name);
      }
      if (TYPE(value) == T_SYMBOL) {
        // Ensure that the given symbol exists in the enum module.
        VALUE lookup = rb_funcall(type_class, rb_intern("resolve"), 1, value);
        if (lookup == Qnil) {
          rb_raise(rb_eRangeError, "Unknown symbol value for enum field '%s'.", name);
        } else {
          int_val = NUM2INT(lookup);
        }
      } else {
        native_slot_check_int_range_precision(name, UPB_TYPE_INT32, value);
        int_val = NUM2INT(value);
      }
      DEREF(memory, int32_t) = int_val;
      break;
    }
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      native_slot_check_int_range_precision(name, type, value);
      switch (type) {
      case UPB_TYPE_INT32:
        DEREF(memory, int32_t) = NUM2INT(value);
        break;
      case UPB_TYPE_INT64:
        DEREF(memory, int64_t) = NUM2LL(value);
        break;
      case UPB_TYPE_UINT32:
        DEREF(memory, uint32_t) = NUM2UINT(value);
        break;
      case UPB_TYPE_UINT64:
        DEREF(memory, uint64_t) = NUM2ULL(value);
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

VALUE native_slot_get(upb_fieldtype_t type,
                      VALUE type_class,
                      const void* memory) {
  switch (type) {
    case UPB_TYPE_FLOAT:
      return DBL2NUM(DEREF(memory, float));
    case UPB_TYPE_DOUBLE:
      return DBL2NUM(DEREF(memory, double));
    case UPB_TYPE_BOOL:
      return DEREF(memory, int8_t) ? Qtrue : Qfalse;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      return DEREF(memory, VALUE);
    case UPB_TYPE_ENUM: {
      int32_t val = DEREF(memory, int32_t);
      VALUE symbol = enum_lookup(type_class, INT2NUM(val));
      if (symbol == Qnil) {
        return INT2NUM(val);
      } else {
        return symbol;
      }
    }
    case UPB_TYPE_INT32:
      return INT2NUM(DEREF(memory, int32_t));
    case UPB_TYPE_INT64:
      return LL2NUM(DEREF(memory, int64_t));
    case UPB_TYPE_UINT32:
      return UINT2NUM(DEREF(memory, uint32_t));
    case UPB_TYPE_UINT64:
      return ULL2NUM(DEREF(memory, uint64_t));
    default:
      return Qnil;
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
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      DEREF(memory, VALUE) = rb_str_new2("");
      rb_enc_associate(DEREF(memory, VALUE), (type == UPB_TYPE_BYTES) ?
                       kRubyString8bitEncoding : kRubyStringUtf8Encoding);
      break;
    case UPB_TYPE_MESSAGE:
      DEREF(memory, VALUE) = Qnil;
      break;
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

void native_slot_mark(upb_fieldtype_t type, void* memory) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      rb_gc_mark(DEREF(memory, VALUE));
      break;
    default:
      break;
  }
}

void native_slot_dup(upb_fieldtype_t type, void* to, void* from) {
  memcpy(to, from, native_slot_size(type));
}

void native_slot_deep_copy(upb_fieldtype_t type, void* to, void* from) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      VALUE from_val = DEREF(from, VALUE);
      DEREF(to, VALUE) = (from_val != Qnil) ?
          rb_funcall(from_val, rb_intern("dup"), 0) : Qnil;
      break;
    }
    case UPB_TYPE_MESSAGE: {
      VALUE from_val = DEREF(from, VALUE);
      DEREF(to, VALUE) = (from_val != Qnil) ?
          Message_deep_copy(from_val) : Qnil;
      break;
    }
    default:
      memcpy(to, from, native_slot_size(type));
  }
}

bool native_slot_eq(upb_fieldtype_t type, void* mem1, void* mem2) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE: {
      VALUE val1 = DEREF(mem1, VALUE);
      VALUE val2 = DEREF(mem2, VALUE);
      VALUE ret = rb_funcall(val1, rb_intern("=="), 1, val2);
      return ret == Qtrue;
    }
    default:
      return !memcmp(mem1, mem2, native_slot_size(type));
  }
}

// -----------------------------------------------------------------------------
// Map field utilities.
// -----------------------------------------------------------------------------

const upb_msgdef* tryget_map_entry_msgdef(const upb_fielddef* field) {
  const upb_msgdef* subdef;
  if (upb_fielddef_label(field) != UPB_LABEL_REPEATED ||
      upb_fielddef_type(field) != UPB_TYPE_MESSAGE) {
    return NULL;
  }
  subdef = upb_fielddef_msgsubdef(field);
  return upb_msgdef_mapentry(subdef) ? subdef : NULL;
}

const upb_msgdef *map_entry_msgdef(const upb_fielddef* field) {
  const upb_msgdef* subdef = tryget_map_entry_msgdef(field);
  assert(subdef);
  return subdef;
}

bool is_map_field(const upb_fielddef *field) {
  const upb_msgdef* subdef = tryget_map_entry_msgdef(field);
  if (subdef == NULL) return false;

  // Map fields are a proto3 feature.
  // If we're using proto2 syntax we need to fallback to the repeated field.
  return upb_msgdef_syntax(subdef) == UPB_SYNTAX_PROTO3;
}

const upb_fielddef* map_field_key(const upb_fielddef* field) {
  const upb_msgdef* subdef = map_entry_msgdef(field);
  return map_entry_key(subdef);
}

const upb_fielddef* map_field_value(const upb_fielddef* field) {
  const upb_msgdef* subdef = map_entry_msgdef(field);
  return map_entry_value(subdef);
}

const upb_fielddef* map_entry_key(const upb_msgdef* msgdef) {
  const upb_fielddef* key_field = upb_msgdef_itof(msgdef, MAP_KEY_FIELD);
  assert(key_field != NULL);
  return key_field;
}

const upb_fielddef* map_entry_value(const upb_msgdef* msgdef) {
  const upb_fielddef* value_field = upb_msgdef_itof(msgdef, MAP_VALUE_FIELD);
  assert(value_field != NULL);
  return value_field;
}

// -----------------------------------------------------------------------------
// Memory layout management.
// -----------------------------------------------------------------------------

bool field_contains_hasbit(MessageLayout* layout,
                            const upb_fielddef* field) {
  return layout->fields[upb_fielddef_index(field)].hasbit !=
      MESSAGE_FIELD_NO_HASBIT;
}

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

  size_t hasbit = 0;
  for (upb_msg_field_begin(&it, msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    if (upb_fielddef_haspresence(field)) {
      layout->fields[upb_fielddef_index(field)].hasbit = hasbit++;
    } else {
      layout->fields[upb_fielddef_index(field)].hasbit =
	  MESSAGE_FIELD_NO_HASBIT;
    }
  }

  if (hasbit != 0) {
    off += (hasbit + 8 - 1) / 8;
  }

  for (upb_msg_field_begin(&it, msgdef);
       !upb_msg_field_done(&it);
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
      field_size = sizeof(VALUE);
    } else {
      field_size = native_slot_size(upb_fielddef_type(field));
    }
    // Align current offset up to |size| granularity.
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
  // space for oneof cases is conceptually as wide as field tag numbers. In
  // practice, it's unlikely that a oneof would have more than e.g. 256 or 64K
  // members (8 or 16 bits respectively), so conceivably we could assign
  // consecutive case numbers and then pick a smaller oneof case slot size, but
  // the complexity to implement this indirection is probably not worthwhile.
  for (upb_msg_oneof_begin(&oit, msgdef);
       !upb_msg_oneof_done(&oit);
       upb_msg_oneof_next(&oit)) {
    const upb_oneofdef* oneof = upb_msg_iter_oneof(&oit);
    upb_oneof_iter fit;

    // Always allocate NATIVE_SLOT_MAX_SIZE bytes, but share the slot between
    // all fields.
    size_t field_size = NATIVE_SLOT_MAX_SIZE;
    // Align the offset.
    off = align_up_to(off, field_size);
    // Assign all fields in the oneof this same offset.
    for (upb_oneof_begin(&fit, oneof);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* field = upb_oneof_iter_field(&fit);
      layout->fields[upb_fielddef_index(field)].offset = off;
    }
    off += field_size;
  }

  // Now the case fields.
  for (upb_msg_oneof_begin(&oit, msgdef);
       !upb_msg_oneof_done(&oit);
       upb_msg_oneof_next(&oit)) {
    const upb_oneofdef* oneof = upb_msg_iter_oneof(&oit);
    upb_oneof_iter fit;

    size_t field_size = sizeof(uint32_t);
    // Align the offset.
    off = (off + field_size - 1) & ~(field_size - 1);
    // Assign all fields in the oneof this same offset.
    for (upb_oneof_begin(&fit, oneof);
         !upb_oneof_done(&fit);
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
  xfree(layout->fields);
  upb_msgdef_unref(layout->msgdef, &layout->msgdef);
  xfree(layout);
}

VALUE field_type_class(const upb_fielddef* field) {
  VALUE type_class = Qnil;
  if (upb_fielddef_type(field) == UPB_TYPE_MESSAGE) {
    VALUE submsgdesc =
        get_def_obj(upb_fielddef_subdef(field));
    type_class = Descriptor_msgclass(submsgdesc);
  } else if (upb_fielddef_type(field) == UPB_TYPE_ENUM) {
    VALUE subenumdesc =
        get_def_obj(upb_fielddef_subdef(field));
    type_class = EnumDescriptor_enummodule(subenumdesc);
  }
  return type_class;
}

static void* slot_memory(MessageLayout* layout,
                         const void* storage,
                         const upb_fielddef* field) {
  return ((uint8_t *)storage) +
      layout->fields[upb_fielddef_index(field)].offset;
}

static uint32_t* slot_oneof_case(MessageLayout* layout,
                                 const void* storage,
                                 const upb_fielddef* field) {
  return (uint32_t *)(((uint8_t *)storage) +
      layout->fields[upb_fielddef_index(field)].case_offset);
}

static void slot_set_hasbit(MessageLayout* layout,
                            const void* storage,
                            const upb_fielddef* field) {
  size_t hasbit = layout->fields[upb_fielddef_index(field)].hasbit;
  assert(hasbit != MESSAGE_FIELD_NO_HASBIT);

  ((uint8_t*)storage)[hasbit / 8] |= 1 << (hasbit % 8);
}

static void slot_clear_hasbit(MessageLayout* layout,
                              const void* storage,
                              const upb_fielddef* field) {
  size_t hasbit = layout->fields[upb_fielddef_index(field)].hasbit;
  assert(hasbit != MESSAGE_FIELD_NO_HASBIT);
  ((uint8_t*)storage)[hasbit / 8] &= ~(1 << (hasbit % 8));
}

static bool slot_is_hasbit_set(MessageLayout* layout,
                            const void* storage,
                            const upb_fielddef* field) {
  size_t hasbit = layout->fields[upb_fielddef_index(field)].hasbit;
  if (hasbit == MESSAGE_FIELD_NO_HASBIT) {
    return false;
  }

  return DEREF_OFFSET(
      (uint8_t*)storage, hasbit / 8, char) & (1 << (hasbit % 8));
}

VALUE layout_has(MessageLayout* layout,
                 const void* storage,
                 const upb_fielddef* field) {
  assert(field_contains_hasbit(layout, field));
  return slot_is_hasbit_set(layout, storage, field) ? Qtrue : Qfalse;
}

void layout_clear(MessageLayout* layout,
                 const void* storage,
                 const upb_fielddef* field) {
  void* memory = slot_memory(layout, storage, field);
  uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

  if (field_contains_hasbit(layout, field)) {
    slot_clear_hasbit(layout, storage, field);
  }

  if (upb_fielddef_containingoneof(field)) {
    memset(memory, 0, NATIVE_SLOT_MAX_SIZE);
    *oneof_case = ONEOF_CASE_NONE;
  } else if (is_map_field(field)) {
    VALUE map = Qnil;

    const upb_fielddef* key_field = map_field_key(field);
    const upb_fielddef* value_field = map_field_value(field);
    VALUE type_class = field_type_class(value_field);

    if (type_class != Qnil) {
      VALUE args[3] = {
        fieldtype_to_ruby(upb_fielddef_type(key_field)),
        fieldtype_to_ruby(upb_fielddef_type(value_field)),
        type_class,
      };
      map = rb_class_new_instance(3, args, cMap);
    } else {
      VALUE args[2] = {
        fieldtype_to_ruby(upb_fielddef_type(key_field)),
        fieldtype_to_ruby(upb_fielddef_type(value_field)),
      };
      map = rb_class_new_instance(2, args, cMap);
    }

    DEREF(memory, VALUE) = map;
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    VALUE ary = Qnil;

    VALUE type_class = field_type_class(field);

    if (type_class != Qnil) {
      VALUE args[2] = {
        fieldtype_to_ruby(upb_fielddef_type(field)),
        type_class,
      };
      ary = rb_class_new_instance(2, args, cRepeatedField);
    } else {
      VALUE args[1] = { fieldtype_to_ruby(upb_fielddef_type(field)) };
      ary = rb_class_new_instance(1, args, cRepeatedField);
    }

    DEREF(memory, VALUE) = ary;
  } else {
    native_slot_set(upb_fielddef_name(field),
                    upb_fielddef_type(field), field_type_class(field),
                    memory, layout_get_default(field));
  }
}

VALUE layout_get_default(const upb_fielddef *field) {
  switch (upb_fielddef_type(field)) {
    case UPB_TYPE_FLOAT:   return DBL2NUM(upb_fielddef_defaultfloat(field));
    case UPB_TYPE_DOUBLE:  return DBL2NUM(upb_fielddef_defaultdouble(field));
    case UPB_TYPE_BOOL:
      return upb_fielddef_defaultbool(field) ? Qtrue : Qfalse;
    case UPB_TYPE_MESSAGE: return Qnil;
    case UPB_TYPE_ENUM: {
      const upb_enumdef *enumdef = upb_fielddef_enumsubdef(field);
      int32_t num = upb_fielddef_defaultint32(field);
      const char *label = upb_enumdef_iton(enumdef, num);
      if (label) {
        return ID2SYM(rb_intern(label));
      } else {
        return INT2NUM(num);
      }
    }
    case UPB_TYPE_INT32:   return INT2NUM(upb_fielddef_defaultint32(field));
    case UPB_TYPE_INT64:   return LL2NUM(upb_fielddef_defaultint64(field));;
    case UPB_TYPE_UINT32:  return UINT2NUM(upb_fielddef_defaultuint32(field));
    case UPB_TYPE_UINT64:  return ULL2NUM(upb_fielddef_defaultuint64(field));
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      size_t size;
      const char *str = upb_fielddef_defaultstr(field, &size);
      VALUE str_rb = rb_str_new(str, size);

      rb_enc_associate(str_rb, (upb_fielddef_type(field) == UPB_TYPE_BYTES) ?
                 kRubyString8bitEncoding : kRubyStringUtf8Encoding);
      rb_obj_freeze(str_rb);
      return str_rb;
    }
    default: return Qnil;
  }
}

VALUE layout_get(MessageLayout* layout,
                 const void* storage,
                 const upb_fielddef* field) {
  void* memory = slot_memory(layout, storage, field);
  uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

  bool field_set;
  if (field_contains_hasbit(layout, field)) {
    field_set = slot_is_hasbit_set(layout, storage, field);
  } else {
    field_set = true;
  }

  if (upb_fielddef_containingoneof(field)) {
    if (*oneof_case != upb_fielddef_number(field)) {
      return layout_get_default(field);
    }
    return native_slot_get(upb_fielddef_type(field),
                           field_type_class(field),
                           memory);
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    return *((VALUE *)memory);
  } else if (!field_set) {
    return layout_get_default(field);
  } else {
    return native_slot_get(upb_fielddef_type(field),
                           field_type_class(field),
                           memory);
  }
}

static void check_repeated_field_type(VALUE val, const upb_fielddef* field) {
  RepeatedField* self;
  assert(upb_fielddef_label(field) == UPB_LABEL_REPEATED);

  if (!RB_TYPE_P(val, T_DATA) || !RTYPEDDATA_P(val) ||
      RTYPEDDATA_TYPE(val) != &RepeatedField_type) {
    rb_raise(cTypeError, "Expected repeated field array");
  }

  self = ruby_to_RepeatedField(val);
  if (self->field_type != upb_fielddef_type(field)) {
    rb_raise(cTypeError, "Repeated field array has wrong element type");
  }

  if (self->field_type == UPB_TYPE_MESSAGE) {
    if (self->field_type_class !=
        Descriptor_msgclass(get_def_obj(upb_fielddef_subdef(field)))) {
      rb_raise(cTypeError,
               "Repeated field array has wrong message class");
    }
  }


  if (self->field_type == UPB_TYPE_ENUM) {
    if (self->field_type_class !=
        EnumDescriptor_enummodule(get_def_obj(upb_fielddef_subdef(field)))) {
      rb_raise(cTypeError,
               "Repeated field array has wrong enum class");
    }
  }
}

static void check_map_field_type(VALUE val, const upb_fielddef* field) {
  const upb_fielddef* key_field = map_field_key(field);
  const upb_fielddef* value_field = map_field_value(field);
  Map* self;

  if (!RB_TYPE_P(val, T_DATA) || !RTYPEDDATA_P(val) ||
      RTYPEDDATA_TYPE(val) != &Map_type) {
    rb_raise(cTypeError, "Expected Map instance");
  }

  self = ruby_to_Map(val);
  if (self->key_type != upb_fielddef_type(key_field)) {
    rb_raise(cTypeError, "Map key type does not match field's key type");
  }
  if (self->value_type != upb_fielddef_type(value_field)) {
    rb_raise(cTypeError, "Map value type does not match field's value type");
  }
  if (upb_fielddef_type(value_field) == UPB_TYPE_MESSAGE ||
      upb_fielddef_type(value_field) == UPB_TYPE_ENUM) {
    if (self->value_type_class !=
        get_def_obj(upb_fielddef_subdef(value_field))) {
      rb_raise(cTypeError,
               "Map value type has wrong message/enum class");
    }
  }
}


void layout_set(MessageLayout* layout,
                void* storage,
                const upb_fielddef* field,
                VALUE val) {
  void* memory = slot_memory(layout, storage, field);
  uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

  if (upb_fielddef_containingoneof(field)) {
    if (val == Qnil) {
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
      native_slot_set_value_and_case(
          upb_fielddef_name(field),
          upb_fielddef_type(field), field_type_class(field),
          memory, val,
          oneof_case, upb_fielddef_number(field));
    }
  } else if (is_map_field(field)) {
    check_map_field_type(val, field);
    DEREF(memory, VALUE) = val;
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    check_repeated_field_type(val, field);
    DEREF(memory, VALUE) = val;
  } else {
    native_slot_set(upb_fielddef_name(field),
                    upb_fielddef_type(field), field_type_class(field),
                    memory, val);
  }

  if (layout->fields[upb_fielddef_index(field)].hasbit !=
      MESSAGE_FIELD_NO_HASBIT) {
    slot_set_hasbit(layout, storage, field);
  }
}

void layout_init(MessageLayout* layout,
                 void* storage) {

  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, layout->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    layout_clear(layout, storage, upb_msg_iter_field(&it));
  }
}

void layout_mark(MessageLayout* layout, void* storage) {
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, layout->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    void* memory = slot_memory(layout, storage, field);
    uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

    if (upb_fielddef_containingoneof(field)) {
      if (*oneof_case == upb_fielddef_number(field)) {
        native_slot_mark(upb_fielddef_type(field), memory);
      }
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      rb_gc_mark(DEREF(memory, VALUE));
    } else {
      native_slot_mark(upb_fielddef_type(field), memory);
    }
  }
}

void layout_dup(MessageLayout* layout, void* to, void* from) {
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, layout->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);

    void* to_memory = slot_memory(layout, to, field);
    uint32_t* to_oneof_case = slot_oneof_case(layout, to, field);
    void* from_memory = slot_memory(layout, from, field);
    uint32_t* from_oneof_case = slot_oneof_case(layout, from, field);

    if (upb_fielddef_containingoneof(field)) {
      if (*from_oneof_case == upb_fielddef_number(field)) {
        *to_oneof_case = *from_oneof_case;
        native_slot_dup(upb_fielddef_type(field), to_memory, from_memory);
      }
    } else if (is_map_field(field)) {
      DEREF(to_memory, VALUE) = Map_dup(DEREF(from_memory, VALUE));
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      DEREF(to_memory, VALUE) = RepeatedField_dup(DEREF(from_memory, VALUE));
    } else {
      if (field_contains_hasbit(layout, field)) {
        if (!slot_is_hasbit_set(layout, from, field)) continue;
        slot_set_hasbit(layout, to, field);
      }

      native_slot_dup(upb_fielddef_type(field), to_memory, from_memory);
    }
  }
}

void layout_deep_copy(MessageLayout* layout, void* to, void* from) {
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, layout->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);

    void* to_memory = slot_memory(layout, to, field);
    uint32_t* to_oneof_case = slot_oneof_case(layout, to, field);
    void* from_memory = slot_memory(layout, from, field);
    uint32_t* from_oneof_case = slot_oneof_case(layout, from, field);

    if (upb_fielddef_containingoneof(field)) {
      if (*from_oneof_case == upb_fielddef_number(field)) {
        *to_oneof_case = *from_oneof_case;
        native_slot_deep_copy(upb_fielddef_type(field), to_memory, from_memory);
      }
    } else if (is_map_field(field)) {
      DEREF(to_memory, VALUE) =
          Map_deep_copy(DEREF(from_memory, VALUE));
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      DEREF(to_memory, VALUE) =
          RepeatedField_deep_copy(DEREF(from_memory, VALUE));
    } else {
      if (field_contains_hasbit(layout, field)) {
        if (!slot_is_hasbit_set(layout, from, field)) continue;
        slot_set_hasbit(layout, to, field);
      }

      native_slot_deep_copy(upb_fielddef_type(field), to_memory, from_memory);
    }
  }
}

VALUE layout_eq(MessageLayout* layout, void* msg1, void* msg2) {
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, layout->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);

    void* msg1_memory = slot_memory(layout, msg1, field);
    uint32_t* msg1_oneof_case = slot_oneof_case(layout, msg1, field);
    void* msg2_memory = slot_memory(layout, msg2, field);
    uint32_t* msg2_oneof_case = slot_oneof_case(layout, msg2, field);

    if (upb_fielddef_containingoneof(field)) {
      if (*msg1_oneof_case != *msg2_oneof_case ||
          (*msg1_oneof_case == upb_fielddef_number(field) &&
           !native_slot_eq(upb_fielddef_type(field),
                           msg1_memory,
                           msg2_memory))) {
        return Qfalse;
      }
    } else if (is_map_field(field)) {
      if (!Map_eq(DEREF(msg1_memory, VALUE),
                  DEREF(msg2_memory, VALUE))) {
        return Qfalse;
      }
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      if (!RepeatedField_eq(DEREF(msg1_memory, VALUE),
                            DEREF(msg2_memory, VALUE))) {
        return Qfalse;
      }
    } else {
      if (slot_is_hasbit_set(layout, msg1, field) !=
	  slot_is_hasbit_set(layout, msg2, field) ||
          !native_slot_eq(upb_fielddef_type(field),
			  msg1_memory, msg2_memory)) {
        return Qfalse;
      }
    }
  }
  return Qtrue;
}

VALUE layout_hash(MessageLayout* layout, void* storage) {
  upb_msg_field_iter it;
  st_index_t h = rb_hash_start(0);
  VALUE hash_sym = rb_intern("hash");
  for (upb_msg_field_begin(&it, layout->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    VALUE field_val = layout_get(layout, storage, field);
    h = rb_hash_uint(h, NUM2LONG(rb_funcall(field_val, hash_sym, 0)));
  }
  h = rb_hash_end(h);

  return INT2FIX(h);
}

VALUE layout_inspect(MessageLayout* layout, void* storage) {
  VALUE str = rb_str_new2("");

  upb_msg_field_iter it;
  bool first = true;
  for (upb_msg_field_begin(&it, layout->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    VALUE field_val = layout_get(layout, storage, field);

    if (!first) {
      str = rb_str_cat2(str, ", ");
    } else {
      first = false;
    }
    str = rb_str_cat2(str, upb_fielddef_name(field));
    str = rb_str_cat2(str, ": ");

    str = rb_str_append(str, rb_funcall(field_val, rb_intern("inspect"), 0));
  }

  return str;
}
