#ifndef PERL_REPEATED_H_
#define PERL_REPEATED_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"
#include "xs/repeated/composite.h"
#include "xs/repeated/repeated.h"

// Top-level init
bool PerlUpb_InitRepeated(pTHX_ SV* module);

#endif  // PERL_REPEATED_H_
