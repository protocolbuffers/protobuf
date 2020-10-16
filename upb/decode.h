/*
** upb_decode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_

#include "upb/msg.h"

#ifdef __cplusplus
extern "C" {
#endif

bool upb_decode(const char *buf, size_t size, upb_msg *msg,
                const upb_msglayout *l, upb_arena *arena);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODE_H_ */
