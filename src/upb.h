/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * This file contains shared definitions that are widely used across upb.
 */

#ifndef UPB_H_
#define UPB_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // only for size_t.
#include "descriptor_const.h"

#ifdef __cplusplus
extern "C" {
#endif

// inline if possible, emit standalone code if required.
#ifndef INLINE
#define INLINE static inline
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

// The maximum that any submessages can be nested.  Matches proto2's limit.
#define UPB_MAX_NESTING 64

// The maximum number of fields that any one .proto type can have.
#define UPB_MAX_FIELDS (1<<16)

// Nested type names are separated by periods.
#define UPB_SYMBOL_SEPARATOR '.'
#define UPB_SYMBOL_MAX_LENGTH 256

#define UPB_INDEX(base, i, m) (void*)((char*)(base) + ((i)*(m)))


/* Fundamental types and type constants. **************************************/

// A list of types as they are encoded on-the-wire.
enum upb_wire_type {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
};

typedef uint8_t upb_wire_type_t;

// Value type as defined in a .proto file.  eg. string, int32, etc.  The
// integers that represent this are defined by descriptor.proto.  Note that
// descriptor.proto reserves "0" for errors, and we use it to represent
// exceptional circumstances.
typedef uint8_t upb_field_type_t;

// For referencing the type constants tersely.
#define UPB_TYPENUM(type) GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## type

INLINE bool upb_issubmsgtype(upb_field_type_t type) {
  return type == UPB_TYPENUM(GROUP) || type == UPB_TYPENUM(MESSAGE);
}

INLINE bool upb_isstringtype(upb_field_type_t type) {
  return type == UPB_TYPENUM(STRING) || type == UPB_TYPENUM(BYTES);
}

// Info for a given field type.
struct upb_type_info {
  uint8_t align;
  uint8_t size;
  upb_wire_type_t expected_wire_type;
  char *ctype;
};

// A static array of info about all of the field types, indexed by type number.
extern struct upb_type_info upb_type_info[];

// The number of a field, eg. "optional string foo = 3".
typedef int32_t upb_field_number_t;

// Label (optional, repeated, required) as defined in a .proto file.  The
// values of this are defined by google.protobuf.FieldDescriptorProto.Label
// (from descriptor.proto).
typedef uint8_t  upb_label_t;

// A scalar (non-string) wire value.  Used only for parsing unknown fields.
union upb_wire_value {
  uint64_t varint;
  uint64_t _64bit;
  uint32_t _32bit;
};

// A tag occurs before each value on-the-wire.
struct upb_tag {
  upb_field_number_t field_number;
  upb_wire_type_t wire_type;
};


/* Polymorphic values of .proto types *****************************************/

struct upb_string;
struct upb_array;
struct upb_msg;

// A single .proto value.  The owner must have an out-of-band way of knowing
// the type, so that it knows which union member to use.
union upb_value {
  double _double;
  float _float;
  int32_t int32;
  int64_t int64;
  uint32_t uint32;
  uint64_t uint64;
  bool _bool;
  struct upb_string *str;
  struct upb_array *arr;
  struct upb_msg *msg;
};

// A pointer to a .proto value.  The owner must have an out-of-band way of
// knowing the type, so it knows which union member to use.
union upb_value_ptr {
  double *_double;
  float *_float;
  int32_t *int32;
  int64_t *int64;
  uint32_t *uint32;
  uint64_t *uint64;
  bool *_bool;
  struct upb_string **str;
  struct upb_array **arr;
  struct upb_msg **msg;
  void *_void;
};

// Unfortunately there is no way to define this so that it can be used as a
// generic expression, a la:
//   foo(UPB_VALUE_ADDROF(bar));
// ...you have to use it as the initializer of a upb_value_ptr:
//   union upb_value_ptr p = UPB_VALUE_ADDROF(bar);
//   foo(p);
#define UPB_VALUE_ADDROF(val) {(void*)&val._double}

/**
 * Converts upb_value_ptr -> upb_value by reading from the pointer.  We need to
 * know the field type to perform this operation, because we need to know how
 * much memory to copy.
 */
INLINE union upb_value upb_value_read(union upb_value_ptr ptr,
                                      upb_field_type_t ft) {
  union upb_value val;

#define CASE(t, member_name) \
  case UPB_TYPENUM(t): val.member_name = *ptr.member_name; break;

  switch(ft) {
    CASE(DOUBLE,   _double)
    CASE(FLOAT,    _float)
    CASE(INT32,    int32)
    CASE(INT64,    int64)
    CASE(UINT32,   uint32)
    CASE(UINT64,   uint64)
    CASE(SINT32,   int32)
    CASE(SINT64,   int64)
    CASE(FIXED32,  uint32)
    CASE(FIXED64,  uint64)
    CASE(SFIXED32, int32)
    CASE(SFIXED64, int64)
    CASE(BOOL,     _bool)
    CASE(ENUM,     int32)
    CASE(STRING,   str)
    CASE(BYTES,    str)
    CASE(MESSAGE,  msg)
    CASE(GROUP,    msg)
    default: break;
  }
  return val;

#undef CASE
}

/**
 * Writes a upb_value to a upb_value_ptr location. We need to know the field
 * type to perform this operation, because we need to know how much memory to
 * copy.
 */
INLINE void upb_value_write(union upb_value_ptr ptr, union upb_value val,
                            upb_field_type_t ft) {
#define CASE(t, member_name) \
  case UPB_TYPENUM(t): *ptr.member_name = val.member_name; break;

  switch(ft) {
    CASE(DOUBLE,   _double)
    CASE(FLOAT,    _float)
    CASE(INT32,    int32)
    CASE(INT64,    int64)
    CASE(UINT32,   uint32)
    CASE(UINT64,   uint64)
    CASE(SINT32,   int32)
    CASE(SINT64,   int64)
    CASE(FIXED32,  uint32)
    CASE(FIXED64,  uint64)
    CASE(SFIXED32, int32)
    CASE(SFIXED64, int64)
    CASE(BOOL,     _bool)
    CASE(ENUM,     int32)
    CASE(STRING,   str)
    CASE(BYTES,    str)
    CASE(MESSAGE,  msg)
    CASE(GROUP,    msg)
    default: break;
  }

#undef CASE
}

// All the different definitions that can occur in .proto files.
union upb_symbol_ref {
  struct upb_msgdef *msg;
  struct upb_enum *_enum;
  struct upb_svc *svc;
};

// Status codes used as a return value.  Codes >0 are not fatal and can be
// resumed.
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
