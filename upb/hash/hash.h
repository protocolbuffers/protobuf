#ifndef THIRD_PARTY_UPB_UPB_HASH_HASH_H_
#define THIRD_PARTY_UPB_UPB_HASH_HASH_H_

#include <stdint.h>
#include <string.h>

#include "upb/hash/internal/hash.h"

// Must be last.
#include "upb/port/def.inc"

UPB_INLINE uint32_t _upb_Hash(const void* p, size_t n, uint64_t seed) {
  return _upb_Wyhash(p, n, seed, kWyhashSalt);
}

#include "upb/port/undef.inc"

#endif  // THIRD_PARTY_UPB_UPB_HASH_HASH_H_
