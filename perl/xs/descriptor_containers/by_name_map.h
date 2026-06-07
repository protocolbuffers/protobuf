#ifndef PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_BY_NAME_MAP_H_
#define PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_BY_NAME_MAP_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "xs/protobuf.h"

typedef struct {
  int (*count)(const void* parent);
  const void* (*lookup)(const void* parent, const char* name);
  const char* (*key)(const void* parent, int index);
  const void* (*value)(const void* parent, int index);
  SV* (*wrap)(pTHX_ const void* item);
} PerlUpb_ByNameMap_VTable;

typedef struct {
  const void* parent;
  SV* parent_sv;
  const PerlUpb_ByNameMap_VTable* vtable;
} PerlUpb_ByNameMap;

SV* PerlUpb_ByNameMap_New(pTHX_ SV* parent_sv, const void* parent,
                          const PerlUpb_ByNameMap_VTable* vtable);
PerlUpb_ByNameMap* PerlUpb_ByNameMap_Get(pTHX_ SV* sv);

// Perl-facing functions (to be used in XS)
int PerlUpb_ByNameMap_Count(pTHX_ SV* self);
SV* PerlUpb_ByNameMap_Lookup(pTHX_ SV* self, const char* name);
SV* PerlUpb_ByNameMap_Key(pTHX_ SV* self, int index);
SV* PerlUpb_ByNameMap_Value(pTHX_ SV* self, int index);

// Returns a standard Perl hash containing all items in the map.
SV* PerlUpb_ByNameMap_AsHash(pTHX_ SV* self);

#endif  // PERL_PROTOBUF_DESCRIPTOR_CONTAINERS_BY_NAME_MAP_H_
