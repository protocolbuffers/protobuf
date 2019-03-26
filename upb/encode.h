/*
** upb_encode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_ENCODE_H_
#define UPB_ENCODE_H_

#include "upb/msg.h"

#ifdef __cplusplus
extern "C" {
#endif

char *upb_encode(const void *msg, const upb_msglayout *l, upb_arena *arena,
                 size_t *size);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_ENCODE_H_ */
