#ifndef PERL_PROTOBUF_CONVERT_TYPES_INT32_H_
#define PERL_PROTOBUF_CONVERT_TYPES_INT32_H_

#include "t/c/convert/test_util.h"

void set_int32_123(upb_MessageValue* val, upb_Arena* arena);
void check_sv_int32_123(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case int32_upb_to_sv_test_cases[];

SV* create_sv_int32_123(pTHX);
void check_upb_int32_123(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case int32_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_INT32_H_
