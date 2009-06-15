/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_TABLE_H_
#define UPB_TABLE_H_

#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Inttables are keyed by 32-bit integer. */
typedef int32_t upb_inttable_key_t;

#define UPB_EMPTY_ENTRY (upb_inttable_key_t)-1

struct upb_inttable_entry {
  upb_inttable_key_t key;
  struct upb_inttable_entry *next;  /* Internal chaining. */
};

struct upb_inttable {
  uint32_t size;  /* Is a power of two. */
  uint32_t entry_size;  /* How big is each entry? */
  void *entries;
};

/* Builds an int32_t -> <entry> table, optimized for very fast lookup by
 * number.  table is a pointer to a previously allocated upb_inttable.
 * entries points to an array of the desired entries themselves, each of size
 * entry_size.  The table is allocated in dynamic memory, and does not reference
 * the data in entries.  Entries may be modified by the function.
 *
 * The table must be freed with upb_inttable_free. */
void upb_inttable_init(struct upb_inttable *table, void *entries,
                       int num_entries, int entry_size);

/* Frees any data that was allocated by upb_inttable_init. */
void upb_inttable_free(struct upb_inttable *table);

inline struct upb_inttable_entry *upb_inttable_entry_get(
    void *entries, int32_t pos, int entry_size) {
  return (struct upb_inttable_entry*)(((char*)entries) + pos*entry_size);
}

inline struct upb_inttable_entry *upb_inttable_mainpos2(
    struct upb_inttable *table, upb_inttable_key_t key, int32_t entry_size) {
  /* Identity hash for integers. */
  int32_t pos = key & (table->size-1);
  return upb_inttable_entry_get(table->entries, pos, entry_size);
}

inline struct upb_inttable_entry *upb_inttable_mainpos(
    struct upb_inttable *table, upb_inttable_key_t key) {
  return upb_inttable_mainpos2(table, key, table->entry_size);
}

/* Lookups up key in this table.  Inlined because this is in the critical path
 * of parsing. */
inline void *upb_inttable_lookup(struct upb_inttable *table,
                                 int32_t key,
                                 int32_t entry_size) {
  /* TODO: experiment with Cuckoo Hashing, which can perform lookups without
   * any branches, cf:
   * http://www.cs.cmu.edu/~damon2006/pdf/zukowski06archconscioushashing.pdf.
   * The tradeoff is having to calculate a second hash for every lookup, which
   * will hurt the simple array case. */
  struct upb_inttable_entry *e = upb_inttable_mainpos2(table, key, entry_size);
  do {
    if (key == e->key) return e;
    e = e->next;
  } while (e);
  return NULL;  /* Not found. */
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TABLE_H_ */
