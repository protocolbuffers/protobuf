// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/generated_registry.h"

#include <stdint.h>

#include "upb/base/shutdown.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/internal/generated_registry.h"  // IWYU pragma: keep
#include "upb/port/atomic.h"
#include "upb/port/sanitizers.h"

// Must be last.
#include "upb/port/def.inc"

const UPB_PRIVATE(upb_GeneratedExtensionListEntry) *
    UPB_PRIVATE(upb_generated_extension_list) = NULL;

typedef struct upb_GeneratedRegistry {
  UPB_ATOMIC(upb_GeneratedRegistryRef*) ref;
#ifndef NDEBUG
  UPB_ATOMIC(uintptr_t) ref_count;
#endif
} upb_GeneratedRegistry;

static upb_GeneratedRegistry* _upb_generated_registry(void) {
  static upb_GeneratedRegistry r = {};
  return &r;
}

static bool _upb_GeneratedRegistry_AddAllLinkedExtensions(
    upb_ExtensionRegistry* r) {
  const UPB_PRIVATE(upb_GeneratedExtensionListEntry)* entry =
      UPB_PRIVATE(upb_generated_extension_list);
  while (entry != NULL) {
    // Comparing pointers to different objects is undefined behavior, so we
    // convert them to uintptr_t and compare their values.
    uintptr_t begin = (uintptr_t)entry->start;
    uintptr_t end = (uintptr_t)entry->stop;
    uintptr_t current = begin;
    while (current < end) {
      const upb_MiniTableExtension* ext =
          (const upb_MiniTableExtension*)current;
      // Sentinels and padding introduced by the linker can result in zeroed
      // entries, so simply skip them.
      if (upb_MiniTableExtension_Number(ext) == 0) {
        // MSVC introduces padding that might not be sized exactly the same as
        // upb_MiniTableExtension, so we can't iterate by sizeof.  This is a
        // valid thing for any linker to do, so it's safer to just always do it.
        current += UPB_ALIGN_OF(upb_MiniTableExtension);
        continue;
      }

      if (upb_ExtensionRegistry_Add(r, ext) !=
          kUpb_ExtensionRegistryStatus_Ok) {
        return false;
      }
      current += sizeof(upb_MiniTableExtension);
    }
    entry = entry->next;
  }
  return true;
}

// Constructs a new GeneratedRegistryRef, adding all linked extensions to the
// registry or returning NULL on failure.
static upb_GeneratedRegistryRef* _upb_GeneratedRegistry_New(void) {
  upb_Arena* arena = upb_Arena_New();
  if (arena == NULL) return NULL;
  upb_GeneratedRegistryRef* ref =
      upb_Arena_Malloc(arena, sizeof(upb_GeneratedRegistryRef));
  if (ref == NULL) goto err;
  ref->UPB_PRIVATE(arena) = arena;

  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena);
  if (extreg == NULL) goto err;
  ref->UPB_PRIVATE(registry) = extreg;

  if (!_upb_GeneratedRegistry_AddAllLinkedExtensions(extreg)) goto err;

  UPB_PRIVATE(upb_Xsan_Init)(UPB_XSAN(ref));
  return ref;

err:
  upb_Arena_Free(arena);
  return NULL;
}

const upb_GeneratedRegistryRef* upb_GeneratedRegistry_Load(void) {
  upb_GeneratedRegistry* registry = _upb_generated_registry();
  const upb_GeneratedRegistryRef* ref =
      upb_Atomic_Load(&registry->ref, memory_order_acquire);
  if (ref == NULL) {
    upb_GeneratedRegistryRef* new_ref = _upb_GeneratedRegistry_New();
    if (!new_ref) return NULL;
    upb_GeneratedRegistryRef* expected = NULL;
    if (!upb_Atomic_CompareExchangeStrong(&registry->ref, &expected, new_ref,
                                          memory_order_release,
                                          memory_order_acquire)) {
      upb_Arena_Free(new_ref->UPB_PRIVATE(arena));
      ref = expected;  // Another thread won, use their ref.
    } else {
      ref = new_ref;  // We won race, our new_ref is now in registry->ref.
    }
  }
  UPB_ASSERT(ref != NULL);
  UPB_PRIVATE(upb_Xsan_AccessReadOnly)(UPB_XSAN(ref));

#ifndef NDEBUG
  upb_Atomic_Add(&registry->ref_count, 1, memory_order_relaxed);
#endif
  return ref;
}

void upb_GeneratedRegistry_Release(const upb_GeneratedRegistryRef* r) {
#ifndef NDEBUG
  if (r == NULL) return;
  upb_GeneratedRegistry* registry = _upb_generated_registry();
  // Relaxed order safe here since it's exclusively used for debug checks, and
  // upb_Shutdown expects external synchronization. Using a stronger memory
  // order could impede tsan
  int ref_count = upb_Atomic_Sub(&registry->ref_count, 1, memory_order_relaxed);
  UPB_ASSERT(ref_count > 0);
#else
  UPB_UNUSED(r);
#endif
}

void upb_Shutdown(void) {
  upb_GeneratedRegistry* registry = _upb_generated_registry();
#ifndef NDEBUG
  UPB_ASSERT(upb_Atomic_Load(&registry->ref_count, memory_order_relaxed) == 0);
#endif
  upb_GeneratedRegistryRef* ref =
      upb_Atomic_Exchange(&registry->ref, NULL, memory_order_relaxed);
  if (ref) {
    UPB_PRIVATE(upb_Xsan_AccessReadWrite)(UPB_XSAN(ref));
    upb_Arena_Free(ref->UPB_PRIVATE(arena));
  }
}

const upb_ExtensionRegistry* upb_GeneratedRegistry_Get(
    const upb_GeneratedRegistryRef* r) {
  if (r == NULL) return NULL;
  return r->UPB_PRIVATE(registry);
}
