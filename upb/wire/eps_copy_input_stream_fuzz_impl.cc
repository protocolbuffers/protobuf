// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/eps_copy_input_stream_fuzz_impl.h"

#include <cstring>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"
#include "upb/wire/eps_copy_input_stream.h"

namespace {

// We create a simple, trivial implementation of the stream that we can test
// our real implementation against.

class FakeStream {
 public:
  explicit FakeStream(const std::string& data) : data_(data), offset_(0) {
    limits_.push_back(data.size());
  }

  // If we reached one or more limits correctly, returns the number of limits
  // ended.  If we tried to read beyond the current limit, returns -1.
  // Otherwise, for simple success, returns 0.
  int ReadData(int n, std::string* data) {
    if (n > BytesUntilLimit()) return -1;

    data->assign(data_.data() + offset_, n);
    offset_ += n;

    int end_limit_count = 0;

    while (BytesUntilLimit() == 0) {
      if (PopLimit()) {
        end_limit_count++;
      } else {
        eof_ = true;
        break;
      }
    }

    return end_limit_count;
  }

  bool TryPushLimit(int limit) {
    if (limit > BytesUntilLimit()) return false;
    limits_.push_back(offset_ + limit);
    return true;
  }

  bool IsEof() const { return eof_; }

 private:
  int BytesUntilLimit() const { return limits_.back() - offset_; }

  // Return false on EOF.
  bool PopLimit() {
    limits_.pop_back();
    return !limits_.empty();
  }

  std::string data_;
  // Limits, specified in absolute stream terms.
  std::vector<int> limits_;
  int offset_;
  bool eof_ = false;
};

char tmp_buf[kUpb_EpsCopyInputStream_SlopBytes];

class EpsStream {
 public:
  explicit EpsStream(const std::string& data) : data_(data) {
    ptr_ = data_.data();
    upb_EpsCopyInputStream_Init(&eps_, &ptr_, data_.size());
  }

  // Returns false at EOF or error.
  int ReadData(int n, std::string* data) {
    EXPECT_LE(n, kUpb_EpsCopyInputStream_SlopBytes);
    // We want to verify that we can read kUpb_EpsCopyInputStream_SlopBytes
    // safely, even if we haven't actually been requested to read that much.
    // We copy to a global buffer so the copy can't be optimized away.
    memcpy(&tmp_buf, ptr_, kUpb_EpsCopyInputStream_SlopBytes);
    data->assign(tmp_buf, n);
    ptr_ += n;
    return PopLimits();
  }

  int ReadString(int n, std::string* data) {
    upb_StringView sv;
    ptr_ = upb_EpsCopyInputStream_ReadStringEphemeral(&eps_, ptr_, n, &sv);
    if (!ptr_) return -1;
    EXPECT_EQ(sv.size, n);
    data->assign(sv.data, sv.size);
    return PopLimits();
  }

  int ReadStringAlias(int n, std::string* data) {
    upb_StringView sv;
    ptr_ = upb_EpsCopyInputStream_ReadStringAlwaysAlias(&eps_, ptr_, n, &sv);
    if (!ptr_) return -1;
    if (n) {
      EXPECT_GE(reinterpret_cast<uintptr_t>(sv.data),
                reinterpret_cast<uintptr_t>(data_.data()));
      EXPECT_LT(reinterpret_cast<uintptr_t>(sv.data),
                reinterpret_cast<uintptr_t>(data_.data() + data_.size()));
    }
    data->assign(sv.data, n);
    return PopLimits();
  }

  bool TryPushLimit(int limit) {
    deltas_.push_back(upb_EpsCopyInputStream_PushLimit(&eps_, ptr_, limit));
    return deltas_.back() >= 0;
  }

  bool IsEof() const { return eof_; }

 private:
  int PopLimits() {
    int end_limit_count = 0;

    while (upb_EpsCopyInputStream_IsDone(&eps_, &ptr_)) {
      if (upb_EpsCopyInputStream_IsError(&eps_)) return -1;
      if (PopLimit()) {
        end_limit_count++;
      } else {
        eof_ = true;  // EOF.
        break;
      }
    }

    return upb_EpsCopyInputStream_IsError(&eps_) ? -1 : end_limit_count;
  }

  bool IsAtLimit() { return upb_EpsCopyInputStream_IsDone(&eps_, &ptr_); }

  // Return false on EOF.
  bool PopLimit() {
    if (deltas_.empty()) return false;
    upb_EpsCopyInputStream_PopLimit(&eps_, ptr_, deltas_.back());
    deltas_.pop_back();
    return true;
  }

  upb_EpsCopyInputStream eps_;
  std::string data_;
  const char* ptr_;
  std::vector<ptrdiff_t> deltas_;
  upb::Arena arena_;
  bool eof_ = false;
};

}  // namespace

void TestAgainstFakeStream(const EpsCopyTestScript& script) {
  std::string data(script.data_size, 'x');
  for (int i = 0; i < script.data_size; ++i) {
    data[i] = static_cast<char>(i & 0xff);
  }

  FakeStream fake_stream(data);
  EpsStream eps_stream(data);

  for (const auto& op : script.ops) {
    if (const ReadOp* read_op = std::get_if<ReadOp>(&op)) {
      std::string data_fake;
      std::string data_eps;
      int fake_result = fake_stream.ReadData(read_op->bytes, &data_fake);
      int eps_result = eps_stream.ReadData(read_op->bytes, &data_eps);
      EXPECT_EQ(fake_result, eps_result);
      if (fake_result == -1) break;  // Error
      EXPECT_EQ(data_fake, data_eps);
      EXPECT_EQ(fake_stream.IsEof(), eps_stream.IsEof());
      if (fake_stream.IsEof()) break;
    } else if (const ReadStringOp* read_op = std::get_if<ReadStringOp>(&op)) {
      std::string data_fake;
      std::string data_eps;
      int fake_result = fake_stream.ReadData(read_op->bytes, &data_fake);
      int eps_result = eps_stream.ReadString(read_op->bytes, &data_eps);
      EXPECT_EQ(fake_result, eps_result);
      if (fake_result == -1) break;  // Error
      EXPECT_EQ(data_fake, data_eps);
      EXPECT_EQ(fake_stream.IsEof(), eps_stream.IsEof());
      if (fake_stream.IsEof()) break;
    } else if (const ReadStringAliasOp* read_op =
                   std::get_if<ReadStringAliasOp>(&op)) {
      std::string data_fake;
      std::string data_eps;
      int fake_result = fake_stream.ReadData(read_op->bytes, &data_fake);
      int eps_result = eps_stream.ReadStringAlias(read_op->bytes, &data_eps);
      EXPECT_EQ(fake_result, eps_result);
      if (fake_result == -1) break;  // Error
      EXPECT_EQ(data_fake, data_eps);
      EXPECT_EQ(fake_stream.IsEof(), eps_stream.IsEof());
      if (fake_stream.IsEof()) break;
    } else if (const PushLimitOp* push = std::get_if<PushLimitOp>(&op)) {
      bool fake_result = fake_stream.TryPushLimit(push->bytes);
      bool eps_result = eps_stream.TryPushLimit(push->bytes);
      EXPECT_EQ(fake_result, eps_result);
      if (!fake_result) break;  // Error
    } else {
      EXPECT_TRUE(false);  // Unknown op.
    }
  }
}
