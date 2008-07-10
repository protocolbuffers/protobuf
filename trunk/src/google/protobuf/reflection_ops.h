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
// This header is logically internal, but is made public because it is used
// from protocol-compiler-generated code, which may reside in other components.

#ifndef GOOGLE_PROTOBUF_REFLECTION_OPS_H__
#define GOOGLE_PROTOBUF_REFLECTION_OPS_H__

#include <google/protobuf/message.h>

namespace google {
namespace protobuf {
namespace internal {

// Basic operations that can be performed using Message::Reflection.
// These can be used as a cheap way to implement the corresponding
// methods of the Message interface, though they are likely to be
// slower than implementations tailored for the specific message type.
//
// This class should stay limited to operations needed to implement
// the Message interface.
//
// This class is really a namespace that contains only static methods.
class LIBPROTOBUF_EXPORT ReflectionOps {
 public:
  static void Copy(const Descriptor* descriptor,
                   const Message::Reflection& from,
                   Message::Reflection* to);
  static void Merge(const Descriptor* descriptor,
                    const Message::Reflection& from,
                    Message::Reflection* to);
  static void Clear(const Descriptor* descriptor,
                    Message::Reflection* reflection);
  static bool IsInitialized(const Descriptor* descriptor,
                            const Message::Reflection& reflection);
  static void DiscardUnknownFields(const Descriptor* descriptor,
                                   Message::Reflection* reflection);

  // Finds all unset required fields in the message and adds their full
  // paths (e.g. "foo.bar[5].baz") to *names.  "prefix" will be attached to
  // the front of each name.
  static void FindInitializationErrors(const Descriptor* descriptor,
                                       const Message::Reflection& reflection,
                                       const string& prefix,
                                       vector<string>* errors);

 private:
  // All methods are static.  No need to construct.
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ReflectionOps);
};

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_REFLECTION_OPS_H__
