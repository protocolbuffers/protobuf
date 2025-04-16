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

#include "upb/hash/common.h"
#include "upb/hash/str_table.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#define EXTREG_KEY_SIZE (sizeof(upb_MiniTable*) + sizeof(uint32_t))

struct upb_ExtensionRegistry {
  upb_Arena* arena;
  upb_strtable exts;  // Key is upb_MiniTable* concatenated with fieldnum.
};

static void extreg_key(char* buf, const upb_MiniTable* l, uint32_t fieldnum) {
  memcpy(buf, &l, sizeof(l));
  memcpy(buf + sizeof(l), &fieldnum, sizeof(fieldnum));
}

upb_ExtensionRegistry* upb_ExtensionRegistry_New(upb_Arena* arena) {
  upb_ExtensionRegistry* r = upb_Arena_Malloc(arena, sizeof(*r));
  if (!r) return NULL;
  r->arena = arena;
  if (!upb_strtable_init(&r->exts, 8, arena)) return NULL;
  return r;
}

UPB_API upb_ExtensionRegistryStatus upb_ExtensionRegistry_Add(
    upb_ExtensionRegistry* r, const upb_MiniTableExtension* e) {
  char buf[EXTREG_KEY_SIZE];
  extreg_key(buf, e->UPB_PRIVATE(extendee), upb_MiniTableExtension_Number(e));

  if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, NULL)) {
    return kUpb_ExtensionRegistryStatus_DuplicateEntry;
  }

  if (!upb_strtable_insert(&r->exts, buf, EXTREG_KEY_SIZE,
                           upb_value_constptr(e), r->arena)) {
    return kUpb_ExtensionRegistryStatus_OutOfMemory;
  }
  return kUpb_ExtensionRegistryStatus_Ok;
}

upb_ExtensionRegistryStatus upb_ExtensionRegistry_AddArray(
    upb_ExtensionRegistry* r, const upb_MiniTableExtension** e, size_t count) {
  const upb_MiniTableExtension** start = e;
  const upb_MiniTableExtension** end = UPB_PTRADD(e, count);
  upb_ExtensionRegistryStatus status = kUpb_ExtensionRegistryStatus_Ok;
  for (; e < end; e++) {
    status = upb_ExtensionRegistry_Add(r, *e);
    if (status != kUpb_ExtensionRegistryStatus_Ok) goto failure;
  }
  return kUpb_ExtensionRegistryStatus_Ok;

failure:
  // Back out the entries previously added.
  for (end = e, e = start; e < end; e++) {
    const upb_MiniTableExtension* ext = *e;
    char buf[EXTREG_KEY_SIZE];
    extreg_key(buf, ext->UPB_PRIVATE(extendee),
               upb_MiniTableExtension_Number(ext));
    upb_strtable_remove2(&r->exts, buf, EXTREG_KEY_SIZE, NULL);
  }
  UPB_ASSERT(status != kUpb_ExtensionRegistryStatus_Ok);
  return status;
}

#ifdef UPB_LINKARR_DECLARE

UPB_LINKARR_DECLARE(upb_AllExts, const upb_MiniTableExtension);

bool upb_ExtensionRegistry_AddAllLinkedExtensions(upb_ExtensionRegistry* r) {
  const upb_MiniTableExtension* start = UPB_LINKARR_START(upb_AllExts);
  const upb_MiniTableExtension* stop = UPB_LINKARR_STOP(upb_AllExts);
  for (const upb_MiniTableExtension* p = start; p < stop; p++) {
    // Windows can introduce zero padding, so we have to skip zeroes.
    if (upb_MiniTableExtension_Number(p) != 0) {
      if (upb_ExtensionRegistry_Add(r, p) != kUpb_ExtensionRegistryStatus_Ok) {
        return false;
      }
    }
  }
  return true;
}

#endif  // UPB_LINKARR_DECLARE

const upb_MiniTableExtension* upb_ExtensionRegistry_Lookup(
    const upb_ExtensionRegistry* r, const upb_MiniTable* t, uint32_t num) {
  char buf[EXTREG_KEY_SIZE];
  upb_value v;
  extreg_key(buf, t, num);
  if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}
