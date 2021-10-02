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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_REFLECTION_H_
#define UPB_REFLECTION_H_

#include "upb/def.h"
#include "upb/msg.h"
#include "upb/upb.h"

#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
  bool bool_val;
  float float_val;
  double double_val;
  int32_t int32_val;
  int64_t int64_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  const upb_map* map_val;
  const upb_msg* msg_val;
  const upb_array* array_val;
  upb_strview str_val;
} upb_msgval;

typedef union {
  upb_map* map;
  upb_msg* msg;
  upb_array* array;
} upb_mutmsgval;

upb_msgval upb_fielddef_default(const upb_fielddef *f);

/** upb_msg *******************************************************************/

/* Creates a new message of the given type in the given arena. */
upb_msg *upb_msg_new(const upb_msgdef *m, upb_arena *a);

/* Returns the value associated with this field. */
upb_msgval upb_msg_get(const upb_msg *msg, const upb_fielddef *f);

/* Returns a mutable pointer to a map, array, or submessage value.  If the given
 * arena is non-NULL this will construct a new object if it was not previously
 * present.  May not be called for primitive fields. */
upb_mutmsgval upb_msg_mutable(upb_msg *msg, const upb_fielddef *f, upb_arena *a);

/* May only be called for fields where upb_fielddef_haspresence(f) == true. */
bool upb_msg_has(const upb_msg *msg, const upb_fielddef *f);

/* Returns the field that is set in the oneof, or NULL if none are set. */
const upb_fielddef *upb_msg_whichoneof(const upb_msg *msg,
                                       const upb_oneofdef *o);

/* Sets the given field to the given value.  For a msg/array/map/string, the
 * caller must ensure that the target data outlives |msg| (by living either in
 * the same arena or a different arena that outlives it).
 *
 * Returns false if allocation fails. */
bool upb_msg_set(upb_msg *msg, const upb_fielddef *f, upb_msgval val,
                 upb_arena *a);


/* Clears any field presence and sets the value back to its default. */
void upb_msg_clearfield(upb_msg *msg, const upb_fielddef *f);

/* Clear all data and unknown fields. */
void upb_msg_clear(upb_msg *msg, const upb_msgdef *m);

/* Iterate over present fields.
 *
 * size_t iter = UPB_MSG_BEGIN;
 * const upb_fielddef *f;
 * upb_msgval val;
 * while (upb_msg_next(msg, m, ext_pool, &f, &val, &iter)) {
 *   process_field(f, val);
 * }
 *
 * If ext_pool is NULL, no extensions will be returned.  If the given symtab
 * returns extensions that don't match what is in this message, those extensions
 * will be skipped.
 */

#define UPB_MSG_BEGIN -1
bool upb_msg_next(const upb_msg *msg, const upb_msgdef *m,
                  const upb_symtab *ext_pool, const upb_fielddef **f,
                  upb_msgval *val, size_t *iter);

/* Clears all unknown field data from this message and all submessages. */
bool upb_msg_discardunknown(upb_msg *msg, const upb_msgdef *m, int maxdepth);

/** upb_array *****************************************************************/

/* Creates a new array on the given arena that holds elements of this type. */
upb_array *upb_array_new(upb_arena *a, upb_fieldtype_t type);

/* Returns the size of the array. */
size_t upb_array_size(const upb_array *arr);

/* Returns the given element, which must be within the array's current size. */
upb_msgval upb_array_get(const upb_array *arr, size_t i);

/* Sets the given element, which must be within the array's current size. */
void upb_array_set(upb_array *arr, size_t i, upb_msgval val);

/* Appends an element to the array.  Returns false on allocation failure. */
bool upb_array_append(upb_array *array, upb_msgval val, upb_arena *arena);

/* Changes the size of a vector.  New elements are initialized to empty/0.
 * Returns false on allocation failure. */
bool upb_array_resize(upb_array *array, size_t size, upb_arena *arena);

/** upb_map *******************************************************************/

/* Creates a new map on the given arena with the given key/value size. */
upb_map *upb_map_new(upb_arena *a, upb_fieldtype_t key_type,
                     upb_fieldtype_t value_type);

/* Returns the number of entries in the map. */
size_t upb_map_size(const upb_map *map);

/* Stores a value for the given key into |*val| (or the zero value if the key is
 * not present).  Returns whether the key was present.  The |val| pointer may be
 * NULL, in which case the function tests whether the given key is present.  */
bool upb_map_get(const upb_map *map, upb_msgval key, upb_msgval *val);

/* Removes all entries in the map. */
void upb_map_clear(upb_map *map);

/* Sets the given key to the given value.  Returns true if this was a new key in
 * the map, or false if an existing key was replaced. */
bool upb_map_set(upb_map *map, upb_msgval key, upb_msgval val,
                 upb_arena *arena);

/* Deletes this key from the table.  Returns true if the key was present. */
bool upb_map_delete(upb_map *map, upb_msgval key);

/* Map iteration:
 *
 * size_t iter = UPB_MAP_BEGIN;
 * while (upb_mapiter_next(map, &iter)) {
 *   upb_msgval key = upb_mapiter_key(map, iter);
 *   upb_msgval val = upb_mapiter_value(map, iter);
 *
 *   // If mutating is desired.
 *   upb_mapiter_setvalue(map, iter, value2);
 * }
 */

/* Advances to the next entry.  Returns false if no more entries are present. */
bool upb_mapiter_next(const upb_map *map, size_t *iter);

/* Returns true if the iterator still points to a valid entry, or false if the
 * iterator is past the last element. It is an error to call this function with
 * UPB_MAP_BEGIN (you must call next() at least once first). */
bool upb_mapiter_done(const upb_map *map, size_t iter);

/* Returns the key and value for this entry of the map. */
upb_msgval upb_mapiter_key(const upb_map *map, size_t iter);
upb_msgval upb_mapiter_value(const upb_map *map, size_t iter);

/* Sets the value for this entry.  The iterator must not be done, and the
 * iterator must not have been initialized const. */
void upb_mapiter_setvalue(upb_map *map, size_t iter, upb_msgval value);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_REFLECTION_H_ */
