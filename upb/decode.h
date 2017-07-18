/*
** upb_decode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_

#include "upb/msg.h"

UPB_BEGIN_EXTERN_C

bool upb_decode(upb_stringview buf, void *msg,
                const upb_msglayout_msginit_v1 *l, upb_env *env);

UPB_END_EXTERN_C

#endif  /* UPB_DECODE_H_ */
