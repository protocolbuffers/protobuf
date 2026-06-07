#ifndef PERL_PROTOBUF_REGISTRY_H_
#define PERL_PROTOBUF_REGISTRY_H_

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/mem/arena.h"
#include "xs/protobuf/arena.h"

// Forward declaration of internal audit log type
typedef struct obj_cache_audit_log_s obj_cache_audit_log_t;

typedef struct {
  double fail_probability;   // 0.0 to 1.0
  double delay_probability;  // 0.0 to 1.0
  uint32_t max_delay_ms;
  unsigned int seed;
  bool enabled;
} PerlUpb_ChaosConfig;

typedef struct {
  HV* obj_cache;
  AV* obj_lru;
  HV* descriptor_fingerprints;  // Fingerprint (uint64) -> MessageDef (cached
                                // wrapper)
  HV* stash_cache;              // MessageDef* (IV) -> Stash (HV*)
  HV* stash_message;
  HV* stash_repeated;
  HV* stash_map;
  HV* stash_arena;
  HV* stash_unknown_fields;
  HV* stash_repeated_public;
  HV* stash_map_public;
  obj_cache_audit_log_t* audit_log;
  size_t max_cache_capacity;
  upb_Arena* cached_transient_arena;
  PerlUpb_StatsAlloc stats_alloc;
  PerlUpb_ChaosConfig chaos;
  void (*idle_time_arena_hook)(pTHX);
} PerlUpb_Registry;

// Registry Management
void PerlUpb_Registry_Init(pTHX);
PerlUpb_Registry* PerlUpb_Registry_Get(pTHX);

// Arena Pre-allocation
void PerlUpb_Registry_PreallocateArena(pTHX);

#endif  // PERL_PROTOBUF_REGISTRY_H_
