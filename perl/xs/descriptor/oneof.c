#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor/oneof.h"

#include "xs/protobuf/obj_cache.h"
#include "xs/descriptor/base.h"

SV* PerlUpb_OneofDef_GetWrapper(pTHX_ const upb_OneofDef *o) {
    RETURN_CACHED_OR_CREATE_BLESSED(o, "Protobuf::Descriptor::OneofDef");
}

const upb_OneofDef* PerlUpb_OneofDef_GetOneof(pTHX_ SV *sv) {
    EXTRACT_CACHED_DESCRIPTOR(upb_OneofDef, sv, "Protobuf::Descriptor::OneofDef");
}
