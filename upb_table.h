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

typedef int32_t upb_inttable_key_t;

#define UPB_END_OF_CHAIN (upb_inttable_key_t)-1

struct upb_inttable_entry {
  upb_inttable_key_t key;
  int32_t next;
};

struct upb_inttable {
  uint32_t size;  /* Is a power of two. */
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

inline int32_t upb_inttable_hash(struct upb_inttable * table,
                                 upb_inttable_key_t key) {
  return key & (table->size-1);
}

/* Frees any data that was allocated by upb_inttable_init. */
void upb_inttable_free(struct upb_inttable *table);

inline struct upb_inttable_entry *upb_inttable_entry_get(
    void *entries, int32_t pos, int entry_size) {
  return (struct upb_inttable_entry*)((char*)entries) + pos*entry_size;
}

inline struct upb_inttable_entry *upb_inttable_get(
    struct upb_inttable *table, int32_t pos, int entry_size) {
  return upb_inttable_entry_get(table->entries, pos, entry_size);
}

/* Lookups up key in this table.  Inlined because this is in the critical path
 * of parsing. */
inline void *upb_inttable_lookup(struct upb_inttable *table, int32_t key,
                                 int entry_size) {
  int32_t pos = upb_inttable_hash(table, key);
  do {
    struct upb_inttable_entry *e = upb_inttable_get(table, pos, entry_size);
    if (key == e->key) return e;
    pos = e->next;
  } while (pos != UPB_END_OF_CHAIN);
  return NULL;  /* Not found. */
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TABLE_H_ */
