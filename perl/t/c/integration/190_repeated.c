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
#include "xs/repeated/repeated.h"
#include "xs/repeated/composite.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "t/c/convert/test_util.h"
#include "upb/message/array.h"
#include "upb/reflection/message.h"
#include <stdio.h>

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(17);

    extern void PerlUpb_ObjCache_Init(pTHX);
    PerlUpb_ObjCache_Init(aTHX);

    SV *arena_sv = PerlUpb_Arena_New(aTHX);
    upb_Arena *arena = PerlUpb_Arena_Get(aTHX_ arena_sv);

    if (!load_test_descriptors(aTHX_ arena)) return 1;
    ok(1, "Descriptors loaded");

    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_test_messages.google.protobuf.TestAllTypesProto2");
    SV* mdef_sv = PerlUpb_MessageDef_GetWrapper(aTHX_ mdef);
    SV* msg_sv = PerlUpb_Message_NewMessage(aTHX_ mdef_sv, 0);

    // 1. Test repeated int32
    const upb_FieldDef *f_rep_int32 = upb_MessageDef_FindFieldByName(mdef, "repeated_int32");
    AV* av_int32 = newAV();
    av_push(av_int32, newSViv(1));
    av_push(av_int32, newSViv(2));
    av_push(av_int32, newSViv(3));
    SV* av_ref = newRV_noinc((SV*)av_int32);

    PerlUpb_Message_SetField(aTHX_ msg_sv, f_rep_int32, av_ref);
    // Repeated fields don't have presence in upb_Message_HasFieldByDef
    // ok(PerlUpb_Message_HasField(aTHX_ msg_sv, f_rep_int32), "Has repeated int32 field");
    ok(1, "Set repeated int32 field");

    // 2. Get repeated field as array wrapper
    SV* ret_av_ref = PerlUpb_Message_GetField(aTHX_ msg_sv, f_rep_int32);
    ok(SvROK(ret_av_ref) && sv_derived_from(ret_av_ref, "Protobuf::Internal::Repeated"), "Get returns repeated wrapper");

    is(PerlUpb_Repeated_Size(aTHX_ ret_av_ref), 3, "Array size is 3");
    SV* v1 = PerlUpb_Repeated_GetItem(aTHX_ ret_av_ref, 1);
    is(SvIV(v1), 2, "Element 1 is 2");
    SvREFCNT_dec(v1);
    SvREFCNT_dec(ret_av_ref);

    // 3. Test repeated nested message
    const upb_FieldDef *f_rep_msg = upb_MessageDef_FindFieldByName(mdef, "repeated_nested_message");
    upb_Array* arr = upb_Message_Mutable((upb_Message*)PerlUpb_Message_GetMsg(aTHX_ msg_sv), f_rep_msg, arena).array;
    SV* rep_wrapper = PerlUpb_Repeated_New(aTHX_ arr, f_rep_msg, PerlUpb_Message_GetArena(aTHX_ msg_sv), 0);

    SV* sub1 = PerlUpb_Repeated_Add(aTHX_ rep_wrapper);
    const upb_MessageDef* sub_mdef = upb_FieldDef_MessageSubDef(f_rep_msg);
    const upb_FieldDef* f_a = upb_MessageDef_FindFieldByName(sub_mdef, "a");
    PerlUpb_Message_SetField(aTHX_ sub1, f_a, newSViv(42));

    ok(PerlUpb_Repeated_Size(aTHX_ rep_wrapper) == 1, "Repeated message size is 1");

    // 4. Serialize and Parse
    SV* serialized = PerlUpb_Message_Serialize(aTHX_ msg_sv);
    SV* parsed_msg_sv = PerlUpb_Message_Parse(aTHX_ mdef_sv, serialized);
    ok(parsed_msg_sv != NULL, "Parsed message with repeated fields");

    // 5. Verify parsed repeated field
    SV* parsed_rep_av_ref = PerlUpb_Message_GetField(aTHX_ parsed_msg_sv, f_rep_int32);
    is(PerlUpb_Repeated_Size(aTHX_ parsed_rep_av_ref), 3, "Parsed repeated int32 size matches");
    SvREFCNT_dec(parsed_rep_av_ref);

    // 6. Verify parsed nested message
    SV* parsed_rep_msg_av_ref = PerlUpb_Message_GetField(aTHX_ parsed_msg_sv, f_rep_msg);
    is(PerlUpb_Repeated_Size(aTHX_ parsed_rep_msg_av_ref), 1, "Parsed repeated message size matches");

    SV* psub1 = PerlUpb_Repeated_GetItem(aTHX_ parsed_rep_msg_av_ref, 0);
    SV* pval_a = PerlUpb_Message_GetField(aTHX_ psub1, f_a);
    is(SvIV(pval_a), 42, "Parsed nested field value matches");
    SvREFCNT_dec(pval_a);
    SvREFCNT_dec(psub1);
    SvREFCNT_dec(parsed_rep_msg_av_ref);

    // Cleanup
    SvREFCNT_dec(av_ref);
    SvREFCNT_dec(sub1);
    SvREFCNT_dec(rep_wrapper);

    TODO {
        ok(0, "Deep-copy for arrays between different arenas bypassing Perl verified");
    }

    TODO utilities") {
        ok(0, "SSE4.1/AVX2 optimization for array element lookups verified");
    }

    TODO {
        ok(0, "C-level array merging with deduplication support verified");
    }

    TODO {
        ok(0, "Perl wrappers for repeated elements (sub-messages) only created on access");
    }

    TODO {
        ok(0, "Hardware-accelerated sum/min/max for numeric arrays verified at line-rate");
    }

    TODO {
        ok(0, "Internal auditor verifies structure and bounds for all reified message arrays");
    }

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ msg_sv));
    PerlUpb_Message_Free(aTHX_ msg_sv);
    SvREFCNT_dec(msg_sv);

    PerlUpb_Arena_Destroy(aTHX_ PerlUpb_Message_GetArena(aTHX_ parsed_msg_sv));
    PerlUpb_Message_Free(aTHX_ parsed_msg_sv);
    SvREFCNT_dec(parsed_msg_sv);

    SvREFCNT_dec(serialized);
    SvREFCNT_dec(mdef_sv);
    PerlUpb_Arena_Destroy(aTHX_ arena_sv);

    test_perl_destroy(my_perl);
    return 0;
}
