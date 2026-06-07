#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor/method.h"

const char* PerlUpb_MethodDef_FullName(pTHX_ const upb_MethodDef *m) {
    return upb_MethodDef_FullName(m);
}

const char* PerlUpb_MethodDef_Name(pTHX_ const upb_MethodDef *m) {
    return upb_MethodDef_Name(m);
}

int PerlUpb_MethodDef_Index(pTHX_ const upb_MethodDef *m) {
    return upb_MethodDef_Index(m);
}

const upb_MessageDef* PerlUpb_MethodDef_InputType(pTHX_ const upb_MethodDef *m) {
    return upb_MethodDef_InputType(m);
}

const upb_MessageDef* PerlUpb_MethodDef_OutputType(pTHX_ const upb_MethodDef *m) {
    return upb_MethodDef_OutputType(m);
}

bool PerlUpb_MethodDef_ClientStreaming(pTHX_ const upb_MethodDef *m) {
    return upb_MethodDef_ClientStreaming(m);
}

bool PerlUpb_MethodDef_ServerStreaming(pTHX_ const upb_MethodDef *m) {
    return upb_MethodDef_ServerStreaming(m);
}

const upb_ServiceDef* PerlUpb_MethodDef_Service(pTHX_ const upb_MethodDef *m) {
    return upb_MethodDef_Service(m);
}
