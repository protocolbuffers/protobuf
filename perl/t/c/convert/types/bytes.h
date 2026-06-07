#ifndef PERL_PROTOBUF_CONVERT_TYPES_BYTES_H_
#define PERL_PROTOBUF_CONVERT_TYPES_BYTES_H_

#include "t/c/convert/test_util.h"

void set_bytes_data(upb_MessageValue* val, upb_Arena* arena);
void check_sv_bytes_data(pTHX_ SV* sv, const char* prefix);
extern const upb_to_sv_test_case bytes_upb_to_sv_test_cases[];

SV* create_sv_bytes_data(pTHX);
void check_upb_bytes_data(const upb_MessageValue* val, const char* prefix);
extern const sv_to_upb_test_case bytes_sv_to_upb_test_cases[];

#endif  // PERL_PROTOBUF_CONVERT_TYPES_BYTES_H_
