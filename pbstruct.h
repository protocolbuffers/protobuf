/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * pbstruct is an efficient, compact, self-describing format for storing
 * protobufs in-memory.  In many ways it is a dynamic implementation of C
 * structures, which nonetheless is efficient because it does offset-based (as
 * opposed to name-based) access.  It can be used when representing a message in
 * memory is desired, but both the parsing and emitting modules can be used
 * without ever using pbstruct.
 *
 * There is one fundamental choice pbstruct is forced to make: whether to
 * include substructures by value or by reference.  Supporting both doesn't
 * seem worth the complexity.  The tradeoffs are:
 *
 * +value:
 *  - fewer malloc/free calls
 *  - better locality
 *  - one less deref to access data
 *  - simpler to delete/GC (since the data can't nest arbitrarily)
 * +ref:
 *  - you pay only sizeof(void*) for unused fields, not sizeof(struct)
 *  - you can keep substructs around while deleting the enclosing struct
 *  - copies of the surrounding struct are cheaper (including array reallocs)
 *
 * My view of these tradeoffs is that while by value semantics could yield
 * small performance gains in its best case, and is somewhat simpler, it has
 * much worse degenerate cases.  For example, consider a protobuf like that is
 * only meant to contain one of several possible large structures:
 *
 * message {
 *   optional LargeMessage1 m1 = 1;
 *   optional LargeMessage2 m2 = 2;
 *   optional LargeMessage3 m3 = 2;
 *   // ...
 * }
 *
 * This proto will take N times more memory if structures are by value than if
 * they are by reference.  To avoid such bad cases, submessages are by
 * reference.
 */

#ifndef PBSTRUCT_H_
#define PBSTRUCT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum pbstruct_type {
  PBSTRUCT_TYPE_DOUBLE = 0,
  PBSTRUCT_TYPE_FLOAT = 1,
  PBSTRUCT_TYPE_INT32 = 2,
  PBSTRUCT_TYPE_UINT32 = 3,
  PBSTRUCT_TYPE_INT64 = 4,
  PBSTRUCT_TYPE_UINT64 = 5,
  PBSTRUCT_TYPE_BOOL = 6,
  /* Main struct contains a pointer to a pbstruct. */
  PBSTRUCT_TYPE_SUBSTRUCT = 7,

  /* Main struct contains a pointer to a pbstruct_delimited. */
  PBSTRUCT_DELIMITED = 8,  /* Not itself a real type. */
  PBSTRUCT_TYPE_BYTES     = PBSTRUCT_DELIMITED | 1,
  PBSTRUCT_TYPE_STRING    = PBSTRUCT_DELIMITED | 2,

  /* The main struct contains a pointer to a pbstruct_array, for which each
   * element is identical to the non-array form. */
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
  size_t len;  /* Measured in bytes. */
  uint8_t data[];
};

struct pbstruct_array {
  size_t len;  /* Measured in elements. */
  uint8_t data[];
};

struct pbstruct_definition {
  char *name;
  size_t struct_size;
  int num_fields;
  int num_required_fields;  /* Required fields have the lowest set bytemasks. */
  struct pbstruct_field fields[];
};

struct pbstruct {
  struct pbstruct_definition *definition;
  uint8_t data[];  /* layout is described by definition. */
};

/* Initializes everything to unset. */
void pbstruct_init(struct pbstruct *s, struct pbstruct_definition *definition);
bool pbstruct_is_set(struct pbstruct *s, struct pbstruct_field *f);
/* Doesn't check whether the field is holding allocated memory, and will leak
 * the memory if it is. */
void pbstruct_unset(struct pbstruct *s, struct pbstruct_field *f);
void pbstruct_set(struct pbstruct *s, struct pbstruct_field *f);

/* These do no existence checks or type checks. */
#define DECLARE_GETTERS(ctype, name) \
  ctype *pbstruct_get_ ## name(struct pbstruct *s, struct pbstruct_field *f);

DECLARE_GETTERS(double,   double)
DECLARE_GETTERS(float,    float)
DECLARE_GETTERS(int32_t,  int32)
DECLARE_GETTERS(int64_t,  int64)
DECLARE_GETTERS(uint32_t, uint32)
DECLARE_GETTERS(uint64_t, uint64)
DECLARE_GETTERS(bool,     bool)
DECLARE_GETTERS(struct pbstruct_delimited*, bytes)
DECLARE_GETTERS(struct pbstruct_delimited*, string)
DECLARE_GETTERS(struct pbstruct*, substruct)

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PBSTRUCT_H_ */
