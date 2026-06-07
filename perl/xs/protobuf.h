#ifndef PERL_PROTOBUF_H_
#define PERL_PROTOBUF_H_

#include "EXTERN.h"
#include "perl.h"                // NOLINT(build/include)
#include "upb/reflection/def.h"  // Keep for now, might be needed by other components
#include "upb/wire/types.h"
#include "xs/descriptor/message.h"
#include "xs/protobuf/arena.h"
#include "xs/protobuf/message.h"
#include "xs/protobuf/obj_cache.h"
#include "xs/protobuf/utils.h"

// Top-level module initialization
void PerlUpb_Protobuf_InitModule(pTHX);

SV* get_descriptor_proto_fds(void);  // Remains here for now

#endif  // PERL_PROTOBUF_H_
