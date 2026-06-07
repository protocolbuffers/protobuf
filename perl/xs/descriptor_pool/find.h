#ifndef PERL_PROTOBUF_DESCRIPTOR_POOL_FIND_H_
#define PERL_PROTOBUF_DESCRIPTOR_POOL_FIND_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/protobuf.h"

SV* PerlUpb_DescriptorPool_FindFileByName(pTHX_ SV* self, const char* name);
SV* PerlUpb_DescriptorPool_FindMessageByName(pTHX_ SV* self, const char* name);
SV* PerlUpb_DescriptorPool_FindEnumByName(pTHX_ SV* self, const char* name);
SV* PerlUpb_DescriptorPool_FindServiceByName(pTHX_ SV* self, const char* name);
SV* PerlUpb_DescriptorPool_FindExtensionByName(pTHX_ SV* self,
                                               const char* name);

#endif  // PERL_PROTOBUF_DESCRIPTOR_POOL_FIND_H_
