#ifndef GOOGLE_UPB_UPB_WIRE_WRITER_H__
#define GOOGLE_UPB_UPB_WIRE_WRITER_H__

#include <stdint.h>

// Must be last.
#include "upb/port/def.inc"

UPB_FORCEINLINE uint32_t
UPB_PRIVATE(upb_WireWriter_VarintUnusedSizeFromLeadingZeros64)(uint64_t clz) {
  // Calculate how many bytes of the possible 10 bytes we will *not* encode,
  // because they are part of a zero prefix. For the number 300, it would use 2
  // bytes encoded, so the number of bytes to skip would be 8. Adding 7 to the
  // clz input ensures that we're rounding up.
  return (((uint32_t)clz + 7) * 9) >> 6;
}

#include "upb/port/undef.inc"

#endif  // GOOGLE_UPB_UPB_WIRE_WRITER_H__
