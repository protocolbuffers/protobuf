#ifndef PERL_PROTOBUF_DESCRIPTOR_POOL_ADD_H_
#define PERL_PROTOBUF_DESCRIPTOR_POOL_ADD_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/protobuf.h"

// Adds a serialized FileDescriptorProto to the pool.
// Returns the Perl wrapper for the new upb_FileDef.
SV* PerlUpb_DescriptorPool_AddSerializedFile(pTHX_ SV* self, SV* serialized);
SV* PerlUpb_DescriptorPool_AddSerializedFileDescriptorSet(pTHX_ SV* self,
                                                          SV* serialized);

#endif  // PERL_PROTOBUF_DESCRIPTOR_POOL_ADD_H_
