#include "t/c/convert/types/fixed32.h"
#include "t/c/upb-perl-test.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"
#include <math.h>
#include <stdbool.h>

// test_num is external from the main runner
extern int test_num;

// upb_to_sv
void set_fixed32_val(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->uint32_val = 12345; }
void check_sv_uint32(pTHX_ SV *sv, uint32_t expected_val, const char *prefix);
void check_sv_fixed32_val(pTHX_ SV *sv, const char *prefix) {
    check_sv_uint32(aTHX_ sv, 12345, prefix);
}

static void set_repeated_fixed32_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_UInt32);
    upb_MessageValue msg_val;

    msg_val.uint32_val = 111;
    upb_Array_Append(arr, msg_val, arena);
    msg_val.uint32_val = 222;
    upb_Array_Append(arr, msg_val, arena);
    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(fixed32, 2)

const upb_to_sv_test_case fixed32_upb_to_sv_test_cases[] = {
    {"optional_fixed32", "fixed32", kUpb_FieldType_Fixed32, set_fixed32_val, check_sv_fixed32_val, 1},
    {"repeated_fixed32", "repeated fixed32", kUpb_FieldType_Fixed32, set_repeated_fixed32_val, check_sv_repeated_fixed32_val, 5},
    {NULL}
};

// sv_to_upb
SV* create_sv_fixed32_val(pTHX) { return newSVuv(54321); }
void check_upb_fixed32_val(const upb_MessageValue *val, const char *prefix) {
    is_u(val->uint32_val, 54321, sdiagnostic("%s: fixed32 value correct", prefix));
}
const sv_to_upb_test_case fixed32_sv_to_upb_test_cases[] = {
    {"optional_fixed32", "fixed32", kUpb_FieldType_Fixed32, create_sv_fixed32_val, check_upb_fixed32_val, 1},
    {NULL}
};
