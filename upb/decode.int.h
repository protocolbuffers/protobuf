/*
** Internal implementation details of the decoder that are shared between
** decode.c and decode_fast.c.
*/

#ifndef UPB_DECODE_INT_H_
#define UPB_DECODE_INT_H_

#include <setjmp.h>

#include "upb/msg.h"
#include "upb/upb.int.h"
#include "upb/decode_fast.h"

/* Must be last. */
#include "upb/port_def.inc"

typedef struct upb_decstate {
  const char *limit;       /* End of delimited region or end of buffer. */
  const char *fastend;     /* The end of the entire buffer - 16 */
  const char *fastlimit;   /* UPB_MIN(limit, fastend) */
  upb_arena arena;
  int depth;
  uint32_t end_group; /* Set to field number of END_GROUP tag, if any. */
  jmp_buf err;
} upb_decstate;

/* Error function that will abort decoding with longjmp(). We can't declare this
 * UPB_NORETURN, even though it is appropriate, because if we do then compilers
 * will "helpfully" refuse to tailcall to it
 * (see: https://stackoverflow.com/a/55657013), which will defeat a major goal
 * of our optimizations. That is also why we must declare it in a separate file,
 * otherwise the compiler will see that it calls longjmp() and deduce that it is
 * noreturn. */
const char *fastdecode_err(upb_decstate *d);

/* x86-64 pointers always have the high 16 bits matching. So we can shift
 * left 8 and right 8 without loss of information. */
UPB_INLINE intptr_t decode_totable(const upb_msglayout *tablep) {
  return ((intptr_t)tablep << 8) | tablep->table_mask;
}

UPB_INLINE const upb_msglayout *decode_totablep(intptr_t table) {
  return (void*)(table >> 8);
}

UPB_FORCEINLINE static
const char *fastdecode_tagdispatch(upb_decstate *d, const char *ptr,
                                    upb_msg *msg, intptr_t table,
                                    uint64_t hasbits, uint32_t tag) {
  const upb_msglayout *table_p = decode_totablep(table);
  uint8_t mask = table;
  uint64_t data;
  size_t idx = tag & mask;
  __builtin_assume((idx & 7) == 0);
  idx >>= 3;
  data = table_p->fasttable[idx].field_data ^ tag;
  return table_p->fasttable[idx].field_parser(d, ptr, msg, table, hasbits, data);
}

UPB_FORCEINLINE static
uint32_t fastdecode_loadtag(const char* ptr) {
  uint16_t tag;
  memcpy(&tag, ptr, 2);
  return tag;
}

UPB_FORCEINLINE static
const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                intptr_t table, uint64_t hasbits) {
  if (UPB_UNLIKELY(ptr >= d->fastlimit)) {
    if (UPB_LIKELY(ptr == d->limit)) {
      *(uint32_t*)msg |= hasbits >> 16;  /* Sync hasbits. */
      return ptr;
    }
    uint64_t data = 0;
    return fastdecode_generic(d, ptr, msg, table, hasbits, data);
  }
  return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, fastdecode_loadtag(ptr));
}

#include "upb/port_undef.inc"

#endif  /* UPB_DECODE_INT_H_ */
