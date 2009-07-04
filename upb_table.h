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
#define UPB_EMPTY_ENTRY (uint32_t)0
#define UPB_INDEX(base, i, m) (void*)((char*)(base) + ((i)*(m)))

struct upb_inttable_entry {
  upb_inttable_key_t key;
  uint32_t next;  /* Internal chaining. */
};

/* TODO: consider storing the hash in the entry.  This would avoid the need to
 * rehash on table resizes, but more importantly could possibly improve lookup
 * performance by letting us compare hashes before comparing lengths or the
 * strings themselves. */
struct upb_strtable_entry {
  struct upb_string key;
  uint32_t next;  /* Internal chaining. */
};

struct upb_table {
  void *entries;
  uint32_t count;       /* How many elements are currently in the table? */
  uint16_t entry_size;  /* How big is each entry? */
  uint8_t size_lg2;     /* The table is 2^size_lg2 in size. */
};

struct upb_strtable {
  struct upb_table t;
};

struct upb_inttable {
  struct upb_table t;
};

/* Initialize and free a table, respectively.  Specify the initial size
 * with 'size' (the size will be increased as necessary).  Entry size
 * specifies how many bytes each entry in the table is. */
void upb_inttable_init(struct upb_inttable *table,
                       uint32_t size, uint16_t entry_size);
void upb_inttable_free(struct upb_inttable *table);
void upb_strtable_init(struct upb_strtable *table,
                       uint32_t size, uint16_t entry_size);
void upb_strtable_free(struct upb_strtable *table);

INLINE uint32_t upb_table_size(struct upb_table *t) { return 1 << t->size_lg2; }
INLINE uint32_t upb_inttable_size(struct upb_inttable *t) {
  return upb_table_size(&t->t);
}
INLINE uint32_t upb_strtable_size(struct upb_strtable *t) {
  return upb_table_size(&t->t);
}

/* Inserts the given key into the hashtable with the given value.  The key must
 * not already exist in the hash table.  The data will be copied from e into
 * the hashtable (the amount of data copied comes from entry_size when the
 * table was constructed).  Therefore the data at val may be freed once the
 * call returns. */
void upb_inttable_insert(struct upb_inttable *t, struct upb_inttable_entry *e);
void upb_strtable_insert(struct upb_strtable *t, struct upb_strtable_entry *e);

INLINE uint32_t upb_inttable_bucket(struct upb_inttable *t, upb_inttable_key_t k) {
  return (k & (upb_inttable_size(t)-1)) + 1;  /* Identity hash for ints. */
}

/* Looks up key in this table.  Inlined because this is in the critical path
 * of parsing.  We have the caller specify the entry_size because fixing
 * this as a literal (instead of reading table->entry_size) gives the
 * compiler more ability to optimize. */
INLINE void *upb_inttable_lookup(struct upb_inttable *t,
                                 uint32_t key, uint32_t entry_size) {
  assert(key != 0);
  uint32_t bucket = upb_inttable_bucket(t, key);
  struct upb_inttable_entry *e;
  do {
    e = (struct upb_inttable_entry*)UPB_INDEX(t->t.entries, bucket-1, entry_size);
    if(e->key == key) return e;
  } while((bucket = e->next) != UPB_END_OF_CHAIN);
  return NULL;  /* Not found. */
}

void *upb_strtable_lookup(struct upb_strtable *t, struct upb_string *key);

/* Provides iteration over the table.  The order in which the entries are
 * returned is undefined.  Insertions invalidate iterators.  The _next
 * functions return NULL when the end has been reached. */
void *upb_inttable_begin(struct upb_inttable *t);
void *upb_inttable_next(struct upb_inttable *t, struct upb_inttable_entry *cur);

void *upb_strtable_begin(struct upb_strtable *t);
void *upb_strtable_next(struct upb_strtable *t, struct upb_strtable_entry *cur);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TABLE_H_ */
