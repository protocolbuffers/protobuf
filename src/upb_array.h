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
#include "upb_msg.h"  /* Because we use upb_msg_fielddef */

#ifdef __cplusplus
extern "C" {
#endif

struct upb_string;

/* Returns a pointer to an array element.  Does not perform a bounds check! */
INLINE union upb_value_ptr upb_array_getelementptr(
    struct upb_array *arr, upb_arraylen_t n, upb_field_type_t type)
{
  union upb_value_ptr ptr;
  ptr._void = (void*)((char*)arr->elements._void + n*upb_type_info[type].size);
  return ptr;
}

/* Allocation/Deallocation/Resizing. ******************************************/

INLINE struct upb_array *upb_array_new(struct upb_msg_fielddef *f)
{
  struct upb_array *arr = malloc(sizeof(*arr));
  upb_mmhead_init(&arr->mmhead);
  arr->elements._void = NULL;
  arr->len = 0;
  arr->size = 0;
  arr->fielddef = f;
  return arr;
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

/* Resizes array to be "len" elements long (reallocating if necessary). */
INLINE bool upb_array_resize(struct upb_array *arr, upb_arraylen_t newlen)
{
  size_t type_size = upb_type_info[arr->fielddef->type].size;
  bool dropped = false;
  bool ref = arr->size == 0;   /* Ref'ing external memory. */
  void *data = arr->elements._void;
  if(arr->size < newlen) {
    /* Need to resize. */
    arr->size = UPB_MAX(4, upb_round_up_to_pow2(newlen));
    arr->elements._void = realloc(ref ? NULL : data, arr->size * type_size);
  }
  if(ref) {
    /* Need to take referenced data and copy it to memory we own. */
    memcpy(arr->elements._void, data, UPB_MIN(arr->len, newlen) * type_size);
    dropped = true;
  }
  /* TODO: fill with defaults. */
  arr->len = newlen;
  return dropped;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
