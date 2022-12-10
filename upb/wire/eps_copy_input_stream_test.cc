#include "upb/wire/eps_copy_input_stream.h"

#include <string.h>

#include <string>

#include "gtest/gtest.h"
// begin:google_only
// #include "testing/fuzzing/fuzztest.h"
// end:google_only

namespace {

TEST(EpsCopyInputStreamTest, ZeroSize) {
  upb_EpsCopyInputStream stream;
  const char* ptr = NULL;
  upb_EpsCopyInputStream_Init(&stream, &ptr, 0);
  EXPECT_TRUE(upb_EpsCopyInputStream_IsDone(&stream, &ptr, NULL));
}

// begin:google_only
//
// // We create a simple, trivial implementation of the stream that we can test
// // our real implementation against.
//
// class FakeStream {
//  public:
//   FakeStream(const std::string& data) : data_(data), offset_(0) {
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
//   EpsStream(const std::string& data) : data_(data) {
//     ptr_ = data_.data();
//     upb_EpsCopyInputStream_Init(&eps_, &ptr_, data_.size());
//   }
//
//   // Returns false at EOF or error.
//   int ReadData(int n, std::string* data) {
//     // We want to verify that we can read kUpb_EpsCopyInputStream_SlopBytes
//     // safely, even if we haven't actually been requested to read that much.
//     // We copy to a global buffer so the copy can't be optimized away.
//     memcpy(&tmp_buf, ptr_, kUpb_EpsCopyInputStream_SlopBytes);
//     data->assign(tmp_buf, n);
//     ptr_ += n;
//
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
//   bool TryPushLimit(int limit) {
//     if (!upb_EpsCopyInputStream_CheckSize(&eps_, ptr_, limit)) return false;
//     deltas_.push_back(upb_EpsCopyInputStream_PushLimit(&eps_, ptr_, limit));
//     return true;
//   }
//
//   bool IsEof() const { return eof_; }
//
//  private:
//   bool IsAtLimit() {
//     return upb_EpsCopyInputStream_IsDone(&eps_, &ptr_,
//                                          &EpsStream::IsDoneFallback);
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
//   bool error_ = false;
//   bool eof_ = false;
// };
//
// // Reads N bytes from the given position.
// struct ReadOp {
//   int bytes;
// };
//
// // Pushes a new limit of N bytes from the current position.
// struct PushLimitOp {
//   int bytes;
// };
//
// typedef std::variant<ReadOp, PushLimitOp> Op;
//
// struct EpsCopyTestScript {
//   int data_size;
//   std::vector<Op> ops;
// };
//
// auto ArbitraryEpsCopyTestScript() {
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
//       VectorOf(VariantOf(
//           // ReadOp
//           StructOf<ReadOp>(InRange(0, kUpb_EpsCopyInputStream_SlopBytes)),
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
//   EpsStream eps_stream(data);
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
//     } else if (const PushLimitOp* push = std::get_if<PushLimitOp>(&op)) {
//       EXPECT_EQ(fake_stream.TryPushLimit(push->bytes),
//                 eps_stream.TryPushLimit(push->bytes));
//     }
//   }
// }
//
// FUZZ_TEST(EpsCopyFuzzTest, TestAgainstFakeStream)
//     .WithDomains(ArbitraryEpsCopyTestScript());
//
// TEST(EpsCopyFuzzTest, TestAgainstFakeStreamRegression) {
//   TestAgainstFakeStream({299,
//                          {
//                              PushLimitOp{2},
//                              ReadOp{14},
//                          }});
// }
//
// end:google_only

}  // namespace
