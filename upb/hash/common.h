// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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

#ifndef UPB_HASH_COMMON_H_
#define UPB_HASH_COMMON_H_

#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_value ******************************************************************/

typedef struct {
  uint64_t val;
} upb_value;

/* For each value ctype, define the following set of functions:
 *
 * // Get/set an int32 from a upb_value.
 * int32_t upb_value_getint32(upb_value val);
 * void upb_value_setint32(upb_value *val, int32_t cval);
 *
 * // Construct a new upb_value from an int32.
 * upb_value upb_value_int32(int32_t val); */
#define FUNCS(name, membername, type_t, converter)                   \
  UPB_INLINE void upb_value_set##name(upb_value* val, type_t cval) { \
    val->val = (uint64_t)cval;                                       \
  }                                                                  \
  UPB_INLINE upb_value upb_value_##name(type_t val) {                \
    upb_value ret;                                                   \
    upb_value_set##name(&ret, val);                                  \
    return ret;                                                      \
  }                                                                  \
  UPB_INLINE type_t upb_value_get##name(upb_value val) {             \
    return (type_t)(converter)val.val;                               \
  }

FUNCS(int32, int32, int32_t, int32_t)
FUNCS(int64, int64, int64_t, int64_t)
FUNCS(uint32, uint32, uint32_t, uint32_t)
FUNCS(uint64, uint64, uint64_t, uint64_t)
FUNCS(bool, _bool, bool, bool)
FUNCS(cstr, cstr, char*, uintptr_t)
FUNCS(uintptr, uptr, uintptr_t, uintptr_t)
FUNCS(ptr, ptr, void*, uintptr_t)
FUNCS(constptr, constptr, const void*, uintptr_t)

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

/* upb_key *****************************************************************/

// A uint32 size followed by that number of bytes stored contiguously.
typedef struct {
  uint32_t size;
  const char data[];
} upb_SizePrefixString;

/* Either:
 *   1. an actual integer key
 *   2. a SizePrefixString*, owned by us.
 *
 * ...depending on whether this is a string table or an int table. */
typedef union {
  uintptr_t num;
  const upb_SizePrefixString* str;
} upb_key;

UPB_INLINE upb_StringView upb_key_strview(upb_key key) {
  return upb_StringView_FromDataAndSize(key.str->data, key.str->size);
}

/* upb_table ******************************************************************/

typedef struct _upb_tabent {
  upb_value val;
  upb_key key;

  /* Internal chaining.  This is const so we can create static initializers for
   * tables.  We cast away const sometimes, but *only* when the containing
   * upb_table is known to be non-const.  This requires a bit of care, but
   * the subtlety is confined to table.c. */
  const struct _upb_tabent* next;
} upb_tabent;

typedef struct {
  upb_tabent* entries;
  /* Number of entries in the hash part. */
  uint32_t count;

  /* Mask to turn hash value -> bucket. The map's allocated size is mask + 1.*/
  uint32_t mask;
} upb_table;

UPB_INLINE size_t upb_table_size(const upb_table* t) { return t->mask + 1; }

// Internal-only functions, in .h file only out of necessity.

UPB_INLINE upb_key upb_key_empty(void) {
  upb_key ret;
  memset(&ret, 0, sizeof(upb_key));
  return ret;
}

UPB_INLINE bool upb_tabent_isempty(const upb_tabent* e) {
  upb_key key = e->key;
  UPB_ASSERT(sizeof(key.num) == sizeof(key.str));
  uintptr_t val;
  memcpy(&val, &key, sizeof(val));
  // Note: for upb_inttables a tab_key is a true integer key value, but the
  // inttable maintains the invariant that 0 value is always stored in the
  // compact table and never as a upb_tabent* so we can always use the 0
  // key value to identify an empty tabent.
  return val == 0;
}

uint32_t _upb_Hash(const void* p, size_t n, uint64_t seed);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_HASH_COMMON_H_ */
