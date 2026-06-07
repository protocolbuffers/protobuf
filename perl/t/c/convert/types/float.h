#ifndef PERL_PROTOBUF_CONVERT_TYPES_FLOAT_H_
#define PERL_PROTOBUF_CONVERT_TYPES_FLOAT_H_

#include "t/c/convert/test_util.h"

void set_float_123_456(upb_MessageValue* val, upb_Arena* arena);
void check_sv_float_123_456(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case float_upb_to_sv_test_cases[];

SV* create_sv_float_123_456(pTHX);
void check_upb_float_123_456(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case float_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_FLOAT_H_
