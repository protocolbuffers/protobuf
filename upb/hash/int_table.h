/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_HASH_INT_TABLE_H_
#define UPB_HASH_INT_TABLE_H_

#include "upb/hash/common.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_table t;              // For entries that don't fit in the array part.
  const upb_tabval* array;  // Array part of the table. See const note above.
  size_t array_size;        // Array part size.
  size_t array_count;       // Array part number of elements.
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
void upb_inttable_compact(upb_inttable* t, upb_Arena* a);

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_HASH_INT_TABLE_H_ */
