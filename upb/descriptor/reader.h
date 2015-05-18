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

#include "upb/env.h"
#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace descriptor {
class Reader;
}  /* namespace descriptor */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::descriptor::Reader, upb_descreader)

#ifdef __cplusplus

/* Class that receives descriptor data according to the descriptor.proto schema
 * and use it to build upb::Defs corresponding to that schema. */
class upb::descriptor::Reader {
 public:
  /* These handlers must have come from NewHandlers() and must outlive the
   * Reader.
   *
   * TODO: generate the handlers statically (like we do with the
   * descriptor.proto defs) so that there is no need to pass this parameter (or
   * to build/memory-manage the handlers at runtime at all).  Unfortunately this
   * is a bit tricky to implement for Handlers, but necessary to simplify this
   * interface. */
  static Reader* Create(Environment* env, const Handlers* handlers);

  /* The reader's input; this is where descriptor.proto data should be sent. */
  Sink* input();

  /* Returns an array of all defs that have been parsed, and transfers ownership
   * of them to "owner".  The number of defs is stored in *n.  Ownership of the
   * returned array is retained and is invalidated by any other call into
   * Reader.
   *
   * These defs are not frozen or resolved; they are ready to be added to a
   * symtab. */
  upb::Def** GetDefs(void* owner, int* n);

  /* Builds and returns handlers for the reader, owned by "owner." */
  static Handlers* NewHandlers(const void* owner);

 private:
  UPB_DISALLOW_POD_OPS(Reader, upb::descriptor::Reader)
};

#endif

UPB_BEGIN_EXTERN_C

/* C API. */
upb_descreader *upb_descreader_create(upb_env *e, const upb_handlers *h);
upb_sink *upb_descreader_input(upb_descreader *r);
upb_def **upb_descreader_getdefs(upb_descreader *r, void *owner, int *n);
const upb_handlers *upb_descreader_newhandlers(const void *owner);

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ implementation details. ************************************************/
namespace upb {
namespace descriptor {
inline Reader* Reader::Create(Environment* e, const Handlers *h) {
  return upb_descreader_create(e, h);
}
inline Sink* Reader::input() { return upb_descreader_input(this); }
inline upb::Def** Reader::GetDefs(void* owner, int* n) {
  return upb_descreader_getdefs(this, owner, n);
}
}  /* namespace descriptor */
}  /* namespace upb */
#endif

#endif  /* UPB_DESCRIPTOR_H */
