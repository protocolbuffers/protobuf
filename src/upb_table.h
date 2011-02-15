/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
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

#define UPB_END_OF_CHAIN (uint32_t)0

typedef struct {
  upb_inttable_key_t key;
  bool has_entry:1;
  uint32_t next:31;  // Internal chaining.
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
  void *entries;
  uint32_t count;       /* How many elements are currently in the table? */
  uint16_t entry_size;  /* How big is each entry? */
  uint8_t size_lg2;     /* The table is 2^size_lg2 in size. */
  uint32_t mask;
} upb_table;

typedef struct {
  upb_table t;
} upb_strtable;

typedef struct {
  upb_table t;
} upb_inttable;

/* Initialize and free a table, respectively.  Specify the initial size
 * with 'size' (the size will be increased as necessary).  Entry size
 * specifies how many bytes each entry in the table is. */
void upb_inttable_init(upb_inttable *table, uint32_t size, uint16_t entry_size);
void upb_inttable_free(upb_inttable *table);
void upb_strtable_init(upb_strtable *table, uint32_t size, uint16_t entry_size);
void upb_strtable_free(upb_strtable *table);

INLINE uint32_t upb_table_size(upb_table *t) { return 1 << t->size_lg2; }
INLINE uint32_t upb_inttable_size(upb_inttable *t) {
  return upb_table_size(&t->t);
}
INLINE uint32_t upb_strtable_size(upb_strtable *t) {
  return upb_table_size(&t->t);
}

INLINE uint32_t upb_table_count(upb_table *t) { return t->count; }
INLINE uint32_t upb_inttable_count(upb_inttable *t) {
  return upb_table_count(&t->t);
}
INLINE uint32_t upb_strtable_count(upb_strtable *t) {
  return upb_table_count(&t->t);
}

/* Inserts the given key into the hashtable with the given value.  The key must
 * not already exist in the hash table.  The data will be copied from e into
 * the hashtable (the amount of data copied comes from entry_size when the
 * table was constructed).  Therefore the data at val may be freed once the
 * call returns. */
void upb_inttable_insert(upb_inttable *t, upb_inttable_entry *e);
void upb_strtable_insert(upb_strtable *t, upb_strtable_entry *e);

INLINE uint32_t upb_inttable_bucket(upb_inttable *t, upb_inttable_key_t k) {
  return (k & t->t.mask) + 1;  /* Identity hash for ints. */
}

/* Looks up key in this table.  Inlined because this is in the critical path of
 * decoding.  We have the caller specify the entry_size because fixing this as
 * a literal (instead of reading table->entry_size) gives the compiler more
 * ability to optimize. */
INLINE void *upb_inttable_fastlookup(upb_inttable *t, uint32_t key,
                                     uint32_t entry_size) {
  uint32_t bucket = upb_inttable_bucket(t, key);
  upb_inttable_entry *e =
      (upb_inttable_entry*)UPB_INDEX(t->t.entries, bucket-1, entry_size);
  if (key == 0) {
    while (1) {
      if (e->key == 0 && e->has_entry) return e;
      if ((bucket = e->next) == UPB_END_OF_CHAIN) return NULL;
      e = (upb_inttable_entry*)UPB_INDEX(t->t.entries, bucket-1, entry_size);
    }
  } else {
    while (1) {
      if (e->key == key) return e;
      if ((bucket = e->next) == UPB_END_OF_CHAIN) return NULL;
      e = (upb_inttable_entry*)UPB_INDEX(t->t.entries, bucket-1, entry_size);
    }
  }
}

INLINE void *upb_inttable_lookup(upb_inttable *t, uint32_t key) {
  return upb_inttable_fastlookup(t, key, t->t.entry_size);
}

void *upb_strtable_lookup(upb_strtable *t, upb_string *key);

/* Provides iteration over the table.  The order in which the entries are
 * returned is undefined.  Insertions invalidate iterators.  The _next
 * functions return NULL when the end has been reached. */
void *upb_inttable_begin(upb_inttable *t);
void *upb_inttable_next(upb_inttable *t, upb_inttable_entry *cur);

void *upb_strtable_begin(upb_strtable *t);
void *upb_strtable_next(upb_strtable *t, upb_strtable_entry *cur);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TABLE_H_ */
