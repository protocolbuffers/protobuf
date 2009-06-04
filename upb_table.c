/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_table.h"

#include <stdlib.h>

static int compare_entries(const void *f1, const void *f2)
{
  return ((struct upb_inttable_entry*)f1)->key -
         ((struct upb_inttable_entry*)f2)->key;
}

static uint32_t max(uint32_t a, uint32_t b) { return a > b ? a : b; }

static uint32_t round_up_to_pow2(uint32_t v)
{
  /* cf. Bit Twiddling Hacks:
   * http://www-graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
  v--;
  v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
  v++;
  return v;
}

void upb_inttable_init(struct upb_inttable *table, void *entries,
                       int num_entries, int entry_size)
{
  qsort(entries, num_entries, entry_size, compare_entries);

  /* Find the largest n for which at least half the keys <n are used.  We
   * make sure our table size is at least n.  This allows all keys <n to be
   * in their main position (as if it were an array) and only numbers >n might
   * possibly have collisions.  Start at 8 to avoid noise of small numbers. */
  upb_inttable_key_t n = 0, maybe_n;
  bool all_in_array = true;
  for(int i = 0; i < num_entries; i++) {
    struct upb_inttable_entry *e =
        upb_inttable_entry_get(entries, i, entry_size);
    maybe_n = e->key;
    if(maybe_n > 8 && maybe_n/(i+1) >= 2) {
      all_in_array = false;
      break;
    }
    n = maybe_n;
  }

  /* TODO: measure, tweak, optimize this choice of table size.  Possibly test
   * (at runtime) maximum chain length for each proposed size. */
  uint32_t min_size_by_load = all_in_array ? n : (double)num_entries / 0.85;
  uint32_t min_size = max(n, min_size_by_load);
  table->size = round_up_to_pow2(min_size);

  table->entries = malloc(table->size * entry_size);
  /* Initialize to empty. */
  for(size_t i = 0; i < table->size; i++) {
    struct upb_inttable_entry *e = upb_inttable_get(table, i, entry_size);
    e->key = UPB_END_OF_CHAIN;
    e->next = UPB_END_OF_CHAIN;
  }

  /* Insert the elements. */
  for(int i = 0; i < num_entries; i++) {
    struct upb_inttable_entry *e =
        upb_inttable_entry_get(entries, i, entry_size);
    int32_t hash = upb_inttable_hash(table, e->key);
    struct upb_inttable_entry *table_e =
        upb_inttable_get(table, hash, entry_size);
    if(table_e->key != UPB_END_OF_CHAIN) {  /* Collision. */
      if(hash == upb_inttable_hash(table, table_e->key)) {
        /* Existing element is in its main posisiton.  Find an empty slot to
         * place our new element. */
      } else {
        /* Existing element is not in its main position.  Move it to an empty
         * slot and put our element in its main position. */
      }
    }
  }
}

void upb_inttable_free(struct upb_inttable *table)
{
  free(table->entries);
}

/* Emit definition for inline functions. */
