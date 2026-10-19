// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file or at https://developers.google.com/open-source/licenses/bsd

#include "upb/hash/published_map.h"

#include "upb/hash/common.h"
#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_PublicationBatch {
  UPB_ATOMIC(uintptr_t) committed;
};

typedef struct upb_PublishedEntry {
  const struct upb_PublishedEntry* next;
  const upb_PublicationBatch* batch;
  const void* key;
  const void* value;
  uint32_t number;
  uint32_t hash;
} upb_PublishedEntry;

typedef struct {
  size_t count;
  UPB_ATOMIC(const upb_PublishedEntry*)* heads;
} upb_PublishedBuckets;

struct upb_PublishedMap {
  UPB_ATOMIC(upb_PublishedBuckets*) buckets;
  upb_Arena* arena;
  size_t entries;  // Writer-only, including this writer's uncommitted entries.
};

static bool upb_PublishedEntry_IsCommitted(const upb_PublishedEntry* entry) {
  return upb_Atomic_Load(&entry->batch->committed, memory_order_acquire) != 0;
}

static upb_PublishedBuckets* upb_PublishedBuckets_New(upb_Arena* arena,
                                                     size_t count) {
  if (count > SIZE_MAX / sizeof(UPB_ATOMIC(const upb_PublishedEntry*))) {
    return NULL;
  }
  upb_PublishedBuckets* buckets = upb_Arena_Malloc(arena, sizeof(*buckets));
  if (!buckets) return NULL;
  buckets->heads = upb_Arena_Malloc(arena, count * sizeof(*buckets->heads));
  if (!buckets->heads) return NULL;
  buckets->count = count;
  for (size_t i = 0; i < count; i++) upb_Atomic_Init(&buckets->heads[i], NULL);
  return buckets;
}

upb_PublishedMap* upb_PublishedMap_New(upb_Arena* arena) {
  upb_PublishedMap* map = upb_Arena_Malloc(arena, sizeof(*map));
  if (!map) return NULL;
  upb_PublishedBuckets* buckets = upb_PublishedBuckets_New(arena, 8);
  if (!buckets) return NULL;
  upb_Atomic_Init(&map->buckets, buckets);
  map->arena = arena;
  map->entries = 0;
  return map;
}

upb_PublicationBatch* upb_PublicationBatch_New(upb_Arena* arena) {
  upb_PublicationBatch* batch = upb_Arena_Malloc(arena, sizeof(*batch));
  if (batch) upb_Atomic_Init(&batch->committed, 0);
  return batch;
}

void upb_PublicationBatch_Commit(upb_PublicationBatch* batch) {
  UPB_ASSERT(!upb_Atomic_Load(&batch->committed, memory_order_relaxed));
  upb_Atomic_Store(&batch->committed, 1, memory_order_release);
}

static uint32_t upb_PublishedMap_Hash(const upb_PublishedMap* map,
                                      const void* key, uint32_t number) {
  uint64_t pair[2] = {(uintptr_t)key, number};
  return _upb_Hash(pair, sizeof(pair), (uintptr_t)map);
}

static bool upb_PublishedMap_Grow(upb_PublishedMap* map,
                                  upb_PublicationBatch* current) {
  upb_PublishedBuckets* old =
      upb_Atomic_Load(&map->buckets, memory_order_relaxed);
  if (old->count > SIZE_MAX / 2) return false;
  upb_PublishedBuckets* buckets =
      upb_PublishedBuckets_New(map->arena, old->count * 2);
  if (!buckets) return false;
  size_t entries = 0;
  for (size_t i = 0; i < old->count; i++) {
    const upb_PublishedEntry* entry =
        upb_Atomic_Load(&old->heads[i], memory_order_relaxed);
    for (; entry; entry = entry->next) {
      if (entry->batch != current && !upb_PublishedEntry_IsCommitted(entry)) {
        continue;  // Never inspect values belonging to abandoned batches.
      }
      upb_PublishedEntry* copy = upb_Arena_Malloc(map->arena, sizeof(*copy));
      if (!copy) return false;  // The old root is still unchanged.
      *copy = *entry;
      size_t bucket = copy->hash & (buckets->count - 1);
      copy->next = upb_Atomic_Load(&buckets->heads[bucket], memory_order_relaxed);
      upb_Atomic_Store(&buckets->heads[bucket], copy, memory_order_relaxed);
      entries++;
    }
  }
  map->entries = entries;
  upb_Atomic_Store(&map->buckets, buckets, memory_order_release);
  return true;
}

upb_PublishedMapStatus upb_PublishedMap_Insert(
    upb_PublishedMap* map, upb_PublicationBatch* batch, const void* key,
    uint32_t number, const void* value) {
  UPB_ASSERT(key && value && batch);
  UPB_ASSERT(!upb_Atomic_Load(&batch->committed, memory_order_relaxed));
  uint32_t hash = upb_PublishedMap_Hash(map, key, number);
  upb_PublishedBuckets* buckets =
      upb_Atomic_Load(&map->buckets, memory_order_relaxed);
  size_t bucket = hash & (buckets->count - 1);
  const upb_PublishedEntry* head =
      upb_Atomic_Load(&buckets->heads[bucket], memory_order_relaxed);
  size_t abandoned = 0;
  while (head && head->batch != batch && !upb_PublishedEntry_IsCommitted(head)) {
    head = head->next;
    abandoned++;
  }
  for (const upb_PublishedEntry* entry = head; entry; entry = entry->next) {
    if ((entry->batch == batch || upb_PublishedEntry_IsCommitted(entry)) &&
        entry->key == key && entry->number == number) {
      return kUpb_PublishedMap_Duplicate;
    }
  }
  if (map->entries >= buckets->count) {
    if (!upb_PublishedMap_Grow(map, batch)) return kUpb_PublishedMap_OutOfMemory;
    buckets = upb_Atomic_Load(&map->buckets, memory_order_relaxed);
    bucket = hash & (buckets->count - 1);
    head = upb_Atomic_Load(&buckets->heads[bucket], memory_order_relaxed);
    abandoned = 0;  // Growth already discarded all abandoned batches.
  }
  upb_PublishedEntry* entry = upb_Arena_Malloc(map->arena, sizeof(*entry));
  if (!entry) return kUpb_PublishedMap_OutOfMemory;
  *entry = (upb_PublishedEntry){head, batch, key, value, number, hash};
  map->entries = map->entries - abandoned + 1;
  upb_Atomic_Store(&buckets->heads[bucket], entry, memory_order_release);
  return kUpb_PublishedMap_Ok;
}

const void* upb_PublishedMap_Lookup(const upb_PublishedMap* map, const void* key,
                                    uint32_t number) {
  uint32_t hash = upb_PublishedMap_Hash(map, key, number);
  const upb_PublishedBuckets* buckets =
      upb_Atomic_Load(&map->buckets, memory_order_acquire);
  const upb_PublishedEntry* entry =
      upb_Atomic_Load(&buckets->heads[hash & (buckets->count - 1)],
                      memory_order_acquire);
  for (; entry; entry = entry->next) {
    if (upb_PublishedEntry_IsCommitted(entry) && entry->key == key &&
        entry->number == number) {
      return entry->value;
    }
  }
  return NULL;
}

size_t upb_PublishedMap_Size(const upb_PublishedMap* map) {
  const upb_PublishedBuckets* buckets =
      upb_Atomic_Load(&map->buckets, memory_order_acquire);
  size_t count = 0;
  for (size_t i = 0; i < buckets->count; i++) {
    const upb_PublishedEntry* entry =
        upb_Atomic_Load(&buckets->heads[i], memory_order_acquire);
    for (; entry; entry = entry->next) count += upb_PublishedEntry_IsCommitted(entry);
  }
  return count;
}

#include "upb/port/undef.inc"
