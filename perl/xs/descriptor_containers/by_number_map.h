#ifndef PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_BY_NUMBER_MAP_H_
#define PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_BY_NUMBER_MAP_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

typedef struct {
  int (*count)(const void* parent);
  const void* (*lookup)(const void* parent, uint32_t num);
  uint32_t (*key)(const void* parent, int index);
  const void* (*value)(const void* parent, int index);
  SV* (*wrap)(pTHX_ const void* item);
} PerlUpb_ByNumberMap_VTable;

typedef struct {
  const void* parent;
  SV* parent_sv;
  const PerlUpb_ByNumberMap_VTable* vtable;
} PerlUpb_ByNumberMap;

SV* PerlUpb_ByNumberMap_New(pTHX_ SV* parent_sv, const void* parent,
                            const PerlUpb_ByNumberMap_VTable* vtable);
PerlUpb_ByNumberMap* PerlUpb_ByNumberMap_Get(pTHX_ SV* sv);

// Perl-facing functions (to be used in XS)
int PerlUpb_ByNumberMap_Count(pTHX_ SV* self);
SV* PerlUpb_ByNumberMap_Lookup(pTHX_ SV* self, uint32_t num);
SV* PerlUpb_ByNumberMap_Key(pTHX_ SV* self, int index);
SV* PerlUpb_ByNumberMap_Value(pTHX_ SV* self, int index);

#endif  // PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_BY_NUMBER_MAP_H_
