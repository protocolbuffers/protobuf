/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
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
 * The table must be homogenous (all values of the same type).  In debug
 * mode, we check this on insert and lookup.
 */

#ifndef UPB_TABLE_H_
#define UPB_TABLE_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "upb/upb.h"

#ifdef __cplusplus
extern "C" {
#endif


/* upb_value ******************************************************************/

// A tagged union (stored untagged inside the table) so that we can check that
// clients calling table accessors are correctly typed without having to have
// an explosion of accessors.
typedef enum {
  UPB_CTYPE_INT32    = 1,
  UPB_CTYPE_INT64    = 2,
  UPB_CTYPE_UINT32   = 3,
  UPB_CTYPE_UINT64   = 4,
  UPB_CTYPE_BOOL     = 5,
  UPB_CTYPE_CSTR     = 6,
  UPB_CTYPE_PTR      = 7,
  UPB_CTYPE_CONSTPTR = 8,
  UPB_CTYPE_FPTR     = 9,
} upb_ctype_t;

typedef union {
  int32_t  int32;
  int64_t  int64;
  uint64_t uint64;
  uint32_t uint32;
  bool     _bool;
  char     *cstr;
  void     *ptr;
  const void *constptr;
  upb_func *fptr;
} _upb_value;

typedef struct {
  _upb_value val;
#ifndef NDEBUG
  // In debug mode we carry the value type around also so we can check accesses
  // to be sure the right member is being read.
  upb_ctype_t ctype;
#endif
} upb_value;

#ifdef UPB_C99
#define UPB_VALUE_INIT(v, member) {.member = v}
#endif
#define UPB__VALUE_INIT_NONE      UPB_VALUE_INIT(NULL, ptr)

#ifdef NDEBUG
#define SET_TYPE(dest, val)      UPB_UNUSED(val)
#define UPB_VALUE_INIT_NONE      {UPB__VALUE_INIT_NONE}
#else
#define SET_TYPE(dest, val) dest = val
// Non-existent type, all reads will fail.
#define UPB_VALUE_INIT_NONE      {UPB__VALUE_INIT_NONE, -1}
#endif

#define UPB_VALUE_INIT_INT32(v)  UPB_VALUE_INIT(v, int32)
#define UPB_VALUE_INIT_INT64(v)  UPB_VALUE_INIT(v, int64)
#define UPB_VALUE_INIT_UINT32(v) UPB_VALUE_INIT(v, uint32)
#define UPB_VALUE_INIT_UINT64(v) UPB_VALUE_INIT(v, uint64)
#define UPB_VALUE_INIT_BOOL(v)   UPB_VALUE_INIT(v, _bool)
#define UPB_VALUE_INIT_CSTR(v)   UPB_VALUE_INIT(v, cstr)
#define UPB_VALUE_INIT_PTR(v)    UPB_VALUE_INIT(v, ptr)
#define UPB_VALUE_INIT_CONSTPTR(v) UPB_VALUE_INIT(v, constptr)
#define UPB_VALUE_INIT_FPTR(v)   UPB_VALUE_INIT(v, fptr)

// Like strdup(), which isn't always available since it's not ANSI C.
char *upb_strdup(const char *s);
// Variant that works with a length-delimited rather than NULL-delimited string,
// as supported by strtable.
char *upb_strdup2(const char *s, size_t len);

UPB_INLINE void _upb_value_setval(upb_value *v, _upb_value val,
                                  upb_ctype_t ctype) {
  v->val = val;
  SET_TYPE(v->ctype, ctype);
}

UPB_INLINE upb_value _upb_value_val(_upb_value val, upb_ctype_t ctype) {
  upb_value ret;
  _upb_value_setval(&ret, val, ctype);
  return ret;
}

// For each value ctype, define the following set of functions:
//
// // Get/set an int32 from a upb_value.
// int32_t upb_value_getint32(upb_value val);
// void upb_value_setint32(upb_value *val, int32_t cval);
//
// // Construct a new upb_value from an int32.
// upb_value upb_value_int32(int32_t val);
#define FUNCS(name, membername, type_t, proto_type) \
  UPB_INLINE void upb_value_set ## name(upb_value *val, type_t cval) { \
    val->val.uint64 = 0; \
    SET_TYPE(val->ctype, proto_type); \
    val->val.membername = cval; \
  } \
  UPB_INLINE upb_value upb_value_ ## name(type_t val) { \
    upb_value ret; \
    upb_value_set ## name(&ret, val); \
    return ret; \
  } \
  UPB_INLINE type_t upb_value_get ## name(upb_value val) { \
    assert(val.ctype == proto_type); \
    return val.val.membername; \
  }

FUNCS(int32,    int32,        int32_t,      UPB_CTYPE_INT32);
FUNCS(int64,    int64,        int64_t,      UPB_CTYPE_INT64);
FUNCS(uint32,   uint32,       uint32_t,     UPB_CTYPE_UINT32);
FUNCS(uint64,   uint64,       uint64_t,     UPB_CTYPE_UINT64);
FUNCS(bool,     _bool,        bool,         UPB_CTYPE_BOOL);
FUNCS(cstr,     cstr,         char*,        UPB_CTYPE_CSTR);
FUNCS(ptr,      ptr,          void*,        UPB_CTYPE_PTR);
FUNCS(constptr, constptr,     const void*,  UPB_CTYPE_CONSTPTR);
FUNCS(fptr,     fptr,         upb_func*,    UPB_CTYPE_FPTR);

#undef FUNCS


/* upb_table ******************************************************************/

typedef union {
  uintptr_t num;
  struct {
    // We own this. NULL-terminated but may also contain binary data; see
    // explicit length below.
    // TODO: move the length to the start of the string in order to reduce
    // tabkey's size (to one machine word) in a way that supports static
    // initialization.
    const char *str;
    size_t length;
  } s;
} upb_tabkey;

#define UPB_TABKEY_NUM(n) {n}
#ifdef UPB_C99
// Given that |s| is a string literal, sizeof(s) gives us a
// compile-time-constant strlen(). We must ensure that this works for static
// data initializers.
#define UPB_TABKEY_STR(strval) { .s = { .str = strval,                    \
                                        .length = sizeof(strval) - 1 } }
#endif
// TODO(haberman): C++
#define UPB_TABKEY_NONE {0}

typedef struct _upb_tabent {
  upb_tabkey key;
  _upb_value val;
  // Internal chaining.  This is const so we can create static initializers for
  // tables.  We cast away const sometimes, but *only* when the containing
  // upb_table is known to be non-const.  This requires a bit of care, but
  // the subtlety is confined to table.c.
  const struct _upb_tabent *next;
} upb_tabent;

typedef struct {
  size_t count;          // Number of entries in the hash part.
  size_t mask;           // Mask to turn hash value -> bucket.
  upb_ctype_t ctype;     // Type of all values.
  uint8_t size_lg2;      // Size of the hash table part is 2^size_lg2 entries.

  // Hash table entries.
  // Making this const isn't entirely accurate; what we really want is for it to
  // have the same const-ness as the table it's inside.  But there's no way to
  // declare that in C.  So we have to make it const so that we can statically
  // initialize const hash tables.  Then we cast away const when we have to.
  const upb_tabent *entries;
} upb_table;

typedef struct {
  upb_table t;
} upb_strtable;

#define UPB_STRTABLE_INIT(count, mask, ctype, size_lg2, entries) \
  {{count, mask, ctype, size_lg2, entries}}

#define UPB_EMPTY_STRTABLE_INIT(ctype)                           \
  UPB_STRTABLE_INIT(0, 0, ctype, 0, NULL)

typedef struct {
  upb_table t;              // For entries that don't fit in the array part.
  const _upb_value *array;  // Array part of the table.  See const note above.
  size_t array_size;        // Array part size.
  size_t array_count;       // Array part number of elements.
} upb_inttable;

#define UPB_INTTABLE_INIT(count, mask, ctype, size_lg2, ent, a, asize, acount) \
  {{count, mask, ctype, size_lg2, ent}, a, asize, acount}

#define UPB_EMPTY_INTTABLE_INIT(ctype) \
  UPB_INTTABLE_INIT(0, 0, ctype, 0, NULL, NULL, 0, 0)

#define UPB_ARRAY_EMPTYVAL -1
#define UPB_ARRAY_EMPTYENT UPB_VALUE_INIT_INT64(UPB_ARRAY_EMPTYVAL)

UPB_INLINE size_t upb_table_size(const upb_table *t) {
  if (t->size_lg2 == 0)
    return 0;
  else
    return 1 << t->size_lg2;
}

// Internal-only functions, in .h file only out of necessity.
UPB_INLINE bool upb_tabent_isempty(const upb_tabent *e) {
  return e->key.num == 0;
}

// Used by some of the unit tests for generic hashing functionality.
uint32_t MurmurHash2(const void * key, size_t len, uint32_t seed);

UPB_INLINE upb_tabkey upb_intkey(uintptr_t key) {
  upb_tabkey k;
  k.num = key;
  return k;
}

UPB_INLINE uint32_t upb_inthash(uintptr_t key) {
  return (uint32_t)key;
}

static const upb_tabent *upb_getentry(const upb_table *t, uint32_t hash) {
  return t->entries + (hash & t->mask);
}

UPB_INLINE bool upb_arrhas(_upb_value v) {
  return v.uint64 != (uint64_t)UPB_ARRAY_EMPTYVAL;
}

// Initialize and uninitialize a table, respectively.  If memory allocation
// failed, false is returned that the table is uninitialized.
bool upb_inttable_init(upb_inttable *table, upb_ctype_t ctype);
bool upb_strtable_init(upb_strtable *table, upb_ctype_t ctype);
void upb_inttable_uninit(upb_inttable *table);
void upb_strtable_uninit(upb_strtable *table);

// Returns the number of values in the table.
size_t upb_inttable_count(const upb_inttable *t);
UPB_INLINE size_t upb_strtable_count(const upb_strtable *t) {
  return t->t.count;
}

// Inserts the given key into the hashtable with the given value.  The key must
// not already exist in the hash table.  For string tables, the key must be
// NULL-terminated, and the table will make an internal copy of the key.
// Inttables must not insert a value of UINTPTR_MAX.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged.
bool upb_inttable_insert(upb_inttable *t, uintptr_t key, upb_value val);
bool upb_strtable_insert2(upb_strtable *t, const char *key, size_t len,
                          upb_value val);

// For NULL-terminated strings.
UPB_INLINE bool upb_strtable_insert(upb_strtable *t, const char *key,
                                    upb_value val) {
  return upb_strtable_insert2(t, key, strlen(key), val);
}

// Looks up key in this table, returning "true" if the key was found.
// If v is non-NULL, copies the value for this key into *v.
bool upb_inttable_lookup(const upb_inttable *t, uintptr_t key, upb_value *v);
bool upb_strtable_lookup2(const upb_strtable *t, const char *key, size_t len,
                          upb_value *v);

// For NULL-terminated strings.
UPB_INLINE bool upb_strtable_lookup(const upb_strtable *t, const char *key,
                                    upb_value *v) {
  return upb_strtable_lookup2(t, key, strlen(key), v);
}

// Removes an item from the table.  Returns true if the remove was successful,
// and stores the removed item in *val if non-NULL.
bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val);
bool upb_strtable_remove2(upb_strtable *t, const char *key, size_t len,
                          upb_value *val);

// For NULL-terminated strings.
UPB_INLINE bool upb_strtable_remove(upb_strtable *t, const char *key,
                                    upb_value *v) {
  return upb_strtable_remove2(t, key, strlen(key), v);
}

// Updates an existing entry in an inttable.  If the entry does not exist,
// returns false and does nothing.  Unlike insert/remove, this does not
// invalidate iterators.
bool upb_inttable_replace(upb_inttable *t, uintptr_t key, upb_value val);

// Handy routines for treating an inttable like a stack.  May not be mixed with
// other insert/remove calls.
bool upb_inttable_push(upb_inttable *t, upb_value val);
upb_value upb_inttable_pop(upb_inttable *t);

// Convenience routines for inttables with pointer keys.
bool upb_inttable_insertptr(upb_inttable *t, const void *key, upb_value val);
bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val);
bool upb_inttable_lookupptr(
    const upb_inttable *t, const void *key, upb_value *val);

// Optimizes the table for the current set of entries, for both memory use and
// lookup time.  Client should call this after all entries have been inserted;
// inserting more entries is legal, but will likely require a table resize.
void upb_inttable_compact(upb_inttable *t);

// A special-case inlinable version of the lookup routine for 32-bit integers.
UPB_INLINE bool upb_inttable_lookup32(const upb_inttable *t, uint32_t key,
                                      upb_value *v) {
  *v = upb_value_int32(0);  // Silence compiler warnings.
  if (key < t->array_size) {
    _upb_value arrval = t->array[key];
    if (upb_arrhas(arrval)) {
      _upb_value_setval(v, arrval, t->t.ctype);
      return true;
    } else {
      return false;
    }
  } else {
    const upb_tabent *e;
    if (t->t.entries == NULL) return false;
    for (e = upb_getentry(&t->t, upb_inthash(key)); true; e = e->next) {
      if ((uint32_t)e->key.num == key) {
        _upb_value_setval(v, e->val, t->t.ctype);
        return true;
      }
      if (e->next == NULL) return false;
    }
  }
}

// Exposed for testing only.
bool upb_strtable_resize(upb_strtable *t, size_t size_lg2);

/* Iterators ******************************************************************/

// Iterators for int and string tables.  We are subject to some kind of unusual
// design constraints:
//
// For high-level languages:
//  - we must be able to guarantee that we don't crash or corrupt memory even if
//    the program accesses an invalidated iterator.
//
// For C++11 range-based for:
//  - iterators must be copyable
//  - iterators must be comparable
//  - it must be possible to construct an "end" value.
//
// Iteration order is undefined.
//
// Modifying the table invalidates iterators.  upb_{str,int}table_done() is
// guaranteed to work even on an invalidated iterator, as long as the table it
// is iterating over has not been freed.  Calling next() or accessing data from
// an invalidated iterator yields unspecified elements from the table, but it is
// guaranteed not to crash and to return real table elements (except when done()
// is true).


/* upb_strtable_iter **********************************************************/

//   upb_strtable_iter i;
//   upb_strtable_begin(&i, t);
//   for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
//     const char *key = upb_strtable_iter_key(&i);
//     const upb_value val = upb_strtable_iter_value(&i);
//     // ...
//   }

typedef struct {
  const upb_strtable *t;
  size_t index;
} upb_strtable_iter;

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t);
void upb_strtable_next(upb_strtable_iter *i);
bool upb_strtable_done(const upb_strtable_iter *i);
const char *upb_strtable_iter_key(upb_strtable_iter *i);
size_t upb_strtable_iter_keylength(upb_strtable_iter *i);
upb_value upb_strtable_iter_value(const upb_strtable_iter *i);
void upb_strtable_iter_setdone(upb_strtable_iter *i);
bool upb_strtable_iter_isequal(const upb_strtable_iter *i1,
                               const upb_strtable_iter *i2);


/* upb_inttable_iter **********************************************************/

//   upb_inttable_iter i;
//   upb_inttable_begin(&i, t);
//   for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
//     uintptr_t key = upb_inttable_iter_key(&i);
//     upb_value val = upb_inttable_iter_value(&i);
//     // ...
//   }

typedef struct {
  const upb_inttable *t;
  size_t index;
  bool array_part;
} upb_inttable_iter;

void upb_inttable_begin(upb_inttable_iter *i, const upb_inttable *t);
void upb_inttable_next(upb_inttable_iter *i);
bool upb_inttable_done(const upb_inttable_iter *i);
uintptr_t upb_inttable_iter_key(const upb_inttable_iter *i);
upb_value upb_inttable_iter_value(const upb_inttable_iter *i);
void upb_inttable_iter_setdone(upb_inttable_iter *i);
bool upb_inttable_iter_isequal(const upb_inttable_iter *i1,
                               const upb_inttable_iter *i2);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TABLE_H_ */
