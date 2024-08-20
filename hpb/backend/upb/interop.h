// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_UPB_INTEROP_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_UPB_INTEROP_H__

// The sole public header in hpb/backend/upb

#include "google/protobuf/hpb/ptr.h"
#include "upb/mini_table/message.h"

namespace hpb::interop::upb {

template <typename T>
const upb_MiniTable* GetMiniTable(const T*) {
  return T::minitable();
}

template <typename T>
const upb_MiniTable* GetMiniTable(Ptr<T>) {
  return T::minitable();
}

}  // namespace hpb::interop::upb

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_UPB_INTEROP_H__
