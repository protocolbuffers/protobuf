#include "upb/mini_table/generated_registry.h"

#include <stdatomic.h>
#include <stdint.h>

#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/internal/generated_registry.h"
#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct upb_GeneratedRegistry {
  UPB_ATOMIC(upb_GeneratedRegistryRef*) ref;
  UPB_ATOMIC(int32_t) ref_count;
} upb_GeneratedRegistry;

static upb_GeneratedRegistry* _upb_generated_registry(void) {
  static upb_GeneratedRegistry r = {NULL, 0};
  return &r;
}

// Constructs a new GeneratedRegistryRef, adding all linked extensions to the
// registry or returning NULL on failure.
static upb_GeneratedRegistryRef* _upb_GeneratedRegistry_New(void) {
  upb_Arena* arena = NULL;
  upb_ExtensionRegistry* extreg = NULL;
  upb_GeneratedRegistryRef* ref = upb_gmalloc(sizeof(upb_GeneratedRegistry));
  if (ref == NULL) goto err;
  arena = upb_Arena_New();
  if (arena == NULL) goto err;
  extreg = upb_ExtensionRegistry_New(arena);
  if (extreg == NULL) goto err;

  ref->arena = arena;
  ref->registry = extreg;

  if (!upb_ExtensionRegistry_AddAllLinkedExtensions(extreg)) goto err;

  return ref;

err:
  if (arena != NULL) upb_Arena_Free(arena);
  if (ref != NULL) upb_gfree(ref);
  return NULL;
}

const upb_GeneratedRegistryRef* upb_GeneratedRegistry_Load(void) {
  upb_GeneratedRegistry* registry = _upb_generated_registry();

  // Incrementing the refcount is simply book-keeping and doesn't need any
  // memory ordering.
  upb_Atomic_Add(&registry->ref_count, 1, memory_order_relaxed);

  upb_GeneratedRegistryRef* ref =
      upb_Atomic_Load(&registry->ref, memory_order_acquire);

  if (ref == NULL) {
    upb_GeneratedRegistryRef* new_ref = _upb_GeneratedRegistry_New();
    if (new_ref == NULL) {
      // On error, we need to decrement the refcount to clean up.
      upb_Atomic_Sub(&registry->ref_count, 1, memory_order_relaxed);
      return NULL;
    }

    // Attempt to store the new ref, possibly racing with other threads.  This
    // needs to be memory ordered so that it happens after the ref is
    // initialized and before any potential decrement.
    if (upb_Atomic_CompareExchangeStrong(&registry->ref, &ref, new_ref,
                                         memory_order_release,
                                         memory_order_acquire)) {
      return new_ref;
    } else {
      // We lost the race to store new_ref, clean up and return.
      upb_Arena_Free(new_ref->arena);
      upb_gfree(new_ref);
    }
  }
  return ref;
}

void upb_GeneratedRegistry_Release(const upb_GeneratedRegistryRef* r) {
  if (r == NULL) return;

  upb_GeneratedRegistry* registry = _upb_generated_registry();

  int ref_count = upb_Atomic_Sub(&registry->ref_count, 1, memory_order_relaxed);
  UPB_ASSERT(registry->ref_count >= 0);

  if (ref_count == 1) {
    // This is the last reference and we won any potential race to store NULL,
    // so we need to clean up.
    upb_GeneratedRegistryRef* ref =
        upb_Atomic_Exchange(&registry->ref, NULL, memory_order_acq_rel);
    if (ref != NULL) {
      upb_Arena_Free(ref->arena);
      upb_gfree(ref);
    }
  }
}
