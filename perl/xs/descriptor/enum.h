#ifndef PERLUPB_DESCRIPTOR_ENUM_H
#define PERLUPB_DESCRIPTOR_ENUM_H

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"

SV* PerlUpb_EnumDef_GetWrapper(pTHX_ const upb_EnumDef* e);
const upb_EnumDef* PerlUpb_EnumDef_GetEnum(pTHX_ SV* sv);

#endif /* PERLUPB_DESCRIPTOR_ENUM_H */
