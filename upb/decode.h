/*
** upb_decode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_

#include <setjmp.h>

#include "upb/msg.h"

#ifdef __cplusplus
extern "C" {
#endif

bool upb_decode(const char *buf, size_t size, upb_msg *msg,
                const upb_msglayout *l, upb_arena *arena);

struct upb_fasttable;
struct upb_decstate;

typedef const char *_upb_field_parser(struct upb_decstate *d, const char *ptr,
                                      upb_msg *msg, struct upb_fasttable *table,
                                      uint64_t hasbits, uint64_t data);

typedef struct upb_fasttable {
  _upb_field_parser *field_parser[16];
  uint64_t field_data[16];
  _upb_field_parser *fallback;
} upb_fasttable;

/* Internal only: data pertaining to the parse. */
typedef struct upb_decstate {
  char *arena_ptr, *arena_end;
  const void *rep_end;
  const char *limit;       /* End of delimited region or end of buffer. */
  const char *fastlimit;   /* End of delimited region or end of buffer. */
  upb_array *arr;
  _upb_field_parser *resume;
  upb_arena *arena;
  int depth;
  uint32_t end_group; /* Set to field number of END_GROUP tag, if any. */
  jmp_buf err;
} upb_decstate;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODE_H_ */
