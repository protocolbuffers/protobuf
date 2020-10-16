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
                                const upb_msglayout *table, uint64_t hasbits);

/* Error function that will abort decoding with longjmp(). We can't declare this
 * UPB_NORETURN, even though it is appropriate, because if we do then compilers
 * will "helpfully" refuse to tailcall to it
 * (see: https://stackoverflow.com/a/55657013), which will defeat a major goal
 * of our optimizations. That is also why we must declare it in a separate file,
 * otherwise the compiler will see that it calls longjmp() and deduce that it is
 * noreturn. */
const char *fastdecode_err(upb_decstate *d);

#include "upb/port_undef.inc"

#endif  /* UPB_DECODE_INT_H_ */
