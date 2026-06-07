#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "xs/protobuf.h"
#include "xs/protobuf/registry.h"

#include "xs/protobuf/utils.h"

XS(XS_Protobuf_Internal_get_cache_audit_log) {
    dXSARGS;
    if (items != 0) {
        croak_xs_usage(cv, "()");
    }

    SV* result = PerlUpb_ObjCache_GetAuditLog(aTHX);
    ST(0) = result;
    XSRETURN(1);
}

XS(XS_Protobuf_Internal_set_cache_capacity) {
    dXSARGS;
    if (items != 1) {
        croak_xs_usage(cv, "capacity");
    }
    size_t capacity = (size_t)SvUV(ST(0));
    PerlUpb_ObjCache_SetCapacity(aTHX_ capacity);
    XSRETURN_EMPTY;
}

XS(XS_Protobuf_Internal_get_cache_capacity) {
    dXSARGS;
    if (items != 0) {
        croak_xs_usage(cv, "()");
    }
    size_t capacity = PerlUpb_ObjCache_GetCapacity(aTHX);
    ST(0) = newSVuv(capacity);
    XSRETURN(1);
}

XS(XS_Protobuf_Internal_clear_cache) {
    dXSARGS;
    if (items != 0) {
        croak_xs_usage(cv, "()");
    }
    PerlUpb_ObjCache_Clear(aTHX);
    XSRETURN_EMPTY;
}

// Initialize all sub-components
void PerlUpb_Protobuf_InitModule(pTHX) {
    PerlUpb_Registry_Init(aTHX);
    PerlUpb_ObjCache_Init(aTHX);
    PerlUpb_InitCpuFeatures();
    newXS("Protobuf::Internal::get_cache_audit_log", XS_Protobuf_Internal_get_cache_audit_log, __FILE__);
    newXS("Protobuf::Internal::set_cache_capacity", XS_Protobuf_Internal_set_cache_capacity, __FILE__);
    newXS("Protobuf::Internal::get_cache_capacity", XS_Protobuf_Internal_get_cache_capacity, __FILE__);
    newXS("Protobuf::Internal::clear_cache", XS_Protobuf_Internal_clear_cache, __FILE__);
    // Other initializations if needed
}

// get_descriptor_proto_fds() function remains here for now
SV* get_descriptor_proto_fds(void) {
    dTHX;
    // This is a stub for now or would return the serialized descriptor data.
    return &PL_sv_undef;
}
