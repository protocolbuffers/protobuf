#ifndef PERL_MAP_H_
#define PERL_MAP_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/map/iterator.h"
#include "xs/map/map.h"
#include "xs/protobuf.h"

// Top-level init
bool PerlUpb_InitMap(pTHX_ SV* module);

#endif  // PERL_MAP_H_
