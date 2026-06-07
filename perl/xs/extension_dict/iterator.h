#ifndef PERL_PROTOBUF_EXTENSION_DICT_ITERATOR_H_
#define PERL_PROTOBUF_EXTENSION_DICT_ITERATOR_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

// Iterator for ExtensionDict
SV* PerlUpb_ExtensionDict_GetIterator(pTHX_ SV* dict_sv);

// Returns the next extension field (as a Protobuf::FieldDescriptor)
SV* PerlUpb_ExtensionDict_Iterator_Next(pTHX_ SV* self);

#endif  // PERL_PROTOBUF_EXTENSION_DICT_ITERATOR_H_
