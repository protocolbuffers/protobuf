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
  upb_table t;
} upb_inttable;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize a table. If memory allocation failed, false is returned and
// the table is uninitialized.
UPB_NODISCARD bool upb_inttable_init(upb_inttable* table, upb_Arena* a);

// Returns the number of values in the table.
UPB_INLINE size_t upb_inttable_count(const upb_inttable* t) {
  return t->t.count;
}

// Inserts the given key into the hashtable with the given value.
// The key must not already exist in the hash table.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged.
UPB_NODISCARD bool upb_inttable_insert(upb_inttable* t, uintptr_t key,
                                       upb_value val, upb_Arena* a);

UPB_NODISCARD bool upb_inttable_insert_or_replace_slow(upb_inttable* t,
                                                       uintptr_t key,
                                                       upb_value val,
                                                       bool* replaced,
                                                       upb_Arena* a);

UPB_INLINE uint32_t upb_inthash(uintptr_t key) {
  UPB_STATIC_ASSERT(sizeof(uintptr_t) == 4 || sizeof(uintptr_t) == 8,
                    "Pointers don't fit");
  if (sizeof(uintptr_t) == 8) {
    return (uint32_t)key ^ (uint32_t)((uint64_t)key >> 32);
  } else {
    return (uint32_t)key;
  }
}

// Inserts or updates the given key with the given value. Sets *replaced to
// true if an existing entry was updated, or false if a new entry was inserted.
// Returns false if a table resize was required and memory allocation failed.
UPB_NODISCARD UPB_FORCEINLINE bool upb_inttable_insert_or_replace(
    upb_inttable* t, uintptr_t key, upb_value val, bool* replaced,
    upb_Arena* a) {
  if (UPB_LIKELY(t->t.entries != NULL)) {
    upb_tabent* e = upb_getentry(&t->t, upb_inthash(key));
    if (UPB_LIKELY(upb_tabent_isempty(e) && !upb_table_isfull(&t->t))) {
      t->t.count++;
      e->key.num = key;
      e->val = val;
      upb_tabent_clearnext(e);
      *replaced = false;
      return true;
    }
  }
  // Use a separate temporary so that `replaced` does not escape on the fast
  // path above. It must be initialized because the slow path leaves it unset
  // on allocation failure.
  bool slow_replaced = false;
  bool ok = upb_inttable_insert_or_replace_slow(t, key, val, &slow_replaced, a);
  *replaced = slow_replaced;
  return ok;
}

// Resizes the table to 1 << size_lg2.
UPB_NODISCARD bool upb_inttable_resize(upb_inttable* t, size_t size_lg2,
                                       upb_Arena* a);

// Copies the table without rehashing. Performing a shallow copy of entries;
// the caller is responsible for cloning non-primitive values.
bool upb_inttable_copy(upb_inttable* dest, const upb_inttable* src,
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_HASH_INT_TABLE_H_ */
