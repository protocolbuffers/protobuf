// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/wire_format.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include "absl/base/casts.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_protos/lazy_field_test.pb.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"
#include "google/protobuf/unittest_mset.pb.h"
#include "google/protobuf/unittest_mset_wire_format.pb.h"
#include "google/protobuf/unittest_proto3_arena.pb.h"
#include "google/protobuf/wire_format_lite.h"
#include "google/protobuf/wire_format_unittest.h"

#include <gtest/gtest.h>

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {

PROTOBUF_INSTANTIATE_WIRE_FORMAT_UNITTEST(, );

TEST(RepeatedVarint, Int32) {
  RepeatedField<int32_t> v;

  // Insert -2^n, 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(-(1 << n));
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar Int32Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::Int32Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::Int32Size(v));
}

TEST(RepeatedVarint, Int32Overflow) {
#if defined(__aarch64__)
  GTEST_SKIP() << "Skipping test on aarch64 (unvectorized loop times out)";
#endif
  if (sizeof(size_t) < 8) {
    GTEST_SKIP() << "Skipping test on 32-bit platform";
  }

  RepeatedField<int32_t> v;
  constexpr size_t kNumElements = std::numeric_limits<uint32_t>::max() / 10 + 1;
  // Each -1 will be 10 bytes.
  v.resize(kNumElements, -1);
  EXPECT_EQ(kNumElements * 10, WireFormatLite::Int32Size(v));
}

TEST(RepeatedVarint, Int64) {
  RepeatedField<int64_t> v;

  // Insert -2^n, 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(-(1 << n));
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar Int64Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::Int64Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::Int64Size(v));
}

TEST(RepeatedVarint, SInt32) {
  RepeatedField<int32_t> v;

  // Insert -2^n, 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(-(1 << n));
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar SInt32Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::SInt32Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::SInt32Size(v));
}

TEST(RepeatedVarint, SInt64) {
  RepeatedField<int64_t> v;

  // Insert -2^n, 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(-(1 << n));
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar SInt64Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::SInt64Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::SInt64Size(v));
}

TEST(RepeatedVarint, UInt32) {
  RepeatedField<uint32_t> v;

  // Insert 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar UInt32Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::UInt32Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::UInt32Size(v));
}

TEST(RepeatedVarint, UInt64) {
  RepeatedField<uint64_t> v;

  // Insert 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar UInt64Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::UInt64Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::UInt64Size(v));
}

TEST(RepeatedVarint, Enum) {
  RepeatedField<int> v;

  // Insert 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar EnumSize.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::EnumSize(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::EnumSize(v));
}

TEST(WireFormatTest, EnumsInSync) {
  // Verify that WireFormatLite::FieldType and WireFormatLite::CppType match
  // FieldDescriptor::Type and FieldDescriptor::CppType.

  EXPECT_EQ(absl::implicit_cast<int>(FieldDescriptor::MAX_TYPE),
            absl::implicit_cast<int>(WireFormatLite::MAX_FIELD_TYPE));
  EXPECT_EQ(absl::implicit_cast<int>(FieldDescriptor::MAX_CPPTYPE),
            absl::implicit_cast<int>(WireFormatLite::MAX_CPPTYPE));

  for (int i = 1; i <= WireFormatLite::MAX_FIELD_TYPE; i++) {
    EXPECT_EQ(absl::implicit_cast<int>(FieldDescriptor::TypeToCppType(
                  static_cast<FieldDescriptor::Type>(i))),
              absl::implicit_cast<int>(WireFormatLite::FieldTypeToCppType(
                  static_cast<WireFormatLite::FieldType>(i))));
  }
}

TEST(WireFormatTest, MaxFieldNumber) {
  // Make sure the max field number constant is accurate.
  EXPECT_EQ((1 << (32 - WireFormatLite::kTagTypeBits)) - 1,
            FieldDescriptor::kMaxNumber);
}

TEST(WireFormatTest, CppTypeForWorksForAllSupportedTypes) {
  using WFL = WireFormatLite;
  EXPECT_EQ(WFL::CppTypeFor<int32_t>(), WFL::CPPTYPE_INT32);
  EXPECT_EQ(WFL::CppTypeFor<int64_t>(), WFL::CPPTYPE_INT64);
  EXPECT_EQ(WFL::CppTypeFor<uint32_t>(), WFL::CPPTYPE_UINT32);
  EXPECT_EQ(WFL::CppTypeFor<uint64_t>(), WFL::CPPTYPE_UINT64);
  EXPECT_EQ(WFL::CppTypeFor<float>(), WFL::CPPTYPE_FLOAT);
  EXPECT_EQ(WFL::CppTypeFor<double>(), WFL::CPPTYPE_DOUBLE);
  EXPECT_EQ(WFL::CppTypeFor<bool>(), WFL::CPPTYPE_BOOL);
  EXPECT_EQ(WFL::CppTypeFor<proto2_unittest::TestAllTypes::NestedEnum>(),
            WFL::CPPTYPE_ENUM);
  EXPECT_EQ(WFL::CppTypeFor<std::string>(), WFL::CPPTYPE_STRING);
  EXPECT_EQ(WFL::CppTypeFor<absl::Cord>(), WFL::CPPTYPE_STRING);
  EXPECT_EQ(WFL::CppTypeFor<absl::string_view>(), WFL::CPPTYPE_STRING);
  EXPECT_EQ(WFL::CppTypeFor<google::protobuf::MessageLite>(), WFL::CPPTYPE_MESSAGE);
  EXPECT_EQ(WFL::CppTypeFor<proto2_unittest::TestAllTypes>(),
            WFL::CPPTYPE_MESSAGE);

  // And repeated too
  EXPECT_EQ(WFL::CppTypeFor<RepeatedField<int32_t>>(), WFL::CPPTYPE_INT32);
  EXPECT_EQ(WFL::CppTypeFor<RepeatedField<double>>(), WFL::CPPTYPE_DOUBLE);
  EXPECT_EQ(WFL::CppTypeFor<RepeatedPtrField<std::string>>(),
            WFL::CPPTYPE_STRING);
  EXPECT_EQ(WFL::CppTypeFor<RepeatedPtrField<proto2_unittest::TestAllTypes>>(),
            WFL::CPPTYPE_MESSAGE);
}

TEST(WireFormatLiteTest, ReadPackedPrimitiveInvalidInputAllocatesHuge) {
  uint8_t buffer[10];
  uint8_t* target = buffer;
  // Encode length 2,000,000,000
  target = google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(2'000'000'000,
                                                               target);
  int encoded_len = target - buffer;

  // Use ArrayInputStream to allow current_limit_ to be INT_MAX initially,
  // enabling PushLimit to push large values.
  google::protobuf::io::ArrayInputStream array_input(buffer, encoded_len);
  google::protobuf::io::CodedInputStream coded_input(&array_input);

  // Set TotalBytesLimit to large value to allow fast path to be considered.
  coded_input.SetTotalBytesLimit(2'100'000'000);

  // Simulate being inside an embedded message with large limit
  auto limit = coded_input.PushLimit(2'050'000'000);

  RepeatedField<uint32_t> values;

  // It should return false because it cannot read 2,000,000,000 bytes.
  EXPECT_FALSE(
      (WireFormatLite::ReadPackedPrimitive<uint32_t,
                                           WireFormatLite::TYPE_FIXED32>(
          &coded_input, &values)));

  // Verify that it did NOT allocate huge memory.
  // If this assertion FAILS, it means the code allocated 2G, proving the issue.
  EXPECT_LT(values.Capacity(), 1000);

  coded_input.PopLimit(limit);
}

TEST(WireFormatTest, ParseMessageSetItemInvalidInputAllocatesHuge) {
  uint8_t buffer[20];
  uint8_t* target = buffer;
  // Encode Tag kMessageSetMessageTag
  target = google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(
      WireFormatLite::kMessageSetMessageTag, target);
  // Encode length 2,000,000,000
  target = google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(2'000'000'000,
                                                               target);
  int encoded_len = target - buffer;

  google::protobuf::io::ArrayInputStream array_input(buffer, encoded_len);
  google::protobuf::io::CodedInputStream coded_input(&array_input);

  coded_input.SetTotalBytesLimit(2'100'000'000);

  proto2_wireformat_unittest::TestMessageSet message_set;

  // It should return false because it cannot read 2,000,000,000 bytes.
  EXPECT_FALSE(
      WireFormat::ParseAndMergeMessageSetItem(&coded_input, &message_set));
}

// ZeroCopyOutputStream that discards output into a small scratch buffer until
// Capture() is called, allowing >2 GiB / >4 GiB offsets to be reached cheaply.
class TailCapturingOutputStream final : public io::ZeroCopyOutputStream {
 public:
  bool Next(void** data, int* size) override {
    last_captured_ = false;
    if (fail_) return false;
    if (chunk_size_ > 0) {
      captured_.resize(captured_.size() + chunk_size_);
      *data = captured_.data() + captured_.size() - chunk_size_;
      *size = static_cast<int>(chunk_size_);
      last_captured_ = true;
    } else {
      *data = scratch_;
      *size = sizeof(scratch_);
    }
    byte_count_ += *size;
    return true;
  }

  void BackUp(int count) override {
    ASSERT_GE(count, 0);
    byte_count_ -= count;
    if (last_captured_) {
      ASSERT_LE(static_cast<size_t>(count), captured_.size());
      captured_.resize(captured_.size() - static_cast<size_t>(count));
      last_captured_ = false;
    }
  }

  int64_t ByteCount() const override { return byte_count_; }

  void Capture(size_t chunk_size) {
    captured_.clear();
    chunk_size_ = chunk_size;
  }
  void Discard() { chunk_size_ = 0; }
  void Fail() { fail_ = true; }
  absl::string_view captured() const { return captured_; }

 private:
  char scratch_[4096]{};
  std::string captured_;
  int64_t byte_count_ = 0;
  size_t chunk_size_ = 0;
  bool last_captured_ = false;
  bool fail_ = false;
};

TEST(WireFormatTest, CodedOutputStreamSerializeOver2G) {
  proto2_unittest::TestAllTypes message;
  message.set_optional_string("0123456789abcdef");
  const std::string expected = message.SerializeAsString();
  const int size = static_cast<int>(expected.size());
  ASSERT_GT(size, 10);

  auto verify = [&](absl::string_view name, auto serialize) {
    SCOPED_TRACE(name);
    TailCapturingOutputStream raw_output;
    {
      // Exercise non-zero CodedOutputStream::start_count_.
      io::CodedOutputStream initial(&raw_output);
      initial.WriteRaw("12345", 5);
    }
    io::CodedOutputStream output(&raw_output);

    // Cross 2 GiB from an active direct buffer (chunk_size = 64 > kSlopBytes)
    // and >4 GiB in patch-buffer mode (chunk_size = 16 <= kSlopBytes), each
    // after a 3-byte prefix so EpsCopyOutputStream's delta is non-zero.
    const struct {
      int skip;
      size_t chunk_size;
    } kPhases[] = {{std::numeric_limits<int>::max() - 10, 64},
                   {std::numeric_limits<int>::max(), 16}};
    for (const auto& [skip, chunk_size] : kPhases) {
      SCOPED_TRACE(chunk_size);
      raw_output.Discard();
      ASSERT_TRUE(output.Skip(skip));
      output.Trim();
      const int64_t start = raw_output.ByteCount();
      raw_output.Capture(chunk_size);
      output.WriteRaw("abc", 3);
      EXPECT_TRUE(serialize(output));
      EXPECT_EQ(internal::ByteCount64(&output), start + 3 + size - 5);
      output.Trim();
      EXPECT_FALSE(output.HadError());
      EXPECT_EQ(raw_output.captured(), "abc" + expected);
      EXPECT_EQ(raw_output.ByteCount(), start + 3 + size);
    }

    raw_output.Fail();
    EXPECT_FALSE(serialize(output));
    EXPECT_TRUE(output.HadError());
  };

  verify("SerializePartialToCodedStream", [&](io::CodedOutputStream& out) {
    return message.SerializePartialToCodedStream(&out);
  });
  verify("WireFormat::SerializeWithCachedSizes",
         [&](io::CodedOutputStream& out) {
           WireFormat::SerializeWithCachedSizes(message, size, &out);
           return !out.HadError();
         });
}


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
