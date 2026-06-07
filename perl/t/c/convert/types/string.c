#include "t/c/convert/types/string.h"
#include "t/c/upb-perl-test.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"

// test_num is external from the main runner
extern int test_num;

void set_string_hello(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->str_val = upb_StringView_FromString("hello upb"); }

void check_sv_string_hello(pTHX_ SV *sv, const char *prefix) {
    ok(SvPOK(sv), sdiagnostic("%s: SV is POK", prefix));
    is_string(SvPV_nolen(sv), "hello upb", sdiagnostic("%s: SV value correct", prefix));
}

static void set_repeated_string_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_String);
    upb_MessageValue msg_val;

    msg_val.str_val = upb_StringView_FromString("repeat one");
    upb_Array_Append(arr, msg_val, arena);

    msg_val.str_val = upb_StringView_FromString("repeat two");
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(string, 2)

const upb_to_sv_test_case string_upb_to_sv_test_cases[] = {
    {"repeated_string", "repeated string", kUpb_FieldType_String, set_repeated_string_val, check_sv_repeated_string_val, 9},
    {NULL} // Terminator
};

// sv_to_upb cases
SV* create_sv_string_hello(pTHX) { return newSVpvn("hello world", 11); }

void check_upb_string_hello(const upb_MessageValue *val, const char *prefix) {
    is_string_view(val->str_val, "hello world", 11, sdiagnostic("%s: string value correct", prefix));
}

static SV* create_sv_repeated_string(pTHX) {
    AV *av = newAV();
    av_push(av, newSVpvn("rep one", 7));
    av_push(av, newSVpvn("rep two", 7));
    return newRV_inc((SV*)av);
}

static void check_upb_repeated_string(const upb_MessageValue *val, const char *prefix) {
    const upb_Array *arr = val->array_val;
    is(upb_Array_Size(arr), 2, sdiagnostic("%s: array size", prefix));
    is_string_view(upb_Array_Get(arr, 0).str_val, "rep one", 7, sdiagnostic("%s: element 0", prefix));
    is_string_view(upb_Array_Get(arr, 1).str_val, "rep two", 7, sdiagnostic("%s: element 1", prefix));
}

const sv_to_upb_test_case string_sv_to_upb_test_cases[] = {
    {"repeated_string", "repeated string", kUpb_FieldType_String, create_sv_repeated_string, check_upb_repeated_string, 3},
    {NULL} // Terminator
};
