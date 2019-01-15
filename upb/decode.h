/*
** upb_decode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_

#include "upb/msg.h"

#ifdef __cplusplus
extern "C" {
#endif

bool upb_decode(upb_strview buf, upb_msg *msg, const upb_msglayout *l);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODE_H_ */
