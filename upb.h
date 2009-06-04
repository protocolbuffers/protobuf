/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_H_
#define UPB_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  /* for size_t. */

#ifdef __cplusplus
extern "C" {
#endif

/* The maximum that any submessages can be nested.  Matches proto2's limit. */
#define UPB_MAX_NESTING 64

/* A list of types as they are encoded on-the-wire. */
enum upb_wire_type {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5,
};
typedef int8_t upb_wire_type_t;

/* A value as it is encoded on-the-wire, except delimited, which is handled
 * separately. */
union upb_wire_value {
  uint64_t varint;
  uint64_t _64bit;
  uint32_t _32bit;
};

/* Value type as defined in a .proto file.  The values of this are defined by
 * google_protobuf_FieldDescriptorProto_Type (from descriptor.proto).  */
typedef int32_t upb_field_type_t;

/* A value as described in a .proto file, except delimited, which is handled
 * separately. */
union upb_value {
  double _double;
  float  _float;
  int32_t int32;
  int64_t int64;
  uint32_t uint32;
  uint64_t uint64;
  bool _bool;
  uint32_t delim_len;
};

/* The number of a field, eg. "optional string foo = 3". */
typedef int32_t upb_field_number_t;

/* A tag occurs before each value on-the-wire. */
struct upb_tag {
  upb_field_number_t field_number;
  upb_wire_type_t wire_type;
};

/* Status codes used as a return value. */
typedef enum upb_status {
  UPB_STATUS_OK = 0,
  UPB_STATUS_SUBMESSAGE_END = 1,

  /** FATAL ERRORS: these indicate corruption, and cannot be recovered. */

  // A varint did not terminate before hitting 64 bits.
  UPB_ERROR_UNTERMINATED_VARINT = -1,

  // A submessage ended in the middle of data.
  UPB_ERROR_BAD_SUBMESSAGE_END = -2,

  // Encountered a "group" on the wire (deprecated and unsupported).
  UPB_ERROR_GROUP = -3,

  // Input was nested more than UPB_MAX_NESTING deep.
  UPB_ERROR_STACK_OVERFLOW = -4,

  // The input data caused the pb's offset (a size_t) to overflow.
  UPB_ERROR_OVERFLOW = -5,

  // Generic error.
  UPB_ERROR = -6,

  /** NONFATAL ERRORS: the input was invalid, but we can continue if desired. */

  // A value was encountered that was not defined in the .proto file.
  UPB_ERROR_UNKNOWN_VALUE = 2,

  // A field was encoded with the wrong wire type.
  UPB_ERROR_MISMATCHED_TYPE = 3,
} upb_status_t;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
