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
  upb_symtab_iter i;
  for (upb_symtab_begin(&i, s, UPB_DEF_ANY); !upb_symtab_done(&i);
       upb_symtab_next(&i)) {
    ASSERT(false);  // Should not get here.
  }
  upb_symtab_unref(s, &s);
}

static void test_noreftracking() {
  // Reftracking is not required; clients can pass UPB_UNTRACKED_REF for owner.
  upb_msgdef *md = upb_msgdef_new(UPB_UNTRACKED_REF);
  upb_msgdef_ref(md, UPB_UNTRACKED_REF);

  // Clients can mix tracked and untracked refs.
  upb_msgdef_ref(md, &md);

  upb_msgdef_unref(md, UPB_UNTRACKED_REF);
  upb_msgdef_unref(md, UPB_UNTRACKED_REF);

  // Call some random function on the messagedef to test that it is alive.
  ASSERT(!upb_msgdef_isfrozen(md));

  upb_msgdef_unref(md, &md);
}

static upb_symtab *load_test_proto(void *owner) {
  upb_symtab *s = upb_symtab_new(owner);
  ASSERT(s);
  upb_status status = UPB_STATUS_INIT;
  if (!upb_load_descriptor_file_into_symtab(s, descriptor_file, &status)) {
    fprintf(stderr, "Error loading descriptor file: %s\n",
            upb_status_errmsg(&status));
    ASSERT(false);
  }
  ASSERT(!upb_symtab_isfrozen(s));
  upb_symtab_freeze(s);
  ASSERT(upb_symtab_isfrozen(s));
  return s;
}

static void test_cycles() {
  upb_symtab *s = load_test_proto(&s);

  // Test cycle detection by making a cyclic def's main refcount go to zero
  // and then be incremented to one again.
  const upb_def *def = upb_symtab_lookup(s, "A");
  upb_def_ref(def, &def);
  ASSERT(def);
  ASSERT(upb_def_isfrozen(def));
  upb_symtab_unref(s, &s);

  // Message A has only one subfield: "optional B b = 1".
  const upb_msgdef *m = upb_downcast_msgdef(def);
  const upb_fielddef *f = upb_msgdef_itof(m, 1);
  ASSERT(f);
  ASSERT(upb_fielddef_hassubdef(f));
  ASSERT(upb_msgdef_ntofz(m, "b") == f);
  ASSERT(upb_msgdef_ntof(m, "b", 1) == f);
  const upb_def *def2 = upb_fielddef_subdef(f);
  ASSERT(upb_downcast_msgdef(def2));
  ASSERT(strcmp(upb_def_fullname(def2), "B") == 0);

  upb_def_ref(def2, &def2);
  upb_def_unref(def, &def);

  // We know "def" is still alive because it's reachable from def2.
  ASSERT(strcmp(upb_def_fullname(def), "A") == 0);
  upb_def_unref(def2, &def2);
}

static void test_symbol_resolution() {
  upb_status s = UPB_STATUS_INIT;

  upb_symtab *symtab = upb_symtab_new(&symtab);
  ASSERT(symtab);

  // m1 has name "A.B.C" and no fields. We'll add it to the symtab now.
  upb_msgdef *m1 = upb_msgdef_new(&m1);
  ASSERT(m1);
  ASSERT_STATUS(upb_msgdef_setfullname(m1, "A.B.C", &s), &s);
  ASSERT_STATUS(upb_symtab_add(symtab, (upb_def**)&m1, 1,
                               NULL, &s), &s);

  // m2 has name "D.E" and no fields. We'll add it in the same batch as m3
  // below.
  upb_msgdef *m2 = upb_msgdef_new(&m2);
  ASSERT(m2);
  ASSERT_STATUS(upb_msgdef_setfullname(m2, "D.E", &s), &s);

  // m3 has name "F.G" and two fields, of type A.B.C and D.E respectively. We'll
  // add it in the same batch as m2 above.
  upb_msgdef *m3 = upb_msgdef_new(&m3);
  ASSERT(m3);
  ASSERT_STATUS(upb_msgdef_setfullname(m3, "F.G", &s), &s);
  upb_fielddef *m3_field1 = upb_fielddef_new(&m3_field1);
  ASSERT_STATUS(upb_fielddef_setname(m3_field1, "field1", &s), &s);
  ASSERT_STATUS(upb_fielddef_setnumber(m3_field1, 1, &s), &s);
  upb_fielddef_setlabel(m3_field1, UPB_LABEL_OPTIONAL);
  upb_fielddef_settype(m3_field1, UPB_TYPE_MESSAGE);
  ASSERT_STATUS(upb_fielddef_setsubdefname(m3_field1, ".A.B.C", &s), &s);
  ASSERT_STATUS(upb_msgdef_addfield(m3, m3_field1, NULL, &s), &s);

  upb_fielddef *m3_field2 = upb_fielddef_new(&m3_field2);
  ASSERT_STATUS(upb_fielddef_setname(m3_field2, "field2", &s), &s);
  ASSERT_STATUS(upb_fielddef_setnumber(m3_field2, 2, &s), &s);
  upb_fielddef_setlabel(m3_field2, UPB_LABEL_OPTIONAL);
  upb_fielddef_settype(m3_field2, UPB_TYPE_MESSAGE);
  ASSERT_STATUS(upb_fielddef_setsubdefname(m3_field2, ".D.E", &s), &s);
  ASSERT_STATUS(upb_msgdef_addfield(m3, m3_field2, NULL, &s), &s);

  upb_def *defs[2] = { (upb_def*)m2, (upb_def*)m3 };
  ASSERT_STATUS(upb_symtab_add(symtab, defs, 2, NULL, &s), &s);

  upb_fielddef_unref(m3_field2, &m3_field2);
  upb_fielddef_unref(m3_field1, &m3_field1);
  upb_msgdef_unref(m3, &m3);
  upb_msgdef_unref(m2, &m2);
  upb_msgdef_unref(m1, &m1);
  upb_symtab_unref(symtab, &symtab);
}

static void test_fielddef_unref() {
  upb_symtab *s = load_test_proto(&s);
  const upb_msgdef *md = upb_symtab_lookupmsg(s, "A");
  const upb_fielddef *f = upb_msgdef_itof(md, 1);
  upb_fielddef_ref(f, &f);

  // Unref symtab; now fielddef is the only thing keeping the msgdef alive.
  upb_symtab_unref(s, &s);
  // Check that md is still alive.
  ASSERT(strcmp(upb_msgdef_fullname(md), "A") == 0);

  // Check that unref of fielddef frees the whole remaining graph.
  upb_fielddef_unref(f, &f);
}

static void test_fielddef_accessors() {
  upb_fielddef *f1 = upb_fielddef_new(&f1);
  upb_fielddef *f2 = upb_fielddef_new(&f2);

  ASSERT(!upb_fielddef_isfrozen(f1));
  ASSERT(upb_fielddef_setname(f1, "f1", NULL));
  ASSERT(upb_fielddef_setnumber(f1, 1937, NULL));
  upb_fielddef_settype(f1, UPB_TYPE_INT64);
  upb_fielddef_setlabel(f1, UPB_LABEL_REPEATED);
  ASSERT(upb_fielddef_number(f1) == 1937);

  ASSERT(!upb_fielddef_isfrozen(f2));
  ASSERT(upb_fielddef_setname(f2, "f2", NULL));
  ASSERT(upb_fielddef_setnumber(f2, 123456789, NULL));
  upb_fielddef_settype(f2, UPB_TYPE_BYTES);
  upb_fielddef_setlabel(f2, UPB_LABEL_REPEATED);
  ASSERT(upb_fielddef_number(f2) == 123456789);

  upb_fielddef_unref(f1, &f1);
  upb_fielddef_unref(f2, &f2);

  // Test that we don't leak an unresolved subdef name.
  f1 = upb_fielddef_new(&f1);
  upb_fielddef_settype(f1, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setsubdefname(f1, "YO", NULL));
  upb_fielddef_unref(f1, &f1);
}

static upb_fielddef *newfield(
    const char *name, int32_t num, uint8_t type, uint8_t label,
    const char *type_name, void *owner) {
  upb_fielddef *f = upb_fielddef_new(owner);
  ASSERT(upb_fielddef_setname(f, name, NULL));
  ASSERT(upb_fielddef_setnumber(f, num, NULL));
  upb_fielddef_settype(f, type);
  upb_fielddef_setlabel(f, label);
  if (type_name) {
    ASSERT(upb_fielddef_setsubdefname(f, type_name, NULL));
  }
  return f;
}

static upb_msgdef *upb_msgdef_newnamed(const char *name, void *owner) {
  upb_msgdef *m = upb_msgdef_new(owner);
  upb_msgdef_setfullname(m, name, NULL);
  return m;
}

static upb_enumdef *upb_enumdef_newnamed(const char *name, void *owner) {
  upb_enumdef *e = upb_enumdef_new(owner);
  upb_enumdef_setfullname(e, name, NULL);
  return e;
}

static void test_replacement() {
  upb_symtab *s = upb_symtab_new(&s);

  upb_msgdef *m = upb_msgdef_newnamed("MyMessage", &s);
  upb_msgdef_addfield(m, newfield("field1", 1, UPB_TYPE_ENUM,
                                  UPB_LABEL_OPTIONAL, ".MyEnum", &s),
                      &s, NULL);
  upb_msgdef *m2 = upb_msgdef_newnamed("MyMessage2", &s);
  upb_enumdef *e = upb_enumdef_newnamed("MyEnum", &s);
  upb_status status = UPB_STATUS_INIT;
  ASSERT_STATUS(upb_enumdef_addval(e, "VAL1", 1, &status), &status);

  upb_def *newdefs[] = {UPB_UPCAST(m), UPB_UPCAST(m2), UPB_UPCAST(e)};
  ASSERT_STATUS(upb_symtab_add(s, newdefs, 3, &s, &status), &status);

  // Try adding a new definition of MyEnum, MyMessage should get replaced with
  // a new version.
  upb_enumdef *e2 = upb_enumdef_newnamed("MyEnum", &s);
  ASSERT_STATUS(upb_enumdef_addval(e2, "VAL1", 1, &status), &status);
  upb_def *newdefs2[] = {UPB_UPCAST(e2)};
  ASSERT_STATUS(upb_symtab_add(s, newdefs2, 1, &s, &status), &status);

  const upb_msgdef *m3 = upb_symtab_lookupmsg(s, "MyMessage");
  ASSERT(m3);
  // Must be different because it points to MyEnum which was replaced.
  ASSERT(m3 != m);

  m3 = upb_symtab_lookupmsg(s, "MyMessage2");
  // Should be the same because it was not replaced, nor were any defs that
  // are reachable from it.
  ASSERT(m3 == m2);

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
  ASSERT(upb_fielddef_setnumber(f, 1, NULL));
  ASSERT(upb_fielddef_setname(f, "foo", NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f, m4, NULL));

  ASSERT(upb_msgdef_addfield(m1, f, &f, NULL));

  // After this unref, M1 is the only thing keeping M4 alive.
  upb_msgdef_unref(m4, &m4);

  // Force M1/M2/M3 into a single mutable refcounting group.
  f = upb_fielddef_new(&f);
  upb_fielddef_settype(f, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f, 1, NULL));
  ASSERT(upb_fielddef_setname(f, "foo", NULL));

  ASSERT(upb_fielddef_setmsgsubdef(f, m1, NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f, m2, NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f, m3, NULL));

  // Make M3 cyclic with itself.
  ASSERT(upb_msgdef_addfield(m3, f, &f, NULL));

  // These will be kept alive since they are in the same refcounting group as
  // M3, which still has a ref.  Note: this behavior is not guaranteed by the
  // API, but true in practice with its current implementation.
  upb_msgdef_unref(m1, &m1);
  upb_msgdef_unref(m2, &m2);

  // Test that they are still alive (NOT allowed by the API).
  ASSERT(strcmp("M1", upb_msgdef_fullname(m1)) == 0);
  ASSERT(strcmp("M2", upb_msgdef_fullname(m2)) == 0);

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
  ASSERT(upb_fielddef_setnumber(f1, 1, NULL));
  ASSERT(upb_fielddef_setname(f1, "f1", NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f1, m1, NULL));

  upb_fielddef *f2 = upb_fielddef_new(&f2);
  upb_fielddef_settype(f2, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f2, 2, NULL));
  ASSERT(upb_fielddef_setname(f2, "f2", NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f2, m2, NULL));

  ASSERT(upb_msgdef_addfield(m3, f1, &f1, NULL));
  ASSERT(upb_msgdef_addfield(m3, f2, &f2, NULL));

  // Freeze M1 and M2, which should cause the group to be split
  // and m3 (left mutable) to take references on m1 and m2.
  upb_def *defs[] = {UPB_UPCAST(m1), UPB_UPCAST(m2)};
  ASSERT(upb_def_freeze(defs, 2, NULL));

  ASSERT(upb_msgdef_isfrozen(m1));
  ASSERT(upb_msgdef_isfrozen(m2));
  ASSERT(!upb_msgdef_isfrozen(m3));

  upb_msgdef_unref(m1, &m1);
  upb_msgdef_unref(m2, &m2);
  upb_msgdef_unref(m3, &m3);
}

static void test_descriptor_flags() {
  upb_msgdef *m = upb_msgdef_new(&m);
  ASSERT(upb_msgdef_mapentry(m) == false);
  upb_status s = UPB_STATUS_INIT;
  upb_msgdef_setfullname(m, "TestMessage", &s);
  ASSERT(upb_ok(&s));
  upb_msgdef_setmapentry(m, true);
  ASSERT(upb_msgdef_mapentry(m) == true);
  upb_msgdef *m2 = upb_msgdef_dup(m, &m2);
  ASSERT(upb_msgdef_mapentry(m2) == true);
  upb_msgdef_unref(m, &m);
  upb_msgdef_unref(m2, &m2);
}

static void test_oneofs() {
  upb_status s = UPB_STATUS_INIT;
  bool ok = true;

  upb_symtab *symtab = upb_symtab_new(&symtab);
  ASSERT(symtab != NULL);

  // Create a test message for fields to refer to.
  upb_msgdef *subm = upb_msgdef_newnamed("SubMessage", &symtab);
  upb_msgdef_addfield(subm, newfield("field1", 1, UPB_TYPE_INT32,
                                     UPB_LABEL_OPTIONAL, NULL, &symtab),
                      &symtab, NULL);
  upb_def *subm_defs[] = {UPB_UPCAST(subm)};
  ASSERT_STATUS(upb_symtab_add(symtab, subm_defs, 1, &symtab, &s), &s);

  upb_msgdef *m = upb_msgdef_newnamed("TestMessage", &symtab);
  ASSERT(upb_msgdef_numoneofs(m) == 0);

  upb_oneofdef *o = upb_oneofdef_new(&o);
  ASSERT(upb_oneofdef_numfields(o) == 0);
  ASSERT(upb_oneofdef_name(o) == NULL);

  ok = upb_oneofdef_setname(o, "test_oneof", &s);
  ASSERT_STATUS(ok, &s);

  ok = upb_oneofdef_addfield(o, newfield("field1", 1, UPB_TYPE_INT32,
                                         UPB_LABEL_OPTIONAL, NULL, &symtab),
                             &symtab, NULL);
  ASSERT_STATUS(ok, &s);
  ok = upb_oneofdef_addfield(o, newfield("field2", 2, UPB_TYPE_MESSAGE,
                                         UPB_LABEL_OPTIONAL, ".SubMessage",
                                         &symtab),
                             &symtab, NULL);
  ASSERT_STATUS(ok, &s);

  ok = upb_msgdef_addoneof(m, o, NULL, &s);
  ASSERT_STATUS(ok, &s);

  upb_def *defs[] = {UPB_UPCAST(m)};
  ASSERT_STATUS(upb_symtab_add(symtab, defs, 1, &symtab, &s), &s);

  ASSERT(upb_msgdef_numoneofs(m) == 1);
  const upb_oneofdef *lookup_o = upb_msgdef_ntooz(m, "test_oneof");
  ASSERT(lookup_o == o);

  const upb_fielddef *lookup_field = upb_oneofdef_ntofz(o, "field1");
  ASSERT(lookup_field != NULL && upb_fielddef_number(lookup_field) == 1);

  upb_symtab_unref(symtab, &symtab);
  upb_oneofdef_unref(o, &o);
}

int run_tests(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: test_def <test.proto.pb>\n");
    return 1;
  }
  descriptor_file = argv[1];
  test_empty_symtab();
  test_cycles();
  test_symbol_resolution();
  test_fielddef_accessors();
  test_fielddef_unref();
  test_replacement();
  test_freeze_free();
  test_partial_freeze();
  test_noreftracking();
  test_descriptor_flags();
  test_oneofs();
  return 0;
}
