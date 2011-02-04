
#undef NDEBUG  /* ensure tests always assert. */
#include "upb_def.h"
#include <stdlib.h>

int main() {
  upb_symtab *s = upb_symtab_new();
  upb_symtab_add_descriptorproto(s);

  int count;
  upb_def **defs = upb_symtab_getdefs(s, &count, UPB_DEF_ANY);
  for (int i = 0; i < count; i++) {
    upb_def_unref(defs[i]);
  }
  free(defs);

  upb_string *str = upb_strdupc("google.protobuf.FileDescriptorSet");
  upb_def *fds = upb_symtab_lookup(s, str);
  assert(fds != NULL);
  assert(upb_dyncast_msgdef(fds) != NULL);
  upb_def_unref(fds);
  upb_string_unref(str);
  upb_symtab_unref(s);
  return 0;
}
