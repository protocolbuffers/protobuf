// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_CPP_INTEROP_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_CPP_INTEROP_H__

#include "hpb/internal/internal.h"

// The sole public header in hpb/backend/cpp

namespace hpb::interop::cpp {

/*
 * Returns the underlying google::protobuf::message for a given hpb model.
 */
template <typename T>
auto* GetMessage(T&& message) {
  return internal::PrivateAccess::GetInternalMsg(std::forward<T>(message));
}

}  // namespace hpb::interop::cpp

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_CPP_INTEROP_H__
