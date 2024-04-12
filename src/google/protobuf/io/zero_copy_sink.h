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

#ifndef GOOGLE_PROTOBUF_UTIL_ZERO_COPY_SINK_H__
#define GOOGLE_PROTOBUF_UTIL_ZERO_COPY_SINK_H__

#include <cstddef>

#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace io {
namespace zc_sink_internal {

// Internal helper class, for turning a ZeroCopyOutputStream into a sink.
class PROTOBUF_EXPORT ZeroCopyStreamByteSink {
 public:
  explicit ZeroCopyStreamByteSink(io::ZeroCopyOutputStream* stream)
      : stream_(stream) {}
  ZeroCopyStreamByteSink(const ZeroCopyStreamByteSink&) = delete;
  ZeroCopyStreamByteSink& operator=(const ZeroCopyStreamByteSink&) = delete;

  ~ZeroCopyStreamByteSink() {
    if (buffer_size_ > 0) {
      stream_->BackUp(buffer_size_);
    }
  }

  void Append(const char* bytes, size_t len);
  void Write(absl::string_view str) { Append(str.data(), str.size()); }

  size_t bytes_written() { return bytes_written_; }
  bool failed() { return failed_; }

 private:
  io::ZeroCopyOutputStream* stream_;
  void* buffer_ = nullptr;
  size_t buffer_size_ = 0;
  size_t bytes_written_ = 0;
  bool failed_ = false;
};
}  // namespace zc_sink_internal
}  // namespace io
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_UTIL_ZERO_COPY_SINK_H__
