#ifndef PERL_PROTOBUF_OBJ_CACHE_H_
#define PERL_PROTOBUF_OBJ_CACHE_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)

// Initialize the global object cache
void PerlUpb_ObjCache_Init(pTHX);

// Adds a Perl object to the cache for the given C pointer.
// The reference in the cache is weakened.
void PerlUpb_ObjCache_Add(pTHX_ const void* ptr, SV* obj);

// Retrieves the Perl object associated with the C pointer.
// Returns NULL if not found or if the weak reference has been collected.
SV* PerlUpb_ObjCache_Get(pTHX_ const void* ptr);

// Removes the entry for the given C pointer from the cache.
void PerlUpb_ObjCache_Delete(pTHX_ const void* ptr);

// Removes an entry by its binary key (raw pointer bytes)
void PerlUpb_ObjCache_DeleteEntry(pTHX_ const char* key_bytes, STRLEN len);

// Clears the entire cache (during interpreter shutdown)
void PerlUpb_ObjCache_Clear(pTHX);

// Sets the maximum number of items in the cache.
void PerlUpb_ObjCache_SetCapacity(pTHX_ size_t capacity);

// Returns the current cache capacity.
size_t PerlUpb_ObjCache_GetCapacity(pTHX);

// Audit log event types
#define OBJ_CACHE_EVENT_ADD 1
#define OBJ_CACHE_EVENT_HIT 2
#define OBJ_CACHE_EVENT_MISS 3
#define OBJ_CACHE_EVENT_DELETE 4
#define OBJ_CACHE_EVENT_EVICT 5

// Allocation event types
#define ALLOC_EVENT_MALLOC 10
#define ALLOC_EVENT_FREE 11
#define ALLOC_EVENT_REALLOC 12

void PerlUpb_ObjCache_LogEvent(int type, const void* ptr);

// Returns the audit log as a Perl array reference.
SV* PerlUpb_ObjCache_GetAuditLog(pTHX);

// Returns lock contention statistics as a Perl hash reference.
SV* PerlUpb_ObjCache_GetContentionStats(pTHX);

#endif  // PERL_PROTOBUF_OBJ_CACHE_H_
