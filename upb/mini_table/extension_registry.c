// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/extension_registry.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/hash/published_map.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_ExtensionRegistry {
  upb_PublishedMap* exts;
  upb_Arena* arena;
};

upb_ExtensionRegistry* upb_ExtensionRegistry_New(upb_Arena* arena) {
  upb_ExtensionRegistry* r = upb_Arena_Malloc(arena, sizeof(*r));
  if (!r) return NULL;
  r->arena = arena;
  r->exts = upb_PublishedMap_New(arena);
  if (!r->exts) return NULL;
  return r;
}

UPB_API upb_ExtensionRegistryStatus upb_ExtensionRegistry_Add(
    upb_ExtensionRegistry* r, const upb_MiniTableExtension* e) {
  const upb_MiniTableExtension* array[] = {e};
  return upb_ExtensionRegistry_AddArray(r, array, 1);
}

upb_ExtensionRegistryStatus upb_ExtensionRegistry_AddArray(
    upb_ExtensionRegistry* r, const upb_MiniTableExtension** e, size_t count) {
  if (!count) return kUpb_ExtensionRegistryStatus_Ok;
  upb_PublicationBatch* batch = upb_PublicationBatch_New(r->arena);
  if (!batch) return kUpb_ExtensionRegistryStatus_OutOfMemory;
  upb_ExtensionRegistryStatus status =
      _upb_ExtensionRegistry_AddArrayPending(r, e, count, batch);
  if (status == kUpb_ExtensionRegistryStatus_Ok) {
    upb_PublicationBatch_Commit(batch);
  }
  return status;
}

upb_ExtensionRegistryStatus _upb_ExtensionRegistry_AddArrayPending(
    upb_ExtensionRegistry* r, const upb_MiniTableExtension** e, size_t count,
    upb_PublicationBatch* batch) {
  for (size_t i = 0; i < count; i++) {
    upb_PublishedMapStatus status = upb_PublishedMap_Insert(
        r->exts, batch, upb_MiniTableExtension_Extendee(e[i]),
        upb_MiniTableExtension_Number(e[i]), e[i]);
    if (status == kUpb_PublishedMap_Duplicate) {
      return kUpb_ExtensionRegistryStatus_DuplicateEntry;
    }
    if (status == kUpb_PublishedMap_OutOfMemory) {
      return kUpb_ExtensionRegistryStatus_OutOfMemory;
    }
  }
  return kUpb_ExtensionRegistryStatus_Ok;
}

const upb_MiniTableExtension* upb_ExtensionRegistry_Lookup(
    const upb_ExtensionRegistry* r, const upb_MiniTable* t, uint32_t num) {
  return upb_PublishedMap_Lookup(r->exts, t, num);
}

size_t upb_ExtensionRegistry_Size(const upb_ExtensionRegistry* r) {
  return upb_PublishedMap_Size(r->exts);
}
