#ifndef PERL_PROTOBUF_DESCRIPTOR_ONEOF_H_
#define PERL_PROTOBUF_DESCRIPTOR_ONEOF_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/descriptor/base.h"

SV* PerlUpb_OneofDef_GetWrapper(pTHX_ const upb_OneofDef* o);
const upb_OneofDef* PerlUpb_OneofDef_GetOneof(pTHX_ SV* sv);

#endif  // PERL_PROTOBUF_DESCRIPTOR_ONEOF_H_
