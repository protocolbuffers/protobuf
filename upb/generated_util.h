/*
** Functions for use by generated code.  These are not public and users must
** not call them directly.
*/

#ifndef UPB_GENERATED_UTIL_H_
#define UPB_GENERATED_UTIL_H_

#include <stdint.h>
#include "upb/msg.h"

#include "upb/port_def.inc"

#define PTR_AT(msg, ofs, type) (type*)((const char*)msg + ofs)

UPB_INLINE const void *_upb_array_accessor(const void *msg, size_t ofs,
                                           size_t *size) {
  const upb_array *arr = *PTR_AT(msg, ofs, const upb_array*);
  if (arr) {
    if (size) *size = arr->len;
    return arr->data;
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void *_upb_array_mutable_accessor(void *msg, size_t ofs,
                                             size_t *size) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);
  if (arr) {
    if (size) *size = arr->len;
    return arr->data;
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void *_upb_array_resize_accessor(void *msg, size_t ofs, size_t size,
                                            size_t elem_size,
                                            upb_arena *arena) {
  upb_array **arr_ptr = PTR_AT(msg, ofs, upb_array*);
  upb_array *arr = *arr_ptr;
  if (!arr || arr->size < size) {
    return _upb_array_resize_fallback(arr_ptr, size, elem_size, arena);
  }
  arr->len = size;
  return arr->data;
}

UPB_INLINE bool _upb_array_append_accessor(void *msg, size_t ofs,
                                           size_t elem_size,
                                           const void *value,
                                           upb_arena *arena) {
  upb_array **arr_ptr = PTR_AT(msg, ofs, upb_array*);
  upb_array *arr = *arr_ptr;
  if (!arr || arr->len == arr->size) {
    return _upb_array_append_fallback(arr_ptr, elem_size, value, arena);
  }
  memcpy(PTR_AT(arr->data, arr->len * elem_size, char), value, elem_size);
  arr->len++;
  return true;
}

UPB_INLINE bool _upb_has_field(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, const char) & (1 << (idx % 8))) != 0;
}

UPB_INLINE bool _upb_sethas(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, char)) |= (char)(1 << (idx % 8));
}

UPB_INLINE bool _upb_clearhas(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, char)) &= (char)(~(1 << (idx % 8)));
}

UPB_INLINE bool _upb_has_oneof_field(const void *msg, size_t case_ofs, int32_t num) {
  return *PTR_AT(msg, case_ofs, int32_t) == num;
}

#undef PTR_AT

#include "upb/port_undef.inc"

#endif  /* UPB_GENERATED_UTIL_H_ */
