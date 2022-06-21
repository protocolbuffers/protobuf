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

/*
 * upb_table
 *
 * This header is INTERNAL-ONLY!  Its interfaces are not public or stable!
 * This file defines very fast int->upb_value (inttable) and string->upb_value
 * (strtable) hash tables.
 *
 * The table uses chained scatter with Brent's variation (inspired by the Lua
 * implementation of hash tables).  The hash function for strings is Austin
 * Appleby's "MurmurHash."
 *
 * The inttable uses uintptr_t as its key, which guarantees it can be used to
 * store pointers or integers of at least 32 bits (upb isn't really useful on
 * systems where sizeof(void*) < 4).
 *
 * The table must be homogeneous (all values of the same type).  In debug
 * mode, we check this on insert and lookup.
 */

#ifndef UPB_INTERNAL_TABLE_H_
#define UPB_INTERNAL_TABLE_H_

#include <stdint.h>
#include <string.h>

#include "upb/upb.h"

// Must be last.
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_value ******************************************************************/

typedef struct {
  uint64_t val;
} upb_value;

/* Variant that works with a length-delimited rather than NULL-delimited string,
 * as supported by strtable. */
char* upb_strdup2(const char* s, size_t len, upb_Arena* a);

UPB_INLINE void _upb_value_setval(upb_value* v, uint64_t val) { v->val = val; }

/* For each value ctype, define the following set of functions:
 *
 * // Get/set an int32 from a upb_value.
 * int32_t upb_value_getint32(upb_value val);
 * void upb_value_setint32(upb_value *val, int32_t cval);
 *
 * // Construct a new upb_value from an int32.
 * upb_value upb_value_int32(int32_t val); */
#define FUNCS(name, membername, type_t, converter, proto_type)       \
  UPB_INLINE void upb_value_set##name(upb_value* val, type_t cval) { \
    val->val = (converter)cval;                                      \
  }                                                                  \
  UPB_INLINE upb_value upb_value_##name(type_t val) {                \
    upb_value ret;                                                   \
    upb_value_set##name(&ret, val);                                  \
    return ret;                                                      \
  }                                                                  \
  UPB_INLINE type_t upb_value_get##name(upb_value val) {             \
    return (type_t)(converter)val.val;                               \
  }

FUNCS(int32, int32, int32_t, int32_t, UPB_CTYPE_INT32)
FUNCS(int64, int64, int64_t, int64_t, UPB_CTYPE_INT64)
FUNCS(uint32, uint32, uint32_t, uint32_t, UPB_CTYPE_UINT32)
FUNCS(uint64, uint64, uint64_t, uint64_t, UPB_CTYPE_UINT64)
FUNCS(bool, _bool, bool, bool, UPB_CTYPE_BOOL)
FUNCS(cstr, cstr, char*, uintptr_t, UPB_CTYPE_CSTR)
FUNCS(ptr, ptr, void*, uintptr_t, UPB_CTYPE_PTR)
FUNCS(constptr, constptr, const void*, uintptr_t, UPB_CTYPE_CONSTPTR)

#undef FUNCS

UPB_INLINE void upb_value_setfloat(upb_value* val, float cval) {
  memcpy(&val->val, &cval, sizeof(cval));
}

UPB_INLINE void upb_value_setdouble(upb_value* val, double cval) {
  memcpy(&val->val, &cval, sizeof(cval));
}

UPB_INLINE upb_value upb_value_float(float cval) {
  upb_value ret;
  upb_value_setfloat(&ret, cval);
  return ret;
}

UPB_INLINE upb_value upb_value_double(double cval) {
  upb_value ret;
  upb_value_setdouble(&ret, cval);
  return ret;
}

#undef SET_TYPE

/* upb_tabkey *****************************************************************/

/* Either:
 *   1. an actual integer key, or
 *   2. a pointer to a string prefixed by its uint32_t length, owned by us.
 *
 * ...depending on whether this is a string table or an int table.  We would
 * make this a union of those two types, but C89 doesn't support statically
 * initializing a non-first union member. */
typedef uintptr_t upb_tabkey;

UPB_INLINE char* upb_tabstr(upb_tabkey key, uint32_t* len) {
  char* mem = (char*)key;
  if (len) memcpy(len, mem, sizeof(*len));
  return mem + sizeof(*len);
}

UPB_INLINE upb_StringView upb_tabstrview(upb_tabkey key) {
  upb_StringView ret;
  uint32_t len;
  ret.data = upb_tabstr(key, &len);
  ret.size = len;
  return ret;
}

/* upb_tabval *****************************************************************/

typedef struct upb_tabval {
  uint64_t val;
} upb_tabval;

#define UPB_TABVALUE_EMPTY_INIT \
  { -1 }

/* upb_table ******************************************************************/

typedef struct _upb_tabent {
  upb_tabkey key;
  upb_tabval val;

  /* Internal chaining.  This is const so we can create static initializers for
   * tables.  We cast away const sometimes, but *only* when the containing
   * upb_table is known to be non-const.  This requires a bit of care, but
   * the subtlety is confined to table.c. */
  const struct _upb_tabent* next;
} upb_tabent;

typedef struct {
  size_t count;       /* Number of entries in the hash part. */
  uint32_t mask;      /* Mask to turn hash value -> bucket. */
  uint32_t max_count; /* Max count before we hit our load limit. */
  uint8_t size_lg2;   /* Size of the hashtable part is 2^size_lg2 entries. */
  upb_tabent* entries;
} upb_table;

typedef struct {
  upb_table t;
} upb_strtable;

typedef struct {
  upb_table t;             /* For entries that don't fit in the array part. */
  const upb_tabval* array; /* Array part of the table. See const note above. */
  size_t array_size;       /* Array part size. */
  size_t array_count;      /* Array part number of elements. */
} upb_inttable;

UPB_INLINE size_t upb_table_size(const upb_table* t) {
  if (t->size_lg2 == 0)
    return 0;
  else
    return 1 << t->size_lg2;
}

/* Internal-only functions, in .h file only out of necessity. */
UPB_INLINE bool upb_tabent_isempty(const upb_tabent* e) { return e->key == 0; }

/* Initialize and uninitialize a table, respectively.  If memory allocation
 * failed, false is returned that the table is uninitialized. */
bool upb_inttable_init(upb_inttable* table, upb_Arena* a);
bool upb_strtable_init(upb_strtable* table, size_t expected_size, upb_Arena* a);

/* Returns the number of values in the table. */
size_t upb_inttable_count(const upb_inttable* t);
UPB_INLINE size_t upb_strtable_count(const upb_strtable* t) {
  return t->t.count;
}

void upb_strtable_clear(upb_strtable* t);

/* Inserts the given key into the hashtable with the given value.  The key must
 * not already exist in the hash table.  For strtables, the key is not required
 * to be NULL-terminated, and the table will make an internal copy of the key.
 * Inttables must not insert a value of UINTPTR_MAX.
 *
 * If a table resize was required but memory allocation failed, false is
 * returned and the table is unchanged. */
bool upb_inttable_insert(upb_inttable* t, uintptr_t key, upb_value val,
                         upb_Arena* a);
bool upb_strtable_insert(upb_strtable* t, const char* key, size_t len,
                         upb_value val, upb_Arena* a);

/* Looks up key in this table, returning "true" if the key was found.
 * If v is non-NULL, copies the value for this key into *v. */
bool upb_inttable_lookup(const upb_inttable* t, uintptr_t key, upb_value* v);
bool upb_strtable_lookup2(const upb_strtable* t, const char* key, size_t len,
                          upb_value* v);

/* For NULL-terminated strings. */
UPB_INLINE bool upb_strtable_lookup(const upb_strtable* t, const char* key,
                                    upb_value* v) {
  return upb_strtable_lookup2(t, key, strlen(key), v);
}

/* Removes an item from the table.  Returns true if the remove was successful,
 * and stores the removed item in *val if non-NULL. */
bool upb_inttable_remove(upb_inttable* t, uintptr_t key, upb_value* val);
bool upb_strtable_remove2(upb_strtable* t, const char* key, size_t len,
                          upb_value* val);

UPB_INLINE bool upb_strtable_remove(upb_strtable* t, const char* key,
                                    upb_value* v) {
  return upb_strtable_remove2(t, key, strlen(key), v);
}

/* Updates an existing entry in an inttable.  If the entry does not exist,
 * returns false and does nothing.  Unlike insert/remove, this does not
 * invalidate iterators. */
bool upb_inttable_replace(upb_inttable* t, uintptr_t key, upb_value val);

/* Optimizes the table for the current set of entries, for both memory use and
 * lookup time.  Client should call this after all entries have been inserted;
 * inserting more entries is legal, but will likely require a table resize. */
void upb_inttable_compact(upb_inttable* t, upb_Arena* a);

/* Exposed for testing only. */
bool upb_strtable_resize(upb_strtable* t, size_t size_lg2, upb_Arena* a);

/* Iterators ******************************************************************/

/* Iteration over inttable.
 *
 *   intptr_t iter = UPB_INTTABLE_BEGIN;
 *   uintptr_t key;
 *   upb_value val;
 *   while (upb_inttable_next2(t, &key, &val, &iter)) {
 *      // ...
 *   }
 */

#define UPB_INTTABLE_BEGIN -1

bool upb_inttable_next2(const upb_inttable* t, uintptr_t* key, upb_value* val,
                        intptr_t* iter);
void upb_inttable_removeiter(upb_inttable* t, intptr_t* iter);

/* Iteration over strtable.
 *
 *   intptr_t iter = UPB_INTTABLE_BEGIN;
 *   upb_StringView key;
 *   upb_value val;
 *   while (upb_strtable_next2(t, &key, &val, &iter)) {
 *      // ...
 *   }
 */

#define UPB_STRTABLE_BEGIN -1

bool upb_strtable_next2(const upb_strtable* t, upb_StringView* key,
                        upb_value* val, intptr_t* iter);
void upb_strtable_removeiter(upb_strtable* t, intptr_t* iter);

/* DEPRECATED iterators, slated for removal.
 *
 * Iterators for int and string tables.  We are subject to some kind of unusual
 * design constraints:
 *
 * For high-level languages:
 *  - we must be able to guarantee that we don't crash or corrupt memory even if
 *    the program accesses an invalidated iterator.
 *
 * For C++11 range-based for:
 *  - iterators must be copyable
 *  - iterators must be comparable
 *  - it must be possible to construct an "end" value.
 *
 * Iteration order is undefined.
 *
 * Modifying the table invalidates iterators.  upb_{str,int}table_done() is
 * guaranteed to work even on an invalidated iterator, as long as the table it
 * is iterating over has not been freed.  Calling next() or accessing data from
 * an invalidated iterator yields unspecified elements from the table, but it is
 * guaranteed not to crash and to return real table elements (except when done()
 * is true). */

/* upb_strtable_iter **********************************************************/

/*   upb_strtable_iter i;
 *   upb_strtable_begin(&i, t);
 *   for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
 *     const char *key = upb_strtable_iter_key(&i);
 *     const upb_value val = upb_strtable_iter_value(&i);
 *     // ...
 *   }
 */

typedef struct {
  const upb_strtable* t;
  size_t index;
} upb_strtable_iter;

void upb_strtable_begin(upb_strtable_iter* i, const upb_strtable* t);
void upb_strtable_next(upb_strtable_iter* i);
bool upb_strtable_done(const upb_strtable_iter* i);
upb_StringView upb_strtable_iter_key(const upb_strtable_iter* i);
upb_value upb_strtable_iter_value(const upb_strtable_iter* i);
void upb_strtable_iter_setdone(upb_strtable_iter* i);
bool upb_strtable_iter_isequal(const upb_strtable_iter* i1,
                               const upb_strtable_iter* i2);

/* upb_inttable_iter **********************************************************/

/*   upb_inttable_iter i;
 *   upb_inttable_begin(&i, t);
 *   for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
 *     uintptr_t key = upb_inttable_iter_key(&i);
 *     upb_value val = upb_inttable_iter_value(&i);
 *     // ...
 *   }
 */

typedef struct {
  const upb_inttable* t;
  size_t index;
  bool array_part;
} upb_inttable_iter;

UPB_INLINE const upb_tabent* str_tabent(const upb_strtable_iter* i) {
  return &i->t->t.entries[i->index];
}

void upb_inttable_begin(upb_inttable_iter* i, const upb_inttable* t);
void upb_inttable_next(upb_inttable_iter* i);
bool upb_inttable_done(const upb_inttable_iter* i);
uintptr_t upb_inttable_iter_key(const upb_inttable_iter* i);
upb_value upb_inttable_iter_value(const upb_inttable_iter* i);
void upb_inttable_iter_setdone(upb_inttable_iter* i);
bool upb_inttable_iter_isequal(const upb_inttable_iter* i1,
                               const upb_inttable_iter* i2);

uint32_t _upb_Hash(const void* p, size_t n, uint64_t seed);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_INTERNAL_TABLE_H_ */
