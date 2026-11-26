// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_ARENA_ALLOC_RESULT_H__
#define GOOGLE_PROTOBUF_ARENA_ALLOC_RESULT_H__

#include "google/protobuf/arena_seed.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// The result of a non-message allocation from an arena.
struct AllocationResult {
  void* ptr;
};

// The result of a message allocation from an arena.
struct MessageAllocationResult {
  void* ptr;
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  EncodedArenaAlignment encoded_arena_alignment;
#else
  EncodedArenaAlignment encoded_arena_alignment =
      EncodedArenaAlignment::NoArena();
#endif
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_ALLOC_RESULT_H__
