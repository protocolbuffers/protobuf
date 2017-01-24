/*
** Test of defs and symtab.  There should be far more tests of edge conditions
** (like attempts to link defs that don't have required properties set).
*/

#include "tests/test_util.h"
#include "upb/def.h"
#include "upb/pb/glue.h"
#include "upb_test.h"
#include <stdlib.h>
#include <string.h>

const char *descriptor_file;

static void test_empty_symtab() {
  upb_symtab *s = upb_symtab_new();
  upb_symtab_iter i;
  for (upb_symtab_begin(&i, s, UPB_DEF_ANY); !upb_symtab_done(&i);
       upb_symtab_next(&i)) {
    ASSERT(false);  /* Should not get here. */
  }
  upb_symtab_free(s);
}

static void test_noreftracking() {
  /* Reftracking is not required; clients can pass UPB_UNTRACKED_REF for owner. */
  upb_msgdef *md = upb_msgdef_new(UPB_UNTRACKED_REF);
  upb_msgdef_ref(md, UPB_UNTRACKED_REF);

  /* Clients can mix tracked and untracked refs. */
  upb_msgdef_ref(md, &md);

  upb_msgdef_unref(md, UPB_UNTRACKED_REF);
  upb_msgdef_unref(md, UPB_UNTRACKED_REF);

  /* Call some random function on the messagedef to test that it is alive. */
  ASSERT(!upb_msgdef_isfrozen(md));

  upb_msgdef_unref(md, &md);
}

static upb_symtab *load_test_proto() {
  upb_symtab *s = upb_symtab_new();
  upb_status status = UPB_STATUS_INIT;
  size_t len;
  char *data = upb_readfile(descriptor_file, &len);
  upb_filedef **files, **files_ptr;
  ASSERT(s);
  ASSERT(data);
  files = upb_loaddescriptor(data, len, &files, &status);
  ASSERT(files);
  free(data);

  files_ptr = files;
  while (*files_ptr) {
    bool ok;
    ASSERT(!upb_filedef_isfrozen(*files_ptr));
    ok = upb_symtab_addfile(s, *files_ptr, &status);
    ASSERT(ok);
    ASSERT(upb_filedef_isfrozen(*files_ptr));
    upb_filedef_unref(*files_ptr, &files);
    files_ptr++;
  }

  upb_gfree(files);

  return s;
}

static void test_cycles() {
  bool ok;
  upb_symtab *s = load_test_proto();
  const upb_msgdef *m;
  const upb_fielddef *f;
  const upb_def *def;
  const upb_def *def2;

  /* Test cycle detection by making a cyclic def's main refcount go to zero
   * and then be incremented to one again. */
  def = upb_symtab_lookup(s, "A");
  upb_def_ref(def, &def);
  ASSERT(def);
  ASSERT(upb_def_isfrozen(def));
  upb_symtab_free(s);

  /* Message A has only one subfield: "optional B b = 1". */
  m = upb_downcast_msgdef(def);
  f = upb_msgdef_itof(m, 1);
  ASSERT(f);
  ASSERT(upb_fielddef_hassubdef(f));
  ASSERT(upb_msgdef_ntofz(m, "b") == f);
  ASSERT(upb_msgdef_ntof(m, "b", 1) == f);
  def2 = upb_fielddef_subdef(f);
  ASSERT(upb_downcast_msgdef(def2));
  ok = strcmp(upb_def_fullname(def2), "B") == 0;
  ASSERT(ok);

  upb_def_ref(def2, &def2);
  upb_def_unref(def, &def);

  /* We know "def" is still alive because it's reachable from def2. */
  ok = strcmp(upb_def_fullname(def), "A") == 0;
  ASSERT(ok);
  upb_def_unref(def2, &def2);
}

static void test_symbol_resolution() {
  upb_status s = UPB_STATUS_INIT;
  upb_def *defs[2];
  upb_msgdef *m1;
  upb_msgdef *m2;
  upb_msgdef *m3;
  upb_fielddef *m3_field1;
  upb_fielddef *m3_field2;

  upb_symtab *symtab = upb_symtab_new(&symtab);
  ASSERT(symtab);

  /* m1 has name "A.B.C" and no fields. We'll add it to the symtab now. */
  m1 = upb_msgdef_new(&m1);
  ASSERT(m1);
  ASSERT_STATUS(upb_msgdef_setfullname(m1, "A.B.C", &s), &s);
  ASSERT_STATUS(upb_symtab_add(symtab, (upb_def**)&m1, 1,
                               NULL, &s), &s);

  /* m2 has name "D.E" and no fields. We'll add it in the same batch as m3
   * below. */
  m2 = upb_msgdef_new(&m2);
  ASSERT(m2);
  ASSERT_STATUS(upb_msgdef_setfullname(m2, "D.E", &s), &s);

  /* m3 has name "F.G" and two fields, of type A.B.C and D.E respectively. We'll
   * add it in the same batch as m2 above. */
  m3 = upb_msgdef_new(&m3);
  ASSERT(m3);
  ASSERT_STATUS(upb_msgdef_setfullname(m3, "F.G", &s), &s);
  m3_field1 = upb_fielddef_new(&m3_field1);
  ASSERT_STATUS(upb_fielddef_setname(m3_field1, "field1", &s), &s);
  ASSERT_STATUS(upb_fielddef_setnumber(m3_field1, 1, &s), &s);
  upb_fielddef_setlabel(m3_field1, UPB_LABEL_OPTIONAL);
  upb_fielddef_settype(m3_field1, UPB_TYPE_MESSAGE);
  ASSERT_STATUS(upb_fielddef_setsubdefname(m3_field1, ".A.B.C", &s), &s);
  ASSERT_STATUS(upb_msgdef_addfield(m3, m3_field1, NULL, &s), &s);

  m3_field2 = upb_fielddef_new(&m3_field2);
  ASSERT_STATUS(upb_fielddef_setname(m3_field2, "field2", &s), &s);
  ASSERT_STATUS(upb_fielddef_setnumber(m3_field2, 2, &s), &s);
  upb_fielddef_setlabel(m3_field2, UPB_LABEL_OPTIONAL);
  upb_fielddef_settype(m3_field2, UPB_TYPE_MESSAGE);
  ASSERT_STATUS(upb_fielddef_setsubdefname(m3_field2, ".D.E", &s), &s);
  ASSERT_STATUS(upb_msgdef_addfield(m3, m3_field2, NULL, &s), &s);

  defs[0] = upb_msgdef_upcast_mutable(m2);
  defs[1] = upb_msgdef_upcast_mutable(m3);
  ASSERT_STATUS(upb_symtab_add(symtab, defs, 2, NULL, &s), &s);

  upb_fielddef_unref(m3_field2, &m3_field2);
  upb_fielddef_unref(m3_field1, &m3_field1);
  upb_msgdef_unref(m3, &m3);
  upb_msgdef_unref(m2, &m2);
  upb_msgdef_unref(m1, &m1);
  upb_symtab_free(symtab);
}

static void test_fielddef_unref() {
  bool ok;
  upb_symtab *s = load_test_proto();
  const upb_msgdef *md = upb_symtab_lookupmsg(s, "A");
  const upb_fielddef *f = upb_msgdef_itof(md, 1);
  upb_fielddef_ref(f, &f);

  /* Unref symtab; now fielddef is the only thing keeping the msgdef alive. */
  upb_symtab_free(s);
  /* Check that md is still alive. */
  ok = strcmp(upb_msgdef_fullname(md), "A") == 0;
  ASSERT(ok);

  /* Check that unref of fielddef frees the whole remaining graph. */
  upb_fielddef_unref(f, &f);
}

static void test_fielddef() {
  /* Test that we don't leak an unresolved subdef name. */
  upb_fielddef *f1 = upb_fielddef_new(&f1);
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

static void test_replacement_fails() {
  bool ok;
  upb_symtab *s = upb_symtab_new(&s);
  upb_status status = UPB_STATUS_INIT;
  upb_def *newdefs[2];

  upb_msgdef *m = upb_msgdef_newnamed("MyMessage", &s);
  upb_msgdef *m2 = upb_msgdef_newnamed("MyMessage", &s);

  newdefs[0] = upb_msgdef_upcast_mutable(m);
  newdefs[1] = upb_msgdef_upcast_mutable(m2);
  ok = upb_symtab_add(s, newdefs, 2, &s, &status);
  ASSERT(ok == false);
  upb_status_clear(&status);

  /* Adding just one is ok. */
  ASSERT_STATUS(upb_symtab_add(s, newdefs, 1, &s, &status), &status);

  /* Adding a conflicting one is not ok. */
  newdefs[0] = upb_msgdef_upcast_mutable(m2);
  ok = upb_symtab_add(s, newdefs, 1, &s, &status);
  ASSERT(ok == false);

  upb_symtab_free(s);
}

static void test_freeze_free() {
  bool ok;

  /* Test that freeze frees defs that were only being kept alive by virtue of
   * sharing a group with other defs that are being frozen. */
  upb_msgdef *m1 = upb_msgdef_newnamed("M1", &m1);
  upb_msgdef *m2 = upb_msgdef_newnamed("M2", &m2);
  upb_msgdef *m3 = upb_msgdef_newnamed("M3", &m3);
  upb_msgdef *m4 = upb_msgdef_newnamed("M4", &m4);
  upb_fielddef *f = upb_fielddef_new(&f);

  /* Freeze M4 and make M1 point to it. */
  upb_def_freeze((upb_def*const*)&m4, 1, NULL);

  upb_fielddef_settype(f, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f, 1, NULL));
  ASSERT(upb_fielddef_setname(f, "foo", NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f, m4, NULL));

  ASSERT(upb_msgdef_addfield(m1, f, &f, NULL));

  /* After this unref, M1 is the only thing keeping M4 alive. */
  upb_msgdef_unref(m4, &m4);

  /* Force M1/M2/M3 into a single mutable refcounting group. */
  f = upb_fielddef_new(&f);
  upb_fielddef_settype(f, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f, 1, NULL));
  ASSERT(upb_fielddef_setname(f, "foo", NULL));

  ASSERT(upb_fielddef_setmsgsubdef(f, m1, NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f, m2, NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f, m3, NULL));

  /* Make M3 cyclic with itself. */
  ASSERT(upb_msgdef_addfield(m3, f, &f, NULL));

  /* These will be kept alive since they are in the same refcounting group as
   * M3, which still has a ref.  Note: this behavior is not guaranteed by the
   * API, but true in practice with its current implementation. */
  upb_msgdef_unref(m1, &m1);
  upb_msgdef_unref(m2, &m2);

  /* Test that they are still alive (NOT allowed by the API). */
  ok = strcmp("M1", upb_msgdef_fullname(m1)) == 0;
  ASSERT(ok);
  ok = strcmp("M2", upb_msgdef_fullname(m2)) == 0;
  ASSERT(ok);

  /* Freeze M3.  If the test leaked no memory, then freeing m1 and m2 was
   * successful. */
  ASSERT(upb_def_freeze((upb_def*const*)&m3, 1, NULL));

  upb_msgdef_unref(m3, &m3);
}

static void test_partial_freeze() {
  /* Test that freeze of only part of the graph correctly adjusts objects that
   * point to the newly-frozen objects. */
  upb_msgdef *m1 = upb_msgdef_newnamed("M1", &m1);
  upb_msgdef *m2 = upb_msgdef_newnamed("M2", &m2);
  upb_msgdef *m3 = upb_msgdef_newnamed("M3", &m3);
  upb_fielddef *f1 = upb_fielddef_new(&f1);
  upb_fielddef *f2 = upb_fielddef_new(&f2);
  upb_def *defs[2];
  defs[0] = upb_msgdef_upcast_mutable(m1);
  defs[1] = upb_msgdef_upcast_mutable(m2);

  upb_fielddef_settype(f1, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f1, 1, NULL));
  ASSERT(upb_fielddef_setname(f1, "f1", NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f1, m1, NULL));

  upb_fielddef_settype(f2, UPB_TYPE_MESSAGE);
  ASSERT(upb_fielddef_setnumber(f2, 2, NULL));
  ASSERT(upb_fielddef_setname(f2, "f2", NULL));
  ASSERT(upb_fielddef_setmsgsubdef(f2, m2, NULL));

  ASSERT(upb_msgdef_addfield(m3, f1, &f1, NULL));
  ASSERT(upb_msgdef_addfield(m3, f2, &f2, NULL));

  /* Freeze M1 and M2, which should cause the group to be split
   * and m3 (left mutable) to take references on m1 and m2. */
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
  upb_status s = UPB_STATUS_INIT;

  ASSERT(upb_msgdef_mapentry(m) == false);
  upb_msgdef_setfullname(m, "TestMessage", &s);
  ASSERT(upb_ok(&s));
  upb_msgdef_setmapentry(m, true);
  ASSERT(upb_msgdef_mapentry(m) == true);
  upb_msgdef_unref(m, &m);
}

static void test_mapentry_check() {
  upb_status s = UPB_STATUS_INIT;
  upb_msgdef *m = upb_msgdef_new(&m);
  upb_fielddef *f = upb_fielddef_new(&f);
  upb_symtab *symtab = upb_symtab_new(&symtab);
  upb_msgdef *subm = upb_msgdef_new(&subm);
  upb_def *defs[2];

  upb_msgdef_setfullname(m, "TestMessage", &s);
  upb_fielddef_setname(f, "field1", &s);
  upb_fielddef_setnumber(f, 1, &s);
  upb_fielddef_setlabel(f, UPB_LABEL_OPTIONAL);
  upb_fielddef_settype(f, UPB_TYPE_MESSAGE);
  upb_fielddef_setsubdefname(f, ".MapEntry", &s);
  upb_msgdef_addfield(m, f, &f, &s);
  ASSERT(upb_ok(&s));

  upb_msgdef_setfullname(subm, "MapEntry", &s);
  upb_msgdef_setmapentry(subm, true);

  defs[0] = upb_msgdef_upcast_mutable(m);
  defs[1] = upb_msgdef_upcast_mutable(subm);
  upb_symtab_add(symtab, defs, 2, NULL, &s);
  /* Should not have succeeded: non-repeated field pointing to a MapEntry. */
  ASSERT(!upb_ok(&s));

  upb_status_clear(&s);
  upb_fielddef_setlabel(f, UPB_LABEL_REPEATED);
  upb_symtab_add(symtab, defs, 2, NULL, &s);
  ASSERT(upb_ok(&s));

  upb_symtab_free(symtab);
  upb_msgdef_unref(subm, &subm);
  upb_msgdef_unref(m, &m);
}

static void test_oneofs() {
  upb_status s = UPB_STATUS_INIT;
  bool ok = true;
  upb_def *subm_defs[1];
  upb_symtab *symtab = upb_symtab_new(&symtab);
  upb_msgdef *subm = upb_msgdef_newnamed("SubMessage", &symtab);
  upb_msgdef *m = upb_msgdef_newnamed("TestMessage", &symtab);
  upb_oneofdef *o = upb_oneofdef_new(&o);
  const upb_oneofdef *lookup_o;
  const upb_fielddef *lookup_field;
  upb_def *defs[1];

  ASSERT(symtab != NULL);

  /* Create a test message for fields to refer to. */
  upb_msgdef_addfield(subm, newfield("field1", 1, UPB_TYPE_INT32,
                                     UPB_LABEL_OPTIONAL, NULL, &symtab),
                      &symtab, NULL);
  subm_defs[0] = upb_msgdef_upcast_mutable(subm);
  ASSERT_STATUS(upb_symtab_add(symtab, subm_defs, 1, &symtab, &s), &s);

  ASSERT(upb_msgdef_numoneofs(m) == 0);

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

  defs[0] = upb_msgdef_upcast_mutable(m);
  ASSERT_STATUS(upb_symtab_add(symtab, defs, 1, &symtab, &s), &s);

  ASSERT(upb_msgdef_numoneofs(m) == 1);
  lookup_o = upb_msgdef_ntooz(m, "test_oneof");
  ASSERT(lookup_o == o);

  lookup_field = upb_oneofdef_ntofz(o, "field1");
  ASSERT(lookup_field != NULL && upb_fielddef_number(lookup_field) == 1);

  upb_symtab_free(symtab);
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
  test_fielddef();
  test_fielddef_unref();
  test_replacement_fails();
  test_freeze_free();
  test_partial_freeze();
  test_noreftracking();
  test_descriptor_flags();
  test_mapentry_check();
  test_oneofs();
  return 0;
}
