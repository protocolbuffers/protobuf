#ifndef PERL_PROTOBUF_REPEATED_COMPOSITE_H_
#define PERL_PROTOBUF_REPEATED_COMPOSITE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/repeated/repeated.h"

// Adds a new message to a repeated field of message types and returns it.
SV* PerlUpb_Repeated_Add(pTHX_ SV* self);

#endif  // PERL_PROTOBUF_REPEATED_COMPOSITE_H_
