// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_PROTOBUF_ARENA_ALLOCATION_POLICY_H__
#define GOOGLE_PROTOBUF_ARENA_ALLOCATION_POLICY_H__

#include <cstddef>
#include <cstdint>

#include "google/protobuf/arena_config.h"

namespace google {
namespace protobuf {
namespace internal {

// `AllocationPolicy` defines `Arena` allocation policies. Applications can
// customize the initial and maximum sizes for arena allocation, as well as set
// custom allocation and deallocation functions. `AllocationPolicy` is for
// protocol buffer internal use only, and typically created from a user facing
// public configuration class such as `ArenaOptions`.
struct AllocationPolicy {
  static constexpr size_t kDefaultStartBlockSize = 256;

  size_t start_block_size = kDefaultStartBlockSize;
  size_t max_block_size = GetDefaultArenaMaxBlockSize();

  void* (*block_alloc)(size_t) = nullptr;
  void (*block_dealloc)(void*, size_t) = nullptr;

  bool IsDefault() const {
    return start_block_size == kDefaultStartBlockSize &&
           max_block_size == GetDefaultArenaMaxBlockSize() &&
           block_alloc == nullptr && block_dealloc == nullptr;
  }
};

// Tagged pointer to an AllocationPolicy.
class TaggedAllocationPolicyPtr {
 public:
  constexpr TaggedAllocationPolicyPtr() : policy_(0) {}

  explicit TaggedAllocationPolicyPtr(AllocationPolicy* policy)
      : policy_(reinterpret_cast<uintptr_t>(policy)) {}

  void set_policy(AllocationPolicy* policy) {
    auto bits = policy_ & kTagsMask;
    policy_ = reinterpret_cast<uintptr_t>(policy) | bits;
  }

  AllocationPolicy* get() {
    return reinterpret_cast<AllocationPolicy*>(policy_ & kPtrMask);
  }
  const AllocationPolicy* get() const {
    return reinterpret_cast<const AllocationPolicy*>(policy_ & kPtrMask);
  }

  AllocationPolicy& operator*() { return *get(); }
  const AllocationPolicy& operator*() const { return *get(); }

  AllocationPolicy* operator->() { return get(); }
  const AllocationPolicy* operator->() const { return get(); }

  bool is_user_owned_initial_block() const {
    return static_cast<bool>(get_mask<kUserOwnedInitialBlock>());
  }
  void set_is_user_owned_initial_block(bool v) {
    set_mask<kUserOwnedInitialBlock>(v);
  }

  uintptr_t get_raw() const { return policy_; }

 private:
  enum : uintptr_t {
    kUserOwnedInitialBlock = 1,
  };

  static constexpr uintptr_t kTagsMask = 7;
  static constexpr uintptr_t kPtrMask = ~kTagsMask;

  template <uintptr_t kMask>
  uintptr_t get_mask() const {
    return policy_ & kMask;
  }
  template <uintptr_t kMask>
  void set_mask(bool v) {
    if (v) {
      policy_ |= kMask;
    } else {
      policy_ &= ~kMask;
    }
  }
  uintptr_t policy_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ARENA_ALLOCATION_POLICY_H__
