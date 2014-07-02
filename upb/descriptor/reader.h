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

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace descriptor {
class Reader;
}  // namespace descriptor
}  // namespace upb
#endif

UPB_DECLARE_TYPE(upb::descriptor::Reader, upb_descreader);

// Internal-only structs used by Reader.

// upb_deflist is an internal-only dynamic array for storing a growing list of
// upb_defs.
typedef struct {
 UPB_PRIVATE_FOR_CPP
  upb_def **defs;
  size_t len;
  size_t size;
  bool owned;
} upb_deflist;

// We keep a stack of all the messages scopes we are currently in, as well as
// the top-level file scope.  This is necessary to correctly qualify the
// definitions that are contained inside.  "name" tracks the name of the
// message or package (a bare name -- not qualified by any enclosing scopes).
typedef struct {
 UPB_PRIVATE_FOR_CPP
  char *name;
  // Index of the first def that is under this scope.  For msgdefs, the
  // msgdef itself is at start-1.
  int start;
} upb_descreader_frame;

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

// Class that receives descriptor data according to the descriptor.proto schema
// and use it to build upb::Defs corresponding to that schema.
UPB_DEFINE_CLASS0(upb::descriptor::Reader,
 public:
  // These handlers must have come from NewHandlers() and must outlive the
  // Reader.
  //
  // TODO: generate the handlers statically (like we do with the
  // descriptor.proto defs) so that there is no need to pass this parameter (or
  // to build/memory-manage the handlers at runtime at all).  Unfortunately this
  // is a bit tricky to implement for Handlers, but necessary to simplify this
  // interface.
  Reader(const Handlers* handlers, Status* status);
  ~Reader();

  // Resets the reader's state and discards any defs it may have built.
  void Reset();

  // The reader's input; this is where descriptor.proto data should be sent.
  Sink* input();

  // Returns an array of all defs that have been parsed, and transfers ownership
  // of them to "owner".  The number of defs is stored in *n.  Ownership of the
  // returned array is retained and is invalidated by any other call into
  // Reader.
  //
  // These defs are not frozen or resolved; they are ready to be added to a
  // symtab.
  upb::Def** GetDefs(void* owner, int* n);

  // Builds and returns handlers for the reader, owned by "owner."
  static Handlers* NewHandlers(const void* owner);
,
UPB_DEFINE_STRUCT0(upb_descreader,
  upb_sink sink;
  upb_deflist defs;
  upb_descreader_frame stack[UPB_MAX_MESSAGE_NESTING];
  int stack_len;

  uint32_t number;
  char *name;
  bool saw_number;
  bool saw_name;

  char *default_string;

  upb_fielddef *f;
));

UPB_BEGIN_EXTERN_C  // {

// C API.
void upb_descreader_init(upb_descreader *r, const upb_handlers *handlers,
                         upb_status *status);
void upb_descreader_uninit(upb_descreader *r);
void upb_descreader_reset(upb_descreader *r);
upb_sink *upb_descreader_input(upb_descreader *r);
upb_def **upb_descreader_getdefs(upb_descreader *r, void *owner, int *n);
const upb_handlers *upb_descreader_newhandlers(const void *owner);

UPB_END_EXTERN_C  // }

#ifdef __cplusplus
// C++ implementation details. /////////////////////////////////////////////////
namespace upb {
namespace descriptor {
inline Reader::Reader(const Handlers *h, Status *s) {
  upb_descreader_init(this, h, s);
}
inline Reader::~Reader() { upb_descreader_uninit(this); }
inline void Reader::Reset() { upb_descreader_reset(this); }
inline Sink* Reader::input() { return upb_descreader_input(this); }
inline upb::Def** Reader::GetDefs(void* owner, int* n) {
  return upb_descreader_getdefs(this, owner, n);
}
}  // namespace descriptor
}  // namespace upb
#endif

#endif  // UPB_DESCRIPTOR_H
