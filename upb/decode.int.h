/*
** Internal implementation details of the decoder that are shared between
** decode.c and decode_fast.c.
*/

#ifndef UPB_DECODE_INT_H_
#define UPB_DECODE_INT_H_

#include <setjmp.h>

#include "upb/msg.h"
#include "upb/upb.int.h"

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

const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                intptr_t table, uint64_t hasbits);
const char *fastdecode_err(upb_decstate *d);

/* x86-64 pointers always have the high 16 bits matching. So we can shift
 * left 8 and right 8 without loss of information. */
UPB_INLINE intptr_t decode_totable(const upb_msglayout *tablep) {
  return ((intptr_t)tablep << 8) | tablep->table_mask;
}

UPB_INLINE const upb_msglayout *decode_totablep(intptr_t table) {
  return (void*)(table >> 8);
}

#include "upb/port_undef.inc"

#endif  /* UPB_DECODE_INT_H_ */
