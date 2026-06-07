#include <setjmp.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef PERLUPB_DESCRIPTOR_FIELD_H
#define PERLUPB_DESCRIPTOR_FIELD_H

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/base/descriptor_constants.h"
#include "upb/reflection/def.h"

SV* PerlUpb_FieldDef_GetWrapper(pTHX_ const upb_FieldDef* f);
const upb_FieldDef* PerlUpb_FieldDef_GetField(pTHX_ SV* sv);

SV* PerlUpb_FieldDef_Name(pTHX_ const upb_FieldDef* f);
const char* PerlUpb_FieldDef_TypeName(pTHX_ const upb_FieldDef* f);
const char* PerlUpb_FieldDef_LabelName(pTHX_ const upb_FieldDef* f);
int PerlUpb_FieldDef_Type(pTHX_ const upb_FieldDef* f);
int PerlUpb_FieldDef_Label(pTHX_ const upb_FieldDef* f);
uint32_t PerlUpb_FieldDef_Number(pTHX_ const upb_FieldDef* f);
bool PerlUpb_FieldDef_IsRepeated(pTHX_ const upb_FieldDef* f);
bool PerlUpb_FieldDef_IsMap(pTHX_ const upb_FieldDef* f);
bool PerlUpb_FieldDef_IsSubMessage(pTHX_ const upb_FieldDef* f);
bool PerlUpb_FieldDef_IsExtension(pTHX_ const upb_FieldDef* f);
bool PerlUpb_FieldDef_IsPacked(pTHX_ const upb_FieldDef* f);

#endif  // PERL_PROTOBUF_DESCRIPTOR_FIELD_H_
