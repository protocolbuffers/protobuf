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

/* We use 1-based indexes into the table so that 0 can be "NULL". */
static struct upb_inttable_entry *intent(struct upb_inttable *t, int32_t i) {
  return UPB_INDEX(t->t.entries, i-1, t->t.entry_size);
}
static struct upb_strtable_entry *strent(struct upb_strtable *t, int32_t i) {
  return UPB_INDEX(t->t.entries, i-1, t->t.entry_size);
}

void upb_table_init(struct upb_table *t, uint32_t size, uint16_t entry_size)
{
  t->count = 0;
  t->entry_size = entry_size;
  t->size_lg2 = 1;
  while(size >>= 1) t->size_lg2++;
  t->size_lg2 = max(t->size_lg2, 4);  /* Min size of 16. */
  size_t bytes = upb_table_size(t) * t->entry_size;
  t->entries = malloc(bytes);
  memset(t->entries, 0, bytes);  /* Both tables consider 0's an empty entry. */
}

void upb_inttable_init(struct upb_inttable *t, uint32_t size, uint16_t entsize)
{
  upb_table_init(&t->t, size, entsize);
}

void upb_strtable_init(struct upb_strtable *t, uint32_t size, uint16_t entsize)
{
  upb_table_init(&t->t, size, entsize);
}

void upb_table_free(struct upb_table *t) { free(t->entries); }
void upb_inttable_free(struct upb_inttable *t) { upb_table_free(&t->t); }
void upb_strtable_free(struct upb_strtable *t) { upb_table_free(&t->t); }

static uint32_t strtable_bucket(struct upb_strtable *t, struct upb_string *key)
{
  uint32_t hash = MurmurHash2(key->ptr, key->byte_len, 0);
  return (hash & (upb_strtable_size(t)-1)) + 1;
}

void *upb_strtable_lookup(struct upb_strtable *t, struct upb_string *key)
{
  uint32_t bucket = strtable_bucket(t, key);
  struct upb_strtable_entry *e;
  do {
    e = strent(t, bucket);
    if(upb_streql(&e->key, key)) return e;
  } while((bucket = e->next) != UPB_END_OF_CHAIN);
  return NULL;
}

static uint32_t empty_intbucket(struct upb_inttable *table)
{
  /* TODO: does it matter that this is biased towards the front of the table? */
  for(uint32_t i = 1; i <= upb_inttable_size(table); i++) {
    struct upb_inttable_entry *e = intent(table, i);
    if(e->key == EMPTYENT) return i;
  }
  assert(false);
  return 0;
}

/* The insert routines have a lot more code duplication between int/string
 * variants than I would like, but there's just a bit too much that varies to
 * parameterize them. */
static void intinsert(struct upb_inttable *t, struct upb_inttable_entry *e)
{
  assert(upb_inttable_lookup(t, e->key, t->t.entry_size) == NULL);
  uint32_t bucket = upb_inttable_bucket(t, e->key);
  struct upb_inttable_entry *table_e = intent(t, bucket);
  if(table_e->key != EMPTYENT) {  /* Collision. */
    if(bucket == upb_inttable_bucket(t, table_e->key)) {
      /* Existing element is in its main posisiton.  Find an empty slot to
       * place our new element and append it to this key's chain. */
      uint32_t empty_bucket = empty_intbucket(t);
      while (table_e->next != UPB_END_OF_CHAIN)
        table_e = intent(t, table_e->next);
      table_e->next = empty_bucket;
      table_e = intent(t, empty_bucket);
    } else {
      /* Existing element is not in its main position.  Move it to an empty
       * slot and put our element in its main position. */
      uint32_t empty_bucket = empty_intbucket(t);
      uint32_t evictee_bucket = upb_inttable_bucket(t, table_e->key);
      memcpy(intent(t, empty_bucket), table_e, t->t.entry_size); /* copies next */
      struct upb_inttable_entry *evictee_e = intent(t, evictee_bucket);
      while(1) {
        assert(evictee_e->key != UPB_EMPTY_ENTRY);
        assert(evictee_e->next != UPB_END_OF_CHAIN);
        if(evictee_e->next == bucket) {
          evictee_e->next = empty_bucket;
          break;
        }
      }
      /* table_e remains set to our mainpos. */
    }
  }
  memcpy(table_e, e, t->t.entry_size);
  table_e->next = UPB_END_OF_CHAIN;
  assert(upb_inttable_lookup(t, e->key, t->t.entry_size) == table_e);
}

void upb_inttable_insert(struct upb_inttable *t, struct upb_inttable_entry *e)
{
  assert(e->key != 0);
  if((double)++t->t.count / upb_inttable_size(t) > MAX_LOAD) {
    /* Need to resize.  New table of double the size, add old elements to it. */
    struct upb_inttable new_table;
    upb_inttable_init(&new_table, upb_inttable_size(t)*2, t->t.entry_size);
    struct upb_inttable_entry *old_e;
    for(old_e = upb_inttable_begin(t); old_e; old_e = upb_inttable_next(t, old_e))
      intinsert(&new_table, old_e);
    upb_inttable_free(t);
    *t = new_table;
  }
  intinsert(t, e);
}

static uint32_t empty_strbucket(struct upb_strtable *table)
{
  /* TODO: does it matter that this is biased towards the front of the table? */
  for(uint32_t i = 1; i <= upb_strtable_size(table); i++) {
    struct upb_strtable_entry *e = strent(table, i);
    if(e->key.byte_len == 0) return i;
  }
  assert(false);
  return 0;
}

static void strinsert(struct upb_strtable *t, struct upb_strtable_entry *e)
{
  assert(upb_strtable_lookup(t, &e->key) == NULL);
  uint32_t bucket = strtable_bucket(t, &e->key);
  struct upb_strtable_entry *table_e = strent(t, bucket);
  if(table_e->key.byte_len != 0) {  /* Collision. */
    if(bucket == strtable_bucket(t, &table_e->key)) {
      /* Existing element is in its main posisiton.  Find an empty slot to
       * place our new element and append it to this key's chain. */
      uint32_t empty_bucket = empty_strbucket(t);
      while (table_e->next != UPB_END_OF_CHAIN)
        table_e = strent(t, table_e->next);
      table_e->next = empty_bucket;
      table_e = strent(t, empty_bucket);
    } else {
      /* Existing element is not in its main position.  Move it to an empty
       * slot and put our element in its main position. */
      uint32_t empty_bucket = empty_strbucket(t);
      uint32_t evictee_bucket = strtable_bucket(t, &table_e->key);
      memcpy(strent(t, empty_bucket), table_e, t->t.entry_size); /* copies next */
      struct upb_strtable_entry *evictee_e = strent(t, evictee_bucket);
      while(1) {
        assert(evictee_e->key.byte_len != 0);
        assert(evictee_e->next != UPB_END_OF_CHAIN);
        if(evictee_e->next == bucket) {
          evictee_e->next = empty_bucket;
          break;
        }
        evictee_e = strent(t, evictee_e->next);
      }
      /* table_e remains set to our mainpos. */
    }
  }
  memcpy(table_e, e, t->t.entry_size);
  table_e->next = UPB_END_OF_CHAIN;
  assert(upb_strtable_lookup(t, &e->key) == table_e);
}

void upb_strtable_insert(struct upb_strtable *t, struct upb_strtable_entry *e)
{
  if((double)++t->t.count / upb_strtable_size(t) > MAX_LOAD) {
    /* Need to resize.  New table of double the size, add old elements to it. */
    struct upb_strtable new_table;
    upb_strtable_init(&new_table, upb_strtable_size(t)*2, t->t.entry_size);
    struct upb_strtable_entry *old_e;
    for(old_e = upb_strtable_begin(t); old_e; old_e = upb_strtable_next(t, old_e))
      strinsert(&new_table, old_e);
    upb_strtable_free(t);
    *t = new_table;
  }
  strinsert(t, e);
}

void *upb_inttable_begin(struct upb_inttable *t) {
  return upb_inttable_next(t, intent(t, 0));
}

void *upb_inttable_next(struct upb_inttable *t, struct upb_inttable_entry *cur) {
  struct upb_inttable_entry *end = intent(t, upb_inttable_size(t)+1);
  do {
    cur = (void*)((char*)cur + t->t.entry_size);
    if(cur == end) return NULL;
  } while(cur->key == UPB_EMPTY_ENTRY);
  return cur;
}

void *upb_strtable_begin(struct upb_strtable *t) {
  return upb_strtable_next(t, strent(t, 0));
}

void *upb_strtable_next(struct upb_strtable *t, struct upb_strtable_entry *cur) {
  struct upb_strtable_entry *end = strent(t, upb_strtable_size(t)+1);
  do {
    cur = (void*)((char*)cur + t->t.entry_size);
    if(cur == end) return NULL;
  } while(cur->key.byte_len == 0);
  return cur;
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
