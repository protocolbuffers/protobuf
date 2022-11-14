// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/arena_cleanupx.h"

#include <ios>
#include <ostream>

namespace google {
namespace protobuf {
namespace internal {
namespace cleanupx {

std::ostream& operator<<(std::ostream& s, StringAllocation) {
  return s << "StringAllocation";
}

std::ostream& operator<<(std::ostream& s, CordAllocation) {
  return s << "CordAllocation";
}

std::ostream& operator<<(std::ostream& s, TaggedAllocation arg) {
  return s << "TaggedAllocation(" << arg.tag << ")";
}

std::ostream& operator<<(std::ostream& s, TaggedCleanup arg) {
  return s << "TaggedCleanup(" << arg.tag << ", object = " << arg.object << ")";
}

std::ostream& operator<<(std::ostream& s, DynamicAllocation arg) {
  return s << "DynamicAllocation(size = " << arg.size
           << ", dtor = " << AsPointer(arg.dtor) << ")";
}

std::ostream& operator<<(std::ostream& s, DynamicCleanup arg) {
  return s << "DynamicCleanup(object = " << arg.object
           << ", dtor = " << AsPointer(arg.dtor) << ")";
}

std::ostream& operator<<(std::ostream& s, Tag tag) {  // NOLINT
  switch (tag) {
    case Tag::kDynamic:
      return s << "Tag::kDynamic";
    case Tag::kDynamicPointer:
      return s << "Tag::kDynamicPointer";
    case Tag::kString:
      return s << "Tag::kString";
    case Tag::kCord:
      return s << "Tag::kCord";
  }
  return s << "Tag::???";
}

std::ostream& operator<<(std::ostream& s, DynamicNode node) {
  void* dtor;
  memcpy(&dtor, &node.dtor, sizeof(dtor));
  uintptr_t tag = node.ptr_or_size & 3;
  switch (static_cast<Tag>(tag)) {
    case Tag::kDynamic:
      return s << "DynamicNode(size = " << (node.ptr_or_size - tag)
               << ", dtor = " << dtor << ")";

    case Tag::kDynamicPointer:
      return s << "DynamicNode(object = " << std::hex
               << (node.ptr_or_size - tag) << ", dtor = " << dtor << ")";

    default:
      return s << "<illegal node tag " << static_cast<Tag>(tag) << ">";
  }
}

std::ostream& operator<<(std::ostream& s, TaggedNode node) {
  uintptr_t tag = node.object & 3;
  switch (static_cast<Tag>(tag)) {
    case Tag::kString:
      if (node.object == tag) {
        return s << "TaggedStringNode(embedded)";
      } else {
        return s << "TaggedStringNode(object = " << std::hex
                 << node.object - tag << ")";
      }

    case Tag::kCord:
      if (node.object == tag) {
        return s << "TaggedCordNode(embedded)";
      } else {
        return s << "TaggedCordNode(object = " << std::hex << node.object - tag
                 << ")";
      }

    default:
      return s << "<illegal node tag " << static_cast<Tag>(tag) << ">";
  }
}

std::ostream& operator<<(std::ostream& s, NodeRef ref) {
  uintptr_t head = *static_cast<const uintptr_t*>(ref.address);
  uintptr_t tag = head & 3;
  s << "Node ref @" << ref.address << ": ";
  if (tag <= Cast(Tag::kDynamicPointer)) {
    return s << *static_cast<const DynamicNode*>(ref.address);
  } else {
    return s << *static_cast<const TaggedNode*>(ref.address);
  }
}

}  // namespace cleanupx
}  // namespace internal
}  // namespace protobuf
}  // namespace google
