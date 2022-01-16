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

#ifndef UPB_REFLECTION_H_
#define UPB_REFLECTION_H_

#include "upb/def.h"
#include "upb/msg.h"
#include "upb/port_def.inc"
#include "upb/upb.h"

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
  const upb_Map* map_val;
  const upb_Message* msg_val;
  const upb_Array* array_val;
  upb_StringView str_val;
} upb_MessageValue;

typedef union {
  upb_Map* map;
  upb_Message* msg;
  upb_Array* array;
} upb_MutableMessageValue;

upb_MessageValue upb_FieldDef_Default(const upb_FieldDef* f);

/** upb_Message
 * *******************************************************************/

/* Creates a new message of the given type in the given arena. */
upb_Message* upb_Message_New(const upb_MessageDef* m, upb_Arena* a);

/* Returns the value associated with this field. */
upb_MessageValue upb_Message_Get(const upb_Message* msg, const upb_FieldDef* f);

/* Returns a mutable pointer to a map, array, or submessage value.  If the given
 * arena is non-NULL this will construct a new object if it was not previously
 * present.  May not be called for primitive fields. */
upb_MutableMessageValue upb_Message_Mutable(upb_Message* msg,
                                            const upb_FieldDef* f,
                                            upb_Arena* a);

/* May only be called for fields where upb_FieldDef_HasPresence(f) == true. */
bool upb_Message_Has(const upb_Message* msg, const upb_FieldDef* f);

/* Returns the field that is set in the oneof, or NULL if none are set. */
const upb_FieldDef* upb_Message_WhichOneof(const upb_Message* msg,
                                           const upb_OneofDef* o);

/* Sets the given field to the given value.  For a msg/array/map/string, the
 * caller must ensure that the target data outlives |msg| (by living either in
 * the same arena or a different arena that outlives it).
 *
 * Returns false if allocation fails. */
bool upb_Message_Set(upb_Message* msg, const upb_FieldDef* f,
                     upb_MessageValue val, upb_Arena* a);

/* Clears any field presence and sets the value back to its default. */
void upb_Message_ClearField(upb_Message* msg, const upb_FieldDef* f);

/* Clear all data and unknown fields. */
void upb_Message_Clear(upb_Message* msg, const upb_MessageDef* m);

/* Iterate over present fields.
 *
 * size_t iter = kUpb_Message_Begin;
 * const upb_FieldDef *f;
 * upb_MessageValue val;
 * while (upb_Message_Next(msg, m, ext_pool, &f, &val, &iter)) {
 *   process_field(f, val);
 * }
 *
 * If ext_pool is NULL, no extensions will be returned.  If the given symtab
 * returns extensions that don't match what is in this message, those extensions
 * will be skipped.
 */

#define kUpb_Message_Begin -1
bool upb_Message_Next(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, const upb_FieldDef** f,
                      upb_MessageValue* val, size_t* iter);

/* Clears all unknown field data from this message and all submessages. */
bool upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                int maxdepth);

/** upb_Array *****************************************************************/

/* Creates a new array on the given arena that holds elements of this type. */
upb_Array* upb_Array_New(upb_Arena* a, upb_CType type);

/* Returns the size of the array. */
size_t upb_Array_Size(const upb_Array* arr);

/* Returns the given element, which must be within the array's current size. */
upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i);

/* Sets the given element, which must be within the array's current size. */
void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val);

/* Appends an element to the array.  Returns false on allocation failure. */
bool upb_Array_Append(upb_Array* array, upb_MessageValue val, upb_Arena* arena);

/* Moves elements within the array using memmove(). Like memmove(), the source
 * and destination elements may be overlapping. */
void upb_Array_Move(upb_Array* array, size_t dst_idx, size_t src_idx,
                    size_t count);

/* Inserts one or more empty elements into the array.  Existing elements are
 * shifted right.  The new elements have undefined state and must be set with
 * `upb_Array_Set()`.
 * REQUIRES: `i <= upb_Array_Size(arr)` */
bool upb_Array_Insert(upb_Array* array, size_t i, size_t count,
                      upb_Arena* arena);

/* Deletes one or more elements from the array.  Existing elements are shifted
 * left.
 * REQUIRES: `i + count <= upb_Array_Size(arr)` */
void upb_Array_Delete(upb_Array* array, size_t i, size_t count);

/* Changes the size of a vector.  New elements are initialized to empty/0.
 * Returns false on allocation failure. */
bool upb_Array_Resize(upb_Array* array, size_t size, upb_Arena* arena);

/** upb_Map *******************************************************************/

/* Creates a new map on the given arena with the given key/value size. */
upb_Map* upb_Map_New(upb_Arena* a, upb_CType key_type, upb_CType value_type);

/* Returns the number of entries in the map. */
size_t upb_Map_Size(const upb_Map* map);

/* Stores a value for the given key into |*val| (or the zero value if the key is
 * not present).  Returns whether the key was present.  The |val| pointer may be
 * NULL, in which case the function tests whether the given key is present.  */
bool upb_Map_Get(const upb_Map* map, upb_MessageValue key,
                 upb_MessageValue* val);

/* Removes all entries in the map. */
void upb_Map_Clear(upb_Map* map);

/* Sets the given key to the given value.  Returns true if this was a new key in
 * the map, or false if an existing key was replaced. */
bool upb_Map_Set(upb_Map* map, upb_MessageValue key, upb_MessageValue val,
                 upb_Arena* arena);

/* Deletes this key from the table.  Returns true if the key was present. */
bool upb_Map_Delete(upb_Map* map, upb_MessageValue key);

/* Map iteration:
 *
 * size_t iter = kUpb_Map_Begin;
 * while (upb_MapIterator_Next(map, &iter)) {
 *   upb_MessageValue key = upb_MapIterator_Key(map, iter);
 *   upb_MessageValue val = upb_MapIterator_Value(map, iter);
 *
 *   // If mutating is desired.
 *   upb_MapIterator_SetValue(map, iter, value2);
 * }
 */

/* Advances to the next entry.  Returns false if no more entries are present. */
bool upb_MapIterator_Next(const upb_Map* map, size_t* iter);

/* Returns true if the iterator still points to a valid entry, or false if the
 * iterator is past the last element. It is an error to call this function with
 * kUpb_Map_Begin (you must call next() at least once first). */
bool upb_MapIterator_Done(const upb_Map* map, size_t iter);

/* Returns the key and value for this entry of the map. */
upb_MessageValue upb_MapIterator_Key(const upb_Map* map, size_t iter);
upb_MessageValue upb_MapIterator_Value(const upb_Map* map, size_t iter);

/* Sets the value for this entry.  The iterator must not be done, and the
 * iterator must not have been initialized const. */
void upb_MapIterator_SetValue(upb_Map* map, size_t iter,
                              upb_MessageValue value);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_REFLECTION_H_ */
