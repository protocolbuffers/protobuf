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

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/arena_test_util.h>

#define EXPECT_EQ GOOGLE_CHECK_EQ

static bool hook_;
static int alloc_count_;
static int free_count_;

void* operator new(size_t size) {
  if (hook_) {
    assert(!"should not be called.");
    ++alloc_count_;
  }
  return malloc(size);
}

void operator delete(void* p) {
  if (hook_) {
    assert(!"should not be called.");
    ++free_count_;
  }
  free(p);
}

namespace google {
namespace protobuf {
namespace internal {

NoHeapChecker::~NoHeapChecker() {
  capture_alloc.Unhook();
  EXPECT_EQ(0, capture_alloc.alloc_count());
  EXPECT_EQ(0, capture_alloc.free_count());
}

void NoHeapChecker::NewDeleteCapture::Hook() {
  hook_ = true;
  alloc_count_ = 0;
  free_count_ = 0;
}

void NoHeapChecker::NewDeleteCapture::Unhook() {
  hook_ = false;
}

int NoHeapChecker::NewDeleteCapture::alloc_count() {
  return alloc_count_;
}

int NoHeapChecker::NewDeleteCapture::free_count() {
  return free_count_;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
