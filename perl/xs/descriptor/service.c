#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor/service.h"

const char* PerlUpb_ServiceDef_FullName(pTHX_ const upb_ServiceDef *s) {
    return upb_ServiceDef_FullName(s);
}

const char* PerlUpb_ServiceDef_Name(pTHX_ const upb_ServiceDef *s) {
    return upb_ServiceDef_Name(s);
}

int PerlUpb_ServiceDef_Index(pTHX_ const upb_ServiceDef *s) {
    return upb_ServiceDef_Index(s);
}

int PerlUpb_ServiceDef_MethodCount(pTHX_ const upb_ServiceDef *s) {
    return upb_ServiceDef_MethodCount(s);
}

const upb_MethodDef* PerlUpb_ServiceDef_Method(pTHX_ const upb_ServiceDef *s, int i) {
    return upb_ServiceDef_Method(s, i);
}

const upb_MethodDef* PerlUpb_ServiceDef_FindMethodByName(pTHX_ const upb_ServiceDef *s, const char *name) {
    return upb_ServiceDef_FindMethodByName(s, name);
}

const upb_FileDef* PerlUpb_ServiceDef_File(pTHX_ const upb_ServiceDef *s) {
    return upb_ServiceDef_File(s);
}

SV* PerlUpb_ServiceDef_GetWrapper(pTHX_ const upb_ServiceDef *s) {
    if (!s) return &PL_sv_undef;
    RETURN_CACHED_OR_CREATE_BLESSED(s, "Protobuf::Descriptor::Service");
}
