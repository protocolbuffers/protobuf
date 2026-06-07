#ifndef PERL_PROTOBUF_MESSAGE_MESSAGE_H_
#define PERL_PROTOBUF_MESSAGE_MESSAGE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

// Creates a new Protobuf::Message wrapper, allocating a new upb_Message
// and upb_Arena internally.
SV* PerlUpb_Message_NewMessage(pTHX_ SV* descriptor_sv, uint16_t flags);

void PerlUpb_Message_MergeFrom(pTHX_ SV* dst_sv, SV* src_sv);
void PerlUpb_Message_CopyFrom(pTHX_ SV* dst_sv, SV* src_sv);

#endif  // PERL_PROTOBUF_MESSAGE_MESSAGE_H_
