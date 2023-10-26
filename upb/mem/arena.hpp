// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MEM_ARENA_HPP_
#define UPB_MEM_ARENA_HPP_

#include <cstddef>

#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"

namespace upb {

// Base C++ API type for upb_Arena. Use for APIs operating on upb_Arena that do
// not deal with ownership / lifetime.
class ArenaBase {
 public:
  upb_Arena* ptr() const { return ptr_; }

 protected:
  explicit ArenaBase(upb_Arena* ptr) : ptr_(ptr) {}
  ArenaBase(const ArenaBase& other) = default;
  ArenaBase& operator=(const ArenaBase& other) = default;
  ArenaBase(ArenaBase&& other) = default;
  ArenaBase& operator=(ArenaBase&& other) = default;

  upb_Arena* ptr_;
};

// Arena whose ownership can be shared between instances via reference counting.
class Arena : public ArenaBase {
 public:
  // A simple arena with no initial memory block and the default allocator.
  Arena() : ArenaBase(upb_Arena_New()) {}

  // Attempts to wrap the provided upb_Arena* in C++ and increment the reference
  // count. Fails and returns a nullptr Arena if ptr has an initial block
  // (is inlined).
  static Arena Wrap(upb_Arena* ptr) {
    if (!upb_Arena_IncRefFor(ptr, /*owner=*/nullptr)) {
      return Arena(nullptr);
    }
    return Arena(ptr);
  }

  // Transfers ownership of a upb_Arena to C++. Does not modify the reference
  // count.
  static Arena MoveOwnership(upb_Arena* ptr) { return Arena(ptr); }

  explicit Arena(std::nullptr_t) : ArenaBase(nullptr) {}

  Arena(const Arena& other) : Arena(other.ptr_) {
    upb_Arena_IncRefFor(ptr_, /*owner=*/nullptr);
  }
  Arena& operator=(const Arena& other) {
    if (ptr_ != nullptr) {
      upb_Arena_DecRefFor(ptr_, /*owner=*/nullptr);
    }
    upb_Arena_IncRefFor(other.ptr_, /*owner=*/nullptr);
    ptr_ = other.ptr_;
    return *this;
  }
  Arena(Arena&& other) : Arena(other.ptr_) { other.ptr_ = nullptr; }
  Arena& operator=(Arena&& other) {
    if (ptr_ != nullptr) {
      upb_Arena_DecRefFor(ptr_, /*owner=*/nullptr);
    }
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
  }
  ~Arena() {
    if (ptr_ != nullptr) {
      upb_Arena_DecRefFor(ptr_, /*owner=*/nullptr);
    }
  }

  // Attempts to reset the Arena to claim ownership of ptr.
  // Fails before reset if ptr has an initial block (is inlined).
  bool Reset(upb_Arena* ptr) {
    if (ptr == nullptr || !upb_Arena_IncRefFor(ptr, /*owner=*/nullptr)) {
      return false;
    }
    upb_Arena_DecRefFor(ptr_, /*owner=*/nullptr);
    ptr_ = ptr;
    return true;
  }

  bool Fuse(Arena& other) {
    if (ptr_ == nullptr || other.ptr_ == nullptr) {
      return false;
    }
    return upb_Arena_Fuse(ptr_, other.ptr_);
  }

  // Can fail if other_ptr has an initial block / is inlined.
  bool Fuse(upb_Arena* other_ptr) {
    if (ptr_ == nullptr || other_ptr == nullptr) {
      return false;
    }
    return upb_Arena_Fuse(ptr_, other_ptr);
  }

 private:
  explicit Arena(upb_Arena* ptr) : ArenaBase(ptr) {}
};

// InlinedArena seeds the arenas with a predefined amount of memory.  No
// heap memory will be allocated until the initial block is exceeded. Cannot be
// shared or moved.
template <int N>
class InlinedArena : public ArenaBase {
 public:
  InlinedArena()
      : ArenaBase(upb_Arena_Init(initial_block_, N, &upb_alloc_global)) {}
  InlinedArena(InlinedArena&) = delete;
  InlinedArena& operator=(InlinedArena&) = delete;
  InlinedArena(InlinedArena&& other) = delete;
  Arena& operator=(Arena&& other) = delete;
  ~InlinedArena() {
    if (ptr_ != nullptr) {
      upb_Arena_Free(ptr_);
      ptr_ = nullptr;
    }
  }

 private:
  char initial_block_[N];
};

}  // namespace upb

#endif  // UPB_MEM_ARENA_HPP_
