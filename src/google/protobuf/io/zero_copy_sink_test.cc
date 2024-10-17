// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/io/zero_copy_sink.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace io {
namespace zc_sink_internal {
namespace {

class ChunkedString {
 public:
  explicit ChunkedString(absl::string_view data, size_t skipped_patterns)
      : data_(data), skipped_patterns_(skipped_patterns) {}

  // Returns the next chunk; empty if out of chunks.
  absl::string_view NextChunk() {
    if (pattern_bit_idx_ == data_.size()) {
      return "";
    }
    size_t start = pattern_bit_idx_;
    do {
      ++pattern_bit_idx_;
    } while (pattern_bit_idx_ < data_.size() &&
             (pattern_ >> pattern_bit_idx_ & 1) == 0);
    size_t end = pattern_bit_idx_;
    return data_.substr(start, end - start);
  }

  // Resets the stream and runs the next pattern of splits.
  bool NextPattern() {
    pattern_ += skipped_patterns_;
    if (pattern_ >= (1 << data_.size())) {
      return false;
    }
    pattern_bit_idx_ = 0;
    return true;
  }

  // prints out the pattern as a sequence of quoted strings.
  std::string PatternAsQuotedString() {
    std::string out;
    size_t start = 0;
    for (size_t i = 0; i <= data_.size(); ++i) {
      if (i == data_.size() || (pattern_ >> start & 1) != 0) {
        if (!out.empty()) {
          absl::StrAppend(&out, " ");
        }
        absl::StrAppend(
            &out, "\"",
            absl::CHexEscape(std::string{data_.substr(start, i - start)}),
            "\"");
        start = i;
      }
    }
    return out;
  }

 private:
  absl::string_view data_;
  size_t skipped_patterns_;
  // pattern_ is a bitset indicating at which indices we insert a seam.
  uint64_t pattern_ = 0;
  size_t pattern_bit_idx_ = 0;
};

class PatternedOutputStream : public io::ZeroCopyOutputStream {
 public:
  explicit PatternedOutputStream(ChunkedString data) : data_(data) {}

  bool Next(void** buffer, int* length) override {
    absl::string_view segment;
    if (!back_up_.empty()) {
      segment = back_up_.back();
      back_up_.pop_back();
    } else {
      segment_ = data_.NextChunk();
      segment = segment_;
    }

    if (segment_.empty()) {
      return false;
    }

    // TODO: This is only ever constructed in test code, and only
    // from non-const bytes, so this is a valid cast. We need to do this since
    // OSS proto does not yet have absl::Span; once we take a full Abseil
    // dependency we should use that here instead.
    *buffer = const_cast<char*>(segment.data());
    *length = static_cast<int>(segment.size());
    byte_count_ += static_cast<int64_t>(segment.size());
    return true;
  }

  void BackUp(int length) override {
    ABSL_CHECK(length <= static_cast<int>(segment_.size()));

    size_t backup = segment_.size() - static_cast<size_t>(length);
    back_up_.push_back(segment_.substr(backup));
    segment_ = segment_.substr(0, backup);
    byte_count_ -= static_cast<int64_t>(length);
  }

  int64_t ByteCount() const override { return byte_count_; }

 private:
  ChunkedString data_;
  absl::string_view segment_;

  std::vector<absl::string_view> back_up_;
  int64_t byte_count_ = 0;
};

class ZeroCopyStreamByteSinkTest : public testing::Test {
 protected:
  std::array<char, 10> output_{};
  absl::string_view output_view_{output_.data(), output_.size()};
  ChunkedString output_chunks_{output_view_, 7};
};

TEST_F(ZeroCopyStreamByteSinkTest, WriteExact) {
  do {
    SCOPED_TRACE(output_chunks_.PatternAsQuotedString());
    ChunkedString input("0123456789", 1);

    do {
      SCOPED_TRACE(input.PatternAsQuotedString());
      output_ = {};
      PatternedOutputStream output_stream(output_chunks_);
      ZeroCopyStreamByteSink byte_sink(&output_stream);
      SCOPED_TRACE(input.PatternAsQuotedString());
      absl::string_view chunk;
      while (!(chunk = input.NextChunk()).empty()) {
        byte_sink.Append(chunk.data(), chunk.size());
      }
    } while (input.NextPattern());

    ASSERT_EQ(output_view_, "0123456789");
  } while (output_chunks_.NextPattern());
}

TEST_F(ZeroCopyStreamByteSinkTest, WriteShort) {
  do {
    SCOPED_TRACE(output_chunks_.PatternAsQuotedString());
    ChunkedString input("012345678", 1);

    do {
      SCOPED_TRACE(input.PatternAsQuotedString());
      output_ = {};
      PatternedOutputStream output_stream(output_chunks_);
      ZeroCopyStreamByteSink byte_sink(&output_stream);
      SCOPED_TRACE(input.PatternAsQuotedString());
      absl::string_view chunk;
      while (!(chunk = input.NextChunk()).empty()) {
        byte_sink.Append(chunk.data(), chunk.size());
      }
    } while (input.NextPattern());

    ASSERT_EQ(output_view_, absl::string_view("012345678\0", 10));
  } while (output_chunks_.NextPattern());
}

TEST_F(ZeroCopyStreamByteSinkTest, WriteLong) {
  do {
    SCOPED_TRACE(output_chunks_.PatternAsQuotedString());
    ChunkedString input("0123456789A", 1);

    do {
      SCOPED_TRACE(input.PatternAsQuotedString());
      output_ = {};
      PatternedOutputStream output_stream(output_chunks_);
      ZeroCopyStreamByteSink byte_sink(&output_stream);
      SCOPED_TRACE(input.PatternAsQuotedString());
      absl::string_view chunk;
      while (!(chunk = input.NextChunk()).empty()) {
        byte_sink.Append(chunk.data(), chunk.size());
      }
    } while (input.NextPattern());

    ASSERT_EQ(output_view_, "0123456789");
  } while (output_chunks_.NextPattern());
}
}  // namespace
}  // namespace zc_sink_internal
}  // namespace io
}  // namespace protobuf
}  // namespace google
