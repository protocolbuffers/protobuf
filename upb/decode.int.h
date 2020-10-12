
#ifndef UPB_DECODE_INT_H_
#define UPB_DECODE_INT_H_

#include "upb/upb.int.h"

/* Must be last. */
#include "upb/port_def.inc"

typedef struct upb_decstate {
  const char *limit;       /* End of delimited region or end of buffer. */
  const char *fastlimit;   /* End of delimited region or end of buffer. */
  const char *fastend;
  upb_arena arena;
  int depth;
  uint32_t end_group; /* Set to field number of END_GROUP tag, if any. */
  jmp_buf err;
} upb_decstate;

const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *table, uint64_t hasbits);
const char *fastdecode_err(upb_decstate *d);


UPB_INLINE bool decode_arenahas(upb_decstate *d, size_t bytes) {
  return (size_t)(d->arena.head.end - d->arena.head.ptr) >= bytes;
}

UPB_INLINE
upb_msg *decode_newmsg_ceil(upb_decstate *d, const upb_msglayout *l,
                            int msg_ceil_bytes) {
  size_t size = l->size + sizeof(upb_msg_internal);
  char *msg_data;
  if (UPB_LIKELY(msg_ceil_bytes > 0 && decode_arenahas(d, msg_ceil_bytes))) {
    UPB_ASSERT(size <= (size_t)msg_ceil_bytes);
    msg_data = d->arena.head.ptr;
    memset(msg_data, 0, msg_ceil_bytes);
    d->arena.head.ptr += size;
  } else {
    msg_data = (char*)upb_arena_malloc(&d->arena, size);
    memset(msg_data, 0, size);
  }
  return msg_data + sizeof(upb_msg_internal);
}

#include "upb/port_undef.inc"

#endif  /* UPB_DECODE_INT_H_ */
