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

// This file exists for testing allocation behavior when using arenas by hooking
// new/delete. It is a copy of //experimental/mvels/util/new_delete_capture.cc.

#include <google/protobuf/new_delete_capture.h>

#include <pthread.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/malloc_hook.h>
#include <google/protobuf/stubs/spinlock.h>

namespace google {
namespace {

pthread_t gthread;
protobuf_unittest::NewDeleteCapture *ghooked_instance = NULL;
SpinLock gspinlock(base::LINKER_INITIALIZED);

}  // namespace

namespace protobuf_unittest {

NewDeleteCapture::NewDeleteCapture()
    : alloc_count_(0),
      alloc_size_(0),
      alloc_ptr_(NULL),
      free_count_(0),
      free_ptr_(NULL) {}

NewDeleteCapture::~NewDeleteCapture() { Unhook(); }

void NewDeleteCapture::Reset() {
  alloc_count_ = 0;
  alloc_size_ = 0;
  free_count_ = 0;
  alloc_ptr_ = NULL;
  free_ptr_ = NULL;
}

bool NewDeleteCapture::Hook(bool reset) {
  SpinLockHolder spinlock(&gspinlock);
  if (ghooked_instance != this) {
    GOOGLE_CHECK(ghooked_instance == NULL)
        << " NewDeleteCapture can have only 1 active instance";
    GOOGLE_CHECK(MallocHook::AddNewHook(NewHook));
    GOOGLE_CHECK(MallocHook::AddDeleteHook(DeleteHook));
    gthread = pthread_self();
    ghooked_instance = this;
    if (reset) {
      Reset();
    }
    return true;
  }
  return false;
}

bool NewDeleteCapture::Unhook() {
  SpinLockHolder spinlock(&gspinlock);
  if (ghooked_instance == this) {
    gthread = pthread_t();
    ghooked_instance = NULL;
    GOOGLE_CHECK(MallocHook::RemoveDeleteHook(DeleteHook));
    GOOGLE_CHECK(MallocHook::RemoveNewHook(NewHook));
    return true;
  }
  return false;
}

void NewDeleteCapture::NewHook(const void *ptr, size_t size) {
  SpinLockHolder spinlock(&gspinlock);
  if (gthread == pthread_self()) {
    auto &rthis = *ghooked_instance;
    if (++rthis.alloc_count_ == 1) {
      rthis.alloc_size_ = size;
      rthis.alloc_ptr_ = ptr;
    }
  }
}

void NewDeleteCapture::DeleteHook(const void *ptr) {
  SpinLockHolder spinlock(&gspinlock);
  if (gthread == pthread_self()) {
    auto &rthis = *ghooked_instance;
    if (++rthis.free_count_ == 1) {
      rthis.free_ptr_ = ptr;
    }
  }
}

}  // namespace protobuf_unittest
}  // namespace google
