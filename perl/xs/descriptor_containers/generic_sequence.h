#ifndef PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_GENERIC_SEQUENCE_H_
#define PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_GENERIC_SEQUENCE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

typedef struct {
  int (*count)(const void* parent);
  const void* (*get)(const void* parent, int index);
  SV* (*wrap)(pTHX_ const void* item);
} PerlUpb_GenericSequence_VTable;

typedef struct {
  const void* parent;
  SV* parent_sv;
  const PerlUpb_GenericSequence_VTable* vtable;
} PerlUpb_GenericSequence;

SV* PerlUpb_GenericSequence_New(pTHX_ SV* parent_sv, const void* parent,
                                const PerlUpb_GenericSequence_VTable* vtable);
PerlUpb_GenericSequence* PerlUpb_GenericSequence_Get(pTHX_ SV* sv);

// Perl-facing functions (to be used in XS)
int PerlUpb_GenericSequence_Count(pTHX_ SV* self);
SV* PerlUpb_GenericSequence_GetItem(pTHX_ SV* self, int index);

#endif  // PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_GENERIC_SEQUENCE_H_
