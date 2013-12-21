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
// Example usage:
//
//   // JIT the parser; should only be done once ahead-of-time.
//   upb::reffed_ptr<const upb::Handlers> write_myproto(
//       upb::google::NewWriteHandlers(MyProto()));
//   upb::reffed_ptr<const upb::Handlers> parse_myproto(
//       upb::Decoder::NewDecoderHandlers(write_myproto.get(), true));
//
//   // The actual parsing.
//   MyProto proto;
//   upb::SeededPipeline<8192> pipeline(upb_realloc, NULL);
//   upb::Sink* write_sink = pipeline.NewSink(write_myproto.get());
//   upb::Sink* parse_sink = pipeline.NewSink(parse_myproto.get());
//   upb::pb::Decoder* decoder = decoder_sink->GetObject<upb::pb::Decoder>();
//   upb::pb::ResetDecoderSink(decoder, write_sink);
//   write_sink->Reset(&proto);
//
// Note that there is currently no support for
// CodedInputStream::SetExtensionRegistry(), which allows specifying a separate
// DescriptorPool and MessageFactory for extensions.  Since this is a property
// of the input in proto2, it's difficult to build a plan ahead-of-time that
// can properly support this.  If it's an important use case, the caller should
// probably build a upb plan explicitly.

#ifndef UPB_GOOGLE_BRIDGE_H_
#define UPB_GOOGLE_BRIDGE_H_

#include <map>
#include <vector>
#include "upb/upb.h"

namespace google {
namespace protobuf {
class FieldDescriptor;
class Descriptor;
class EnumDescriptor;
class Message;
}  // namespace protobuf
}  // namespace google

namespace proto2 {
class FieldDescriptor;
class Descriptor;
class EnumDescriptor;
class Message;
}

namespace upb {

class Def;
class EnumDef;
class FieldDef;
class MessageDef;
class Handlers;

namespace google {

// Returns a upb::Handlers object that can be used to populate a proto2::Message
// object of the same type as "m."  For more control over handler caching and
// reuse, instantiate a CodeCache object below.
upb::reffed_ptr<const upb::Handlers> NewWriteHandlers(const proto2::Message& m);
upb::reffed_ptr<const upb::Handlers> NewWriteHandlers(
    const ::google::protobuf::Message& m);

// Builds upb::Defs from proto2::Descriptors, and caches all built Defs for
// reuse.  CodeCache (below) uses this internally; there is no need to use this
// class directly unless you only want Defs without corresponding Handlers.
//
// This class is NOT thread-safe.
class DefBuilder {
 public:
  // Functions to get or create a Def from a corresponding proto2 Descriptor.
  // The returned def will be frozen.
  //
  // The caller must take a ref on the returned value if it needs it long-term.
  // The DefBuilder will retain a ref so it can keep the Def cached, but
  // garbage-collection functionality may be added to DefBuilder later that
  // could unref the returned pointer.
  const EnumDef* GetOrCreateEnumDef(const proto2::EnumDescriptor* d);
  const EnumDef* GetOrCreateEnumDef(
      const ::google::protobuf::EnumDescriptor* d);
  const MessageDef* GetOrCreateMessageDef(const proto2::Descriptor* d);
  const MessageDef* GetOrCreateMessageDef(
      const ::google::protobuf::Descriptor* d);

  // Gets or creates a frozen MessageDef, properly expanding weak fields.
  //
  // Weak fields are only represented as BYTES fields in the Descriptor (unless
  // you construct your descriptors in a somewhat complicated way; see
  // https://goto.google.com/weak-field-descriptor), but we can get their true
  // definitions relatively easily from the proto Message class.
  const MessageDef* GetOrCreateMessageDefExpandWeak(const proto2::Message& m);
  const MessageDef* GetOrCreateMessageDefExpandWeak(
      const ::google::protobuf::Message& m);

 private:
  // Like GetOrCreateMessageDef*(), except the returned def might not be frozen.
  // We need this function because circular graphs of MessageDefs need to all
  // be frozen together, to we have to create the graphs of defs in an unfrozen
  // state first.
  //
  // If m is non-NULL, expands weak message fields.
  const MessageDef* GetOrCreateMaybeUnfrozenMessageDef(
      const proto2::Descriptor* d, const proto2::Message* m);
  const MessageDef* GetOrCreateMaybeUnfrozenMessageDef(
      const ::google::protobuf::Descriptor* d,
      const ::google::protobuf::Message* m);

  // Returns a new-unfrozen FieldDef corresponding to this FieldDescriptor.
  // The return value is always newly created (never cached) and the returned
  // pointer is the only owner of it.
  //
  // If "m" is non-NULL, expands the weak field if it is one, and populates
  // *subm_prototype with a prototype of the submessage if this is a weak or
  // non-weak MESSAGE or GROUP field.
  reffed_ptr<FieldDef> NewFieldDef(const proto2::FieldDescriptor* f,
                                   const proto2::Message* m);
  reffed_ptr<FieldDef> NewFieldDef(const ::google::protobuf::FieldDescriptor* f,
                                   const ::google::protobuf::Message* m);

  // Freeze all defs that haven't been frozen yet.
  void Freeze();

  template <class T>
  T* AddToCache(const void *proto2_descriptor, reffed_ptr<T> def) {
    assert(def_cache_.find(proto2_descriptor) == def_cache_.end());
    def_cache_[proto2_descriptor] = def;
    return def.get();  // Continued lifetime is guaranteed by cache.
  }

  template <class T>
  const T* FindInCache(const void *proto2_descriptor) {
    DefCache::iterator iter = def_cache_.find(proto2_descriptor);
    return iter == def_cache_.end() ? NULL :
        upb::down_cast<const T*>(iter->second.get());
  }

 private:
  // Maps a proto2 descriptor to the corresponding upb Def we have constructed.
  // The proto2 descriptor is void* because the proto2 descriptor types do not
  // share a common base.
  typedef std::map<const void*, reffed_ptr<upb::Def> > DefCache;
  DefCache def_cache_;

  // Defs that have not been frozen yet.
  vector<Def*> to_freeze_;
};

// Builds and caches upb::Handlers for populating proto2 generated classes.
//
// This class is NOT thread-safe.
class CodeCache {
 public:
  // Gets or creates handlers for populating messages of the given message type.
  //
  // The caller must take a ref on the returned value if it needs it long-term.
  // The CodeCache will retain a ref so it can keep the Def cached, but
  // garbage-collection functionality may be added to CodeCache later that could
  // unref the returned pointer.
  const Handlers* GetOrCreateWriteHandlers(const proto2::Message& m);
  const Handlers* GetOrCreateWriteHandlers(
      const ::google::protobuf::Message& m);

 private:
  const Handlers* GetOrCreateMaybeUnfrozenWriteHandlers(
      const MessageDef* md, const proto2::Message& m);
  const Handlers* GetOrCreateMaybeUnfrozenWriteHandlers(
      const MessageDef* md, const ::google::protobuf::Message& m);

  Handlers* AddToCache(const MessageDef* md, reffed_ptr<Handlers> handlers) {
    assert(handlers_cache_.find(md) == handlers_cache_.end());
    handlers_cache_[md] = handlers;
    return handlers.get();  // Continue lifetime is guaranteed by the cache.
  }

  const Handlers* FindInCache(const MessageDef* md) {
    HandlersCache::iterator iter = handlers_cache_.find(md);
    return iter == handlers_cache_.end() ? NULL : iter->second.get();
  }

  DefBuilder def_builder_;

  typedef std::map<const MessageDef*, upb::reffed_ptr<const Handlers> >
      HandlersCache;
  HandlersCache handlers_cache_;

  vector<Handlers*> to_freeze_;
};

}  // namespace google
}  // namespace upb

#endif  // UPB_GOOGLE_BRIDGE_H_
