#ifndef PERL_PROTOBUF_MESSAGE_ACCESS_H_
#define PERL_PROTOBUF_MESSAGE_ACCESS_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/protobuf.h"

SV* PerlUpb_Message_GetField(pTHX_ SV* message_sv, const upb_FieldDef* f);
void PerlUpb_Message_SetField(pTHX_ SV* message_sv, const upb_FieldDef* f,
                              SV* val_sv);
bool PerlUpb_Message_HasField(pTHX_ SV* message_sv, const upb_FieldDef* f);
void PerlUpb_Message_ClearField(pTHX_ SV* message_sv, const upb_FieldDef* f);
const char* PerlUpb_Message_WhichOneof(pTHX_ SV* message_sv,
                                       const upb_OneofDef* o);
void PerlUpb_Message_Clear(pTHX_ SV* message_sv);
SV* PerlUpb_Message_ToPerl(pTHX_ SV* message_sv);
SV* PerlUpb_Message_Fields(pTHX_ SV* message_sv);

#endif  // PERL_PROTOBUF_MESSAGE_ACCESS_H_
