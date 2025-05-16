// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_ARENA_H__
#define GOOGLE_PROTOBUF_HPB_ARENA_H__

#include "google/protobuf/hpb/multibackend.h"

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
#include "upb/mem/arena.hpp"
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
#include "google/protobuf/arena.h"
#else
#error "Unsupported hpb backend"
#endif

namespace hpb {
namespace internal {
struct PrivateAccess;
}

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
using ArenaType = upb::Arena;
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
using ArenaType = google::protobuf::Arena;
#else
#error hpb backend unknown
#endif

class Arena {
 public:
  Arena() = default;

 private:
  ArenaType arena_;

  friend struct hpb::internal::PrivateAccess;
};
}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_ARENA_H__
