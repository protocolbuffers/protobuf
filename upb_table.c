/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef UPB_UNALIGNED_READS_OK
//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby
// Reformatted and C99-ified by Joshua Haberman.
// Note - This code makes a few assumptions about how your machine behaves -
//   1. We can read a 4-byte value from any address without crashing
//   2. sizeof(int) == 4 (in upb this limitation is removed by using uint32_t
// And it has a few limitations -
//   1. It will not work incrementally.
//   2. It will not produce the same results on little-endian and big-endian
//      machines.
static uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed)
{
  // 'm' and 'r' are mixing constants generated offline.
  // They're not really 'magic', they just happen to work well.
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;

  // Initialize the hash to a 'random' value
  uint32_t h = seed ^ len;

  // Mix 4 bytes at a time into the hash
  const uint8_t * data = (const uint8_t *)key;
  while(len >= 4) {
    uint32_t k = *(uint32_t *)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  // Handle the last few bytes of the input array
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

#else // !UPB_UNALIGNED_READS_OK

//-----------------------------------------------------------------------------
// MurmurHashAligned2, by Austin Appleby
// Same algorithm as MurmurHash2, but only does aligned reads - should be safer
// on certain platforms.
// Performance will be lower than MurmurHash2

#define MIX(h,k,m) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

static uint32_t MurmurHash2(const void * key, size_t len, uint32_t seed)
{
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;
  const uint8_t * data = (const uint8_t *)key;
  uint32_t h = seed ^ len;
  uint8_t align = (uintptr_t)data & 3;

  if(align && (len >= 4)) {
    // Pre-load the temp registers
    uint32_t t = 0, d = 0;

    switch(align) {
      case 1: t |= data[2] << 16;
      case 2: t |= data[1] << 8;
      case 3: t |= data[0];
    }

    t <<= (8 * align);

    data += 4-align;
    len -= 4-align;

    int32_t sl = 8 * (4-align);
    int32_t sr = 8 * align;

    // Mix

    while(len >= 4) {
      d = *(uint32_t *)data;
      t = (t >> sr) | (d << sl);

      uint32_t k = t;

      MIX(h,k,m);

      t = d;

      data += 4;
      len -= 4;
    }

    // Handle leftover data in temp registers

    d = 0;

    if(len >= align) {
      switch(align) {
        case 3: d |= data[2] << 16;
        case 2: d |= data[1] << 8;
        case 1: d |= data[0];
      }

      uint32_t k = (t >> sr) | (d << sl);
      MIX(h,k,m);

      data += align;
      len -= align;

      //----------
      // Handle tail bytes

      switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0]; h *= m;
      };
    } else {
      switch(len) {
        case 3: d |= data[2] << 16;
        case 2: d |= data[1] << 8;
        case 1: d |= data[0];
        case 0: h ^= (t >> sr) | (d << sl); h *= m;
      }
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  } else {
    while(len >= 4) {
      uint32_t k = *(uint32_t *)data;

      MIX(h,k,m);

      data += 4;
      len -= 4;
    }

    //----------
    // Handle tail bytes

    switch(len) {
      case 3: h ^= data[2] << 16;
      case 2: h ^= data[1] << 8;
      case 1: h ^= data[0]; h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }
}
#undef MIX

#endif // UPB_UNALIGNED_READS_OK

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

static struct upb_inttable_entry *find_empty_slot(struct upb_inttable *table)
{
  /* TODO: does it matter that this is biased towards the front of the table? */
  for(uint32_t i = 0; i < table->size; i++) {
    struct upb_inttable_entry *e =
        upb_inttable_entry_get(table->entries, i, table->entry_size);
    if(e->key == UPB_EMPTY_ENTRY) return e;
  }
  assert(false);
  return NULL;
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
  table->entry_size = entry_size;
  table->entries = malloc(table->size * entry_size);

  /* Initialize to empty. */
  for(size_t i = 0; i < table->size; i++) {
    struct upb_inttable_entry *e =
        upb_inttable_entry_get(table->entries, i, entry_size);
    e->key = UPB_EMPTY_ENTRY;
    e->next = NULL;
  }

  /* Insert the elements. */
  for(int i = 0; i < num_entries; i++) {
    struct upb_inttable_entry *e, *table_e;
    e = upb_inttable_entry_get(entries, i, entry_size);
    table_e = upb_inttable_mainpos(table, e->key);
    if(table_e->key != UPB_EMPTY_ENTRY) {  /* Collision. */
      if(table_e == upb_inttable_mainpos(table, table_e->key)) {
        /* Existing element is in its main posisiton.  Find an empty slot to
         * place our new element and append it to this key's chain. */
        struct upb_inttable_entry *empty = find_empty_slot(table);
        while (table_e->next) table_e = table_e->next;
        table_e->next = empty;
        table_e = empty;
      } else {
        /* Existing element is not in its main position.  Move it to an empty
         * slot and put our element in its main position. */
        struct upb_inttable_entry *empty, *colliding_key_mainpos;
        empty = find_empty_slot(table);
        colliding_key_mainpos = upb_inttable_mainpos(table, table_e->key);
        assert(colliding_key_mainpos->key != UPB_EMPTY_ENTRY);
        assert(colliding_key_mainpos->next);
        memcpy(empty, table_e, entry_size);  /* next is copied also. */
        while(1) {
          assert(colliding_key_mainpos->next);
          if(colliding_key_mainpos->next == table_e) {
            colliding_key_mainpos->next = empty;
            break;
          }
        }
        /* table_e remains set to our mainpos. */
      }
    }
    memcpy(table_e, e, entry_size);
    table_e->next = NULL;
  }
}

void upb_inttable_free(struct upb_inttable *table)
{
  free(table->entries);
}

