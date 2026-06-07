#ifndef PERL_PROTOBUF_CONVERT_TYPES_FIXED64_H_
#define PERL_PROTOBUF_CONVERT_TYPES_FIXED64_H_

#include "t/c/convert/test_util.h"

void set_fixed64_val(upb_MessageValue* val, upb_Arena* arena);
void check_sv_fixed64_val(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case fixed64_upb_to_sv_test_cases[];

SV* create_sv_fixed64_val(pTHX);
void check_upb_fixed64_val(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case fixed64_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_FIXED64_H_
