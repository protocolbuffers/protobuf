#ifndef PERL_PROTOBUF_SV_TO_UPB_H_
#define PERL_PROTOBUF_SV_TO_UPB_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/mem/arena.h"
#include "upb/message/value.h"
#include "upb/reflection/def.h"

bool PerlUpb_SvToUpb(pTHX_ SV* sv, const upb_FieldDef* f, upb_MessageValue* val,
                     upb_Arena* arena);

// Converts a singular Perl SV to a upb_MessageValue, even if the field is
// repeated. Used for elements of repeated fields or maps.
bool PerlUpb_SvToUpb_Element(pTHX_ SV* sv, const upb_FieldDef* f,
                             upb_MessageValue* val, upb_Arena* arena);

// Batch Conversion (VPP Style)
void PerlUpb_SvToUpb_BatchRaw(pTHX_ SV** src, upb_FieldType type, void* dst,
                              size_t count, const upb_FieldDef* f,
                              upb_Arena* arena);

#endif  // PERL_PROTOBUF_SV_TO_UPB_H_
