#include "t/c/convert/types/int32.h"
#include "t/c/upb-perl-test.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/message.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"

// test_num is external from the main runner
extern int test_num;

void set_int32_123(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->int32_val = 123; }

void check_sv_int32_123(pTHX_ SV *sv, const char *prefix) {
    ok(SvIOK(sv), sdiagnostic("%s: SV is IOK", prefix));
    is(SvIV(sv), 123, sdiagnostic("%s: SV value correct", prefix));
}

static void set_repeated_int32_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_Int32);
    upb_MessageValue msg_val;

    msg_val.int32_val = 10;
    upb_Array_Append(arr, msg_val, arena);

    msg_val.int32_val = 20;
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(int32, 2)

const upb_to_sv_test_case int32_upb_to_sv_test_cases[] = {
    {"repeated_int32", "repeated int32", kUpb_FieldType_Int32, set_repeated_int32_val, check_sv_repeated_int32_val, 9},
    {NULL} // Terminator
};

// Calculate the number of tests
int num_int32_upb_to_sv_tests = 0;
static void __attribute__((constructor)) init_num_int32_tests() {
    for (int i = 0; int32_upb_to_sv_test_cases[i].field_name; ++i) {
        num_int32_upb_to_sv_tests += 2 + int32_upb_to_sv_test_cases[i].num_checks;
    }
}

// sv_to_upb cases
SV* create_sv_int32_123(pTHX) { return newSViv(123); }

void check_upb_int32_123(const upb_MessageValue *val, const char *prefix) {
    is(val->int32_val, 123, sdiagnostic("%s: int32 value correct", prefix));
}

static SV* create_sv_repeated_int32(pTHX) {
    AV *av = newAV();
    av_push(av, newSViv(10));
    av_push(av, newSViv(20));
    return newRV_inc((SV*)av);
}

static void check_upb_repeated_int32(const upb_MessageValue *val, const char *prefix) {
    const upb_Array *arr = val->array_val;
    size_t upb_size = upb_Array_Size(arr);
    is(upb_size, 2, sdiagnostic("%s: array size", prefix));
    if (upb_size < 2) return;
    upb_MessageValue elem0 = upb_Array_Get(arr, 0);
    is(elem0.int32_val, 10, sdiagnostic("%s: element 0", prefix));
    upb_MessageValue elem1 = upb_Array_Get(arr, 1);
    is(elem1.int32_val, 20, sdiagnostic("%s: element 1", prefix));
}

const sv_to_upb_test_case int32_sv_to_upb_test_cases[] = {
    {"repeated_int32", "repeated int32", kUpb_FieldType_Int32, create_sv_repeated_int32, check_upb_repeated_int32, 3},
    {NULL} // Terminator
};
