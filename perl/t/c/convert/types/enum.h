#ifndef PERL_PROTOBUF_CONVERT_TYPES_ENUM_H_
#define PERL_PROTOBUF_CONVERT_TYPES_ENUM_H_

#include "t/c/convert/test_util.h"

void set_enum_bar(upb_MessageValue* val, upb_Arena* arena);
void check_sv_enum_bar(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case enum_upb_to_sv_test_cases[];

SV* create_sv_enum_bar(pTHX);
void check_upb_enum_bar(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case enum_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_ENUM_H_
