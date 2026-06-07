#include "t/c/convert/types/sint64.h"
#include "t/c/upb-perl-test.h"
#include "t/c/convert/test_util.h" // Add this
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"

// test_num is external from the main runner
extern int test_num;

// upb_to_sv
void set_sint64_val(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->int64_val = -64000000000LL; }
void check_sv_sint64_val(pTHX_ SV *sv, const char *prefix) {
    ok(SvIOK(sv), sdiagnostic("%s: SV is IOK", prefix));
    is(SvIV(sv), -64000000000LL, sdiagnostic("%s: SV value correct", prefix));
}

static void set_repeated_sint64_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_Int64);
    upb_MessageValue msg_val;
    msg_val.int64_val = -12345678912345LL;
    upb_Array_Append(arr, msg_val, arena);
    msg_val.int64_val = 0LL;
    upb_Array_Append(arr, msg_val, arena);
    msg_val.int64_val = 98765432198765LL;
    upb_Array_Append(arr, msg_val, arena);
    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(sint64, 3)

const upb_to_sv_test_case sint64_upb_to_sv_test_cases[] = {
    {"optional_sint64", "sint64", kUpb_FieldType_SInt64, set_sint64_val, check_sv_sint64_val, 2},
    {"repeated_sint64", "repeated sint64", kUpb_FieldType_SInt64, set_repeated_sint64_val, check_sv_repeated_sint64_val, 8},
    {NULL}
};

// sv_to_upb
SV* create_sv_sint64_val(pTHX) { return newSViv(-128000000000LL); }

void check_upb_sint64_val(const upb_MessageValue *val, const char *prefix) {
    is(val->int64_val, -128000000000LL, sdiagnostic("%s: sint64 value correct", prefix));
}
const sv_to_upb_test_case sint64_sv_to_upb_test_cases[] = {
    {"optional_sint64", "sint64", kUpb_FieldType_SInt64, create_sv_sint64_val, check_upb_sint64_val, 1},
    {NULL}
};
