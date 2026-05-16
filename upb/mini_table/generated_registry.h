// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_GENERATED_REGISTRY_H_
#define UPB_MINI_TABLE_GENERATED_REGISTRY_H_

#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/internal/generated_registry.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

/* Generated registry: a global singleton that gathers all extensions linked
 * into the binary.
 *
 * This singleton is thread-safe and lock-free, implemented using atomics.  The
 * registry is lazily initialized the first time it is loaded.  When all
 * references are released, the registry will be destroyed.  New loads
 * afterwards will simply reload the same registry as needed.
 *
 * The extension minitables are registered in gencode using linker arrays.  Each
 * .proto file produces a weak, hidden, constructor function that adds all
 * visible extensions from the array into the registry.  In each binary, only
 * one copy of the constructor will actually be preserved by the linker, and
 * that copy will add all of the extensions for the entire binary.  All of these
 * are added to a global linked list of minitables pre-main, which are then used
 * to construct this singleton as needed.
 */

typedef struct upb_GeneratedRegistryRef upb_GeneratedRegistryRef;

// Loads the generated registry, returning a reference to it.  The reference
// must be held for the lifetime of any ExtensionRegistry obtained from it.
//
// Returns NULL on failure.
UPB_API const upb_GeneratedRegistryRef* upb_GeneratedRegistry_Load(void);

// Releases a reference to the generated registry.  This may destroy the
// registry if there are no other references to it.
//
// NULL is a valid argument and is simply ignored for easier error handling in
// callers.
UPB_API void upb_GeneratedRegistry_Release(const upb_GeneratedRegistryRef* r);

// Returns the extension registry contained by a reference to the generated
// registry.
//
// The reference must be held for the lifetime of the registry.
UPB_API const upb_ExtensionRegistry* upb_GeneratedRegistry_Get(
    const upb_GeneratedRegistryRef* r);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MINI_TABLE_GENERATED_REGISTRY_H_
