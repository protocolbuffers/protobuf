// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This file defines internal helpers for arena seeding.

#ifndef GOOGLE_PROTOBUF_ARENA_SEED_H__
#define GOOGLE_PROTOBUF_ARENA_SEED_H__

// Must be included last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

struct ArenaSeedPolicy {
  // When PROTOBUF_SMALLER_MESSAGE_LITE is enabled, we attempt to place `Arena*`
  // "seeds" at highly-aligned addresses (i.e. divisible by a large power of 2)
  // in arena blocks. Messages allocated in an arena will record the alignment
  // of the arena seed that preceeds them, allowing them to recover the `Arena*`
  // by aligning down. This prevents us from needing to store an `Arena*` in
  // every message.
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  static constexpr bool kEnableArenaSeeding = true;
#else
  static constexpr bool kEnableArenaSeeding = false;
#endif
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_SEED_H__
