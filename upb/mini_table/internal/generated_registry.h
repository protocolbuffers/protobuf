#ifndef UPB_MINI_TABLE_INTERNAL_GENERATED_EXTENSION_REGISTRY_H_
#define UPB_MINI_TABLE_INTERNAL_GENERATED_EXTENSION_REGISTRY_H_

#include "upb/mini_table/internal/extension.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_GeneratedExtensionListEntry {
  const struct upb_MiniTableExtension* start;
  const struct upb_MiniTableExtension* stop;
  const struct upb_GeneratedExtensionListEntry* next;
} upb_GeneratedExtensionListEntry;

struct upb_GeneratedRegistryRef {
  struct upb_Arena* arena;
  const struct upb_ExtensionRegistry* registry;
};

extern const upb_GeneratedExtensionListEntry* UPB_PRIVATE(
    upb_generated_extension_list);

// This function is defined in generated code.  It is responsible for adding
// all of the extensions for the binary to the global registry.
// __attribute__((weak, visibility("hidden"), constructor)) void UPB_PRIVATE(
//    upb_GeneratedRegistry_Constructor)(void);

void UPB_PRIVATE(upb_GeneratedRegistry_Register)(
    upb_GeneratedExtensionListEntry* entry);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_GENERATED_EXTENSION_REGISTRY_H_ */
