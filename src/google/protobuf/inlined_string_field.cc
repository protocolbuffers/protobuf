// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/inlined_string_field.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/strings/internal/resize_uninitialized.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"


// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

#if defined(NDEBUG) || !defined(GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE)

class InlinedStringField::ScopedCheckInvariants {
 public:
  constexpr explicit ScopedCheckInvariants(const InlinedStringField*) {}
};

#endif  // NDEBUG || !GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE


std::string* InlinedStringField::Mutable(Arena* arena) {
  ScopedCheckInvariants invariants(this);
  if (arena == nullptr || !IsDonated()) {
    return UnsafeMutablePointer();
  }
  return MutableSlow(arena);
}

bool InlinedStringField::IsDonated(const std::string& str) {
#if defined(GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE)
  return robber::IsLong(str) ? (robber::GetLongCap(str) & kDonatedBit) != 0
                             : true;
#else
  return false;
#endif
}

size_t InlinedStringField::SpaceUsedExcludingSelfLong() const {
#if defined(GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE)
  if (robber::IsLong(str_)) {
    // Use our accessor that strips the donation bit.
    return Capacity();
  }
#endif
  return StringSpaceUsedExcludingSelfLong(str_);
}

bool InlinedStringField::IsDonated() const { return IsDonated(str_); }

bool InlinedStringField::IsLongDonated() const {
#if defined(GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE)
  return robber::IsLong(str_) && IsDonated();
#else
  return false;
#endif
}

size_t InlinedStringField::Capacity() const {
#if defined(GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE)
  if (robber::IsLong(str_)) {
    return (robber::GetLongCap(str_) & ~kDonatedBit) - 1;
  }
#endif
  return str_.capacity();
}

void InlinedStringField::RegisterForDestruction(Arena* arena,
                                                std::string* str) {
  arena->OwnCustomDestructor(str, DestroyArenaString);
}

void InlinedStringField::DestroyArenaString(void* p) {
  std::string* str = static_cast<std::string*>(p);
  if (IsDonated(*str)) return;

  str->~basic_string();
  // Reset in case we destroy more than once.
  ::new (p) std::string();
}

std::string* InlinedStringField::MutableSlow(::google::protobuf::Arena* arena) {
  return UnsafeMutablePointer();
}

void InlinedStringField::SetAllocated(std::string* value, Arena* arena) {
  SetAllocatedNoArena(value);
}

void InlinedStringField::Set(std::string&& value, Arena* arena) {
  SetNoArena(std::move(value));
}

std::string* InlinedStringField::Release() {
  auto* released = new std::string(std::move(*get_mutable()));
  get_mutable()->clear();
  return released;
}

std::string* InlinedStringField::Release(Arena* arena) {
  // We can not steal donated arena strings.
  std::string* released = (arena != nullptr && IsDonated())
                              ? new std::string(*get_mutable())
                              : new std::string(std::move(*get_mutable()));
  get_mutable()->clear();
  return released;
}

void InlinedStringField::ClearToDefault(const LazyString& default_value,
                                        Arena* arena, bool) {
  (void)arena;
  get_mutable()->assign(default_value.get());
}


}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
