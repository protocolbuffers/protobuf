/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const upb_inttable_key_t EMPTYENT = 0;
static const double MAX_LOAD = 0.85;

static uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed);

static uint32_t max(uint32_t a, uint32_t b) { return a > b ? a : b; }
static struct upb_inttable_entry *intent(struct upb_inttable *t, uint32_t i) {
  return (struct upb_inttable_entry*)((char*)t->t.entries + i*t->t.entry_size);
}
static struct upb_strtable_entry *strent(struct upb_strtable *t, uint32_t i) {
  return (struct upb_strtable_entry*)((char*)t->t.entries + i*t->t.entry_size);
}

void upb_table_init(struct upb_table *t, uint32_t size, uint16_t entry_size)
{
  t->count = 0;
  t->entry_size = entry_size;
  t->size_lg2 = 1;
  while(size >>= 1) t->size_lg2++;
  t->size_lg2 = max(t->size_lg2, 4);  /* Min size of 16. */
  t->entries = malloc(upb_table_size(t) * t->entry_size);
}

void upb_inttable_init(struct upb_inttable *t, uint32_t size, uint16_t entsize)
{
  upb_table_init(&t->t, size, entsize);
  for(uint32_t i = 0; i < upb_table_size(&t->t); i++)
    intent(t, i)->key = EMPTYENT;
}

void upb_strtable_init(struct upb_strtable *t, uint32_t size, uint16_t entsize)
{
  upb_table_init(&t->t, size, entsize);
  for(uint32_t i = 0; i < upb_table_size(&t->t); i++)
    strent(t, i)->key.data = NULL;
}

void upb_table_free(struct upb_table *t) { free(t->entries); }
void upb_inttable_free(struct upb_inttable *t) { upb_table_free(&t->t); }
void upb_strtable_free(struct upb_strtable *t) { upb_table_free(&t->t); }

void *upb_strtable_lookup(struct upb_strtable *t, struct upb_string *key)
{
  uint32_t hash =
      MurmurHash2(key->data, key->byte_len, 0) & (upb_strtable_size(t)-1);
  while(1) {
    struct upb_strtable_entry *e =
        (struct upb_strtable_entry*)(char*)t->t.entries + hash*t->t.entry_size;
    if(e->key.data == NULL) return NULL;
    else if(upb_string_eql(&e->key, key)) return e;
    hash = e->next;
  }
}

static struct upb_inttable_entry *find_empty_slot(struct upb_inttable *table)
{
  /* TODO: does it matter that this is biased towards the front of the table? */
  for(uint32_t i = 0; i < upb_inttable_size(table); i++) {
    struct upb_inttable_entry *e = intent(table, i);
    if(e->key == EMPTYENT) return e;
  }
  assert(false);
  return NULL;
}

static void *maybe_resize(struct upb_table *t) {
  if((double)++t->count / upb_table_size(t) > MAX_LOAD) {
    void *old_entries = t->entries;
    t->size_lg2++;  /* Double the table size. */
    t->entries = malloc(upb_table_size(t) * t->entry_size);
    return old_entries;
  } else {
    return NULL;  /* No resize necessary. */
  }
}

static void intinsert(void *table, struct upb_inttable_entry *e, uint32_t size)
{
  /* TODO */
#if 0
  struct upb_inttable_entry *e, *table_e;
  e = upb_inttable_entry_get(entries, i, entry_size);
  table_e = upb_inttable_mainpos2(table, e->key);
  if(table_e->key != UPB_EMPTY_ENTRY) {  /* Collision. */
    if(table_e == upb_inttable_mainpos2(table, table_e->key)) {
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
      colliding_key_mainpos = upb_inttable_mainpos2(table, table_e->key);
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
#endif
}

void upb_inttable_insert(struct upb_inttable *t, struct upb_inttable_entry *e)
{
  void *new_entries = maybe_resize(&t->t);
  if(new_entries) {  /* Are we doing a resize? */
    for(uint32_t i = 0; i < (upb_inttable_size(t)>>1); i++) {
      struct upb_inttable_entry *old_e = intent(t, i);
      if(old_e->key != EMPTYENT) intinsert(new_entries, old_e, t->t.entry_size);
    }
  }
  intinsert(t->t.entries, e, t->t.entry_size);
}

static void strinsert(void *table, struct upb_strtable_entry *e, uint32_t size)
{
  /* TODO */
}

void upb_strtable_insert(struct upb_strtable *t, struct upb_strtable_entry *e)
{
  void *new_entries = maybe_resize(&t->t);
  if(new_entries) {  /* Are we doing a resize? */
    for(uint32_t i = 0; i < (upb_strtable_size(t)>>1); i++) {
      struct upb_strtable_entry *old_e = strent(t, i);
      if(old_e->key.data != NULL) strinsert(new_entries, old_e, t->t.entry_size);
    }
  }
  strinsert(t->t.entries, e, t->t.entry_size);
}

#ifdef UPB_UNALIGNED_READS_OK
//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby (released as public domain).
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
