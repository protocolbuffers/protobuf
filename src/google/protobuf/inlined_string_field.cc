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


std::string* InlinedStringField::Mutable(const LazyString& /*default_value*/,
                                         Arena* arena, bool donated,
                                         uint32_t* donating_states,
                                         uint32_t mask, MessageLite* msg) {
  ScopedCheckInvariants invariants(this);
  if (arena == nullptr || !donated) {
    return UnsafeMutablePointer();
  }
  return MutableSlow(arena, donated, donating_states, mask, msg);
}

std::string* InlinedStringField::Mutable(Arena* arena, bool donated,
                                         uint32_t* donating_states,
                                         uint32_t mask, MessageLite* msg) {
  ScopedCheckInvariants invariants(this);
  if (arena == nullptr || !donated) {
    return UnsafeMutablePointer();
  }
  return MutableSlow(arena, donated, donating_states, mask, msg);
}

std::string* InlinedStringField::MutableSlow(::google::protobuf::Arena* arena,
                                             bool donated,
                                             uint32_t* donating_states,
                                             uint32_t mask, MessageLite* msg) {
  (void)mask;
  (void)msg;
  return UnsafeMutablePointer();
}

void InlinedStringField::SetAllocated(const std::string* default_value,
                                      std::string* value, Arena* arena,
                                      bool donated, uint32_t* donating_states,
                                      uint32_t mask, MessageLite* msg) {
  (void)mask;
  (void)msg;
  SetAllocatedNoArena(default_value, value);
}

void InlinedStringField::Set(std::string&& value, Arena* arena, bool donated,
                             uint32_t* donating_states, uint32_t mask,
                             MessageLite* msg) {
  (void)donating_states;
  (void)mask;
  (void)msg;
  SetNoArena(std::move(value));
}

std::string* InlinedStringField::Release() {
  auto* released = new std::string(std::move(*get_mutable()));
  get_mutable()->clear();
  return released;
}

std::string* InlinedStringField::Release(Arena* arena, bool donated) {
  // We can not steal donated arena strings.
  std::string* released = (arena != nullptr && donated)
                              ? new std::string(*get_mutable())
                              : new std::string(std::move(*get_mutable()));
  get_mutable()->clear();
  return released;
}

void InlinedStringField::ClearToDefault(const LazyString& default_value,
                                        Arena* arena, bool donated) {
  (void)arena;
  get_mutable()->assign(default_value.get());
}


}  // namespace internal
}  // namespace protobuf
}  // namespace google
