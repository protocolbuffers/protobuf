/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * Defines an in-memory, polymorphic array type.  The array does not know its
 * own type -- its owner must know that information out-of-band.
 *
 * upb_arrays are memory-managed in the sense that they contain a pointer
 * ("mem") to memory that is "owned" by the array (which may be NULL if the
 * array owns no memory).  There is a separate pointer ("elements") that points
 * to the the array's currently "effective" memory, which is either equal to
 * mem (if the array's current value is memory we own) or not (if the array is
 * referencing other memory).
 *
 * If the array is referencing other memory, it is up to the array's owner to
 * ensure that the other memory remains valid for as long as the array is
 * referencing it.
 *
 */

#ifndef UPB_ARRAY_H_
#define UPB_ARRAY_H_

#include <stdlib.h>
#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct upb_string;

/* upb_arrays can be at most 2**32 elements long. */
typedef uint32_t upb_arraylen_t;

/* Represents an array (a repeated field) of any type.  The interpretation of
 * the data in the array depends on the type. */
struct upb_array {
  union upb_value_ptr elements;
  void *mem;
  upb_arraylen_t len;     /* Number of elements in "elements". */
  upb_arraylen_t size;    /* Memory allocated in "mem" (measured in elements) */
};

INLINE void upb_array_init(struct upb_array *arr)
{
  arr->elements._void = NULL;
  arr->mem = NULL;
  arr->len = 0;
  arr->size = 0;
}

INLINE void upb_array_free(struct upb_array *arr)
{
  free(arr->mem);
}

/* Returns a pointer to an array element.  Does not perform a bounds check! */
INLINE union upb_value_ptr upb_array_getelementptr(
    struct upb_array *arr, upb_arraylen_t n, upb_field_type_t type)
{
  union upb_value_ptr ptr;
  ptr._void = (void*)((char*)arr->elements._void + n*upb_type_info[type].size);
  return ptr;
}

INLINE union upb_value upb_array_getelement(
    struct upb_array *arr, upb_arraylen_t n, upb_field_type_t type)
{
  return upb_deref(upb_array_getelementptr(arr, n, type), type);
}

INLINE uint32_t upb_round_up_to_pow2(uint32_t v)
{
  /* cf. http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

/* Resizes array to be "len" elements long and ensures we have write access
 * to the array (reallocating if necessary).  Returns true iff we were
 * referencing memory for the array and dropped the reference. */
INLINE bool upb_array_resize(struct upb_array *arr, upb_arraylen_t newlen,
                             upb_field_type_t type)
{
  size_t type_size = upb_type_info[type].size;
  bool dropped = false;
  bool ref = arr->elements._void != arr->mem;  /* Ref'ing external memory. */
  if(arr->size < newlen) {
    /* Need to resize. */
    arr->size = max(4, upb_round_up_to_pow2(newlen));
    arr->mem = realloc(arr->mem, arr->size * type_size);
  }
  if(ref) {
    /* Need to take referenced data and copy it to memory we own. */
    memcpy(arr->mem, arr->elements._void, UPB_MIN(arr->len, newlen) * type_size);
    dropped = true;
  }
  arr->elements._void = arr->mem;
  arr->len = newlen;
  return dropped;
}

/* These are all overlays on upb_array, pointers between them can be cast. */
#define UPB_DEFINE_ARRAY_TYPE(name, type) \
  struct name ## _array { \
    type *elements; \
    type *mem; \
    upb_arraylen_t len; \
    upb_arraylen_t size; \
  };

UPB_DEFINE_ARRAY_TYPE(upb_double, double)
UPB_DEFINE_ARRAY_TYPE(upb_float,  float)
UPB_DEFINE_ARRAY_TYPE(upb_int32,  int32_t)
UPB_DEFINE_ARRAY_TYPE(upb_int64,  int64_t)
UPB_DEFINE_ARRAY_TYPE(upb_uint32, uint32_t)
UPB_DEFINE_ARRAY_TYPE(upb_uint64, uint64_t)
UPB_DEFINE_ARRAY_TYPE(upb_bool,   bool)
UPB_DEFINE_ARRAY_TYPE(upb_string, struct upb_string*)
UPB_DEFINE_ARRAY_TYPE(upb_msg,    void*)

/* Defines an array of a specific message type (an overlay of upb_array). */
#define UPB_MSG_ARRAY(msg_type) struct msg_type ## _array
#define UPB_DEFINE_MSG_ARRAY(msg_type) \
  UPB_MSG_ARRAY(msg_type) { \
    msg_type **elements; \
    msg_type **mem; \
    upb_arraylen_t len; \
    upb_arraylen_t size; \
  };

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
