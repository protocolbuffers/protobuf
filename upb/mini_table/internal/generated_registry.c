#include "upb/mini_table/internal/generated_registry.h"

// Must be last.
#include "upb/port/def.inc"

const upb_GeneratedExtensionListEntry* UPB_PRIVATE(
    upb_generated_extension_list) = NULL;

void UPB_PRIVATE(upb_GeneratedRegistry_Register)(
    upb_GeneratedExtensionListEntry* entry) {
  UPB_ASSERT(entry->next == NULL);
  entry->next = UPB_PRIVATE(upb_generated_extension_list);
  UPB_PRIVATE(upb_generated_extension_list) = entry;
}
