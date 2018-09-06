/*
** upb_encode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_ENCODE_H_
#define UPB_ENCODE_H_

#include "upb/msg.h"

UPB_BEGIN_EXTERN_C

char *upb_encode(const void *msg, const upb_msglayout *l, upb_arena *arena,
                 size_t *size);

UPB_END_EXTERN_C

#endif  /* UPB_ENCODE_H_ */
