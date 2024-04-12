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

// Author: kenton@google.com (Kenton Varda)

#include "google/protobuf/stubs/common.h"

#include <errno.h>
#include <stdio.h>

#include <atomic>
#include <sstream>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // We only need minimal includes
#endif
#include <windows.h>
#define snprintf _snprintf    // see comment in strutil.cc
#endif
#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/stubs/callback.h"

// Must be last.
#include "google/protobuf/port_def.inc"  // NOLINT

namespace google {
namespace protobuf {

namespace internal {

void VerifyVersion(int headerVersion,
                   int minLibraryVersion,
                   const char* filename) {
  if (GOOGLE_PROTOBUF_VERSION < minLibraryVersion) {
    // Library is too old for headers.
    ABSL_LOG(FATAL)
        << "This program requires version " << VersionString(minLibraryVersion)
        << " of the Protocol Buffer runtime library, but the installed version "
           "is "
        << VersionString(GOOGLE_PROTOBUF_VERSION)
        << ".  Please update "
           "your library.  If you compiled the program yourself, make sure "
           "that "
           "your headers are from the same version of Protocol Buffers as your "
           "link-time library.  (Version verification failed in \""
        << filename << "\".)";
  }
  if (headerVersion < kMinHeaderVersionForLibrary) {
    // Headers are too old for library.
    ABSL_LOG(FATAL)
        << "This program was compiled against version "
        << VersionString(headerVersion)
        << " of the Protocol Buffer runtime "
           "library, which is not compatible with the installed version ("
        << VersionString(GOOGLE_PROTOBUF_VERSION)
        << ").  Contact the program "
           "author for an update.  If you compiled the program yourself, make "
           "sure that your headers are from the same version of Protocol "
           "Buffers "
           "as your link-time library.  (Version verification failed in \""
        << filename << "\".)";
  }
}

std::string VersionString(int version) {
  int major = version / 1000000;
  int minor = (version / 1000) % 1000;
  int micro = version % 1000;

  // 128 bytes should always be enough, but we use snprintf() anyway to be
  // safe.
  char buffer[128];
  snprintf(buffer, sizeof(buffer), "%d.%d.%d", major, minor, micro);

  // Guard against broken MSVC snprintf().
  buffer[sizeof(buffer)-1] = '\0';

  return buffer;
}

std::string ProtocVersionString(int version) {
  int minor = (version / 1000) % 1000;
  int micro = version % 1000;

  // 128 bytes should always be enough, but we use snprintf() anyway to be
  // safe.
  char buffer[128];
  snprintf(buffer, sizeof(buffer), "%d.%d", minor, micro);

  // Guard against broken MSVC snprintf().
  buffer[sizeof(buffer) - 1] = '\0';

  return buffer;
}

}  // namespace internal


// ===================================================================
// emulates google3/base/callback.cc

Closure::~Closure() {}

namespace internal { FunctionClosure0::~FunctionClosure0() {} }

void DoNothing() {}

// ===================================================================
// emulates google3/util/endian/endian.h
//
// TODO(xiaofeng): PROTOBUF_LITTLE_ENDIAN is unfortunately defined in
// google/protobuf/io/coded_stream.h and therefore can not be used here.
// Maybe move that macro definition here in the future.
uint32_t ghtonl(uint32_t x) {
  union {
    uint32_t result;
    uint8_t result_array[4];
  };
  result_array[0] = static_cast<uint8_t>(x >> 24);
  result_array[1] = static_cast<uint8_t>((x >> 16) & 0xFF);
  result_array[2] = static_cast<uint8_t>((x >> 8) & 0xFF);
  result_array[3] = static_cast<uint8_t>(x & 0xFF);
  return result;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"  // NOLINT
