/*
** upb_decode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_

#include <setjmp.h>

#include "upb/msg.h"

#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

bool upb_decode(const char *buf, size_t size, upb_msg *msg,
                const upb_msglayout *l, upb_arena *arena);

/* Internal only: data pertaining to the parse. */
typedef struct upb_decstate {
  const char *limit;       /* End of delimited region or end of buffer. */
  const char *fastlimit;   /* End of delimited region or end of buffer. */
  const char *fastend;
  char *arena_ptr;
  char *arena_end;
  upb_array *arr;
  upb_arena *arena;
  int depth;
  uint32_t end_group; /* Set to field number of END_GROUP tag, if any. */
  jmp_buf err;
} upb_decstate;

const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *table, uint64_t hasbits);
const char *fastdecode_generic(upb_decstate *d, const char *ptr, upb_msg *msg,
                               const upb_msglayout *table, uint64_t hasbits,
                               uint64_t data);
const char *fastdecode_err(upb_decstate *d);

void *decode_mallocfallback(upb_decstate *d, size_t size);

UPB_FORCEINLINE
static void *decode_malloc(upb_decstate *d, size_t size) {
  UPB_ASSERT((size & 7) == 0);
  char *ptr = d->arena_ptr;
  if (UPB_UNLIKELY((size_t)(d->arena_end - d->arena_ptr) < size)) {
    return decode_mallocfallback(d, size);
  }
  d->arena_ptr += size;
  return ptr;
}

UPB_INLINE
upb_msg *decode_newmsg(upb_decstate *d, const upb_msglayout *l) {
  size_t size = l->size + sizeof(upb_msg_internal);
  char *msg_data = (char*)decode_malloc(d, size);
  memset(msg_data, 0, size);
  return msg_data + sizeof(upb_msg_internal);
}


#define UPB_PARSE_PARAMS                                                      \
  upb_decstate *d, const char *ptr, upb_msg *msg, const upb_msglayout *table, \
      uint64_t hasbits, uint64_t data

#define F(card, type, valbytes, tagbytes) \
  const char *upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS);

#define TYPES(card, tagbytes) \
  F(card, b, 1, tagbytes)     \
  F(card, v, 4, tagbytes)     \
  F(card, v, 8, tagbytes)     \
  F(card, z, 4, tagbytes)     \
  F(card, z, 8, tagbytes)

#define TAGBYTES(card) \
  TYPES(card, 1)       \
  TYPES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
//TAGBYTES(r)

const char *upb_pss_1bt(UPB_PARSE_PARAMS);
const char *upb_pss_2bt(UPB_PARSE_PARAMS);
const char *upb_pos_1bt(UPB_PARSE_PARAMS);
const char *upb_pos_2bt(UPB_PARSE_PARAMS);
const char *upb_psm_1bt(UPB_PARSE_PARAMS);
const char *upb_pom_1bt(UPB_PARSE_PARAMS);
const char *upb_prm_1bt(UPB_PARSE_PARAMS);
const char *upb_psm_2bt(UPB_PARSE_PARAMS);
const char *upb_pom_2bt(UPB_PARSE_PARAMS);
const char *upb_prm_2bt(UPB_PARSE_PARAMS);

#undef F
#undef TYPES
#undef TAGBYTES
#undef UPB_PARSE_PARAMS

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  /* UPB_DECODE_H_ */
