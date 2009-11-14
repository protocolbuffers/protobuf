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
#include "upb_def.h"  /* Because we use upb_fielddef */

#ifdef __cplusplus
extern "C" {
#endif

struct upb_string;

/* Returns a pointer to an array element.  Does not perform a bounds check! */
INLINE union upb_value_ptr upb_array_getelementptr(struct upb_array *arr,
                                                   upb_arraylen_t n)
{
  union upb_value_ptr ptr;
  ptr._void = UPB_INDEX(arr->elements._void, n,
                        upb_type_info[arr->fielddef->type].size);
  return ptr;
}

/* Allocation/Deallocation/Resizing. ******************************************/

INLINE struct upb_array *upb_array_new(struct upb_fielddef *f)
{
  struct upb_array *arr = (struct upb_array*)malloc(sizeof(*arr));
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

INLINE union upb_value_ptr upb_array_append(struct upb_array *arr)
{
  size_t size = upb_type_info[arr->fielddef->type].size;
  if(arr->len == arr->size) {
    arr->size = UPB_MAX(4, upb_round_up_to_pow2(arr->len + 1));
    arr->elements._void = realloc(arr->elements._void, arr->size * size);
    memset((char*)arr->elements._void + (arr->len * size), 0,
           (arr->size - arr->len) * size);
  }
  return upb_array_getelementptr(arr, arr->len++);
}

INLINE void upb_array_truncate(struct upb_array *arr)
{
  arr->len = 0;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
