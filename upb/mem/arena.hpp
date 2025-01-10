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

namespace upb {

class Arena {
 public:
  // A simple arena with no initial memory block and the default allocator.
  Arena() : ptr_(upb_Arena_New(), upb_Arena_Free) {}
  Arena(char* initial_block, size_t size)
      : ptr_(upb_Arena_Init(initial_block, size, &upb_alloc_global),
             upb_Arena_Free) {}
  explicit Arena(size_t size)
      : ptr_(upb_Arena_NewSized(size), upb_Arena_Free) {}

  upb_Arena* ptr() const { return ptr_.get(); }

  // Fuses the arenas together.
  // This operation can only be performed on arenas with no initial blocks. Will
  // return false if the fuse failed due to either arena having an initial
  // block.
  bool Fuse(Arena& other) { return upb_Arena_Fuse(ptr(), other.ptr()); }

 protected:
  std::unique_ptr<upb_Arena, decltype(&upb_Arena_Free)> ptr_;
};
}  // namespace upb

#endif  // __cplusplus

#endif  // UPB_MEM_ARENA_HPP_
