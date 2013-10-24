/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb::descriptor::Reader provides a way of building upb::Defs from
 * data in descriptor.proto format.
 */

#ifndef UPB_DESCRIPTOR_H
#define UPB_DESCRIPTOR_H

#include "upb/handlers.h"

// The maximum number of nested declarations that are allowed, ie.
// message Foo {
//   message Bar {
//     message Baz {
//     }
//   }
// }
//
// This is a resource limit that affects how big our runtime stack can grow.
// TODO: make this a runtime-settable property of the Reader instance.
#define UPB_MAX_MESSAGE_NESTING 64

#ifdef __cplusplus
namespace upb {
namespace descriptor {

// Frame type that accumulates defs as they are being built from a descriptor
// according to the descriptor.proto schema.
class Reader;

// Gets the array of defs that have been parsed and removes them from the
// descreader.  Ownership of the defs is passed to the caller using the given
// owner), but the ownership of the returned array is retained and is
// invalidated by any other call into the descreader.  The defs will not have
// been resolved, and are ready to be added to a symtab.
inline upb::Def** GetDefs(Reader* r, void* owner, int* n);

// Gets the handlers for reading a FileDescriptorSet, which builds defs and
// accumulates them in a Reader object (which the handlers use as their
// FrameType).
inline const upb::Handlers* GetReaderHandlers(const void* owner);

}  // namespace descriptor
}  // namespace upb

typedef upb::descriptor::Reader upb_descreader;

extern "C" {
#else
struct upb_descreader;
typedef struct upb_descreader upb_descreader;
#endif

// C API.
const upb_frametype *upb_descreader_getframetype();
upb_def **upb_descreader_getdefs(upb_descreader *r, void *owner, int *n);
const upb_handlers *upb_descreader_gethandlers(const void *owner);


// C++ implementation details. /////////////////////////////////////////////////

#ifdef __cplusplus
}  // extern "C"

namespace upb {

namespace descriptor {
inline upb::Def** GetDefs(Reader* r, void* owner, int* n) {
  return upb_descreader_getdefs(r, owner, n);
}
inline const upb::Handlers* GetReaderHandlers(const void* owner) {
  return upb_descreader_gethandlers(owner);
}
}  // namespace descriptor
}  // namespace upb
#endif

#endif  // UPB_DESCRIPTOR_H
