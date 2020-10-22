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
  const char *end;         /* Can read up to 16 bytes slop beyond this. */
  const char *limit_ptr;   /* = end + UPB_MIN(limit, 0) */
  int limit;               /* Submessage limit relative to end. */
  int depth;
  uint32_t end_group; /* Set to field number of END_GROUP tag, if any. */
  bool alias;
  char patch[32];
  upb_arena arena;
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

const char *decode_isdonefallback(upb_decstate *d, const char *ptr,
                                  int overrun);

UPB_INLINE
const char *decode_isdonefallback_inl(upb_decstate *d, const char *ptr,
                                      int overrun) {
  if (overrun < d->limit) {
    /* Need to copy remaining data into patch buffer. */
    UPB_ASSERT(overrun < 16);
    memset(d->patch + 16, 0, 16);
    memcpy(d->patch, d->end, 16);
    ptr = &d->patch[0] + overrun;
    d->end = &d->patch[16];
    d->limit -= 16;
    d->limit_ptr = d->end + d->limit;
    d->alias = false;
    UPB_ASSERT(ptr < d->limit_ptr);
    return ptr;
  } else {
    return NULL;
  }
}

UPB_INLINE
bool decode_isdone(upb_decstate *d, const char **ptr) {
  int overrun = *ptr - d->end;
  if (UPB_LIKELY(*ptr < d->limit_ptr)) {
    return false;
  } else if (UPB_LIKELY(overrun == d->limit)) {
    return true;
  } else {
    *ptr = decode_isdonefallback(d, *ptr, overrun);
    return false;
  }
}

UPB_INLINE
int decode_pushlimit(upb_decstate *d, const char *ptr, int size) {
  int limit = size + (int)(ptr - d->end);
  int delta = d->limit - limit;
  d->limit = limit;
  d->limit_ptr = d->end + UPB_MIN(0, limit);
  return delta;
}

UPB_INLINE
void decode_poplimit(upb_decstate *d, int saved_delta) {
  d->limit += saved_delta;
  d->limit_ptr = d->end + UPB_MIN(0, d->limit);
}

#include "upb/port_undef.inc"

#endif  /* UPB_DECODE_INT_H_ */
