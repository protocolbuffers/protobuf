
#ifndef UPB_JSONDECODE_H_
#define UPB_JSONDECODE_H_

#include "upb/def.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  UPB_JSONDEC_IGNOREUNKNOWN = 1
};

bool upb_json_decode(const char *buf, size_t size, upb_msg *msg,
                     const upb_msgdef *m, const upb_symtab *any_pool,
                     int options, upb_arena *arena, upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_JSONDECODE_H_ */
