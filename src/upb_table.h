/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file defines very fast int->struct (inttable) and string->struct
 * (strtable) hash tables.  The struct can be of any size, and it is stored
 * in the table itself, for cache-friendly performance.
 *
 * The table uses internal chaining with Brent's variation (inspired by the
 * Lua implementation of hash tables).  The hash function for strings is
 * Austin Appleby's "MurmurHash."
 */

#ifndef UPB_TABLE_H_
#define UPB_TABLE_H_

#include <assert.h>
#include "upb.h"
#include "upb_string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t upb_inttable_key_t;

#define UPB_END_OF_CHAIN (uint32_t)-1

typedef struct {
  bool has_entry:1;
  // The rest of the bits are the user's.
} upb_inttable_value;

typedef struct {
  upb_inttable_key_t key;
  uint32_t next;  // Internal chaining.
} upb_inttable_header;

typedef struct {
  upb_inttable_header hdr;
  upb_inttable_value val;
} upb_inttable_entry;

// TODO: consider storing the hash in the entry.  This would avoid the need to
// rehash on table resizes, but more importantly could possibly improve lookup
// performance by letting us compare hashes before comparing lengths or the
// strings themselves.
typedef struct {
  upb_string *key;         // We own a ref.
  uint32_t next;           // Internal chaining.
} upb_strtable_entry;

typedef struct {
  void *entries;        // Hash table.
  uint32_t count;       // Number of entries in the hash part.
  uint32_t mask;        // Mask to turn hash value -> bucket.
  uint16_t entry_size;  // Size of each entry.
  uint16_t value_size;  // Size of each value.
  uint8_t size_lg2;     // Size of the hash table part is 2^size_lg2 entries.
} upb_table;

typedef struct {
  upb_table t;
} upb_strtable;

typedef struct {
  upb_table t;
  void *array;           // Array part of the table.
  uint32_t array_size;   // Array part size.
  uint32_t array_count;  // Array part number of elements.
} upb_inttable;

// Initialize and free a table, respectively.  Specify the initial size
// with 'size' (the size will be increased as necessary).  Value size
// specifies how many bytes each value in the table is.
//
// WARNING!  The lowest bit of every entry is reserved by the hash table.
// It will always be overwritten when you insert, and must not be modified
// when looked up!
void upb_inttable_init(upb_inttable *table, uint32_t size, uint16_t value_size);
void upb_inttable_free(upb_inttable *table);
void upb_strtable_init(upb_strtable *table, uint32_t size, uint16_t entry_size);  // TODO: update
void upb_strtable_free(upb_strtable *table);

// Number of values in the hash table.
INLINE uint32_t upb_table_count(upb_table *t) { return t->count; }
INLINE uint32_t upb_inttable_count(upb_inttable *t) {
  return t->array_count + upb_table_count(&t->t);
}
INLINE uint32_t upb_strtable_count(upb_strtable *t) {
  return upb_table_count(&t->t);
}

// Inserts the given key into the hashtable with the given value.  The key must
// not already exist in the hash table.  The data will be copied from val into
// the hashtable (the amount of data copied comes from value_size when the
// table was constructed).  Therefore the data at val may be freed once the
// call returns.  For string tables, the table takes a ref on str.
//
// WARNING: the lowest bit of val is reserved and will be overwritten!
void upb_inttable_insert(upb_inttable *t, upb_inttable_key_t key, void *val);
void upb_strtable_insert(upb_strtable *t, upb_strtable_entry *ent);  // TODO: update
void upb_inttable_compact(upb_inttable *t);

INLINE uint32_t _upb_inttable_bucket(upb_inttable *t, upb_inttable_key_t k) {
  uint32_t bucket = k & t->t.mask;  // Identity hash for ints.
  assert(bucket != UPB_END_OF_CHAIN);
  return bucket;
}

// Returns true if this key belongs in the array part of the table.
INLINE bool _upb_inttable_isarrkey(upb_inttable *t, upb_inttable_key_t k) {
  return (k < t->array_size);
}

// Looks up key in this table, returning a pointer to the user's inserted data.
// We have the caller specify the entry_size because fixing this as a literal
// (instead of reading table->entry_size) gives the compiler more ability to
// optimize.
INLINE void *_upb_inttable_fastlookup(upb_inttable *t, uint32_t key,
                                      size_t entry_size, size_t value_size) {
  upb_inttable_value *arrval =
      (upb_inttable_value*)UPB_INDEX(t->array, key, value_size);
  if (_upb_inttable_isarrkey(t, key)) {
    //DEBUGPRINTF("array lookup for key %d, &val=%p, has_entry=%d\n", key, val, val->has_entry);
    return (arrval->has_entry) ? arrval : NULL;
  }
  uint32_t bucket = _upb_inttable_bucket(t, key);
  upb_inttable_entry *e =
      (upb_inttable_entry*)UPB_INDEX(t->t.entries, bucket, entry_size);
  //DEBUGPRINTF("looking in first bucket %d, entry size=%zd, addr=%p\n", bucket, entry_size, e);
  while (1) {
    //DEBUGPRINTF("%d, %d, %d\n", e->val.has_entry, e->hdr.key, key);
    if (e->hdr.key == key) {
      //DEBUGPRINTF("returning val from hash part\n");
      return &e->val;
    }
    if ((bucket = e->hdr.next) == UPB_END_OF_CHAIN) return NULL;
    //DEBUGPRINTF("looking in bucket %d\n", bucket);
    e = (upb_inttable_entry*)UPB_INDEX(t->t.entries, bucket, entry_size);
  }
}

INLINE size_t _upb_inttable_entrysize(size_t value_size) {
  return upb_align_up(sizeof(upb_inttable_header) + value_size, 8);
}

INLINE void *upb_inttable_fastlookup(upb_inttable *t, uint32_t key,
                                      uint32_t value_size) {
  return _upb_inttable_fastlookup(t, key, _upb_inttable_entrysize(value_size), value_size);
}

INLINE void *upb_inttable_lookup(upb_inttable *t, uint32_t key) {
  return _upb_inttable_fastlookup(t, key, t->t.entry_size, t->t.value_size);
}

void *upb_strtable_lookup(upb_strtable *t, upb_string *key);

// Provides iteration over the table.  The order in which the entries are
// returned is undefined.  Insertions invalidate iterators.
void *upb_strtable_begin(upb_strtable *t);
void *upb_strtable_next(upb_strtable *t, upb_strtable_entry *cur);

// Inttable iteration (should update strtable iteration to use this scheme too).
// The order is undefined.
//   for(upb_inttable_iter i = upb_inttable_begin(t); !upb_inttable_done(i);
//       i = upb_inttable_next(t, i)) {
//     // ...
//   }
typedef struct {
  upb_inttable_key_t key;
  upb_inttable_value *value;
  bool array_part;
} upb_inttable_iter;

upb_inttable_iter upb_inttable_begin(upb_inttable *t);
upb_inttable_iter upb_inttable_next(upb_inttable *t, upb_inttable_iter iter);
INLINE bool upb_inttable_done(upb_inttable_iter iter) { return iter.value == NULL; }
INLINE upb_inttable_key_t upb_inttable_iter_key(upb_inttable_iter iter) {
  return iter.key;
}
INLINE void *upb_inttable_iter_value(upb_inttable_iter iter) {
  return iter.value;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TABLE_H_ */
