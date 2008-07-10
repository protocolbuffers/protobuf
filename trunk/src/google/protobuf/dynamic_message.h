// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Defines an implementation of Message which can emulate types which are not
// known at compile-time.

#ifndef GOOGLE_PROTOBUF_DYNAMIC_MESSAGE_H__
#define GOOGLE_PROTOBUF_DYNAMIC_MESSAGE_H__

#include <google/protobuf/message.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

// Defined in other files.
class Descriptor;        // descriptor.h
class DescriptorPool;    // descriptor.h

// Constructs implementations of Message which can emulate types which are not
// known at compile-time.
//
// Sometimes you want to be able to manipulate protocol types that you don't
// know about at compile time.  It would be nice to be able to construct
// a Message object which implements the message type given by any arbitrary
// Descriptor.  DynamicMessage provides this.
//
// As it turns out, a DynamicMessage needs to construct extra
// information about its type in order to operate.  Most of this information
// can be shared between all DynamicMessages of the same type.  But, caching
// this information in some sort of global map would be a bad idea, since
// the cached information for a particular descriptor could outlive the
// descriptor itself.  To avoid this problem, DynamicMessageFactory
// encapsulates this "cache".  All DynamicMessages of the same type created
// from the same factory will share the same support data.  Any Descriptors
// used with a particular factory must outlive the factory.
class LIBPROTOBUF_EXPORT DynamicMessageFactory : public MessageFactory {
 public:
  // Construct a DynamicMessageFactory that will search for extensions in
  // the DescriptorPool in which the exendee is defined.
  DynamicMessageFactory();

  // Construct a DynamicMessageFactory that will search for extensions in
  // the given DescriptorPool.
  DynamicMessageFactory(const DescriptorPool* pool);
  ~DynamicMessageFactory();

  // implements MessageFactory ---------------------------------------

  // Given a Descriptor, constructs the default (prototype) Message of that
  // type.  You can then call that message's New() method to construct a
  // mutable message of that type.
  //
  // Calling this method twice with the same Descriptor returns the same
  // object.  The returned object remains property of the factory and will
  // be destroyed when the factory is destroyed.  Also, any objects created
  // by calling the prototype's New() method share some data with the
  // prototype, so these must be destoyed before the DynamicMessageFactory
  // is destroyed.
  //
  // The given descriptor must outlive the returned message, and hence must
  // outlive the DynamicMessageFactory.
  //
  // Note that while GetPrototype() is idempotent, it is not const.  This
  // implies that it is not thread-safe to call GetPrototype() on the same
  // DynamicMessageFactory in two different threads simultaneously.  However,
  // the returned objects are just as thread-safe as any other Message.
  const Message* GetPrototype(const Descriptor* type);

 private:
  const DescriptorPool* pool_;

  // This struct just contains a hash_map.  We can't #include <google/protobuf/stubs/hash.h> from
  // this header due to hacks needed for hash_map portability in the open source
  // release.  Namely, stubs/hash.h, which defines hash_map portably, is not a
  // public header (for good reason), but dynamic_message.h is, and public
  // headers may only #include other public headers.
  struct PrototypeMap;
  scoped_ptr<PrototypeMap> prototypes_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DynamicMessageFactory);
};

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_DYNAMIC_MESSAGE_H__

