#ifndef PERL_PROTOBUF_DESCRIPTOR_MESSAGE_H_
#define PERL_PROTOBUF_DESCRIPTOR_MESSAGE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/descriptor/base.h"

const upb_FieldDef* PerlUpb_MessageDef_FindFieldByNameWithSize(
    pTHX_ const upb_MessageDef* m, const char* name, size_t len);

SV* PerlUpb_MessageDef_GetWrapper(pTHX_ const upb_MessageDef* m);
const upb_MessageDef* PerlUpb_MessageDef_GetMessage(pTHX_ SV* sv);

// Returns the full name of the message as a Perl SV
SV* PerlUpb_Message_FullName(pTHX_ const upb_MessageDef* m);
SV* PerlUpb_Message_File(pTHX_ const upb_MessageDef* m);
SV* PerlUpb_MessageDef_PerlClassName(pTHX_ const upb_MessageDef* m);

// Fingerprinting and ultra-fast lookup
uint64_t PerlUpb_MessageDef_GetFingerprint(pTHX_ const upb_MessageDef* m);
void PerlUpb_MessageDef_RegisterFingerprint(pTHX_ const upb_MessageDef* m);
const upb_MessageDef* PerlUpb_MessageDef_FindByFingerprint(
    pTHX_ uint64_t fingerprint);

// Audit identity connection for descriptor
void PerlUpb_MessageDef_AuditIdentity(pTHX_ SV* self);

#endif  // PERL_PROTOBUF_DESCRIPTOR_MESSAGE_H_
