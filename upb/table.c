/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Implementation is heavily inspired by Lua's ltable.c.
 *
 * TODO: for table iteration we use (array - 1) in several places; is this
 * undefined behavior?  If so find a better solution.
 */

#include "upb/table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define UPB_MAXARRSIZE 16  // 64k.

static const double MAX_LOAD = 0.85;

// The minimum percentage of an array part that we will allow.  This is a
// speed/memory-usage tradeoff (though it's not straightforward because of
// cache effects).  The lower this is, the more memory we'll use.
static const double MIN_DENSITY = 0.1;

int upb_log2(uint64_t v) {
#ifdef __GNUC__
  int ret = 31 - __builtin_clz(v);
#else
  int ret = 0;
  while (v >>= 1) ret++;
#endif
  return UPB_MIN(UPB_MAXARRSIZE, ret);
}

static upb_tabkey upb_strkey(const char *str) {
  upb_tabkey k;
  k.str = (char*)str;
  return k;
}

static uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed);
typedef upb_tabent *upb_hashfunc_t(const upb_table *t, upb_tabkey key);
typedef bool upb_eqlfunc_t(upb_tabkey k1, upb_tabkey k2);

/* Base table (shared code) ***************************************************/

static size_t upb_table_size(const upb_table *t) { return 1 << t->size_lg2; }

static bool upb_table_isfull(upb_table *t) {
  return (double)(t->count + 1) / upb_table_size(t) > MAX_LOAD;
}

static bool upb_table_init(upb_table *t, uint8_t size_lg2) {
  t->count = 0;
  t->size_lg2 = size_lg2;
  size_t bytes = upb_table_size(t) * sizeof(upb_tabent);
  t->mask = upb_table_size(t) - 1;
  t->entries = malloc(bytes);
  if (!t->entries) return false;
  memset(t->entries, 0, bytes);
  return true;
}

static void upb_table_uninit(upb_table *t) { free(t->entries); }

static bool upb_tabent_isempty(const upb_tabent *e) { return e->key.num == 0; }

static upb_tabent *upb_table_emptyent(const upb_table *t) {
  upb_tabent *e = t->entries + upb_table_size(t);
  while (1) { if (upb_tabent_isempty(--e)) return e; assert(e > t->entries); }
}

static upb_value *upb_table_lookup(const upb_table *t, upb_tabkey key,
                                   upb_hashfunc_t *hash, upb_eqlfunc_t *eql) {
  upb_tabent *e = hash(t, key);
  if (upb_tabent_isempty(e)) return NULL;
  while (1) {
    if (eql(e->key, key)) return &e->val;
    if ((e = e->next) == NULL) return NULL;
  }
}

// The given key must not already exist in the table.
static void upb_table_insert(upb_table *t, upb_tabkey key, upb_value val,
                             upb_hashfunc_t *hash, upb_eqlfunc_t *eql) {
  assert(upb_table_lookup(t, key, hash, eql) == NULL);
  t->count++;
  upb_tabent *mainpos_e = hash(t, key);
  upb_tabent *our_e = mainpos_e;
  if (!upb_tabent_isempty(mainpos_e)) {  // Collision.
    upb_tabent *new_e = upb_table_emptyent(t);
    upb_tabent *chain = hash(t, mainpos_e->key);  // Head of collider's chain.
    if (chain == mainpos_e) {
      // Existing ent is in its main posisiton (it has the same hash as us, and
      // is the head of our chain).  Insert to new ent and append to this chain.
      new_e->next = mainpos_e->next;
      mainpos_e->next = new_e;
      our_e = new_e;
    } else {
      // Existing ent is not in its main position (it is a node in some other
      // chain).  This implies that no existing ent in the table has our hash.
      // Evict it (updating its chain) and use its ent for head of our chain.
      *new_e = *mainpos_e;  // copies next.
      while (chain->next != mainpos_e) chain = chain->next;
      chain->next = new_e;
      our_e = mainpos_e;
      our_e->next = NULL;
    }
  }
  our_e->key = key;
  our_e->val = val;
  assert(upb_table_lookup(t, key, hash, eql) == &our_e->val);
}

static bool upb_table_remove(upb_table *t, upb_tabkey key, upb_value *val,
                             upb_hashfunc_t *hash, upb_eqlfunc_t *eql) {
  upb_tabent *chain = hash(t, key);
  if (eql(chain->key, key)) {
    t->count--;
    if (val) *val = chain->val;
    if (chain->next) {
      upb_tabent *move = chain->next;
      *chain = *move;
      move->key.num = 0;  // Make the slot empty.
    } else {
      chain->key.num = 0;  // Make the slot empty.
    }
    return true;
  } else {
    while (chain->next && !eql(chain->next->key, key))
      chain = chain->next;
    if (chain->next) {
      // Found element to remove.
      if (val) *val = chain->next->val;
      chain->next->key.num = 0;
      chain->next = chain->next->next;
      t->count--;
      return true;
    } else {
      return false;
    }
  }
}

static upb_tabent *upb_table_next(const upb_table *t, upb_tabent *e) {
  upb_tabent *end = t->entries + upb_table_size(t);
  do { if (++e == end) return NULL; } while(e->key.num == 0);
  return e;
}

static upb_tabent *upb_table_begin(const upb_table *t) {
  return upb_table_next(t, t->entries - 1);
}


/* upb_strtable ***************************************************************/

// A simple "subclass" of upb_table that only adds a hash function for strings.

static upb_tabent *upb_strhash(const upb_table *t, upb_tabkey key) {
  // Could avoid the strlen() by using a hash function that terminates on NULL.
  return t->entries + (MurmurHash2(key.str, strlen(key.str), 0) & t->mask);
}

static bool upb_streql(upb_tabkey k1, upb_tabkey k2) {
  return strcmp(k1.str, k2.str) == 0;
}

bool upb_strtable_init(upb_strtable *t) { return upb_table_init(&t->t, 4); }

void upb_strtable_uninit(upb_strtable *t) {
  for (size_t i = 0; i < upb_table_size(&t->t); i++)
    free(t->t.entries[i].key.str);
  upb_table_uninit(&t->t);
}

bool upb_strtable_insert(upb_strtable *t, const char *k, upb_value v) {
  if (upb_table_isfull(&t->t)) {
    // Need to resize.  New table of double the size, add old elements to it.
    upb_strtable new_table;
    if (!upb_table_init(&new_table.t, t->t.size_lg2 + 1)) return false;
    upb_strtable_iter i;
    upb_strtable_begin(&i, t);
    for ( ; !upb_strtable_done(&i); upb_strtable_next(&i)) {
      upb_strtable_insert(
          &new_table, upb_strtable_iter_key(&i), upb_strtable_iter_value(&i));
    }
    upb_strtable_uninit(t);
    *t = new_table;
  }
  if ((k = strdup(k)) == NULL) return false;
  upb_table_insert(&t->t, upb_strkey(k), v, &upb_strhash, &upb_streql);
  return true;
}

upb_value *upb_strtable_lookup(const upb_strtable *t, const char *key) {
  return upb_table_lookup(&t->t, upb_strkey(key), &upb_strhash, &upb_streql);
}

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t) {
  i->t = t;
  i->e = upb_table_begin(&t->t);
}

void upb_strtable_next(upb_strtable_iter *i) {
  i->e = upb_table_next(&i->t->t, i->e);
}


/* upb_inttable ***************************************************************/

// For inttables we use a hybrid structure where small keys are kept in an
// array and large keys are put in the hash table.

static bool upb_inteql(upb_tabkey k1, upb_tabkey k2) {
  return k1.num == k2.num;
}

size_t upb_inttable_count(const upb_inttable *t) {
  return t->t.count + t->array_count;
}

bool upb_inttable_sizedinit(upb_inttable *t, size_t asize, int hsize_lg2) {
  if (!upb_table_init(&t->t, hsize_lg2)) return false;
  // Always make the array part at least 1 long, so that we know key 0
  // won't be in the hash part, which simplifies things.
  t->array_size = UPB_MAX(1, asize);
  t->array_count = 0;
  size_t array_bytes = t->array_size * sizeof(upb_value);
  t->array = malloc(array_bytes);
  if (!t->array) {
    upb_table_uninit(&t->t);
    return false;
  }
  memset(t->array, 0xff, array_bytes);
  return true;
}

bool upb_inttable_init(upb_inttable *t) {
  return upb_inttable_sizedinit(t, 0, 4);
}

void upb_inttable_uninit(upb_inttable *t) {
  upb_table_uninit(&t->t);
  free(t->array);
}

bool upb_inttable_insert(upb_inttable *t, uintptr_t key, upb_value val) {
  assert(upb_arrhas(val));
  if (key < t->array_size) {
    assert(!upb_arrhas(t->array[key]));
    t->array_count++;
    t->array[key] = val;
  } else {
    if (upb_table_isfull(&t->t)) {
      // Need to resize the hash part, but we re-use the array part.
      upb_table new_table;
      if (!upb_table_init(&new_table, t->t.size_lg2 + 1)) return false;
      upb_tabent *e;
      for (e = upb_table_begin(&t->t); e; e = upb_table_next(&t->t, e))
        upb_table_insert(&new_table, e->key, e->val, &upb_inthash, &upb_inteql);
      upb_table_uninit(&t->t);
      t->t = new_table;
    }
    upb_table_insert(&t->t, upb_intkey(key), val, &upb_inthash, &upb_inteql);
  }
  return true;
}

upb_value *upb_inttable_lookup(const upb_inttable *t, uintptr_t key) {
  if (key < t->array_size) {
    upb_value *v = &t->array[key];
    return upb_arrhas(*v) ? v : NULL;
  }
  return upb_table_lookup(&t->t, upb_intkey(key), &upb_inthash, &upb_inteql);
}

bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val) {
  if (key < t->array_size) {
    if (upb_arrhas(t->array[key])) {
      t->array_count--;
      if (val) *val = t->array[key];
      t->array[key] = upb_value_uint64(-1);
      return true;
    } else {
      return false;
    }
  } else {
    return upb_table_remove(
        &t->t, upb_intkey(key), val, &upb_inthash, &upb_inteql);
  }
}

void upb_inttable_compact(upb_inttable *t) {
  // Find the largest power of two that satisfies the MIN_DENSITY definition.
  int counts[UPB_MAXARRSIZE + 1] = {0};
  upb_inttable_iter i;
  for (upb_inttable_begin(&i, t); !upb_inttable_done(&i); upb_inttable_next(&i))
    counts[upb_log2(upb_inttable_iter_key(&i))]++;
  int count = upb_inttable_count(t);
  int size;
  for (size = UPB_MAXARRSIZE; size > 1; size--) {
    count -= counts[size];
    if (count >= (1 << size) * MIN_DENSITY) break;
  }

  // Insert all elements into new, perfectly-sized table.
  upb_inttable new_table;
  int hashsize = (upb_inttable_count(t) - count + 1) / MAX_LOAD;
  upb_inttable_sizedinit(&new_table, size, upb_log2(hashsize) + 1);
  for (upb_inttable_begin(&i, t); !upb_inttable_done(&i); upb_inttable_next(&i))
    upb_inttable_insert(
        &new_table, upb_inttable_iter_key(&i), upb_inttable_iter_value(&i));
  upb_inttable_uninit(t);
  *t = new_table;
}

void upb_inttable_begin(upb_inttable_iter *i, const upb_inttable *t) {
  i->t = t;
  i->arrkey = -1;
  i->array_part = true;
  upb_inttable_next(i);
}

void upb_inttable_next(upb_inttable_iter *iter) {
  const upb_inttable *t = iter->t;
  if (iter->array_part) {
    for (size_t i = iter->arrkey; ++i < t->array_size; )
      if (upb_arrhas(t->array[i])) {
        iter->ptr.val = &t->array[i];
        iter->arrkey = i;
        return;
      }
    iter->array_part = false;
    iter->ptr.ent = t->t.entries - 1;
  }
  iter->ptr.ent = upb_table_next(&t->t, iter->ptr.ent);
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
static uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed) {
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

static uint32_t MurmurHash2(const void * key, size_t len, uint32_t seed) {
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
