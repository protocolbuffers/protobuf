// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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

  // If we have hit EOF then that means we might be buffering one or more
  // chunks of data that we have not yet logically advanced through. We need to
  // leave the buffer in place to ensure that we do not inadvertently drop such
  // chunks.
  if (eof_) {
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

#include "google/protobuf/port_undef.inc"
