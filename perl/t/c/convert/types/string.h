#ifndef PERL_PROTOBUF_CONVERT_TYPES_STRING_H_
#define PERL_PROTOBUF_CONVERT_TYPES_STRING_H_

#include "t/c/convert/test_util.h"

void set_string_hello(upb_MessageValue* val, upb_Arena* arena);
void check_sv_string_hello(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case string_upb_to_sv_test_cases[];

SV* create_sv_string_hello(pTHX);
void check_upb_string_hello(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case string_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_STRING_H_
