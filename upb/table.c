/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Implementation is heavily inspired by Lua's ltable.c.
 */

#include "upb/table.int.h"

#include <stdlib.h>
#include <string.h>

#define UPB_MAXARRSIZE 16  // 64k.

// From Chromium.
#define ARRAY_SIZE(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static const double MAX_LOAD = 0.85;

// The minimum utilization of the array part of a mixed hash/array table.  This
// is a speed/memory-usage tradeoff (though it's not straightforward because of
// cache effects).  The lower this is, the more memory we'll use.
static const double MIN_DENSITY = 0.1;

bool is_pow2(uint64_t v) { return v == 0 || (v & (v - 1)) == 0; }

int log2ceil(uint64_t v) {
  int ret = 0;
  bool pow2 = is_pow2(v);
  while (v >>= 1) ret++;
  ret = pow2 ? ret : ret + 1;  // Ceiling.
  return UPB_MIN(UPB_MAXARRSIZE, ret);
}

char *upb_strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *p = malloc(n);
  if (p) memcpy(p, s, n);
  return p;
}

static upb_tabkey strkey(const char *str) {
  upb_tabkey k;
  k.str = (char*)str;
  return k;
}

typedef const upb_tabent *hashfunc_t(const upb_table *t, upb_tabkey key);
typedef bool eqlfunc_t(upb_tabkey k1, upb_tabkey k2);

/* Base table (shared code) ***************************************************/

static bool isfull(upb_table *t) {
  return (double)(t->count + 1) / upb_table_size(t) > MAX_LOAD;
}

static bool init(upb_table *t, upb_ctype_t ctype, uint8_t size_lg2) {
  t->count = 0;
  t->ctype = ctype;
  t->size_lg2 = size_lg2;
  t->mask = upb_table_size(t) ? upb_table_size(t) - 1 : 0;
  size_t bytes = upb_table_size(t) * sizeof(upb_tabent);
  if (bytes > 0) {
    t->entries = malloc(bytes);
    if (!t->entries) return false;
    memset((void*)t->entries, 0, bytes);
  } else {
    t->entries = NULL;
  }
  return true;
}

static void uninit(upb_table *t) { free((void*)t->entries); }

static upb_tabent *emptyent(upb_table *t) {
  upb_tabent *e = (upb_tabent*)t->entries + upb_table_size(t);
  while (1) { if (upb_tabent_isempty(--e)) return e; assert(e > t->entries); }
}

static const upb_tabent *findentry(const upb_table *t, upb_tabkey key,
                                   hashfunc_t *hash, eqlfunc_t *eql) {
  if (t->size_lg2 == 0) return NULL;
  const upb_tabent *e = hash(t, key);
  if (upb_tabent_isempty(e)) return NULL;
  while (1) {
    if (eql(e->key, key)) return e;
    if ((e = e->next) == NULL) return NULL;
  }
}

static bool lookup(const upb_table *t, upb_tabkey key, upb_value *v,
                   hashfunc_t *hash, eqlfunc_t *eql) {
  const upb_tabent *e = findentry(t, key, hash, eql);
  if (e) {
    if (v) {
      _upb_value_setval(v, e->val, t->ctype);
    }
    return true;
  } else {
    return false;
  }
}

// The given key must not already exist in the table.
static void insert(upb_table *t, upb_tabkey key, upb_value val,
                   hashfunc_t *hash, eqlfunc_t *eql) {
  assert(findentry(t, key, hash, eql) == NULL);
  assert(val.ctype == t->ctype);
  t->count++;
  upb_tabent *mainpos_e = (upb_tabent*)hash(t, key);
  upb_tabent *our_e = mainpos_e;
  if (upb_tabent_isempty(mainpos_e)) {
    // Our main position is empty; use it.
    our_e->next = NULL;
  } else {
    // Collision.
    upb_tabent *new_e = emptyent(t);
    // Head of collider's chain.
    upb_tabent *chain = (upb_tabent*)hash(t, mainpos_e->key);
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
      while (chain->next != mainpos_e) {
        chain = (upb_tabent*)chain->next;
        assert(chain);
      }
      chain->next = new_e;
      our_e = mainpos_e;
      our_e->next = NULL;
    }
  }
  our_e->key = key;
  our_e->val = val.val;
  assert(findentry(t, key, hash, eql) == our_e);
}

static bool rm(upb_table *t, upb_tabkey key, upb_value *val,
               upb_tabkey *removed, hashfunc_t *hash, eqlfunc_t *eql) {
  upb_tabent *chain = (upb_tabent*)hash(t, key);
  if (upb_tabent_isempty(chain)) return false;
  if (eql(chain->key, key)) {
    // Element to remove is at the head of its chain.
    t->count--;
    if (val) {
      _upb_value_setval(val, chain->val, t->ctype);
    }
    if (chain->next) {
      upb_tabent *move = (upb_tabent*)chain->next;
      *chain = *move;
      if (removed) *removed = move->key;
      move->key.num = 0;  // Make the slot empty.
    } else {
      if (removed) *removed = chain->key;
      chain->key.num = 0;  // Make the slot empty.
    }
    return true;
  } else {
    // Element to remove is either in a non-head position or not in the table.
    while (chain->next && !eql(chain->next->key, key))
      chain = (upb_tabent*)chain->next;
    if (chain->next) {
      // Found element to remove.
      if (val) {
        _upb_value_setval(val, chain->next->val, t->ctype);
      }
      upb_tabent *rm = (upb_tabent*)chain->next;
      if (removed) *removed = rm->key;
      rm->key.num = 0;
      chain->next = rm->next;
      t->count--;
      return true;
    } else {
      return false;
    }
  }
}

static size_t next(const upb_table *t, size_t i) {
  do {
    if (++i >= upb_table_size(t))
      return SIZE_MAX;
  } while(upb_tabent_isempty(&t->entries[i]));

  return i;
}

static size_t begin(const upb_table *t) {
  return next(t, -1);
}


/* upb_strtable ***************************************************************/

// A simple "subclass" of upb_table that only adds a hash function for strings.

static const upb_tabent *strhash(const upb_table *t, upb_tabkey key) {
  // Could avoid the strlen() by using a hash function that terminates on NULL.
  return t->entries + (MurmurHash2(key.str, strlen(key.str), 0) & t->mask);
}

static bool streql(upb_tabkey k1, upb_tabkey k2) {
  return strcmp(k1.str, k2.str) == 0;
}

bool upb_strtable_init(upb_strtable *t, upb_ctype_t ctype) {
  return init(&t->t, ctype, 2);
}

void upb_strtable_uninit(upb_strtable *t) {
  for (size_t i = 0; i < upb_table_size(&t->t); i++)
    free((void*)t->t.entries[i].key.str);
  uninit(&t->t);
}

bool upb_strtable_resize(upb_strtable *t, size_t size_lg2) {
  upb_strtable new_table;
  if (!init(&new_table.t, t->t.ctype, size_lg2))
    return false;
  upb_strtable_iter i;
  upb_strtable_begin(&i, t);
  for ( ; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_strtable_insert(
        &new_table, upb_strtable_iter_key(&i), upb_strtable_iter_value(&i));
  }
  upb_strtable_uninit(t);
  *t = new_table;
  return true;
}

bool upb_strtable_insert(upb_strtable *t, const char *k, upb_value v) {
  if (isfull(&t->t)) {
    // Need to resize.  New table of double the size, add old elements to it.
    if (!upb_strtable_resize(t, t->t.size_lg2 + 1)) {
      return false;
    }
  }
  if ((k = upb_strdup(k)) == NULL) return false;
  insert(&t->t, strkey(k), v, &strhash, &streql);
  return true;
}

bool upb_strtable_lookup(const upb_strtable *t, const char *key, upb_value *v) {
  return lookup(&t->t, strkey(key), v, &strhash, &streql);
}

bool upb_strtable_remove(upb_strtable *t, const char *key, upb_value *val) {
  upb_tabkey tabkey;
  if (rm(&t->t, strkey(key), val, &tabkey, &strhash, &streql)) {
    free((void*)tabkey.str);
    return true;
  } else {
    return false;
  }
}

// Iteration

static const upb_tabent *str_tabent(const upb_strtable_iter *i) {
  return &i->t->t.entries[i->index];
}

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t) {
  i->t = t;
  i->index = begin(&t->t);
}

void upb_strtable_next(upb_strtable_iter *i) {
  i->index = next(&i->t->t, i->index);
}

bool upb_strtable_done(const upb_strtable_iter *i) {
  return i->index >= upb_table_size(&i->t->t) ||
         upb_tabent_isempty(str_tabent(i));
}

const char *upb_strtable_iter_key(upb_strtable_iter *i) {
  assert(!upb_strtable_done(i));
  return str_tabent(i)->key.str;
}

upb_value upb_strtable_iter_value(const upb_strtable_iter *i) {
  assert(!upb_strtable_done(i));
  return _upb_value_val(str_tabent(i)->val, i->t->t.ctype);
}

void upb_strtable_iter_setdone(upb_strtable_iter *i) {
  i->index = SIZE_MAX;
}

bool upb_strtable_iter_isequal(const upb_strtable_iter *i1,
                               const upb_strtable_iter *i2) {
  if (upb_strtable_done(i1) && upb_strtable_done(i2))
    return true;
  return i1->t == i2->t && i1->index == i2->index;
}


/* upb_inttable ***************************************************************/

// For inttables we use a hybrid structure where small keys are kept in an
// array and large keys are put in the hash table.

static bool inteql(upb_tabkey k1, upb_tabkey k2) {
  return k1.num == k2.num;
}

static _upb_value *inttable_val(upb_inttable *t, uintptr_t key) {
  if (key < t->array_size) {
    return upb_arrhas(t->array[key]) ? (_upb_value*)&t->array[key] : NULL;
  } else {
    upb_tabent *e =
        (upb_tabent*)findentry(&t->t, upb_intkey(key), &upb_inthash, &inteql);
    return e ? &e->val : NULL;
  }
}

static const _upb_value *inttable_val_const(const upb_inttable *t,
                                            uintptr_t key) {
  return inttable_val((upb_inttable*)t, key);
}

size_t upb_inttable_count(const upb_inttable *t) {
  return t->t.count + t->array_count;
}

static void check(upb_inttable *t) {
  UPB_UNUSED(t);
#if defined(UPB_DEBUG_TABLE) && !defined(NDEBUG)
  // This check is very expensive (makes inserts/deletes O(N)).
  size_t count = 0;
  upb_inttable_iter i;
  upb_inttable_begin(&i, t);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i), count++) {
    assert(upb_inttable_lookup(t, upb_inttable_iter_key(&i), NULL));
  }
  assert(count == upb_inttable_count(t));
#endif
}

bool upb_inttable_sizedinit(upb_inttable *t, upb_ctype_t ctype,
                            size_t asize, int hsize_lg2) {
  if (!init(&t->t, ctype, hsize_lg2)) return false;
  // Always make the array part at least 1 long, so that we know key 0
  // won't be in the hash part, which simplifies things.
  t->array_size = UPB_MAX(1, asize);
  t->array_count = 0;
  size_t array_bytes = t->array_size * sizeof(upb_value);
  t->array = malloc(array_bytes);
  if (!t->array) {
    uninit(&t->t);
    return false;
  }
  memset((void*)t->array, 0xff, array_bytes);
  check(t);
  return true;
}

bool upb_inttable_init(upb_inttable *t, upb_ctype_t ctype) {
  return upb_inttable_sizedinit(t, ctype, 0, 4);
}

void upb_inttable_uninit(upb_inttable *t) {
  uninit(&t->t);
  free((void*)t->array);
}

bool upb_inttable_insert(upb_inttable *t, uintptr_t key, upb_value val) {
  assert(upb_arrhas(val.val));
  if (key < t->array_size) {
    assert(!upb_arrhas(t->array[key]));
    t->array_count++;
    ((_upb_value*)t->array)[key] = val.val;
  } else {
    if (isfull(&t->t)) {
      // Need to resize the hash part, but we re-use the array part.
      upb_table new_table;
      if (!init(&new_table, t->t.ctype, t->t.size_lg2 + 1))
        return false;
      size_t i;
      for (i = begin(&t->t); i < upb_table_size(&t->t); i = next(&t->t, i)) {
        const upb_tabent *e = &t->t.entries[i];
        upb_value v;
        _upb_value_setval(&v, e->val, t->t.ctype);
        insert(&new_table, e->key, v, &upb_inthash, &inteql);
      }

      assert(t->t.count == new_table.count);

      uninit(&t->t);
      t->t = new_table;
    }
    insert(&t->t, upb_intkey(key), val, &upb_inthash, &inteql);
  }
  check(t);
  return true;
}

bool upb_inttable_lookup(const upb_inttable *t, uintptr_t key, upb_value *v) {
  const _upb_value *table_v = inttable_val_const(t, key);
  if (!table_v) return false;
  if (v) _upb_value_setval(v, *table_v, t->t.ctype);
  return true;
}

bool upb_inttable_replace(upb_inttable *t, uintptr_t key, upb_value val) {
  _upb_value *table_v = inttable_val(t, key);
  if (!table_v) return false;
  *table_v = val.val;
  return true;
}

bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val) {
  bool success;
  if (key < t->array_size) {
    if (upb_arrhas(t->array[key])) {
      t->array_count--;
      if (val) {
        _upb_value_setval(val, t->array[key], t->t.ctype);
      }
      ((upb_value*)t->array)[key] = upb_value_uint64(-1);
      success = true;
    } else {
      success = false;
    }
  } else {
    upb_tabkey removed;
    success = rm(&t->t, upb_intkey(key), val, &removed, &upb_inthash, &inteql);
  }
  check(t);
  return success;
}

bool upb_inttable_push(upb_inttable *t, upb_value val) {
  return upb_inttable_insert(t, upb_inttable_count(t), val);
}

upb_value upb_inttable_pop(upb_inttable *t) {
  upb_value val;
  bool ok = upb_inttable_remove(t, upb_inttable_count(t) - 1, &val);
  UPB_ASSERT_VAR(ok, ok);
  return val;
}

bool upb_inttable_insertptr(upb_inttable *t, const void *key, upb_value val) {
  return upb_inttable_insert(t, (uintptr_t)key, val);
}

bool upb_inttable_lookupptr(const upb_inttable *t, const void *key,
                            upb_value *v) {
  return upb_inttable_lookup(t, (uintptr_t)key, v);
}

bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val) {
  return upb_inttable_remove(t, (uintptr_t)key, val);
}

void upb_inttable_compact(upb_inttable *t) {
  // Create a power-of-two histogram of the table keys.
  int counts[UPB_MAXARRSIZE + 1] = {0};
  uintptr_t max_key = 0;
  upb_inttable_iter i;
  upb_inttable_begin(&i, t);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    uintptr_t key = upb_inttable_iter_key(&i);
    if (key > max_key) {
      max_key = key;
    }
    counts[log2ceil(key)]++;
  }

  int arr_size;
  int arr_count = upb_inttable_count(t);

  if (upb_inttable_count(t) >= max_key * MIN_DENSITY) {
    // We can put 100% of the entries in the array part.
    arr_size = max_key + 1;
  } else {
    // Find the largest power of two that satisfies the MIN_DENSITY definition.
    for (int size_lg2 = ARRAY_SIZE(counts) - 1; size_lg2 > 1; size_lg2--) {
      arr_size = 1 << size_lg2;
      arr_count -= counts[size_lg2];
      if (arr_count >= arr_size * MIN_DENSITY) {
        break;
      }
    }
  }

  // Array part must always be at least 1 entry large to catch lookups of key
  // 0.  Key 0 must always be in the array part because "0" in the hash part
  // denotes an empty entry.
  arr_size = UPB_MAX(arr_size, 1);

  // Insert all elements into new, perfectly-sized table.
  int hash_count = upb_inttable_count(t) - arr_count;
  int hash_size = hash_count ? (hash_count / MAX_LOAD) + 1 : 0;
  int hashsize_lg2 = log2ceil(hash_size);
  assert(hash_count >= 0);

  upb_inttable new_t;
  upb_inttable_sizedinit(&new_t, t->t.ctype, arr_size, hashsize_lg2);
  upb_inttable_begin(&i, t);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    uintptr_t k = upb_inttable_iter_key(&i);
    upb_inttable_insert(&new_t, k, upb_inttable_iter_value(&i));
  }
  assert(new_t.array_size == arr_size);
  assert(new_t.t.size_lg2 == hashsize_lg2);
  upb_inttable_uninit(t);
  *t = new_t;
}

// Iteration.

static const upb_tabent *int_tabent(const upb_inttable_iter *i) {
  assert(!i->array_part);
  return &i->t->t.entries[i->index];
}

static _upb_value int_arrent(const upb_inttable_iter *i) {
  assert(i->array_part);
  return i->t->array[i->index];
}

void upb_inttable_begin(upb_inttable_iter *i, const upb_inttable *t) {
  i->t = t;
  i->index = -1;
  i->array_part = true;
  upb_inttable_next(i);
}

void upb_inttable_next(upb_inttable_iter *iter) {
  const upb_inttable *t = iter->t;
  if (iter->array_part) {
    while (++iter->index < t->array_size) {
      if (upb_arrhas(int_arrent(iter))) {
        return;
      }
    }
    iter->array_part = false;
    iter->index = begin(&t->t);
  } else {
    iter->index = next(&t->t, iter->index);
  }
}

bool upb_inttable_done(const upb_inttable_iter *i) {
  if (i->array_part) {
    return i->index >= i->t->array_size ||
           !upb_arrhas(int_arrent(i));
  } else {
    return i->index >= upb_table_size(&i->t->t) ||
           upb_tabent_isempty(int_tabent(i));
  }
}

uintptr_t upb_inttable_iter_key(const upb_inttable_iter *i) {
  assert(!upb_inttable_done(i));
  return i->array_part ? i->index : int_tabent(i)->key.num;
}

upb_value upb_inttable_iter_value(const upb_inttable_iter *i) {
  assert(!upb_inttable_done(i));
  return _upb_value_val(
      i->array_part ? i->t->array[i->index] : int_tabent(i)->val,
      i->t->t.ctype);
}

void upb_inttable_iter_setdone(upb_inttable_iter *i) {
  i->index = SIZE_MAX;
  i->array_part = false;
}

bool upb_inttable_iter_isequal(const upb_inttable_iter *i1,
                                          const upb_inttable_iter *i2) {
  if (upb_inttable_done(i1) && upb_inttable_done(i2))
    return true;
  return i1->t == i2->t && i1->index == i2->index &&
         i1->array_part == i2->array_part;
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
uint32_t MurmurHash2(const void *key, size_t len, uint32_t seed) {
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

uint32_t MurmurHash2(const void * key, size_t len, uint32_t seed) {
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
