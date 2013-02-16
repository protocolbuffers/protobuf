//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// Support for registering field handlers that can write into a legacy proto1
// message.  This functionality is only needed inside Google.
//
// This is a low-level interface; the high-level interface in google.h is
// more user-friendly.

#ifndef UPB_GOOGLE_PROTO1_H_
#define UPB_GOOGLE_PROTO1_H_

namespace proto2 {
class FieldDescriptor;
class Message;
}

namespace upb {
class FieldDef;
class Handlers;
}

namespace upb {
namespace google {

// Sets field handlers in the given Handlers object for writing to a single
// field (as described by "proto2_f" and "upb_f") into a message constructed
// by the same factory as "prototype."  Returns true if this was successful
// (this will fail if "prototype" is not a proto1 message, or if we can't
// handle it for some reason).
bool TrySetProto1WriteHandlers(const proto2::FieldDescriptor* proto2_f,
                               const proto2::Message& prototype,
                               const upb::FieldDef* upb_f, upb::Handlers* h);

// Returns a prototype for the given field in "m", if it is weak.  The returned
// message could be the linked-in message type or OpaqueMessage, if the weak
// message is *not* linked in.  Otherwise returns NULL.
const proto2::Message* GetProto1WeakPrototype(
    const proto2::Message& m,
    const proto2::FieldDescriptor* f);

// Returns a prototype for the given non-weak field in "m".
const proto2::Message* GetProto1FieldPrototype(
    const proto2::Message& m,
    const proto2::FieldDescriptor* f);

}  // namespace google
}  // namespace upb

#endif  // UPB_GOOGLE_PROTO1_H_
