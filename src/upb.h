/*
 * upb - a minimalist implementation of protocol buffers.

 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * This file contains shared definitions that are widely used across upb.
 */

#ifndef UPB_H_
#define UPB_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  /* for size_t. */
#include "upb_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/* inline if possible, emit standalone code if required. */
#ifndef INLINE
#define INLINE static inline
#endif

/* The maximum that any submessages can be nested.  Matches proto2's limit. */
#define UPB_MAX_NESTING 64

/* The maximum number of fields that any one .proto type can have. */
#define UPB_MAX_FIELDS (1<<16)

/* Nested type names are separated by periods. */
#define UPB_SYMBOL_SEPARATOR '.'
#define UPB_SYMBOL_MAX_LENGTH 256

#define UPB_INDEX(base, i, m) (void*)((char*)(base) + ((i)*(m)))

/* Fundamental types and type constants. **************************************/

/* A list of types as they are encoded on-the-wire. */
enum upb_wire_type {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
};
typedef uint8_t upb_wire_type_t;

/* Value type as defined in a .proto file.  eg. string, int32, etc.
 *
 * The values of this are defined by google_protobuf_FieldDescriptorProto_Type
 * (from descriptor.proto).  Note that descriptor.proto reserves "0" for
 * errors, and we use it to represent exceptional circumstances. */
typedef uint8_t upb_field_type_t;

/* Information about a given value type (upb_field_type_t). */
struct upb_type_info {
  uint8_t align;
  uint8_t size;
  upb_wire_type_t expected_wire_type;
  struct upb_string ctype;
};

/* Contains information for all .proto types.  Indexed by upb_field_type_t. */
extern struct upb_type_info upb_type_info[];

/* The number of a field, eg. "optional string foo = 3". */
typedef int32_t upb_field_number_t;

/* Label (optional, repeated, required) as defined in a .proto file.  The values
 * of this are defined by google.protobuf.FieldDescriptorProto.Label (from
 * descriptor.proto). */
typedef uint8_t  upb_label_t;

/* A value as it is encoded on-the-wire, except delimited, which is handled
 * separately. */
union upb_wire_value {
  uint64_t varint;
  uint64_t _64bit;
  uint32_t _32bit;
};

/* A tag occurs before each value on-the-wire. */
struct upb_tag {
  upb_field_number_t field_number;
  upb_wire_type_t wire_type;
};

/* Polymorphic values of .proto types *****************************************/

/* A single .proto value.  The owner must have an out-of-band way of knowing
 * the type, so that it knows which union member to use. */
union upb_value {
  double   _double;
  float    _float;
  int32_t  int32;
  int64_t  int64;
  uint32_t uint32;
  uint64_t uint64;
  bool     _bool;
  struct upb_string *str;
  struct upb_array *arr;
  struct upb_msg *msg;
};

/* A pointer to a .proto value.  The owner must have an out-of-band way of
 * knowing the type, so it knows which union member to use. */
union upb_value_ptr {
  double   *_double;
  float    *_float;
  int32_t  *int32;
  int64_t  *int64;
  uint32_t *uint32;
  uint64_t *uint64;
  bool     *_bool;
  struct upb_string **str;
  struct upb_array **arr;
  struct upb_msg **msg;
  void     *_void;
};

/* Converts upb_value_ptr -> upb_value by "dereferencing" the pointer.  We need
 * to know the field type to perform this operation, because we need to know
 * how much memory to copy. */
INLINE union upb_value upb_deref(union upb_value_ptr ptr, upb_field_type_t t) {
  union upb_value val;
  memcpy(&val, ptr._void, upb_type_info[t].size);
  return val;
}

union upb_symbol_ref {
  struct upb_msgdef *msg;
  struct upb_enum *_enum;
  struct upb_svc *svc;
};

/* Status codes used as a return value.  Codes >0 are not fatal and can be
 * resumed. */
typedef enum upb_status {
  UPB_STATUS_OK = 0,

  // The input byte stream ended in the middle of a record.
  UPB_STATUS_NEED_MORE_DATA = 1,

  // The user value callback opted to stop parsing.
  UPB_STATUS_USER_CANCELLED = 2,

  // A varint did not terminate before hitting 64 bits.
  UPB_ERROR_UNTERMINATED_VARINT = -1,

  // A submessage or packed array ended in the middle of data.
  UPB_ERROR_BAD_SUBMESSAGE_END = -2,

  // Input was nested more than UPB_MAX_NESTING deep.
  UPB_ERROR_STACK_OVERFLOW = -3,

  // The input data caused the pb's offset (a size_t) to overflow.
  UPB_ERROR_OVERFLOW = -4,

  // An "end group" tag was encountered in an inappropriate place.
  UPB_ERROR_SPURIOUS_END_GROUP = -5,

  UPB_ERROR_ILLEGAL = -6
} upb_status_t;

#define UPB_CHECK(func) do { \
  upb_status_t status = func; \
  if(status != UPB_STATUS_OK) return status; \
  } while (0)

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
