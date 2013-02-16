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
  const upb_def **defs = upb_symtab_getdefs(s, UPB_DEF_ANY, NULL, &count);
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
    ASSERT(false);
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
  ASSERT(upb_def_isfrozen(def));
  upb_symtab_unref(s, &s);

  // Message A has only one subfield: "optional B b = 1".
  const upb_msgdef *m = upb_downcast_msgdef(def);
  const upb_fielddef *f = upb_msgdef_itof(m, 1);
  ASSERT(f);
  ASSERT(upb_fielddef_hassubdef(f));
  const upb_def *def2 = upb_fielddef_subdef(f);
  ASSERT(upb_downcast_msgdef(def2));
  ASSERT(strcmp(upb_def_fullname(def2), "B") == 0);

  upb_def_ref(def2, &def2);
  upb_def_unref(def, &def);

  // We know "def" is still alive because it's reachable from def2.
  ASSERT(strcmp(upb_def_fullname(def), "A") == 0);
  upb_def_unref(def2, &def2);
}

static void test_fielddef_unref() {
  upb_symtab *s = load_test_proto(&s);
  const upb_msgdef *md = upb_symtab_lookupmsg(s, "A", &md);
  const upb_fielddef *f = upb_msgdef_itof(md, 1);
  upb_fielddef_ref(f, &f);

  // Unref symtab and msgdef; now fielddef is the only thing keeping the msgdef
  // alive.
  upb_symtab_unref(s, &s);
  upb_msgdef_unref(md, &md);
  // Check that md is still alive.
  ASSERT(strcmp(upb_def_fullname(upb_upcast(md)), "A") == 0);

  // Check that unref of fielddef frees the whole remaining graph.
  upb_fielddef_unref(f, &f);
}

static void test_fielddef_accessors() {
  upb_fielddef *f1 = upb_fielddef_new(&f1);
  upb_fielddef *f2 = upb_fielddef_new(&f2);

  ASSERT(!upb_fielddef_isfrozen(f1));
  upb_fielddef_setname(f1, "f1");
  upb_fielddef_setnumber(f1, 1937);
  upb_fielddef_settype(f1, UPB_TYPE(FIXED64));
  upb_fielddef_setlabel(f1, UPB_LABEL(REPEATED));
  ASSERT(upb_fielddef_number(f1) == 1937);

  ASSERT(!upb_fielddef_isfrozen(f2));
  upb_fielddef_setname(f2, "f2");
  upb_fielddef_setnumber(f2, 1572);
  upb_fielddef_settype(f2, UPB_TYPE(BYTES));
  upb_fielddef_setlabel(f2, UPB_LABEL(REPEATED));
  ASSERT(upb_fielddef_number(f2) == 1572);

  upb_fielddef_unref(f1, &f1);
  upb_fielddef_unref(f2, &f2);

  // Test that we don't leak an unresolved subdef name.
  f1 = upb_fielddef_new(&f1);
  upb_fielddef_settype(f1, UPB_TYPE(MESSAGE));
  upb_fielddef_setsubdefname(f1, "YO");
  upb_fielddef_unref(f1, &f1);
}

static upb_fielddef *newfield(
    const char *name, int32_t num, uint8_t type, uint8_t label,
    const char *type_name, void *owner) {
  upb_fielddef *f = upb_fielddef_new(owner);
  upb_fielddef_setname(f, name);
  upb_fielddef_setnumber(f, num);
  upb_fielddef_settype(f, type);
  upb_fielddef_setlabel(f, label);
  upb_fielddef_setsubdefname(f, type_name);
  return f;
}

static upb_msgdef *upb_msgdef_newnamed(const char *name, void *owner) {
  upb_msgdef *m = upb_msgdef_new(owner);
  upb_def_setfullname(upb_upcast(m), name);
  return m;
}

INLINE upb_enumdef *upb_enumdef_newnamed(const char *name, void *owner) {
  upb_enumdef *e = upb_enumdef_new(owner);
  upb_def_setfullname(upb_upcast(e), name);
  return e;
}

static void test_replacement() {
  upb_symtab *s = upb_symtab_new(&s);

  upb_msgdef *m = upb_msgdef_newnamed("MyMessage", &s);
  upb_msgdef_addfield(m, newfield(
      "field1", 1, UPB_TYPE(ENUM), UPB_LABEL(OPTIONAL), ".MyEnum", &s), &s);
  upb_msgdef *m2 = upb_msgdef_newnamed("MyMessage2", &s);
  upb_enumdef *e = upb_enumdef_newnamed("MyEnum", &s);

  upb_def *newdefs[] = {upb_upcast(m), upb_upcast(m2), upb_upcast(e)};
  upb_status status = UPB_STATUS_INIT;
  ASSERT_STATUS(upb_symtab_add(s, newdefs, 3, &s, &status), &status);

  // Try adding a new definition of MyEnum, MyMessage should get replaced with
  // a new version.
  upb_enumdef *e2 = upb_enumdef_new(&s);
  upb_def_setfullname(upb_upcast(e2), "MyEnum");
  upb_def *newdefs2[] = {upb_upcast(e2)};
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

static void test_freeze_free() {
  // Test that freeze frees defs that were only being kept alive by virtue of
  // sharing a group with other defs that are being frozen.
  upb_msgdef *m1 = upb_msgdef_newnamed("M1", &m1);
  upb_msgdef *m2 = upb_msgdef_newnamed("M2", &m2);
  upb_msgdef *m3 = upb_msgdef_newnamed("M3", &m3);
  upb_msgdef *m4 = upb_msgdef_newnamed("M4", &m4);

  // Freeze M4 and make M1 point to it.
  upb_def_freeze((upb_def*const*)&m4, 1, NULL);

  upb_fielddef *f = upb_fielddef_new(&f);
  upb_fielddef_settype(f, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f, 1));
  ASSERT(upb_fielddef_setname(f, "foo"));
  ASSERT(upb_fielddef_setsubdef(f, upb_upcast(m4)));

  ASSERT(upb_msgdef_addfield(m1, f, &f));

  // After this unref, M1 is the only thing keeping M4 alive.
  upb_msgdef_unref(m4, &m4);

  // Force M1/M2/M3 into a single mutable refcounting group.
  f = upb_fielddef_new(&f);
  upb_fielddef_settype(f, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f, 1));
  ASSERT(upb_fielddef_setname(f, "foo"));

  ASSERT(upb_fielddef_setsubdef(f, upb_upcast(m1)));
  ASSERT(upb_fielddef_setsubdef(f, upb_upcast(m2)));
  ASSERT(upb_fielddef_setsubdef(f, upb_upcast(m3)));

  // Make M3 cyclic with itself.
  ASSERT(upb_msgdef_addfield(m3, f, &f));

  // These will be kept alive since they are in the same refcounting group as
  // M3, which still has a ref.  Note: this behavior is not guaranteed by the
  // API, but true in practice with its current implementation.
  upb_msgdef_unref(m1, &m1);
  upb_msgdef_unref(m2, &m2);

  // Test that they are still alive (NOT allowed by the API).
  ASSERT(strcmp("M1", upb_def_fullname(upb_upcast(m1))) == 0);
  ASSERT(strcmp("M2", upb_def_fullname(upb_upcast(m2))) == 0);

  // Freeze M3.  If the test leaked no memory, then freeing m1 and m2 was
  // successful.
  ASSERT(upb_def_freeze((upb_def*const*)&m3, 1, NULL));

  upb_msgdef_unref(m3, &m3);
}

static void test_partial_freeze() {
  // Test that freeze of only part of the graph correctly adjusts objects that
  // point to the newly-frozen objects.
  upb_msgdef *m1 = upb_msgdef_newnamed("M1", &m1);
  upb_msgdef *m2 = upb_msgdef_newnamed("M2", &m2);
  upb_msgdef *m3 = upb_msgdef_newnamed("M3", &m3);

  upb_fielddef *f1 = upb_fielddef_new(&f1);
  upb_fielddef_settype(f1, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f1, 1));
  ASSERT(upb_fielddef_setname(f1, "f1"));
  ASSERT(upb_fielddef_setsubdef(f1, upb_upcast(m1)));

  upb_fielddef *f2 = upb_fielddef_new(&f2);
  upb_fielddef_settype(f2, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f2, 2));
  ASSERT(upb_fielddef_setname(f2, "f2"));
  ASSERT(upb_fielddef_setsubdef(f2, upb_upcast(m2)));

  ASSERT(upb_msgdef_addfield(m3, f1, &f1));
  ASSERT(upb_msgdef_addfield(m3, f2, &f2));

  // Freeze M1 and M2, which should cause the group to be split
  // and m3 (left mutable) to take references on m1 and m2.
  upb_def *defs[] = {upb_upcast(m1), upb_upcast(m2)};
  ASSERT(upb_def_freeze(defs, 2, NULL));

  ASSERT(upb_msgdef_isfrozen(m1));
  ASSERT(upb_msgdef_isfrozen(m2));
  ASSERT(!upb_msgdef_isfrozen(m3));

  upb_msgdef_unref(m1, &m1);
  upb_msgdef_unref(m2, &m2);
  upb_msgdef_unref(m3, &m3);
}

int run_tests(int argc, char *argv[]) {
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
  test_freeze_free();
  test_partial_freeze();
  return 0;
}
