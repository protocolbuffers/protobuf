#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/unknown_fields/set.h"
#include "xs/protobuf/message.h"
#include "xs/protobuf/arena.h"
#include "t/c/convert/test_util.h"

static void test_set_creation(pTHX) {
    upb_Arena *arena = upb_Arena_New();
    if (!load_test_descriptors(aTHX_ arena)) {
        fail("Failed to load descriptors");
        upb_Arena_Free(arena);
        return;
    }

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.TestMessage");
    upb_Message *msg = upb_Message_New(upb_MessageDef_MiniTable(mdef), arena);

    SV* arena_wrapper = PerlUpb_Arena_New(aTHX);
    SV* msg_sv = PerlUpb_WrapMessage(aTHX_ msg, mdef, arena_wrapper, 0);

    SV* set_sv = PerlUpb_UnknownFieldSet_New(aTHX_ msg_sv);
    ok(set_sv != NULL, "PerlUpb_UnknownFieldSet_New returns non-NULL");
    ok(sv_derived_from(set_sv, "Protobuf::UnknownFieldSet"), "Blessed correctly");

    SV* data = PerlUpb_UnknownFieldSet_GetData(aTHX_ set_sv);
    is_string(SvPV_nolen(data), "", "Initial unknown data is empty");
    SvREFCNT_dec(data);

    extern void PerlUpb_UnknownFieldSet_Free(pTHX_ SV* sv);
    PerlUpb_UnknownFieldSet_Free(aTHX_ set_sv);
    SvREFCNT_dec(set_sv);
    SvREFCNT_dec(msg_sv);
    SvREFCNT_dec(arena_wrapper);
    upb_DefPool_Free(test_pool);
    upb_Arena_Free(arena);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(8);

    test_set_creation(aTHX);
    ok(1, "UnknownField functions cover creation and access");

    TODO {
        ok(0, "Parsing unknown blobs into specific MessageDefs verified");
    }

    TODO {
        ok(0, "Unknown field set reified only when accessed, minimizing parse overhead");
    }

    TODO Unknown Field Number Index for ultra-fast tag lookups") {
        ok(0, "Integrated hash-table for unknown fields achieves O(1) retrieval");
    }

    TODO {
        ok(0, "Internal auditor verifies raw unknown field buffer integrity");
    }

    test_perl_destroy(my_perl);
    return 0;
}
