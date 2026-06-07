#include "t/c/convert/types/message.h"
#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "upb/reflection/def.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/message.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"
#include <string.h>

// test_num is external from the main runner
extern int test_num;

// upb_to_sv
static upb_Message* create_nested_upb(upb_Arena *arena, const char *str) {
    const upb_MessageDef *mdef = upb_DefPool_FindMessageByName(test_pool, "protobuf_perl_test.NestedMessage");
    if (!mdef) {
        fprintf(stderr, "ERROR: create_nested_upb - mdef is NULL for protobuf_perl_test.NestedMessage\n");
        return NULL;
    }
    upb_Message *msg = upb_Message_New(upb_MessageDef_MiniTable(mdef), arena);
    const upb_FieldDef *fdef = upb_MessageDef_FindFieldByName(mdef, "nested_string");
    const upb_MiniTableField *mfield = upb_FieldDef_MiniTable(fdef);
    upb_StringView str_val = upb_StringView_FromString(str);
    upb_Message_SetBaseField(msg, mfield, &str_val);
    return msg;
}

static void set_message_val(upb_MessageValue *val, upb_Arena *arena) {
    val->msg_val = create_nested_upb(arena, "inside");
}

static void check_sv_message(pTHX_ SV *sv, const char *prefix) {
    ok(SvROK(sv), sdiagnostic("%s: SV is a reference", prefix));
    if (!SvROK(sv)) return;
    SV *deref = SvRV(sv);
    ok(SvTYPE(deref) == SVt_PVHV, sdiagnostic("%s: Dereferenced SV is a HASH", prefix));
    if (SvTYPE(deref) != SVt_PVHV) return;

    HV *hv = (HV*)deref;
    ok(hv_exists(hv, "_upb_ptr", 8), sdiagnostic("%s: Hash has _upb_ptr key", prefix));
    ok(hv_exists(hv, "_arena_sv", 9), sdiagnostic("%s: Hash has _arena_sv key", prefix));
    TODO") {
        ok(hv_exists(hv, "_descriptor", 11), sdiagnostic("%s: Hash has _descriptor key", prefix));
    }
    // Tests for object blessing and ISA are done in Perl-level tests.
}

static void set_repeated_message_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_Message);
    upb_MessageValue msg_val;

    msg_val.msg_val = create_nested_upb(arena, "one");
    upb_Array_Append(arr, msg_val, arena);

    msg_val.msg_val = create_nested_upb(arena, "two");
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

static void check_sv_repeated_message_val(pTHX_ SV *sv, const char *prefix) {
    ok(sv_derived_from(sv, "Protobuf::Internal::Repeated"), sdiagnostic("%s: SV is a Repeated wrapper", prefix));
    if (!sv_derived_from(sv, "Protobuf::Internal::Repeated")) return;

    int size = PerlUpb_Repeated_Size(aTHX_ sv);
    is(size, 2, sdiagnostic("%s: Array has 2 elements", prefix));
    SV *elem0 = PerlUpb_Repeated_GetItem(aTHX_ sv, 0);
    ok(elem0, sdiagnostic("%s: Fetched element 0", prefix));
    if (elem0) {
        ok(SvROK(elem0), sdiagnostic("%s: Element 0 is a reference", prefix));
        SvREFCNT_dec(elem0);
    }
}

const upb_to_sv_test_case message_upb_to_sv_test_cases[] = {
    {"optional_nested_message", "message", kUpb_FieldType_Message, set_message_val, check_sv_message, 5},
    {"repeated_nested_message", "repeated message", kUpb_FieldType_Message, set_repeated_message_val, check_sv_repeated_message_val, 14},
    {NULL}
};

// sv_to_upb
// sv_to_upb tests for Message types are best handled in Perl-level tests (*.t)
// as they require creating and manipulating Perl Message objects.
const sv_to_upb_test_case message_sv_to_upb_test_cases[] = {
    {NULL}
};
