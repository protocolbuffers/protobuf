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
 *
 * Ownership semantics:
 * - Each pbstruct absolutely owns any data allocated for its arrays or
 *   strings.  If you want a copy of this data that will outlive the struct,
 *   you must copy it out.
 * - Each pbstruct optionally owns any data allocated for substructs, based
 *   on whether you pass the free_substructs parameter to any of several
 *   functions.
 */

#ifndef PBSTRUCT_H_
#define PBSTRUCT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum pbstruct_type {
  PBSTRUCT_TYPE_DOUBLE = 1,
  PBSTRUCT_TYPE_FLOAT = 2,
  PBSTRUCT_TYPE_INT32 = 3,
  PBSTRUCT_TYPE_UINT32 = 4,
  PBSTRUCT_TYPE_INT64 = 5,
  PBSTRUCT_TYPE_UINT64 = 6,
  PBSTRUCT_TYPE_BOOL = 7,

  /* Main struct contains a pointer to a pbstruct. */
  PBSTRUCT_TYPE_SUBSTRUCT = 8,

  /* Main struct contains a pointer to a pbstruct_delimited. */
  PBSTRUCT_DYNAMIC = 16,
  PBSTRUCT_TYPE_BYTES     = PBSTRUCT_DYNAMIC | 1,
  PBSTRUCT_TYPE_STRING    = PBSTRUCT_DYNAMIC | 2,

  /* The main struct contains a pointer to a pbstruct_array, for which each
   * element is identical to the non-array form. */
  PBSTRUCT_ARRAY = PBSTRUCT_DYNAMIC | 32,  /* Not itself a real type. */
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

struct pbstruct_dynamic {
  size_t len;   /* Number of present elements (bytes for string/bytes). */
  size_t size;  /* Total size in present elements. */
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

struct pballoc {
  void *(*realloc)(void *ptr, size_t size, void *user_data);
  void (*free)(void *ptr, void *user_data);
};

/* Initializes everything to unset, with no dynamic memory alocated. */
void pbstruct_init(struct pbstruct *s, struct pbstruct_definition *d);
pbstruct *pbstruct_new(struct pbstruct_defintion *d, struct pballoc *a);
/* Frees the struct itself and any dynamic string or array data, and substructs
 * also if free_substructs is true. */
void pbstruct_free(struct pbstruct *s, struct pballoc *a, bool free_substructs);

/* For setting, clearing, and testing the "set" flag for each field.
 * Clearing the "set" flag does not free any dynamically allocated
 * storage -- it will be reused again if the field becomes set again. */
bool pbstruct_is_set(struct pbstruct *s, struct pbstruct_field *f);
void pbstruct_unset(struct pbstruct *s, struct pbstruct_field *f);
void pbstruct_unset_all(struct pbstruct *s);
void pbstruct_set(struct pbstruct *s, struct pbstruct_field *f);

/* Resizes the given string field to len, triggering a realloc() if
 * necessary.  Never reallocs() the string to be shorter. */
void pbstruct_resize_str(struct pbstruct *s, struct pbstruct_field *f,
                         size_t len, struct pballoc *a);

/* Appends an element to the given array field, triggering a realloc() if
 * necessary.  Returns the offset of the new array element. */
size_t pbstruct_append_array(struct pbstruct *s, struct pbstruct_field *f,
                             struct pballoc *a);

/* Sets the size of the given array field to zero, but does not free the
 * array itself (call pbstruct_free_field() for that).  Does free any
 * strings that were in the array (for string arrays), and any substructs
 * if free_substructs is true.  Does not affect the "set" flag (if the
 * field was previously set, it will remain set, but as zero elements long). */
void pbstruct_clear_array(struct pbstruct *s, struct pbstruct_field *f,
                          struct pballoc *a, bool free_substructs);

/* Resizes a delimited field (string or bytes) string that is within an array.
 * The array must already be large enough that str_offset is a valid member. */
void pbstruct_resize_arraystr(struct pbstruct *s, struct pbstruct_field *f,
                              size_t str_offset, size_t len, struct pballoc *a);

/* Like pbstruct_append_array, but appends a string of the specified length. */
size_t pbstruct_append_arraystr(struct pbstruct *s, struct pbstruct_field *f,
                                size_t len, struct pballoc *a);

/* Clears all data associated with f (like pbstruct_clear_array()), but also
 * frees all underlying storage.  For string array fields, also frees the
 * individual strings.  Clears the "set" flag since it is an error for an
 * array field to be set if it has no dynamic data allocated. */
void pbstruct_free_field(struct pbstruct *s, struct pbstruct_field *f,
                         bool free_substructs);
void pbstruct_free_all_fields(struct pbstruct *s, bool free_substructs);

/* These do no existence checks, bounds checks, or type checks. */
#define DECLARE_GETTERS(ctype, name) \
  ctype pbstruct_get_ ## name(struct pbstruct *s, \
                              struct pbstruct_field *f); \
  ctype *pbstruct_get_ ## name ## _ptr(struct pbstruct *s, \
                                       struct pbstruct_field *f); \
  ctype pbstruct_get_ ## name ## _array(struct pbstruct *s, \
                                        struct pbstruct_field *f, \
                                        int i); \
  ctype *pbstruct_get_ ## name ## _array_ptr(struct pbstruct *s, \
                                             struct pbstruct_field *f, \
                                             int i); \


DECLARE_GETTERS(double,   double)
DECLARE_GETTERS(float,    float)
DECLARE_GETTERS(int32_t,  int32)
DECLARE_GETTERS(int64_t,  int64)
DECLARE_GETTERS(uint32_t, uint32)
DECLARE_GETTERS(uint64_t, uint64)
DECLARE_GETTERS(bool,     bool)
DECLARE_GETTERS(struct pbstruct*, substruct)
DECLARE_GETTERS(struct pbstruct_delimited*, bytes)
DECLARE_GETTERS(struct pbstruct_delimited*, string)

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PBSTRUCT_H_ */
