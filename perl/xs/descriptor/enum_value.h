#include <setjmp.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef PERLUPB_DESCRIPTOR_ENUM_VALUE_H
#define PERLUPB_DESCRIPTOR_ENUM_VALUE_H

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"

const char* PerlUpb_EnumValueDef_Name(pTHX_ const upb_EnumValueDef* ev);
int32_t PerlUpb_EnumValueDef_Number(pTHX_ const upb_EnumValueDef* ev);
int PerlUpb_EnumValueDef_Index(pTHX_ const upb_EnumValueDef* ev);

SV* PerlUpb_EnumValueDef_GetWrapper(pTHX_ const upb_EnumValueDef* ev);
const upb_EnumValueDef* PerlUpb_EnumValueDef_GetEnumValue(pTHX_ SV* sv);

#endif /* PERLUPB_DESCRIPTOR_ENUM_VALUE_H */
