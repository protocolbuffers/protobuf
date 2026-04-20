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

// Looks up key and ext_number in this table, returning the value if the key was
// found, or NULL otherwise.
const uint32_t* upb_exttable_lookup(const upb_exttable* t, const void* k,
                                    uint32_t ext_number);

// Removes an item from the table. Returns the removed item if the remove was
// successful, or NULL if the key was not found.
const uint32_t* upb_exttable_remove(upb_exttable* t, const void* k,
                                    uint32_t ext_number);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_HASH_EXT_TABLE_H_ */
