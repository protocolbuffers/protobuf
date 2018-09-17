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

// ThreadUnsafeSharedPtr<T> is the same as shared_ptr<T> without the locking
// overhread (and thread-safety).
#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_THREAD_UNSAFE_SHARED_PTR_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_THREAD_UNSAFE_SHARED_PTR_H__

#include <algorithm>
#include <utility>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace python {

template <typename T>
class ThreadUnsafeSharedPtr {
 public:
  // Takes ownership.
  explicit ThreadUnsafeSharedPtr(T* ptr)
      : ptr_(ptr), refcount_(ptr ? new RefcountT(1) : nullptr) {
  }

  ThreadUnsafeSharedPtr(const ThreadUnsafeSharedPtr& other)
      : ThreadUnsafeSharedPtr(nullptr) {
    *this = other;
  }

  ThreadUnsafeSharedPtr& operator=(const ThreadUnsafeSharedPtr& other) {
    if (other.refcount_ == refcount_) {
      return *this;
    }
    this->~ThreadUnsafeSharedPtr();
    ptr_ = other.ptr_;
    refcount_ = other.refcount_;
    if (refcount_) {
      ++*refcount_;
    }
    return *this;
  }

  ~ThreadUnsafeSharedPtr() {
    if (refcount_ == nullptr) {
      GOOGLE_DCHECK(ptr_ == nullptr);
      return;
    }
    if (--*refcount_ == 0) {
      delete refcount_;
      delete ptr_;
    }
  }

  void reset(T* ptr = nullptr) { *this = ThreadUnsafeSharedPtr(ptr); }

  T* get() { return ptr_; }
  const T* get() const { return ptr_; }

  void swap(ThreadUnsafeSharedPtr& other) {
    using std::swap;
    swap(ptr_, other.ptr_);
    swap(refcount_, other.refcount_);
  }

 private:
  typedef int RefcountT;
  T* ptr_;
  RefcountT* refcount_;
};

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_THREAD_UNSAFE_SHARED_PTR_H__
