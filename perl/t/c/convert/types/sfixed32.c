#include "t/c/convert/types/sfixed32.h"
#include "t/c/upb-perl-test.h"
#include "t/c/convert/test_util.h" // Add this
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"

// test_num is external from the main runner
extern int test_num;

// upb_to_sv
void set_sfixed32_val(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->int32_val = -12345; }
void check_sv_sfixed32_val(pTHX_ SV *sv, const char *prefix) {
    ok(SvIOK(sv), sdiagnostic("%s: SV is IOK", prefix));
    is(SvIV(sv), -12345, sdiagnostic("%s: SV value correct", prefix));
}

static void set_repeated_sfixed32_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_Int32);
    upb_MessageValue msg_val;
    msg_val.int32_val = -111;
    upb_Array_Append(arr, msg_val, arena);
    msg_val.int32_val = 222;
    upb_Array_Append(arr, msg_val, arena);
    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(sfixed32, 2)

const upb_to_sv_test_case sfixed32_upb_to_sv_test_cases[] = {
    {"optional_sfixed32", "sfixed32", kUpb_FieldType_SFixed32, set_sfixed32_val, check_sv_sfixed32_val, 2},
    {"repeated_sfixed32", "repeated sfixed32", kUpb_FieldType_SFixed32, set_repeated_sfixed32_val, check_sv_repeated_sfixed32_val, 7},
    {NULL}
};

// sv_to_upb
SV* create_sv_sfixed32_val(pTHX) { return newSViv(-54321); }
void check_upb_sfixed32_val(const upb_MessageValue *val, const char *prefix) {
    is(val->int32_val, -54321, sdiagnostic("%s: sfixed32 value correct", prefix));
}
const sv_to_upb_test_case sfixed32_sv_to_upb_test_cases[] = {
    {"optional_sfixed32", "sfixed32", kUpb_FieldType_SFixed32, create_sv_sfixed32_val, check_upb_sfixed32_val, 1},
    {NULL}
};
