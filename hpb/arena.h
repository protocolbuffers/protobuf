// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_ARENA_H__
#define GOOGLE_PROTOBUF_HPB_ARENA_H__

#include <cstddef>

#include "hpb/backend/types.h"
#include "hpb/multibackend.h"

namespace hpb {
namespace internal {
struct PrivateAccess;
}

class Arena {
 public:
  Arena() = default;
  Arena(char* initial_block, size_t size) : arena_(initial_block, size) {}

// There are certain operations that are only supported by upb, e.g. fusing.
#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
  bool Fuse(Arena& other) { return arena_.Fuse(other.arena_); };
  bool IsFused(Arena& other) { return arena_.IsFused(other.arena_); };
#endif

 private:
  backend::Arena arena_;

  friend struct hpb::internal::PrivateAccess;
};
}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_ARENA_H__
