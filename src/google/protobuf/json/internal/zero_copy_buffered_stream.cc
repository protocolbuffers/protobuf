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

#include "google/protobuf/json/internal/zero_copy_buffered_stream.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {
absl::Status ZeroCopyBufferedStream::Advance(size_t bytes) {
  while (bytes != 0) {
    if (Unread().empty() && !ReadChunk()) {
      return absl::InvalidArgumentError("unexpected EOF");
    }
    size_t to_skip = std::min(bytes, Unread().size());
    cursor_ += to_skip;
    bytes -= to_skip;
  }

  if (using_buf_) {
    ABSL_DCHECK_LE(cursor_, buffer_start_ + buf_.size());
  } else {
    ABSL_DCHECK_LE(cursor_, last_chunk_.size());
  }

  return absl::OkStatus();
}

absl::StatusOr<BufferingGuard> ZeroCopyBufferedStream::BufferAtLeast(
    size_t bytes) {
  // This MUST be an empty guard before the first call to ReadChunk();
  // otherwise we risk unconditional buffering.
  BufferingGuard guard;
  while (Unread().size() < bytes) {
    if (!Unread().empty()) {
      // We must buffer before reading if Unread() is nonempty; otherwise we
      // risk discarding part of the unread buffer. However, we must NOT
      // buffer before calling ReadChunk if it *is* empty, because then we
      // would buffer unconditionally.
      //
      // There are tests to verify both of these cases.
      guard = BufferingGuard(this);
    }
    if (!ReadChunk()) {
      return absl::InvalidArgumentError("unexpected EOF");
    }
    guard = BufferingGuard(this);
  }
  ABSL_DCHECK_GE(Unread().size(), bytes);
  return BufferingGuard(this);
}

void ZeroCopyBufferedStream::DownRefBuffer() {
  ABSL_DCHECK_GT(outstanding_buffer_borrows_, 0);

  --outstanding_buffer_borrows_;
  if (outstanding_buffer_borrows_ > 0 || !using_buf_) {
    return;
  }

  // The "virtual length" is the size of the buffer cursor_ indexes into, which
  // is bigger than buf_.
  size_t virtual_buf_len = buf_.size() + buffer_start_;
  size_t last_chunk_in_buf = virtual_buf_len - last_chunk_.size();
  // If we are inside of `last_chunk_`, set the cursor there; otherwise, we have
  // a dangling reference somewhere.
  ABSL_DCHECK_LE(last_chunk_in_buf, virtual_buf_len) << absl::StrFormat(
      "%d, %d, %d", buf_.size(), last_chunk_.size(), buffer_start_);
  if (cursor_ <= last_chunk_in_buf) {
    cursor_ = 0;
  } else {
    cursor_ -= last_chunk_in_buf;
  }
  buf_.clear();
  using_buf_ = false;
}

bool ZeroCopyBufferedStream::ReadChunk() {
  if (eof_) {
    return false;
  }
  // We are buffering a second chunk, so we need to put the current chunk
  // into the buffer.
  if (outstanding_buffer_borrows_ > 0 && !using_buf_) {
    absl::c_copy(RawBuffer(buffer_start_), std::back_inserter(buf_));
    using_buf_ = true;
  }

  const void* data;
  int len;
  if (!stream_->Next(&data, &len)) {
    eof_ = true;
    return false;
  }

  last_chunk_ = absl::string_view(static_cast<const char*>(data),
                                  static_cast<size_t>(len));
  if (using_buf_) {
    absl::c_copy(last_chunk_, std::back_inserter(buf_));
    // Cursor does not need to move, because it is still inside of `buf_`.
  } else {
    cursor_ = 0;
    buffer_start_ = 0;
  }
  return true;
}
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google
