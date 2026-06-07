#include "t/c/convert/types/fixed64.h"
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
void set_fixed64_val(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->uint64_val = 123456789012345ULL; }
void check_sv_uint64(pTHX_ SV *sv, uint64_t expected_val, const char *prefix);
void check_sv_fixed64_val(pTHX_ SV *sv, const char *prefix) {
    check_sv_uint64(aTHX_ sv, 123456789012345ULL, prefix);
}

static void set_repeated_fixed64_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_UInt64);
    upb_MessageValue msg_val;

    msg_val.uint64_val = 9999ULL;
    upb_Array_Append(arr, msg_val, arena);
    msg_val.uint64_val = 8888ULL;
    upb_Array_Append(arr, msg_val, arena);
    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(fixed64, 2)

const upb_to_sv_test_case fixed64_upb_to_sv_test_cases[] = {
    {"optional_fixed64", "fixed64", kUpb_FieldType_Fixed64, set_fixed64_val, check_sv_fixed64_val, 1},
    {"repeated_fixed64", "repeated fixed64", kUpb_FieldType_Fixed64, set_repeated_fixed64_val, check_sv_repeated_fixed64_val, 5},
    {NULL}
};

// sv_to_upb cases
SV* create_sv_fixed64_val(pTHX) { return newSVuv(123456789012345ULL); }

void check_upb_fixed64_val(const upb_MessageValue *val, const char *prefix) {
    is_u(val->uint64_val, 123456789012345ULL, sdiagnostic("%s: fixed64 value correct", prefix));
}
const sv_to_upb_test_case fixed64_sv_to_upb_test_cases[] = {
    {"optional_fixed64", "fixed64", kUpb_FieldType_Fixed64, create_sv_fixed64_val, check_upb_fixed64_val, 1},
    {NULL}
};
