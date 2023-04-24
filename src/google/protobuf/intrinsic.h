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

#ifndef GOOGLE_PROTOBUF_INTRINSIC_H__
#define GOOGLE_PROTOBUF_INTRINSIC_H__

#include <cstddef>

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

// Set bit `bit_index` in `destination`, using `Unit` as the integral type that
// exists in that location.
// `destination` does not need to be a of type `Unit`. The offset can point into
// a suboject of `destination`. Eg one obtained via `offsetof`.
template <typename Unit, typename T>
inline void BitSet(T* destination, size_t bit_index) {
#if defined(__x86_64__) && defined(__GNUC__)
  asm("bts %1, %0\n" : "+m"(*destination) : "r"(bit_index));
#else
  constexpr size_t bits_per_unit = 8 * sizeof(Unit);
  Unit& unit =
      *reinterpret_cast<Unit*>(reinterpret_cast<char*>(destination) +
                               bit_index / bits_per_unit * sizeof(Unit));
  unit |= Unit{1} << (bit_index % bits_per_unit);
#endif
}

// Test bit `bit_index` in `destination`, using `Unit` as the integral type that
// exists in that location.
// `destination` does not need to be a of type `Unit`. The offset can point into
// a suboject of `destination`. Eg one obtained via `offsetof`.
template <typename Unit, typename T>
inline bool BitTest(const T& source, size_t bit_index) {
#if defined(__x86_64__) && defined(__GNUC__) && \
    defined(__GCC_ASM_FLAG_OUTPUTS__)
  bool result;
  asm("bt %1, %2" : "=@ccc"(result) : "r"(bit_index), "m"(source));
  return result;
#else
  constexpr size_t bits_per_unit = 8 * sizeof(Unit);
  const Unit& unit =
      *reinterpret_cast<const Unit*>(reinterpret_cast<const char*>(&source) +
                                     bit_index / bits_per_unit * sizeof(Unit));
  return (unit & (1 << (bit_index % bits_per_unit))) != 0;
#endif
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_INTRINSIC_H__
