#ifndef PERL_PROTOBUF_DESCRIPTOR_SERVICE_H_
#define PERL_PROTOBUF_DESCRIPTOR_SERVICE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/descriptor/base.h"

const char* PerlUpb_ServiceDef_FullName(pTHX_ const upb_ServiceDef* s);
const char* PerlUpb_ServiceDef_Name(pTHX_ const upb_ServiceDef* s);
int PerlUpb_ServiceDef_Index(pTHX_ const upb_ServiceDef* s);
int PerlUpb_ServiceDef_MethodCount(pTHX_ const upb_ServiceDef* s);
const upb_MethodDef* PerlUpb_ServiceDef_Method(pTHX_ const upb_ServiceDef* s,
                                               int i);
const upb_MethodDef* PerlUpb_ServiceDef_FindMethodByName(
    pTHX_ const upb_ServiceDef* s, const char* name);
const upb_FileDef* PerlUpb_ServiceDef_File(pTHX_ const upb_ServiceDef* s);

SV* PerlUpb_ServiceDef_GetWrapper(pTHX_ const upb_ServiceDef* s);

#endif  // PERL_PROTOBUF_DESCRIPTOR_SERVICE_H_
