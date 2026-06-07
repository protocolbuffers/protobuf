#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor.h"
#include "t/c/convert/test_util.h"

static void test_message_def(pTHX) {
    upb_Arena *arena = upb_Arena_New();
    if (!load_test_descriptors(aTHX_ arena)) {
        fail("Failed to load descriptors");
        upb_Arena_Free(arena);
        return;
    }

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    ok(mdef != NULL, "Got TestMessage Def");

    SV *wrapper = PerlUpb_MessageDef_GetWrapper(aTHX_ mdef);
    ok(wrapper != NULL, "Got wrapper SV");
    ok(sv_isobject(wrapper) && sv_derived_from(wrapper, "Protobuf::Descriptor::MessageDef"), "Wrapper is blessed correctly");

    SV *name_sv = PerlUpb_Message_FullName(aTHX_ mdef);
    is_string(SvPV_nolen(name_sv), "protobuf_perl_test.TestMessage", "FullName matches");
    SvREFCNT_dec(name_sv);

    // Check field count (from test.proto)
    int field_count = upb_MessageDef_FieldCount(mdef);
    ok(field_count > 0, "TestMessage has fields");

    // Use a field that actually exists in protobuf_perl_test.TestMessage
    const upb_FieldDef *f = PerlUpb_MessageDef_FindFieldByNameWithSize(aTHX_ mdef, "optional_bool", 13);
    ok(f != NULL, "Found optional_bool field");
    if (f) {
        is_string(upb_FieldDef_Name(f), "optional_bool", "Field name matches");
    } else {
        ok(0, "Skip Field name match because field not found");
    }

    SvREFCNT_dec(wrapper);
    upb_DefPool_Free(test_pool);
    upb_Arena_Free(arena);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(8);
    test_num = 0;

    test_message_def(aTHX);

    TODO {
        ok(0, "message unit tests covering all edge cases");
    }

    test_perl_destroy(my_perl);
    return 0;
}
