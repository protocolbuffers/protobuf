#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor.h"
#include "t/c/convert/test_util.h"

static void test_field_def(pTHX) {
    upb_Arena *arena = upb_Arena_New();
    if (!load_test_descriptors(aTHX_ arena)) {
        fail("Failed to load descriptors");
        upb_Arena_Free(arena);
        return;
    }

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    ok(mdef != NULL, "Got TestMessage Def");

    const upb_FieldDef *f = upb_MessageDef_FindFieldByName(mdef, "optional_bool");
    ok(f != NULL, "Got optional_bool field");

    SV *wrapper = PerlUpb_FieldDef_GetWrapper(aTHX_ f);
    ok(wrapper != NULL, "Got field wrapper SV");
    ok(sv_isobject(wrapper) && sv_derived_from(wrapper, "Protobuf::Descriptor::Field"), "Field wrapper is blessed correctly");

    SV *name_sv = PerlUpb_FieldDef_Name(aTHX_ f);
    is_string(SvPV_nolen(name_sv), "optional_bool", "Field name matches");
    SvREFCNT_dec(name_sv);

    is(PerlUpb_FieldDef_Type(aTHX_ f), kUpb_FieldType_Bool, "optional_bool type is Bool");
    is(PerlUpb_FieldDef_Label(aTHX_ f), (int)kUpb_Label_Optional, "optional_bool label is Optional");

    const upb_FieldDef *f_rep = upb_MessageDef_FindFieldByName(mdef, "repeated_int");
    ok(f_rep != NULL, "Got repeated_int field");
    is(PerlUpb_FieldDef_Type(aTHX_ f_rep), kUpb_FieldType_Int32, "repeated_int type is Int32");
    is(PerlUpb_FieldDef_Label(aTHX_ f_rep), (int)kUpb_Label_Repeated, "repeated_int label is Repeated");

    SvREFCNT_dec(wrapper);
    upb_DefPool_Free(test_pool);
    upb_Arena_Free(arena);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(11);
    test_num = 0;

    test_field_def(aTHX);

    TODO {
        ok(0, "field unit tests covering all types and options");
    }

    test_perl_destroy(my_perl);
    return 0;
}
