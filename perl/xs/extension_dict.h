#ifndef PERL_EXTENSION_DICT_H_
#define PERL_EXTENSION_DICT_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/extension_dict/dict.h"
#include "xs/extension_dict/iterator.h"
#include "xs/protobuf.h"

// Top-level init
bool PerlUpb_InitExtensionDict(pTHX_ SV* module);

#endif  // PERL_EXTENSION_DICT_H_
