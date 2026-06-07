#ifndef PERL_DESCRIPTOR_POOL_H_
#define PERL_DESCRIPTOR_POOL_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/descriptor_pool/add.h"
#include "xs/descriptor_pool/find.h"
#include "xs/descriptor_pool/pool.h"
#include "xs/protobuf.h"

// Top-level init
bool PerlUpb_InitDescriptorPool(pTHX_ SV* module);

#endif  // PERL_DESCRIPTOR_POOL_H_
