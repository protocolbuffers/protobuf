// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/*
 * upb_table Implementation
 *
 * Implementation is heavily inspired by Lua's ltable.c.
 */

#include "upb/hash/common.h"

#include <stdint.h>
#include <string.h>

#include "upb/base/internal/log2.h"
#include "upb/base/string_view.h"
#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

#define UPB_MAXARRSIZE 16  // 2**16 = 64k.

// From Chromium.
#define ARRAY_SIZE(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

/* The minimum utilization of the array part of a mixed hash/array table.  This
 * is a speed/memory-usage tradeoff (though it's not straightforward because of
 * cache effects).  The lower this is, the more memory we'll use. */
static const double MIN_DENSITY = 0.1;

#if defined(__has_builtin)
#if __has_builtin(__builtin_popcount)
#define UPB_FAST_POPCOUNT32(i) __builtin_popcount(i)
#endif
#elif defined(__GNUC__)
#define UPB_FAST_POPCOUNT32(i) __builtin_popcount(i)
#elif defined(_MSC_VER)
#define UPB_FAST_POPCOUNT32(i) __popcnt(i)
#endif

UPB_INLINE int _upb_popcnt32(uint32_t i) {
#ifdef UPB_FAST_POPCOUNT32
  return UPB_FAST_POPCOUNT32(i);
#else
  int count = 0;
  while (i != 0) {
    count += i & 1;
    i >>= 1;
  }
  return count;
#endif
}

#undef UPB_FAST_POPCOUNT32

UPB_INLINE uint8_t _upb_log2_table_size(upb_table* t) {
  return _upb_popcnt32(t->mask);
}

static bool is_pow2(uint64_t v) { return v == 0 || (v & (v - 1)) == 0; }

static int log2ceil(uint64_t v) {
  int ret = 0;
  bool pow2 = is_pow2(v);
  while (v >>= 1) ret++;
  ret = pow2 ? ret : ret + 1;  // Ceiling.
  return UPB_MIN(UPB_MAXARRSIZE, ret);
}

/* A type to represent the lookup key of either a strtable or an inttable. */
typedef union {
  uintptr_t num;
  upb_StringView str;
} lookupkey_t;

static lookupkey_t strkey2(const char* str, size_t len) {
  return (lookupkey_t){.str = upb_StringView_FromDataAndSize(str, len)};
}

static lookupkey_t intkey(uintptr_t key) { return (lookupkey_t){.num = key}; }

typedef uint32_t hashfunc_t(upb_key key);
typedef bool eqlfunc_t(upb_key k1, lookupkey_t k2);

/* Base table (shared code) ***************************************************/

static uint32_t upb_inthash(uintptr_t key) {
  if (sizeof(uintptr_t) == 8) {
    return (uint32_t)key ^ (uint32_t)(key >> 32);
  } else {
    UPB_ASSERT(sizeof(uintptr_t) == 4);
    return (uint32_t)key;
  }
}

static const upb_tabent* upb_getentry(const upb_table* t, uint32_t hash) {
  return t->entries + (hash & t->mask);
}

static bool isfull(upb_table* t) {
  uint32_t size = upb_table_size(t);
  // 0.875 load factor
  return t->count == (size - (size >> 3));
}

static bool init(upb_table* t, uint8_t size_lg2, upb_Arena* a) {
  if (size_lg2 >= 32) {
    return false;
  }
  t->count = 0;
  uint32_t size = 1 << size_lg2;
  t->mask = size - 1;  // 0 mask if size_lg2 is 0
  if (upb_table_size(t) > (SIZE_MAX / sizeof(upb_tabent))) {
    return false;
  }
  size_t bytes = upb_table_size(t) * sizeof(upb_tabent);
  if (bytes > 0) {
    t->entries = upb_Arena_Malloc(a, bytes);
    if (!t->entries) return false;
    memset(t->entries, 0, bytes);
  } else {
    t->entries = NULL;
  }
  return true;
}

static upb_tabent* emptyent(upb_table* t, upb_tabent* e) {
  upb_tabent* begin = t->entries;
  upb_tabent* end = begin + upb_table_size(t);
  for (e = e + 1; e < end; e++) {
    if (upb_tabent_isempty(e)) return e;
  }
  for (e = begin; e < end; e++) {
    if (upb_tabent_isempty(e)) return e;
  }
  UPB_ASSERT(false);
  return NULL;
}

static upb_tabent* getentry_mutable(upb_table* t, uint32_t hash) {
  return (upb_tabent*)upb_getentry(t, hash);
}

static const upb_tabent* findentry(const upb_table* t, lookupkey_t key,
                                   uint32_t hash, eqlfunc_t* eql) {
  const upb_tabent* e;

  if (t->count == 0) return NULL;
  e = upb_getentry(t, hash);
  if (upb_tabent_isempty(e)) return NULL;
  while (1) {
    if (eql(e->key, key)) return e;
    if ((e = e->next) == NULL) return NULL;
  }
}

static upb_tabent* findentry_mutable(upb_table* t, lookupkey_t key,
                                     uint32_t hash, eqlfunc_t* eql) {
  return (upb_tabent*)findentry(t, key, hash, eql);
}

static bool lookup(const upb_table* t, lookupkey_t key, upb_value* v,
                   uint32_t hash, eqlfunc_t* eql) {
  const upb_tabent* e = findentry(t, key, hash, eql);
  if (e) {
    if (v) *v = e->val;
    return true;
  } else {
    return false;
  }
}

/* The given key must not already exist in the table. */
static void insert(upb_table* t, lookupkey_t key, upb_key tabkey, upb_value val,
                   uint32_t hash, hashfunc_t* hashfunc, eqlfunc_t* eql) {
  upb_tabent* mainpos_e;
  upb_tabent* our_e;

  UPB_ASSERT(findentry(t, key, hash, eql) == NULL);

  t->count++;
  mainpos_e = getentry_mutable(t, hash);
  our_e = mainpos_e;

  if (upb_tabent_isempty(mainpos_e)) {
    /* Our main position is empty; use it. */
    our_e->next = NULL;
  } else {
    /* Collision. */
    upb_tabent* new_e = emptyent(t, mainpos_e);
    /* Head of collider's chain. */
    upb_tabent* chain = getentry_mutable(t, hashfunc(mainpos_e->key));
    if (chain == mainpos_e) {
      /* Existing ent is in its main position (it has the same hash as us, and
       * is the head of our chain).  Insert to new ent and append to this chain.
       */
      new_e->next = mainpos_e->next;
      mainpos_e->next = new_e;
      our_e = new_e;
    } else {
      /* Existing ent is not in its main position (it is a node in some other
       * chain).  This implies that no existing ent in the table has our hash.
       * Evict it (updating its chain) and use its ent for head of our chain. */
      *new_e = *mainpos_e; /* copies next. */
      while (chain->next != mainpos_e) {
        chain = (upb_tabent*)chain->next;
        UPB_ASSERT(chain);
      }
      chain->next = new_e;
      our_e = mainpos_e;
      our_e->next = NULL;
    }
  }
  our_e->key = tabkey;
  our_e->val = val;
  UPB_ASSERT(findentry(t, key, hash, eql) == our_e);
}

static bool rm(upb_table* t, lookupkey_t key, upb_value* val, uint32_t hash,
               eqlfunc_t* eql) {
  upb_tabent* chain = getentry_mutable(t, hash);
  if (upb_tabent_isempty(chain)) return false;
  if (eql(chain->key, key)) {
    /* Element to remove is at the head of its chain. */
    t->count--;
    if (val) *val = chain->val;
    if (chain->next) {
      upb_tabent* move = (upb_tabent*)chain->next;
      *chain = *move;
      move->key = upb_key_empty();
    } else {
      chain->key = upb_key_empty();
    }
    return true;
  } else {
    /* Element to remove is either in a non-head position or not in the
     * table. */
    while (chain->next && !eql(chain->next->key, key)) {
      chain = (upb_tabent*)chain->next;
    }
    if (chain->next) {
      /* Found element to remove. */
      upb_tabent* rm = (upb_tabent*)chain->next;
      t->count--;
      if (val) *val = chain->next->val;
      rm->key = upb_key_empty();
      chain->next = rm->next;
      return true;
    } else {
      /* Element to remove is not in the table. */
      return false;
    }
  }
}

static size_t next(const upb_table* t, size_t i) {
  do {
    if (++i >= upb_table_size(t)) return SIZE_MAX - 1; /* Distinct from -1. */
  } while (upb_tabent_isempty(&t->entries[i]));

  return i;
}

static size_t begin(const upb_table* t) { return next(t, -1); }

/* upb_strtable ***************************************************************/

// A simple "subclass" of upb_table that only adds a hash function for strings.

static upb_SizePrefixString* upb_SizePrefixString_Copy(upb_StringView s,
                                                       upb_Arena* a) {
  // A 2GB string will fail at serialization time, but we accept up to 4GB in
  // memory here.
  if (s.size > UINT32_MAX) return NULL;
  upb_SizePrefixString* str =
      upb_Arena_Malloc(a, sizeof(uint32_t) + s.size + 1);
  if (str == NULL) return NULL;
  str->size = s.size;
  char* data = (char*)str->data;
  if (s.size) memcpy(data, s.data, s.size);
  data[s.size] = '\0';
  return str;
}

/* Adapted from ABSL's wyhash. */

static uint64_t UnalignedLoad64(const void* p) {
  uint64_t val;
  memcpy(&val, p, 8);
  return val;
}

static uint32_t UnalignedLoad32(const void* p) {
  uint32_t val;
  memcpy(&val, p, 4);
  return val;
}

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

/* Computes a * b, returning the low 64 bits of the result and storing the high
 * 64 bits in |*high|. */
static uint64_t upb_umul128(uint64_t v0, uint64_t v1, uint64_t* out_high) {
#ifdef __SIZEOF_INT128__
  __uint128_t p = v0;
  p *= v1;
  *out_high = (uint64_t)(p >> 64);
  return (uint64_t)p;
#elif defined(_MSC_VER) && defined(_M_X64)
  return _umul128(v0, v1, out_high);
#else
  uint64_t a32 = v0 >> 32;
  uint64_t a00 = v0 & 0xffffffff;
  uint64_t b32 = v1 >> 32;
  uint64_t b00 = v1 & 0xffffffff;
  uint64_t high = a32 * b32;
  uint64_t low = a00 * b00;
  uint64_t mid1 = a32 * b00;
  uint64_t mid2 = a00 * b32;
  low += (mid1 << 32) + (mid2 << 32);
  // Omit carry bit, for mixing we do not care about exact numerical precision.
  high += (mid1 >> 32) + (mid2 >> 32);
  *out_high = high;
  return low;
#endif
}

static uint64_t WyhashMix(uint64_t v0, uint64_t v1) {
  uint64_t high;
  uint64_t low = upb_umul128(v0, v1, &high);
  return low ^ high;
}

static uint64_t Wyhash(const void* data, size_t len, uint64_t seed,
                       const uint64_t salt[]) {
  const uint8_t* ptr = (const uint8_t*)data;
  uint64_t starting_length = (uint64_t)len;
  uint64_t current_state = seed ^ salt[0];

  if (len > 64) {
    // If we have more than 64 bytes, we're going to handle chunks of 64
    // bytes at a time. We're going to build up two separate hash states
    // which we will then hash together.
    uint64_t duplicated_state = current_state;

    do {
      uint64_t a = UnalignedLoad64(ptr);
      uint64_t b = UnalignedLoad64(ptr + 8);
      uint64_t c = UnalignedLoad64(ptr + 16);
      uint64_t d = UnalignedLoad64(ptr + 24);
      uint64_t e = UnalignedLoad64(ptr + 32);
      uint64_t f = UnalignedLoad64(ptr + 40);
      uint64_t g = UnalignedLoad64(ptr + 48);
      uint64_t h = UnalignedLoad64(ptr + 56);

      uint64_t cs0 = WyhashMix(a ^ salt[1], b ^ current_state);
      uint64_t cs1 = WyhashMix(c ^ salt[2], d ^ current_state);
      current_state = (cs0 ^ cs1);

      uint64_t ds0 = WyhashMix(e ^ salt[3], f ^ duplicated_state);
      uint64_t ds1 = WyhashMix(g ^ salt[4], h ^ duplicated_state);
      duplicated_state = (ds0 ^ ds1);

      ptr += 64;
      len -= 64;
    } while (len > 64);

    current_state = current_state ^ duplicated_state;
  }

  // We now have a data `ptr` with at most 64 bytes and the current state
  // of the hashing state machine stored in current_state.
  while (len > 16) {
    uint64_t a = UnalignedLoad64(ptr);
    uint64_t b = UnalignedLoad64(ptr + 8);

    current_state = WyhashMix(a ^ salt[1], b ^ current_state);

    ptr += 16;
    len -= 16;
  }

  // We now have a data `ptr` with at most 16 bytes.
  uint64_t a = 0;
  uint64_t b = 0;
  if (len > 8) {
    // When we have at least 9 and at most 16 bytes, set A to the first 64
    // bits of the input and B to the last 64 bits of the input. Yes, they will
    // overlap in the middle if we are working with less than the full 16
    // bytes.
    a = UnalignedLoad64(ptr);
    b = UnalignedLoad64(ptr + len - 8);
  } else if (len > 3) {
    // If we have at least 4 and at most 8 bytes, set A to the first 32
    // bits and B to the last 32 bits.
    a = UnalignedLoad32(ptr);
    b = UnalignedLoad32(ptr + len - 4);
  } else if (len > 0) {
    // If we have at least 1 and at most 3 bytes, read all of the provided
    // bits into A, with some adjustments.
    a = ((ptr[0] << 16) | (ptr[len >> 1] << 8) | ptr[len - 1]);
    b = 0;
  } else {
    a = 0;
    b = 0;
  }

  uint64_t w = WyhashMix(a ^ salt[1], b ^ current_state);
  uint64_t z = salt[1] ^ starting_length;
  return WyhashMix(w, z);
}

const uint64_t kWyhashSalt[5] = {
    0x243F6A8885A308D3ULL, 0x13198A2E03707344ULL, 0xA4093822299F31D0ULL,
    0x082EFA98EC4E6C89ULL, 0x452821E638D01377ULL,
};

uint32_t _upb_Hash(const void* p, size_t n, uint64_t seed) {
  return Wyhash(p, n, seed, kWyhashSalt);
}

static const void* const _upb_seed;

// Returns a random seed for upb's hash function. This does not provide
// high-quality randomness, but it should be enough to prevent unit tests from
// relying on a deterministic map ordering. By returning the address of a
// variable, we are able to get some randomness for free provided that ASLR is
// enabled.
static uint64_t _upb_Seed(void) { return (uint64_t)&_upb_seed; }

static uint32_t _upb_Hash_NoSeed(const char* p, size_t n) {
  return _upb_Hash(p, n, _upb_Seed());
}

static uint32_t strhash(upb_key key) {
  return _upb_Hash_NoSeed(key.str->data, key.str->size);
}

static bool streql(upb_key k1, lookupkey_t k2) {
  const upb_SizePrefixString* k1s = k1.str;
  const upb_StringView k2s = k2.str;
  return k1s->size == k2s.size &&
         (k1s->size == 0 || memcmp(k1s->data, k2s.data, k1s->size) == 0);
}

/** Calculates the number of entries required to hold an expected number of
 * values, within the table's load factor. */
static size_t _upb_entries_needed_for(size_t expected_size) {
  size_t need_entries = expected_size + 1 + expected_size / 7;
  UPB_ASSERT(need_entries - (need_entries >> 3) >= expected_size);
  return need_entries;
}

bool upb_strtable_init(upb_strtable* t, size_t expected_size, upb_Arena* a) {
  int size_lg2 = upb_Log2Ceiling(_upb_entries_needed_for(expected_size));
  return init(&t->t, size_lg2, a);
}

void upb_strtable_clear(upb_strtable* t) {
  size_t bytes = upb_table_size(&t->t) * sizeof(upb_tabent);
  t->t.count = 0;
  memset((char*)t->t.entries, 0, bytes);
}

bool upb_strtable_resize(upb_strtable* t, size_t size_lg2, upb_Arena* a) {
  upb_strtable new_table;
  if (!init(&new_table.t, size_lg2, a)) return false;

  intptr_t iter = UPB_STRTABLE_BEGIN;
  upb_StringView sv;
  upb_value val;
  while (upb_strtable_next2(t, &sv, &val, &iter)) {
    // Unlike normal insert, does not copy string data or possibly reallocate
    // the table
    // The data pointer used in the table is guaranteed to point at a
    // upb_SizePrefixString, we just need to back up by the size of the uint32_t
    // length prefix.
    const upb_SizePrefixString* keystr =
        (const upb_SizePrefixString*)(sv.data - sizeof(uint32_t));
    UPB_ASSERT(keystr->data == sv.data);
    UPB_ASSERT(keystr->size == sv.size);

    lookupkey_t lookupkey = {.str = sv};
    upb_key tabkey = {.str = keystr};
    uint32_t hash = _upb_Hash_NoSeed(sv.data, sv.size);
    insert(&new_table.t, lookupkey, tabkey, val, hash, &strhash, &streql);
  }
  *t = new_table;
  return true;
}

bool upb_strtable_insert(upb_strtable* t, const char* k, size_t len,
                         upb_value v, upb_Arena* a) {
  if (isfull(&t->t)) {
    /* Need to resize.  New table of double the size, add old elements to it. */
    if (!upb_strtable_resize(t, _upb_log2_table_size(&t->t) + 1, a)) {
      return false;
    }
  }

  upb_StringView sv = upb_StringView_FromDataAndSize(k, len);
  upb_SizePrefixString* size_prefix_string = upb_SizePrefixString_Copy(sv, a);
  if (!size_prefix_string) return false;

  lookupkey_t lookupkey = {.str = sv};
  upb_key key = {.str = size_prefix_string};
  uint32_t hash = _upb_Hash_NoSeed(k, len);
  insert(&t->t, lookupkey, key, v, hash, &strhash, &streql);
  return true;
}

bool upb_strtable_lookup2(const upb_strtable* t, const char* key, size_t len,
                          upb_value* v) {
  uint32_t hash = _upb_Hash_NoSeed(key, len);
  return lookup(&t->t, strkey2(key, len), v, hash, &streql);
}

bool upb_strtable_remove2(upb_strtable* t, const char* key, size_t len,
                          upb_value* val) {
  uint32_t hash = _upb_Hash_NoSeed(key, len);
  return rm(&t->t, strkey2(key, len), val, hash, &streql);
}

/* Iteration */

void upb_strtable_begin(upb_strtable_iter* i, const upb_strtable* t) {
  i->t = t;
  i->index = begin(&t->t);
}

void upb_strtable_next(upb_strtable_iter* i) {
  i->index = next(&i->t->t, i->index);
}

bool upb_strtable_done(const upb_strtable_iter* i) {
  if (!i->t) return true;
  return i->index >= upb_table_size(&i->t->t) ||
         upb_tabent_isempty(str_tabent(i));
}

upb_StringView upb_strtable_iter_key(const upb_strtable_iter* i) {
  UPB_ASSERT(!upb_strtable_done(i));
  return upb_key_strview(str_tabent(i)->key);
}

upb_value upb_strtable_iter_value(const upb_strtable_iter* i) {
  UPB_ASSERT(!upb_strtable_done(i));
  return str_tabent(i)->val;
}

void upb_strtable_iter_setdone(upb_strtable_iter* i) {
  i->t = NULL;
  i->index = SIZE_MAX;
}

bool upb_strtable_iter_isequal(const upb_strtable_iter* i1,
                               const upb_strtable_iter* i2) {
  if (upb_strtable_done(i1) && upb_strtable_done(i2)) return true;
  return i1->t == i2->t && i1->index == i2->index;
}

bool upb_strtable_next2(const upb_strtable* t, upb_StringView* key,
                        upb_value* val, intptr_t* iter) {
  size_t tab_idx = next(&t->t, *iter);
  if (tab_idx < upb_table_size(&t->t)) {
    upb_tabent* ent = &t->t.entries[tab_idx];
    *key = upb_key_strview(ent->key);
    *val = ent->val;
    *iter = tab_idx;
    return true;
  }

  return false;
}

void upb_strtable_removeiter(upb_strtable* t, intptr_t* iter) {
  intptr_t i = *iter;
  upb_tabent* ent = &t->t.entries[i];
  upb_tabent* prev = NULL;

  // Linear search, not great.
  upb_tabent* end = &t->t.entries[upb_table_size(&t->t)];
  for (upb_tabent* e = t->t.entries; e != end; e++) {
    if (e->next == ent) {
      prev = e;
      break;
    }
  }

  if (prev) {
    prev->next = ent->next;
  }

  t->t.count--;
  ent->key = upb_key_empty();
  ent->next = NULL;
}

void upb_strtable_setentryvalue(upb_strtable* t, intptr_t iter, upb_value v) {
  t->t.entries[iter].val = v;
}

/* upb_inttable ***************************************************************/

/* For inttables we use a hybrid structure where small keys are kept in an
 * array and large keys are put in the hash table. */

// The sentinel value used in the dense array part. Note that callers must
// ensure that inttable is never used with a value of this sentinel type
// (pointers and u32 values will never be; i32 needs to be handled carefully
// to avoid sign-extending into this value).
static const upb_value kInttableSentinel = {.val = UINT64_MAX};
static uint32_t presence_mask_arr_size(uint32_t array_size) {
  return (array_size + 7) / 8;  // sizeof(uint8_t) is always 1.
}

static uint32_t inthash(upb_key key) { return upb_inthash(key.num); }

static bool inteql(upb_key k1, lookupkey_t k2) { return k1.num == k2.num; }

static upb_value* mutable_array(upb_inttable* t) {
  return (upb_value*)t->array;
}

static const upb_value* inttable_array_get(const upb_inttable* t,
                                           uintptr_t key) {
  UPB_ASSERT(key < t->array_size);
  const upb_value* val = &t->array[key];
  return upb_inttable_arrhas(t, key) ? val : NULL;
}

static upb_value* inttable_val(upb_inttable* t, uintptr_t key) {
  if (key < t->array_size) {
    return (upb_value*)inttable_array_get(t, key);
  } else {
    upb_tabent* e =
        findentry_mutable(&t->t, intkey(key), upb_inthash(key), &inteql);
    return e ? &e->val : NULL;
  }
}

static const upb_value* inttable_val_const(const upb_inttable* t,
                                           uintptr_t key) {
  return inttable_val((upb_inttable*)t, key);
}

size_t upb_inttable_count(const upb_inttable* t) {
  return t->t.count + t->array_count;
}

static void check(upb_inttable* t) {
  UPB_UNUSED(t);
#if defined(UPB_DEBUG_TABLE) && !defined(NDEBUG)
  {
    // This check is very expensive (makes inserts/deletes O(N)).
    size_t count = 0;
    intptr_t iter = UPB_INTTABLE_BEGIN;
    uintptr_t key;
    upb_value val;
    while (upb_inttable_next(t, &key, &val, &iter)) {
      UPB_ASSERT(upb_inttable_lookup(t, key, NULL));
    }
    UPB_ASSERT(count == upb_inttable_count(t));
  }
#endif
}

bool upb_inttable_sizedinit(upb_inttable* t, uint32_t asize, int hsize_lg2,
                            upb_Arena* a) {
  if (!init(&t->t, hsize_lg2, a)) return false;
  /* Always make the array part at least 1 long, so that we know key 0
   * won't be in the hash part, which simplifies things. */
  t->array_size = UPB_MAX(1, asize);
  t->array_count = 0;
#if UINT32_MAX >= SIZE_MAX
  if (UPB_UNLIKELY(SIZE_MAX / sizeof(upb_value) < t->array_size)) {
    return false;
  }
#endif

  // Allocate the array part and the presence mask array in one allocation.
  size_t array_bytes = t->array_size * sizeof(upb_value);
  uint32_t presence_bytes = presence_mask_arr_size(t->array_size);
  uintptr_t total_bytes = array_bytes + presence_bytes;
  if (UPB_UNLIKELY(total_bytes > SIZE_MAX)) {
    return false;
  }
  void* alloc = upb_Arena_Malloc(a, total_bytes);
  if (!alloc) {
    return false;
  }
  t->array = alloc;
  memset(mutable_array(t), 0xff, array_bytes);
  t->presence_mask = (uint8_t*)alloc + array_bytes;
  memset((uint8_t*)t->presence_mask, 0, presence_bytes);

  check(t);
  return true;
}

bool upb_inttable_init(upb_inttable* t, upb_Arena* a) {
  // The init size of the table part to match that of strtable.
  return upb_inttable_sizedinit(t, 0, 3, a);
}

bool upb_inttable_insert(upb_inttable* t, uintptr_t key, upb_value val,
                         upb_Arena* a) {
  if (key < t->array_size) {
    UPB_ASSERT(!upb_inttable_arrhas(t, key));
    t->array_count++;
    mutable_array(t)[key] = val;
    ((uint8_t*)t->presence_mask)[key / 8] |= (1 << (key % 8));
  } else {
    if (isfull(&t->t)) {
      /* Need to resize the hash part, but we re-use the array part. */
      size_t i;
      upb_table new_table;

      if (!init(&new_table, _upb_log2_table_size(&t->t) + 1, a)) {
        return false;
      }

      for (i = begin(&t->t); i < upb_table_size(&t->t); i = next(&t->t, i)) {
        const upb_tabent* e = &t->t.entries[i];
        insert(&new_table, intkey(e->key.num), e->key, e->val, inthash(e->key),
               &inthash, &inteql);
      }

      UPB_ASSERT(t->t.count == new_table.count);

      t->t = new_table;
    }
    upb_key tabkey = {.num = key};
    insert(&t->t, intkey(key), tabkey, val, upb_inthash(key), &inthash,
           &inteql);
  }
  check(t);
  return true;
}

bool upb_inttable_lookup(const upb_inttable* t, uintptr_t key, upb_value* v) {
  const upb_value* table_v = inttable_val_const(t, key);
  if (!table_v) return false;
  if (v) *v = *table_v;
  return true;
}

bool upb_inttable_replace(upb_inttable* t, uintptr_t key, upb_value val) {
  upb_value* table_v = inttable_val(t, key);
  if (!table_v) return false;
  *table_v = val;
  return true;
}

bool upb_inttable_remove(upb_inttable* t, uintptr_t key, upb_value* val) {
  bool success;
  if (key < t->array_size) {
    if (upb_inttable_arrhas(t, key)) {
      t->array_count--;
      if (val) {
        *val = t->array[key];
      }
      mutable_array(t)[key] = kInttableSentinel;
      ((uint8_t*)t->presence_mask)[key / 8] &= ~(1 << (key % 8));
      success = true;
    } else {
      success = false;
    }
  } else {
    success = rm(&t->t, intkey(key), val, upb_inthash(key), &inteql);
  }
  check(t);
  return success;
}

bool upb_inttable_compact(upb_inttable* t, upb_Arena* a) {
  /* A power-of-two histogram of the table keys. */
  uint32_t counts[UPB_MAXARRSIZE + 1] = {0};

  /* The max key in each bucket. */
  uintptr_t max[UPB_MAXARRSIZE + 1] = {0};

  {
    intptr_t iter = UPB_INTTABLE_BEGIN;
    uintptr_t key;
    upb_value val;
    while (upb_inttable_next(t, &key, &val, &iter)) {
      int bucket = log2ceil(key);
      max[bucket] = UPB_MAX(max[bucket], key);
      counts[bucket]++;
    }
  }

  /* Find the largest power of two that satisfies the MIN_DENSITY
   * definition (while actually having some keys). */
  uint32_t arr_count = upb_inttable_count(t);

  // Scan all buckets except capped bucket
  int size_lg2 = ARRAY_SIZE(counts) - 1;
  for (; size_lg2 > 0; size_lg2--) {
    if (counts[size_lg2] == 0) {
      /* We can halve again without losing any entries. */
      continue;
    } else if (arr_count >= (1 << size_lg2) * MIN_DENSITY) {
      break;
    }

    arr_count -= counts[size_lg2];
  }

  UPB_ASSERT(arr_count <= upb_inttable_count(t));

  upb_inttable new_t;
  {
    /* Insert all elements into new, perfectly-sized table. */
    uintptr_t arr_size = max[size_lg2] + 1; /* +1 so arr[max] will fit. */
    uint32_t hash_count = upb_inttable_count(t) - arr_count;
    size_t hash_size = hash_count ? _upb_entries_needed_for(hash_count) : 0;
    int hashsize_lg2 = log2ceil(hash_size);

    if (!upb_inttable_sizedinit(&new_t, arr_size, hashsize_lg2, a)) {
      return false;
    }

    {
      intptr_t iter = UPB_INTTABLE_BEGIN;
      uintptr_t key;
      upb_value val;
      while (upb_inttable_next(t, &key, &val, &iter)) {
        upb_inttable_insert(&new_t, key, val, a);
      }
    }

    UPB_ASSERT(new_t.array_size == arr_size);
  }
  *t = new_t;
  return true;
}

void upb_inttable_clear(upb_inttable* t) {
  // Clear the array part.
  size_t array_bytes = t->array_size * sizeof(upb_value);
  t->array_count = 0;
  // Clear the array by setting all bits to 1, as UINT64_MAX is the sentinel
  // value for an empty array.
  memset(mutable_array(t), 0xff, array_bytes);
  // Clear the presence mask array.
  memset((uint8_t*)t->presence_mask, 0, presence_mask_arr_size(t->array_size));
  // Clear the table part.
  size_t bytes = upb_table_size(&t->t) * sizeof(upb_tabent);
  t->t.count = 0;
  memset((char*)t->t.entries, 0, bytes);
}

// Iteration.

bool upb_inttable_next(const upb_inttable* t, uintptr_t* key, upb_value* val,
                       intptr_t* iter) {
  intptr_t i = *iter;
  if ((size_t)(i + 1) <= t->array_size) {
    while ((size_t)++i < t->array_size) {
      const upb_value* ent = inttable_array_get(t, i);
      if (ent) {
        *key = i;
        *val = *ent;
        *iter = i;
        return true;
      }
    }
    i--;  // Back up to exactly one position before the start of the table.
  }

  size_t tab_idx = next(&t->t, i - t->array_size);
  if (tab_idx < upb_table_size(&t->t)) {
    upb_tabent* ent = &t->t.entries[tab_idx];
    *key = ent->key.num;
    *val = ent->val;
    *iter = tab_idx + t->array_size;
    return true;
  } else {
    // We should set the iterator any way. When we are done, the iterator value
    // is invalidated. `upb_inttable_done` will check on the iterator value to
    // determine if the iteration is done.
    *iter = INTPTR_MAX - 1;  // To disambiguate from UPB_INTTABLE_BEGIN, to
                             // match the behavior of `upb_strtable_iter`.
    return false;
  }
}

void upb_inttable_removeiter(upb_inttable* t, intptr_t* iter) {
  intptr_t i = *iter;
  if ((size_t)i < t->array_size) {
    t->array_count--;
    mutable_array(t)[i].val = -1;
  } else {
    upb_tabent* ent = &t->t.entries[i - t->array_size];
    upb_tabent* prev = NULL;

    // Linear search, not great.
    upb_tabent* end = &t->t.entries[upb_table_size(&t->t)];
    for (upb_tabent* e = t->t.entries; e != end; e++) {
      if (e->next == ent) {
        prev = e;
        break;
      }
    }

    if (prev) {
      prev->next = ent->next;
    }

    t->t.count--;
    ent->key = upb_key_empty();
    ent->next = NULL;
  }
}

void upb_inttable_setentryvalue(upb_inttable* t, intptr_t iter, upb_value v) {
  if ((size_t)iter < t->array_size) {
    mutable_array(t)[iter] = v;
  } else {
    upb_tabent* ent = &t->t.entries[iter - t->array_size];
    ent->val = v;
  }
}

bool upb_inttable_done(const upb_inttable* t, intptr_t iter) {
  if ((uintptr_t)iter >= t->array_size + upb_table_size(&t->t)) {
    return true;
  } else if ((size_t)iter < t->array_size) {
    return !upb_inttable_arrhas(t, iter);
  } else {
    return upb_tabent_isempty(&t->t.entries[iter - t->array_size]);
  }
}

uintptr_t upb_inttable_iter_key(const upb_inttable* t, intptr_t iter) {
  UPB_ASSERT(!upb_inttable_done(t, iter));
  return (size_t)iter < t->array_size
             ? iter
             : t->t.entries[iter - t->array_size].key.num;
}

upb_value upb_inttable_iter_value(const upb_inttable* t, intptr_t iter) {
  UPB_ASSERT(!upb_inttable_done(t, iter));
  if ((size_t)iter < t->array_size) {
    return t->array[iter];
  } else {
    return t->t.entries[iter - t->array_size].val;
  }
}
