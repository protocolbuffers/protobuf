#ifndef PERL_PROTOBUF_UPB_TO_SV_H_
#define PERL_PROTOBUF_UPB_TO_SV_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/message/value.h"
#include "upb/reflection/def.h"

SV* PerlUpb_UpbToSv(pTHX_ const upb_MessageValue* val, const upb_FieldDef* f,
                    SV* parent_arena_sv);

// Converts a singular upb_MessageValue to a Perl SV, even if the field is
// repeated. Used for elements of repeated fields or maps.
SV* PerlUpb_UpbToSv_Element(pTHX_ const upb_MessageValue* val,
                            const upb_FieldDef* f, SV* parent_arena_sv);

// Batch Conversion (VPP Style)
void PerlUpb_UpbToSv_Batch(pTHX_ const upb_MessageValue* vals,
                           const upb_FieldDef* f, SV* parent_arena_sv, SV** out,
                           size_t count);
void PerlUpb_UpbToSv_BatchRaw(pTHX_ const void* data, upb_CType type, SV** out,
                              size_t count);

// Converts a whole upb_Message to a deep Perl HashRef.
SV* PerlUpb_Message_ToSv(pTHX_ const upb_Message* msg,
                         const upb_MessageDef* mdef, SV* parent_arena_sv);

#endif  // PERL_PROTOBUF_UPB_TO_SV_H_
