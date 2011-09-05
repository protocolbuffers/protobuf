/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * There are a few printf's strewn throughout this file, uncommenting them
 * can be useful for debugging.
 */

#include "upb/table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const double MAX_LOAD = 0.85;

// The minimum percentage of an array part that we will allow.  This is a
// speed/memory-usage tradeoff (though it's not straightforward because of
// cache effects).  The lower this is, the more memory we'll use.
static const double MIN_DENSITY = 0.1;

static uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed);

/* Base table (shared code) ***************************************************/

static uint32_t upb_table_size(const upb_table *t) { return 1 << t->size_lg2; }
static size_t upb_table_entrysize(const upb_table *t) { return t->entry_size; }
static size_t upb_table_valuesize(const upb_table *t) { return t->value_size; }

void upb_table_init(upb_table *t, uint32_t size, uint16_t entry_size) {
  t->count = 0;
  t->entry_size = entry_size;
  t->size_lg2 = 1;
  while(upb_table_size(t) < size) t->size_lg2++;
  size_t bytes = upb_table_size(t) * t->entry_size;
  t->mask = upb_table_size(t) - 1;
  t->entries = malloc(bytes);
}

void upb_table_free(upb_table *t) { free(t->entries); }

/* upb_inttable ***************************************************************/

static upb_inttable_entry *intent(const upb_inttable *t, int32_t i) {
  //printf("looking up int entry %d, size of entry: %d\n", i, t->t.entry_size);
  return UPB_INDEX(t->t.entries, i, t->t.entry_size);
}

static uint32_t upb_inttable_hashtablesize(const upb_inttable *t) {
  return upb_table_size(&t->t);
}

void upb_inttable_sizedinit(upb_inttable *t, uint32_t arrsize, uint32_t hashsize,
                            uint16_t value_size) {
  size_t entsize = _upb_inttable_entrysize(value_size);
  upb_table_init(&t->t, hashsize, entsize);
  for (uint32_t i = 0; i < upb_table_size(&t->t); i++) {
    upb_inttable_entry *e = intent(t, i);
    e->hdr.key = 0;
    e->hdr.next = UPB_END_OF_CHAIN;
    e->val.has_entry = 0;
  }
  t->t.value_size = value_size;
  // Always make the array part at least 1 long, so that we know key 0
  // won't be in the hash part (which lets us speed up that code path).
  t->array_size = UPB_MAX(1, arrsize);
  t->array = malloc(upb_table_valuesize(&t->t) * t->array_size);
  t->array_count = 0;
  for (uint32_t i = 0; i < t->array_size; i++) {
    upb_inttable_value *val = UPB_INDEX(t->array, i, upb_table_valuesize(&t->t));
    val->has_entry = false;
  }
}

void upb_inttable_init(upb_inttable *t, uint32_t hashsize, uint16_t value_size) {
  upb_inttable_sizedinit(t, 0, hashsize, value_size);
}

void upb_inttable_free(upb_inttable *t) {
  upb_table_free(&t->t);
  free(t->array);
}

static uint32_t empty_intbucket(upb_inttable *table)
{
  // TODO: does it matter that this is biased towards the front of the table?
  for(uint32_t i = 0; i < upb_inttable_hashtablesize(table); i++) {
    upb_inttable_entry *e = intent(table, i);
    if(!e->val.has_entry) return i;
  }
  assert(false);
  return 0;
}

// The insert routines have a lot more code duplication between int/string
// variants than I would like, but there's just a bit too much that varies to
// parameterize them.
static void intinsert(upb_inttable *t, uint32_t key, const void *val) {
  assert(upb_inttable_lookup(t, key) == NULL);
  upb_inttable_value *table_val;
  if (_upb_inttable_isarrkey(t, key)) {
    table_val = UPB_INDEX(t->array, key, upb_table_valuesize(&t->t));
    t->array_count++;
    //printf("Inserting key %d to Array part! %p\n", key, table_val);
  } else {
    t->t.count++;
    uint32_t bucket = _upb_inttable_bucket(t, key);
    upb_inttable_entry *table_e = intent(t, bucket);
    //printf("Hash part!  Inserting into bucket %d?\n", bucket);
    if(table_e->val.has_entry) {  /* Collision. */
      //printf("Collision!\n");
      if(bucket == _upb_inttable_bucket(t, table_e->hdr.key)) {
        /* Existing element is in its main posisiton.  Find an empty slot to
         * place our new element and append it to this key's chain. */
        uint32_t empty_bucket = empty_intbucket(t);
        while (table_e->hdr.next != UPB_END_OF_CHAIN)
          table_e = intent(t, table_e->hdr.next);
        table_e->hdr.next = empty_bucket;
        table_e = intent(t, empty_bucket);
      } else {
        /* Existing element is not in its main position.  Move it to an empty
         * slot and put our element in its main position. */
        uint32_t empty_bucket = empty_intbucket(t);
        uint32_t evictee_bucket = _upb_inttable_bucket(t, table_e->hdr.key);
        memcpy(intent(t, empty_bucket), table_e, t->t.entry_size); /* copies next */
        upb_inttable_entry *evictee_e = intent(t, evictee_bucket);
        while(1) {
          assert(evictee_e->val.has_entry);
          assert(evictee_e->hdr.next != UPB_END_OF_CHAIN);
          if(evictee_e->hdr.next == bucket) {
            evictee_e->hdr.next = empty_bucket;
            break;
          }
          evictee_e = intent(t, evictee_e->hdr.next);
        }
        /* table_e remains set to our mainpos. */
      }
    }
    //printf("Inserting!  to:%p, copying to: %p\n", table_e, &table_e->val);
    table_val = &table_e->val;
    table_e->hdr.key = key;
    table_e->hdr.next = UPB_END_OF_CHAIN;
  }
  memcpy(table_val, val, upb_table_valuesize(&t->t));
  table_val->has_entry = true;
  assert(upb_inttable_lookup(t, key) == table_val);
}

// Insert all elements from src into dest.  Caller ensures that a resize will
// not be necessary.
static void upb_inttable_insertall(upb_inttable *dst, upb_inttable *src) {
  for(upb_inttable_iter i = upb_inttable_begin(src); !upb_inttable_done(i);
      i = upb_inttable_next(src, i)) {
    //printf("load check: %d %d\n", upb_table_count(&dst->t), upb_inttable_hashtablesize(dst));
    assert((double)(upb_table_count(&dst->t)) /
                    upb_inttable_hashtablesize(dst) <= MAX_LOAD);
    intinsert(dst, upb_inttable_iter_key(i), upb_inttable_iter_value(i));
  }
}

void upb_inttable_insert(upb_inttable *t, uint32_t key, const void *val) {
  if((double)(t->t.count + 1) / upb_inttable_hashtablesize(t) > MAX_LOAD) {
    //printf("RESIZE!\n");
    // Need to resize.  Allocate new table with double the size of however many
    // elements we have now, add old elements to it.  We create the new hash
    // table without an array part, even if the old table had an array part.
    // If/when the user calls upb_inttable_compact() again, we'll create an
    // array part then.
    upb_inttable new_table;
    //printf("Old table count=%d, size=%d\n", upb_inttable_count(t), upb_inttable_hashtablesize(t));
    upb_inttable_init(&new_table, upb_inttable_count(t)*2, upb_table_valuesize(&t->t));
    upb_inttable_insertall(&new_table, t);
    upb_inttable_free(t);
    *t = new_table;
  }
  intinsert(t, key, val);
}

void upb_inttable_compact(upb_inttable *t) {
  // Find the largest array part we can that satisfies the MIN_DENSITY
  // definition.  For now we just count down powers of two.
  uint32_t largest_key = 0;
  for(upb_inttable_iter i = upb_inttable_begin(t); !upb_inttable_done(i);
      i = upb_inttable_next(t, i)) {
    largest_key = UPB_MAX(largest_key, upb_inttable_iter_key(i));
  }
  int lg2_array = 0;
  while ((1UL << lg2_array) < largest_key) ++lg2_array;
  ++lg2_array;  // Undo the first iteration.
  size_t array_size;
  int array_count = 0;
  while (lg2_array > 0) {
    array_size = (1 << --lg2_array);
    //printf("Considering size %d (btw, our table has %d things total)\n", array_size, upb_inttable_count(t));
    if ((double)upb_inttable_count(t) / array_size < MIN_DENSITY) {
      // Even if 100% of the keys were in the array pary, an array of this
      // size would not be dense enough.
      continue;
    }
    array_count = 0;
    for(upb_inttable_iter i = upb_inttable_begin(t); !upb_inttable_done(i);
        i = upb_inttable_next(t, i)) {
      if (upb_inttable_iter_key(i) < array_size)
        array_count++;
    }
    //printf("There would be %d things in that array\n", array_count);
    if ((double)array_count / array_size >= MIN_DENSITY) break;
  }
  upb_inttable new_table;
  int hash_size = (upb_inttable_count(t) - array_count + 1) / MAX_LOAD;
  //printf("array_count: %d, array_size: %d, hash_size: %d, table size: %d\n", array_count, array_size, hash_size, upb_inttable_count(t));
  upb_inttable_sizedinit(&new_table, array_size, hash_size,
                         upb_table_valuesize(&t->t));
  //printf("For %d things, using array size=%d, hash_size = %d\n", upb_inttable_count(t), array_size, hash_size);
  upb_inttable_insertall(&new_table, t);
  upb_inttable_free(t);
  *t = new_table;
}

upb_inttable_iter upb_inttable_begin(const upb_inttable *t) {
  upb_inttable_iter iter = {-1, NULL, true};  // -1 will overflow to 0 on the first iteration.
  return upb_inttable_next(t, iter);
}

upb_inttable_iter upb_inttable_next(const upb_inttable *t,
                                    upb_inttable_iter iter) {
  const size_t hdrsize = sizeof(upb_inttable_header);
  const size_t entsize = upb_table_entrysize(&t->t);
  if (iter.array_part) {
    while (++iter.key < t->array_size) {
      //printf("considering value %d\n", iter.key);
      iter.value = UPB_INDEX(t->array, iter.key, t->t.value_size);
      if (iter.value->has_entry) return iter;
    }
    //printf("Done with array part!\n");
    iter.array_part = false;
    // Point to the value of the table[-1] entry.
    iter.value = UPB_INDEX(intent(t, -1), 1, hdrsize);
  }
  void *end = intent(t, upb_inttable_hashtablesize(t));
  // Point to the entry for the value that was previously in iter.
  upb_inttable_entry *e = UPB_INDEX(iter.value, -1, hdrsize);
  do {
    e = UPB_INDEX(e, 1, entsize);
    //printf("considering value %p (val: %p)\n", e, &e->val);
    if(e == end) {
      //printf("No values.\n");
      iter.value = NULL;
      return iter;
    }
  } while(!e->val.has_entry);
  //printf("USING VALUE! %p\n", e);
  iter.key = e->hdr.key;
  iter.value = &e->val;
  return iter;
}


/* upb_strtable ***************************************************************/

static upb_strtable_entry *strent(const upb_strtable *t, int32_t i) {
  //fprintf(stderr, "i: %d, table_size: %d\n", i, upb_table_size(&t->t));
  assert(i <= (int32_t)upb_table_size(&t->t));
  return UPB_INDEX(t->t.entries, i, t->t.entry_size);
}

static uint32_t upb_strtable_size(const upb_strtable *t) {
  return upb_table_size(&t->t);
}

void upb_strtable_init(upb_strtable *t, uint32_t size, uint16_t valuesize) {
  t->t.value_size = valuesize;
  size_t entsize = upb_align_up(sizeof(upb_strtable_header) + valuesize, 8);
  upb_table_init(&t->t, size, entsize);
  for (uint32_t i = 0; i < upb_table_size(&t->t); i++) {
    upb_strtable_entry *e = strent(t, i);
    e->hdr.key = NULL;
    e->hdr.next = UPB_END_OF_CHAIN;
  }
}

void upb_strtable_free(upb_strtable *t) {
  // Free keys from the strtable.
  upb_strtable_iter i;
  for(upb_strtable_begin(&i, t); !upb_strtable_done(&i); upb_strtable_next(&i))
    free((char*)upb_strtable_iter_key(&i));
  upb_table_free(&t->t);
}

static uint32_t strtable_bucket(const upb_strtable *t, const char *key) {
  uint32_t hash = MurmurHash2(key, strlen(key), 0);
  return (hash & t->t.mask);
}

void *upb_strtable_lookup(const upb_strtable *t, const char *key) {
  uint32_t bucket = strtable_bucket(t, key);
  upb_strtable_entry *e;
  do {
    e = strent(t, bucket);
    if(e->hdr.key && strcmp(e->hdr.key, key) == 0) return &e->val;
  } while((bucket = e->hdr.next) != UPB_END_OF_CHAIN);
  return NULL;
}

void *upb_strtable_lookupl(const upb_strtable *t, const char *key, size_t len) {
  // TODO: improve.
  char key2[len+1];
  memcpy(key2, key, len);
  key2[len] = '\0';
  return upb_strtable_lookup(t, key2);
}

static uint32_t empty_strbucket(upb_strtable *table) {
  // TODO: does it matter that this is biased towards the front of the table?
  for(uint32_t i = 0; i < upb_strtable_size(table); i++) {
    upb_strtable_entry *e = strent(table, i);
    if(!e->hdr.key) return i;
  }
  assert(false);
  return 0;
}

static void strinsert(upb_strtable *t, const char *key, const void *val) {
  assert(upb_strtable_lookup(t, key) == NULL);
  t->t.count++;
  uint32_t bucket = strtable_bucket(t, key);
  upb_strtable_entry *table_e = strent(t, bucket);
  if(table_e->hdr.key) {  /* Collision. */
    if(bucket == strtable_bucket(t, table_e->hdr.key)) {
      /* Existing element is in its main posisiton.  Find an empty slot to
       * place our new element and append it to this key's chain. */
      uint32_t empty_bucket = empty_strbucket(t);
      while (table_e->hdr.next != UPB_END_OF_CHAIN)
        table_e = strent(t, table_e->hdr.next);
      table_e->hdr.next = empty_bucket;
      table_e = strent(t, empty_bucket);
    } else {
      /* Existing element is not in its main position.  Move it to an empty
       * slot and put our element in its main position. */
      uint32_t empty_bucket = empty_strbucket(t);
      uint32_t evictee_bucket = strtable_bucket(t, table_e->hdr.key);
      memcpy(strent(t, empty_bucket), table_e, t->t.entry_size); /* copies next */
      upb_strtable_entry *evictee_e = strent(t, evictee_bucket);
      while(1) {
        assert(evictee_e->hdr.key);
        assert(evictee_e->hdr.next != UPB_END_OF_CHAIN);
        if(evictee_e->hdr.next == bucket) {
          evictee_e->hdr.next = empty_bucket;
          break;
        }
        evictee_e = strent(t, evictee_e->hdr.next);
      }
      /* table_e remains set to our mainpos. */
    }
  }
  //fprintf(stderr, "val: %p\n", val);
  //fprintf(stderr, "val size: %d\n", t->t.value_size);
  memcpy(&table_e->val, val, t->t.value_size);
  table_e->hdr.key = strdup(key);
  table_e->hdr.next = UPB_END_OF_CHAIN;
  //fprintf(stderr, "Looking up, string=%s...\n", key);
  assert(upb_strtable_lookup(t, key) == &table_e->val);
  //printf("Yay!\n");
}

void upb_strtable_insert(upb_strtable *t, const char *key, const void *val) {
  if((double)(t->t.count + 1) / upb_strtable_size(t) > MAX_LOAD) {
    // Need to resize.  New table of double the size, add old elements to it.
    //printf("RESIZE!!\n");
    upb_strtable new_table;
    upb_strtable_init(&new_table, upb_strtable_size(t)*2, t->t.value_size);
    upb_strtable_iter i;
    upb_strtable_begin(&i, t);
    for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
      strinsert(&new_table,
                upb_strtable_iter_key(&i),
                upb_strtable_iter_value(&i));
    }
    upb_strtable_free(t);
    *t = new_table;
  }
  strinsert(t, key, val);
}

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t) {
  i->e = strent(t, -1);
  i->t = t;
  upb_strtable_next(i);
}

void upb_strtable_next(upb_strtable_iter *i) {
  upb_strtable_entry *end = strent(i->t, upb_strtable_size(i->t));
  upb_strtable_entry *cur = i->e;
  do {
    cur = (void*)((char*)cur + i->t->t.entry_size);
    if(cur == end) { i->e = NULL; return; }
  } while(cur->hdr.key == NULL);
  i->e = cur;
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
