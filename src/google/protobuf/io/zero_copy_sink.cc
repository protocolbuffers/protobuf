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

#include "google/protobuf/io/zero_copy_sink.h"

#include <algorithm>
#include <cstddef>

namespace google {
namespace protobuf {
namespace io {
namespace zc_sink_internal {
void ZeroCopyStreamByteSink::Append(const char* bytes, size_t len) {
  while (!failed_ && len > 0) {
    if (buffer_size_ == 0) {
      int size;
      if (!stream_->Next(&buffer_, &size)) {
        // There isn't a way for ByteSink to report errors.
        buffer_size_ = 0;
        failed_ = true;
        return;
      }
      buffer_size_ = static_cast<unsigned int>(size);
    }

    auto to_write = std::min(len, buffer_size_);
    memcpy(buffer_, bytes, to_write);

    buffer_ = static_cast<char*>(buffer_) + to_write;
    buffer_size_ -= to_write;

    bytes += to_write;
    len -= to_write;

    bytes_written_ += to_write;
  }
}
}  // namespace zc_sink_internal
}  // namespace io
}  // namespace protobuf
}  // namespace google
