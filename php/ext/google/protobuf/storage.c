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

static bool native_slot_is_default(upb_fieldtype_t type, const void* memory) {
  switch (type) {
#define CASE_TYPE(upb_type, c_type)    \
  case UPB_TYPE_##upb_type: {          \
    return DEREF(memory, c_type) == 0; \
  }
    CASE_TYPE(INT32,  int32_t )
    CASE_TYPE(UINT32, uint32_t)
    CASE_TYPE(ENUM,   int32_t )
    CASE_TYPE(INT64,  int64_t )
    CASE_TYPE(UINT64, uint64_t)
    CASE_TYPE(FLOAT,  float   )
    CASE_TYPE(DOUBLE, double  )
    CASE_TYPE(BOOL,   int8_t  )

#undef CASE_TYPE
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      return Z_STRLEN_P(CACHED_PTR_TO_ZVAL_PTR(DEREF(memory, CACHED_VALUE*))) ==
             0;
    case UPB_TYPE_MESSAGE:
      return Z_TYPE_P(CACHED_PTR_TO_ZVAL_PTR(DEREF(memory, CACHED_VALUE*))) ==
             IS_NULL;
    default: return false;
  }
}

bool native_slot_set(upb_fieldtype_t type, const zend_class_entry* klass,
                     void* memory, zval* value PHP_PROTO_TSRMLS_DC) {
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

      zval* cached_zval = CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory);
      if (EXPECTED(cached_zval != NULL)) {
#if PHP_MAJOR_VERSION < 7
        REPLACE_ZVAL_VALUE((zval**)memory, value, 1);
#else
        zend_assign_to_variable(cached_zval, value, IS_CV);
#endif
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

      zval* property_ptr = CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory);
      if (EXPECTED(property_ptr != value)) {
        php_proto_zval_ptr_dtor(property_ptr);
      }

#if PHP_MAJOR_VERSION < 7
      DEREF(memory, zval*) = value;
      Z_ADDREF_P(value);
#else
      ZVAL_ZVAL(property_ptr, value, 1, 0);
#endif
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

bool native_slot_set_by_array(upb_fieldtype_t type,
                              const zend_class_entry* klass, void* memory,
                              zval* value TSRMLS_DC) {
#if PHP_MAJOR_VERSION >= 7
  if (Z_ISREF_P(value)) {
    ZVAL_DEREF(value);
  }
#endif
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

      // Handles repeated/map string field. Memory provided by
      // RepeatedField/Map is not initialized.
#if PHP_MAJOR_VERSION < 7
      MAKE_STD_ZVAL(DEREF(memory, zval*));
      PHP_PROTO_ZVAL_STRINGL(DEREF(memory, zval*), Z_STRVAL_P(value),
                             Z_STRLEN_P(value), 1);
#else
      *(zend_string**)memory =
          zend_string_init(Z_STRVAL_P(value), Z_STRLEN_P(value), 0);
#endif
      break;
    }
    case UPB_TYPE_MESSAGE: {
      if (Z_TYPE_P(value) != IS_OBJECT) {
        zend_error(E_USER_ERROR, "Given value is not message.");
        return false;
      }
      if (Z_TYPE_P(value) == IS_OBJECT && klass != Z_OBJCE_P(value)) {
        zend_error(E_USER_ERROR, "Given message does not have correct class.");
        return false;
      }
#if PHP_MAJOR_VERSION < 7
      if (EXPECTED(DEREF(memory, zval*) != value)) {
        DEREF(memory, zval*) = value;
        Z_ADDREF_P(value);
      }
#else
      DEREF(memory, zval*) = value;
      GC_ADDREF(Z_OBJ_P(value));
#endif
      break;
    }
    default:
      return native_slot_set(type, klass, memory, value TSRMLS_CC);
  }
  return true;
}

bool native_slot_set_by_map(upb_fieldtype_t type, const zend_class_entry* klass,
                            void* memory, zval* value TSRMLS_DC) {
#if PHP_MAJOR_VERSION >= 7
  if (Z_ISREF_P(value)) {
    ZVAL_DEREF(value);
  }
#endif
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

      // Handles repeated/map string field. Memory provided by
      // RepeatedField/Map is not initialized.
#if PHP_MAJOR_VERSION < 7
      MAKE_STD_ZVAL(DEREF(memory, zval*));
      PHP_PROTO_ZVAL_STRINGL(DEREF(memory, zval*), Z_STRVAL_P(value),
                             Z_STRLEN_P(value), 1);
#else
      *(zend_string**)memory =
          zend_string_init(Z_STRVAL_P(value), Z_STRLEN_P(value), 0);
#endif
      break;
    }
    case UPB_TYPE_MESSAGE: {
      if (Z_TYPE_P(value) != IS_OBJECT) {
        zend_error(E_USER_ERROR, "Given value is not message.");
        return false;
      }
      if (Z_TYPE_P(value) == IS_OBJECT && klass != Z_OBJCE_P(value)) {
        zend_error(E_USER_ERROR, "Given message does not have correct class.");
        return false;
      }
#if PHP_MAJOR_VERSION < 7
      if (EXPECTED(DEREF(memory, zval*) != value)) {
        DEREF(memory, zval*) = value;
        Z_ADDREF_P(value);
      }
#else
      DEREF(memory, zend_object*) = Z_OBJ_P(value);
      GC_ADDREF(Z_OBJ_P(value));
#endif
      break;
    }
    default:
      return native_slot_set(type, klass, memory, value TSRMLS_CC);
  }
  return true;
}

void native_slot_init(upb_fieldtype_t type, void* memory, CACHED_VALUE* cache) {
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
      DEREF(memory, CACHED_VALUE*) = cache;
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
                     CACHED_VALUE* cache TSRMLS_DC) {
  switch (type) {
#define CASE(upb_type, php_type, c_type)                                   \
  case UPB_TYPE_##upb_type:                                                \
    PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);                             \
    ZVAL_##php_type(CACHED_PTR_TO_ZVAL_PTR(cache), DEREF(memory, c_type)); \
    return;

    CASE(FLOAT, DOUBLE, float)
    CASE(DOUBLE, DOUBLE, double)
    CASE(BOOL, BOOL, int8_t)
    CASE(INT32, LONG, int32_t)
    CASE(ENUM, LONG, uint32_t)

#undef CASE

#if SIZEOF_LONG == 4
#define CASE(upb_type, c_type)                                       \
  case UPB_TYPE_##upb_type: {                                        \
    PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);                       \
    char buffer[MAX_LENGTH_OF_INT64];                                \
    sprintf(buffer, "%lld", DEREF(memory, c_type));                  \
    PHP_PROTO_ZVAL_STRING(CACHED_PTR_TO_ZVAL_PTR(cache), buffer, 1); \
    return;                                                          \
  }
#else
#define CASE(upb_type, c_type)                                       \
  case UPB_TYPE_##upb_type: {                                        \
    PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);                       \
    ZVAL_LONG(CACHED_PTR_TO_ZVAL_PTR(cache), DEREF(memory, c_type)); \
    return;                                                          \
  }
#endif
CASE(UINT64, uint64_t)
CASE(INT64,  int64_t)
#undef CASE

    case UPB_TYPE_UINT32: {
      // Prepend bit-1 for negative numbers, so that uint32 value will be
      // consistent on both 32-bit and 64-bit architectures.
      PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);
      int value = DEREF(memory, int32_t);
      if (sizeof(int) == 8) {
        value |= (-((value >> 31) & 0x1) & 0xFFFFFFFF00000000);
      }
      ZVAL_LONG(CACHED_PTR_TO_ZVAL_PTR(cache), value);
      return;
    }

    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      // For optional string/bytes/message fields, the cache is owned by the
      // containing message and should have been updated during
      // setting/decoding. However, oneof accessor call this function by
      // providing the return value directly, which is not the same as the cache
      // value.
      zval* value = CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory);
      if (CACHED_PTR_TO_ZVAL_PTR(cache) != value) {
        PHP_PROTO_ZVAL_STRINGL(CACHED_PTR_TO_ZVAL_PTR(cache), Z_STRVAL_P(value),
                               Z_STRLEN_P(value), 1);
      }
      break;
    }
    case UPB_TYPE_MESSAGE: {
      // Same as above for string/bytes fields.
      zval* value = CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory);
      if (CACHED_PTR_TO_ZVAL_PTR(cache) != value) {
        ZVAL_ZVAL(CACHED_PTR_TO_ZVAL_PTR(cache), value, 1, 0);
      }
      return;
    }
    default:
      return;
  }
}

void native_slot_get_by_array(upb_fieldtype_t type, const void* memory,
                              CACHED_VALUE* cache TSRMLS_DC) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
#if PHP_MAJOR_VERSION < 7
      zval* value = CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory);
      if (EXPECTED(CACHED_PTR_TO_ZVAL_PTR(cache) != value)) {
        PHP_PROTO_ZVAL_STRINGL(CACHED_PTR_TO_ZVAL_PTR(cache),
                               Z_STRVAL_P(value), Z_STRLEN_P(value), 1);
      }
#else
      ZVAL_NEW_STR(cache, zend_string_dup(*(zend_string**)memory, 0));
#endif
      return;
    }
    case UPB_TYPE_MESSAGE: {
#if PHP_MAJOR_VERSION < 7
      zval* value = CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory);
      if (EXPECTED(CACHED_PTR_TO_ZVAL_PTR(cache) != value)) {
        ZVAL_ZVAL(CACHED_PTR_TO_ZVAL_PTR(cache), value, 1, 0);
      }
#else
      ZVAL_COPY(CACHED_PTR_TO_ZVAL_PTR(cache), memory);
#endif
      return;
    }
    default:
      native_slot_get(type, memory, cache TSRMLS_CC);
  }
}

void native_slot_get_by_map_key(upb_fieldtype_t type, const void* memory,
                                int length, CACHED_VALUE* cache TSRMLS_DC) {
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      PHP_PROTO_ZVAL_STRINGL(CACHED_PTR_TO_ZVAL_PTR(cache), memory, length, 1);
      return;
    }
    default:
      native_slot_get(type, memory, cache TSRMLS_CC);
  }
}

void native_slot_get_by_map_value(upb_fieldtype_t type, const void* memory,
                              CACHED_VALUE* cache TSRMLS_DC) {
  switch (type) {
    case UPB_TYPE_MESSAGE: {
#if PHP_MAJOR_VERSION < 7
      zval* value = CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory);
      if (EXPECTED(CACHED_PTR_TO_ZVAL_PTR(cache) != value)) {
        ZVAL_ZVAL(CACHED_PTR_TO_ZVAL_PTR(cache), value, 1, 0);
      }
#else
      GC_ADDREF(*(zend_object**)memory);
      ZVAL_OBJ(cache, *(zend_object**)memory);
#endif
      return;
    }
    default:
      native_slot_get_by_array(type, memory, cache TSRMLS_CC);
  }
}

void native_slot_get_default(upb_fieldtype_t type,
                             CACHED_VALUE* cache TSRMLS_DC) {
  switch (type) {
#define CASE(upb_type, php_type)                       \
  case UPB_TYPE_##upb_type:                            \
    PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);         \
    ZVAL_##php_type(CACHED_PTR_TO_ZVAL_PTR(cache), 0); \
    return;

    CASE(FLOAT, DOUBLE)
    CASE(DOUBLE, DOUBLE)
    CASE(BOOL, BOOL)
    CASE(INT32, LONG)
    CASE(UINT32, LONG)
    CASE(ENUM, LONG)

#undef CASE

#if SIZEOF_LONG == 4
#define CASE(upb_type)                                            \
  case UPB_TYPE_##upb_type: {                                     \
    PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);                    \
    PHP_PROTO_ZVAL_STRING(CACHED_PTR_TO_ZVAL_PTR(cache), "0", 1); \
    return;                                                       \
  }
#else
#define CASE(upb_type)                           \
  case UPB_TYPE_##upb_type: {                    \
    PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);   \
    ZVAL_LONG(CACHED_PTR_TO_ZVAL_PTR(cache), 0); \
    return;                                      \
  }
#endif
CASE(UINT64)
CASE(INT64)
#undef CASE

    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);
      PHP_PROTO_ZVAL_STRINGL(CACHED_PTR_TO_ZVAL_PTR(cache), "", 0, 1);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(cache);
      ZVAL_NULL(CACHED_PTR_TO_ZVAL_PTR(cache));
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

const zend_class_entry* field_type_class(
    const upb_fielddef* field PHP_PROTO_TSRMLS_DC) {
  if (upb_fielddef_type(field) == UPB_TYPE_MESSAGE) {
    Descriptor* desc = UNBOX_HASHTABLE_VALUE(
        Descriptor, get_def_obj(upb_fielddef_msgsubdef(field)));
    return desc->klass;
  } else if (upb_fielddef_type(field) == UPB_TYPE_ENUM) {
    EnumDescriptor* desc = UNBOX_HASHTABLE_VALUE(
        EnumDescriptor, get_def_obj(upb_fielddef_enumsubdef(field)));
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

uint32_t* slot_oneof_case(MessageLayout* layout, const void* storage,
                          const upb_fielddef* field) {
  return (uint32_t*)(((uint8_t*)storage) +
                     layout->fields[upb_fielddef_index(field)].case_offset);
}

static int slot_property_cache(MessageLayout* layout, const void* storage,
                               const upb_fielddef* field) {
  return layout->fields[upb_fielddef_index(field)].cache_index;
}

void* slot_memory(MessageLayout* layout, const void* storage,
                         const upb_fielddef* field) {
  return ((uint8_t*)storage) + layout->fields[upb_fielddef_index(field)].offset;
}

MessageLayout* create_layout(const upb_msgdef* msgdef) {
  MessageLayout* layout = ALLOC(MessageLayout);
  int nfields = upb_msgdef_numfields(msgdef);
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  size_t off = 0;
  int i = 0;

  // Reserve space for unknown fields.
  off += sizeof(void*);

  TSRMLS_FETCH();
  Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(msgdef));
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

    const char* fieldname = upb_fielddef_name(field);

#if PHP_MAJOR_VERSION < 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
    zend_class_entry* old_scope = EG(scope);
    EG(scope) = desc->klass;
#else
    zend_class_entry* old_scope = EG(fake_scope);
    EG(fake_scope) = desc->klass;
#endif

#if PHP_MAJOR_VERSION < 7
    zval member;
    ZVAL_STRINGL(&member, fieldname, strlen(fieldname), 0);
    zend_property_info* property_info =
        zend_get_property_info(desc->klass, &member, true TSRMLS_CC);
#else
    zend_string* member = zend_string_init(fieldname, strlen(fieldname), 1);
    zend_property_info* property_info =
        zend_get_property_info(desc->klass, member, true);
    zend_string_release(member);
#endif

#if PHP_MAJOR_VERSION < 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
    EG(scope) = old_scope;
#else
    EG(fake_scope) = old_scope;
#endif

    layout->fields[upb_fielddef_index(field)].cache_index =
        property_info->offset;
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
    const char* oneofname = upb_oneofdef_name(oneof);
    for (upb_oneof_begin(&fit, oneof); !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* field = upb_oneof_iter_field(&fit);
      layout->fields[upb_fielddef_index(field)].offset = off;

#if PHP_MAJOR_VERSION < 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
      zend_class_entry* old_scope = EG(scope);
      EG(scope) = desc->klass;
#else
      zend_class_entry* old_scope = EG(fake_scope);
      EG(fake_scope) = desc->klass;
#endif

#if PHP_MAJOR_VERSION < 7
      zval member;
      ZVAL_STRINGL(&member, oneofname, strlen(oneofname), 0);
      zend_property_info* property_info =
          zend_get_property_info(desc->klass, &member, true TSRMLS_CC);
#else
      zend_string* member = zend_string_init(oneofname, strlen(oneofname), 1);
      zend_property_info* property_info =
          zend_get_property_info(desc->klass, member, true);
      zend_string_release(member);
#endif

#if PHP_MAJOR_VERSION < 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
      EG(scope) = old_scope;
#else
      EG(fake_scope) = old_scope;
#endif

      layout->fields[upb_fielddef_index(field)].cache_index =
          property_info->offset;
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

  return layout;
}

void free_layout(MessageLayout* layout) {
  FREE(layout->fields);
  FREE(layout);
}

void layout_init(MessageLayout* layout, void* storage,
                 zend_object* object PHP_PROTO_TSRMLS_DC) {
  int i;
  upb_msg_field_iter it;

  // Init unknown fields
  memset(storage, 0, sizeof(void*));

  for (upb_msg_field_begin(&it, layout->msgdef), i = 0; !upb_msg_field_done(&it);
       upb_msg_field_next(&it), i++) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    void* memory = slot_memory(layout, storage, field);
    uint32_t* oneof_case = slot_oneof_case(layout, storage, field);
    int cache_index = slot_property_cache(layout, storage, field);
    CACHED_VALUE* property_ptr = OBJ_PROP(object, cache_index);

    if (upb_fielddef_containingoneof(field)) {
      memset(memory, 0, NATIVE_SLOT_MAX_SIZE);
      *oneof_case = ONEOF_CASE_NONE;
    } else if (is_map_field(field)) {
      zval_ptr_dtor(property_ptr);
#if PHP_MAJOR_VERSION < 7
      MAKE_STD_ZVAL(*property_ptr);
#endif
      map_field_create_with_field(map_field_type, field,
                                  property_ptr PHP_PROTO_TSRMLS_CC);
      DEREF(memory, CACHED_VALUE*) = property_ptr;
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      zval_ptr_dtor(property_ptr);
#if PHP_MAJOR_VERSION < 7
      MAKE_STD_ZVAL(*property_ptr);
#endif
      repeated_field_create_with_field(repeated_field_type, field,
                                       property_ptr PHP_PROTO_TSRMLS_CC);
      DEREF(memory, CACHED_VALUE*) = property_ptr;
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
      memory = DEREF(memory, CACHED_VALUE*);
      break;
    default:
      // No operation
      break;
  }
  return memory;
}

zval* layout_get(MessageLayout* layout, const void* storage,
                 const upb_fielddef* field, CACHED_VALUE* cache TSRMLS_DC) {
  void* memory = slot_memory(layout, storage, field);
  uint32_t* oneof_case = slot_oneof_case(layout, storage, field);

  if (upb_fielddef_containingoneof(field)) {
    if (*oneof_case != upb_fielddef_number(field)) {
      native_slot_get_default(upb_fielddef_type(field), cache TSRMLS_CC);
    } else {
      native_slot_get(upb_fielddef_type(field), value_memory(field, memory),
                      cache TSRMLS_CC);
    }
    return CACHED_PTR_TO_ZVAL_PTR(cache);
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    return CACHED_PTR_TO_ZVAL_PTR(cache);
  } else {
    native_slot_get(upb_fielddef_type(field), value_memory(field, memory),
                    cache TSRMLS_CC);
    return CACHED_PTR_TO_ZVAL_PTR(cache);
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
        Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(msg));
        ce = desc->klass;
        // Intentionally fall through.
      }
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES: {
        int property_cache_index =
            header->descriptor->layout->fields[upb_fielddef_index(field)]
                .cache_index;
        DEREF(memory, CACHED_VALUE*) =
            OBJ_PROP(&header->std, property_cache_index);
        memory = DEREF(memory, CACHED_VALUE*);
        break;
      }
      default:
        break;
    }

    native_slot_set(type, ce, memory, val TSRMLS_CC);
    *oneof_case = upb_fielddef_number(field);
  } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
    // Works for both repeated and map fields
    memory = DEREF(memory, void**);
    zval* property_ptr = CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory);

    if (EXPECTED(property_ptr != val)) {
      zend_class_entry *subce = NULL;
      zval converted_value;

      if (upb_fielddef_ismap(field)) {
        const upb_msgdef* mapmsg = upb_fielddef_msgsubdef(field);
        const upb_fielddef* keyfield = upb_msgdef_ntof(mapmsg, "key", 3);
        const upb_fielddef* valuefield = upb_msgdef_ntof(mapmsg, "value", 5);
        if (upb_fielddef_descriptortype(valuefield) ==
            UPB_DESCRIPTOR_TYPE_MESSAGE) {
          const upb_msgdef* submsg = upb_fielddef_msgsubdef(valuefield);
          Descriptor* subdesc =
              UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(submsg));
          subce = subdesc->klass;
        }
        check_map_field(subce, upb_fielddef_descriptortype(keyfield),
                        upb_fielddef_descriptortype(valuefield), val,
                        &converted_value);
      } else {
        if (upb_fielddef_type(field) == UPB_TYPE_MESSAGE) {
          const upb_msgdef* submsg = upb_fielddef_msgsubdef(field);
          Descriptor* subdesc =
              UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(submsg));
          subce = subdesc->klass;
        }

        check_repeated_field(subce, upb_fielddef_descriptortype(field), val,
                             &converted_value);
      }
#if PHP_MAJOR_VERSION < 7
      REPLACE_ZVAL_VALUE((zval**)memory, &converted_value, 1);
#else
      php_proto_zval_ptr_dtor(property_ptr);
      ZVAL_ZVAL(property_ptr, &converted_value, 1, 0);
#endif
      zval_dtor(&converted_value);
    }
  } else {
    upb_fieldtype_t type = upb_fielddef_type(field);
    zend_class_entry *ce = NULL;
    if (type == UPB_TYPE_MESSAGE) {
      const upb_msgdef* msg = upb_fielddef_msgsubdef(field);
      Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(msg));
      ce = desc->klass;
    }
    native_slot_set(type, ce, value_memory(field, memory), val TSRMLS_CC);
  }
}

static void native_slot_merge(const upb_fielddef* field, const void* from_memory,
                         void* to_memory PHP_PROTO_TSRMLS_DC) {
  upb_fieldtype_t type = upb_fielddef_type(field);
  zend_class_entry* ce = NULL;
  if (!native_slot_is_default(type, from_memory)) {
    switch (type) {
#define CASE_TYPE(upb_type, c_type)                        \
  case UPB_TYPE_##upb_type: {                              \
    DEREF(to_memory, c_type) = DEREF(from_memory, c_type); \
    break;                                                 \
  }
      CASE_TYPE(INT32, int32_t)
      CASE_TYPE(UINT32, uint32_t)
      CASE_TYPE(ENUM, int32_t)
      CASE_TYPE(INT64, int64_t)
      CASE_TYPE(UINT64, uint64_t)
      CASE_TYPE(FLOAT, float)
      CASE_TYPE(DOUBLE, double)
      CASE_TYPE(BOOL, int8_t)

#undef CASE_TYPE
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        native_slot_set(type, NULL, value_memory(field, to_memory),
                        CACHED_PTR_TO_ZVAL_PTR(DEREF(
                            from_memory, CACHED_VALUE*)) PHP_PROTO_TSRMLS_CC);
        break;
      case UPB_TYPE_MESSAGE: {
        const upb_msgdef* msg = upb_fielddef_msgsubdef(field);
        Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(msg));
        ce = desc->klass;
        if (native_slot_is_default(type, to_memory)) {
#if PHP_MAJOR_VERSION < 7
          SEPARATE_ZVAL_IF_NOT_REF((zval**)value_memory(field, to_memory));
#endif
          CREATE_OBJ_ON_ALLOCATED_ZVAL_PTR(
              CACHED_PTR_TO_ZVAL_PTR(DEREF(to_memory, CACHED_VALUE*)), ce);
          MessageHeader* submsg =
              UNBOX(MessageHeader,
                    CACHED_PTR_TO_ZVAL_PTR(DEREF(to_memory, CACHED_VALUE*)));
          custom_data_init(ce, submsg PHP_PROTO_TSRMLS_CC);
        }

        MessageHeader* sub_from =
            UNBOX(MessageHeader,
                  CACHED_PTR_TO_ZVAL_PTR(DEREF(from_memory, CACHED_VALUE*)));
        MessageHeader* sub_to =
            UNBOX(MessageHeader,
                  CACHED_PTR_TO_ZVAL_PTR(DEREF(to_memory, CACHED_VALUE*)));

        layout_merge(desc->layout, sub_from, sub_to PHP_PROTO_TSRMLS_CC);
        break;
      }
    }
  }
}

static void native_slot_merge_by_array(const upb_fielddef* field, const void* from_memory,
                         void* to_memory PHP_PROTO_TSRMLS_DC) {
  upb_fieldtype_t type = upb_fielddef_type(field);
  switch (type) {
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
#if PHP_MAJOR_VERSION < 7
      MAKE_STD_ZVAL(DEREF(to_memory, zval*));
      PHP_PROTO_ZVAL_STRINGL(DEREF(to_memory, zval*),
                             Z_STRVAL_P(*(zval**)from_memory),
                             Z_STRLEN_P(*(zval**)from_memory), 1);
#else
      DEREF(to_memory, zend_string*) =
          zend_string_dup(*(zend_string**)from_memory, 0);
#endif
      break;
    }
    case UPB_TYPE_MESSAGE: {
      const upb_msgdef* msg = upb_fielddef_msgsubdef(field);
      Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, get_def_obj(msg));
      zend_class_entry* ce = desc->klass;
#if PHP_MAJOR_VERSION < 7
      MAKE_STD_ZVAL(DEREF(to_memory, zval*));
      CREATE_OBJ_ON_ALLOCATED_ZVAL_PTR(DEREF(to_memory, zval*), ce);
#else
      DEREF(to_memory, zend_object*) = ce->create_object(ce TSRMLS_CC);
#endif
      MessageHeader* sub_from = UNBOX_HASHTABLE_VALUE(
          MessageHeader, DEREF(from_memory, PHP_PROTO_HASHTABLE_VALUE));
      MessageHeader* sub_to = UNBOX_HASHTABLE_VALUE(
          MessageHeader, DEREF(to_memory, PHP_PROTO_HASHTABLE_VALUE));
      custom_data_init(ce, sub_to PHP_PROTO_TSRMLS_CC);

      layout_merge(desc->layout, sub_from, sub_to PHP_PROTO_TSRMLS_CC);
      break;
    }
    default:
      native_slot_merge(field, from_memory, to_memory PHP_PROTO_TSRMLS_CC);
      break;
  }
}

void layout_merge(MessageLayout* layout, MessageHeader* from,
                  MessageHeader* to PHP_PROTO_TSRMLS_DC) {
  int i, j;
  upb_msg_field_iter it;

  for (upb_msg_field_begin(&it, layout->msgdef), i = 0; !upb_msg_field_done(&it);
       upb_msg_field_next(&it), i++) {
    const upb_fielddef* field = upb_msg_iter_field(&it);

    void* to_memory = slot_memory(layout, message_data(to), field);
    void* from_memory = slot_memory(layout, message_data(from), field);

    if (upb_fielddef_containingoneof(field)) {
      uint32_t oneof_case_offset =
          layout->fields[upb_fielddef_index(field)].case_offset;
      // For a oneof, check that this field is actually present -- skip all the
      // below if not.
      if (DEREF((message_data(from) + oneof_case_offset), uint32_t) !=
          upb_fielddef_number(field)) {
        continue;
      }
      uint32_t* from_oneof_case = slot_oneof_case(layout, message_data(from), field);
      uint32_t* to_oneof_case = slot_oneof_case(layout, message_data(to), field);

      // For non-singular fields, the related memory needs to point to the
      // actual zval in properties table first.
      switch (upb_fielddef_type(field)) {
        case UPB_TYPE_MESSAGE:
        case UPB_TYPE_STRING:
        case UPB_TYPE_BYTES: {
          int property_cache_index =
              layout->fields[upb_fielddef_index(field)].cache_index;
          DEREF(to_memory, CACHED_VALUE*) =
              OBJ_PROP(&to->std, property_cache_index);
          break;
        }
        default:
          break;
      }

      *to_oneof_case = *from_oneof_case;

      // Otherwise, fall through to the appropriate singular-field handler
      // below.
    }

    if (is_map_field(field)) {
      int size, key_length, value_length;
      MapIter map_it;

      zval* to_map_php =
          CACHED_PTR_TO_ZVAL_PTR(DEREF(to_memory, CACHED_VALUE*));
      zval* from_map_php =
          CACHED_PTR_TO_ZVAL_PTR(DEREF(from_memory, CACHED_VALUE*));
      Map* to_map = UNBOX(Map, to_map_php);
      Map* from_map = UNBOX(Map, from_map_php);

      size = upb_strtable_count(&from_map->table);
      if (size == 0) continue;

      const upb_msgdef *mapentry_def = upb_fielddef_msgsubdef(field);
      const upb_fielddef *value_field = upb_msgdef_itof(mapentry_def, 2);

      for (map_begin(from_map_php, &map_it TSRMLS_CC); !map_done(&map_it);
           map_next(&map_it)) {
        const char* key = map_iter_key(&map_it, &key_length);
        upb_value from_value = map_iter_value(&map_it, &value_length);
        upb_value to_value;
        void* from_mem = upb_value_memory(&from_value);
        void* to_mem = upb_value_memory(&to_value);
        memset(to_mem, 0, native_slot_size(to_map->value_type));

        native_slot_merge_by_array(value_field, from_mem,
                                   to_mem PHP_PROTO_TSRMLS_CC);

        map_index_set(to_map, key, key_length, to_value);
      }

    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      zval* to_array_php = CACHED_PTR_TO_ZVAL_PTR(DEREF(to_memory, CACHED_VALUE*));
      zval* from_array_php = CACHED_PTR_TO_ZVAL_PTR(DEREF(from_memory, CACHED_VALUE*));
      RepeatedField* to_array = UNBOX(RepeatedField, to_array_php);
      RepeatedField* from_array = UNBOX(RepeatedField, from_array_php);

      int size = zend_hash_num_elements(PHP_PROTO_HASH_OF(from_array->array));
      if (size > 0) {
        for (j = 0; j < size; j++) {
          void* from_memory = NULL;
          void* to_memory =
              ALLOC_N(char, native_slot_size(upb_fielddef_type(field)));
          memset(to_memory, 0, native_slot_size(upb_fielddef_type(field)));

          if (to_array->type == UPB_TYPE_MESSAGE) {
            php_proto_zend_hash_index_find_zval(
                PHP_PROTO_HASH_OF(from_array->array), j, (void**)&from_memory);
#if PHP_MAJOR_VERSION >= 7
            from_memory = &Z_OBJ_P((zval*)from_memory);
#endif
          } else {
            php_proto_zend_hash_index_find_mem(
                PHP_PROTO_HASH_OF(from_array->array), j, (void**)&from_memory);
          }

          native_slot_merge_by_array(field, from_memory,
                                     to_memory PHP_PROTO_TSRMLS_CC);
          repeated_field_push_native(to_array, to_memory);
          FREE(to_memory);
        }
      }
    } else {
      native_slot_merge(field, from_memory, to_memory PHP_PROTO_TSRMLS_CC);
    }
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
