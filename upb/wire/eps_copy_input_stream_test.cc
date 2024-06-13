// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/eps_copy_input_stream.h"

#include <string.h>

#include <string>

#include <gtest/gtest.h>
#include "upb/mem/arena.hpp"
// begin:google_only
// #include "testing/fuzzing/fuzztest.h"
// end:google_only

namespace {

TEST(EpsCopyInputStreamTest, ZeroSize) {
  upb_EpsCopyInputStream stream;
  const char* ptr = nullptr;
  upb_EpsCopyInputStream_Init(&stream, &ptr, 0, false);
  EXPECT_TRUE(
      upb_EpsCopyInputStream_IsDoneWithCallback(&stream, &ptr, nullptr));
}

// begin:google_only
//
// // We create a simple, trivial implementation of the stream that we can test
// // our real implementation against.
//
// class FakeStream {
//  public:
//   explicit FakeStream(const std::string& data) : data_(data), offset_(0) {
//     limits_.push_back(data.size());
//   }
//
//   // If we reached one or more limits correctly, returns the number of limits
//   // ended.  If we tried to read beyond the current limit, returns -1.
//   // Otherwise, for simple success, returns 0.
//   int ReadData(int n, std::string* data) {
//     if (n > BytesUntilLimit()) return -1;
//
//     data->assign(data_.data() + offset_, n);
//     offset_ += n;
//
//     int end_limit_count = 0;
//
//     while (BytesUntilLimit() == 0) {
//       if (PopLimit()) {
//         end_limit_count++;
//       } else {
//         eof_ = true;
//         break;
//       }
//     }
//
//     return end_limit_count;
//   }
//
//   bool TryPushLimit(int limit) {
//     if (!CheckSize(limit)) return false;
//     limits_.push_back(offset_ + limit);
//     return true;
//   }
//
//   bool IsEof() const { return eof_; }
//
//  private:
//   int BytesUntilLimit() const { return limits_.back() - offset_; }
//   bool CheckSize(int size) const { return BytesUntilLimit() >= size; }
//
//   // Return false on EOF.
//   bool PopLimit() {
//     limits_.pop_back();
//     return !limits_.empty();
//   }
//
//   std::string data_;
//   // Limits, specified in absolute stream terms.
//   std::vector<int> limits_;
//   int offset_;
//   bool eof_ = false;
// };
//
// char tmp_buf[kUpb_EpsCopyInputStream_SlopBytes];
//
// class EpsStream {
//  public:
//   EpsStream(const std::string& data, bool enable_aliasing)
//       : data_(data), enable_aliasing_(enable_aliasing) {
//     ptr_ = data_.data();
//     upb_EpsCopyInputStream_Init(&eps_, &ptr_, data_.size(), enable_aliasing);
//   }
//
//   // Returns false at EOF or error.
//   int ReadData(int n, std::string* data) {
//     EXPECT_LE(n, kUpb_EpsCopyInputStream_SlopBytes);
//     if (enable_aliasing_) {
//       EXPECT_TRUE(upb_EpsCopyInputStream_AliasingAvailable(&eps_, ptr_, n));
//     }
//     // We want to verify that we can read kUpb_EpsCopyInputStream_SlopBytes
//     // safely, even if we haven't actually been requested to read that much.
//     // We copy to a global buffer so the copy can't be optimized away.
//     memcpy(&tmp_buf, ptr_, kUpb_EpsCopyInputStream_SlopBytes);
//     data->assign(tmp_buf, n);
//     ptr_ += n;
//     if (enable_aliasing_) {
//       EXPECT_TRUE(upb_EpsCopyInputStream_AliasingAvailable(&eps_, ptr_, 0));
//     }
//     return PopLimits();
//   }
//
//   int ReadString(int n, std::string* data) {
//     if (!upb_EpsCopyInputStream_CheckSize(&eps_, ptr_, n)) return -1;
//     const char* str_data = ptr_;
//     if (enable_aliasing_) {
//       EXPECT_TRUE(upb_EpsCopyInputStream_AliasingAvailable(&eps_, ptr_, n));
//     }
//     ptr_ = upb_EpsCopyInputStream_ReadString(&eps_, &str_data, n, arena_.ptr());
//     if (!ptr_) return -1;
//     if (enable_aliasing_ && n) {
//       EXPECT_GE(reinterpret_cast<uintptr_t>(str_data),
//                 reinterpret_cast<uintptr_t>(data_.data()));
//       EXPECT_LT(reinterpret_cast<uintptr_t>(str_data),
//                 reinterpret_cast<uintptr_t>(data_.data() + data_.size()));
//       EXPECT_TRUE(upb_EpsCopyInputStream_AliasingAvailable(&eps_, ptr_, 0));
//     }
//     data->assign(str_data, n);
//     return PopLimits();
//   }
//
//   bool TryPushLimit(int limit) {
//     if (!upb_EpsCopyInputStream_CheckSize(&eps_, ptr_, limit)) return false;
//     deltas_.push_back(upb_EpsCopyInputStream_PushLimit(&eps_, ptr_, limit));
//     return true;
//   }
//
//   bool IsEof() const { return eof_; }
//
//  private:
//   int PopLimits() {
//     int end_limit_count = 0;
//
//     while (IsAtLimit()) {
//       if (error_) return -1;
//       if (PopLimit()) {
//         end_limit_count++;
//       } else {
//         eof_ = true;  // EOF.
//         break;
//       }
//     }
//
//     return error_ ? -1 : end_limit_count;
//   }
//
//   bool IsAtLimit() {
//     return upb_EpsCopyInputStream_IsDoneWithCallback(
//         &eps_, &ptr_, &EpsStream::IsDoneFallback);
//   }
//
//   // Return false on EOF.
//   bool PopLimit() {
//     if (deltas_.empty()) return false;
//     upb_EpsCopyInputStream_PopLimit(&eps_, ptr_, deltas_.back());
//     deltas_.pop_back();
//     return true;
//   }
//
//   static const char* IsDoneFallback(upb_EpsCopyInputStream* e, const char* ptr,
//                                     int overrun) {
//     return _upb_EpsCopyInputStream_IsDoneFallbackInline(
//         e, ptr, overrun, &EpsStream::BufferFlipCallback);
//   }
//
//   static const char* BufferFlipCallback(upb_EpsCopyInputStream* e,
//                                         const char* old_end,
//                                         const char* new_start) {
//     EpsStream* stream = reinterpret_cast<EpsStream*>(e);
//     if (!old_end) stream->error_ = true;
//     return new_start;
//   }
//
//   upb_EpsCopyInputStream eps_;
//   std::string data_;
//   const char* ptr_;
//   std::vector<int> deltas_;
//   upb::Arena arena_;
//   bool error_ = false;
//   bool eof_ = false;
//   bool enable_aliasing_;
// };
//
// // Reads N bytes from the given position.
// struct ReadOp {
//   int bytes;  // Must be <= kUpb_EpsCopyInputStream_SlopBytes.
// };
//
// struct ReadStringOp {
//   int bytes;
// };
//
// // Pushes a new limit of N bytes from the current position.
// struct PushLimitOp {
//   int bytes;
// };
//
// typedef std::variant<ReadOp, ReadStringOp, PushLimitOp> Op;
//
// struct EpsCopyTestScript {
//   int data_size;
//   bool enable_aliasing;
//   std::vector<Op> ops;
// };
//
// auto ArbitraryEpsCopyTestScript() {
//   using ::fuzztest::Arbitrary;
//   using ::fuzztest::InRange;
//   using ::fuzztest::NonNegative;
//   using ::fuzztest::StructOf;
//   using ::fuzztest::VariantOf;
//   using ::fuzztest::VectorOf;
//
//   int max_data_size = 512;
//
//   return StructOf<EpsCopyTestScript>(
//       InRange(0, max_data_size),  // data_size
//       Arbitrary<bool>(),          // enable_aliasing
//       VectorOf(VariantOf(
//           // ReadOp
//           StructOf<ReadOp>(InRange(0, kUpb_EpsCopyInputStream_SlopBytes)),
//           // ReadStringOp
//           StructOf<ReadStringOp>(NonNegative<int>()),
//           // PushLimitOp
//           StructOf<PushLimitOp>(NonNegative<int>()))));
// }
//
// // Run a test that creates both real stream and a fake stream, and validates
// // that they have the same behavior.
// void TestAgainstFakeStream(const EpsCopyTestScript& script) {
//   std::string data(script.data_size, 'x');
//   for (int i = 0; i < script.data_size; ++i) {
//     data[i] = static_cast<char>(i & 0xff);
//   }
//
//   FakeStream fake_stream(data);
//   EpsStream eps_stream(data, script.enable_aliasing);
//
//   for (const auto& op : script.ops) {
//     if (const ReadOp* read_op = std::get_if<ReadOp>(&op)) {
//       std::string data_fake;
//       std::string data_eps;
//       int fake_result = fake_stream.ReadData(read_op->bytes, &data_fake);
//       int eps_result = eps_stream.ReadData(read_op->bytes, &data_eps);
//       EXPECT_EQ(fake_result, eps_result);
//       if (fake_result == -1) break;  // Error
//       EXPECT_EQ(data_fake, data_eps);
//       EXPECT_EQ(fake_stream.IsEof(), eps_stream.IsEof());
//       if (fake_stream.IsEof()) break;
//     } else if (const ReadStringOp* read_op = std::get_if<ReadStringOp>(&op)) {
//       std::string data_fake;
//       std::string data_eps;
//       int fake_result = fake_stream.ReadData(read_op->bytes, &data_fake);
//       int eps_result = eps_stream.ReadString(read_op->bytes, &data_eps);
//       EXPECT_EQ(fake_result, eps_result);
//       if (fake_result == -1) break;  // Error
//       EXPECT_EQ(data_fake, data_eps);
//       EXPECT_EQ(fake_stream.IsEof(), eps_stream.IsEof());
//       if (fake_stream.IsEof()) break;
//     } else if (const PushLimitOp* push = std::get_if<PushLimitOp>(&op)) {
//       EXPECT_EQ(fake_stream.TryPushLimit(push->bytes),
//                 eps_stream.TryPushLimit(push->bytes));
//     } else {
//       EXPECT_TRUE(false);  // Unknown op.
//     }
//   }
// }
//
// // Test with:
// //  $ blaze run --config=fuzztest third_party/upb:eps_copy_input_stream_test \
// //   -- --gunit_fuzz=
// FUZZ_TEST(EpsCopyFuzzTest, TestAgainstFakeStream)
//     .WithDomains(ArbitraryEpsCopyTestScript());
//
// TEST(EpsCopyFuzzTest, TestAgainstFakeStreamRegression) {
//   TestAgainstFakeStream({299,
//                          false,
//                          {
//                              PushLimitOp{2},
//                              ReadOp{14},
//                          }});
// }
//
// TEST(EpsCopyFuzzTest, AliasingEnabledZeroSizeReadString) {
//   TestAgainstFakeStream({510, true, {ReadStringOp{0}}});
// }
//
// TEST(EpsCopyFuzzTest, AliasingDisabledZeroSizeReadString) {
//   TestAgainstFakeStream({510, false, {ReadStringOp{0}}});
// }
//
// TEST(EpsCopyFuzzTest, ReadStringZero) {
//   TestAgainstFakeStream({0, true, {ReadStringOp{0}}});
// }
//
// TEST(EpsCopyFuzzTest, ReadZero) {
//   TestAgainstFakeStream({0, true, {ReadOp{0}}});
// }
//
// TEST(EpsCopyFuzzTest, ReadZeroTwice) {
//   TestAgainstFakeStream({0, true, {ReadOp{0}, ReadOp{0}}});
// }
//
// TEST(EpsCopyFuzzTest, ReadStringZeroThenRead) {
//   TestAgainstFakeStream({0, true, {ReadStringOp{0}, ReadOp{0}}});
// }
//
// TEST(EpsCopyFuzzTest, ReadStringOverflowsBufferButNotLimit) {
//   TestAgainstFakeStream({351,
//                          false,
//                          {
//                              ReadOp{7},
//                              PushLimitOp{2147483647},
//                              ReadStringOp{344},
//                          }});
// }
//
// TEST(EpsCopyFuzzTest, LastBufferAliasing) {
//   TestAgainstFakeStream({27, true, {ReadOp{12}, ReadStringOp{3}}});
// }
//
// TEST(EpsCopyFuzzTest, FirstBufferAliasing) {
//   TestAgainstFakeStream({7, true, {ReadStringOp{3}}});
// }
//
// end:google_only

}  // namespace
