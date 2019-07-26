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

/* TODO(haberman): this is a mess.  It will improve when upb_array no longer
 * carries reflective state (type, elem_size). */
UPB_INLINE void *_upb_array_resize_accessor(void *msg, size_t ofs, size_t size,
                                            size_t elem_size,
                                            upb_fieldtype_t type,
                                            upb_arena *arena) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);

  if (!arr) {
    arr = upb_array_new(arena);
    if (!arr) return NULL;
    *PTR_AT(msg, ofs, upb_array*) = arr;
  }

  if (size > arr->size) {
    size_t new_size = UPB_MAX(arr->size, 4);
    size_t old_bytes = arr->size * elem_size;
    size_t new_bytes;
    while (new_size < size) new_size *= 2;
    new_bytes = new_size * elem_size;
    arr->data = upb_arena_realloc(arena, arr->data, old_bytes, new_bytes);
    if (!arr->data) {
      return NULL;
    }
    arr->size = new_size;
  }

  arr->len = size;
  return arr->data;
}

UPB_INLINE bool _upb_array_append_accessor(void *msg, size_t ofs,
                                           size_t elem_size,
                                           upb_fieldtype_t type,
                                           const void *value,
                                           upb_arena *arena) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);
  size_t i = arr ? arr->len : 0;
  void *data =
      _upb_array_resize_accessor(msg, ofs, i + 1, elem_size, type, arena);
  if (!data) return false;
  memcpy(PTR_AT(data, i * elem_size, char), value, elem_size);
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
