#include <stddef.h>
#include "upb/generated_code_support.h"
#include "upb/reflection/json_enumvalue_options_bootstrap.h"

#include "upb/reflection/descriptor_bootstrap.h"
static upb_Arena* upb_BootstrapArena(void) {
  static upb_Arena* arena = NULL;
  if (!arena) arena = upb_Arena_New();
  return arena;
}

const upb_MiniTable* pb__enumvalue__JsonEnumValueOptions_msg_init(void) {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$M1";
  if (mini_table) return mini_table;
  upb_Status status;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), &status);
  if (!mini_table) {
    fprintf(stderr, "Failed to build mini_table for pb__enumvalue__JsonEnumValueOptions_msg_init: %s\n",
            upb_Status_ErrorMessage(&status));
    abort();
  }
   return mini_table;
}

