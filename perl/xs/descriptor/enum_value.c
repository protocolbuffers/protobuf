#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor/enum_value.h"

const char* PerlUpb_EnumValueDef_Name(pTHX_ const upb_EnumValueDef *ev) {
    return upb_EnumValueDef_Name(ev);
}

int32_t PerlUpb_EnumValueDef_Number(pTHX_ const upb_EnumValueDef *ev) {
    return upb_EnumValueDef_Number(ev);
}

int PerlUpb_EnumValueDef_Index(pTHX_ const upb_EnumValueDef *ev) {
    return upb_EnumValueDef_Index(ev);
}

#include "xs/protobuf/obj_cache.h"
#include "xs/descriptor/base.h"

SV* PerlUpb_EnumValueDef_GetWrapper(pTHX_ const upb_EnumValueDef *ev) {
    RETURN_CACHED_OR_CREATE_BLESSED(ev, "Protobuf::Descriptor::EnumValue");
}

const upb_EnumValueDef* PerlUpb_EnumValueDef_GetEnumValue(pTHX_ SV *sv) {
    EXTRACT_CACHED_DESCRIPTOR(upb_EnumValueDef, sv, "Protobuf::Descriptor::EnumValue");
}
