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
#include "upb_atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

// inline if possible, emit standalone code if required.
#ifndef INLINE
#define INLINE static inline
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))
#define UPB_INDEX(base, i, m) (void*)((char*)(base) + ((i)*(m)))

// The maximum that any submessages can be nested.  Matches proto2's limit.
#define UPB_MAX_NESTING 64

// The maximum number of fields that any one .proto type can have.  Note that
// this is very different than the max field number.  It is hard to imagine a
// scenario where more than 32k fields makes sense.
#define UPB_MAX_FIELDS (1<<15)
typedef int16_t upb_field_count_t;

// Nested type names are separated by periods.
#define UPB_SYMBOL_SEPARATOR '.'

// This limit is for the longest fully-qualified symbol, eg. foo.bar.MsgType
#define UPB_SYMBOL_MAXLEN 128

// The longest chain that mutually-recursive types are allowed to form.  For
// example, this is a type cycle of length 2:
//   message A {
//     B b = 1;
//   }
//   message B {
//     A a = 1;
//   }
#define UPB_MAX_TYPE_CYCLE_LEN 16

// The maximum depth that the type graph can have.
#define UPB_MAX_TYPE_DEPTH 64

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
#define UPB_TYPE(type) GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## type
#define UPB_LABEL(type) GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_ ## type

INLINE bool upb_issubmsgtype(upb_field_type_t type) {
  return type == UPB_TYPE(GROUP) || type == UPB_TYPE(MESSAGE);
}

INLINE bool upb_isstringtype(upb_field_type_t type) {
  return type == UPB_TYPE(STRING) || type == UPB_TYPE(BYTES);
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

// INTERNAL-ONLY: never refer to these types with a tag ("union", "struct").
// Always use the typedefs.
union _upb_array;
struct _upb_msg;

typedef union _upb_array upb_array;
typedef struct _upb_msg upb_msg;
typedef upb_atomic_refcount_t upb_data;

typedef uint32_t upb_strlen_t;

// We have several different representations for string, depending on whether
// it has a refcount (and likely in the future, depending on whether it is a
// slice of another string).  We could just have one representation with
// members that are sometimes unused, but this is wasteful in memory.  The
// flags that are always part of the first word tell us which representation
// to use.
//
// In a way, this is like inheritance but instead of using a virtual pointer,
// we do switch/case in every "virtual" method.  This may sound expensive but
// in many cases the different cases compile to exactly the same code, so there
// is no branch.

typedef struct {
  uint32_t byte_size_and_flags;
  upb_strlen_t byte_len;
  // We expect the data to be 8-bit clean (uint8_t), but char* is such an
  // ingrained convention that we follow it.
  char *ptr;
} upb_norefcount_string;

// Used for a string with a refcount.
typedef struct {
  upb_data base;
  upb_strlen_t byte_len;
  char *ptr;
  uint32_t byte_size;
} upb_refcounted_string;

typedef union {
  // Must be first, for the UPB_STATIC_STRING_PTR_INIT() macro.
  upb_norefcount_string *norefcount;
  upb_data *base;
  upb_refcounted_string *refcounted;
} upb_strptr;

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
  upb_strptr str;
  upb_array *arr;
  upb_msg *msg;
  upb_data *data;
};

// A pointer to a .proto value.  The owner must have an out-of-band way of
// knowing the type, so it knows which union member to use.
union upb_value_ptr {
  double *_double;
  float *_float;
  int32_t *int32;
  int64_t *int64;
  uint8_t *uint8;
  uint32_t *uint32;
  uint64_t *uint64;
  bool *_bool;
  upb_strptr *str;
  upb_array **arr;
  upb_msg **msg;
  upb_data **data;
  void *_void;
};

INLINE union upb_value_ptr upb_value_addrof(union upb_value *val) {
  union upb_value_ptr ptr = {&val->_double};
  return ptr;
}

/**
 * Converts upb_value_ptr -> upb_value by reading from the pointer.  We need to
 * know the field type to perform this operation, because we need to know how
 * much memory to copy.
 */
INLINE union upb_value upb_value_read(union upb_value_ptr ptr,
                                      upb_field_type_t ft) {
  union upb_value val;

#define CASE(t, member_name) \
  case UPB_TYPE(t): val.member_name = *ptr.member_name; break;

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
  case UPB_TYPE(t): *ptr.member_name = val.member_name; break;

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

// Status codes used as a return value.  Codes >0 are not fatal and can be
// resumed.
enum upb_status_code {
  UPB_STATUS_OK = 0,

  // The input byte stream ended in the middle of a record.
  UPB_STATUS_NEED_MORE_DATA = 1,

  // The user value callback opted to stop parsing.
  UPB_STATUS_USER_CANCELLED = 2,

  // An unrecoverable error occurred.
  UPB_STATUS_ERROR = -1,

  // A varint went for 10 bytes without terminating.
  UPB_ERROR_UNTERMINATED_VARINT = -2
};

#define UPB_ERRORMSG_MAXLEN 256
struct upb_status {
  enum upb_status_code code;
  char msg[UPB_ERRORMSG_MAXLEN];
};

#define UPB_STATUS_INIT {UPB_STATUS_OK, ""}

INLINE bool upb_ok(struct upb_status *status) {
  return status->code == UPB_STATUS_OK;
}

INLINE void upb_reset(struct upb_status *status) {
  status->code = UPB_STATUS_OK;
  status->msg[0] = '\0';
}

void upb_seterr(struct upb_status *status, enum upb_status_code code,
                const char *msg, ...);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
