//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// This file contains functionality for constructing upb Defs and Handlers
// corresponding to proto2 messages.  Using this functionality, you can use upb
// to dynamically generate parsing code that can behave exactly like proto2's
// generated parsing code.  Alternatively, you can configure things to
// read/write only a subset of the fields for higher performance when only some
// fields are needed.
//
// Example usage (FIX XXX):
//
//   // Build a def that will have all fields and parse just like proto2 would.
//   const upb::MessageDef* md = upb::proto2_bridge::NewMessageDef(&MyProto());
//
//   // JIT the parser; should only be done once ahead-of-time.
//   upb::Handlers* handlers = upb::NewHandlersForMessage(md);
//   upb::DecoderPlan* plan = upb::DecoderPlan::New(handlers);
//   handlers->Unref();
//
//   // The actual parsing.
//   MyProto proto;
//   upb::Decoder decoder;
//   upb::StringSource source(buf, len);
//   decoder.ResetPlan(plan, 0);
//   decoder.ResetInput(source.AllBytes(), &proto);
//   CHECK(decoder.Decode() == UPB_OK) << decoder.status();
//
// To parse only one field and skip all others:
//
//   const upb::MessageDef* md =
//       upb::proto2_bridge::NewEmptyMessageDef(MyProto().GetPrototype());
//   upb::proto2_bridge::AddFieldDef(
//       MyProto::descriptor()->FindFieldByName("my_field"), md);
//   upb::Freeze(md);
//
//   // Now continue with "JIT the parser" from above.
//
// Note that there is currently no support for
// CodedInputStream::SetExtensionRegistry(), which allows specifying a separate
// DescriptorPool and MessageFactory for extensions.  Since this is a property
// of the input in proto2, it's difficult to build a plan ahead-of-time that
// can properly support this.  If it's an important use case, the caller should
// probably build a upb plan explicitly.

#ifndef UPB_GOOGLE_BRIDGE_H_
#define UPB_GOOGLE_BRIDGE_H_

namespace google {
namespace protobuf { class Message; }
}  // namespace google

namespace proto2 { class Message; }

namespace upb {

class Handlers;

namespace google {

// Returns a upb::Handlers object that can be used to populate a proto2::Message
// object of the same type as "m."
//
// TODO(haberman): Add handler caching functionality so that we don't use
// O(n^2) memory in the worst case when incrementally building handlers.
const upb::Handlers* NewWriteHandlers(const proto2::Message& m, void *owner);
const upb::Handlers* NewWriteHandlers(const ::google::protobuf::Message& m,
                                      void *owner);

}  // namespace google
}  // namespace upb

#endif  // UPB_GOOGLE_BRIDGE_H_
