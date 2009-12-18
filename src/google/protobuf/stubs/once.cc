// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Author: kenton@google.com (Kenton Varda)
//
// emulates google3/base/once.h
//
// This header is intended to be included only by internal .cc files and
// generated .pb.cc files.  Users should not use this directly.

#ifdef _WIN32
#include <windows.h>
#endif

#include <google/protobuf/stubs/once.h>

namespace google {
namespace protobuf {

#ifdef _WIN32

struct ProtobufOnceInternal {
  ProtobufOnceInternal() {
    InitializeCriticalSection(&critical_section);
  }
  ~ProtobufOnceInternal() {
    DeleteCriticalSection(&critical_section);
  }
  CRITICAL_SECTION critical_section;
};

ProtobufOnceType::~ProtobufOnceType()
{
  delete internal_;
  internal_ = NULL;
}

ProtobufOnceType::ProtobufOnceType() {
  // internal_ may be non-NULL if Init() was already called.
  if (internal_ == NULL) internal_ = new ProtobufOnceInternal;
}

void ProtobufOnceType::Init(void (*init_func)()) {
  // internal_ may be NULL if we're still in dynamic initialization and the
  // constructor has not been called yet.  As mentioned in once.h, we assume
  // that the program is still single-threaded at this time, and therefore it
  // should be safe to initialize internal_ like so.
  if (internal_ == NULL) internal_ = new ProtobufOnceInternal;

  EnterCriticalSection(&internal_->critical_section);
  if (!initialized_) {
    init_func();
    initialized_ = true;
  }
  LeaveCriticalSection(&internal_->critical_section);
}

#endif

}  // namespace protobuf
}  // namespace google
