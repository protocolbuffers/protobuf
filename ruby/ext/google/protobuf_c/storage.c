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

static bool is_ruby_num(VALUE value) {
  return (TYPE(value) == T_FLOAT ||
          TYPE(value) == T_FIXNUM ||
          TYPE(value) == T_BIGNUM);
}


void native_slot_set(const char* name,
                     upb_fieldtype_t type, VALUE type_class,
                     void* memory, VALUE value) {
  native_slot_set_value_and_case(name, type, type_class, memory, value, NULL, 0);
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

void native_slot_deep_copy(upb_fieldtype_t type, VALUE type_class, void* to,
                           void* from) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      VALUE from_val = DEREF(from, VALUE);
      DEREF(to, VALUE) = (from_val != Qnil) ?
          rb_funcall(from_val, rb_intern("dup"), 0) : Qnil;
      break;
    }
    case UPB_TYPE_MESSAGE: {
      VALUE from_val = native_slot_get(type, type_class, from);
      DEREF(to, VALUE) = (from_val != Qnil) ?
          Message_deep_copy(from_val) : Qnil;
      break;
    }
    default:
      memcpy(to, from, native_slot_size(type));
  }
}

bool native_slot_eq(upb_fieldtype_t type, VALUE type_class, void* mem1,
                    void* mem2) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE: {
      VALUE val1 = native_slot_get(type, type_class, mem1);
      VALUE val2 = native_slot_get(type, type_class, mem2);
      VALUE ret = rb_funcall(val1, rb_intern("=="), 1, val2);
      return ret == Qtrue;
    }
    default:
      return !memcmp(mem1, mem2, native_slot_size(type));
  }
}

// -----------------------------------------------------------------------------
// Memory layout management.
// -----------------------------------------------------------------------------

VALUE layout_get(MessageLayout* layout,
                 const void* storage,
                 const upb_fielddef* field) {
  void* memory = slot_memory(layout, storage, field);
  const upb_oneofdef* oneof = upb_fielddef_realcontainingoneof(field);
  bool field_set;
  if (field_contains_hasbit(layout, field)) {
    field_set = slot_is_hasbit_set(layout, storage, field);
  } else {
    field_set = true;
  }

  if (oneof) {
    uint32_t oneof_case = slot_read_oneof_case(layout, storage, oneof);
    if (oneof_case != upb_fielddef_number(field)) {
      return layout_get_default(field);
    }
    return native_slot_get(upb_fielddef_type(field),
                           field_type_class(layout, field), memory);
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    return *((VALUE *)memory);
  } else if (!field_set) {
    return layout_get_default(field);
  } else {
    return native_slot_get(upb_fielddef_type(field),
                           field_type_class(layout, field), memory);
  }
}

void layout_set(MessageLayout* layout,
                void* storage,
                const upb_fielddef* field,
                VALUE val) {
  void* memory = slot_memory(layout, storage, field);
  const upb_oneofdef* oneof = upb_fielddef_realcontainingoneof(field);

  if (oneof) {
    uint32_t* oneof_case = slot_oneof_case(layout, storage, oneof);
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
      uint32_t case_value = upb_fielddef_number(field);
      if (upb_fielddef_issubmsg(field) || upb_fielddef_isstring(field)) {
        case_value |= ONEOF_CASE_MASK;
      }

      native_slot_set_value_and_case(
          upb_fielddef_name(field), upb_fielddef_type(field),
          field_type_class(layout, field), memory, val, oneof_case, case_value);
    }
  } else if (is_map_field(field)) {
    check_map_field_type(layout, val, field);
    DEREF(memory, VALUE) = val;
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    check_repeated_field_type(layout, val, field);
    DEREF(memory, VALUE) = val;
  } else {
    native_slot_set(upb_fielddef_name(field), upb_fielddef_type(field),
                    field_type_class(layout, field), memory, val);
  }

  if (layout->fields[upb_fielddef_index(field)].hasbit !=
      MESSAGE_FIELD_NO_HASBIT) {
    if (val == Qnil) {
      // No other field type has a hasbit and allows nil assignment.
      if (upb_fielddef_type(field) != UPB_TYPE_MESSAGE) {
        fprintf(stderr, "field: %s\n", upb_fielddef_fullname(field));
      }
      assert(upb_fielddef_type(field) == UPB_TYPE_MESSAGE);
      slot_clear_hasbit(layout, storage, field);
    } else {
      slot_set_hasbit(layout, storage, field);
    }
  }
}

