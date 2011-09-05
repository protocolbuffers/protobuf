
#undef NDEBUG  /* ensure tests always assert. */
#include "upb/def.h"
#include <stdlib.h>

int main() {
  upb_symtab *s = upb_symtab_new();

  // Will be empty atm since we haven't added anything to the symtab.
  int count;
  const upb_def **defs = upb_symtab_getdefs(s, &count, UPB_DEF_ANY);
  for (int i = 0; i < count; i++) {
    upb_def_unref(defs[i]);
  }
  free(defs);

  upb_symtab_unref(s);
  return 0;
}
