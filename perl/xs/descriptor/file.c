#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "xs/descriptor/file.h"

#include "xs/protobuf/obj_cache.h"
#include "xs/descriptor/base.h"

SV* PerlUpb_FileDef_GetWrapper(pTHX_ const upb_FileDef *f) {
    RETURN_CACHED_OR_CREATE_BLESSED(f, "Protobuf::Descriptor::File");
}

const upb_FileDef* PerlUpb_FileDef_GetFile(pTHX_ SV *sv) {
    EXTRACT_CACHED_DESCRIPTOR(upb_FileDef, sv, "Protobuf::Descriptor::File");
}
