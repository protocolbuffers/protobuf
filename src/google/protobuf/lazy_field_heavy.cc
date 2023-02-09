// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include "google/protobuf/lazy_field.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {
class ByFactory {
 public:
  explicit ByFactory(const Descriptor* type, MessageFactory* factory)
      : type_(type), factory_(factory) {}

  Message* New(Arena* arena) const {
    return factory_->GetPrototype(type_)->New(arena);
  }

  const Message& Default() const { return *factory_->GetPrototype(type_); }

 private:
  const Descriptor* type_;
  MessageFactory* factory_;
};

}  // namespace

const Message& LazyField::GetDynamic(const Descriptor* type,
                                     MessageFactory* factory,
                                     Arena* arena) const {
  return DownCast<const Message&>(GetGeneric(ByFactory(type, factory), arena));
}

Message* LazyField::MutableDynamic(const Descriptor* type,
                                   MessageFactory* factory, Arena* arena) {
  return DownCast<Message*>(
      MutableGeneric(ByFactory(type, factory), arena, nullptr));
}

Message* LazyField::ReleaseDynamic(const Descriptor* type,
                                   MessageFactory* factory, Arena* arena) {
  return DownCast<Message*>(ReleaseGeneric(ByFactory(type, factory), arena));
}

Message* LazyField::UnsafeArenaReleaseDynamic(const Descriptor* type,
                                              MessageFactory* factory,
                                              Arena* arena) {
  return DownCast<Message*>(
      UnsafeArenaReleaseGeneric(ByFactory(type, factory), arena));
}

size_t LazyField::SpaceUsedExcludingSelfLong() const {
  // absl::Cord::EstimatedMemoryUsage counts itself that should be excluded
  // because sizeof(Cord) is already counted in self.
  size_t total_size = unparsed_.EstimatedMemoryUsage() - sizeof(absl::Cord);
  switch (GetLogicalState()) {
    case LogicalState::kClearExposed:
    case LogicalState::kNoParseRequired:
    case LogicalState::kDirty:
    case LogicalState::kParseError: {
      const auto* message = raw_.load(std::memory_order_relaxed).message();
      total_size += DownCast<const Message*>(message)->SpaceUsedLong();
    } break;
    case LogicalState::kClear:
    case LogicalState::kParseRequired:
      // We may have a `Message*` here, but we cannot safely access it
      // because, a racing SharedInit could delete it out from under us.
      // Other states in this structure are already passed kSharedInit and are
      // thus safe.
      break;  // Nothing to add.
  }
  return total_size;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
