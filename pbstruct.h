/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * pbstruct is an efficient, compact, self-describing format for storing
 * pb messages in-memory.  It can be used when parsing a protobuf into
 * a structure is desired, but both the parsing and emitting modules can
 * be used without ever using pbstruct.
 */

#include <stdbool.h>
#include <stdint.h>

typedef enum pbstruct_type {
  PBSTRUCT_TYPE_DOUBLE = 1,
  PBSTRUCT_TYPE_FLOAT = 2,
  PBSTRUCT_TYPE_INT32 = 3,
  PBSTRUCT_TYPE_UINT32 = 4,
  PBSTRUCT_TYPE_INT64 = 5,
  PBSTRUCT_TYPE_UINT64 = 6,
  PBSTRUCT_TYPE_BOOL = 7,

  /* For these types, the main struct contains a pointer to the real data. */
  PBSTRUCT_TYPE_BYTES = 8,
  PBSTRUCT_TYPE_STRING = 9,
  PBSTRUCT_TYPE_SUBSTRUCT = 10,

  /* The array types are just like the primitive types, except the main struct
   * contains a pointer to an array of the primitive type. */
  PBSTRUCT_ARRAY = 16,  /* Not itself a real type. */
  PBSTRUCT_TYPE_DOUBLE_ARRAY    = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_DOUBLE,
  PBSTRUCT_TYPE_FLOAT_ARRAY     = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_FLOAT,
  PBSTRUCT_TYPE_INT32_ARRAY     = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_INT32,
  PBSTRUCT_TYPE_UINT32_ARRAY    = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_UINT32,
  PBSTRUCT_TYPE_INT64_ARRAY     = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_INT64,
  PBSTRUCT_TYPE_UINT64_ARRAY    = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_UINT64,
  PBSTRUCT_TYPE_BOOL_ARRAY      = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_BOOL,
  PBSTRUCT_TYPE_BYTES_ARRAY     = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_BYTES,
  PBSTRUCT_TYPE_STRING_ARRAY    = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_STRING,
  PBSTRUCT_TYPE_SUBSTRUCT_ARRAY = PBSTRUCT_ARRAY | PBSTRUCT_TYPE_SUBSTRUCT,
} pbstruct_type_t;

struct pbstruct_field {
  char *name;
  pbstruct_type_t type;
  ptrdiff_t byte_offset;
  ptrdiff_t isset_byte_offset;
  uint8_t isset_byte_mask;
};

struct pbstruct_delimited {
  size_t len;
  char data[];
};

struct pbstruct_definition {
  char *name;
  size_t struct_size;
  int num_fields;
  int num_required_fields;
  struct pbstruct_field fields[];
};

struct pbstruct {
  struct pbstruct_definition *definition;
  uint8_t data[];
};

struct pbstruct* pbstruct_new(struct pbstruct_definition *definition);
/* Deletes any sub-structs also. */
struct pbstruct* pbstruct_delete(struct pbstruct_definition *definition);

inline bool pbstruct_is_set(struct pbstruct *s, struct pbstruct_field *f) {
  return s->data[f->isset_byte_offset] & f->isset_byte_mask;
}

/* These do no existence checks or type checks. */
#define DEFINE_GETTERS(ctype, name) \
  inline ctype pbstruct_get_ ## name(struct pbstruct *s, struct pbstruct_field *f) { \
    /* TODO: make sure the compiler knows this is an aligned access. */ \
    return *(ctype*)(s->data + f->byte_offset); \
  } \
  inline ctype *pbstruct_get_ ## name ## _array(struct pbstruct *s, \
                                         struct pbstruct_field *f) { \
    /* TODO: make sure the compiler knows this is an aligned access. */ \
    return *(ctype**)(s->data + f->byte_offset); \
  }

DEFINE_GETTERS(double,   double)
DEFINE_GETTERS(float,    float)
DEFINE_GETTERS(int32_t,  int32)
DEFINE_GETTERS(int64_t,  int64)
DEFINE_GETTERS(uint32_t, uint32)
DEFINE_GETTERS(uint64_t, uint64)
DEFINE_GETTERS(bool,     bool)
DEFINE_GETTERS(struct pbstruct_delimited*, bytes)
DEFINE_GETTERS(struct pbstruct_delimited*, string)
DEFINE_GETTERS(struct pbstruct*, substruct)

