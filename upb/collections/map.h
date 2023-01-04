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

#ifndef UPB_COLLECTIONS_MAP_H_
#define UPB_COLLECTIONS_MAP_H_

#include "upb/base/descriptor_constants.h"
#include "upb/collections/message_value.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new map on the given arena with the given key/value size.
UPB_API upb_Map* upb_Map_New(upb_Arena* a, upb_CType key_type,
                             upb_CType value_type);

// Returns the number of entries in the map.
UPB_API size_t upb_Map_Size(const upb_Map* map);

// Stores a value for the given key into |*val| (or the zero value if the key is
// not present). Returns whether the key was present. The |val| pointer may be
// NULL, in which case the function tests whether the given key is present.
UPB_API bool upb_Map_Get(const upb_Map* map, upb_MessageValue key,
                         upb_MessageValue* val);

// Removes all entries in the map.
UPB_API void upb_Map_Clear(upb_Map* map);

typedef enum {
  kUpb_MapInsertStatus_Inserted = 0,
  kUpb_MapInsertStatus_Replaced = 1,
  kUpb_MapInsertStatus_OutOfMemory = 2,
} upb_MapInsertStatus;

// Sets the given key to the given value, returning whether the key was inserted
// or replaced. If the key was inserted, then any existing iterators will be
// invalidated.
UPB_API upb_MapInsertStatus upb_Map_Insert(upb_Map* map, upb_MessageValue key,
                                           upb_MessageValue val,
                                           upb_Arena* arena);

// Sets the given key to the given value. Returns false if memory allocation
// failed. If the key is newly inserted, then any existing iterators will be
// invalidated.
UPB_API_INLINE bool upb_Map_Set(upb_Map* map, upb_MessageValue key,
                                upb_MessageValue val, upb_Arena* arena) {
  return upb_Map_Insert(map, key, val, arena) !=
         kUpb_MapInsertStatus_OutOfMemory;
}

// Deletes this key from the table. Returns true if the key was present.
// If present and |val| is non-NULL, stores the deleted value.
UPB_API bool upb_Map_Delete(upb_Map* map, upb_MessageValue key,
                            upb_MessageValue* val);

// (DEPRECATED and going away soon. Do not use.)
UPB_INLINE bool upb_Map_Delete2(upb_Map* map, upb_MessageValue key,
                                upb_MessageValue* val) {
  return upb_Map_Delete(map, key, val);
}

// Map iteration:
//
// size_t iter = kUpb_Map_Begin;
// upb_MessageValue key, val;
// while (upb_Map_Next(map, &key, &val, &iter)) {
//   ...
// }

#define kUpb_Map_Begin ((size_t)-1)

// Advances to the next entry. Returns false if no more entries are present.
// Otherwise returns true and populates both *key and *value.
UPB_API bool upb_Map_Next(const upb_Map* map, upb_MessageValue* key,
                          upb_MessageValue* val, size_t* iter);

// DEPRECATED iterator, slated for removal.

/* Map iteration:
 *
 * size_t iter = kUpb_Map_Begin;
 * while (upb_MapIterator_Next(map, &iter)) {
 *   upb_MessageValue key = upb_MapIterator_Key(map, iter);
 *   upb_MessageValue val = upb_MapIterator_Value(map, iter);
 * }
 */

// Advances to the next entry. Returns false if no more entries are present.
bool upb_MapIterator_Next(const upb_Map* map, size_t* iter);

// Returns true if the iterator still points to a valid entry, or false if the
// iterator is past the last element. It is an error to call this function with
// kUpb_Map_Begin (you must call next() at least once first).
bool upb_MapIterator_Done(const upb_Map* map, size_t iter);

// Returns the key and value for this entry of the map.
upb_MessageValue upb_MapIterator_Key(const upb_Map* map, size_t iter);
upb_MessageValue upb_MapIterator_Value(const upb_Map* map, size_t iter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_COLLECTIONS_MAP_H_ */
