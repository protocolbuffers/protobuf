// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/generated_registry.h"

#include <stdint.h>

#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/internal/generated_registry.h"  // IWYU pragma: keep
#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"

#if UPB_TSAN
#include <sched.h>
#endif  // UPB_TSAN

const UPB_PRIVATE(upb_GeneratedExtensionListEntry) *
    UPB_PRIVATE(upb_generated_extension_list) = NULL;

typedef struct upb_GeneratedRegistry {
  UPB_ATOMIC(upb_GeneratedRegistryRef*) ref;
  UPB_ATOMIC(int32_t) ref_count;
} upb_GeneratedRegistry;

static upb_GeneratedRegistry* _upb_generated_registry(void) {
  static upb_GeneratedRegistry r = {NULL, 0};
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
  upb_Arena* arena = NULL;
  upb_ExtensionRegistry* extreg = NULL;
  upb_GeneratedRegistryRef* ref = upb_gmalloc(sizeof(upb_GeneratedRegistryRef));
  if (ref == NULL) goto err;
  arena = upb_Arena_New();
  if (arena == NULL) goto err;
  extreg = upb_ExtensionRegistry_New(arena);
  if (extreg == NULL) goto err;

  ref->UPB_PRIVATE(arena) = arena;
  ref->UPB_PRIVATE(registry) = extreg;

  if (!_upb_GeneratedRegistry_AddAllLinkedExtensions(extreg)) goto err;

  return ref;

err:
  if (arena != NULL) upb_Arena_Free(arena);
  if (ref != NULL) upb_gfree(ref);
  return NULL;
}

const upb_GeneratedRegistryRef* upb_GeneratedRegistry_Load(void) {
  upb_GeneratedRegistry* registry = _upb_generated_registry();

  // Loop until we successfully acquire a reference.  This loop should only
  // kick in under extremely high contention, and it should be guaranteed to
  // succeed.
  while (true) {
    int32_t count = upb_Atomic_Load(&registry->ref_count, memory_order_acquire);

    // Try to increment the refcount, but only if it's not zero.
    while (count > 0) {
      if (upb_Atomic_CompareExchangeStrong(&registry->ref_count, &count,
                                           count + 1, memory_order_acquire,
                                           memory_order_relaxed)) {
        // Successfully incremented. We can now safely load and return the
        // pointer.
        const upb_GeneratedRegistryRef* ref =
            upb_Atomic_Load(&registry->ref, memory_order_acquire);
        UPB_ASSERT(ref != NULL);
        return ref;
      }
      // CAS failed, `count` was updated. Loop will retry.
    }

    // If we're here, the count was 0. Time for the slow path.
    // Double-check that the pointer is NULL before trying to create.
    upb_GeneratedRegistryRef* ref =
        upb_Atomic_Load(&registry->ref, memory_order_acquire);
    if (ref == NULL) {
      // Pointer is NULL, try to create and publish a new registry.
      upb_GeneratedRegistryRef* new_ref = _upb_GeneratedRegistry_New();
      if (new_ref == NULL) return NULL;  // OOM

      // Try to CAS the pointer from NULL to our new_ref.
      if (upb_Atomic_CompareExchangeStrong(&registry->ref, &ref, new_ref,
                                           memory_order_release,
                                           memory_order_acquire)) {
        // We won the race. Set the ref count to 1.
        upb_Atomic_Store(&registry->ref_count, 1, memory_order_release);
        return new_ref;
      } else {
        // We lost the race. `ref` now holds the pointer from the winning
        // thread. Clean up our unused one and loop to try again to get a
        // reference.
        upb_Arena_Free(new_ref->UPB_PRIVATE(arena));
        upb_gfree(new_ref);
      }
    }
    // If we are here, either we lost the CAS race, or the pointer was already
    // non-NULL. In either case, we loop to the top and try to increment the
    // refcount of the existing object.

#if UPB_TSAN
    // Yield to give other threads a chance to increment the refcount.  This is
    // especially an issue for TSAN builds, which are prone to locking up from
    // the thread with the upb_Atomic_Store call above getting starved.
    sched_yield();
#endif  // UPB_TSAN
  }
}

void upb_GeneratedRegistry_Release(const upb_GeneratedRegistryRef* r) {
  if (r == NULL) return;

  upb_GeneratedRegistry* registry = _upb_generated_registry();

  int ref_count = upb_Atomic_Sub(&registry->ref_count, 1, memory_order_acq_rel);
  UPB_ASSERT(registry->ref_count >= 0);

  // A ref_count of 1 means that we decremented the refcount to 0.
  if (ref_count == 1) {
    upb_GeneratedRegistryRef* ref =
        upb_Atomic_Exchange(&registry->ref, NULL, memory_order_acq_rel);
    if (ref != NULL) {
      // This is the last reference and we won any potential race to store NULL,
      // so we need to clean up.
      upb_Arena_Free(ref->UPB_PRIVATE(arena));
      upb_gfree(ref);
    }
  }
}

const upb_ExtensionRegistry* upb_GeneratedRegistry_Get(
    const upb_GeneratedRegistryRef* r) {
  if (r == NULL) return NULL;
  return r->UPB_PRIVATE(registry);
}
