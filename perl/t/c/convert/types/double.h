#ifndef PERL_PROTOBUF_CONVERT_TYPES_DOUBLE_H_
#define PERL_PROTOBUF_CONVERT_TYPES_DOUBLE_H_

#include "t/c/convert/test_util.h"

void set_double_789_123(upb_MessageValue* val, upb_Arena* arena);
void check_sv_double_789_123(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case double_upb_to_sv_test_cases[];

SV* create_sv_double_789_123(pTHX);
void check_upb_double_789_123(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case double_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_DOUBLE_H_
