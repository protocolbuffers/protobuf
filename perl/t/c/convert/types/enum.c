#include "t/c/convert/types/enum.h"
#include "t/c/upb-perl-test.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"

// test_num is external from the main runner
extern int test_num;

void set_enum_bar(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->int32_val = 2; }

void check_sv_enum_bar(pTHX_ SV *sv, const char *prefix) {
    ok(SvIOK(sv), sdiagnostic("%s: SV is IOK", prefix));
    is(SvIV(sv), 2, sdiagnostic("%s: SV value correct", prefix));
}

static void set_repeated_enum_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_Int32);
    upb_MessageValue msg_val;

    msg_val.int32_val = 1; // FOO
    upb_Array_Append(arr, msg_val, arena);

    msg_val.int32_val = 2; // BAR
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(enum, 2)

const upb_to_sv_test_case enum_upb_to_sv_test_cases[] = {
    {"optional_nested_enum", "optional enum", kUpb_FieldType_Enum, set_enum_bar, check_sv_enum_bar, 2},
    {"repeated_nested_enum", "repeated enum", kUpb_FieldType_Enum, set_repeated_enum_val, check_sv_repeated_enum_val, 7},
    {NULL} // Terminator
};

// sv_to_upb cases
SV* create_sv_enum_bar(pTHX) { return newSViv(2); }

void check_upb_enum_bar(const upb_MessageValue *val, const char *prefix) {
    is(val->int32_val, 2, sdiagnostic("%s: enum value correct", prefix));
}

const sv_to_upb_test_case enum_sv_to_upb_test_cases[] = {
    {"optional_nested_enum", "enum", kUpb_FieldType_Enum, create_sv_enum_bar, check_upb_enum_bar, 1},
    {NULL} // Terminator
};
