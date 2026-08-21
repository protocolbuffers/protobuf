// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
