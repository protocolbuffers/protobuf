/*
** upb_table Implementation
**
** Implementation is heavily inspired by Lua's ltable.c.
*/

#include "upb/table.int.h"

#include <string.h>

#include "upb/port_def.inc"

#define UPB_MAXARRSIZE 16  /* 64k. */

/* From Chromium. */
#define ARRAY_SIZE(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static void upb_check_alloc(upb_table *t, upb_alloc *a) {
  UPB_UNUSED(t);
  UPB_UNUSED(a);
  UPB_ASSERT_DEBUGVAR(t->alloc == a);
}

static const double MAX_LOAD = 0.85;

/* The minimum utilization of the array part of a mixed hash/array table.  This
 * is a speed/memory-usage tradeoff (though it's not straightforward because of
 * cache effects).  The lower this is, the more memory we'll use. */
static const double MIN_DENSITY = 0.1;

bool is_pow2(uint64_t v) { return v == 0 || (v & (v - 1)) == 0; }

int log2ceil(uint64_t v) {
  int ret = 0;
  bool pow2 = is_pow2(v);
  while (v >>= 1) ret++;
  ret = pow2 ? ret : ret + 1;  /* Ceiling. */
  return UPB_MIN(UPB_MAXARRSIZE, ret);
}

char *upb_strdup(const char *s, upb_alloc *a) {
  return upb_strdup2(s, strlen(s), a);
}

char *upb_strdup2(const char *s, size_t len, upb_alloc *a) {
  size_t n;
  char *p;

  /* Prevent overflow errors. */
  if (len == SIZE_MAX) return NULL;
  /* Always null-terminate, even if binary data; but don't rely on the input to
   * have a null-terminating byte since it may be a raw binary buffer. */
  n = len + 1;
  p = upb_malloc(a, n);
  if (p) {
    memcpy(p, s, len);
    p[len] = 0;
  }
  return p;
}

/* A type to represent the lookup key of either a strtable or an inttable. */
typedef union {
  uintptr_t num;
  struct {
    const char *str;
    size_t len;
  } str;
} lookupkey_t;

static lookupkey_t strkey2(const char *str, size_t len) {
  lookupkey_t k;
  k.str.str = str;
  k.str.len = len;
  return k;
}

static lookupkey_t intkey(uintptr_t key) {
  lookupkey_t k;
  k.num = key;
  return k;
}

typedef uint32_t hashfunc_t(upb_tabkey key);
typedef bool eqlfunc_t(upb_tabkey k1, lookupkey_t k2);

/* Base table (shared code) ***************************************************/

/* For when we need to cast away const. */
static upb_tabent *mutable_entries(upb_table *t) {
  return (upb_tabent*)t->entries;
}

static bool isfull(upb_table *t) {
  if (upb_table_size(t) == 0) {
    return true;
  } else {
    return ((double)(t->count + 1) / upb_table_size(t)) > MAX_LOAD;
  }
}

static bool init(upb_table *t, upb_ctype_t ctype, uint8_t size_lg2,
                 upb_alloc *a) {
  size_t bytes;

  t->count = 0;
  t->ctype = ctype;
  t->size_lg2 = size_lg2;
  t->mask = upb_table_size(t) ? upb_table_size(t) - 1 : 0;
#ifndef NDEBUG
  t->alloc = a;
#endif
  bytes = upb_table_size(t) * sizeof(upb_tabent);
  if (bytes > 0) {
    t->entries = upb_malloc(a, bytes);
    if (!t->entries) return false;
    memset(mutable_entries(t), 0, bytes);
  } else {
    t->entries = NULL;
  }
  return true;
}

static void uninit(upb_table *t, upb_alloc *a) {
  upb_check_alloc(t, a);
  upb_free(a, mutable_entries(t));
}

static upb_tabent *emptyent(upb_table *t) {
  upb_tabent *e = mutable_entries(t) + upb_table_size(t);
  while (1) { if (upb_tabent_isempty(--e)) return e; UPB_ASSERT(e > t->entries); }
}

static upb_tabent *getentry_mutable(upb_table *t, uint32_t hash) {
  return (upb_tabent*)upb_getentry(t, hash);
}

static const upb_tabent *findentry(const upb_table *t, lookupkey_t key,
                                   uint32_t hash, eqlfunc_t *eql) {
  const upb_tabent *e;

  if (t->size_lg2 == 0) return NULL;
  e = upb_getentry(t, hash);
  if (upb_tabent_isempty(e)) return NULL;
  while (1) {
    if (eql(e->key, key)) return e;
    if ((e = e->next) == NULL) return NULL;
  }
}

static upb_tabent *findentry_mutable(upb_table *t, lookupkey_t key,
                                     uint32_t hash, eqlfunc_t *eql) {
  return (upb_tabent*)findentry(t, key, hash, eql);
}

static bool lookup(const upb_table *t, lookupkey_t key, upb_value *v,
                   uint32_t hash, eqlfunc_t *eql) {
  const upb_tabent *e = findentry(t, key, hash, eql);
  if (e) {
    if (v) {
      _upb_value_setval(v, e->val.val, t->ctype);
    }
    return true;
  } else {
    return false;
  }
}

/* The given key must not already exist in the table. */
static void insert(upb_table *t, lookupkey_t key, upb_tabkey tabkey,
                   upb_value val, uint32_t hash,
                   hashfunc_t *hashfunc, eqlfunc_t *eql) {
  upb_tabent *mainpos_e;
  upb_tabent *our_e;

  UPB_ASSERT(findentry(t, key, hash, eql) == NULL);
  UPB_ASSERT_DEBUGVAR(val.ctype == t->ctype);

  t->count++;
  mainpos_e = getentry_mutable(t, hash);
  our_e = mainpos_e;

  if (upb_tabent_isempty(mainpos_e)) {
    /* Our main position is empty; use it. */
    our_e->next = NULL;
  } else {
    /* Collision. */
    upb_tabent *new_e = emptyent(t);
    /* Head of collider's chain. */
    upb_tabent *chain = getentry_mutable(t, hashfunc(mainpos_e->key));
    if (chain == mainpos_e) {
      /* Existing ent is in its main posisiton (it has the same hash as us, and
       * is the head of our chain).  Insert to new ent and append to this chain. */
      new_e->next = mainpos_e->next;
      mainpos_e->next = new_e;
      our_e = new_e;
    } else {
      /* Existing ent is not in its main position (it is a node in some other
       * chain).  This implies that no existing ent in the table has our hash.
       * Evict it (updating its chain) and use its ent for head of our chain. */
      *new_e = *mainpos_e;  /* copies next. */
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
  our_e->val.val = val.val;
  UPB_ASSERT(findentry(t, key, hash, eql) == our_e);
}

static bool rm(upb_table *t, lookupkey_t key, upb_value *val,
               upb_tabkey *removed, uint32_t hash, eqlfunc_t *eql) {
  upb_tabent *chain = getentry_mutable(t, hash);
  if (upb_tabent_isempty(chain)) return false;
  if (eql(chain->key, key)) {
    /* Element to remove is at the head of its chain. */
    t->count--;
    if (val) _upb_value_setval(val, chain->val.val, t->ctype);
    if (removed) *removed = chain->key;
    if (chain->next) {
      upb_tabent *move = (upb_tabent*)chain->next;
      *chain = *move;
      move->key = 0;  /* Make the slot empty. */
    } else {
      chain->key = 0;  /* Make the slot empty. */
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
      upb_tabent *rm = (upb_tabent*)chain->next;
      t->count--;
      if (val) _upb_value_setval(val, chain->next->val.val, t->ctype);
      if (removed) *removed = rm->key;
      rm->key = 0;  /* Make the slot empty. */
      chain->next = rm->next;
      return true;
    } else {
      /* Element to remove is not in the table. */
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

/* A simple "subclass" of upb_table that only adds a hash function for strings. */

static upb_tabkey strcopy(lookupkey_t k2, upb_alloc *a) {
  uint32_t len = (uint32_t) k2.str.len;
  char *str = upb_malloc(a, k2.str.len + sizeof(uint32_t) + 1);
  if (str == NULL) return 0;
  memcpy(str, &len, sizeof(uint32_t));
  memcpy(str + sizeof(uint32_t), k2.str.str, k2.str.len + 1);
  return (uintptr_t)str;
}

static uint32_t strhash(upb_tabkey key) {
  uint32_t len;
  char *str = upb_tabstr(key, &len);
  return upb_murmur_hash2(str, len, 0);
}

static bool streql(upb_tabkey k1, lookupkey_t k2) {
  uint32_t len;
  char *str = upb_tabstr(k1, &len);
  return len == k2.str.len && memcmp(str, k2.str.str, len) == 0;
}

bool upb_strtable_init2(upb_strtable *t, upb_ctype_t ctype, upb_alloc *a) {
  return init(&t->t, ctype, 2, a);
}

void upb_strtable_uninit2(upb_strtable *t, upb_alloc *a) {
  size_t i;
  for (i = 0; i < upb_table_size(&t->t); i++)
    upb_free(a, (void*)t->t.entries[i].key);
  uninit(&t->t, a);
}

bool upb_strtable_resize(upb_strtable *t, size_t size_lg2, upb_alloc *a) {
  upb_strtable new_table;
  upb_strtable_iter i;

  upb_check_alloc(&t->t, a);

  if (!init(&new_table.t, t->t.ctype, size_lg2, a))
    return false;
  upb_strtable_begin(&i, t);
  for ( ; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_strtable_insert3(
        &new_table,
        upb_strtable_iter_key(&i),
        upb_strtable_iter_keylength(&i),
        upb_strtable_iter_value(&i),
        a);
  }
  upb_strtable_uninit2(t, a);
  *t = new_table;
  return true;
}

bool upb_strtable_insert3(upb_strtable *t, const char *k, size_t len,
                          upb_value v, upb_alloc *a) {
  lookupkey_t key;
  upb_tabkey tabkey;
  uint32_t hash;

  upb_check_alloc(&t->t, a);

  if (isfull(&t->t)) {
    /* Need to resize.  New table of double the size, add old elements to it. */
    if (!upb_strtable_resize(t, t->t.size_lg2 + 1, a)) {
      return false;
    }
  }

  key = strkey2(k, len);
  tabkey = strcopy(key, a);
  if (tabkey == 0) return false;

  hash = upb_murmur_hash2(key.str.str, key.str.len, 0);
  insert(&t->t, key, tabkey, v, hash, &strhash, &streql);
  return true;
}

bool upb_strtable_lookup2(const upb_strtable *t, const char *key, size_t len,
                          upb_value *v) {
  uint32_t hash = upb_murmur_hash2(key, len, 0);
  return lookup(&t->t, strkey2(key, len), v, hash, &streql);
}

bool upb_strtable_remove3(upb_strtable *t, const char *key, size_t len,
                         upb_value *val, upb_alloc *alloc) {
  uint32_t hash = upb_murmur_hash2(key, len, 0);
  upb_tabkey tabkey;
  if (rm(&t->t, strkey2(key, len), val, &tabkey, hash, &streql)) {
    upb_free(alloc, (void*)tabkey);
    return true;
  } else {
    return false;
  }
}

/* Iteration */

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
  if (!i->t) return true;
  return i->index >= upb_table_size(&i->t->t) ||
         upb_tabent_isempty(str_tabent(i));
}

const char *upb_strtable_iter_key(const upb_strtable_iter *i) {
  UPB_ASSERT(!upb_strtable_done(i));
  return upb_tabstr(str_tabent(i)->key, NULL);
}

size_t upb_strtable_iter_keylength(const upb_strtable_iter *i) {
  uint32_t len;
  UPB_ASSERT(!upb_strtable_done(i));
  upb_tabstr(str_tabent(i)->key, &len);
  return len;
}

upb_value upb_strtable_iter_value(const upb_strtable_iter *i) {
  UPB_ASSERT(!upb_strtable_done(i));
  return _upb_value_val(str_tabent(i)->val.val, i->t->t.ctype);
}

void upb_strtable_iter_setdone(upb_strtable_iter *i) {
  i->t = NULL;
  i->index = SIZE_MAX;
}

bool upb_strtable_iter_isequal(const upb_strtable_iter *i1,
                               const upb_strtable_iter *i2) {
  if (upb_strtable_done(i1) && upb_strtable_done(i2))
    return true;
  return i1->t == i2->t && i1->index == i2->index;
}


/* upb_inttable ***************************************************************/

/* For inttables we use a hybrid structure where small keys are kept in an
 * array and large keys are put in the hash table. */

static uint32_t inthash(upb_tabkey key) { return upb_inthash(key); }

static bool inteql(upb_tabkey k1, lookupkey_t k2) {
  return k1 == k2.num;
}

static upb_tabval *mutable_array(upb_inttable *t) {
  return (upb_tabval*)t->array;
}

static upb_tabval *inttable_val(upb_inttable *t, uintptr_t key) {
  if (key < t->array_size) {
    return upb_arrhas(t->array[key]) ? &(mutable_array(t)[key]) : NULL;
  } else {
    upb_tabent *e =
        findentry_mutable(&t->t, intkey(key), upb_inthash(key), &inteql);
    return e ? &e->val : NULL;
  }
}

static const upb_tabval *inttable_val_const(const upb_inttable *t,
                                            uintptr_t key) {
  return inttable_val((upb_inttable*)t, key);
}

size_t upb_inttable_count(const upb_inttable *t) {
  return t->t.count + t->array_count;
}

static void check(upb_inttable *t) {
  UPB_UNUSED(t);
#if defined(UPB_DEBUG_TABLE) && !defined(NDEBUG)
  {
    /* This check is very expensive (makes inserts/deletes O(N)). */
    size_t count = 0;
    upb_inttable_iter i;
    upb_inttable_begin(&i, t);
    for(; !upb_inttable_done(&i); upb_inttable_next(&i), count++) {
      UPB_ASSERT(upb_inttable_lookup(t, upb_inttable_iter_key(&i), NULL));
    }
    UPB_ASSERT(count == upb_inttable_count(t));
  }
#endif
}

bool upb_inttable_sizedinit(upb_inttable *t, upb_ctype_t ctype,
                            size_t asize, int hsize_lg2, upb_alloc *a) {
  size_t array_bytes;

  if (!init(&t->t, ctype, hsize_lg2, a)) return false;
  /* Always make the array part at least 1 long, so that we know key 0
   * won't be in the hash part, which simplifies things. */
  t->array_size = UPB_MAX(1, asize);
  t->array_count = 0;
  array_bytes = t->array_size * sizeof(upb_value);
  t->array = upb_malloc(a, array_bytes);
  if (!t->array) {
    uninit(&t->t, a);
    return false;
  }
  memset(mutable_array(t), 0xff, array_bytes);
  check(t);
  return true;
}

bool upb_inttable_init2(upb_inttable *t, upb_ctype_t ctype, upb_alloc *a) {
  return upb_inttable_sizedinit(t, ctype, 0, 4, a);
}

void upb_inttable_uninit2(upb_inttable *t, upb_alloc *a) {
  uninit(&t->t, a);
  upb_free(a, mutable_array(t));
}

bool upb_inttable_insert2(upb_inttable *t, uintptr_t key, upb_value val,
                          upb_alloc *a) {
  upb_tabval tabval;
  tabval.val = val.val;
  UPB_ASSERT(upb_arrhas(tabval));  /* This will reject (uint64_t)-1.  Fix this. */

  upb_check_alloc(&t->t, a);

  if (key < t->array_size) {
    UPB_ASSERT(!upb_arrhas(t->array[key]));
    t->array_count++;
    mutable_array(t)[key].val = val.val;
  } else {
    if (isfull(&t->t)) {
      /* Need to resize the hash part, but we re-use the array part. */
      size_t i;
      upb_table new_table;

      if (!init(&new_table, t->t.ctype, t->t.size_lg2 + 1, a)) {
        return false;
      }

      for (i = begin(&t->t); i < upb_table_size(&t->t); i = next(&t->t, i)) {
        const upb_tabent *e = &t->t.entries[i];
        uint32_t hash;
        upb_value v;

        _upb_value_setval(&v, e->val.val, t->t.ctype);
        hash = upb_inthash(e->key);
        insert(&new_table, intkey(e->key), e->key, v, hash, &inthash, &inteql);
      }

      UPB_ASSERT(t->t.count == new_table.count);

      uninit(&t->t, a);
      t->t = new_table;
    }
    insert(&t->t, intkey(key), key, val, upb_inthash(key), &inthash, &inteql);
  }
  check(t);
  return true;
}

bool upb_inttable_lookup(const upb_inttable *t, uintptr_t key, upb_value *v) {
  const upb_tabval *table_v = inttable_val_const(t, key);
  if (!table_v) return false;
  if (v) _upb_value_setval(v, table_v->val, t->t.ctype);
  return true;
}

bool upb_inttable_replace(upb_inttable *t, uintptr_t key, upb_value val) {
  upb_tabval *table_v = inttable_val(t, key);
  if (!table_v) return false;
  table_v->val = val.val;
  return true;
}

bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val) {
  bool success;
  if (key < t->array_size) {
    if (upb_arrhas(t->array[key])) {
      upb_tabval empty = UPB_TABVALUE_EMPTY_INIT;
      t->array_count--;
      if (val) {
        _upb_value_setval(val, t->array[key].val, t->t.ctype);
      }
      mutable_array(t)[key] = empty;
      success = true;
    } else {
      success = false;
    }
  } else {
    success = rm(&t->t, intkey(key), val, NULL, upb_inthash(key), &inteql);
  }
  check(t);
  return success;
}

bool upb_inttable_push2(upb_inttable *t, upb_value val, upb_alloc *a) {
  upb_check_alloc(&t->t, a);
  return upb_inttable_insert2(t, upb_inttable_count(t), val, a);
}

upb_value upb_inttable_pop(upb_inttable *t) {
  upb_value val;
  bool ok = upb_inttable_remove(t, upb_inttable_count(t) - 1, &val);
  UPB_ASSERT(ok);
  return val;
}

bool upb_inttable_insertptr2(upb_inttable *t, const void *key, upb_value val,
                             upb_alloc *a) {
  upb_check_alloc(&t->t, a);
  return upb_inttable_insert2(t, (uintptr_t)key, val, a);
}

bool upb_inttable_lookupptr(const upb_inttable *t, const void *key,
                            upb_value *v) {
  return upb_inttable_lookup(t, (uintptr_t)key, v);
}

bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val) {
  return upb_inttable_remove(t, (uintptr_t)key, val);
}

void upb_inttable_compact2(upb_inttable *t, upb_alloc *a) {
  /* A power-of-two histogram of the table keys. */
  size_t counts[UPB_MAXARRSIZE + 1] = {0};

  /* The max key in each bucket. */
  uintptr_t max[UPB_MAXARRSIZE + 1] = {0};

  upb_inttable_iter i;
  size_t arr_count;
  int size_lg2;
  upb_inttable new_t;

  upb_check_alloc(&t->t, a);

  upb_inttable_begin(&i, t);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    uintptr_t key = upb_inttable_iter_key(&i);
    int bucket = log2ceil(key);
    max[bucket] = UPB_MAX(max[bucket], key);
    counts[bucket]++;
  }

  /* Find the largest power of two that satisfies the MIN_DENSITY
   * definition (while actually having some keys). */
  arr_count = upb_inttable_count(t);

  for (size_lg2 = ARRAY_SIZE(counts) - 1; size_lg2 > 0; size_lg2--) {
    if (counts[size_lg2] == 0) {
      /* We can halve again without losing any entries. */
      continue;
    } else if (arr_count >= (1 << size_lg2) * MIN_DENSITY) {
      break;
    }

    arr_count -= counts[size_lg2];
  }

  UPB_ASSERT(arr_count <= upb_inttable_count(t));

  {
    /* Insert all elements into new, perfectly-sized table. */
    size_t arr_size = max[size_lg2] + 1;  /* +1 so arr[max] will fit. */
    size_t hash_count = upb_inttable_count(t) - arr_count;
    size_t hash_size = hash_count ? (hash_count / MAX_LOAD) + 1 : 0;
    int hashsize_lg2 = log2ceil(hash_size);

    upb_inttable_sizedinit(&new_t, t->t.ctype, arr_size, hashsize_lg2, a);
    upb_inttable_begin(&i, t);
    for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
      uintptr_t k = upb_inttable_iter_key(&i);
      upb_inttable_insert2(&new_t, k, upb_inttable_iter_value(&i), a);
    }
    UPB_ASSERT(new_t.array_size == arr_size);
    UPB_ASSERT(new_t.t.size_lg2 == hashsize_lg2);
  }
  upb_inttable_uninit2(t, a);
  *t = new_t;
}

/* Iteration. */

static const upb_tabent *int_tabent(const upb_inttable_iter *i) {
  UPB_ASSERT(!i->array_part);
  return &i->t->t.entries[i->index];
}

static upb_tabval int_arrent(const upb_inttable_iter *i) {
  UPB_ASSERT(i->array_part);
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
  if (!i->t) return true;
  if (i->array_part) {
    return i->index >= i->t->array_size ||
           !upb_arrhas(int_arrent(i));
  } else {
    return i->index >= upb_table_size(&i->t->t) ||
           upb_tabent_isempty(int_tabent(i));
  }
}

uintptr_t upb_inttable_iter_key(const upb_inttable_iter *i) {
  UPB_ASSERT(!upb_inttable_done(i));
  return i->array_part ? i->index : int_tabent(i)->key;
}

upb_value upb_inttable_iter_value(const upb_inttable_iter *i) {
  UPB_ASSERT(!upb_inttable_done(i));
  return _upb_value_val(
      i->array_part ? i->t->array[i->index].val : int_tabent(i)->val.val,
      i->t->t.ctype);
}

void upb_inttable_iter_setdone(upb_inttable_iter *i) {
  i->t = NULL;
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

#if defined(UPB_UNALIGNED_READS_OK) || defined(__s390x__)
/* -----------------------------------------------------------------------------
 * MurmurHash2, by Austin Appleby (released as public domain).
 * Reformatted and C99-ified by Joshua Haberman.
 * Note - This code makes a few assumptions about how your machine behaves -
 *   1. We can read a 4-byte value from any address without crashing
 *   2. sizeof(int) == 4 (in upb this limitation is removed by using uint32_t
 * And it has a few limitations -
 *   1. It will not work incrementally.
 *   2. It will not produce the same results on little-endian and big-endian
 *      machines. */
uint32_t upb_murmur_hash2(const void *key, size_t len, uint32_t seed) {
  /* 'm' and 'r' are mixing constants generated offline.
   * They're not really 'magic', they just happen to work well. */
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;

  /* Initialize the hash to a 'random' value */
  uint32_t h = seed ^ len;

  /* Mix 4 bytes at a time into the hash */
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

  /* Handle the last few bytes of the input array */
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
  };

  /* Do a few final mixes of the hash to ensure the last few
   * bytes are well-incorporated. */
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

#else /* !UPB_UNALIGNED_READS_OK */

/* -----------------------------------------------------------------------------
 * MurmurHashAligned2, by Austin Appleby
 * Same algorithm as MurmurHash2, but only does aligned reads - should be safer
 * on certain platforms.
 * Performance will be lower than MurmurHash2 */

#define MIX(h,k,m) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

uint32_t upb_murmur_hash2(const void * key, size_t len, uint32_t seed) {
  const uint32_t m = 0x5bd1e995;
  const int32_t r = 24;
  const uint8_t * data = (const uint8_t *)key;
  uint32_t h = (uint32_t)(seed ^ len);
  uint8_t align = (uintptr_t)data & 3;

  if(align && (len >= 4)) {
    /* Pre-load the temp registers */
    uint32_t t = 0, d = 0;
    int32_t sl;
    int32_t sr;

    switch(align) {
      case 1: t |= data[2] << 16;
      case 2: t |= data[1] << 8;
      case 3: t |= data[0];
    }

    t <<= (8 * align);

    data += 4-align;
    len -= 4-align;

    sl = 8 * (4-align);
    sr = 8 * align;

    /* Mix */

    while(len >= 4) {
      uint32_t k;

      d = *(uint32_t *)data;
      t = (t >> sr) | (d << sl);

      k = t;

      MIX(h,k,m);

      t = d;

      data += 4;
      len -= 4;
    }

    /* Handle leftover data in temp registers */

    d = 0;

    if(len >= align) {
      uint32_t k;

      switch(align) {
        case 3: d |= data[2] << 16;
        case 2: d |= data[1] << 8;
        case 1: d |= data[0];
      }

      k = (t >> sr) | (d << sl);
      MIX(h,k,m);

      data += align;
      len -= align;

      /* ----------
       * Handle tail bytes */

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

    /* ----------
     * Handle tail bytes */

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

#endif /* UPB_UNALIGNED_READS_OK */
