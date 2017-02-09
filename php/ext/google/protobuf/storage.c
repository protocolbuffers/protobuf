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

#include <stdint.h>
#include <protobuf.h>
#include <Zend/zend.h>

#include "utf8.h"

// -----------------------------------------------------------------------------
// Native slot storage.
// -----------------------------------------------------------------------------

#define DEREF(memory, type) *(type*)(memory)

size_t native_slot_size(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_FLOAT: return 4;
    case UPB_TYPE_DOUBLE: return 8;
    case UPB_TYPE_BOOL: return 1;
    case UPB_TYPE_STRING: return sizeof(void*);
    case UPB_TYPE_BYTES: return sizeof(void*);
    case UPB_TYPE_MESSAGE: return sizeof(void*);
    case UPB_TYPE_ENUM: return 4;
    case UPB_TYPE_INT32: return 4;
    case UPB_TYPE_INT64: return 8;
    case UPB_TYPE_UINT32: return 4;
    case UPB_TYPE_UINT64: return 8;
    default: return 0;
  }
}

bool native_slot_set(upb_fieldtype_t type, const zend_class_entry* klass,
                     void* memory, zval* value TSRMLS_DC) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      if (!protobuf_convert_to_string(value)) {
        return false;
      }
      if (type == UPB_TYPE_STRING &&
          !is_structurally_valid_utf8(Z_STRVAL_P(value), Z_STRLEN_P(value))) {
        zend_error(E_USER_ERROR, "Given string is not UTF8 encoded.");
        return false;
      }
      if (*(zval**)memory != NULL) {
        REPLACE_ZVAL_VALUE((zval**)memory, value, 1);
      } else {
        // Handles repeated/map string field. Memory provided by
        // RepeatedField/Map is not initialized.
        MAKE_STD_ZVAL(DEREF(memory, zval*));
        ZVAL_STRINGL(DEREF(memory, zval*), Z_STRVAL_P(value), Z_STRLEN_P(value),
                     1);
      }
      break;
    }
    case UPB_TYPE_MESSAGE: {
      if (Z_TYPE_P(value) != IS_OBJECT && Z_TYPE_P(value) != IS_NULL) {
        zend_error(E_USER_ERROR, "Given value is not message.");
        return false;
      }
      if (Z_TYPE_P(value) == IS_OBJECT && klass != Z_OBJCE_P(value)) {
        zend_error(E_USER_ERROR, "Given message does not have correct class.");
        return false;
      }
      if (EXPECTED(DEREF(memory, zval*) != value)) {
        if (DEREF(memory, zval*) != NULL) {
          zval_ptr_dtor((zval**)memory);
        }
        DEREF(memory, zval*) = value;
        Z_ADDREF_P(value);
      }
      break;
    }

#define CASE_TYPE(upb_type, type, c_type, php_type)              \
  case UPB_TYPE_##upb_type: {                                    \
    c_type type##_value;                                         \
    if (protobuf_convert_to_##type(value, &type##_value)) {      \
      DEREF(memory, c_type) = type##_value;                      \
    }                                                            \
    break;                                                       \
  }
      CASE_TYPE(INT32,  int32,  int32_t,  LONG)
      CASE_TYPE(UINT32, uint32, uint32_t, LONG)
      CASE_TYPE(ENUM,   int32,  int32_t,  LONG)
      CASE_TYPE(INT64,  int64,  int64_t,  LONG)
      CASE_TYPE(UINT64, uint64, uint64_t, LONG)
      CASE_TYPE(FLOAT,  float,  float,    DOUBLE)
      CASE_TYPE(DOUBLE, double, double,   DOUBLE)
      CASE_TYPE(BOOL,   bool,   int8_t,   BOOL)

#undef CASE_TYPE

    default:
      break;
  }

  return true;
}

void native_slot_init(upb_fieldtype_t type, void* memory, zval** cache) {
  zval* tmp = NULL;
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
    case UPB_TYPE_MESSAGE:
      DEREF(memory, zval**) = cache;
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

void native_slot_get(upb_fieldtype_t type, const void* memory,
                     zval** cache TSRMLS_DC) {
  switch (type) {
#define CASE(upb_type, php_type, c_type) \
    case UPB_TYPE_##upb_type: \
      SEPARATE_ZVAL_IF_NOT_REF(cache); \
      ZVAL_##php_type(*cache, DEREF(memory, c_type)); \
      return;

CASE(FLOAT,  DOUBLE, float)
CASE(DOUBLE, DOUBLE, double)
CASE(BOOL,   BOOL,   int8_t)
CASE(INT32,  LONG,   int32_t)
CASE(ENUM,   LONG,   uint32_t)

#undef CASE

#if SIZEOF_LONG == 4
#define CASE(upb_type, c_type)                        \
    case UPB_TYPE_##upb_type: {                       \
      SEPARATE_ZVAL_IF_NOT_REF(cache);                \
      char buffer[MAX_LENGTH_OF_INT64];               \
      sprintf(buffer, "%lld", DEREF(memory, c_type)); \
      ZVAL_STRING(*cache, buffer, 1);                 \
      return;                                         \
    }
#else
#define CASE(upb_type, c_type)                        \
    case UPB_TYPE_##upb_type: {                       \
      SEPARATE_ZVAL_IF_NOT_REF(cache);                \
      ZVAL_LONG(*cache, DEREF(memory, c_type));       \
      return;                                         \
    }
#endif
CASE(UINT64, uint64_t)
CASE(INT64,  int64_t)
#undef CASE

    case UPB_TYPE_UINT32: {
      // Prepend bit-1 for negative numbers, so that uint32 value will be
      // consistent on both 32-bit and 64-bit architectures.
      SEPARATE_ZVAL_IF_NOT_REF(cache);
      int value = DEREF(memory, int32_t);
      if (sizeof(int) == 8) {
        value |= (-((value >> 31) & 0x1) & 0xFFFFFFFF00000000);
      }
      ZVAL_LONG(*cache, value);
      return;
    }

    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      // For optional string/bytes fields, the cache is owned by the containing
      // message and should have been updated during setting/decoding. However,
      // for repeated string/bytes fields, the cache is provided by zend engine
      // and has not been updated.
      zval* value = DEREF(memory, zval*);
      if (*cache != value) {
        ZVAL_STRINGL(*cache, Z_STRVAL_P(value), Z_STRLEN_P(value), 1);
      }
      break;
    }
    case UPB_TYPE_MESSAGE: {
      // Same as above for string/bytes fields.
      zval* value = DEREF(memory, zval*);
      if (*cache != value) {
        ZVAL_ZVAL(*cache, value, 1, 0);
      }
      return;
    }
    default:
      return;
  }
}

void native_slot_get_default(upb_fieldtype_t type, zval** cache TSRMLS_DC) {
  switch (type) {
#define CASE(upb_type, php_type)     \
  case UPB_TYPE_##upb_type:          \
    SEPARATE_ZVAL_IF_NOT_REF(cache); \
    ZVAL_##php_type(*cache, 0);      \
    return;

    CASE(FLOAT, DOUBLE)
    CASE(DOUBLE, DOUBLE)
    CASE(BOOL, BOOL)
    CASE(INT32, LONG)
    CASE(UINT32, LONG)
    CASE(ENUM, LONG)

#undef CASE

#if SIZEOF_LONG == 4
#define CASE(upb_type)                                \
    case UPB_TYPE_##upb_type: {                       \
      SEPARATE_ZVAL_IF_NOT_REF(cache);                \
      ZVAL_STRING(*cache, "0", 1);                    \
      return;                                         \
    }
#else
#define CASE(upb_type)                                \
    case UPB_TYPE_##upb_type: {                       \
      SEPARATE_ZVAL_IF_NOT_REF(cache);                \
      ZVAL_LONG(*cache, 0);                           \
      return;                                         \
    }
#endif
CASE(UINT64)
CASE(INT64)
#undef CASE

    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      SEPARATE_ZVAL_IF_NOT_REF(cache);
      ZVAL_STRINGL(*cache, "", 0, 1);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      SEPARATE_ZVAL_IF_NOT_REF(cache);
      ZVAL_NULL(*cache);
      return;
    }
    default:
      return;
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

const zend_class_entry* field_type_class(const upb_fielddef* field TSRMLS_DC) {
  if (upb_fielddef_type(field) == UPB_TYPE_MESSAGE) {
    zval* desc_php = get_def_obj(upb_fielddef_subdef(field));
    Descriptor* desc = zend_object_store_get_object(desc_php TSRMLS_CC);
    return desc->klass;
  } else if (upb_fielddef_type(field) == UPB_TYPE_ENUM) {
    zval* desc_php = get_def_obj(upb_fielddef_subdef(field));
    EnumDescriptor* desc = zend_object_store_get_object(desc_php TSRMLS_CC);
    return desc->klass;
  }
  return NULL;
}

// -----------------------------------------------------------------------------
// Memory layout management.
// -----------------------------------------------------------------------------

static size_t align_up_to(size_t offset, size_t granularity) {
  // Granularity must be a power of two.
  return (offset + granularity - 1) & ~(granularity - 1);
}

static void* slot_memory(MessageLayout* layout, const void* storage,
                         const upb_fielddef* field) {
  return ((uint8_t*)storage) + layout->fields[upb_fielddef_index(field)].offset;
}

static uint32_t* slot_oneof_case(MessageLayout* layout, const void* storage,
                                 const upb_fielddef* field) {
  return (uint32_t*)(((uint8_t*)storage) +
                     layout->fields[upb_fielddef_index(field)].case_offset);
}

static int slot_property_cache(MessageLayout* layout, const void* storage,
                               const upb_fielddef* field) {
  return layout->fields[upb_fielddef_index(field)].cache_index;
}

MessageLayout* create_layout(const upb_msgdef* msgdef) {
  MessageLayout* layout = ALLOC(MessageLayout);
  int nfields = upb_msgdef_numfields(msgdef);
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  size_t off = 0;
  int i = 0;

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
    layout->fields[upb_fielddef_index(field)].cache_index = i++;
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
      layout->fields[upb_fielddef_index(field)].cache_index = i;
    }
    i++;
    off += field_size;
  }

  // Now the case offset.
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

void layout_init(MessageLayout* layout, void* storage,
                 zval** properties_table TSRMLS_DC) {
  int i;
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, layout->msgdef), i = 0; !upb_msg_field_done(&it);
       upb_msg_field_next(&it), i++) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    void* memory = slot_memory(layout, storage, field);
    uint32_t* oneof_case = slot_oneof_case(layout, storage, field);
    int cache_index = slot_property_cache(layout, storage, field);
    zval** property_ptr = &properties_table[cache_index];

    if (upb_fielddef_containingoneof(field)) {
      memset(memory, 0, NATIVE_SLOT_MAX_SIZE);
      *oneof_case = ONEOF_CASE_NONE;
    } else if (is_map_field(field)) {
      zval_ptr_dtor(property_ptr);
      map_field_create_with_type(map_field_type, field, property_ptr TSRMLS_CC);
      DEREF(memory, zval**) = property_ptr;
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      zval_ptr_dtor(property_ptr);
      repeated_field_create_with_type(repeated_field_type, field,
                                      property_ptr TSRMLS_CC);
      DEREF(memory, zval**) = property_ptr;
      property_ptr = NULL;
    } else {
      native_slot_init(upb_fielddef_type(field), memory, property_ptr);
    }
  }
}

// For non-singular fields, the related memory needs to point to the actual
// zval in properties table first.
static void* value_memory(const upb_fielddef* field, void* memory) {
  switch (upb_fielddef_type(field)) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
    case UPB_TYPE_MESSAGE:
      memory = DEREF(memory, zval**);
      break;
    default:
      // No operation
      break;
  }
  return memory;
}

zval* layout_get(MessageLayout* layout, const void* storage,
                 const upb_fielddef* field, zval** cache TSRMLS_DC) {
  void* memory = slot_memory(layout, storage, field);
  uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

  if (upb_fielddef_containingoneof(field)) {
    if (*oneof_case != upb_fielddef_number(field)) {
      native_slot_get_default(upb_fielddef_type(field), cache TSRMLS_CC);
    } else {
      native_slot_get(upb_fielddef_type(field), value_memory(field, memory),
                      cache TSRMLS_CC);
    }
    return *cache;
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    return *cache;
  } else {
    native_slot_get(upb_fielddef_type(field), value_memory(field, memory),
                    cache TSRMLS_CC);
    return *cache;
  }
}

void layout_set(MessageLayout* layout, MessageHeader* header,
                const upb_fielddef* field, zval* val TSRMLS_DC) {
  void* storage = message_data(header);
  void* memory = slot_memory(layout, storage, field);
  uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

  if (upb_fielddef_containingoneof(field)) {
    upb_fieldtype_t type = upb_fielddef_type(field);
    zend_class_entry *ce = NULL;

    // For non-singular fields, the related memory needs to point to the actual
    // zval in properties table first.
    switch (type) {
      case UPB_TYPE_MESSAGE: {
        const upb_msgdef* msg = upb_fielddef_msgsubdef(field);
        zval* desc_php = get_def_obj(msg);
        Descriptor* desc = zend_object_store_get_object(desc_php TSRMLS_CC);
        ce = desc->klass;
        // Intentionally fall through.
      }
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES: {
        int property_cache_index =
            header->descriptor->layout->fields[upb_fielddef_index(field)]
                .cache_index;
        DEREF(memory, zval**) =
            &(header->std.properties_table)[property_cache_index];
        memory = DEREF(memory, zval**);
        break;
      }
      default:
        break;
    }

    native_slot_set(type, ce, memory, val TSRMLS_CC);
    *oneof_case = upb_fielddef_number(field);
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    // Works for both repeated and map fields
    memory = DEREF(memory, zval**);
    if (EXPECTED(DEREF(memory, zval*) != val)) {
        zval_ptr_dtor(memory);
        DEREF(memory, zval*) = val;
        Z_ADDREF_P(val);
    }
  } else {
    upb_fieldtype_t type = upb_fielddef_type(field);
    zend_class_entry *ce = NULL;
    if (type == UPB_TYPE_MESSAGE) {
      const upb_msgdef* msg = upb_fielddef_msgsubdef(field);
      zval* desc_php = get_def_obj(msg);
      Descriptor* desc = zend_object_store_get_object(desc_php TSRMLS_CC);
      ce = desc->klass;
    }
    native_slot_set(type, ce, value_memory(field, memory), val TSRMLS_CC);
  }
}

const char* layout_get_oneof_case(MessageLayout* layout, const void* storage,
                                  const upb_oneofdef* oneof TSRMLS_DC) {
  upb_oneof_iter i;
  const upb_fielddef* first_field;

  // Oneof is guaranteed to have at least one field. Get the first field.
  for(upb_oneof_begin(&i, oneof); !upb_oneof_done(&i); upb_oneof_next(&i)) {
    first_field = upb_oneof_iter_field(&i);
    break;
  }

  uint32_t* oneof_case = slot_oneof_case(layout, storage, first_field);
  if (*oneof_case == 0) {
    return "";
  }
  const upb_fielddef* field = upb_oneofdef_itof(oneof, *oneof_case);
  return upb_fielddef_name(field);
}
