// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/generated_registry.h"

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/arena.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/internal/generated_registry.h"  // IWYU pragma: keep
#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"
#include "upb/port/sanitizers.h"

#if UPB_TSAN
#include <sched.h>
#endif  // UPB_TSAN

const UPB_PRIVATE(upb_GeneratedExtensionListEntry) *
    UPB_PRIVATE(upb_generated_extension_list) = NULL;

typedef struct upb_GeneratedRegistry {
  // Exclusively accessed via relaxed memory order; writes are made visible by
  // aquire/release on the refcount. The initial write of a non-NULL pointer is
  // made visible by the release store of 1 to ref_count and the acquire CAS to
  // increment it.
  UPB_ATOMIC(upb_GeneratedRegistryRef*) ref;
  UPB_ATOMIC(uintptr_t) ref_count;
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
  upb_GeneratedRegistryRef* new_ref = NULL;
  size_t num_spins = 0;
  const size_t kMaxSpins = 1000;

  // Loop until we successfully acquire a reference.  This loop should only
  // kick in under extremely high contention.
  while (num_spins < kMaxSpins) {
    uintptr_t count =
        upb_Atomic_Load(&registry->ref_count, memory_order_relaxed);

    // Try to increment the refcount, but only if it's not zero.
    while (count > 0) {
      if (upb_Atomic_CompareExchangeStrong(&registry->ref_count, &count,
                                           count + 1, memory_order_acquire,
                                           memory_order_relaxed)) {
        // Successfully incremented. We can now safely load and return the
        // pointer. Relaxed order is safe here because our CAS acquired from the
        // original release-ordered store of 1 to the refcount.
        const upb_GeneratedRegistryRef* ref =
            upb_Atomic_Load(&registry->ref, memory_order_relaxed);
        UPB_ASSERT(ref != NULL);
        UPB_PRIVATE(upb_Xsan_AccessReadOnly)(UPB_XSAN(ref));
        if (new_ref != NULL) {
          // If we initialized a ref but never published it, free it.
          upb_Arena_Free(new_ref->UPB_PRIVATE(arena));
        }
        return ref;
      }
      // CAS failed, `count` was updated. Loop will retry.
    }

    // If we're here, the count was 0. Time for the slow path.
    // Double-check that the pointer is NULL before trying to create.
    upb_GeneratedRegistryRef* ref =
        upb_Atomic_Load(&registry->ref, memory_order_relaxed);
    if (ref == NULL) {
      if (new_ref == NULL) {
        // Pointer is NULL, try to create and publish a new registry.
        new_ref = _upb_GeneratedRegistry_New();
        if (new_ref == NULL) return NULL;  // OOM
      }

      // Try to CAS the pointer from NULL to our new_ref.
      if (upb_Atomic_CompareExchangeStrong(&registry->ref, &ref, new_ref,
                                           memory_order_relaxed,
                                           memory_order_relaxed)) {
        // We won the race. Because of our relaxed store, it's not safe for
        // other threads to actually dereference this pointer until they read
        // our store of 1 to ref_count. Note that if the current thread halts
        // here, other threads will spin forever.
        upb_Atomic_Store(&registry->ref_count, 1, memory_order_release);
        return new_ref;
      }
      // Loop to the top and try to increment the refcount of the existing
      // object.
    } else {
      // We don't know if the non-null pointer we observed was being initialized
      // but the refcount was 0 because the publishing thread hadn't set it to 1
      // yet, or the pointer we observed was dangling because the freeing thread
      // hadn't set it to NULL yet. To avoid use-after-free, we can't read it,
      // and we need to wait until we see either a nonzero refcount or a NULL
      // ref, making this a spin section.
      upb_Atomic_SpinWait();
      num_spins++;
    }
  }
  // To avoid spinning indefinitely, bail out and make a new one after a bunch
  // of failed attempts.
  if (new_ref == NULL) {
    new_ref = _upb_GeneratedRegistry_New();
  }
  return new_ref;
}

void upb_GeneratedRegistry_Release(const upb_GeneratedRegistryRef* r) {
  if (r == NULL) return;

  upb_GeneratedRegistry* registry = _upb_generated_registry();

  // If the registry we're freeing was not the global (because this ref was
  // created when bailing out after too many spin attempts) we don't want to
  // mess with global state. Relaxed order is fine here as if r is the current
  // global, nothing else can be changing the global's value because we haven't
  // decremented the refcount yet; if r is not the global, then it can't be
  // pointer equal to the global regardless of what value we racily observe.
  if (r != upb_Atomic_Load(&registry->ref, memory_order_relaxed)) {
    upb_Arena_Free(r->UPB_PRIVATE(arena));
    return;
  }

  // acq_rel ensures that decrements that don't hit zero release all their
  // prior operations, to be caught by the acquire when the refcount does hit
  // zero.
  uintptr_t ref_count =
      upb_Atomic_Sub(&registry->ref_count, 1, memory_order_acq_rel);

  UPB_ASSERT(ref_count != 0);

  // A ref_count of 1 means that we decremented the refcount to 0.
  if (ref_count == 1) {
    UPB_ASSERT(upb_Atomic_Load(&registry->ref, memory_order_relaxed) == r);
    // Now that the refcount is 0, other threads can't re-initialize it until we
    // store NULL. If the current thread hangs here, other threads may spin
    // forever.
    upb_Atomic_Store(&registry->ref, NULL, memory_order_relaxed);
    upb_Arena_Free(r->UPB_PRIVATE(arena));
  }
}

const upb_ExtensionRegistry* upb_GeneratedRegistry_Get(
    const upb_GeneratedRegistryRef* r) {
  if (r == NULL) return NULL;
  return r->UPB_PRIVATE(registry);
}
