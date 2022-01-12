// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef UPB_HPP_
#define UPB_HPP_

#include <memory>

#include "upb/upb.h"

namespace upb {

class Status {
 public:
  Status() { upb_Status_Clear(&status_); }

  upb_Status* ptr() { return &status_; }

  // Returns true if there is no error.
  bool ok() const { return upb_Status_IsOk(&status_); }

  // Guaranteed to be NULL-terminated.
  const char* error_message() const {
    return upb_Status_ErrorMessage(&status_);
  }

  // The error message will be truncated if it is longer than
  // _kUpb_Status_MaxMessage-4.
  void SetErrorMessage(const char* msg) {
    upb_Status_SetErrorMessage(&status_, msg);
  }
  void SetFormattedErrorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    upb_Status_VSetErrorFormat(&status_, fmt, args);
    va_end(args);
  }

  // Resets the status to a successful state with no message.
  void Clear() { upb_Status_Clear(&status_); }

 private:
  upb_Status status_;
};

class Arena {
 public:
  // A simple arena with no initial memory block and the default allocator.
  Arena() : ptr_(upb_Arena_New(), upb_Arena_Free) {}
  Arena(char* initial_block, size_t size)
      : ptr_(upb_Arena_Init(initial_block, size, &upb_alloc_global),
             upb_Arena_Free) {}

  upb_Arena* ptr() { return ptr_.get(); }

  // Allows this arena to be used as a generic allocator.
  //
  // The arena does not need free() calls so when using Arena as an allocator
  // it is safe to skip them.  However they are no-ops so there is no harm in
  // calling free() either.
  upb_alloc* allocator() { return upb_Arena_Alloc(ptr_.get()); }

  // Add a cleanup function to run when the arena is destroyed.
  // Returns false on out-of-memory.
  template <class T>
  bool Own(T* obj) {
    return upb_Arena_AddCleanup(ptr_.get(), obj,
                                [](void* obj) { delete static_cast<T*>(obj); });
  }

  void Fuse(Arena& other) { upb_Arena_Fuse(ptr(), other.ptr()); }

 private:
  std::unique_ptr<upb_Arena, decltype(&upb_Arena_Free)> ptr_;
};

// InlinedArena seeds the arenas with a predefined amount of memory.  No
// heap memory will be allocated until the initial block is exceeded.
template <int N>
class InlinedArena : public Arena {
 public:
  InlinedArena() : Arena(initial_block_, N) {}

 private:
  InlinedArena(const InlinedArena*) = delete;
  InlinedArena& operator=(const InlinedArena*) = delete;

  char initial_block_[N];
};

}  // namespace upb

#endif  // UPB_HPP_
