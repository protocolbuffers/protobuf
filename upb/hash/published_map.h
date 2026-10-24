// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file or at https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_HASH_PUBLISHED_MAP_H_
#define UPB_HASH_PUBLISHED_MAP_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/arena.h"

#ifdef __cplusplus
extern "C" {
#endif

// An arena-owned, append-only (pointer, number) map. Writers must be externally
// serialized; lookups may run concurrently without acquiring a lock. Entries,
// old bucket arrays, and old chains remain alive until the arena is freed.
// A batch may span multiple maps; its arena must outlive all of those maps.
// Commit it only after every referenced value
// is initialized and guaranteed to remain alive for the readers' lifetimes.
// Abandon a failed batch without committing: its values are never exposed.
typedef struct upb_PublishedMap upb_PublishedMap;
typedef struct upb_PublicationBatch upb_PublicationBatch;

typedef enum {
  kUpb_PublishedMap_Ok,
  kUpb_PublishedMap_Duplicate,
  kUpb_PublishedMap_OutOfMemory,
} upb_PublishedMapStatus;

upb_PublishedMap* upb_PublishedMap_New(upb_Arena* arena);
upb_PublicationBatch* upb_PublicationBatch_New(upb_Arena* arena);
void upb_PublicationBatch_Commit(upb_PublicationBatch* batch);
upb_PublishedMapStatus upb_PublishedMap_Insert(
    upb_PublishedMap* map, upb_PublicationBatch* batch, const void* key,
    uint32_t number, const void* value);
const void* upb_PublishedMap_Lookup(const upb_PublishedMap* map, const void* key,
                                    uint32_t number);
// For diagnostics only; the count need not be a snapshot during registration.
size_t upb_PublishedMap_Size(const upb_PublishedMap* map);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // UPB_HASH_PUBLISHED_MAP_H_
