/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
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
 * This header is internal to upb; its interface should not be considered
 * public or stable.
 *
 * The table must be homogenous (all values of the same type).  We currently
 * enforce this on insert but store the full upb_value (with type) anyway.
 * This is required with the current interface because lookups vend a pointer
 * to the table's internal storage.
 */

#ifndef UPB_TABLE_H_
#define UPB_TABLE_H_

#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
  uintptr_t num;
  const char *str;  // We own, nullz.
} upb_tabkey;

#define UPB_TABKEY_NUM(n) {n}
#ifdef UPB_C99
#define UPB_TABKEY_STR(s) {.str = s}
#endif
// TODO(haberman): C++
#define UPB_TABKEY_NONE {0}

typedef struct _upb_tabent {
  upb_tabkey key;
  // Storing a upb_value here wastes a bit of memory in debug mode because
  // we are storing the type for each value even though we enforce that all
  // values are the same.  But since this only affects debug mode, we don't
  // worry too much about it.  The same applies to upb_inttable.array below.
  upb_value val;
  // Internal chaining.  This is const so we can create static initializers for
  // tables.  We cast away const sometimes, but *only* when the containing
  // upb_table is known to be non-const.  This requires a bit of care, but
  // the subtlety is confined to table.c.
  const struct _upb_tabent *next;
} upb_tabent;

typedef struct {
  size_t count;          // Number of entries in the hash part.
  size_t mask;           // Mask to turn hash value -> bucket.
  upb_ctype_t type;      // Type of all values.
  uint8_t size_lg2;      // Size of the hash table part is 2^size_lg2 entries.
  const upb_tabent *entries;   // Hash table.
} upb_table;

typedef struct {
  upb_table t;
} upb_strtable;

#define UPB_STRTABLE_INIT(count, mask, type, size_lg2, entries) \
  {{count, mask, type, size_lg2, entries}}

typedef struct {
  upb_table t;             // For entries that don't fit in the array part.
  const upb_value *array;  // Array part of the table.
  size_t array_size;       // Array part size.
  size_t array_count;      // Array part number of elements.
} upb_inttable;

#define UPB_INTTABLE_INIT(count, mask, type, size_lg2, ent, a, asize, acount) \
  {{count, mask, type, size_lg2, ent}, a, asize, acount}

#define UPB_EMPTY_INTTABLE_INIT(type) \
  UPB_INTTABLE_INIT(0, 0, type, 0, NULL, NULL, 0, 0)

#define UPB_ARRAY_EMPTYENT UPB_VALUE_INIT_INT64(-1)

INLINE size_t upb_table_size(const upb_table *t) {
  if (t->size_lg2 == 0)
    return 0;
  else
    return 1 << t->size_lg2;
}

// Internal-only functions, in .h file only out of necessity.
INLINE bool upb_tabent_isempty(const upb_tabent *e) { return e->key.num == 0; }
INLINE upb_tabkey upb_intkey(uintptr_t key) { upb_tabkey k = {key}; return k; }
INLINE const upb_tabent *upb_inthash(const upb_table *t, upb_tabkey key) {
  return t->entries + ((uint32_t)key.num & t->mask);
}
INLINE bool upb_arrhas(upb_value v) { return v.val.uint64 != (uint64_t)-1; }
uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed);

// Initialize and uninitialize a table, respectively.  If memory allocation
// failed, false is returned that the table is uninitialized.
bool upb_inttable_init(upb_inttable *table, upb_ctype_t type);
bool upb_strtable_init(upb_strtable *table, upb_ctype_t type);
void upb_inttable_uninit(upb_inttable *table);
void upb_strtable_uninit(upb_strtable *table);

// Returns the number of values in the table.
size_t upb_inttable_count(const upb_inttable *t);
INLINE size_t upb_strtable_count(const upb_strtable *t) { return t->t.count; }

// Inserts the given key into the hashtable with the given value.  The key must
// not already exist in the hash table.  For string tables, the key must be
// NULL-terminated, and the table will make an internal copy of the key.
// Inttables must not insert a value of UINTPTR_MAX.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged.
bool upb_inttable_insert(upb_inttable *t, uintptr_t key, upb_value val);
bool upb_strtable_insert(upb_strtable *t, const char *key, upb_value val);

// Looks up key in this table, returning a pointer to the table's internal copy
// of the user's inserted data, or NULL if this key is not in the table.  The
// returned pointer is invalidated by inserts.
const upb_value *upb_inttable_lookup(const upb_inttable *t, uintptr_t key);
const upb_value *upb_strtable_lookup(const upb_strtable *t, const char *key);

// Removes an item from the table.  Returns true if the remove was successful,
// and stores the removed item in *val if non-NULL.
bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val);
bool upb_strtable_remove(upb_strtable *t, const char *key, upb_value *val);

// Handy routines for treating an inttable like a stack.  May not be mixed with
// other insert/remove calls.
bool upb_inttable_push(upb_inttable *t, upb_value val);
upb_value upb_inttable_pop(upb_inttable *t);

// Convenience routines for inttables with pointer keys.
bool upb_inttable_insertptr(upb_inttable *t, const void *key, upb_value val);
bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val);
const upb_value *upb_inttable_lookupptr(const upb_inttable *t, const void *key);

// Optimizes the table for the current set of entries, for both memory use and
// lookup time.  Client should call this after all entries have been inserted;
// inserting more entries is legal, but will likely require a table resize.
void upb_inttable_compact(upb_inttable *t);

// A special-case inlinable version of the lookup routine for 32-bit integers.
INLINE const upb_value *upb_inttable_lookup32(const upb_inttable *t,
                                              uint32_t key) {
  if (key < t->array_size) {
    const upb_value *v = &t->array[key];
    return upb_arrhas(*v) ? v : NULL;
  }
  const upb_tabent *e;
  if (t->t.entries == NULL) return NULL;
  for (e = upb_inthash(&t->t, upb_intkey(key)); true; e = e->next) {
    if ((uint32_t)e->key.num == key) return &e->val;
    if (e->next == NULL) return NULL;
  }
}


/* upb_strtable_iter **********************************************************/

// Strtable iteration.  Order is undefined.  Insertions invalidate iterators.
//   upb_strtable_iter i;
//   upb_strtable_begin(&i, t);
//   for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
//     const char *key = upb_strtable_iter_key(&i);
//     const upb_value val = upb_strtable_iter_value(&i);
//     // ...
//   }
typedef struct {
  const upb_strtable *t;
  const upb_tabent *e;
} upb_strtable_iter;

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t);
void upb_strtable_next(upb_strtable_iter *i);
INLINE bool upb_strtable_done(upb_strtable_iter *i) { return i->e == NULL; }
INLINE const char *upb_strtable_iter_key(upb_strtable_iter *i) {
  return i->e->key.str;
}
INLINE upb_value upb_strtable_iter_value(upb_strtable_iter *i) {
  return i->e->val;
}


/* upb_inttable_iter **********************************************************/

// Inttable iteration.  Order is undefined.  Insertions invalidate iterators.
//   upb_inttable_iter i;
//   upb_inttable_begin(&i, t);
//   for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
//     uintptr_t key = upb_inttable_iter_key(&i);
//     upb_value val = upb_inttable_iter_value(&i);
//     // ...
//   }
typedef struct {
  const upb_inttable *t;
  union {
    const upb_tabent *ent;  // For hash iteration.
    const upb_value *val;   // For array iteration.
  } ptr;
  uintptr_t arrkey;
  bool array_part;
} upb_inttable_iter;

void upb_inttable_begin(upb_inttable_iter *i, const upb_inttable *t);
void upb_inttable_next(upb_inttable_iter *i);
INLINE bool upb_inttable_done(upb_inttable_iter *i) {
  return i->ptr.ent == NULL;
}
INLINE uintptr_t upb_inttable_iter_key(upb_inttable_iter *i) {
  return i->array_part ? i->arrkey : i->ptr.ent->key.num;
}
INLINE upb_value upb_inttable_iter_value(upb_inttable_iter *i) {
  return i->array_part ? *i->ptr.val : i->ptr.ent->val;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TABLE_H_ */
