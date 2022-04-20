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

#ifndef GOOGLE_PROTOBUF_ENDIAN_H__
#define GOOGLE_PROTOBUF_ENDIAN_H__

#if defined(_MSC_VER)
#include <stdlib.h>
#endif

#include <cstdint>

// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {

inline uint64_t BSwap64(uint64_t host_int) {
#if defined(PROTOBUF_BUILTIN_BSWAP64)
  return PROTOBUF_BUILTIN_BSWAP64(host_int);
#elif defined(_MSC_VER)
  return _byteswap_uint64(host_int);
#else
  return (((host_int & uint64_t{0xFF}) << 56) |
          ((host_int & uint64_t{0xFF00}) << 40) |
          ((host_int & uint64_t{0xFF0000}) << 24) |
          ((host_int & uint64_t{0xFF000000}) << 8) |
          ((host_int & uint64_t{0xFF00000000}) >> 8) |
          ((host_int & uint64_t{0xFF0000000000}) >> 24) |
          ((host_int & uint64_t{0xFF000000000000}) >> 40) |
          ((host_int & uint64_t{0xFF00000000000000}) >> 56));
#endif
}

inline uint32_t BSwap32(uint32_t host_int) {
#if defined(PROTOBUF_BUILTIN_BSWAP32)
  return PROTOBUF_BUILTIN_BSWAP32(host_int);
#elif defined(_MSC_VER)
  return _byteswap_ulong(host_int);
#else
  return (((host_int & uint32_t{0xFF}) << 24) |
          ((host_int & uint32_t{0xFF00}) << 8) |
          ((host_int & uint32_t{0xFF0000}) >> 8) |
          ((host_int & uint32_t{0xFF000000}) >> 24));
#endif
}

inline uint16_t BSwap16(uint16_t host_int) {
#if defined(PROTOBUF_BUILTIN_BSWAP16)
  return PROTOBUF_BUILTIN_BSWAP16(host_int);
#elif defined(_MSC_VER)
  return _byteswap_ushort(host_int);
#else
  return (((host_int & uint16_t{0xFF}) << 8) |
          ((host_int & uint16_t{0xFF00}) >> 8));
#endif
}

namespace little_endian {

inline uint16_t FromHost(uint16_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return BSwap16(value);
#else
  return value;
#endif
}

inline uint32_t FromHost(uint32_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return BSwap32(value);
#else
  return value;
#endif
}

inline uint64_t FromHost(uint64_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return BSwap64(value);
#else
  return value;
#endif
}

inline uint16_t ToHost(uint16_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return BSwap16(value);
#else
  return value;
#endif
}

inline uint32_t ToHost(uint32_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return BSwap32(value);
#else
  return value;
#endif
}

inline uint64_t ToHost(uint64_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return BSwap64(value);
#else
  return value;
#endif
}

}  // namespace little_endian

namespace big_endian {

inline uint16_t FromHost(uint16_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return value;
#else
  return BSwap16(value);
#endif
}

inline uint32_t FromHost(uint32_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return value;
#else
  return BSwap32(value);
#endif
}

inline uint64_t FromHost(uint64_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return value;
#else
  return BSwap64(value);
#endif
}

inline uint16_t ToHost(uint16_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return value;
#else
  return BSwap16(value);
#endif
}

inline uint32_t ToHost(uint32_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return value;
#else
  return BSwap32(value);
#endif
}

inline uint64_t ToHost(uint64_t value) {
#if defined(PROTOBUF_BIG_ENDIAN)
  return value;
#else
  return BSwap64(value);
#endif
}

}  // namespace big_endian

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_ENDIAN_H__
