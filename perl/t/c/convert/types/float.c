#include "t/c/convert/types/float.h"
#include "t/c/upb-perl-test.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"
#include <math.h>

// test_num is external from the main runner
extern int test_num;

void set_float_123_456(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->float_val = 123.456f; }

void check_sv_float_123_456(pTHX_ SV *sv, const char *prefix) {
    ok(SvNOK(sv), sdiagnostic("%s: SV is NOK", prefix));
    ok(fabs(SvNV(sv) - 123.456) < 0.0001, sdiagnostic("%s: SV value correct", prefix));
}

static void set_repeated_float_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_Float);
    upb_MessageValue msg_val;

    msg_val.float_val = 1.23f;
    upb_Array_Append(arr, msg_val, arena);

    msg_val.float_val = 4.56f;
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(float, 2)

const upb_to_sv_test_case float_upb_to_sv_test_cases[] = {
    {"optional_float", "float", kUpb_FieldType_Float, set_float_123_456, check_sv_float_123_456, 2},
    {"repeated_float", "repeated float", kUpb_FieldType_Float, set_repeated_float_val, check_sv_repeated_float_val, 7},
    {NULL} // Terminator
};

// sv_to_upb cases
SV* create_sv_float_123_456(pTHX) { return newSVnv(123.456); }

void check_upb_float_123_456(const upb_MessageValue *val, const char *prefix) {
    ok(fabs(val->float_val - 123.456) < 0.0001, sdiagnostic("%s: float value correct", prefix));
}

static SV* create_sv_repeated_float(pTHX) {
    AV *av = newAV();
    av_push(av, newSVnv(1.23f));
    av_push(av, newSVnv(4.56f));
    return newRV_inc((SV*)av);
}

static void check_upb_repeated_float(const upb_MessageValue *val, const char *prefix) {
    const upb_Array *arr = val->array_val;
    is(upb_Array_Size(arr), 2, sdiagnostic("%s: array size", prefix));
    ok(fabs(upb_Array_Get(arr, 0).float_val - 1.23f) < 0.0001, sdiagnostic("%s: element 0", prefix));
    ok(fabs(upb_Array_Get(arr, 1).float_val - 4.56f) < 0.0001, sdiagnostic("%s: element 1", prefix));
}

const sv_to_upb_test_case float_sv_to_upb_test_cases[] = {
    {"optional_float", "float", kUpb_FieldType_Float, create_sv_float_123_456, check_upb_float_123_456, 1},
    {"repeated_float", "repeated float", kUpb_FieldType_Float, create_sv_repeated_float, check_upb_repeated_float, 3},
    {NULL} // Terminator
};
