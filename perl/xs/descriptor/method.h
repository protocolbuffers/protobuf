#ifndef PERL_PROTOBUF_DESCRIPTOR_METHOD_H_
#define PERL_PROTOBUF_DESCRIPTOR_METHOD_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/descriptor/base.h"

const char* PerlUpb_MethodDef_FullName(pTHX_ const upb_MethodDef* m);
const char* PerlUpb_MethodDef_Name(pTHX_ const upb_MethodDef* m);
int PerlUpb_MethodDef_Index(pTHX_ const upb_MethodDef* m);
const upb_MessageDef* PerlUpb_MethodDef_InputType(pTHX_ const upb_MethodDef* m);
const upb_MessageDef* PerlUpb_MethodDef_OutputType(
    pTHX_ const upb_MethodDef* m);
bool PerlUpb_MethodDef_ClientStreaming(pTHX_ const upb_MethodDef* m);
bool PerlUpb_MethodDef_ServerStreaming(pTHX_ const upb_MethodDef* m);
const upb_ServiceDef* PerlUpb_MethodDef_Service(pTHX_ const upb_MethodDef* m);

#endif  // PERL_PROTOBUF_DESCRIPTOR_METHOD_H_
