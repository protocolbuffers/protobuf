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
size_t upb_inttable_count(const upb_inttable* t);

// Inserts the given key into the hashtable with the given value.
// The key must not already exist in the hash table.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged.
UPB_NODISCARD bool upb_inttable_insert(upb_inttable* t, uintptr_t key,
                                       upb_value val, upb_Arena* a);

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

typedef struct {
  uint32_t val;
} upb_map_32;
typedef struct {
  uint64_t val;
} upb_map_64;
typedef struct {
  UPB_SIZE(uint32_t, uint64_t) val;
} upb_map_ptr;

typedef struct {
  upb_inttable t;
} upb_inttable_32_bool;
typedef struct {
  upb_inttable t;
} upb_inttable_64_bool;
typedef struct {
  upb_inttable t;
} upb_inttable_32_32;
typedef struct {
  upb_inttable t;
} upb_inttable_64_32;
typedef struct {
  upb_inttable t;
} upb_inttable_32_64;
typedef struct {
  upb_inttable t;
} upb_inttable_64_64;

#define UPB_INTTABLE_DECLARE(postfix, key_type, val_type, to_val, from_val)   \
  UPB_INLINE bool upb_inttable_##postfix##_init(upb_inttable_##postfix* t,    \
                                                upb_Arena* a) {               \
    return upb_inttable_init(&t->t, a);                                       \
  }                                                                           \
  UPB_INLINE size_t upb_inttable_##postfix##_count(                           \
      const upb_inttable_##postfix* t) {                                      \
    return upb_inttable_count(&t->t);                                         \
  }                                                                           \
  UPB_INLINE bool upb_inttable_##postfix##_insert(                            \
      upb_inttable_##postfix* t, key_type key, val_type val, upb_Arena* a) {  \
    return upb_inttable_insert(&t->t, (uintptr_t)key, to_val(val), a);        \
  }                                                                           \
  UPB_INLINE bool upb_inttable_##postfix##_lookup(                            \
      const upb_inttable_##postfix* t, key_type key, val_type* v) {           \
    upb_value tabval;                                                         \
    if (upb_inttable_lookup(&t->t, (uintptr_t)key, &tabval)) {                \
      if (v) *v = from_val(tabval);                                           \
      return true;                                                            \
    }                                                                         \
    return false;                                                             \
  }                                                                           \
  UPB_INLINE bool upb_inttable_##postfix##_remove(                            \
      upb_inttable_##postfix* t, key_type key, val_type* val) {               \
    upb_value tabval;                                                         \
    if (upb_inttable_remove(&t->t, (uintptr_t)key, &tabval)) {                \
      if (val) *val = from_val(tabval);                                       \
      return true;                                                            \
    }                                                                         \
    return false;                                                             \
  }                                                                           \
  UPB_INLINE bool upb_inttable_##postfix##_replace(                           \
      upb_inttable_##postfix* t, key_type key, val_type val) {                \
    return upb_inttable_replace(&t->t, (uintptr_t)key, to_val(val));          \
  }                                                                           \
  UPB_INLINE void upb_inttable_##postfix##_clear(upb_inttable_##postfix* t) { \
    upb_inttable_clear(&t->t);                                                \
  }                                                                           \
  UPB_INLINE bool upb_inttable_##postfix##_next(                              \
      const upb_inttable_##postfix* t, key_type* key, val_type* val,          \
      intptr_t* iter) {                                                       \
    uintptr_t k;                                                              \
    upb_value v;                                                              \
    if (upb_inttable_next(&t->t, &k, &v, iter)) {                             \
      if (key) *key = (key_type)k;                                            \
      if (val) *val = from_val(v);                                            \
      return true;                                                            \
    }                                                                         \
    return false;                                                             \
  }                                                                           \
  UPB_INLINE void upb_inttable_##postfix##_removeiter(                        \
      upb_inttable_##postfix* t, intptr_t* iter) {                            \
    upb_inttable_removeiter(&t->t, iter);                                     \
  }                                                                           \
  UPB_INLINE void upb_inttable_##postfix##_setentryvalue(                     \
      upb_inttable_##postfix* t, intptr_t iter, val_type v) {                 \
    upb_inttable_setentryvalue(&t->t, iter, to_val(v));                       \
  }                                                                           \
  UPB_INLINE bool upb_inttable_##postfix##_done(                              \
      const upb_inttable_##postfix* t, intptr_t i) {                          \
    return upb_inttable_done(&t->t, i);                                       \
  }                                                                           \
  UPB_INLINE key_type upb_inttable_##postfix##_iter_key(                      \
      const upb_inttable_##postfix* t, intptr_t iter) {                       \
    return (key_type)upb_inttable_iter_key(&t->t, iter);                      \
  }                                                                           \
  UPB_INLINE val_type upb_inttable_##postfix##_iter_value(                    \
      const upb_inttable_##postfix* t, intptr_t iter) {                       \
    return from_val(upb_inttable_iter_value(&t->t, iter));                    \
  }

UPB_INTTABLE_DECLARE(32_bool, uint32_t, bool, upb_value_bool, upb_value_getbool)
UPB_INTTABLE_DECLARE(64_bool, uint64_t, bool, upb_value_bool, upb_value_getbool)
UPB_INTTABLE_DECLARE(32_32, uint32_t, uint32_t, upb_value_uint32,
                     upb_value_getuint32)
UPB_INTTABLE_DECLARE(64_32, uint64_t, uint32_t, upb_value_uint32,
                     upb_value_getuint32)
UPB_INTTABLE_DECLARE(32_64, uint32_t, uint64_t, upb_value_uint64,
                     upb_value_getuint64)
UPB_INTTABLE_DECLARE(64_64, uint64_t, uint64_t, upb_value_uint64,
                     upb_value_getuint64)

#if UINTPTR_MAX == 0xffffffff

typedef upb_inttable_32_32 upb_inttable_32_ptr;
UPB_INLINE bool upb_inttable_32_ptr_init(upb_inttable_32_ptr* t, upb_Arena* a) {
  return upb_inttable_32_32_init(t, a);
}
UPB_INLINE size_t upb_inttable_32_ptr_count(const upb_inttable_32_ptr* t) {
  return upb_inttable_32_32_count(t);
}
UPB_INLINE bool upb_inttable_32_ptr_insert(upb_inttable_32_ptr* t, uint32_t key,
                                           const void* val, upb_Arena* a) {
  return upb_inttable_32_32_insert(t, key, (uint32_t)(uintptr_t)val, a);
}
UPB_INLINE bool upb_inttable_32_ptr_lookup(const upb_inttable_32_ptr* t,
                                           uint32_t key, const void** v) {
  uint32_t val;
  if (upb_inttable_32_32_lookup(t, key, &val)) {
    if (v) *v = (const void*)(uintptr_t)val;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_32_ptr_remove(upb_inttable_32_ptr* t, uint32_t key,
                                           const void** val) {
  uint32_t val_t;
  if (upb_inttable_32_32_remove(t, key, &val_t)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_32_ptr_replace(upb_inttable_32_ptr* t,
                                            uint32_t key, const void* val) {
  return upb_inttable_32_32_replace(t, key, (uint32_t)(uintptr_t)val);
}
UPB_INLINE void upb_inttable_32_ptr_clear(upb_inttable_32_ptr* t) {
  upb_inttable_32_32_clear(t);
}
UPB_INLINE bool upb_inttable_32_ptr_next(const upb_inttable_32_ptr* t,
                                         uint32_t* key, const void** val,
                                         intptr_t* iter) {
  uint32_t val_t;
  if (upb_inttable_32_32_next(t, key, &val_t, iter)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_32_ptr_removeiter(upb_inttable_32_ptr* t,
                                               intptr_t* iter) {
  upb_inttable_32_32_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_32_ptr_setentryvalue(upb_inttable_32_ptr* t,
                                                  intptr_t iter,
                                                  const void* v) {
  upb_inttable_32_32_setentryvalue(t, iter, (uint32_t)(uintptr_t)v);
}
UPB_INLINE bool upb_inttable_32_ptr_done(const upb_inttable_32_ptr* t,
                                         intptr_t i) {
  return upb_inttable_32_32_done(t, i);
}
UPB_INLINE uint32_t upb_inttable_32_ptr_iter_key(const upb_inttable_32_ptr* t,
                                                 intptr_t iter) {
  return upb_inttable_32_32_iter_key(t, iter);
}
UPB_INLINE const void* upb_inttable_32_ptr_iter_value(
    const upb_inttable_32_ptr* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_32_32_iter_value(t, iter);
}

typedef upb_inttable_64_32 upb_inttable_64_ptr;
UPB_INLINE bool upb_inttable_64_ptr_init(upb_inttable_64_ptr* t, upb_Arena* a) {
  return upb_inttable_64_32_init(t, a);
}
UPB_INLINE size_t upb_inttable_64_ptr_count(const upb_inttable_64_ptr* t) {
  return upb_inttable_64_32_count(t);
}
UPB_INLINE bool upb_inttable_64_ptr_insert(upb_inttable_64_ptr* t, uint64_t key,
                                           const void* val, upb_Arena* a) {
  return upb_inttable_64_32_insert(t, key, (uint32_t)(uintptr_t)val, a);
}
UPB_INLINE bool upb_inttable_64_ptr_lookup(const upb_inttable_64_ptr* t,
                                           uint64_t key, const void** v) {
  uint32_t val;
  if (upb_inttable_64_32_lookup(t, key, &val)) {
    if (v) *v = (const void*)(uintptr_t)val;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_64_ptr_remove(upb_inttable_64_ptr* t, uint64_t key,
                                           const void** val) {
  uint32_t val_t;
  if (upb_inttable_64_32_remove(t, key, &val_t)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_64_ptr_replace(upb_inttable_64_ptr* t,
                                            uint64_t key, const void* val) {
  return upb_inttable_64_32_replace(t, key, (uint32_t)(uintptr_t)val);
}
UPB_INLINE void upb_inttable_64_ptr_clear(upb_inttable_64_ptr* t) {
  upb_inttable_64_32_clear(t);
}
UPB_INLINE bool upb_inttable_64_ptr_next(const upb_inttable_64_ptr* t,
                                         uint64_t* key, const void** val,
                                         intptr_t* iter) {
  uint32_t val_t;
  if (upb_inttable_64_32_next(t, key, &val_t, iter)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_64_ptr_removeiter(upb_inttable_64_ptr* t,
                                               intptr_t* iter) {
  upb_inttable_64_32_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_64_ptr_setentryvalue(upb_inttable_64_ptr* t,
                                                  intptr_t iter,
                                                  const void* v) {
  upb_inttable_64_32_setentryvalue(t, iter, (uint32_t)(uintptr_t)v);
}
UPB_INLINE bool upb_inttable_64_ptr_done(const upb_inttable_64_ptr* t,
                                         intptr_t i) {
  return upb_inttable_64_32_done(t, i);
}
UPB_INLINE uint64_t upb_inttable_64_ptr_iter_key(const upb_inttable_64_ptr* t,
                                                 intptr_t iter) {
  return upb_inttable_64_32_iter_key(t, iter);
}
UPB_INLINE const void* upb_inttable_64_ptr_iter_value(
    const upb_inttable_64_ptr* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_64_32_iter_value(t, iter);
}

typedef upb_inttable_32_bool upb_inttable_ptr_bool;
UPB_INLINE bool upb_inttable_ptr_bool_init(upb_inttable_ptr_bool* t,
                                           upb_Arena* a) {
  return upb_inttable_32_bool_init(t, a);
}
UPB_INLINE size_t upb_inttable_ptr_bool_count(const upb_inttable_ptr_bool* t) {
  return upb_inttable_32_bool_count(t);
}
UPB_INLINE bool upb_inttable_ptr_bool_insert(upb_inttable_ptr_bool* t,
                                             const void* key, bool val,
                                             upb_Arena* a) {
  return upb_inttable_32_bool_insert(t, (uint32_t)(uintptr_t)key, val, a);
}
UPB_INLINE bool upb_inttable_ptr_bool_lookup(const upb_inttable_ptr_bool* t,
                                             const void* key, bool* v) {
  return upb_inttable_32_bool_lookup(t, (uint32_t)(uintptr_t)key, v);
}
UPB_INLINE bool upb_inttable_ptr_bool_remove(upb_inttable_ptr_bool* t,
                                             const void* key, bool* val) {
  return upb_inttable_32_bool_remove(t, (uint32_t)(uintptr_t)key, val);
}
UPB_INLINE bool upb_inttable_ptr_bool_replace(upb_inttable_ptr_bool* t,
                                              const void* key, bool val) {
  return upb_inttable_32_bool_replace(t, (uint32_t)(uintptr_t)key, val);
}
UPB_INLINE void upb_inttable_ptr_bool_clear(upb_inttable_ptr_bool* t) {
  upb_inttable_32_bool_clear(t);
}
UPB_INLINE bool upb_inttable_ptr_bool_next(const upb_inttable_ptr_bool* t,
                                           const void** key, bool* val,
                                           intptr_t* iter) {
  uint32_t k;
  if (upb_inttable_32_bool_next(t, &k, val, iter)) {
    if (key) *key = (const void*)(uintptr_t)k;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_ptr_bool_removeiter(upb_inttable_ptr_bool* t,
                                                 intptr_t* iter) {
  upb_inttable_32_bool_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_ptr_bool_setentryvalue(upb_inttable_ptr_bool* t,
                                                    intptr_t iter, bool v) {
  upb_inttable_32_bool_setentryvalue(t, iter, v);
}
UPB_INLINE bool upb_inttable_ptr_bool_done(const upb_inttable_ptr_bool* t,
                                           intptr_t i) {
  return upb_inttable_32_bool_done(t, i);
}
UPB_INLINE const void* upb_inttable_ptr_bool_iter_key(
    const upb_inttable_ptr_bool* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_32_bool_iter_key(t, iter);
}
UPB_INLINE bool upb_inttable_ptr_bool_iter_value(const upb_inttable_ptr_bool* t,
                                                 intptr_t iter) {
  return upb_inttable_32_bool_iter_value(t, iter);
}

typedef upb_inttable_32_32 upb_inttable_ptr_32;
UPB_INLINE bool upb_inttable_ptr_32_init(upb_inttable_ptr_32* t, upb_Arena* a) {
  return upb_inttable_32_32_init(t, a);
}
UPB_INLINE size_t upb_inttable_ptr_32_count(const upb_inttable_ptr_32* t) {
  return upb_inttable_32_32_count(t);
}
UPB_INLINE bool upb_inttable_ptr_32_insert(upb_inttable_ptr_32* t,
                                           const void* key, uint32_t val,
                                           upb_Arena* a) {
  return upb_inttable_32_32_insert(t, (uint32_t)(uintptr_t)key, val, a);
}
UPB_INLINE bool upb_inttable_ptr_32_lookup(const upb_inttable_ptr_32* t,
                                           const void* key, uint32_t* v) {
  return upb_inttable_32_32_lookup(t, (uint32_t)(uintptr_t)key, v);
}
UPB_INLINE bool upb_inttable_ptr_32_remove(upb_inttable_ptr_32* t,
                                           const void* key, uint32_t* val) {
  return upb_inttable_32_32_remove(t, (uint32_t)(uintptr_t)key, val);
}
UPB_INLINE bool upb_inttable_ptr_32_replace(upb_inttable_ptr_32* t,
                                            const void* key, uint32_t val) {
  return upb_inttable_32_32_replace(t, (uint32_t)(uintptr_t)key, val);
}
UPB_INLINE void upb_inttable_ptr_32_clear(upb_inttable_ptr_32* t) {
  upb_inttable_32_32_clear(t);
}
UPB_INLINE bool upb_inttable_ptr_32_next(const upb_inttable_ptr_32* t,
                                         const void** key, uint32_t* val,
                                         intptr_t* iter) {
  uint32_t k;
  if (upb_inttable_32_32_next(t, &k, val, iter)) {
    if (key) *key = (const void*)(uintptr_t)k;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_ptr_32_removeiter(upb_inttable_ptr_32* t,
                                               intptr_t* iter) {
  upb_inttable_32_32_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_ptr_32_setentryvalue(upb_inttable_ptr_32* t,
                                                  intptr_t iter, uint32_t v) {
  upb_inttable_32_32_setentryvalue(t, iter, v);
}
UPB_INLINE bool upb_inttable_ptr_32_done(const upb_inttable_ptr_32* t,
                                         intptr_t i) {
  return upb_inttable_32_32_done(t, i);
}
UPB_INLINE const void* upb_inttable_ptr_32_iter_key(
    const upb_inttable_ptr_32* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_32_32_iter_key(t, iter);
}
UPB_INLINE uint32_t upb_inttable_ptr_32_iter_value(const upb_inttable_ptr_32* t,
                                                   intptr_t iter) {
  return upb_inttable_32_32_iter_value(t, iter);
}

typedef upb_inttable_32_64 upb_inttable_ptr_64;
UPB_INLINE bool upb_inttable_ptr_64_init(upb_inttable_ptr_64* t, upb_Arena* a) {
  return upb_inttable_32_64_init(t, a);
}
UPB_INLINE size_t upb_inttable_ptr_64_count(const upb_inttable_ptr_64* t) {
  return upb_inttable_32_64_count(t);
}
UPB_INLINE bool upb_inttable_ptr_64_insert(upb_inttable_ptr_64* t,
                                           const void* key, uint64_t val,
                                           upb_Arena* a) {
  return upb_inttable_32_64_insert(t, (uint32_t)(uintptr_t)key, val, a);
}
UPB_INLINE bool upb_inttable_ptr_64_lookup(const upb_inttable_ptr_64* t,
                                           const void* key, uint64_t* v) {
  return upb_inttable_32_64_lookup(t, (uint32_t)(uintptr_t)key, v);
}
UPB_INLINE bool upb_inttable_ptr_64_remove(upb_inttable_ptr_64* t,
                                           const void* key, uint64_t* val) {
  return upb_inttable_32_64_remove(t, (uint32_t)(uintptr_t)key, val);
}
UPB_INLINE bool upb_inttable_ptr_64_replace(upb_inttable_ptr_64* t,
                                            const void* key, uint64_t val) {
  return upb_inttable_32_64_replace(t, (uint32_t)(uintptr_t)key, val);
}
UPB_INLINE void upb_inttable_ptr_64_clear(upb_inttable_ptr_64* t) {
  upb_inttable_32_64_clear(t);
}
UPB_INLINE bool upb_inttable_ptr_64_next(const upb_inttable_ptr_64* t,
                                         const void** key, uint64_t* val,
                                         intptr_t* iter) {
  uint32_t k;
  if (upb_inttable_32_64_next(t, &k, val, iter)) {
    if (key) *key = (const void*)(uintptr_t)k;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_ptr_64_removeiter(upb_inttable_ptr_64* t,
                                               intptr_t* iter) {
  upb_inttable_32_64_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_ptr_64_setentryvalue(upb_inttable_ptr_64* t,
                                                  intptr_t iter, uint64_t v) {
  upb_inttable_32_64_setentryvalue(t, iter, v);
}
UPB_INLINE bool upb_inttable_ptr_64_done(const upb_inttable_ptr_64* t,
                                         intptr_t i) {
  return upb_inttable_32_64_done(t, i);
}
UPB_INLINE const void* upb_inttable_ptr_64_iter_key(
    const upb_inttable_ptr_64* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_32_64_iter_key(t, iter);
}
UPB_INLINE uint64_t upb_inttable_ptr_64_iter_value(const upb_inttable_ptr_64* t,
                                                   intptr_t iter) {
  return upb_inttable_32_64_iter_value(t, iter);
}

typedef upb_inttable_32_32 upb_inttable_ptr_ptr;
UPB_INLINE bool upb_inttable_ptr_ptr_init(upb_inttable_ptr_ptr* t,
                                          upb_Arena* a) {
  return upb_inttable_32_32_init(t, a);
}
UPB_INLINE size_t upb_inttable_ptr_ptr_count(const upb_inttable_ptr_ptr* t) {
  return upb_inttable_32_32_count(t);
}
UPB_INLINE bool upb_inttable_ptr_ptr_insert(upb_inttable_ptr_ptr* t,
                                            const void* key, const void* val,
                                            upb_Arena* a) {
  return upb_inttable_32_32_insert(t, (uint32_t)(uintptr_t)key,
                                   (uint32_t)(uintptr_t)val, a);
}
UPB_INLINE bool upb_inttable_ptr_ptr_lookup(const upb_inttable_ptr_ptr* t,
                                            const void* key, const void** v) {
  uint32_t val;
  if (upb_inttable_32_32_lookup(t, (uint32_t)(uintptr_t)key, &val)) {
    if (v) *v = (const void*)(uintptr_t)val;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_ptr_ptr_remove(upb_inttable_ptr_ptr* t,
                                            const void* key, const void** val) {
  uint32_t val_t;
  if (upb_inttable_32_32_remove(t, (uint32_t)(uintptr_t)key, &val_t)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_ptr_ptr_replace(upb_inttable_ptr_ptr* t,
                                             const void* key, const void* val) {
  return upb_inttable_32_32_replace(t, (uint32_t)(uintptr_t)key,
                                    (uint32_t)(uintptr_t)val);
}
UPB_INLINE void upb_inttable_ptr_ptr_clear(upb_inttable_ptr_ptr* t) {
  upb_inttable_32_32_clear(t);
}
UPB_INLINE bool upb_inttable_ptr_ptr_next(const upb_inttable_ptr_ptr* t,
                                          const void** key, const void** val,
                                          intptr_t* iter) {
  uint32_t k, v;
  if (upb_inttable_32_32_next(t, &k, &v, iter)) {
    if (key) *key = (const void*)(uintptr_t)k;
    if (val) *val = (const void*)(uintptr_t)v;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_ptr_ptr_removeiter(upb_inttable_ptr_ptr* t,
                                                intptr_t* iter) {
  upb_inttable_32_32_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_ptr_ptr_setentryvalue(upb_inttable_ptr_ptr* t,
                                                   intptr_t iter,
                                                   const void* v) {
  upb_inttable_32_32_setentryvalue(t, iter, (uint32_t)(uintptr_t)v);
}
UPB_INLINE bool upb_inttable_ptr_ptr_done(const upb_inttable_ptr_ptr* t,
                                          intptr_t i) {
  return upb_inttable_32_32_done(t, i);
}
UPB_INLINE const void* upb_inttable_ptr_ptr_iter_key(
    const upb_inttable_ptr_ptr* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_32_32_iter_key(t, iter);
}
UPB_INLINE const void* upb_inttable_ptr_ptr_iter_value(
    const upb_inttable_ptr_ptr* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_32_32_iter_value(t, iter);
}

#else

typedef upb_inttable_32_64 upb_inttable_32_ptr;
UPB_INLINE bool upb_inttable_32_ptr_init(upb_inttable_32_ptr* t, upb_Arena* a) {
  return upb_inttable_32_64_init(t, a);
}
UPB_INLINE size_t upb_inttable_32_ptr_count(const upb_inttable_32_ptr* t) {
  return upb_inttable_32_64_count(t);
}
UPB_INLINE bool upb_inttable_32_ptr_insert(upb_inttable_32_ptr* t, uint32_t key,
                                           const void* val, upb_Arena* a) {
  return upb_inttable_32_64_insert(t, key, (uint64_t)(uintptr_t)val, a);
}
UPB_INLINE bool upb_inttable_32_ptr_lookup(const upb_inttable_32_ptr* t,
                                           uint32_t key, const void** v) {
  uint64_t val;
  if (upb_inttable_32_64_lookup(t, key, &val)) {
    if (v) *v = (const void*)(uintptr_t)val;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_32_ptr_remove(upb_inttable_32_ptr* t, uint32_t key,
                                           const void** val) {
  uint64_t val_t;
  if (upb_inttable_32_64_remove(t, key, &val_t)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_32_ptr_replace(upb_inttable_32_ptr* t,
                                            uint32_t key, const void* val) {
  return upb_inttable_32_64_replace(t, key, (uint64_t)(uintptr_t)val);
}
UPB_INLINE void upb_inttable_32_ptr_clear(upb_inttable_32_ptr* t) {
  upb_inttable_32_64_clear(t);
}
UPB_INLINE bool upb_inttable_32_ptr_next(const upb_inttable_32_ptr* t,
                                         uint32_t* key, const void** val,
                                         intptr_t* iter) {
  uint64_t val_t;
  if (upb_inttable_32_64_next(t, key, &val_t, iter)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_32_ptr_removeiter(upb_inttable_32_ptr* t,
                                               intptr_t* iter) {
  upb_inttable_32_64_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_32_ptr_setentryvalue(upb_inttable_32_ptr* t,
                                                  intptr_t iter,
                                                  const void* v) {
  upb_inttable_32_64_setentryvalue(t, iter, (uint64_t)(uintptr_t)v);
}
UPB_INLINE bool upb_inttable_32_ptr_done(const upb_inttable_32_ptr* t,
                                         intptr_t i) {
  return upb_inttable_32_64_done(t, i);
}
UPB_INLINE uint32_t upb_inttable_32_ptr_iter_key(const upb_inttable_32_ptr* t,
                                                 intptr_t iter) {
  return upb_inttable_32_64_iter_key(t, iter);
}
UPB_INLINE const void* upb_inttable_32_ptr_iter_value(
    const upb_inttable_32_ptr* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_32_64_iter_value(t, iter);
}

typedef upb_inttable_64_64 upb_inttable_64_ptr;
UPB_INLINE bool upb_inttable_64_ptr_init(upb_inttable_64_ptr* t, upb_Arena* a) {
  return upb_inttable_64_64_init(t, a);
}
UPB_INLINE size_t upb_inttable_64_ptr_count(const upb_inttable_64_ptr* t) {
  return upb_inttable_64_64_count(t);
}
UPB_INLINE bool upb_inttable_64_ptr_insert(upb_inttable_64_ptr* t, uint64_t key,
                                           const void* val, upb_Arena* a) {
  return upb_inttable_64_64_insert(t, key, (uint64_t)(uintptr_t)val, a);
}
UPB_INLINE bool upb_inttable_64_ptr_lookup(const upb_inttable_64_ptr* t,
                                           uint64_t key, const void** v) {
  uint64_t val;
  if (upb_inttable_64_64_lookup(t, key, &val)) {
    if (v) *v = (const void*)(uintptr_t)val;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_64_ptr_remove(upb_inttable_64_ptr* t, uint64_t key,
                                           const void** val) {
  uint64_t val_t;
  if (upb_inttable_64_64_remove(t, key, &val_t)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_64_ptr_replace(upb_inttable_64_ptr* t,
                                            uint64_t key, const void* val) {
  return upb_inttable_64_64_replace(t, key, (uint64_t)(uintptr_t)val);
}
UPB_INLINE void upb_inttable_64_ptr_clear(upb_inttable_64_ptr* t) {
  upb_inttable_64_64_clear(t);
}
UPB_INLINE bool upb_inttable_64_ptr_next(const upb_inttable_64_ptr* t,
                                         uint64_t* key, const void** val,
                                         intptr_t* iter) {
  uint64_t val_t;
  if (upb_inttable_64_64_next(t, key, &val_t, iter)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_64_ptr_removeiter(upb_inttable_64_ptr* t,
                                               intptr_t* iter) {
  upb_inttable_64_64_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_64_ptr_setentryvalue(upb_inttable_64_ptr* t,
                                                  intptr_t iter,
                                                  const void* v) {
  upb_inttable_64_64_setentryvalue(t, iter, (uint64_t)(uintptr_t)v);
}
UPB_INLINE bool upb_inttable_64_ptr_done(const upb_inttable_64_ptr* t,
                                         intptr_t i) {
  return upb_inttable_64_64_done(t, i);
}
UPB_INLINE uint64_t upb_inttable_64_ptr_iter_key(const upb_inttable_64_ptr* t,
                                                 intptr_t iter) {
  return upb_inttable_64_64_iter_key(t, iter);
}
UPB_INLINE const void* upb_inttable_64_ptr_iter_value(
    const upb_inttable_64_ptr* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_64_64_iter_value(t, iter);
}

typedef upb_inttable_64_bool upb_inttable_ptr_bool;
UPB_INLINE bool upb_inttable_ptr_bool_init(upb_inttable_ptr_bool* t,
                                           upb_Arena* a) {
  return upb_inttable_64_bool_init(t, a);
}
UPB_INLINE size_t upb_inttable_ptr_bool_count(const upb_inttable_ptr_bool* t) {
  return upb_inttable_64_bool_count(t);
}
UPB_INLINE bool upb_inttable_ptr_bool_insert(upb_inttable_ptr_bool* t,
                                             const void* key, bool val,
                                             upb_Arena* a) {
  return upb_inttable_64_bool_insert(t, (uint64_t)(uintptr_t)key, val, a);
}
UPB_INLINE bool upb_inttable_ptr_bool_lookup(const upb_inttable_ptr_bool* t,
                                             const void* key, bool* v) {
  return upb_inttable_64_bool_lookup(t, (uint64_t)(uintptr_t)key, v);
}
UPB_INLINE bool upb_inttable_ptr_bool_remove(upb_inttable_ptr_bool* t,
                                             const void* key, bool* val) {
  return upb_inttable_64_bool_remove(t, (uint64_t)(uintptr_t)key, val);
}
UPB_INLINE bool upb_inttable_ptr_bool_replace(upb_inttable_ptr_bool* t,
                                              const void* key, bool val) {
  return upb_inttable_64_bool_replace(t, (uint64_t)(uintptr_t)key, val);
}
UPB_INLINE void upb_inttable_ptr_bool_clear(upb_inttable_ptr_bool* t) {
  upb_inttable_64_bool_clear(t);
}
UPB_INLINE bool upb_inttable_ptr_bool_next(const upb_inttable_ptr_bool* t,
                                           const void** key, bool* val,
                                           intptr_t* iter) {
  uint64_t k;
  if (upb_inttable_64_bool_next(t, &k, val, iter)) {
    if (key) *key = (const void*)(uintptr_t)k;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_ptr_bool_removeiter(upb_inttable_ptr_bool* t,
                                                 intptr_t* iter) {
  upb_inttable_64_bool_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_ptr_bool_setentryvalue(upb_inttable_ptr_bool* t,
                                                    intptr_t iter, bool v) {
  upb_inttable_64_bool_setentryvalue(t, iter, v);
}
UPB_INLINE bool upb_inttable_ptr_bool_done(const upb_inttable_ptr_bool* t,
                                           intptr_t i) {
  return upb_inttable_64_bool_done(t, i);
}
UPB_INLINE const void* upb_inttable_ptr_bool_iter_key(
    const upb_inttable_ptr_bool* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_64_bool_iter_key(t, iter);
}
UPB_INLINE bool upb_inttable_ptr_bool_iter_value(const upb_inttable_ptr_bool* t,
                                                 intptr_t iter) {
  return upb_inttable_64_bool_iter_value(t, iter);
}

typedef upb_inttable_64_32 upb_inttable_ptr_32;
UPB_INLINE bool upb_inttable_ptr_32_init(upb_inttable_ptr_32* t, upb_Arena* a) {
  return upb_inttable_64_32_init(t, a);
}
UPB_INLINE size_t upb_inttable_ptr_32_count(const upb_inttable_ptr_32* t) {
  return upb_inttable_64_32_count(t);
}
UPB_INLINE bool upb_inttable_ptr_32_insert(upb_inttable_ptr_32* t,
                                           const void* key, uint32_t val,
                                           upb_Arena* a) {
  return upb_inttable_64_32_insert(t, (uint64_t)(uintptr_t)key, val, a);
}
UPB_INLINE bool upb_inttable_ptr_32_lookup(const upb_inttable_ptr_32* t,
                                           const void* key, uint32_t* v) {
  return upb_inttable_64_32_lookup(t, (uint64_t)(uintptr_t)key, v);
}
UPB_INLINE bool upb_inttable_ptr_32_remove(upb_inttable_ptr_32* t,
                                           const void* key, uint32_t* val) {
  return upb_inttable_64_32_remove(t, (uint64_t)(uintptr_t)key, val);
}
UPB_INLINE bool upb_inttable_ptr_32_replace(upb_inttable_ptr_32* t,
                                            const void* key, uint32_t val) {
  return upb_inttable_64_32_replace(t, (uint64_t)(uintptr_t)key, val);
}
UPB_INLINE void upb_inttable_ptr_32_clear(upb_inttable_ptr_32* t) {
  upb_inttable_64_32_clear(t);
}
UPB_INLINE bool upb_inttable_ptr_32_next(const upb_inttable_ptr_32* t,
                                         const void** key, uint32_t* val,
                                         intptr_t* iter) {
  uint64_t k;
  if (upb_inttable_64_32_next(t, &k, val, iter)) {
    if (key) *key = (const void*)(uintptr_t)k;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_ptr_32_removeiter(upb_inttable_ptr_32* t,
                                               intptr_t* iter) {
  upb_inttable_64_32_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_ptr_32_setentryvalue(upb_inttable_ptr_32* t,
                                                  intptr_t iter, uint32_t v) {
  upb_inttable_64_32_setentryvalue(t, iter, v);
}
UPB_INLINE bool upb_inttable_ptr_32_done(const upb_inttable_ptr_32* t,
                                         intptr_t i) {
  return upb_inttable_64_32_done(t, i);
}
UPB_INLINE const void* upb_inttable_ptr_32_iter_key(
    const upb_inttable_ptr_32* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_64_32_iter_key(t, iter);
}
UPB_INLINE uint32_t upb_inttable_ptr_32_iter_value(const upb_inttable_ptr_32* t,
                                                   intptr_t iter) {
  return upb_inttable_64_32_iter_value(t, iter);
}

typedef upb_inttable_64_64 upb_inttable_ptr_64;
UPB_INLINE bool upb_inttable_ptr_64_init(upb_inttable_ptr_64* t, upb_Arena* a) {
  return upb_inttable_64_64_init(t, a);
}
UPB_INLINE size_t upb_inttable_ptr_64_count(const upb_inttable_ptr_64* t) {
  return upb_inttable_64_64_count(t);
}
UPB_INLINE bool upb_inttable_ptr_64_insert(upb_inttable_ptr_64* t,
                                           const void* key, uint64_t val,
                                           upb_Arena* a) {
  return upb_inttable_64_64_insert(t, (uint64_t)(uintptr_t)key, val, a);
}
UPB_INLINE bool upb_inttable_ptr_64_lookup(const upb_inttable_ptr_64* t,
                                           const void* key, uint64_t* v) {
  return upb_inttable_64_64_lookup(t, (uint64_t)(uintptr_t)key, v);
}
UPB_INLINE bool upb_inttable_ptr_64_remove(upb_inttable_ptr_64* t,
                                           const void* key, uint64_t* val) {
  return upb_inttable_64_64_remove(t, (uint64_t)(uintptr_t)key, val);
}
UPB_INLINE bool upb_inttable_ptr_64_replace(upb_inttable_ptr_64* t,
                                            const void* key, uint64_t val) {
  return upb_inttable_64_64_replace(t, (uint64_t)(uintptr_t)key, val);
}
UPB_INLINE void upb_inttable_ptr_64_clear(upb_inttable_ptr_64* t) {
  upb_inttable_64_64_clear(t);
}
UPB_INLINE bool upb_inttable_ptr_64_next(const upb_inttable_ptr_64* t,
                                         const void** key, uint64_t* val,
                                         intptr_t* iter) {
  uint64_t k;
  if (upb_inttable_64_64_next(t, &k, val, iter)) {
    if (key) *key = (const void*)(uintptr_t)k;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_ptr_64_removeiter(upb_inttable_ptr_64* t,
                                               intptr_t* iter) {
  upb_inttable_64_64_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_ptr_64_setentryvalue(upb_inttable_ptr_64* t,
                                                  intptr_t iter, uint64_t v) {
  upb_inttable_64_64_setentryvalue(t, iter, v);
}
UPB_INLINE bool upb_inttable_ptr_64_done(const upb_inttable_ptr_64* t,
                                         intptr_t i) {
  return upb_inttable_64_64_done(t, i);
}
UPB_INLINE const void* upb_inttable_ptr_64_iter_key(
    const upb_inttable_ptr_64* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_64_64_iter_key(t, iter);
}
UPB_INLINE uint64_t upb_inttable_ptr_64_iter_value(const upb_inttable_ptr_64* t,
                                                   intptr_t iter) {
  return upb_inttable_64_64_iter_value(t, iter);
}

typedef upb_inttable_64_64 upb_inttable_ptr_ptr;
UPB_INLINE bool upb_inttable_ptr_ptr_init(upb_inttable_ptr_ptr* t,
                                          upb_Arena* a) {
  return upb_inttable_64_64_init(t, a);
}
UPB_INLINE size_t upb_inttable_ptr_ptr_count(const upb_inttable_ptr_ptr* t) {
  return upb_inttable_64_64_count(t);
}
UPB_INLINE bool upb_inttable_ptr_ptr_insert(upb_inttable_ptr_ptr* t,
                                            const void* key, const void* val,
                                            upb_Arena* a) {
  return upb_inttable_64_64_insert(t, (uint64_t)(uintptr_t)key,
                                   (uint64_t)(uintptr_t)val, a);
}
UPB_INLINE bool upb_inttable_ptr_ptr_lookup(const upb_inttable_ptr_ptr* t,
                                            const void* key, const void** v) {
  uint64_t val;
  if (upb_inttable_64_64_lookup(t, (uint64_t)(uintptr_t)key, &val)) {
    if (v) *v = (const void*)(uintptr_t)val;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_ptr_ptr_remove(upb_inttable_ptr_ptr* t,
                                            const void* key, const void** val) {
  uint64_t val_t;
  if (upb_inttable_64_64_remove(t, (uint64_t)(uintptr_t)key, &val_t)) {
    if (val) *val = (const void*)(uintptr_t)val_t;
    return true;
  }
  return false;
}
UPB_INLINE bool upb_inttable_ptr_ptr_replace(upb_inttable_ptr_ptr* t,
                                             const void* key, const void* val) {
  return upb_inttable_64_64_replace(t, (uint64_t)(uintptr_t)key,
                                    (uint64_t)(uintptr_t)val);
}
UPB_INLINE void upb_inttable_ptr_ptr_clear(upb_inttable_ptr_ptr* t) {
  upb_inttable_64_64_clear(t);
}
UPB_INLINE bool upb_inttable_ptr_ptr_next(const upb_inttable_ptr_ptr* t,
                                          const void** key, const void** val,
                                          intptr_t* iter) {
  uint64_t k, v;
  if (upb_inttable_64_64_next(t, &k, &v, iter)) {
    if (key) *key = (const void*)(uintptr_t)k;
    if (val) *val = (const void*)(uintptr_t)v;
    return true;
  }
  return false;
}
UPB_INLINE void upb_inttable_ptr_ptr_removeiter(upb_inttable_ptr_ptr* t,
                                                intptr_t* iter) {
  upb_inttable_64_64_removeiter(t, iter);
}
UPB_INLINE void upb_inttable_ptr_ptr_setentryvalue(upb_inttable_ptr_ptr* t,
                                                   intptr_t iter,
                                                   const void* v) {
  upb_inttable_64_64_setentryvalue(t, iter, (uint64_t)(uintptr_t)v);
}
UPB_INLINE bool upb_inttable_ptr_ptr_done(const upb_inttable_ptr_ptr* t,
                                          intptr_t i) {
  return upb_inttable_64_64_done(t, i);
}
UPB_INLINE const void* upb_inttable_ptr_ptr_iter_key(
    const upb_inttable_ptr_ptr* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_64_64_iter_key(t, iter);
}
UPB_INLINE const void* upb_inttable_ptr_ptr_iter_value(
    const upb_inttable_ptr_ptr* t, intptr_t iter) {
  return (const void*)(uintptr_t)upb_inttable_64_64_iter_value(t, iter);
}

#endif

#undef UPB_INTTABLE_DECLARE

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_HASH_INT_TABLE_H_ */
