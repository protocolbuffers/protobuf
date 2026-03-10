// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_GENERATED_EXTENSION_REGISTRY_H_
#define UPB_MINI_TABLE_INTERNAL_GENERATED_EXTENSION_REGISTRY_H_

#include "upb/mini_table/internal/extension.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UPB_PRIVATE(upb_GeneratedExtensionListEntry) {
  const struct upb_MiniTableExtension* start;
  const struct upb_MiniTableExtension* stop;
  const struct UPB_PRIVATE(upb_GeneratedExtensionListEntry) * next;
} UPB_PRIVATE(upb_GeneratedExtensionListEntry);

struct upb_GeneratedRegistryRef {
  struct upb_Arena* UPB_PRIVATE(arena);
  const struct upb_ExtensionRegistry* UPB_PRIVATE(registry);
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_GENERATED_EXTENSION_REGISTRY_H_ */
