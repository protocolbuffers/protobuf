

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb_test.h"

const char *descriptor_file;

static upb_symtab *load_test_proto() {
  upb_symtab *s = upb_symtab_new();
  ASSERT(s);
  upb_status status = UPB_STATUS_INIT;
  if (!upb_load_descriptor_file_into_symtab(s, descriptor_file, &status)) {
    fprintf(stderr, "Error loading descriptor file: %s\n",
            upb_status_getstr(&status));
    exit(1);
  }
  upb_status_uninit(&status);
  return s;
}

static upb_flow_t upb_test_onvalue(void *c, upb_value fval, upb_value val) {
  (void)c;
  (void)fval;
  (void)val;
  return UPB_CONTINUE;
}

static void test_upb_jit() {
  upb_symtab *s = load_test_proto();
  const upb_def *def = upb_symtab_lookup(s, "SimplePrimitives");
  ASSERT(def);

  upb_handlers *h = upb_handlers_new();
  upb_handlerset hset = {NULL, NULL, &upb_test_onvalue, NULL, NULL, NULL, NULL};
  upb_handlers_reghandlerset(h, upb_downcast_msgdef_const(def), &hset);
  upb_decoderplan *p = upb_decoderplan_new(h, true);
#ifdef UPB_USE_JIT_X64
  ASSERT(upb_decoderplan_hasjitcode(p));
#else
  ASSERT(!upb_decoderplan_hasjitcode(p));
#endif
  upb_decoderplan_unref(p);
  upb_symtab_unref(s);
  upb_def_unref(def);
  upb_handlers_unref(h);
}

static void test_upb_symtab() {
  upb_symtab *s = load_test_proto();

  // Test cycle detection by making a cyclic def's main refcount go to zero
  // and then be incremented to one again.
  const upb_def *def = upb_symtab_lookup(s, "A");
  ASSERT(def);
  upb_symtab_unref(s);
  const upb_msgdef *m = upb_downcast_msgdef_const(def);
  upb_msg_iter i = upb_msg_begin(m);
  ASSERT(!upb_msg_done(i));
  upb_fielddef *f = upb_msg_iter_field(i);
  ASSERT(upb_hassubdef(f));
  upb_def *def2 = f->def;

  i = upb_msg_next(m, i);
  ASSERT(upb_msg_done(i));  // "A" should only have one field.

  ASSERT(upb_downcast_msgdef(def2));
  upb_def_ref(def2);
  upb_def_unref(def);
  upb_def_unref(def2);
}

static void test_upb_two_fielddefs() {
  upb_fielddef *f1 = upb_fielddef_new();
  upb_fielddef *f2 = upb_fielddef_new();

  ASSERT(upb_fielddef_ismutable(f1));
  upb_fielddef_setname(f1, "");
  upb_fielddef_setnumber(f1, 1937);
  upb_fielddef_settype(f1, UPB_TYPE(FIXED64));
  upb_fielddef_setlabel(f1, UPB_LABEL(REPEATED));
  upb_fielddef_settypename(f1, "");
  ASSERT(upb_fielddef_number(f1) == 1937);

  ASSERT(upb_fielddef_ismutable(f2));
  upb_fielddef_setname(f2, "");
  upb_fielddef_setnumber(f2, 1572);
  upb_fielddef_settype(f2, UPB_TYPE(BYTES));
  upb_fielddef_setlabel(f2, UPB_LABEL(REPEATED));
  upb_fielddef_settypename(f2, "");
  ASSERT(upb_fielddef_number(f2) == 1572);

  upb_fielddef_unref(f1);
  upb_fielddef_unref(f2);
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Usage: test_cpp <descriptor file>\n");
    return 1;
  }
  descriptor_file = argv[1];
#define TEST(func) do { \
  int assertions_before = num_assertions; \
  printf("Running " #func "..."); fflush(stdout); \
  func(); \
  printf("ok (%d assertions).\n", num_assertions - assertions_before); \
  } while (0)

  TEST(test_upb_symtab);
  TEST(test_upb_jit);
  TEST(test_upb_two_fielddefs);
  printf("All tests passed (%d assertions).\n", num_assertions);
  return 0;
}
