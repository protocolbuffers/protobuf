#include "t/c/convert/types/int64.h"
#include "t/c/upb-perl-test.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"

// test_num is external from the main runner
extern int test_num;

void set_int64_max(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->int64_val = INT64_MAX; }

void check_sv_int64_max(pTHX_ SV *sv, const char *prefix) {
    ok(SvIOK(sv), sdiagnostic("%s: SV is IOK", prefix));
    is(SvIV(sv), INT64_MAX, sdiagnostic("%s: SV value correct", prefix));
}

static void set_repeated_int64_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_Int64);
    upb_MessageValue msg_val;

    msg_val.int64_val = INT64_MIN;
    upb_Array_Append(arr, msg_val, arena);

    msg_val.int64_val = 0;
    upb_Array_Append(arr, msg_val, arena);

    msg_val.int64_val = INT64_MAX;
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(int64, 3)

const upb_to_sv_test_case int64_upb_to_sv_test_cases[] = {
    {"optional_int64", "int64", kUpb_FieldType_Int64, set_int64_max, check_sv_int64_max, 2},
    {"repeated_int64", "repeated int64", kUpb_FieldType_Int64, set_repeated_int64_val, check_sv_repeated_int64_val, 8},
    {NULL} // Terminator
};

// sv_to_upb cases
SV* create_sv_int64_max(pTHX) { return newSViv(INT64_MAX); }

void check_upb_int64_max(const upb_MessageValue *val, const char *prefix) {
    is(val->int64_val, INT64_MAX, sdiagnostic("%s: int64 value correct", prefix));
}

const sv_to_upb_test_case int64_sv_to_upb_test_cases[] = {
    {"optional_int64", "int64", kUpb_FieldType_Int64, create_sv_int64_max, check_upb_int64_max, 1},
    {NULL} // Terminator
};
