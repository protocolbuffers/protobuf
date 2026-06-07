#ifndef PERL_PROTOBUF_MAP_ITERATOR_H_
#define PERL_PROTOBUF_MAP_ITERATOR_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/map/map.h"

// MapIterator provides iteration over a upb_Map.
SV* PerlUpb_Map_GetIterator(pTHX_ SV* map_sv);

// Returns the next key (as an SV) or undef if done.
SV* PerlUpb_Map_Iterator_NextKey(pTHX_ SV* self);

// Returns the value for the CURRENT key.
SV* PerlUpb_Map_Iterator_Value(pTHX_ SV* self);

#endif  // PERL_PROTOBUF_MAP_ITERATOR_H_
