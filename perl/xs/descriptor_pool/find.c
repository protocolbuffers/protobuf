#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor_pool/find.h"
#include "xs/descriptor_pool/pool.h"
#include "xs/descriptor/file.h"
#include "xs/descriptor/message.h"
#include "xs/descriptor/enum.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/service.h"
#include "upb/reflection/def.h"

SV* PerlUpb_DescriptorPool_FindFileByName(pTHX_ SV* self, const char* name) {
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ self);
    if (!pool) return &PL_sv_undef;
    const upb_FileDef* file = upb_DefPool_FindFileByName(pool, name);
    return PerlUpb_FileDef_GetWrapper(aTHX_ file);
}

SV* PerlUpb_DescriptorPool_FindMessageByName(pTHX_ SV* self, const char* name) {
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ self);
    if (!pool) return &PL_sv_undef;
    const upb_MessageDef* msg = upb_DefPool_FindMessageByName(pool, name);
    return PerlUpb_MessageDef_GetWrapper(aTHX_ msg);
}

SV* PerlUpb_DescriptorPool_FindEnumByName(pTHX_ SV* self, const char* name) {
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ self);
    if (!pool) return &PL_sv_undef;
    const upb_EnumDef* enm = upb_DefPool_FindEnumByName(pool, name);
    return PerlUpb_EnumDef_GetWrapper(aTHX_ enm);
}

SV* PerlUpb_DescriptorPool_FindServiceByName(pTHX_ SV* self, const char* name) {
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ self);
    if (!pool) return &PL_sv_undef;
    const upb_ServiceDef* svc = upb_DefPool_FindServiceByName(pool, name);
    return PerlUpb_ServiceDef_GetWrapper(aTHX_ svc);
}

SV* PerlUpb_DescriptorPool_FindExtensionByName(pTHX_ SV* self, const char* name) {
    const upb_DefPool* pool = PerlUpb_DescriptorPool_GetPool(aTHX_ self);
    if (!pool) return &PL_sv_undef;
    const upb_FieldDef* ext = upb_DefPool_FindExtensionByName(pool, name);
    return PerlUpb_FieldDef_GetWrapper(aTHX_ ext);
}
