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

// A common header that is included across all protobuf headers.  We do our best
// to avoid #defining any macros here; instead we generally put macros in
// port_def.inc and port_undef.inc so they are not visible from outside of
// protobuf.

#ifndef GOOGLE_PROTOBUF_PORT_H__
#define GOOGLE_PROTOBUF_PORT_H__

#include <cstddef>
#include <new>


namespace google {
namespace protobuf {
namespace internal {
inline void SizedDelete(void* p, size_t size) {
#if defined(__cpp_sized_deallocation)
  ::operator delete(p, size);
#else
  // Avoid -Wunused-parameter
  (void)size;
  ::operator delete(p);
#endif
}
inline void SizedArrayDelete(void* p, size_t size) {
#if defined(__cpp_sized_deallocation)
  ::operator delete[](p, size);
#else
  // Avoid -Wunused-parameter
  (void)size;
  ::operator delete[](p);
#endif
}

// Tag type used to invoke the constinit constructor overload of classes
// such as ArenaStringPtr and MapFieldBase. Such constructors are internal
// implementation details of the library.
struct ConstantInitialized {
  explicit ConstantInitialized() = default;
};

// Tag type used to invoke the arena constructor overload of classes such
// as ExtensionSet and MapFieldLite in aggregate initialization. These
// classes typically don't have move/copy constructors, which rules out
// explicit initialization in pre-C++17.
struct ArenaInitialized {
  explicit ArenaInitialized() = default;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PORT_H__
