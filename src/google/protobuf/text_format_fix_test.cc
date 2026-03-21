#include <gtest/gtest.h>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace protobuf {

class FailureCounterStream : public google::protobuf::io::ZeroCopyOutputStream {
 public:
  explicit FailureCounterStream(int limit) : limit_(limit) {}

  bool Next(void** data, int* size) override {
    if (total_bytes_ >= limit_) {
      // Stream has "failed". Count how many times the Printer ignores this.
      attempts_after_failure_++;
      return false;
    }

    // Provide a tiny 1-byte buffer to force frequent Next() calls
    *data = buffer_;
    *size = 1;
    total_bytes_ += 1;
    return true;
  }

  void BackUp(int count) override { total_bytes_ -= count; }
  int64_t ByteCount() const override { return total_bytes_; }

  int attempts_after_failure() const { return attempts_after_failure_; }

 private:
  int limit_;
  int total_bytes_ = 0;
  int attempts_after_failure_ = 0;
  char buffer_[1];
};

TEST(TextFormatFixTest, ProveZeroWaste) {
  DescriptorProto message;
  const int total_fields = 1000;
  // Use a fixed-length string so we can predict the byte size
  std::string long_name = "THIS_IS_A_LONG_FIELD_NAME_32_CHARS_";

  for (int i = 0; i < total_fields; ++i) {
    message.add_reserved_name(long_name);
  }

  // We'll fail after 64 bytes.
  // This is enough for 2 fields.
  int failure_limit = 64;
  FailureCounterStream counter_stream(failure_limit);
  TextFormat::Printer printer;

  EXPECT_FALSE(printer.Print(message, &counter_stream));

  int64_t final_byte_count = counter_stream.ByteCount();

  // We are expecting our two strings to be written and to fill our limit
  // completely.
  EXPECT_EQ(final_byte_count, 64) << "Waste detected! Printer processed too "
                                     "much data after stream failure.";

  // We expect the third write to be our final write. It will increment
  // "attempts after failure" one time.
  EXPECT_EQ(counter_stream.attempts_after_failure(), 1)
      << "The printer tried to call Next() too many times after failure.";
}

}  // namespace protobuf
}  // namespace google