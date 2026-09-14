// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_ARRAY_H_
#define UPB_MESSAGE_INTERNAL_ARRAY_H_

#include <stdint.h>
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/port/overflow.h"

// Must be last.
#include "upb/port/def.inc"

#define _UPB_ARRAY_MASK_IMM 0x4  // Frozen/immutable bit.
#define _UPB_ARRAY_MASK_LG2 0x3  // Encoded elem size.
#define _UPB_ARRAY_MASK_ALL (_UPB_ARRAY_MASK_IMM | _UPB_ARRAY_MASK_LG2)

#ifdef __cplusplus
extern "C" {
#endif

#define _UPB_ARRAY_DEFAULT_INITIAL_SIZE 4

// LINT.IfChange(upb_Array)

// Our internal representation for repeated fields.
struct UPB_ALIGN_AS(UPB_MALLOC_ALIGN) upb_Array {
  // This is a tagged pointer. Bits #0 and #1 encode the elem size as follows:
  //   0 maps to elem size 1
  //   1 maps to elem size 4
  //   2 maps to elem size 8
  //   3 maps to elem size 16
  //
  // Bit #2 contains the frozen/immutable flag.
  uintptr_t UPB_ONLYBITS(data);

#if UPB_FUTURE_32BIT_ARRAY
  uint32_t UPB_ONLYBITS(size);     // The number of elements in the array.
  uint32_t UPB_PRIVATE(capacity);  // Allocated storage. Measured in elements.
#else
  size_t UPB_ONLYBITS(size);     // The number of elements in the array.
  size_t UPB_PRIVATE(capacity);  // Allocated storage. Measured in elements.
#endif
};

UPB_INLINE void UPB_PRIVATE(_upb_Array_ShallowFreeze)(struct upb_Array* arr) {
  arr->UPB_ONLYBITS(data) |= _UPB_ARRAY_MASK_IMM;
}

UPB_API_INLINE bool upb_Array_IsFrozen(const struct upb_Array* arr) {
  return (arr->UPB_ONLYBITS(data) & _UPB_ARRAY_MASK_IMM) != 0;
}

UPB_INLINE void UPB_PRIVATE(_upb_Array_SetTaggedPtr)(struct upb_Array* array,
                                                     void* data, size_t lg2) {
  UPB_ASSERT(lg2 != 1);
  UPB_ASSERT(lg2 <= 4);
  const size_t bits = lg2 - (lg2 != 0);
  array->UPB_ONLYBITS(data) = (uintptr_t)data | bits;
}

UPB_INLINE size_t
UPB_PRIVATE(_upb_Array_ElemSizeLg2)(const struct upb_Array* array) {
  const size_t bits = array->UPB_ONLYBITS(data) & _UPB_ARRAY_MASK_LG2;
  const size_t lg2 = bits + (bits != 0);
  return lg2;
}

UPB_API_INLINE const void* upb_Array_DataPtr(const struct upb_Array* array) {
  UPB_PRIVATE(_upb_Array_ElemSizeLg2)(array);  // Check assertions.
  return (void*)(array->UPB_ONLYBITS(data) & ~(uintptr_t)_UPB_ARRAY_MASK_ALL);
}

UPB_API_INLINE void* upb_Array_MutableDataPtr(struct upb_Array* array) {
  return (void*)upb_Array_DataPtr(array);
}

UPB_NODISCARD UPB_INLINE struct upb_Array* UPB_PRIVATE(
    _upb_Array_NewMaybeAllowSlow)(upb_Arena* arena, size_t init_capacity,
                                  int elem_size_lg2, bool allow_slow) {
  UPB_ASSERT(elem_size_lg2 != 1);
  UPB_ASSERT(elem_size_lg2 <= 4);
#if UPB_FUTURE_32BIT_ARRAY
  if (init_capacity > UINT32_MAX) {
    return NULL;
  }
#endif
  UPB_STATIC_ASSERT(UPB_ALIGN_OF(struct upb_Array) >= 8,
                    "Data must have three zero bits to store tag");
  const size_t array_size = sizeof(struct upb_Array);
  const size_t guard_size = UPB_PRIVATE(kUpb_Asan_GuardSize);
  const size_t max_bytes =
      ((SIZE_MAX - guard_size) / UPB_MALLOC_ALIGN) * UPB_MALLOC_ALIGN;
  size_t data_bytes;
  if (upb_ShlOverflow(init_capacity, elem_size_lg2, &data_bytes)) {
    return NULL;
  }
  size_t total_bytes;
  if (upb_AddOverflow(array_size, data_bytes, &total_bytes)) {
    return NULL;
  }
  if (total_bytes > max_bytes) {
    return NULL;
  }
  const size_t bytes = UPB_ALIGN_MALLOC(total_bytes);
  size_t span = UPB_PRIVATE(_upb_Arena_AllocSpan)(bytes);
  if (!allow_slow && UPB_PRIVATE(_upb_ArenaHas)(arena) < span) return NULL;
  struct upb_Array* array = (struct upb_Array*)upb_Arena_Malloc(arena, bytes);
  if (!array) return NULL;
  UPB_PRIVATE(_upb_Array_SetTaggedPtr)
  (array, UPB_PTR_AT(array, array_size, void), (size_t)elem_size_lg2);
  array->UPB_ONLYBITS(size) = 0;
#if UPB_FUTURE_32BIT_ARRAY
  size_t cap = (bytes - array_size) >> elem_size_lg2;
  if (cap > UINT32_MAX) {
    cap = UINT32_MAX;
  }
  array->UPB_PRIVATE(capacity) = cap;
#else
  array->UPB_PRIVATE(capacity) = (bytes - array_size) >> elem_size_lg2;
#endif
  return array;
}

UPB_NODISCARD UPB_INLINE struct upb_Array* UPB_PRIVATE(_upb_Array_New)(
    upb_Arena* arena, size_t init_capacity, int elem_size_lg2) {
  return UPB_PRIVATE(_upb_Array_NewMaybeAllowSlow)(arena, init_capacity,
                                                   elem_size_lg2, true);
}

UPB_NODISCARD UPB_INLINE struct upb_Array* UPB_PRIVATE(_upb_Array_TryFastNew)(
    upb_Arena* arena, size_t init_capacity, int elem_size_lg2) {
  return UPB_PRIVATE(_upb_Array_NewMaybeAllowSlow)(arena, init_capacity,
                                                   elem_size_lg2, false);
}

// Resizes the capacity of the array to be at least min_size.
UPB_NODISCARD bool UPB_PRIVATE(_upb_Array_Realloc)(struct upb_Array* array,
                                                   size_t min_size,
                                                   upb_Arena* arena);

UPB_NODISCARD UPB_FORCEINLINE bool UPB_PRIVATE(_upb_Array_TryFastRealloc)(
    struct upb_Array* array, size_t capacity, int elem_size_lg2,
    upb_Arena* arena) {
#if UPB_FUTURE_32BIT_ARRAY
  if (capacity > UINT32_MAX) {
    return false;
  }
#endif
  size_t old_bytes = (size_t)array->UPB_PRIVATE(capacity) << elem_size_lg2;
  size_t new_bytes;
  if (upb_ShlOverflow(capacity, elem_size_lg2, &new_bytes)) {
    return false;
  }
  UPB_ASSUME(new_bytes > old_bytes);

  void* data_ptr = (void*)upb_Array_DataPtr(array);
  if (!upb_Arena_TryExtend(arena, data_ptr, old_bytes, new_bytes)) return false;

  array->UPB_PRIVATE(capacity) = capacity;
  return true;
}

UPB_NODISCARD UPB_API_INLINE bool upb_Array_Reserve(struct upb_Array* array,
                                                    size_t size,
                                                    upb_Arena* arena) {
  UPB_ASSERT(!upb_Array_IsFrozen(array));
  if (array->UPB_PRIVATE(capacity) < size)
    return UPB_PRIVATE(_upb_Array_Realloc)(array, size, arena);
  return true;
}

// Resize without initializing new elements.
UPB_NODISCARD UPB_INLINE bool UPB_PRIVATE(_upb_Array_ResizeUninitialized)(
    struct upb_Array* array, size_t size, upb_Arena* arena) {
  UPB_ASSERT(!upb_Array_IsFrozen(array));
#if UPB_FUTURE_32BIT_ARRAY
  if (size > UINT32_MAX) return false;
#endif
  UPB_ASSERT(size <= array->UPB_ONLYBITS(size) ||
             arena);  // Allow NULL arena when shrinking.
  if (!upb_Array_Reserve(array, size, arena)) return false;
  array->UPB_ONLYBITS(size) = size;
  return true;
}

// Grow without initializing new elements.
UPB_NODISCARD UPB_INLINE bool UPB_PRIVATE(_upb_Array_GrowUninitialized)(
    struct upb_Array* array, size_t count, upb_Arena* arena) {
  size_t new_size;
  if (UPB_UNLIKELY(
          upb_AddOverflow(array->UPB_ONLYBITS(size), count, &new_size))) {
    return false;
  }
  return UPB_PRIVATE(_upb_Array_ResizeUninitialized)(array, new_size, arena);
}

// This function is intended for situations where elem_size is compile-time
// constant or a known expression of the form (1 << lg2), so that the expression
// i*elem_size does not result in an actual multiplication.
UPB_INLINE void UPB_PRIVATE(_upb_Array_Set)(struct upb_Array* array, size_t i,
                                            const void* data,
                                            size_t elem_size) {
  UPB_ASSERT(!upb_Array_IsFrozen(array));
  UPB_ASSERT(i < array->UPB_ONLYBITS(size));
  UPB_ASSERT(elem_size == 1U << UPB_PRIVATE(_upb_Array_ElemSizeLg2)(array));
  char* arr_data = (char*)upb_Array_MutableDataPtr(array);
  memcpy(arr_data + (i * elem_size), data, elem_size);
}

UPB_API_INLINE size_t upb_Array_Size(const struct upb_Array* arr) {
  return arr->UPB_ONLYBITS(size);
}

UPB_API_INLINE size_t upb_Array_Capacity(const struct upb_Array* arr) {
  return arr->UPB_PRIVATE(capacity);
}

// LINT.ThenChange(GoogleInternalName0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#undef _UPB_ARRAY_MASK_IMM
#undef _UPB_ARRAY_MASK_LG2
#undef _UPB_ARRAY_MASK_ALL

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_ARRAY_H_ */
