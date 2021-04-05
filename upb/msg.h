/*
** Public APIs for message operations that do not require descriptors.
** These functions can be used even in build that does not want to depend on
** reflection or descriptors.
**
** Descriptor-based reflection functionality lives in reflection.h.
*/

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stddef.h>

#include "upb/upb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void upb_msg;

/* For users these are opaque. They can be obtained from upb_msgdef_layout()
 * but users cannot access any of the members. */
struct upb_msglayout;
typedef struct upb_msglayout upb_msglayout;

/* Adds unknown data (serialized protobuf data) to the given message.  The data
 * is copied into the message instance. */
void upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                        upb_arena *arena);

/* Returns a reference to the message's unknown data. */
const char *upb_msg_getunknown(const upb_msg *msg, size_t *len);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* UPB_MSG_INT_H_ */
