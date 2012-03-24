//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// A bridge between upb and proto2, allows populating proto2 generated
// classes using upb's parser, translating between descriptors and defs, etc.
//
// This is designed to be able to be compiled against either the open-source
// version of protocol buffers or the Google-internal proto2.  The two are
// the same in most ways, but live in different namespaces (proto2 vs
// google::protobuf) and have a few other more minor differences.
//
// The bridge gives you a lot of control over which fields will be written to
// the message (fields that are not written will just be skipped), and whether
// unknown fields are written to the UnknownFieldSet.  This can save a lot of
// work if the client only cares about some subset of the fields.
//
// Example usage:
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
//   upb::Finalize(md);
//
//   // Now continue with "JIT the parser" from above.
//
// Note that there is currently no support for
// CodedInputStream::SetExtensionRegistry(), which allows specifying a separate
// DescriptorPool and MessageFactory for extensions.  Since this is a property
// of the input in proto2, it's difficult to build a plan ahead-of-time that
// can properly support this.  If it's an important use case, the caller should
// probably build a upb plan explicitly.

#ifndef UPB_PROTO2_BRIDGE
#define UPB_PROTO2_BRIDGE

#include <vector>

namespace google {
namespace protobuf {
class Descriptor;
class EnumDescriptor;
class FieldDescriptor;
class FileDescriptor;
class Message;
}  // namespace google
}  // namespace protobuf

namespace proto2 {
class Descriptor;
class EnumDescriptor;
class FieldDescriptor;
class FileDescriptor;
class Message;
}  // namespace proto2


namespace upb {

class Def;
class FieldDef;
class MessageDef;

namespace proto2_bridge {

// Unfinalized defs ////////////////////////////////////////////////////////////

// Creating of UNFINALIZED defs.  All of these functions return defs that are
// still mutable and have not been finalized.  They must be finalized before
// using them to parse anything.  This is useful if you want more control over
// the process of constructing defs, eg. to add the specific set of fields you
// care about.

// Creates a new upb::MessageDef that corresponds to the type in the given
// prototype message.  The MessageDef will not have any fields added to it.
upb::MessageDef *NewEmptyMessageDef(const proto2::Message& m, void *owner);
upb::MessageDef *NewEmptyMessageDef(const google::protobuf::Message& desc,
                                    void *owner);

// Adds a new upb::FieldDef to the given MessageDef corresponding to the given
// FieldDescriptor.  The FieldDef will be given an accessor and offset so that
// it can be used to read and write data into the proto2::Message classes.
// The given MessageDef must have been constructed with NewEmptyDefForMessage()
// and f->containing_type() must correspond to the message that was used.
//
// Any submessage, group, or enum fields will be given symbolic references to
// the subtype, which must be resolved before the MessageDef can be finalized.
//
// On success, returns the FieldDef that was added (caller does not own a ref).
// If an existing field had the same name or number, returns NULL.
upb::FieldDef* AddFieldDef(const proto2::FieldDescriptor* f,
                           upb::MessageDef* md);
upb::FieldDef* AddFieldDef(const google::protobuf::FieldDescriptor* f,
                           upb::MessageDef* md);

// Given a MessageDef that was constructed with NewEmptyDefForMessage(), adds
// FieldDefs for all fields defined in the original message, but not for any
// extensions or unknown fields.  The given MessageDef must not have any fields
// that have the same name or number as any of the fields we are adding (the
// easiest way to guarantee this is to start with an empty MessageDef).
//
// Returns true on success or false if any of the fields could not be added.
void AddAllFields(upb::MessageDef* md);

// TODO(haberman): Add:
// // Adds a handler that will store unknown fields in the UnknownFieldSet.
// void AddUnknownFieldHandler(upb::MessageDef* md);

// Returns a new upb::MessageDef that contains handlers for all fields, unknown
// fields, and any extensions in the descriptor's pool.  The resulting
// def/handlers should be equivalent to the generated code constructed by the
// protobuf compiler (or the code in DynamicMessage) for the given type.
// The subdefs for message/enum fields (if any) will be referenced symbolically,
// and will need to be resolved before being finalized.
//
// TODO(haberman): Add missing support (LazyField, MessageSet, and extensions).
//
// TODO(haberman): possibly add a similar function that lets you supply a
// separate DescriptorPool and MessageFactory for extensions, to support
// proto2's io::CodedInputStream::SetExtensionRegistry().
upb::MessageDef* NewFullMessageDef(const proto2::Message& m, void *owner);
upb::MessageDef* NewFullMessageDef(const google::protobuf::Message& m,
                                   void *owner);

// Returns a new upb::EnumDef that corresponds to the given EnumDescriptor.
// Caller owns a ref on the returned EnumDef.
upb::EnumDef* NewEnumDef(const proto2::EnumDescriptor* desc, void *owner);
upb::EnumDef* NewEnumDef(const google::protobuf::EnumDescriptor* desc,
                         void *owner);

// Finalized defs //////////////////////////////////////////////////////////////

// These functions return FINALIZED defs, meaning that they are immutable and
// ready for use.  Since they are immutable you cannot make any further changes
// to eg. the set of fields, but these functions are more convenient if you
// simply want to parse a message exactly how the built-in proto2 parser would.

// Creates a returns a finalized MessageDef for the give message and its entire
// type tree that will include all fields and unknown handlers (ie. it will
// parse just like proto2 would).
const upb::MessageDef* NewFinalMessageDef(const proto2::Message& m,
                                          void *owner);
const upb::MessageDef* NewFinalMessageDef(const google::protobuf::Message& m,
                                          void *owner);

}  // namespace proto2_bridge
}  // namespace upb

#endif
