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
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Branch prediction hints for GCC. */
#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

/* inline if possible, emit standalone code if required. */
#define INLINE static inline

/* The maximum that any submessages can be nested.  Matches proto2's limit. */
#define UPB_MAX_NESTING 64

/* The maximum number of fields that any one .proto type can have. */
#define UPB_MAX_FIELDS (1<<16)

INLINE uint32_t max(uint32_t a, uint32_t b) { return a > b ? a : b; }

/* Represents a string or bytes. */
struct upb_string {
  /* We expect the data to be 8-bit clean (uint8_t), but char* is such an
   * ingrained convention that we follow it. */
  char *ptr;
  uint32_t byte_len;
};

INLINE bool upb_streql(struct upb_string *s1, struct upb_string *s2) {
  return s1->byte_len == s2->byte_len &&
         memcmp(s1->ptr, s2->ptr, s1->byte_len) == 0;
}

INLINE void upb_strcpy(struct upb_string *dest, struct upb_string *src) {
  memcpy(dest->ptr, src->ptr, dest->byte_len);
  dest->byte_len = src->byte_len;
}

INLINE void upb_print(struct upb_string *str) {
  fwrite(str->ptr, str->byte_len, 1, stdout);
  fputc('\n', stdout);
}

#define UPB_STRLIT(strlit) {.ptr=strlit, .byte_len=sizeof(strlit)-1}
#define UPB_STRFARG(str) (str).byte_len, (str).ptr
#define UPB_STRFMT "%.*s"

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

/* A value as it is encoded on-the-wire, except delimited, which is handled
 * separately. */
union upb_wire_value {
  uint64_t varint;
  uint64_t _64bit;
  uint32_t _32bit;
};

/* Value type as defined in a .proto file.  The values of this are defined by
 * google_protobuf_FieldDescriptorProto_Type (from descriptor.proto).
 * Note that descriptor.proto reserves "0" for errors, and we use it to
 * represent exceptional circumstances. */
typedef uint8_t upb_field_type_t;

/* Label (optional, repeated, required) as defined in a .proto file.  The values
 * of this are defined by google.protobuf.FieldDescriptorProto.Label (from
 * descriptor.proto). */
typedef uint8_t  upb_label_t;

struct upb_type_info {
  uint8_t align;
  uint8_t size;
  uint8_t expected_wire_type;
};

/* This array is indexed by upb_field_type_t. */
extern struct upb_type_info upb_type_info[];

/* A scalar value as described in a .proto file */
union upb_value {
  double _double;
  float  _float;
  int32_t int32;
  int64_t int64;
  uint32_t uint32;
  uint64_t uint64;
  bool _bool;
};

union upb_value_ptr {
  double *_double;
  float  *_float;
  int32_t *int32;
  int64_t *int64;
  uint32_t *uint32;
  uint64_t *uint64;
  bool *_bool;
  struct upb_string **string;
  struct upb_array **array;
  void **message;
  void *_void;
};

/* The number of a field, eg. "optional string foo = 3". */
typedef int32_t upb_field_number_t;

/* A tag occurs before each value on-the-wire. */
struct upb_tag {
  upb_field_number_t field_number;
  upb_wire_type_t wire_type;
};

enum upb_symbol_type {
  UPB_SYM_MESSAGE,
  UPB_SYM_ENUM,
  UPB_SYM_SERVICE,
  UPB_SYM_EXTENSION
};

union upb_symbol_ref {
  struct upb_msg *msg;
  struct upb_enum *_enum;
  struct upb_svc *svc;
};

/* Status codes used as a return value. */
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
