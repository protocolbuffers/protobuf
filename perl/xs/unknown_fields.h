#ifndef PERL_UNKNOWN_FIELDS_H_
#define PERL_UNKNOWN_FIELDS_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"
#include "xs/unknown_fields/set.h"

// Top-level init
bool PerlUpb_InitUnknownFields(pTHX_ SV* module);

#endif  // PERL_UNKNOWN_FIELDS_H_
