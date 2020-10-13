
#ifndef UPB_DECODE_INT_H_
#define UPB_DECODE_INT_H_

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

UPB_INLINE
upb_msg *decode_newmsg_ceil(upb_decstate *d, size_t size, int msg_ceil_bytes) {
  char *msg_data;
  if (UPB_LIKELY(msg_ceil_bytes > 0 && _upb_arenahas(&d->arena, msg_ceil_bytes))) {
    UPB_ASSERT(size <= (size_t)msg_ceil_bytes);
    msg_data = d->arena.head.ptr;
    d->arena.head.ptr += size;
    UPB_UNPOISON_MEMORY_REGION(msg_data, msg_ceil_bytes);
    memset(msg_data, 0, msg_ceil_bytes);
    UPB_POISON_MEMORY_REGION(msg_data + size, msg_ceil_bytes - size);
  } else {
    msg_data = (char*)upb_arena_malloc(&d->arena, size);
    memset(msg_data, 0, size);
  }
  return msg_data + sizeof(upb_msg_internal);
}

#include "upb/port_undef.inc"

#endif  /* UPB_DECODE_INT_H_ */
