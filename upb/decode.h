/*
** upb_decode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_

#include "upb/msg.h"

/* Must be last. */
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* If set, strings will alias the input buffer instead of copying into the
   * arena. */
  UPB_DECODE_ALIAS = 1,
};

#define UPB_DECODE_MAXDEPTH(depth) ((depth) << 16)

bool _upb_decode(const char *buf, size_t size, upb_msg *msg,
                 const upb_msglayout *l, upb_arena *arena, int options);

UPB_INLINE
bool upb_decode(const char *buf, size_t size, upb_msg *msg,
                const upb_msglayout *l, upb_arena *arena) {
  return _upb_decode(buf, size, msg, l, arena, 0);
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  /* UPB_DECODE_H_ */
