// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// The ReflectionModes are ordered in observability levels:
// kDefault: Lowest level. All reflection calls are observable.
// kDebugString: Middle level. Only reflection calls in Message::DebugString are
//               observable.
// kDiagnostics: Highest level. No reflection calls are observable.
enum class ReflectionMode {
  kDefault,
  kDebugString,
  kDiagnostics,
};

// Returns the current ReflectionMode of protobuf for the current thread. This
// reflection mode can be used by interceptors to ignore any reflection calls
// not affecting the application behavior.
// Always returns `kDefault' if the current platform does not support thread
// local data.
ReflectionMode GetReflectionMode();

// Scoping class to set the specific ReflectionMode for a given scope.
class PROTOBUF_EXPORT ScopedReflectionMode final {
 public:
  // Sets the current reflection mode, which will be restored at destruction.
  // The reflection mode can only be 'elevated' in observability levels.
  // For instance, if the current mode is `kDiagnostics` then scope will remain
  // unchanged regardless of `mode`.
  explicit ScopedReflectionMode(ReflectionMode mode);

  // Restores the previous reflection mode.
  ~ScopedReflectionMode();

  // Returns the scoped ReflectionMode for the current thread.
  // See `GetReflectionMode()` for more information on purpose and usage.
  static ReflectionMode current_reflection_mode();

  // ScopedReflectionMode is only intended to be used as a locally scoped
  // instance to set a reflection mode for the code scoped by this instance.
  ScopedReflectionMode(const ScopedReflectionMode&) = delete;
  ScopedReflectionMode& operator=(const ScopedReflectionMode&) = delete;

 private:
#if !defined(PROTOBUF_NO_THREADLOCAL)
  const ReflectionMode previous_mode_;
#if defined(PROTOBUF_USE_DLLS) && defined(_WIN32)
  // Thread local variables cannot be exposed through MSVC DLL interface but we
  // can wrap them in static functions.
  static ReflectionMode& reflection_mode();
#else
  PROTOBUF_CONSTINIT static PROTOBUF_THREAD_LOCAL ReflectionMode
      reflection_mode_;
#endif  // PROTOBUF_USE_DLLS && _MSC_VER
#endif  // !PROTOBUF_NO_THREADLOCAL
};

#if !defined(PROTOBUF_NO_THREADLOCAL)

#if defined(PROTOBUF_USE_DLLS) && defined(_WIN32)

inline ScopedReflectionMode::ScopedReflectionMode(ReflectionMode mode)
    : previous_mode_(reflection_mode()) {
  if (mode > reflection_mode()) {
    reflection_mode() = mode;
  }
}

inline ScopedReflectionMode::~ScopedReflectionMode() {
  reflection_mode() = previous_mode_;
}

inline ReflectionMode ScopedReflectionMode::current_reflection_mode() {
  return reflection_mode();
}

#else

inline ScopedReflectionMode::ScopedReflectionMode(ReflectionMode mode)
    : previous_mode_(reflection_mode_) {
  if (mode > reflection_mode_) {
    reflection_mode_ = mode;
  }
}

inline ScopedReflectionMode::~ScopedReflectionMode() {
  reflection_mode_ = previous_mode_;
}

inline ReflectionMode ScopedReflectionMode::current_reflection_mode() {
  return reflection_mode_;
}

#endif  // PROTOBUF_USE_DLLS && _MSC_VER

#else

inline ScopedReflectionMode::ScopedReflectionMode(ReflectionMode mode) {}
inline ScopedReflectionMode::~ScopedReflectionMode() {}
inline ReflectionMode ScopedReflectionMode::current_reflection_mode() {
  return ReflectionMode::kDefault;
}

#endif  // !PROTOBUF_NO_THREADLOCAL

inline ReflectionMode GetReflectionMode() {
  return ScopedReflectionMode::current_reflection_mode();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REFLECTION_MODE_H__
