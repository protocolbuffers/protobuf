
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "upb_def.h"
#include "upb_glue.h"
#include "upb_test.h"

static void test_upb_symtab() {
  upb_symtab *s = upb_symtab_new();
  ASSERT(s);
  upb_string *descriptor = upb_strreadfile("tests/test.proto.pb");
  if(!descriptor) {
    fprintf(stderr, "Couldn't read input file tests/test.proto.pb\n");
    exit(1);
  }
  upb_status status = UPB_STATUS_INIT;
  upb_parsedesc(s, descriptor, &status);
  upb_printerr(&status);
  ASSERT(upb_ok(&status));
  upb_status_uninit(&status);
  upb_string_unref(descriptor);

  // Test cycle detection by making a cyclic def's main refcount go to zero
  // and then be incremented to one again.
  upb_string *symname = upb_strdupc("A");
  upb_def *def = upb_symtab_lookup(s, symname);
  upb_string_unref(symname);
  ASSERT(def);
  upb_symtab_unref(s);
  upb_msgdef *m = upb_downcast_msgdef(def);
  upb_msg_iter i = upb_msg_begin(m);
  upb_fielddef *f = upb_msg_iter_field(i);
  ASSERT(upb_hasdef(f));
  upb_def *def2 = f->def;

  i = upb_msg_next(m, i);
  ASSERT(upb_msg_done(i));  // "A" should only have one field.

  ASSERT(upb_downcast_msgdef(def2));
  upb_def_ref(def2);
  upb_def_unref(def);
  upb_def_unref(def2);
}

int main()
{
#define TEST(func) do { \
  int assertions_before = num_assertions; \
  printf("Running " #func "..."); fflush(stdout); \
  func(); \
  printf("ok (%d assertions).\n", num_assertions - assertions_before); \
  } while (0)

  TEST(test_upb_symtab);
  printf("All tests passed (%d assertions).\n", num_assertions);
  return 0;
}
