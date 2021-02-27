
#ifndef UPB_TEXTENCODE_H_
#define UPB_TEXTENCODE_H_

#include "upb/def.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* When set, prints everything on a single line. */
  UPB_TXTENC_SINGLELINE = 1,

  /* When set, unknown fields are not printed. */
  UPB_TXTENC_SKIPUNKNOWN = 2,

  /* When set, maps are *not* sorted (this avoids allocating tmp mem). */
  UPB_TXTENC_NOSORT = 4
};

/* Encodes the given |msg| to text format.  The message's reflection is given in
 * |m|.  The symtab in |symtab| is used to find extensions (if NULL, extensions
 * will not be printed).
 *
 * Output is placed in the given buffer, and always NULL-terminated.  The output
 * size (excluding NULL) is returned.  This means that a return value >= |size|
 * implies that the output was truncated.  (These are the same semantics as
 * snprintf()). */
size_t upb_text_encode(const upb_msg *msg, const upb_msgdef *m,
                       const upb_symtab *ext_pool, int options, char *buf,
                       size_t size);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TEXTENCODE_H_ */
