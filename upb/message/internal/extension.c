// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/extension.h"

#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/extension.h"
#include "upb/port/overflow.h"

// Must be last.
#include "upb/port/def.inc"

bool UPB_PRIVATE(_upb_Message_FindExtensionEntry)(
    const struct upb_Message* msg, const upb_MiniTableExtension* e,
    size_t* index, upb_TaggedAuxPtr* out_ptr) {
  const upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return false;

  for (size_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr =
        UPB_PRIVATE(_upb_Message_Internal_GetAux)(in, i);
    const upb_MiniTableExtension* found;
    if (upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
      found = upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr)->ext;
    } else if (upb_TaggedAuxPtr_IsLazyExtension(tagged_ptr)) {
      found = upb_TaggedAuxPtr_LazyExtension(tagged_ptr)->ext;
    } else {
      continue;
    }
    if (found == e) {
      if (index) *index = i;
      if (out_ptr) *out_ptr = tagged_ptr;
      return true;
    }
  }

  return false;
}

const upb_Extension* UPB_PRIVATE(_upb_Message_Getext)(
    const struct upb_Message* msg, const upb_MiniTableExtension* e) {
  upb_TaggedAuxPtr tagged_ptr;
  if (!UPB_PRIVATE(_upb_Message_FindExtensionEntry)(msg, e, NULL,
                                                    &tagged_ptr)) {
    return NULL;
  }
  if (!upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
    // A lazy extension that has not been promoted yet has no parsed value.
    return NULL;
  }
  return upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);
}

static upb_Extension* _upb_Message_NewExtension(const upb_MiniTableExtension* e,
                                                upb_Arena* a) {
  upb_Extension* ext = upb_Arena_Malloc(a, sizeof(upb_Extension));
  if (!ext) return NULL;
  memset(ext, 0, sizeof(upb_Extension));
  ext->ext = e;
  return ext;
}

upb_Extension* UPB_PRIVATE(_upb_Message_GetOrCreateExtensionWithTag)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a,
    upb_TaggedAuxType tag) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  // For Canonical Extensions, we check whether the extension has already been
  // set. If we find an extension with the same pointer and tag, we reuse it to
  // prevent duplicate entries for the same extension.
  //
  // For Non-Canonical Extensions, we do NOT reuse them, matching the behavior
  // of adding a unknown StringView (through `_upb_Message_AddUnknown`) which
  // accumulates.
  if (tag == kUpb_TaggedAuxType_CanonicalExtension) {
    size_t index;
    upb_TaggedAuxPtr tagged_ptr;
    if (UPB_PRIVATE(_upb_Message_FindExtensionEntry)(msg, e, &index,
                                                     &tagged_ptr)) {
      if (upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
        return upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);
      }
      // The message holds a lazy payload for this extension. We cannot parse
      // it here (that would require a dependency on the wire format decoder),
      // and the caller is about to overwrite the value anyway, so the payload
      // is dropped and replaced with a fresh extension in the same slot.
      UPB_ASSERT(upb_TaggedAuxPtr_IsLazyExtension(tagged_ptr));
      upb_Extension* ext = _upb_Message_NewExtension(e, a);
      if (!ext) return NULL;
      upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
      UPB_PRIVATE(_upb_Message_Internal_SetAux)(
          in, index, upb_TaggedAuxPtr_MakeCanonicalExtension(ext));
      return ext;
    }
  }
  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, a)) return NULL;
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  upb_Extension* ext = _upb_Message_NewExtension(e, a);
  if (!ext) return NULL;
  UPB_PRIVATE(_upb_Message_Internal_SetAux)(
      in, in->size, upb_TaggedAuxPtr_MakeExtension(ext, tag));
  in->size++;
  return ext;
}

upb_Extension* UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a) {
  return UPB_PRIVATE(_upb_Message_GetOrCreateExtensionWithTag)(
      msg, e, a, kUpb_TaggedAuxType_CanonicalExtension);
}

upb_Extension* UPB_PRIVATE(_upb_Message_CreateNonCanonicalExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a) {
  return UPB_PRIVATE(_upb_Message_GetOrCreateExtensionWithTag)(
      msg, e, a, kUpb_TaggedAuxType_NonCanonicalExtension);
}

// Combines the decode options of two occurrences of the same lazy extension.
// The low 16 bits (flags) are expected to be identical; the high 16 bits hold
// the remaining recursion depth, of which we keep the smaller one so that the
// deferred parse can never exceed the limit that an eager parse would have
// enforced.
static int _upb_LazyExtension_MergeOptions(int a, int b) {
  uint32_t ua = (uint32_t)a;
  uint32_t ub = (uint32_t)b;
  uint32_t depth = UPB_MIN(ua >> 16, ub >> 16);
  return (int)((depth << 16) | (ua & 0xffff));
}

// Allocates a non-aliased lazy extension block with room for `size` payload
// bytes immediately following the header, and returns the header. The payload
// is left uninitialized.
static upb_LazyExtensionData* _upb_LazyExtension_NewBlock(upb_Arena* a,
                                                          size_t size) {
  size_t alloc_size;
  if (upb_AddOverflow(size, sizeof(upb_LazyExtensionData), &alloc_size)) {
    return NULL;
  }
  upb_LazyExtensionData* lazy = upb_Arena_Malloc(a, alloc_size);
  if (!lazy) return NULL;
  lazy->data.data = (const char*)(lazy + 1);
  lazy->data.size = size;
  return lazy;
}

upb_AddLazyExtensionStatus UPB_PRIVATE(_upb_Message_AddLazyExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e,
    const struct upb_ExtensionRegistry* registry, int options,
    upb_StringView payload, bool alias, upb_Arena* a, upb_Extension** out_ext) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(upb_MiniTableExtension_IsLazy(e));

  size_t index;
  upb_TaggedAuxPtr tagged_ptr;
  if (UPB_PRIVATE(_upb_Message_FindExtensionEntry)(msg, e, &index,
                                                   &tagged_ptr)) {
    upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
    if (upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
      // There is already a parsed value; a lazy payload cannot coexist with it
      // (merging would require a parse), so ask the caller to parse eagerly.
      *out_ext = upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);
      return kUpb_AddLazyExtension_ParseEagerly;
    }

    // A second occurrence of the same lazy extension. Non-repeated message
    // fields merge when they appear multiple times on the wire, and the
    // concatenation of the payloads parses to exactly the merged value, so we
    // coalesce them into one contiguous, non-aliased block.
    UPB_ASSERT(upb_TaggedAuxPtr_IsLazyExtension(tagged_ptr));
    upb_LazyExtensionData* old = upb_TaggedAuxPtr_LazyExtension(tagged_ptr);
    size_t new_size;
    if (upb_AddOverflow(old->data.size, payload.size, &new_size)) {
      return kUpb_AddLazyExtension_OutOfMemory;
    }
    int merged_options = _upb_LazyExtension_MergeOptions(old->options, options);

    if (!upb_TaggedAuxPtr_IsLazyExtensionAliased(tagged_ptr) &&
        old->data.data == (const char*)(old + 1)) {
      // Non-aliased block whose payload still starts right after the header,
      // so we may be able to grow the allocation in place.
      size_t old_alloc = sizeof(upb_LazyExtensionData) + old->data.size;
      size_t new_alloc = sizeof(upb_LazyExtensionData) + new_size;
      if (new_alloc > old_alloc &&
          upb_Arena_TryExtend(a, old, old_alloc, new_alloc)) {
        memcpy((char*)(old + 1) + old->data.size, payload.data, payload.size);
        old->data.size = new_size;
        old->options = merged_options;
        return kUpb_AddLazyExtension_Ok;
      }
    }

    upb_LazyExtensionData* lazy = _upb_LazyExtension_NewBlock(a, new_size);
    if (!lazy) return kUpb_AddLazyExtension_OutOfMemory;
    char* dst = (char*)(lazy + 1);
    memcpy(dst, old->data.data, old->data.size);
    memcpy(dst + old->data.size, payload.data, payload.size);
    lazy->ext = e;
    lazy->registry = registry;
    lazy->options = merged_options;
    UPB_PRIVATE(_upb_Message_Internal_SetAux)(
        in, index, upb_TaggedAuxPtr_MakeLazyExtension(lazy, false));
    return kUpb_AddLazyExtension_Ok;
  }

  // First occurrence: add a new entry.
  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, a)) {
    return kUpb_AddLazyExtension_OutOfMemory;
  }
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  upb_LazyExtensionData* lazy;
  if (alias) {
    lazy = upb_Arena_Malloc(a, sizeof(upb_LazyExtensionData));
    if (!lazy) return kUpb_AddLazyExtension_OutOfMemory;
    lazy->data = payload;
  } else {
    lazy = _upb_LazyExtension_NewBlock(a, payload.size);
    if (!lazy) return kUpb_AddLazyExtension_OutOfMemory;
    if (payload.size) memcpy(lazy + 1, payload.data, payload.size);
  }
  lazy->ext = e;
  lazy->registry = registry;
  lazy->options = options;
  UPB_PRIVATE(_upb_Message_Internal_SetAux)(
      in, in->size, upb_TaggedAuxPtr_MakeLazyExtension(lazy, alias));
  in->size++;
  return kUpb_AddLazyExtension_Ok;
}
