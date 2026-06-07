#ifndef PERL_PROTOBUF_MESSAGE_COMPARE_H_
#define PERL_PROTOBUF_MESSAGE_COMPARE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

bool PerlUpb_Message_IsEqual(pTHX_ SV* message1_sv, SV* message2_sv);

#endif  // PERL_PROTOBUF_MESSAGE_COMPARE_H_
