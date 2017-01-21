/*
** upb's core components like upb_decoder and upb_msg are carefully designed to
** avoid depending on each other for maximum orthogonality.  In other words,
** you can use a upb_decoder to decode into *any* kind of structure; upb_msg is
** just one such structure.  A upb_msg can be serialized/deserialized into any
** format, protobuf binary format is just one such format.
**
** However, for convenience we provide functions here for doing common
** operations like deserializing protobuf binary format into a upb_msg.  The
** compromise is that this file drags in almost all of upb as a dependency,
** which could be undesirable if you're trying to use a trimmed-down build of
** upb.
**
** While these routines are convenient, they do not reuse any encoding/decoding
** state.  For example, if a decoder is JIT-based, it will be re-JITted every
** time these functions are called.  For this reason, if you are parsing lots
** of data and efficiency is an issue, these may not be the best functions to
** use (though they are useful for prototyping, before optimizing).
*/

#ifndef UPB_GLUE_H
#define UPB_GLUE_H

#include <stdbool.h>
#include "upb/def.h"

#ifdef __cplusplus
#include <vector>

extern "C" {
#endif

/* Loads a binary descriptor and returns a NULL-terminated array of unfrozen
 * filedefs.  The caller owns the returned array, which must be freed with
 * upb_gfree(). */
upb_filedef **upb_loaddescriptor(const char *buf, size_t n, const void *owner,
                                 upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {

inline bool LoadDescriptor(const char* buf, size_t n, Status* status,
                           std::vector<reffed_ptr<FileDef> >* files) {
  FileDef** parsed_files = upb_loaddescriptor(buf, n, &parsed_files, status);

  if (parsed_files) {
    FileDef** p = parsed_files;
    while (*p) {
      files->push_back(reffed_ptr<FileDef>(*p, &parsed_files));
      ++p;
    }
    free(parsed_files);
    return true;
  } else {
    return false;
  }
}

/* Templated so it can accept both string and std::string. */
template <typename T>
bool LoadDescriptor(const T& desc, Status* status,
                    std::vector<reffed_ptr<FileDef> >* files) {
  return LoadDescriptor(desc.c_str(), desc.size(), status, files);
}

}  /* namespace upb */

#endif

#endif  /* UPB_GLUE_H */
