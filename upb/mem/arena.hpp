// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MEM_ARENA_HPP_
#define UPB_MEM_ARENA_HPP_

#include "upb/mem/alloc.h"
#ifdef __cplusplus

#include <cstddef>
#include <memory>

#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {

struct ArenaDeleter {
  void operator()(upb_Arena* arena) const {
    if (arena != nullptr) {
      upb_Arena_Free(arena);
    }
  }
  void operator()(const upb_Arena* arena) const {
    (*this)(const_cast<upb_Arena*>(arena));
  }
};

using ArenaPtr = std::unique_ptr<upb_Arena, ArenaDeleter>;

class Arena {
 public:
  // A simple arena with no initial memory block and the default allocator.
  Arena() : ptr_(upb_Arena_New()) {}
  Arena(char* initial_block, size_t size)
      : ptr_(upb_Arena_Init(initial_block, size, &upb_alloc_global)) {}
  explicit Arena(size_t size) : ptr_(upb_Arena_NewSized(size)) {}

  upb_Arena* ptr() const { return ptr_.get(); }

  // Fuses the arenas together.
  // This operation can only be performed on arenas with no initial blocks. Will
  // return false if the fuse failed due to either arena having an initial
  // block.
  bool Fuse(Arena& other) { return upb_Arena_Fuse(ptr(), other.ptr()); }

  bool IsFused(Arena& other) const {
    return upb_Arena_IsFused(ptr(), other.ptr());
  }

  void RefArena(const Arena& to) {
    bool ok = upb_Arena_RefArena(ptr(), to.ptr());
    UPB_UNUSED(ok);
  }

 protected:
  ArenaPtr ptr_;
};
}  // namespace upb

#include "upb/port/undef.inc"

#endif  // __cplusplus

#endif  // UPB_MEM_ARENA_HPP_
