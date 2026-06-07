#ifndef PERL_PROTOBUF_DESCRIPTOR_FILE_H_
#define PERL_PROTOBUF_DESCRIPTOR_FILE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/descriptor/base.h"

SV* PerlUpb_FileDef_GetWrapper(pTHX_ const upb_FileDef* f);
const upb_FileDef* PerlUpb_FileDef_GetFile(pTHX_ SV* sv);

#endif  // PERL_PROTOBUF_DESCRIPTOR_FILE_H_
