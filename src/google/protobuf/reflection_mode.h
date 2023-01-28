// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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
//
// This header provides support for a per thread 'reflection mode'.
//
// Some protocol buffer optimizations use interceptors to determine which
// fields are effectively used in the application. These optimizations are
// disabled if certain reflection calls are intercepted as the assumption is
// then that any field data can be accessed.
//
// The 'reflection mode' defined in this header is intended to be used by
// logic such as ad-hoc profilers to indicate that any scoped reflection usage
// is not originating from, or affecting application code. This reflection mode
// can then be used by such interceptors to ignore any reflection calls not
// affecting the application behavior.

#ifndef GOOGLE_PROTOBUF_REFLECTION_MODE_H__
#define GOOGLE_PROTOBUF_REFLECTION_MODE_H__

#include <cstddef>
namespace google {
namespace protobuf {
namespace internal {

enum class ReflectionMode {
  kDefault,      // Default mode.
  kDiagnostics,  // Reflection is invoked as part of some external diagnostics.
  kDebugString,  // Reflection is invoked during Message::DebugString function.
};

// Return the current ReflectionMode of protobuf. This reflection mode
// can be used by interceptors to ignore any reflection calls not
// affecting the application behavior.
ReflectionMode GetReflectionMode();

// Scoping class to set the specific ReflectionMode for a given scope. When this
// goes out of scope, it will revert back to the previous ReflectionMode.
// Noted that ScopedReflectionMode cannot change to DebugString mode if the
// previous refleciton mode is Diagnostics mode.
class ScopedReflectionMode final {
 public:
  // Sets the current reflection mode, which will be restored at destruction.
  explicit ScopedReflectionMode(ReflectionMode mode);

  // Restores the previous reflection mode.
  ~ScopedReflectionMode();

  // ScopedReflectionMode is neither copyable, movable nor assignable.
  ScopedReflectionMode(const ScopedReflectionMode&) = delete;
  ScopedReflectionMode& operator=(const ScopedReflectionMode&) = delete;
  ScopedReflectionMode(ScopedReflectionMode&&) = delete;
  ScopedReflectionMode& operator=(ScopedReflectionMode&&) = delete;

  // The lifecycle of ScopedReflectionMode is supposed to be within a scope,
  // therefore it should not be dynamically allocated.
  void* operator new(size_t) = delete;
  void* operator new[](size_t) = delete;
  void operator delete(void*) = delete;
  void operator delete[](void*) = delete;

 private:
  const ReflectionMode previous_mode_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_REFLECTION_MODE_H__
