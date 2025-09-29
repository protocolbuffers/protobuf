// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_HASH_INT_TABLE_H_
#define UPB_HASH_INT_TABLE_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/hash/common.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_table t;  // For entries that don't fit in the array part.
  // Array part of the table.
  // Pointers on this table are const so we can create static initializers for
  // tables.  We cast away const sometimes, but *only* when the containing
  // upb_table is known to be non-const.  This requires a bit of care, but
  // the subtlety is confined to table.c.
  const upb_value* array;
  // Track presence in the array part. Each bit at index (key % 8) at the
  // presence_mask[key/8] indicates if the element is present in the array part.
  const uint8_t* presence_mask;
  uint32_t array_size;   // Array part size.
  uint32_t array_count;  // Array part number of elements.
} upb_inttable;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize a table. If memory allocation failed, false is returned and
// the table is uninitialized.
bool upb_inttable_init(upb_inttable* table, upb_Arena* a);

// Returns the number of values in the table.
size_t upb_inttable_count(const upb_inttable* t);

// Inserts the given key into the hashtable with the given value.
// The key must not already exist in the hash table.
// The value must not be UINTPTR_MAX.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged.
bool upb_inttable_insert(upb_inttable* t, uintptr_t key, upb_value val,
                         upb_Arena* a);

// Looks up key in this table, returning "true" if the key was found.
// If v is non-NULL, copies the value for this key into *v.
bool upb_inttable_lookup(const upb_inttable* t, uintptr_t key, upb_value* v);

// Removes an item from the table. Returns true if the remove was successful,
// and stores the removed item in *val if non-NULL.
bool upb_inttable_remove(upb_inttable* t, uintptr_t key, upb_value* val);

// Updates an existing entry in an inttable.
// If the entry does not exist, returns false and does nothing.
// Unlike insert/remove, this does not invalidate iterators.
bool upb_inttable_replace(upb_inttable* t, uintptr_t key, upb_value val);

// Optimizes the table for the current set of entries, for both memory use and
// lookup time. Client should call this after all entries have been inserted;
// inserting more entries is legal, but will likely require a table resize.
// Returns false if reallocation fails.
bool upb_inttable_compact(upb_inttable* t, upb_Arena* a);

// Clears the table.
void upb_inttable_clear(upb_inttable* t);

// Iteration over inttable:
//
//   intptr_t iter = UPB_INTTABLE_BEGIN;
//   uintptr_t key;
//   upb_value val;
//   while (upb_inttable_next(t, &key, &val, &iter)) {
//      // ...
//   }

#define UPB_INTTABLE_BEGIN -1

bool upb_inttable_next(const upb_inttable* t, uintptr_t* key, upb_value* val,
                       intptr_t* iter);
void upb_inttable_removeiter(upb_inttable* t, intptr_t* iter);
void upb_inttable_setentryvalue(upb_inttable* t, intptr_t iter, upb_value v);
bool upb_inttable_done(const upb_inttable* t, intptr_t i);
uintptr_t upb_inttable_iter_key(const upb_inttable* t, intptr_t iter);
upb_value upb_inttable_iter_value(const upb_inttable* t, intptr_t iter);

UPB_INLINE bool upb_inttable_arrhas(const upb_inttable* t, uintptr_t key) {
  return (t->presence_mask[key / 8] & (1 << (key % 8))) != 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_HASH_INT_TABLE_H_ */
