// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_HASH_EXT_TABLE_H_
#define UPB_HASH_EXT_TABLE_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/hash/common.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_table t;
} upb_exttable;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize a table. If memory allocation failed, false is returned and
// the table is uninitialized.
UPB_NODISCARD bool upb_exttable_init(upb_exttable* table, size_t expected_size,
                                     upb_Arena* a);

// Returns the number of values in the table.
UPB_INLINE size_t upb_exttable_count(const upb_exttable* t) {
  return t->t.count;
}

void upb_exttable_clear(upb_exttable* t);

// Inserts the given key and value into the hashtable.
// The key must not already exist in the hash table, and must not be NULL.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged.
UPB_NODISCARD bool upb_exttable_insert(upb_exttable* t, const void* k,
                                       const uint32_t* v, upb_Arena* a);

UPB_INLINE uint32_t _upb_exttable_hash(const void* ptr, uint32_t ext_num) {
  // The low bits of pointers are very low-entropy; an extension's alignment or
  // malloc alignment makes the low 3 bits always 0; the fact that they tend to
  // be allocated in arrays or sequentially in an arena means they exhibit
  // periodicity in their size. Rotate the pointer to bring the higher entropy
  // bits lower.
  uintptr_t rotptr = (uintptr_t)ptr >> 5 | (uintptr_t)ptr
                                               << (sizeof(uintptr_t) * 8 - 5);
  uintptr_t hash = rotptr ^ (uintptr_t)ext_num;
  return upb_inthash(hash);
}

UPB_INLINE bool exteql(upb_key k1, upb_value v1, upb_lookupkey k2) {
  if ((const void*)k1.num == k2.ext.ptr) {
    uint32_t ext_num1 = *(const uint32_t*)upb_value_getconstptr(v1);
    return ext_num1 == k2.ext.ext_num;
  }
  return false;
}

// Looks up key and ext_number in this table, returning the value if the key was
// found, or NULL otherwise.
UPB_INLINE const uint32_t* upb_exttable_lookup(const upb_exttable* t,
                                               const void* k,
                                               uint32_t ext_number) {
  uint32_t hash = _upb_exttable_hash(k, ext_number);
  upb_value val;
  upb_lookupkey key;
  key.ext.ptr = k;
  key.ext.ext_num = ext_number;
  if (upb_lookup(&t->t, key, &val, hash, &exteql)) {
    return (const uint32_t*)upb_value_getconstptr(val);
  }
  return NULL;
}

// Removes an item from the table. Returns the removed item if the remove was
// successful, or NULL if the key was not found.
const uint32_t* upb_exttable_remove(upb_exttable* t, const void* k,
                                    uint32_t ext_number);

size_t upb_exttable_size(const upb_exttable* t);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_HASH_EXT_TABLE_H_ */
