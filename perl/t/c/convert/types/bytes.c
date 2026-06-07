#include "t/c/convert/types/bytes.h"
#include "t/c/upb-perl-test.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include <stdint.h>
#include "xs/repeated/repeated.h"
#include <string.h>

// test_num is external from the main runner
extern int test_num;

void set_bytes_data(upb_MessageValue *val, upb_Arena *arena) { (void)arena; val->str_val = upb_StringView_FromDataAndSize("\x01\x02\x00\x03", 4); }

void check_sv_bytes_data(pTHX_ SV *sv, const char *prefix) {
    ok(SvPOK(sv), sdiagnostic("%s: SV is POK", prefix));
    STRLEN len;
    const char *ptr = SvPV(sv, len);
    ok(len == 4, sdiagnostic("%s: SV length correct", prefix));
    ok(memcmp(ptr, "\x01\x02\x00\x03", 4) == 0, sdiagnostic("%s: SV value correct", prefix));
}

static void set_repeated_bytes_val(upb_MessageValue *val, upb_Arena *arena) {
    upb_Array *arr = upb_Array_New(arena, kUpb_CType_String);
    upb_MessageValue msg_val;

    msg_val.str_val = upb_StringView_FromDataAndSize("\x0a\x0b", 2);
    upb_Array_Append(arr, msg_val, arena);

    msg_val.str_val = upb_StringView_FromDataAndSize("\x0c\x0d\x0e", 3);
    upb_Array_Append(arr, msg_val, arena);

    val->array_val = arr;
}

GEN_CHECK_SV_REPEATED_SCALAR(bytes, 2)

const upb_to_sv_test_case bytes_upb_to_sv_test_cases[] = {
    {"optional_bytes", "bytes", kUpb_FieldType_Bytes, set_bytes_data, check_sv_bytes_data, 3},
    {"repeated_bytes", "repeated bytes", kUpb_FieldType_Bytes, set_repeated_bytes_val, check_sv_repeated_bytes_val, 9},
    {NULL} // Terminator
};

// sv_to_upb cases
SV* create_sv_bytes_data(pTHX) { return newSVpvn("\x01\x02\x00\x03", 4); }

void check_upb_bytes_data(const upb_MessageValue *val, const char *prefix) {
    const char *byte_data = "\x01\x02\x00\x03";
    is_string_view(val->str_val, byte_data, 4, sdiagnostic("%s: bytes value correct", prefix));
}

const sv_to_upb_test_case bytes_sv_to_upb_test_cases[] = {
    {"optional_bytes", "bytes", kUpb_FieldType_Bytes, create_sv_bytes_data, check_upb_bytes_data, 1},
    {NULL} // Terminator
};
