#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor/enum.h"

#include "xs/protobuf/obj_cache.h"
#include "xs/descriptor/base.h"

SV* PerlUpb_EnumDef_GetWrapper(pTHX_ const upb_EnumDef *e) {
    RETURN_CACHED_OR_CREATE_BLESSED(e, "Protobuf::Descriptor::Enum");
}

const upb_EnumDef* PerlUpb_EnumDef_GetEnum(pTHX_ SV *sv) {
    EXTRACT_CACHED_DESCRIPTOR(upb_EnumDef, sv, "Protobuf::Descriptor::Enum");
}
