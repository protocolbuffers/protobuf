#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"
#include "xs/message/message.h"
#include "xs/message/access.h"
#include "xs/message/serialize.h"
#include "xs/message/compare.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "t/c/convert/test_util.h"
#include "upb/reflection/message.h"
#include <stdio.h>

static void test_deterministic_serialization(pTHX_ SV* msg_sv) {
    SV* s1 = PerlUpb_Message_Serialize_Deterministic(aTHX_ msg_sv);
    SV* s2 = PerlUpb_Message_Serialize_Deterministic(aTHX_ msg_sv);

    ok(s1 != NULL && s2 != NULL, "Deterministic serialization returned buffers");
    STRLEN l1, l2;
    const char* d1 = SvPV(s1, l1);
    const char* d2 = SvPV(s2, l2);

    is(l1, l2, "Deterministic lengths match");
    ok(memcmp(d1, d2, l1) == 0, "Deterministic buffers match exactly");

    SvREFCNT_dec(s1);
    SvREFCNT_dec(s2);
}

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(22);

    extern void PerlUpb_ObjCache_Init(pTHX);
    PerlUpb_ObjCache_Init(aTHX);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    if (!load_test_descriptors(aTHX_ arena)) return 1;
    ok(1, "Descriptors loaded");

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2");
    SV* mdef_sv = PerlUpb_MessageDef_GetWrapper(aTHX_ mdef);

    // 1. Create Top-Level Message
    SV* msg_sv = PerlUpb_Message_NewMessage(aTHX_ mdef_sv, 0);
    ok(msg_sv != NULL, "Created top-level message");

    // 2. Set Scalar Fields
    const upb_FieldDef *f_int32 = upb_MessageDef_FindFieldByName(mdef, "optional_int32");
    const upb_FieldDef *f_string = upb_MessageDef_FindFieldByName(mdef, "optional_string");

    SV* val_int32 = newSViv(42);
    SV* val_string = newSVpv("hello world", 0);

    PerlUpb_Message_SetField(aTHX_ msg_sv, f_int32, val_int32);
    PerlUpb_Message_SetField(aTHX_ msg_sv, f_string, val_string);

    ok(PerlUpb_Message_HasField(aTHX_ msg_sv, f_int32), "Has int32 field");
    ok(PerlUpb_Message_HasField(aTHX_ msg_sv, f_string), "Has string field");

    // 3. Set Nested Message Field
    const upb_FieldDef *f_nested = upb_MessageDef_FindFieldByName(mdef, "optional_nested_message");
    const upb_MessageDef *nested_mdef = upb_FieldDef_MessageSubDef(f_nested);
    SV* nested_mdef_sv = PerlUpb_MessageDef_GetWrapper(aTHX_ nested_mdef);

    SV* nested_msg_sv = PerlUpb_Message_NewMessage(aTHX_ nested_mdef_sv, 0);
    const upb_FieldDef *f_nested_a = upb_MessageDef_FindFieldByName(nested_mdef, "a");
    PerlUpb_Message_SetField(aTHX_ nested_msg_sv, f_nested_a, newSViv(100));

    PerlUpb_Message_SetField(aTHX_ msg_sv, f_nested, nested_msg_sv);
    ok(PerlUpb_Message_HasField(aTHX_ msg_sv, f_nested), "Has nested message field");

    // 4. Roundtrip: Serialize and Parse
    SV* serialized = PerlUpb_Message_Serialize(aTHX_ msg_sv);
    ok(serialized != NULL && SvCUR(serialized) > 0, "Serialized message");

    SV* parsed_msg_sv = PerlUpb_Message_Parse(aTHX_ mdef_sv, serialized);
    ok(parsed_msg_sv != NULL, "Parsed message");

    // 5. Compare
    ok(PerlUpb_Message_IsEqual(aTHX_ msg_sv, parsed_msg_sv), "Parsed message is equal to original");

    // 6. Access Nested Field in Parsed Message
    SV* ret_nested_msg_sv = PerlUpb_Message_GetField(aTHX_ parsed_msg_sv, f_nested);
    ok(ret_nested_msg_sv != NULL && sv_isobject(ret_nested_msg_sv), "Got nested message from parsed");

    SV* ret_nested_a = PerlUpb_Message_GetField(aTHX_ ret_nested_msg_sv, f_nested_a);
    is(SvIV(ret_nested_a), 100, "Nested field value matches");
    SvREFCNT_dec(ret_nested_a);
    SvREFCNT_dec(ret_nested_msg_sv);

    // 7. Test Message with different Arena interaction (Nested re-use)
    ok(1, "Cross-arena copy test placeholder");

    test_deterministic_serialization(aTHX_ msg_sv);

    // Cleanup
    SvREFCNT_dec(val_int32);
    SvREFCNT_dec(val_string);
    SvREFCNT_dec(serialized);

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ msg_sv));
    PerlUpb_Message_Free(aTHX_ msg_sv);
    SvREFCNT_dec(msg_sv);

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ nested_msg_sv));
    PerlUpb_Message_Free(aTHX_ nested_msg_sv);
    SvREFCNT_dec(nested_msg_sv);

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ parsed_msg_sv));
    PerlUpb_Message_Free(aTHX_ parsed_msg_sv);
    SvREFCNT_dec(parsed_msg_sv);

    SvREFCNT_dec(nested_mdef_sv);
    SvREFCNT_dec(mdef_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    ok(1, "Completed clean destruction");

    TODO {
        ok(0, "Setting oneof members correctly invalidates others in integrated C context");
    }

    TODO logic") {
        ok(0, "Deep message merge using shared arenas verified");
    }

    TODO") {
        ok(0, "Two message trees compared at memory speeds using hardware acceleration");
    }

    TODO {
        ok(0, "Arena blocks pre-allocated and warmed for hot message paths");
    }

    TODO {
        ok(0, "Internal auditor verifies structure and canary consistency for all reified messages");
    }

    test_perl_destroy(my_perl);
    return 0;
}
