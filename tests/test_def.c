/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 *
 * Test of defs and symtab.  There should be far more tests of edge conditions
 * (like attempts to link defs that don't have required properties set).
 */

#include "upb/def.h"
#include "upb/pb/glue.h"
#include "upb_test.h"
#include <stdlib.h>
#include <string.h>

const char *descriptor_file;

static void test_empty_symtab() {
  upb_symtab *s = upb_symtab_new(&s);
  int count;
  const upb_def **defs = upb_symtab_getdefs(s, &count, UPB_DEF_ANY, NULL);
  ASSERT(count == 0);
  free(defs);
  upb_symtab_unref(s, &s);
}

static upb_symtab *load_test_proto(void *owner) {
  upb_symtab *s = upb_symtab_new(owner);
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

static void test_cycles() {
  upb_symtab *s = load_test_proto(&s);

  // Test cycle detection by making a cyclic def's main refcount go to zero
  // and then be incremented to one again.
  const upb_def *def = upb_symtab_lookup(s, "A", &def);
  ASSERT(def);
  ASSERT(upb_def_isfinalized(def));
  upb_symtab_unref(s, &s);

  // Message A has only one subfield: "optional B b = 1".
  const upb_msgdef *m = upb_downcast_msgdef_const(def);
  upb_fielddef *f = upb_msgdef_itof(m, 1);
  ASSERT(f);
  ASSERT(upb_hassubdef(f));
  const upb_def *def2 = upb_fielddef_subdef(f);
  ASSERT(upb_downcast_msgdef_const(def2));
  ASSERT(strcmp(upb_def_fullname(def2), "B") == 0);

  upb_def_ref(def2, &def2);
  upb_def_unref(def, &def);
  upb_def_unref(def2, &def2);
}

static void test_fielddef_unref() {
  upb_symtab *s = load_test_proto(&s);
  const upb_msgdef *md = upb_symtab_lookupmsg(s, "A", &md);
  upb_fielddef *f = upb_msgdef_itof(md, 1);
  upb_fielddef_ref(f, &f);

  // Unref symtab and msgdef; now fielddef is the only thing keeping the msgdef
  // alive.
  upb_symtab_unref(s, &s);
  upb_msgdef_unref(md, &md);
  // Check that md is still alive.
  ASSERT(strcmp(upb_def_fullname(UPB_UPCAST(md)), "A") == 0);

  // Check that unref of fielddef frees the whole remaining graph.
  upb_fielddef_unref(f, &f);
}

static void test_fielddef_accessors() {
  upb_fielddef *f1 = upb_fielddef_new(&f1);
  upb_fielddef *f2 = upb_fielddef_new(&f2);

  ASSERT(upb_fielddef_ismutable(f1));
  upb_fielddef_setname(f1, "f1");
  upb_fielddef_setnumber(f1, 1937);
  upb_fielddef_settype(f1, UPB_TYPE(FIXED64));
  upb_fielddef_setlabel(f1, UPB_LABEL(REPEATED));
  ASSERT(upb_fielddef_number(f1) == 1937);

  ASSERT(upb_fielddef_ismutable(f2));
  upb_fielddef_setname(f2, "f2");
  upb_fielddef_setnumber(f2, 1572);
  upb_fielddef_settype(f2, UPB_TYPE(BYTES));
  upb_fielddef_setlabel(f2, UPB_LABEL(REPEATED));
  ASSERT(upb_fielddef_number(f2) == 1572);

  upb_fielddef_unref(f1, &f1);
  upb_fielddef_unref(f2, &f2);
}

static upb_fielddef *newfield(
    const char *name, int32_t num, uint8_t type, uint8_t label,
    const char *type_name, void *owner) {
  upb_fielddef *f = upb_fielddef_new(owner);
  upb_fielddef_setname(f, name);
  upb_fielddef_setnumber(f, num);
  upb_fielddef_settype(f, type);
  upb_fielddef_setlabel(f, label);
  upb_fielddef_setsubtypename(f, type_name);
  return f;
}

static upb_msgdef *upb_msgdef_newnamed(const char *name, void *owner) {
  upb_msgdef *m = upb_msgdef_new(owner);
  upb_def_setfullname(UPB_UPCAST(m), name);
  return m;
}

INLINE upb_enumdef *upb_enumdef_newnamed(const char *name, void *owner) {
  upb_enumdef *e = upb_enumdef_new(owner);
  upb_def_setfullname(UPB_UPCAST(e), name);
  return e;
}

void test_replacement() {
  upb_symtab *s = upb_symtab_new(&s);

  upb_msgdef *m = upb_msgdef_newnamed("MyMessage", &s);
  upb_msgdef_addfield(m, newfield(
      "field1", 1, UPB_TYPE(ENUM), UPB_LABEL(OPTIONAL), ".MyEnum", &s), &s);
  upb_msgdef *m2 = upb_msgdef_newnamed("MyMessage2", &s);
  upb_enumdef *e = upb_enumdef_newnamed("MyEnum", &s);

  upb_def *newdefs[] = {UPB_UPCAST(m), UPB_UPCAST(m2), UPB_UPCAST(e)};
  upb_status status = UPB_STATUS_INIT;
  ASSERT_STATUS(upb_symtab_add(s, newdefs, 3, &s, &status), &status);

  // Try adding a new definition of MyEnum, MyMessage should get replaced with
  // a new version.
  upb_enumdef *e2 = upb_enumdef_new(&s);
  upb_def_setfullname(UPB_UPCAST(e2), "MyEnum");
  upb_def *newdefs2[] = {UPB_UPCAST(e2)};
  ASSERT_STATUS(upb_symtab_add(s, newdefs2, 1, &s, &status), &status);

  const upb_msgdef *m3 = upb_symtab_lookupmsg(s, "MyMessage", &m3);
  ASSERT(m3);
  // Must be different because it points to MyEnum which was replaced.
  ASSERT(m3 != m);
  upb_msgdef_unref(m3, &m3);

  m3 = upb_symtab_lookupmsg(s, "MyMessage2", &m3);
  // Should be the same because it was not replaced, nor were any defs that
  // are reachable from it.
  ASSERT(m3 == m2);
  upb_msgdef_unref(m3, &m3);

  upb_symtab_unref(s, &s);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: test_def <test.proto.pb>\n");
    return 1;
  }
  descriptor_file = argv[1];
  test_empty_symtab();
  test_cycles();
  test_fielddef_accessors();
  test_fielddef_unref();
  test_replacement();
  return 0;
}
