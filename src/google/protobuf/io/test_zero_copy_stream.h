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

#ifndef GOOGLE_PROTOBUF_IO_TEST_ZERO_COPY_STREAM_H__
#define GOOGLE_PROTOBUF_IO_TEST_ZERO_COPY_STREAM_H__

#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/types/optional.h"
#include "google/protobuf/io/zero_copy_stream.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace io {
namespace internal {

// Input stream used for testing the proper handling of input fragmentation.
// It also will assert the preconditions of the methods.
class TestZeroCopyInputStream final : public ZeroCopyInputStream {
 public:
  // The input stream will provide the buffers exactly as passed here.
  explicit TestZeroCopyInputStream(const std::vector<std::string>& buffers)
      : buffers_(buffers.begin(), buffers.end()) {}

  // Allow copy to be able to inspect the stream without consuming the original.
  TestZeroCopyInputStream(const TestZeroCopyInputStream& other)
      : ZeroCopyInputStream(),
        buffers_(other.buffers_),
        last_returned_buffer_(other.last_returned_buffer_),
        byte_count_(other.byte_count_) {}

  bool Next(const void** data, int* size) override {
    ABSL_CHECK(data) << "data must not be null";
    ABSL_CHECK(size) << "size must not be null";
    last_returned_buffer_ = absl::nullopt;

    // We are done
    if (buffers_.empty()) return false;

    last_returned_buffer_ = std::move(buffers_.front());
    buffers_.pop_front();
    *data = last_returned_buffer_->data();
    *size = static_cast<int>(last_returned_buffer_->size());
    byte_count_ += *size;
    return true;
  }

  void BackUp(int count) override {
    ABSL_CHECK_GE(count, 0) << "count must not be negative";
    ABSL_CHECK(last_returned_buffer_.has_value())
        << "The last call was not a successful Next()";
    ABSL_CHECK_LE(count, last_returned_buffer_->size())
        << "count must be within bounds of last buffer";
    buffers_.push_front(
        last_returned_buffer_->substr(last_returned_buffer_->size() - count));
    last_returned_buffer_ = absl::nullopt;
    byte_count_ -= count;
  }

  bool Skip(int count) override {
    ABSL_CHECK_GE(count, 0) << "count must not be negative";
    last_returned_buffer_ = absl::nullopt;
    while (true) {
      if (count == 0) return true;
      if (buffers_.empty()) return false;
      auto& front = buffers_[0];
      int front_size = static_cast<int>(front.size());
      if (front_size <= count) {
        count -= front_size;
        byte_count_ += front_size;
        buffers_.pop_front();
      } else {
        // The front is enough, just chomp from it.
        front = front.substr(count);
        byte_count_ += count;
        return true;
      }
    }
  }

  int64_t ByteCount() const override { return byte_count_; }

 private:
  // For simplicity of implementation, we pop the elements from the from and
  // move them to `last_returned_buffer_`. It makes it simpler to keep track of
  // the state of the object. The extra cost is not relevant for testing.
  std::deque<std::string> buffers_;
  absl::optional<std::string> last_returned_buffer_;
  int64_t byte_count_ = 0;
};

}  // namespace internal
}  // namespace io
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_IO_TEST_ZERO_COPY_STREAM_H__
