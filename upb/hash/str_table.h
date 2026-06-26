// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_HASH_STR_TABLE_H_
#define UPB_HASH_STR_TABLE_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_table t;
} upb_strtable;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize a table. If memory allocation failed, false is returned and
// the table is uninitialized.
UPB_NODISCARD bool upb_strtable_init(upb_strtable* table, size_t expected_size,
                                     upb_Arena* a);

// Returns the number of values in the table.
UPB_INLINE size_t upb_strtable_count(const upb_strtable* t) {
  return t->t.count;
}

void upb_strtable_clear(upb_strtable* t);

// Inserts the given key into the hashtable with the given value.
// The key must not already exist in the hash table. The key is not required
// to be NULL-terminated, and the table will make an internal copy of the key.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged. */
UPB_NODISCARD bool upb_strtable_insert(upb_strtable* t, const char* key,
                                       size_t len, upb_value val, upb_Arena* a);

// Looks up key in this table, returning "true" if the key was found.
// If v is non-NULL, copies the value for this key into *v.
bool upb_strtable_lookup2(const upb_strtable* t, const char* key, size_t len,
                          upb_value* v);

// For NULL-terminated strings.
UPB_INLINE bool upb_strtable_lookup(const upb_strtable* t, const char* key,
                                    upb_value* v) {
  return upb_strtable_lookup2(t, key, strlen(key), v);
}

// Removes an item from the table. Returns true if the remove was successful,
// and stores the removed item in *val if non-NULL.
bool upb_strtable_remove2(upb_strtable* t, const char* key, size_t len,
                          upb_value* val);

UPB_INLINE bool upb_strtable_remove(upb_strtable* t, const char* key,
                                    upb_value* v) {
  return upb_strtable_remove2(t, key, strlen(key), v);
}

// Exposed for testing only.
UPB_NODISCARD bool upb_strtable_resize(upb_strtable* t, size_t size_lg2,
                                       upb_Arena* a);

/* Iteration over strtable:
 *
 *   intptr_t iter = UPB_STRTABLE_BEGIN;
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
void upb_strtable_setentryvalue(upb_strtable* t, intptr_t iter, upb_value v);

/* DEPRECATED iterators, slated for removal.
 *
 * Iterators for string tables.  We are subject to some kind of unusual
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

UPB_INLINE const upb_tabent* str_tabent(const upb_strtable_iter* i) {
  return &i->t->t.entries[i->index];
}

void upb_strtable_begin(upb_strtable_iter* i, const upb_strtable* t);
void upb_strtable_next(upb_strtable_iter* i);
bool upb_strtable_done(const upb_strtable_iter* i);
upb_StringView upb_strtable_iter_key(const upb_strtable_iter* i);
upb_value upb_strtable_iter_value(const upb_strtable_iter* i);
void upb_strtable_iter_setdone(upb_strtable_iter* i);
bool upb_strtable_iter_isequal(const upb_strtable_iter* i1,
                               const upb_strtable_iter* i2);

typedef struct {
  upb_strtable t;
} upb_strtable_bool;
typedef struct {
  upb_strtable t;
} upb_strtable_32;
typedef struct {
  upb_strtable t;
} upb_strtable_64;

#define UPB_STRTABLE_DECLARE(postfix, val_type, to_val, from_val)              \
  UPB_INLINE bool upb_strtable_##postfix##_init(                               \
      upb_strtable_##postfix* t, size_t expected_size, upb_Arena* a) {         \
    return upb_strtable_init(&t->t, expected_size, a);                         \
  }                                                                            \
  UPB_INLINE size_t upb_strtable_##postfix##_count(                            \
      const upb_strtable_##postfix* t) {                                       \
    return upb_strtable_count(&t->t);                                          \
  }                                                                            \
  UPB_INLINE bool upb_strtable_##postfix##_insert(                             \
      upb_strtable_##postfix* t, const char* key, size_t len, val_type val,    \
      upb_Arena* a) {                                                          \
    return upb_strtable_insert(&t->t, key, len, to_val(val), a);               \
  }                                                                            \
  UPB_INLINE bool upb_strtable_##postfix##_lookup(                             \
      const upb_strtable_##postfix* t, const char* key, size_t len,            \
      val_type* v) {                                                           \
    upb_value tabval;                                                          \
    if (upb_strtable_lookup2(&t->t, key, len, &tabval)) {                      \
      if (v) *v = from_val(tabval);                                            \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
  UPB_INLINE bool upb_strtable_##postfix##_remove(                             \
      upb_strtable_##postfix* t, const char* key, size_t len, val_type* val) { \
    upb_value tabval;                                                          \
    if (upb_strtable_remove2(&t->t, key, len, &tabval)) {                      \
      if (val) *val = from_val(tabval);                                        \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
  UPB_INLINE void upb_strtable_##postfix##_clear(upb_strtable_##postfix* t) {  \
    upb_strtable_clear(&t->t);                                                 \
  }                                                                            \
  typedef struct {                                                             \
    upb_strtable_iter iter;                                                    \
  } upb_strtable_##postfix##_iter;                                             \
  UPB_INLINE void upb_strtable_##postfix##_begin(                              \
      upb_strtable_##postfix##_iter* i, const upb_strtable_##postfix* t) {     \
    upb_strtable_begin(&i->iter, &t->t);                                       \
  }                                                                            \
  UPB_INLINE void upb_strtable_##postfix##_next(                               \
      upb_strtable_##postfix##_iter* i) {                                      \
    upb_strtable_next(&i->iter);                                               \
  }                                                                            \
  UPB_INLINE bool upb_strtable_##postfix##_done(                               \
      const upb_strtable_##postfix##_iter* i) {                                \
    return upb_strtable_done(&i->iter);                                        \
  }                                                                            \
  UPB_INLINE upb_StringView upb_strtable_##postfix##_iter_key(                 \
      const upb_strtable_##postfix##_iter* i) {                                \
    return upb_strtable_iter_key(&i->iter);                                    \
  }                                                                            \
  UPB_INLINE val_type upb_strtable_##postfix##_iter_value(                     \
      const upb_strtable_##postfix##_iter* i) {                                \
    return from_val(upb_strtable_iter_value(&i->iter));                        \
  }                                                                            \
  UPB_INLINE void upb_strtable_##postfix##_iter_setdone(                       \
      upb_strtable_##postfix##_iter* i) {                                      \
    upb_strtable_iter_setdone(&i->iter);                                       \
  }                                                                            \
  UPB_INLINE bool upb_strtable_##postfix##_iter_isequal(                       \
      const upb_strtable_##postfix##_iter* i1,                                 \
      const upb_strtable_##postfix##_iter* i2) {                               \
    return upb_strtable_iter_isequal(&i1->iter, &i2->iter);                    \
  }                                                                            \
  UPB_INLINE bool upb_strtable_##postfix##_next2(                              \
      const upb_strtable_##postfix* t, upb_StringView* key, val_type* val,     \
      intptr_t* iter) {                                                        \
    upb_value v;                                                               \
    if (upb_strtable_next2(&t->t, key, &v, iter)) {                            \
      if (val) *val = from_val(v);                                             \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
  UPB_INLINE void upb_strtable_##postfix##_removeiter(                         \
      upb_strtable_##postfix* t, intptr_t* iter) {                             \
    upb_strtable_removeiter(&t->t, iter);                                      \
  }

UPB_STRTABLE_DECLARE(bool, bool, upb_value_bool, upb_value_getbool)
UPB_STRTABLE_DECLARE(32, uint32_t, upb_value_uint32, upb_value_getuint32)
UPB_STRTABLE_DECLARE(64, uint64_t, upb_value_uint64, upb_value_getuint64)
#if UINTPTR_MAX == 0xffffffff

typedef upb_strtable_32 upb_strtable_ptr;
typedef upb_strtable_32_iter upb_strtable_ptr_iter;

UPB_INLINE bool upb_strtable_ptr_init(upb_strtable_ptr* t, size_t expected_size,
                                      upb_Arena* a) {
  return upb_strtable_32_init(t, expected_size, a);
}
UPB_INLINE size_t upb_strtable_ptr_count(const upb_strtable_ptr* t) {
  return upb_strtable_32_count(t);
}
UPB_INLINE bool upb_strtable_ptr_insert(upb_strtable_ptr* t, const char* key,
                                        size_t len, const void* val,
                                        upb_Arena* a) {
  return upb_strtable_32_insert(t, key, len, (uint32_t)(uintptr_t)val, a);
}
UPB_INLINE bool upb_strtable_ptr_lookup(const upb_strtable_ptr* t,
                                        const char* key, size_t len,
                                        const void** v) {
  uint32_t val;
  if (upb_strtable_32_lookup(t, key, len, &val)) {
    if (v) *v = (const void*)(uintptr_t)val;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_strtable_ptr_remove(upb_strtable_ptr* t, const char* key,
                                        size_t len, const void** val) {
  uint32_t val_t;
  if (upb_strtable_32_remove(t, key, len, &val_t)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE void upb_strtable_ptr_clear(upb_strtable_ptr* t) {
  upb_strtable_32_clear(t);
}
UPB_INLINE void upb_strtable_ptr_begin(upb_strtable_ptr_iter* i,
                                       const upb_strtable_ptr* t) {
  upb_strtable_32_begin(i, t);
}
UPB_INLINE void upb_strtable_ptr_next(upb_strtable_ptr_iter* i) {
  upb_strtable_32_next(i);
}
UPB_INLINE bool upb_strtable_ptr_done(const upb_strtable_ptr_iter* i) {
  return upb_strtable_32_done(i);
}
UPB_INLINE upb_StringView
upb_strtable_ptr_iter_key(const upb_strtable_ptr_iter* i) {
  return upb_strtable_32_iter_key(i);
}
UPB_INLINE const void* upb_strtable_ptr_iter_value(
    const upb_strtable_ptr_iter* i) {
  return (const void*)(uintptr_t)upb_strtable_32_iter_value(i);
}
UPB_INLINE void upb_strtable_ptr_iter_setdone(upb_strtable_ptr_iter* i) {
  upb_strtable_32_iter_setdone(i);
}
UPB_INLINE bool upb_strtable_ptr_iter_isequal(const upb_strtable_ptr_iter* i1,
                                              const upb_strtable_ptr_iter* i2) {
  return upb_strtable_32_iter_isequal(i1, i2);
}
UPB_INLINE bool upb_strtable_ptr_next2(const upb_strtable_ptr* t,
                                       upb_StringView* key, const void** val,
                                       intptr_t* iter) {
  uint32_t val_t;
  if (upb_strtable_32_next2(t, key, &val_t, iter)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE void upb_strtable_ptr_removeiter(upb_strtable_ptr* t,
                                            intptr_t* iter) {
  upb_strtable_32_removeiter(t, iter);
}

#else

typedef upb_strtable_64 upb_strtable_ptr;
typedef upb_strtable_64_iter upb_strtable_ptr_iter;

UPB_INLINE bool upb_strtable_ptr_init(upb_strtable_ptr* t, size_t expected_size,
                                      upb_Arena* a) {
  return upb_strtable_64_init(t, expected_size, a);
}
UPB_INLINE size_t upb_strtable_ptr_count(const upb_strtable_ptr* t) {
  return upb_strtable_64_count(t);
}
UPB_INLINE bool upb_strtable_ptr_insert(upb_strtable_ptr* t, const char* key,
                                        size_t len, const void* val,
                                        upb_Arena* a) {
  return upb_strtable_64_insert(t, key, len, (uint64_t)(uintptr_t)val, a);
}
UPB_INLINE bool upb_strtable_ptr_lookup(const upb_strtable_ptr* t,
                                        const char* key, size_t len,
                                        const void** v) {
  uint64_t val;
  if (upb_strtable_64_lookup(t, key, len, &val)) {
    if (v) *v = (const void*)(uintptr_t)val;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_strtable_ptr_remove(upb_strtable_ptr* t, const char* key,
                                        size_t len, const void** val) {
  uint64_t val_t;
  if (upb_strtable_64_remove(t, key, len, &val_t)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE void upb_strtable_ptr_clear(upb_strtable_ptr* t) {
  upb_strtable_64_clear(t);
}
UPB_INLINE void upb_strtable_ptr_begin(upb_strtable_ptr_iter* i,
                                       const upb_strtable_ptr* t) {
  upb_strtable_64_begin(i, t);
}
UPB_INLINE void upb_strtable_ptr_next(upb_strtable_ptr_iter* i) {
  upb_strtable_64_next(i);
}
UPB_INLINE bool upb_strtable_ptr_done(const upb_strtable_ptr_iter* i) {
  return upb_strtable_64_done(i);
}
UPB_INLINE upb_StringView
upb_strtable_ptr_iter_key(const upb_strtable_ptr_iter* i) {
  return upb_strtable_64_iter_key(i);
}
UPB_INLINE const void* upb_strtable_ptr_iter_value(
    const upb_strtable_ptr_iter* i) {
  return (const void*)(uintptr_t)upb_strtable_64_iter_value(i);
}
UPB_INLINE void upb_strtable_ptr_iter_setdone(upb_strtable_ptr_iter* i) {
  upb_strtable_64_iter_setdone(i);
}
UPB_INLINE bool upb_strtable_ptr_iter_isequal(const upb_strtable_ptr_iter* i1,
                                              const upb_strtable_ptr_iter* i2) {
  return upb_strtable_64_iter_isequal(i1, i2);
}
UPB_INLINE bool upb_strtable_ptr_next2(const upb_strtable_ptr* t,
                                       upb_StringView* key, const void** val,
                                       intptr_t* iter) {
  uint64_t val_t;
  if (upb_strtable_64_next2(t, key, &val_t, iter)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE void upb_strtable_ptr_removeiter(upb_strtable_ptr* t,
                                            intptr_t* iter) {
  upb_strtable_64_removeiter(t, iter);
}

#endif

#undef UPB_STRTABLE_DECLARE

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_HASH_STR_TABLE_H_ */
