#ifndef PERL_PROTOBUF_DESCRIPTOR_POOL_POOL_H_
#define PERL_PROTOBUF_DESCRIPTOR_POOL_POOL_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/reflection/def.h"
#include "xs/protobuf.h"

// Creates a NEW upb_DefPool and returns its Perl wrapper.
SV* PerlUpb_DescriptorPool_New(pTHX);

// Returns a Perl wrapper for a upb_DefPool.
// Uses the object cache.
SV* PerlUpb_DescriptorPool_GetWrapper(pTHX_ const upb_DefPool* pool);

// Returns the underlying upb_DefPool from a Perl wrapper.
const upb_DefPool* PerlUpb_DescriptorPool_GetPool(pTHX_ SV* sv);

// Freezing
void PerlUpb_DescriptorPool_Freeze(pTHX_ SV* sv);
bool PerlUpb_DescriptorPool_IsFrozen(pTHX_ SV* sv);

// Frees the descriptor pool wrapper.
void PerlUpb_DescriptorPool_Free(pTHX_ SV* sv);

// Returns the underlying upb_DefPool from a raw wrapper pointer (IV inside
// hash).
const upb_DefPool* PerlUpb_DescriptorPool_GetPoolRaw(pTHX_ void* ptr);

// Returns the singleton generated pool wrapper.
SV* PerlUpb_DescriptorPool_GeneratedPool(pTHX);

// File iteration
int PerlUpb_DescriptorPool_FileCount(pTHX_ SV* sv);
SV* PerlUpb_DescriptorPool_GetFile(pTHX_ SV* sv, int index);

// Low-level XS helpers
void* PerlUpb_DescriptorPool_CreateRaw(pTHX);
void PerlUpb_DescriptorPool_DestroyRaw(pTHX_ void* ptr);

#endif  // PERL_PROTOBUF_DESCRIPTOR_POOL_POOL_H_
