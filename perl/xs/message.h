#ifndef PERL_MESSAGE_H_
#define PERL_MESSAGE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/message/access.h"
#include "xs/message/compare.h"
#include "xs/message/message.h"
#include "xs/message/meta.h"
#include "xs/message/serialize.h"
#include "xs/protobuf.h"

// Top-level init
bool PerlUpb_InitMessage(pTHX_ SV* module);

// Forward declaration needed by other parts like convert/upb_to_sv.c
struct upb_MessageDef;
struct upb_Arena;
struct upb_Message;
SV* PerlUpb_Message_Get(pTHX_ const struct upb_Message* msg,
                        const struct upb_MessageDef* m, SV* arena_sv);

#endif  // PERL_MESSAGE_H_
