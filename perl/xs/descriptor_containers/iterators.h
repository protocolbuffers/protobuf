#ifndef PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_ITERATORS_H_
#define PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_ITERATORS_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

typedef struct {
  SV* container_sv;
  int index;
} PerlUpb_DescriptorMapIterator;

SV* PerlUpb_DescriptorMapIterator_New(pTHX_ SV* container_sv);
PerlUpb_DescriptorMapIterator* PerlUpb_DescriptorMapIterator_Get(pTHX_ SV* sv);

// Returns next key/value pair as a list (or undef if done)
// These will be used by the Perl-level iterator methods
SV* PerlUpb_DescriptorMapIterator_NextKey(pTHX_ SV* self);
SV* PerlUpb_DescriptorMapIterator_NextValue(pTHX_ SV* self);

#endif  // PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_ITERATORS_H_
