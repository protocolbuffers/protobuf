// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_ARENA_ALLOC_RESULT_H__
#define GOOGLE_PROTOBUF_ARENA_ALLOC_RESULT_H__

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
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ARENA_ALLOC_RESULT_H__
