// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_INTERNAL_INTERNAL_H__
#define GOOGLE_PROTOBUF_HPB_INTERNAL_INTERNAL_H__

#include <cstdint>

#include "upb/mem/arena.h"
#include "upb/message/message.h"

namespace hpb::internal {

struct PrivateAccess {
  template <typename T>
  static auto* GetInternalMsg(T&& message) {
    return message->msg();
  }
  template <typename T>
  static auto* GetInternalArena(T&& message) {
    return message->arena();
  }
  template <typename T>
  static auto Proxy(upb_Message* p, upb_Arena* arena) {
    return typename T::Proxy(p, arena);
  }
  template <typename T>
  static auto CProxy(const upb_Message* p, upb_Arena* arena) {
    return typename T::CProxy(p, arena);
  }
  template <typename T>
  static auto CreateMessage(upb_Arena* arena) {
    return typename T::Proxy(upb_Message_New(T::minitable(), arena), arena);
  }

  template <typename ExtensionId>
  static constexpr uint32_t GetExtensionNumber(const ExtensionId& id) {
    return id.number();
  }
};

}  // namespace hpb::internal

#endif  // GOOGLE_PROTOBUF_HPB_INTERNAL_INTERNAL_H__
