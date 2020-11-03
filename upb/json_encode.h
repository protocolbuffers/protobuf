
#ifndef UPB_JSONENCODE_H_
#define UPB_JSONENCODE_H_

#include "upb/def.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* When set, emits 0/default values.  TODO(haberman): proto3 only? */
  UPB_JSONENC_EMITDEFAULTS = 1,

  /* When set, use normal (snake_caes) field names instead of JSON (camelCase)
     names. */
  UPB_JSONENC_PROTONAMES = 2
};

/* Encodes the given |msg| to JSON format.  The message's reflection is given in
 * |m|.  The symtab in |symtab| is used to find extensions (if NULL, extensions
 * will not be printed).
 *
 * Output is placed in the given buffer, and always NULL-terminated.  The output
 * size (excluding NULL) is returned.  This means that a return value >= |size|
 * implies that the output was truncated.  (These are the same semantics as
 * snprintf()). */
size_t upb_json_encode(const upb_msg *msg, const upb_msgdef *m,
                       const upb_symtab *ext_pool, int options, char *buf,
                       size_t size, upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_JSONENCODE_H_ */
