#ifndef PERL_PROTOBUF_CONVERT_TYPES_UINT64_H_
#define PERL_PROTOBUF_CONVERT_TYPES_UINT64_H_

#include "t/c/convert/test_util.h"

void set_uint64_max(upb_MessageValue* val, upb_Arena* arena);
void check_sv_uint64_val(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case uint64_upb_to_sv_test_cases[];

SV* create_sv_uint64_max(pTHX);
void check_upb_uint64_max(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case uint64_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_UINT64_H_
