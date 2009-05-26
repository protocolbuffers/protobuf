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
typedef enum upb_wire_type {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5,
} upb_wire_type_t;

struct upb_delimited {
  size_t offset;  /* relative to the beginning of the stream. */
  uint32_t len;
};

/* A value as it is encoded on-the-wire. */
union upb_wire_value {
  uint64_t varint;
  uint64_t _64bit;
  uint32_t _32bit;
  struct upb_delimited delimited;
};

/* A value as described in a .proto file. */
union upb_value {
  double _double;
  float  _float;
  int32_t int32;
  int64_t int64;
  uint32_t uint32;
  uint64_t uint64;
  bool _bool;
  struct upb_delimited delimited;
};

typedef int32_t upb_field_number_t;

/* A tag occurs before each value on-the-wire. */
struct upb_tag {
  upb_field_number_t field_number;
  upb_wire_type_t wire_type;
};

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
