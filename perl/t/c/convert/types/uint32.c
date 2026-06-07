#include "t/c/convert/types/uint32.h"
#include "t/c/upb-perl-test.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"
#include <math.h>
#include <stdbool.h>

// test_num is external from the main runner
extern int test_num;

void set_uint32_max(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->uint32_val = UINT32_MAX; }

void check_sv_uint32(pTHX_ SV *sv, uint32_t expected_val, const char *prefix) {
    uint32_t actual_val = 0;
    bool matched = false;
    if (SvUOK(sv)) { actual_val = SvUV(sv); matched = true; }
    else if (SvIOK(sv) && SvIV(sv) >= 0) { actual_val = (uint32_t)SvIV(sv); matched = true; }
    else if (SvNOK(sv)) { double nv = SvNV(sv); if (nv >= 0 && nv <= UINT32_MAX && floor(nv) == nv) { actual_val = (uint32_t)nv; matched = true; } }
    if (matched) {
        is_u(actual_val, expected_val, sdiagnostic("%s: SV value correct", prefix));
    } else {
        ok(0, sdiagnostic("%s: SV not a valid uint32 representation", prefix));
    }
}

void check_sv_uint32_max(pTHX_ SV *sv, const char *prefix) {
    check_sv_uint32(aTHX_ sv, UINT32_MAX, prefix);
}

static void set_repeated_uint32_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_UInt32);
    upb_MessageValue msg_val;

    msg_val.uint32_val = 100;
    upb_Array_Append(arr, msg_val, arena);

    msg_val.uint32_val = UINT32_MAX;
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(uint32, 2)

const upb_to_sv_test_case uint32_upb_to_sv_test_cases[] = {
    {"optional_uint32", "uint32", kUpb_FieldType_UInt32, set_uint32_max, check_sv_uint32_max, 1},
    {"repeated_uint32", "repeated uint32", kUpb_FieldType_UInt32, set_repeated_uint32_val, check_sv_repeated_uint32_val, 5},
    {NULL} // Terminator
};

// sv_to_upb cases
SV* create_sv_uint32_max(pTHX) { return newSVuv(UINT32_MAX); }

void check_upb_uint32_max(const upb_MessageValue *val, const char *prefix) {
    is_u(val->uint32_val, UINT32_MAX, sdiagnostic("%s: uint32 value correct", prefix));
}

static SV* create_sv_repeated_uint32(pTHX) {
    AV *av = newAV();
    av_push(av, newSVuv(100));
    av_push(av, newSVuv(UINT32_MAX));
    return newRV_inc((SV*)av);
}

static void check_upb_repeated_uint32(const upb_MessageValue *val, const char *prefix) {
    const upb_Array *arr = val->array_val;
    is(upb_Array_Size(arr), 2, sdiagnostic("%s: array size", prefix));
    is_u(upb_Array_Get(arr, 0).uint32_val, 100, sdiagnostic("%s: element 0", prefix));
    is_u(upb_Array_Get(arr, 1).uint32_val, UINT32_MAX, sdiagnostic("%s: element 1", prefix));
}

const sv_to_upb_test_case uint32_sv_to_upb_test_cases[] = {
    {"optional_uint32", "uint32", kUpb_FieldType_UInt32, create_sv_uint32_max, check_upb_uint32_max, 1},
    {"repeated_uint32", "repeated uint32", kUpb_FieldType_UInt32, create_sv_repeated_uint32, check_upb_repeated_uint32, 3},
    {NULL} // Terminator
};
