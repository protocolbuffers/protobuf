#include "t/c/convert/types/uint64.h"
#include "t/c/upb-perl-test.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"
#include <math.h>
#include <stdbool.h>

// test_num is external from the main runner
extern int test_num;

void set_uint64_max(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->uint64_val = UINT64_MAX; }

void check_sv_uint64(pTHX_ SV *sv, uint64_t expected_val, const char *prefix) {
    uint64_t actual_val = 0;
    bool matched = false;
    if (SvUOK(sv)) { actual_val = SvUV(sv); matched = true; }
    else if (SvIOK(sv) && SvIV(sv) >= 0) { actual_val = (uint64_t)SvIV(sv); matched = true; }
    else if (SvNOK(sv)) { double nv = SvNV(sv); if (nv >= 0 && nv <= (double)UINT64_MAX && floor(nv) == nv) { actual_val = (uint64_t)nv; matched = true; } }
    if (matched) {
        is_u(actual_val, expected_val, sdiagnostic("%s: SV value correct", prefix));
    } else {
        ok(0, sdiagnostic("%s: SV not a valid uint64 representation", prefix));
    }
}

void check_sv_uint64_max(pTHX_ SV *sv, const char *prefix) {
    check_sv_uint64(aTHX_ sv, UINT64_MAX, prefix);
}

static void set_repeated_uint64_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_UInt64);
    upb_MessageValue msg_val;

    msg_val.uint64_val = 0;
    upb_Array_Append(arr, msg_val, arena);

    msg_val.uint64_val = 12345678912345ULL;
    upb_Array_Append(arr, msg_val, arena);

    msg_val.uint64_val = UINT64_MAX;
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(uint64, 3)

const upb_to_sv_test_case uint64_upb_to_sv_test_cases[] = {
    {"optional_uint64", "uint64", kUpb_FieldType_UInt64, set_uint64_max, check_sv_uint64_max, 1},
    {"repeated_uint64", "repeated uint64", kUpb_FieldType_UInt64, set_repeated_uint64_val, check_sv_repeated_uint64_val, 8},
    {NULL}
};

// sv_to_upb cases
SV* create_sv_uint64_max(pTHX) { return newSVuv(UINT64_MAX); }

void check_upb_uint64_max(const upb_MessageValue *val, const char *prefix) {
    is_u(val->uint64_val, UINT64_MAX, sdiagnostic("%s: uint64 value correct", prefix));
}

const sv_to_upb_test_case uint64_sv_to_upb_test_cases[] = {
    {"optional_uint64", "uint64", kUpb_FieldType_UInt64, create_sv_uint64_max, check_upb_uint64_max, 1},
    {NULL} // Terminator
};
