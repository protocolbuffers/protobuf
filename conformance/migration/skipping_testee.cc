// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// A conformance testee that answers every request with `skipped`.
//
// Used by the request-golden test in this package, which pins down *which*
// requests the conformance runner sends.  A testee that never parses or
// serializes anything guarantees the golden cannot depend on any real
// implementation's behaviour.  It deliberately has no dependencies.
//
// Wire protocol (see conformance_test_runner.cc): the runner writes a 4-byte
// little-endian length followed by a serialized ConformanceRequest; the testee
// answers with a 4-byte little-endian length followed by a serialized
// ConformanceResponse, and loops until EOF on stdin.

#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include "absl/strings/string_view.h"

namespace {

// The complete framed response, which never changes:
//   * 4-byte little-endian length (11), then
//   * serialized `conformance.ConformanceResponse{skipped: "recording"}`:
//     field 5 (`skipped`), wire type 2 (length-delimited) => tag byte 0x2a,
//     followed by the length (9) and the bytes of "recording".
// "recording" is a separate adjacent literal so that the preceding hex escape
// cannot swallow its leading letters.
constexpr char kFramedResponseChars[] =
    "\x0b\0\0\0\x2a\x09"
    "recording";
constexpr absl::string_view kFramedResponse(kFramedResponseChars,
                                            sizeof(kFramedResponseChars) - 1);
static_assert(kFramedResponse.size() == 4 + 2 + 9,
              "framed response has unexpected size");
static_assert(static_cast<unsigned char>(kFramedResponse[0]) ==
                  kFramedResponse.size() - 4,
              "length prefix does not match body size");

enum class ReadStatus {
  kOk,         // All `len` bytes were read.
  kCleanEof,   // EOF before any byte was read.
  kTruncated,  // EOF after some bytes were read, or a read error.
};

// Reads exactly `len` bytes, retrying on EINTR.
ReadStatus ReadFull(int fd, char* buf, size_t len) {
  size_t total = 0;
  while (total < len) {
    ssize_t n = read(fd, buf + total, len - total);
    if (n < 0 && errno == EINTR) continue;
    if (n < 0) return ReadStatus::kTruncated;
    if (n == 0) {
      return total == 0 ? ReadStatus::kCleanEof : ReadStatus::kTruncated;
    }
    total += static_cast<size_t>(n);
  }
  return ReadStatus::kOk;
}

// Writes exactly `len` bytes, retrying on EINTR.  Returns false on error.
bool WriteFull(int fd, const char* buf, size_t len) {
  while (len > 0) {
    ssize_t n = write(fd, buf, len);
    if (n < 0 && errno == EINTR) continue;
    if (n <= 0) return false;
    len -= static_cast<size_t>(n);
    buf += n;
  }
  return true;
}

}  // namespace

int main() {
  std::string request;
  while (true) {
    unsigned char len_bytes[4];
    switch (ReadFull(STDIN_FILENO, reinterpret_cast<char*>(len_bytes),
                     sizeof(len_bytes))) {
      case ReadStatus::kOk:
        break;
      case ReadStatus::kCleanEof:
        return 0;  // The runner closed its end between frames: we are done.
      case ReadStatus::kTruncated:
        std::fputs("skipping_testee: truncated request length prefix\n",
                   stderr);
        return 1;
    }
    const uint32_t len = static_cast<uint32_t>(len_bytes[0]) |
                         static_cast<uint32_t>(len_bytes[1]) << 8 |
                         static_cast<uint32_t>(len_bytes[2]) << 16 |
                         static_cast<uint32_t>(len_bytes[3]) << 24;
    request.resize(len);
    // A frame with a length prefix but no (complete) body is always an error,
    // even if EOF arrives before the first body byte.
    if (ReadFull(STDIN_FILENO, request.data(), len) != ReadStatus::kOk) {
      std::fputs("skipping_testee: truncated request body\n", stderr);
      return 1;
    }
    if (!WriteFull(STDOUT_FILENO, kFramedResponse.data(),
                   kFramedResponse.size())) {
      std::fputs("skipping_testee: broken pipe to test runner\n", stderr);
      return 1;
    }
  }
}
