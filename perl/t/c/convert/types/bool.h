#ifndef PERL_PROTOBUF_CONVERT_TYPES_BOOL_H_
#define PERL_PROTOBUF_CONVERT_TYPES_BOOL_H_

#include "t/c/convert/test_util.h"

void set_bool_true(upb_MessageValue* val, upb_Arena* arena);
void set_bool_false(upb_MessageValue* val, upb_Arena* arena);
void check_sv_bool_true(pTHX_ SV* sv, const char* prefix);
void check_sv_bool_false(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case bool_upb_to_sv_test_cases[];

SV* create_sv_bool_true(pTHX);
SV* create_sv_bool_false(pTHX);
void check_upb_bool_true(const upb_MessageValue* val, const char* prefix);
void check_upb_bool_false(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case bool_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_BOOL_H_
