// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This file contains tests and benchmarks.

#include "google/protobuf/io/coded_stream.h"

#include <limits.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/casts.h"
#include "absl/base/log_severity.h"
#include "absl/base/macros.h"
#include "absl/log/absl_log.h"
#include "absl/log/scoped_mock_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

#include "google/protobuf/stubs/common.h"
#include "google/protobuf/testing/googletest.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace io {
namespace {

class CodedStreamTest : public testing::Test {
 protected:
  // Buffer used during most of the tests. This assumes tests run sequentially.
  static constexpr int kBufferSize = 1024 * 64;
  static uint8_t buffer_[kBufferSize];
};

uint8_t CodedStreamTest::buffer_[CodedStreamTest::kBufferSize] = {};

// We test each operation over a variety of block sizes to insure that
// we test cases where reads or writes cross buffer boundaries, cases
// where they don't, and cases where there is so much buffer left that
// we can use special optimized paths that don't worry about bounds
// checks.
const int kBlockSizes[] = {1, 2, 3, 5, 7, 13, 32, 1024};

// In several ReadCord test functions, we either clear the Cord before ReadCord
// calls or not.
const bool kResetCords[] = {false, true};

// -------------------------------------------------------------------
// Varint tests.

struct VarintCase {
  uint8_t bytes[10];  // Encoded bytes.
  size_t size;        // Encoded size, in bytes.
  uint64_t value;     // Parsed value.
};

inline std::ostream& operator<<(std::ostream& os, const VarintCase& c) {
  return os << c.value;
}

VarintCase kVarintCases[] = {
    // 32-bit values
    {{0x00}, 1, 0},
    {{0x01}, 1, 1},
    {{0x7f}, 1, 127},
    {{0xa2, 0x74}, 2, (0x22 << 0) | (0x74 << 7)},  // 14882
    {{0xbe, 0xf7, 0x92, 0x84, 0x0b},
     5,  // 2961488830
     (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
         (uint64_t{0x0bu} << 28)},

    // 64-bit
    {{0xbe, 0xf7, 0x92, 0x84, 0x1b},
     5,  // 7256456126
     (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
         (uint64_t{0x1bu} << 28)},
    {{0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49},
     8,  // 41256202580718336
     (0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) |
         (uint64_t{0x43u} << 28) | (uint64_t{0x49u} << 35) |
         (uint64_t{0x24u} << 42) | (uint64_t{0x49u} << 49)},
    // 11964378330978735131
    {{0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01},
     10,
     (0x1b << 0) | (0x28 << 7) | (0x79 << 14) | (0x42 << 21) |
         (uint64_t{0x3bu} << 28) | (uint64_t{0x56u} << 35) |
         (uint64_t{0x00u} << 42) | (uint64_t{0x05u} << 49) |
         (uint64_t{0x26u} << 56) | (uint64_t{0x01u} << 63)},
};

class VarintCasesWithSizes
    : public CodedStreamTest,
      public testing::WithParamInterface<std::tuple<VarintCase, int>> {};

TEST_P(VarintCasesWithSizes, ReadVarint32) {
  const VarintCase& kVarintCases_case = std::get<0>(GetParam());
  const int& kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kVarintCases_case.bytes, kVarintCases_case.size);
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    uint32_t value;
    EXPECT_TRUE(coded_input.ReadVarint32(&value));
    EXPECT_EQ(static_cast<uint32_t>(kVarintCases_case.value), value);
  }

  EXPECT_EQ(kVarintCases_case.size, input.ByteCount());
}

TEST_P(VarintCasesWithSizes, ReadTag) {
  const VarintCase& kVarintCases_case = std::get<0>(GetParam());
  const int& kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kVarintCases_case.bytes, kVarintCases_case.size);
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    uint32_t expected_value = static_cast<uint32_t>(kVarintCases_case.value);
    EXPECT_EQ(expected_value, coded_input.ReadTag());

    EXPECT_TRUE(coded_input.LastTagWas(expected_value));
    EXPECT_FALSE(coded_input.LastTagWas(expected_value + 1));
  }

  EXPECT_EQ(kVarintCases_case.size, input.ByteCount());
}

// This is the regression test that verifies that there is no issues
// with the empty input buffers handling.
TEST_F(CodedStreamTest, EmptyInputBeforeEos) {
  class In : public ZeroCopyInputStream {
   public:
    In() : count_(0) {}

   private:
    bool Next(const void** data, int* size) override {
      *data = nullptr;
      *size = 0;
      return count_++ < 2;
    }
    void BackUp(int count) override {
      ABSL_LOG(FATAL) << "Tests never call this.";
    }
    bool Skip(int count) override {
      ABSL_LOG(FATAL) << "Tests never call this.";
      return false;
    }
    int64_t ByteCount() const override { return 0; }
    int count_;
  } in;
  CodedInputStream input(&in);
  input.ReadTagNoLastTag();
  EXPECT_TRUE(input.ConsumedEntireMessage());
}

class VarintCases : public CodedStreamTest,
                    public testing::WithParamInterface<VarintCase> {};

TEST_P(VarintCases, ExpectTag) {
  const VarintCase& kVarintCases_case = GetParam();
  // Leave one byte at the beginning of the buffer so we can read it
  // to force the first buffer to be loaded.
  buffer_[0] = '\0';
  memcpy(buffer_ + 1, kVarintCases_case.bytes, kVarintCases_case.size);
  ArrayInputStream input(buffer_, sizeof(buffer_));

  {
    CodedInputStream coded_input(&input);

    // Read one byte to force coded_input.Refill() to be called.  Otherwise,
    // ExpectTag() will return a false negative.
    uint8_t dummy;
    coded_input.ReadRaw(&dummy, 1);
    EXPECT_EQ((uint)'\0', (uint)dummy);

    uint32_t expected_value = static_cast<uint32_t>(kVarintCases_case.value);

    // ExpectTag() produces false negatives for large values.
    if (kVarintCases_case.size <= 2) {
      EXPECT_FALSE(coded_input.ExpectTag(expected_value + 1));
      EXPECT_TRUE(coded_input.ExpectTag(expected_value));
    } else {
      EXPECT_FALSE(coded_input.ExpectTag(expected_value));
    }
  }

  if (kVarintCases_case.size <= 2) {
    EXPECT_EQ(kVarintCases_case.size + 1, input.ByteCount());
  } else {
    EXPECT_EQ(1, input.ByteCount());
  }
}

TEST_P(VarintCases, ExpectTagFromArray) {
  const VarintCase& kVarintCases_case = GetParam();
  memcpy(buffer_, kVarintCases_case.bytes, kVarintCases_case.size);

  const uint32_t expected_value =
      static_cast<uint32_t>(kVarintCases_case.value);

  // If the expectation succeeds, it should return a pointer past the tag.
  if (kVarintCases_case.size <= 2) {
    EXPECT_TRUE(nullptr == CodedInputStream::ExpectTagFromArray(
                            buffer_, expected_value + 1));
    EXPECT_TRUE(buffer_ + kVarintCases_case.size ==
                CodedInputStream::ExpectTagFromArray(buffer_, expected_value));
  } else {
    EXPECT_TRUE(nullptr ==
                CodedInputStream::ExpectTagFromArray(buffer_, expected_value));
  }
}

TEST_P(VarintCasesWithSizes, ReadVarint64) {
  const VarintCase& kVarintCases_case = std::get<0>(GetParam());
  const int& kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kVarintCases_case.bytes, kVarintCases_case.size);
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    uint64_t value;
    EXPECT_TRUE(coded_input.ReadVarint64(&value));
    EXPECT_EQ(kVarintCases_case.value, value);
  }

  EXPECT_EQ(kVarintCases_case.size, input.ByteCount());
}

TEST_P(VarintCasesWithSizes, WriteVarint32) {
  const VarintCase& kVarintCases_case = std::get<0>(GetParam());
  const int& kBlockSizes_case = std::get<1>(GetParam());
  if (kVarintCases_case.value > uint64_t{0x00000000FFFFFFFFu}) {
    // Skip this test for the 64-bit values.
    return;
  }

  ArrayOutputStream output(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedOutputStream coded_output(&output);

    coded_output.WriteVarint32(static_cast<uint32_t>(kVarintCases_case.value));
    EXPECT_FALSE(coded_output.HadError());

    EXPECT_EQ(kVarintCases_case.size, coded_output.ByteCount());
  }

  EXPECT_EQ(kVarintCases_case.size, output.ByteCount());
  EXPECT_EQ(0,
            memcmp(buffer_, kVarintCases_case.bytes, kVarintCases_case.size));
}

TEST_P(VarintCasesWithSizes, WriteVarint64) {
  const VarintCase& kVarintCases_case = std::get<0>(GetParam());
  const int& kBlockSizes_case = std::get<1>(GetParam());
  ArrayOutputStream output(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedOutputStream coded_output(&output);

    coded_output.WriteVarint64(kVarintCases_case.value);
    EXPECT_FALSE(coded_output.HadError());

    EXPECT_EQ(kVarintCases_case.size, coded_output.ByteCount());
  }

  EXPECT_EQ(kVarintCases_case.size, output.ByteCount());
  EXPECT_EQ(0,
            memcmp(buffer_, kVarintCases_case.bytes, kVarintCases_case.size));
}

class SignedVarintCasesWithSizes
    : public CodedStreamTest,
      public testing::WithParamInterface<std::tuple<int32_t, int>> {};

int32_t kSignExtendedVarintCases[] = {0, 1, -1, 1237894, -37895138};

TEST_P(SignedVarintCasesWithSizes, WriteVarint32SignExtended) {
  const int32_t& kSignExtendedVarintCases_case = std::get<0>(GetParam());
  const int& kBlockSizes_case = std::get<1>(GetParam());
  ArrayOutputStream output(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedOutputStream coded_output(&output);

    coded_output.WriteVarint32SignExtended(kSignExtendedVarintCases_case);
    EXPECT_FALSE(coded_output.HadError());

    if (kSignExtendedVarintCases_case < 0) {
      EXPECT_EQ(10, coded_output.ByteCount());
    } else {
      EXPECT_LE(coded_output.ByteCount(), 5);
    }
  }

  if (kSignExtendedVarintCases_case < 0) {
    EXPECT_EQ(10, output.ByteCount());
  } else {
    EXPECT_LE(output.ByteCount(), 5);
  }

  // Read value back in as a varint64 and insure it matches.
  ArrayInputStream input(buffer_, sizeof(buffer_));

  {
    CodedInputStream coded_input(&input);

    uint64_t value;
    EXPECT_TRUE(coded_input.ReadVarint64(&value));

    EXPECT_EQ(kSignExtendedVarintCases_case, static_cast<int64_t>(value));
  }

  EXPECT_EQ(output.ByteCount(), input.ByteCount());
}


// -------------------------------------------------------------------
// Varint failure test.

struct VarintErrorCase {
  uint8_t bytes[12];
  size_t size;
  bool can_parse;
  const char* name;
};

inline std::ostream& operator<<(std::ostream& os, const VarintErrorCase& c) {
  return os << "size " << c.size;
}

const VarintErrorCase kVarintErrorCases[] = {
    // Control case.  (Insures that there isn't something else wrong that
    // makes parsing always fail.)
    {{0x00}, 1, true, "Control"},

    // No input data.
    {{}, 0, false, "No_Input"},

    // Input ends unexpectedly.
    {{0xf0, 0xab}, 2, false, "Input_Ends_Unexpectedly"},

    // Input ends unexpectedly after 32 bits.
    {{0xf0, 0xab, 0xc9, 0x9a, 0xf8, 0xb2},
     6,
     false,
     "Input_Ends_Unexpectedly_After_32_Bits"},

    // Longer than 10 bytes.
    {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01},
     11,
     false,
     "Longer_Than_10_Bytes"},
};

class VarintErrorCasesWithSizes
    : public CodedStreamTest,
      public testing::WithParamInterface<std::tuple<VarintErrorCase, int>> {};

TEST_P(VarintErrorCasesWithSizes, ReadVarint32Error) {
  VarintErrorCase kVarintErrorCases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kVarintErrorCases_case.bytes, kVarintErrorCases_case.size);
  ArrayInputStream input(buffer_, static_cast<int>(kVarintErrorCases_case.size),
                         kBlockSizes_case);
  CodedInputStream coded_input(&input);

  uint32_t value;
  EXPECT_EQ(kVarintErrorCases_case.can_parse, coded_input.ReadVarint32(&value));
}

TEST_P(VarintErrorCasesWithSizes,
       ReadVarint32Error_LeavesValueInInitializedState) {
  VarintErrorCase kVarintErrorCases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kVarintErrorCases_case.bytes, kVarintErrorCases_case.size);
  ArrayInputStream input(buffer_, static_cast<int>(kVarintErrorCases_case.size),
                         kBlockSizes_case);
  CodedInputStream coded_input(&input);

  uint32_t value = 0;
  EXPECT_EQ(kVarintErrorCases_case.can_parse, coded_input.ReadVarint32(&value));
  // While the specific value following a failure is not critical, we do want to
  // ensure that it doesn't get set to an uninitialized value. (This check fails
  // in MSAN mode if value has been set to an uninitialized value.)
  EXPECT_EQ(value, value);
}

TEST_P(VarintErrorCasesWithSizes, ReadVarint64Error) {
  VarintErrorCase kVarintErrorCases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kVarintErrorCases_case.bytes, kVarintErrorCases_case.size);
  ArrayInputStream input(buffer_, static_cast<int>(kVarintErrorCases_case.size),
                         kBlockSizes_case);
  CodedInputStream coded_input(&input);

  uint64_t value;
  EXPECT_EQ(kVarintErrorCases_case.can_parse, coded_input.ReadVarint64(&value));
}

TEST_P(VarintErrorCasesWithSizes,
       ReadVarint64Error_LeavesValueInInitializedState) {
  VarintErrorCase kVarintErrorCases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kVarintErrorCases_case.bytes, kVarintErrorCases_case.size);
  ArrayInputStream input(buffer_, static_cast<int>(kVarintErrorCases_case.size),
                         kBlockSizes_case);
  CodedInputStream coded_input(&input);

  uint64_t value = 0;
  EXPECT_EQ(kVarintErrorCases_case.can_parse, coded_input.ReadVarint64(&value));
  // While the specific value following a failure is not critical, we do want to
  // ensure that it doesn't get set to an uninitialized value. (This check fails
  // in MSAN mode if value has been set to an uninitialized value.)
  EXPECT_EQ(value, value);
}

// -------------------------------------------------------------------
// VarintSize

struct VarintSizeCase {
  uint64_t value;
  int size;
};

inline std::ostream& operator<<(std::ostream& os, const VarintSizeCase& c) {
  return os << c.value;
}

VarintSizeCase kVarintSizeCases[] = {
    {0u, 1},
    {1u, 1},
    {127u, 1},
    {128u, 2},
    {758923u, 3},
    {4000000000u, 5},
    {uint64_t{41256202580718336u}, 8},
    {uint64_t{11964378330978735131u}, 10},
};

class VarintSizeCases : public CodedStreamTest,
                        public testing::WithParamInterface<VarintSizeCase> {};

TEST_P(VarintSizeCases, VarintSize32) {
  VarintSizeCase kVarintSizeCases_case = GetParam();
  if (kVarintSizeCases_case.value > 0xffffffffu) {
    // Skip 64-bit values.
    return;
  }

  EXPECT_EQ(kVarintSizeCases_case.size,
            CodedOutputStream::VarintSize32(
                static_cast<uint32_t>(kVarintSizeCases_case.value)));
}

TEST_P(VarintSizeCases, VarintSize64) {
  VarintSizeCase kVarintSizeCases_case = GetParam();
  EXPECT_EQ(kVarintSizeCases_case.size,
            CodedOutputStream::VarintSize64(kVarintSizeCases_case.value));
}

TEST_F(CodedStreamTest, VarintSize32PowersOfTwo) {
  int expected = 1;
  for (int i = 1; i < 32; i++) {
    if (i % 7 == 0) {
      expected += 1;
    }
    EXPECT_EQ(expected, CodedOutputStream::VarintSize32(
                            static_cast<uint32_t>(0x1u << i)));
  }
}

TEST_F(CodedStreamTest, VarintSize64PowersOfTwo) {
  int expected = 1;
  for (int i = 1; i < 64; i++) {
    if (i % 7 == 0) {
      expected += 1;
    }
    EXPECT_EQ(expected, CodedOutputStream::VarintSize64(uint64_t{1} << i));
  }
}

// -------------------------------------------------------------------
// Fixed-size int tests

struct Fixed16Case {
  uint8_t bytes[sizeof(uint16_t)];  // Encoded bytes.
  uint32_t value;                   // Parsed value.
};

struct Fixed32Case {
  uint8_t bytes[sizeof(uint32_t)];  // Encoded bytes.
  uint32_t value;                   // Parsed value.
};

struct Fixed64Case {
  uint8_t bytes[sizeof(uint64_t)];  // Encoded bytes.
  uint64_t value;                   // Parsed value.
};

class Fixed16Cases : public CodedStreamTest,
                     public testing::WithParamInterface<Fixed16Case> {};

class Fixed16CasesWithSizes
    : public CodedStreamTest,
      public testing::WithParamInterface<std::tuple<Fixed16Case, int>> {};

class Fixed32Cases : public CodedStreamTest,
                     public testing::WithParamInterface<Fixed32Case> {};

class Fixed32CasesWithSizes
    : public CodedStreamTest,
      public testing::WithParamInterface<std::tuple<Fixed32Case, int>> {};

class Fixed64Cases : public CodedStreamTest,
                     public testing::WithParamInterface<Fixed64Case> {};

class Fixed64CasesWithSizes
    : public CodedStreamTest,
      public testing::WithParamInterface<std::tuple<Fixed64Case, int>> {};

inline std::ostream& operator<<(std::ostream& os, const Fixed32Case& c) {
  return os << "0x" << std::hex << c.value << std::dec;
}

inline std::ostream& operator<<(std::ostream& os, const Fixed64Case& c) {
  return os << "0x" << std::hex << c.value << std::dec;
}

Fixed16Case kFixed16Cases[] = {
    {{0xef, 0xcd}, 0xcdefu},
    {{0x12, 0x34}, 0x3412u},
};

Fixed32Case kFixed32Cases[] = {
    {{0xef, 0xcd, 0xab, 0x90}, 0x90abcdefu},
    {{0x12, 0x34, 0x56, 0x78}, 0x78563412u},
};

Fixed64Case kFixed64Cases[] = {
    {{0xef, 0xcd, 0xab, 0x90, 0x12, 0x34, 0x56, 0x78},
     uint64_t{0x7856341290abcdefu}},
    {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},
     uint64_t{0x8877665544332211u}},
};

TEST_P(Fixed16CasesWithSizes, ReadLittleEndian16) {
  Fixed16Case kFixed16Cases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kFixed16Cases_case.bytes, sizeof(kFixed16Cases_case.bytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    uint16_t value;
    EXPECT_TRUE(coded_input.ReadLittleEndian16(&value));
    EXPECT_EQ(kFixed16Cases_case.value, value);
  }

  EXPECT_EQ(sizeof(uint16_t), input.ByteCount());
}

TEST_P(Fixed32CasesWithSizes, ReadLittleEndian32) {
  Fixed32Case kFixed32Cases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kFixed32Cases_case.bytes, sizeof(kFixed32Cases_case.bytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    uint32_t value;
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(kFixed32Cases_case.value, value);
  }

  EXPECT_EQ(sizeof(uint32_t), input.ByteCount());
}

TEST_P(Fixed64CasesWithSizes, ReadLittleEndian64) {
  Fixed64Case kFixed64Cases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  memcpy(buffer_, kFixed64Cases_case.bytes, sizeof(kFixed64Cases_case.bytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    uint64_t value;
    EXPECT_TRUE(coded_input.ReadLittleEndian64(&value));
    EXPECT_EQ(kFixed64Cases_case.value, value);
  }

  EXPECT_EQ(sizeof(uint64_t), input.ByteCount());
}

TEST_P(Fixed16CasesWithSizes, WriteLittleEndian16) {
  Fixed16Case kFixed16Cases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  ArrayOutputStream output(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedOutputStream coded_output(&output);

    coded_output.WriteLittleEndian16(kFixed16Cases_case.value);
    EXPECT_FALSE(coded_output.HadError());

    EXPECT_EQ(sizeof(uint16_t), coded_output.ByteCount());
  }

  EXPECT_EQ(sizeof(uint16_t), output.ByteCount());
  EXPECT_EQ(0, memcmp(buffer_, kFixed16Cases_case.bytes, sizeof(uint16_t)));
}

TEST_P(Fixed32CasesWithSizes, WriteLittleEndian32) {
  Fixed32Case kFixed32Cases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  ArrayOutputStream output(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedOutputStream coded_output(&output);

    coded_output.WriteLittleEndian32(kFixed32Cases_case.value);
    EXPECT_FALSE(coded_output.HadError());

    EXPECT_EQ(sizeof(uint32_t), coded_output.ByteCount());
  }

  EXPECT_EQ(sizeof(uint32_t), output.ByteCount());
  EXPECT_EQ(0, memcmp(buffer_, kFixed32Cases_case.bytes, sizeof(uint32_t)));
}

TEST_P(Fixed64CasesWithSizes, WriteLittleEndian64) {
  Fixed64Case kFixed64Cases_case = std::get<0>(GetParam());
  int kBlockSizes_case = std::get<1>(GetParam());
  ArrayOutputStream output(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedOutputStream coded_output(&output);

    coded_output.WriteLittleEndian64(kFixed64Cases_case.value);
    EXPECT_FALSE(coded_output.HadError());

    EXPECT_EQ(sizeof(uint64_t), coded_output.ByteCount());
  }

  EXPECT_EQ(sizeof(uint64_t), output.ByteCount());
  EXPECT_EQ(0, memcmp(buffer_, kFixed64Cases_case.bytes, sizeof(uint64_t)));
}

// Tests using the static methods to read fixed-size values from raw arrays.

TEST_P(Fixed16Cases, ReadLittleEndian16FromArray) {
  Fixed16Case kFixed16Cases_case = GetParam();
  memcpy(buffer_, kFixed16Cases_case.bytes, sizeof(kFixed16Cases_case.bytes));

  uint16_t value;
  const uint8_t* end =
      CodedInputStream::ReadLittleEndian16FromArray(buffer_, &value);
  EXPECT_EQ(kFixed16Cases_case.value, value);
  EXPECT_TRUE(end == buffer_ + sizeof(value));
}

TEST_P(Fixed32Cases, ReadLittleEndian32FromArray) {
  Fixed32Case kFixed32Cases_case = GetParam();
  memcpy(buffer_, kFixed32Cases_case.bytes, sizeof(kFixed32Cases_case.bytes));

  uint32_t value;
  const uint8_t* end =
      CodedInputStream::ReadLittleEndian32FromArray(buffer_, &value);
  EXPECT_EQ(kFixed32Cases_case.value, value);
  EXPECT_TRUE(end == buffer_ + sizeof(value));
}

TEST_P(Fixed64Cases, ReadLittleEndian64FromArray) {
  Fixed64Case kFixed64Cases_case = GetParam();
  memcpy(buffer_, kFixed64Cases_case.bytes, sizeof(kFixed64Cases_case.bytes));

  uint64_t value;
  const uint8_t* end =
      CodedInputStream::ReadLittleEndian64FromArray(buffer_, &value);
  EXPECT_EQ(kFixed64Cases_case.value, value);
  EXPECT_TRUE(end == buffer_ + sizeof(value));
}

// -------------------------------------------------------------------
// Raw reads and writes

const char kRawBytes[] = "Some bytes which will be written and read raw.";

class BlockSizes : public CodedStreamTest,
                   public testing::WithParamInterface<int> {};

TEST_P(BlockSizes, ReadRaw) {
  int kBlockSizes_case = GetParam();
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);
  char read_buffer[sizeof(kRawBytes)];

  {
    CodedInputStream coded_input(&input);

    EXPECT_TRUE(coded_input.ReadRaw(read_buffer, sizeof(kRawBytes)));
    EXPECT_EQ(0, memcmp(kRawBytes, read_buffer, sizeof(kRawBytes)));
  }

  EXPECT_EQ(sizeof(kRawBytes), input.ByteCount());
}

TEST_P(BlockSizes, WriteRaw) {
  int kBlockSizes_case = GetParam();
  ArrayOutputStream output(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedOutputStream coded_output(&output);

    coded_output.WriteRaw(kRawBytes, sizeof(kRawBytes));
    EXPECT_FALSE(coded_output.HadError());

    EXPECT_EQ(sizeof(kRawBytes), coded_output.ByteCount());
  }

  EXPECT_EQ(sizeof(kRawBytes), output.ByteCount());
  EXPECT_EQ(0, memcmp(buffer_, kRawBytes, sizeof(kRawBytes)));
}

TEST_P(BlockSizes, ReadString) {
  int kBlockSizes_case = GetParam();
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    std::string str;
    EXPECT_TRUE(coded_input.ReadString(&str, strlen(kRawBytes)));
    EXPECT_EQ(kRawBytes, str);
  }

  EXPECT_EQ(strlen(kRawBytes), input.ByteCount());
}

// Check to make sure ReadString doesn't crash on impossibly large strings.
TEST_P(BlockSizes, ReadStringImpossiblyLarge) {
  int kBlockSizes_case = GetParam();
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    std::string str;
    // Try to read a gigabyte.
    EXPECT_FALSE(coded_input.ReadString(&str, 1 << 30));
  }
}

TEST_F(CodedStreamTest, ReadStringImpossiblyLargeFromStringOnStack) {
  // Same test as above, except directly use a buffer. This used to cause
  // crashes while the above did not.
  uint8_t buffer[8] = {};
  CodedInputStream coded_input(buffer, 8);
  std::string str;
  EXPECT_FALSE(coded_input.ReadString(&str, 1 << 30));
}

TEST_F(CodedStreamTest, ReadStringImpossiblyLargeFromStringOnHeap) {
  std::unique_ptr<uint8_t[]> buffer(new uint8_t[8]);
  CodedInputStream coded_input(buffer.get(), 8);
  std::string str;
  EXPECT_FALSE(coded_input.ReadString(&str, 1 << 30));
}

TEST_P(BlockSizes, ReadStringReservesMemoryOnTotalLimit) {
  int kBlockSizes_case = GetParam();
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);
    coded_input.SetTotalBytesLimit(sizeof(kRawBytes));
    EXPECT_EQ(sizeof(kRawBytes), coded_input.BytesUntilTotalBytesLimit());

    std::string str;
    EXPECT_TRUE(coded_input.ReadString(&str, strlen(kRawBytes)));
    EXPECT_EQ(sizeof(kRawBytes) - strlen(kRawBytes),
              coded_input.BytesUntilTotalBytesLimit());
    EXPECT_EQ(kRawBytes, str);
    // TODO: Replace with a more meaningful test (see cl/60966023).
    EXPECT_GE(str.capacity(), strlen(kRawBytes));
  }

  EXPECT_EQ(strlen(kRawBytes), input.ByteCount());
}

TEST_P(BlockSizes, ReadStringReservesMemoryOnPushedLimit) {
  int kBlockSizes_case = GetParam();
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);
    coded_input.PushLimit(sizeof(buffer_));

    std::string str;
    EXPECT_TRUE(coded_input.ReadString(&str, strlen(kRawBytes)));
    EXPECT_EQ(kRawBytes, str);
    // TODO: Replace with a more meaningful test (see cl/60966023).
    EXPECT_GE(str.capacity(), strlen(kRawBytes));
  }

  EXPECT_EQ(strlen(kRawBytes), input.ByteCount());
}

TEST_F(CodedStreamTest, ReadStringNoReservationIfLimitsNotSet) {
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  // Buffer size in the input must be smaller than sizeof(kRawBytes),
  // otherwise check against capacity will fail as ReadStringInline()
  // will handle the reading and will reserve the memory as needed.
  ArrayInputStream input(buffer_, sizeof(buffer_), 32);

  {
    CodedInputStream coded_input(&input);

    std::string str;
    EXPECT_TRUE(coded_input.ReadString(&str, strlen(kRawBytes)));
    EXPECT_EQ(kRawBytes, str);
    // Note: this check depends on string class implementation. It
    // expects that string will allocate more than strlen(kRawBytes)
    // if the content of kRawBytes is appended to string in small
    // chunks.
    // TODO: Replace with a more meaningful test (see cl/60966023).
    EXPECT_GE(str.capacity(), strlen(kRawBytes));
  }

  EXPECT_EQ(strlen(kRawBytes), input.ByteCount());
}

TEST_F(CodedStreamTest, ReadStringNoReservationSizeIsNegative) {
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  // Buffer size in the input must be smaller than sizeof(kRawBytes),
  // otherwise check against capacity will fail as ReadStringInline()
  // will handle the reading and will reserve the memory as needed.
  ArrayInputStream input(buffer_, sizeof(buffer_), 32);

  {
    CodedInputStream coded_input(&input);
    coded_input.PushLimit(sizeof(buffer_));

    std::string str;
    EXPECT_FALSE(coded_input.ReadString(&str, -1));
    // Note: this check depends on string class implementation. It
    // expects that string will always allocate the same amount of
    // memory for an empty string.
    EXPECT_EQ(std::string().capacity(), str.capacity());
  }
}

TEST_F(CodedStreamTest, ReadStringNoReservationSizeIsLarge) {
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  // Buffer size in the input must be smaller than sizeof(kRawBytes),
  // otherwise check against capacity will fail as ReadStringInline()
  // will handle the reading and will reserve the memory as needed.
  ArrayInputStream input(buffer_, sizeof(buffer_), 32);

  {
    CodedInputStream coded_input(&input);
    coded_input.PushLimit(sizeof(buffer_));

    std::string str;
    EXPECT_FALSE(coded_input.ReadString(&str, 1 << 30));
    EXPECT_GT(1 << 30, str.capacity());
  }
}

TEST_F(CodedStreamTest, ReadStringNoReservationSizeIsOverTheLimit) {
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  // Buffer size in the input must be smaller than sizeof(kRawBytes),
  // otherwise check against capacity will fail as ReadStringInline()
  // will handle the reading and will reserve the memory as needed.
  ArrayInputStream input(buffer_, sizeof(buffer_), 32);

  {
    CodedInputStream coded_input(&input);
    coded_input.PushLimit(16);

    std::string str;
    EXPECT_FALSE(coded_input.ReadString(&str, strlen(kRawBytes)));
    // Note: this check depends on string class implementation. It
    // expects that string will allocate less than strlen(kRawBytes)
    // for an empty string.
    EXPECT_GT(strlen(kRawBytes), str.capacity());
  }
}

TEST_F(CodedStreamTest, ReadStringNoReservationSizeIsOverTheTotalBytesLimit) {
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  // Buffer size in the input must be smaller than sizeof(kRawBytes),
  // otherwise check against capacity will fail as ReadStringInline()
  // will handle the reading and will reserve the memory as needed.
  ArrayInputStream input(buffer_, sizeof(buffer_), 32);

  {
    CodedInputStream coded_input(&input);
    coded_input.SetTotalBytesLimit(16);

    std::string str;
    EXPECT_FALSE(coded_input.ReadString(&str, strlen(kRawBytes)));
    // Note: this check depends on string class implementation. It
    // expects that string will allocate less than strlen(kRawBytes)
    // for an empty string.
    EXPECT_GT(strlen(kRawBytes), str.capacity());
  }
}

TEST_F(CodedStreamTest,
       ReadStringNoReservationSizeIsOverTheClosestLimit_GlobalLimitIsCloser) {
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  // Buffer size in the input must be smaller than sizeof(kRawBytes),
  // otherwise check against capacity will fail as ReadStringInline()
  // will handle the reading and will reserve the memory as needed.
  ArrayInputStream input(buffer_, sizeof(buffer_), 32);

  {
    CodedInputStream coded_input(&input);
    coded_input.PushLimit(sizeof(buffer_));
    coded_input.SetTotalBytesLimit(16);

    std::string str;
    EXPECT_FALSE(coded_input.ReadString(&str, strlen(kRawBytes)));
    // Note: this check depends on string class implementation. It
    // expects that string will allocate less than strlen(kRawBytes)
    // for an empty string.
    EXPECT_GT(strlen(kRawBytes), str.capacity());
  }
}

TEST_F(CodedStreamTest,
       ReadStringNoReservationSizeIsOverTheClosestLimit_LocalLimitIsCloser) {
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  // Buffer size in the input must be smaller than sizeof(kRawBytes),
  // otherwise check against capacity will fail as ReadStringInline()
  // will handle the reading and will reserve the memory as needed.
  ArrayInputStream input(buffer_, sizeof(buffer_), 32);

  {
    CodedInputStream coded_input(&input);
    coded_input.PushLimit(16);
    coded_input.SetTotalBytesLimit(sizeof(buffer_));
    EXPECT_EQ(sizeof(buffer_), coded_input.BytesUntilTotalBytesLimit());

    std::string str;
    EXPECT_FALSE(coded_input.ReadString(&str, strlen(kRawBytes)));
    // Note: this check depends on string class implementation. It
    // expects that string will allocate less than strlen(kRawBytes)
    // for an empty string.
    EXPECT_GT(strlen(kRawBytes), str.capacity());
  }
}


// -------------------------------------------------------------------
// Cord reads and writes

class BlockSizesWithResetCords
    : public CodedStreamTest,
      public testing::WithParamInterface<std::tuple<int, bool>> {};

class ResetCords : public CodedStreamTest,
                   public testing::WithParamInterface<bool> {};

TEST_P(BlockSizes, ReadCord) {
  int kBlockSizes_case = GetParam();
  memcpy(buffer_, kRawBytes, sizeof(kRawBytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    absl::Cord cord;
    EXPECT_TRUE(coded_input.ReadCord(&cord, strlen(kRawBytes)));
    EXPECT_EQ(absl::Cord(kRawBytes), cord);
  }

  EXPECT_EQ(strlen(kRawBytes), input.ByteCount());
}

TEST_P(BlockSizes, ReadCordReuseCord) {
  ASSERT_GT(sizeof(buffer_), 1362 * sizeof(kRawBytes));
  int kBlockSizes_case = GetParam();
  for (size_t i = 0; i < 1362; i++) {
    memcpy(buffer_ + i * sizeof(kRawBytes), kRawBytes, sizeof(kRawBytes));
  }
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  int total_read = 0;
  {
    const char* buffer_str = reinterpret_cast<const char*>(buffer_);
    CodedInputStream coded_input(&input);
    static const int kSizes[] = {0,  1,  2,  3,   4,    5,    6,    7,
                                 8,  9,  10, 11,  12,   13,   14,   15,
                                 16, 17, 50, 100, 1023, 1024, 8000, 16000};
    int total_size = 0;
    std::vector<int> sizes;
    for (size_t i = 0; i < ABSL_ARRAYSIZE(kSizes); ++i) {
      sizes.push_back(kSizes[i]);
      total_size += kSizes[i];
    }
    ASSERT_GE(1362 * sizeof(kRawBytes), total_size * 2);

    absl::Cord reused_cord;
    for (int pass = 0; pass < 2; ++pass) {
      for (size_t i = 0; i < sizes.size(); ++i) {
        const int size = sizes[i];
        EXPECT_TRUE(coded_input.ReadCord(&reused_cord, size));
        EXPECT_EQ(size, reused_cord.size());
        EXPECT_EQ(
            std::string(buffer_str + total_read, static_cast<size_t>(size)),
            std::string(reused_cord));
        total_read += size;
      }
      std::reverse(sizes.begin(), sizes.end());  // Second pass is in reverse.
    }
  }
  EXPECT_EQ(total_read, input.ByteCount());
}

TEST_P(BlockSizesWithResetCords, ReadCordWithLimit) {
  int kBlockSizes_case = std::get<0>(GetParam());
  bool kResetCords_case = std::get<1>(GetParam());
  memcpy(buffer_, kRawBytes, strlen(kRawBytes));
  ArrayInputStream input(buffer_, strlen(kRawBytes), kBlockSizes_case);
  CodedInputStream coded_input(&input);

  CodedInputStream::Limit limit = coded_input.PushLimit(10);
  absl::Cord cord;
  EXPECT_TRUE(coded_input.ReadCord(&cord, 5));
  EXPECT_EQ(5, coded_input.BytesUntilLimit());
  if (kResetCords_case) cord.Clear();
  EXPECT_TRUE(coded_input.ReadCord(&cord, 4));
  EXPECT_EQ(1, coded_input.BytesUntilLimit());
  if (kResetCords_case) cord.Clear();
  EXPECT_FALSE(coded_input.ReadCord(&cord, 2));
  EXPECT_EQ(0, coded_input.BytesUntilLimit());
  EXPECT_EQ(1, cord.size());

  coded_input.PopLimit(limit);

  if (kResetCords_case) cord.Clear();
  EXPECT_TRUE(coded_input.ReadCord(&cord, strlen(kRawBytes) - 10));
  EXPECT_EQ(std::string(kRawBytes + 10), std::string(cord));
}

TEST_P(ResetCords, ReadLargeCord) {
  bool kResetCords_case = GetParam();
  absl::Cord large_cord;
  for (int i = 0; i < 1024; i++) {
    large_cord.Append(kRawBytes);
  }
  CordInputStream input(&large_cord);

  {
    CodedInputStream coded_input(&input);

    absl::Cord cord;
    if (!kResetCords_case) cord.Append(absl::string_view("value"));
    EXPECT_TRUE(
        coded_input.ReadCord(&cord, static_cast<int>(large_cord.size())));
    EXPECT_EQ(large_cord, cord);
  }

  EXPECT_EQ(large_cord.size(), input.ByteCount());
}

// Check to make sure ReadString doesn't crash on impossibly large strings.
TEST_P(BlockSizesWithResetCords, ReadCordImpossiblyLarge) {
  int kBlockSizes_case = std::get<0>(GetParam());
  bool kResetCords_case = std::get<1>(GetParam());
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    absl::Cord cord;
    if (!kResetCords_case) cord.Append(absl::string_view("value"));
    // Try to read a gigabyte.  This should fail because the input is only
    // sizeof(buffer_) bytes.
    EXPECT_FALSE(coded_input.ReadCord(&cord, 1 << 30));
  }
}

TEST_P(BlockSizes, WriteCord) {
  int kBlockSizes_case = GetParam();
  ArrayOutputStream output(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedOutputStream coded_output(&output);

    absl::Cord cord(kRawBytes);
    coded_output.WriteCord(cord);
    EXPECT_FALSE(coded_output.HadError());

    EXPECT_EQ(strlen(kRawBytes), coded_output.ByteCount());
  }

  EXPECT_EQ(strlen(kRawBytes), output.ByteCount());
  EXPECT_EQ(0, memcmp(buffer_, kRawBytes, strlen(kRawBytes)));
}

TEST_F(CodedStreamTest, WriteLargeCord) {
  absl::Cord large_cord;
  for (int i = 0; i < 1024; i++) {
    large_cord.Append(kRawBytes);
  }

  CordOutputStream output;
  {
    CodedOutputStream coded_output(&output);

    coded_output.WriteCord(large_cord);
    EXPECT_FALSE(coded_output.HadError());

    EXPECT_EQ(large_cord.size(), coded_output.ByteCount());
    EXPECT_EQ(large_cord.size(), output.ByteCount());
  }
  absl::Cord output_cord = output.Consume();
  EXPECT_EQ(large_cord, output_cord);
}

TEST_F(CodedStreamTest, Trim) {
  CordOutputStream cord_output;
  CodedOutputStream coded_output(&cord_output);

  // Verify that any initially reserved output buffers created when the output
  // streams were created are trimmed on an initial Trim call.
  coded_output.Trim();
  EXPECT_EQ(0, coded_output.ByteCount());

  // Write a single byte to the coded stream, ensure the cord stream has been
  // advanced, and then verify Trim() does the right thing.
  const char kTestData[] = "abcdef";
  coded_output.WriteRaw(kTestData, 1);
  coded_output.Trim();
  EXPECT_EQ(1, coded_output.ByteCount());

  // Write some more data to the coded stream, Trim() it, and verify
  // everything behaves as expected.
  coded_output.WriteRaw(kTestData, sizeof(kTestData));
  coded_output.Trim();
  EXPECT_EQ(1 + sizeof(kTestData), coded_output.ByteCount());

  absl::Cord cord = cord_output.Consume();
  EXPECT_EQ(1 + sizeof(kTestData), cord.size());
}


// -------------------------------------------------------------------
// Skip

const char kSkipTestBytes[] =
    "<Before skipping><To be skipped><After skipping>";

TEST_P(BlockSizes, SkipInput) {
  int kBlockSizes_case = GetParam();
  memcpy(buffer_, kSkipTestBytes, sizeof(kSkipTestBytes));
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    std::string str;
    EXPECT_TRUE(coded_input.ReadString(&str, strlen("<Before skipping>")));
    EXPECT_EQ("<Before skipping>", str);
    EXPECT_TRUE(coded_input.Skip(strlen("<To be skipped>")));
    EXPECT_TRUE(coded_input.ReadString(&str, strlen("<After skipping>")));
    EXPECT_EQ("<After skipping>", str);
  }

  EXPECT_EQ(strlen(kSkipTestBytes), input.ByteCount());
}

// -------------------------------------------------------------------
// GetDirectBufferPointer

TEST_F(CodedStreamTest, GetDirectBufferPointerInput) {
  ArrayInputStream input(buffer_, sizeof(buffer_), 8);
  CodedInputStream coded_input(&input);

  const void* ptr;
  int size;

  EXPECT_TRUE(coded_input.GetDirectBufferPointer(&ptr, &size));
  EXPECT_EQ(buffer_, ptr);
  EXPECT_EQ(8, size);

  // Peeking again should return the same pointer.
  EXPECT_TRUE(coded_input.GetDirectBufferPointer(&ptr, &size));
  EXPECT_EQ(buffer_, ptr);
  EXPECT_EQ(8, size);

  // Skip forward in the same buffer then peek again.
  EXPECT_TRUE(coded_input.Skip(3));
  EXPECT_TRUE(coded_input.GetDirectBufferPointer(&ptr, &size));
  EXPECT_EQ(buffer_ + 3, ptr);
  EXPECT_EQ(5, size);

  // Skip to end of buffer and peek -- should get next buffer.
  EXPECT_TRUE(coded_input.Skip(5));
  EXPECT_TRUE(coded_input.GetDirectBufferPointer(&ptr, &size));
  EXPECT_EQ(buffer_ + 8, ptr);
  EXPECT_EQ(8, size);
}

TEST_F(CodedStreamTest, GetDirectBufferPointerInlineInput) {
  ArrayInputStream input(buffer_, sizeof(buffer_), 8);
  CodedInputStream coded_input(&input);

  const void* ptr;
  int size;

  coded_input.GetDirectBufferPointerInline(&ptr, &size);
  EXPECT_EQ(buffer_, ptr);
  EXPECT_EQ(8, size);

  // Peeking again should return the same pointer.
  coded_input.GetDirectBufferPointerInline(&ptr, &size);
  EXPECT_EQ(buffer_, ptr);
  EXPECT_EQ(8, size);

  // Skip forward in the same buffer then peek again.
  EXPECT_TRUE(coded_input.Skip(3));
  coded_input.GetDirectBufferPointerInline(&ptr, &size);
  EXPECT_EQ(buffer_ + 3, ptr);
  EXPECT_EQ(5, size);

  // Skip to end of buffer and peek -- should return false and provide an empty
  // buffer. It does not try to Refresh().
  EXPECT_TRUE(coded_input.Skip(5));
  coded_input.GetDirectBufferPointerInline(&ptr, &size);
  EXPECT_EQ(buffer_ + 8, ptr);
  EXPECT_EQ(0, size);
}

// -------------------------------------------------------------------
// Limits

TEST_P(BlockSizes, BasicLimit) {
  int kBlockSizes_case = GetParam();
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    EXPECT_EQ(-1, coded_input.BytesUntilLimit());
    CodedInputStream::Limit limit = coded_input.PushLimit(8);

    // Read until we hit the limit.
    uint32_t value;
    EXPECT_EQ(8, coded_input.BytesUntilLimit());
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(4, coded_input.BytesUntilLimit());
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());
    EXPECT_FALSE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());

    coded_input.PopLimit(limit);

    EXPECT_EQ(-1, coded_input.BytesUntilLimit());
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
  }

  EXPECT_EQ(12, input.ByteCount());
}

// Test what happens when we push two limits where the second (top) one is
// shorter.
TEST_P(BlockSizes, SmallLimitOnTopOfBigLimit) {
  int kBlockSizes_case = GetParam();
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    EXPECT_EQ(-1, coded_input.BytesUntilLimit());
    CodedInputStream::Limit limit1 = coded_input.PushLimit(8);
    EXPECT_EQ(8, coded_input.BytesUntilLimit());
    CodedInputStream::Limit limit2 = coded_input.PushLimit(4);

    uint32_t value;

    // Read until we hit limit2, the top and shortest limit.
    EXPECT_EQ(4, coded_input.BytesUntilLimit());
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());
    EXPECT_FALSE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());

    coded_input.PopLimit(limit2);

    // Read until we hit limit1.
    EXPECT_EQ(4, coded_input.BytesUntilLimit());
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());
    EXPECT_FALSE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());

    coded_input.PopLimit(limit1);

    // No more limits.
    EXPECT_EQ(-1, coded_input.BytesUntilLimit());
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
  }

  EXPECT_EQ(12, input.ByteCount());
}

// Test what happens when we push two limits where the second (top) one is
// longer.  In this case, the top limit is shortened to match the previous
// limit.
TEST_P(BlockSizes, BigLimitOnTopOfSmallLimit) {
  int kBlockSizes_case = GetParam();
  ArrayInputStream input(buffer_, sizeof(buffer_), kBlockSizes_case);

  {
    CodedInputStream coded_input(&input);

    EXPECT_EQ(-1, coded_input.BytesUntilLimit());
    CodedInputStream::Limit limit1 = coded_input.PushLimit(4);
    EXPECT_EQ(4, coded_input.BytesUntilLimit());
    CodedInputStream::Limit limit2 = coded_input.PushLimit(8);

    uint32_t value;

    // Read until we hit limit2.  Except, wait!  limit1 is shorter, so
    // we end up hitting that first, despite having 4 bytes to go on
    // limit2.
    EXPECT_EQ(4, coded_input.BytesUntilLimit());
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());
    EXPECT_FALSE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());

    coded_input.PopLimit(limit2);

    // OK, popped limit2, now limit1 is on top, which we've already hit.
    EXPECT_EQ(0, coded_input.BytesUntilLimit());
    EXPECT_FALSE(coded_input.ReadLittleEndian32(&value));
    EXPECT_EQ(0, coded_input.BytesUntilLimit());

    coded_input.PopLimit(limit1);

    // No more limits.
    EXPECT_EQ(-1, coded_input.BytesUntilLimit());
    EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
  }

  EXPECT_EQ(8, input.ByteCount());
}

TEST_F(CodedStreamTest, ExpectAtEnd) {
  // Test ExpectAtEnd(), which is based on limits.
  ArrayInputStream input(buffer_, sizeof(buffer_));
  CodedInputStream coded_input(&input);

  EXPECT_FALSE(coded_input.ExpectAtEnd());

  CodedInputStream::Limit limit = coded_input.PushLimit(4);

  uint32_t value;
  EXPECT_TRUE(coded_input.ReadLittleEndian32(&value));
  EXPECT_TRUE(coded_input.ExpectAtEnd());

  coded_input.PopLimit(limit);
  EXPECT_FALSE(coded_input.ExpectAtEnd());
}

TEST_F(CodedStreamTest, NegativeLimit) {
  // Check what happens when we push a negative limit.
  ArrayInputStream input(buffer_, sizeof(buffer_));
  CodedInputStream coded_input(&input);

  CodedInputStream::Limit limit = coded_input.PushLimit(-1234);
  // BytesUntilLimit() returns -1 to mean "no limit", which actually means
  // "the limit is INT_MAX relative to the beginning of the stream".
  EXPECT_EQ(-1, coded_input.BytesUntilLimit());
  coded_input.PopLimit(limit);
}

TEST_F(CodedStreamTest, NegativeLimitAfterReading) {
  // Check what happens when we push a negative limit.
  ArrayInputStream input(buffer_, sizeof(buffer_));
  CodedInputStream coded_input(&input);
  ASSERT_TRUE(coded_input.Skip(128));

  CodedInputStream::Limit limit = coded_input.PushLimit(-64);
  // BytesUntilLimit() returns -1 to mean "no limit", which actually means
  // "the limit is INT_MAX relative to the beginning of the stream".
  EXPECT_EQ(-1, coded_input.BytesUntilLimit());
  coded_input.PopLimit(limit);
}

TEST_F(CodedStreamTest, OverflowLimit) {
  // Check what happens when we push a limit large enough that its absolute
  // position is more than 2GB into the stream.
  ArrayInputStream input(buffer_, sizeof(buffer_));
  CodedInputStream coded_input(&input);
  ASSERT_TRUE(coded_input.Skip(128));

  CodedInputStream::Limit limit = coded_input.PushLimit(INT_MAX);
  // BytesUntilLimit() returns -1 to mean "no limit", which actually means
  // "the limit is INT_MAX relative to the beginning of the stream".
  EXPECT_EQ(-1, coded_input.BytesUntilLimit());
  coded_input.PopLimit(limit);
}

TEST_F(CodedStreamTest, TotalBytesLimit) {
  ArrayInputStream input(buffer_, sizeof(buffer_));
  CodedInputStream coded_input(&input);
  coded_input.SetTotalBytesLimit(16);
  EXPECT_EQ(16, coded_input.BytesUntilTotalBytesLimit());

  std::string str;
  EXPECT_TRUE(coded_input.ReadString(&str, 16));
  EXPECT_EQ(0, coded_input.BytesUntilTotalBytesLimit());

  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(
        log,
        Log(absl::LogSeverity::kError, testing::_,
            testing::HasSubstr(
                "A protocol message was rejected because it was too big")));
    log.StartCapturingLogs();
    EXPECT_FALSE(coded_input.ReadString(&str, 1));
  }

  coded_input.SetTotalBytesLimit(32);
  EXPECT_EQ(16, coded_input.BytesUntilTotalBytesLimit());
  EXPECT_TRUE(coded_input.ReadString(&str, 16));
  EXPECT_EQ(0, coded_input.BytesUntilTotalBytesLimit());
}

TEST_F(CodedStreamTest, TotalBytesLimitNotValidMessageEnd) {
  // total_bytes_limit_ is not a valid place for a message to end.

  ArrayInputStream input(buffer_, sizeof(buffer_));
  CodedInputStream coded_input(&input);

  // Set both total_bytes_limit and a regular limit at 16 bytes.
  coded_input.SetTotalBytesLimit(16);
  CodedInputStream::Limit limit = coded_input.PushLimit(16);

  // Read 16 bytes.
  std::string str;
  EXPECT_TRUE(coded_input.ReadString(&str, 16));

  // Read a tag.  Should fail, but report being a valid endpoint since it's
  // a regular limit.
  EXPECT_EQ(0, coded_input.ReadTagNoLastTag());
  EXPECT_TRUE(coded_input.ConsumedEntireMessage());

  // Pop the limit.
  coded_input.PopLimit(limit);

  // Read a tag.  Should fail, and report *not* being a valid endpoint, since
  // this time we're hitting the total bytes limit.
  EXPECT_EQ(0, coded_input.ReadTagNoLastTag());
  EXPECT_FALSE(coded_input.ConsumedEntireMessage());
}

TEST_F(CodedStreamTest, RecursionLimit) {
  ArrayInputStream input(buffer_, sizeof(buffer_));
  CodedInputStream coded_input(&input);
  coded_input.SetRecursionLimit(4);

  // This is way too much testing for a counter.
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 1
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 2
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 3
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 4
  EXPECT_FALSE(coded_input.IncrementRecursionDepth());  // 5
  EXPECT_FALSE(coded_input.IncrementRecursionDepth());  // 6
  coded_input.DecrementRecursionDepth();                // 5
  EXPECT_FALSE(coded_input.IncrementRecursionDepth());  // 6
  coded_input.DecrementRecursionDepth();                // 5
  coded_input.DecrementRecursionDepth();                // 4
  coded_input.DecrementRecursionDepth();                // 3
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 4
  EXPECT_FALSE(coded_input.IncrementRecursionDepth());  // 5
  coded_input.DecrementRecursionDepth();                // 4
  coded_input.DecrementRecursionDepth();                // 3
  coded_input.DecrementRecursionDepth();                // 2
  coded_input.DecrementRecursionDepth();                // 1
  coded_input.DecrementRecursionDepth();                // 0
  coded_input.DecrementRecursionDepth();                // 0
  coded_input.DecrementRecursionDepth();                // 0
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 1
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 2
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 3
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 4
  EXPECT_FALSE(coded_input.IncrementRecursionDepth());  // 5

  coded_input.SetRecursionLimit(6);
  EXPECT_TRUE(coded_input.IncrementRecursionDepth());   // 6
  EXPECT_FALSE(coded_input.IncrementRecursionDepth());  // 7
}


class ReallyBigInputStream : public ZeroCopyInputStream {
 public:
  ReallyBigInputStream() : backup_amount_(0), buffer_count_(0) {}

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size) override {
    // We only expect BackUp() to be called at the end.
    EXPECT_EQ(0, backup_amount_);

    switch (buffer_count_++) {
      case 0:
        *data = buffer_.get();
        *size = 1024;
        return true;
      case 1:
        // Return an enormously large buffer that, when combined with the 1k
        // returned already, should overflow the total_bytes_read_ counter in
        // CodedInputStream.
        *data = buffer_.get();
        *size = INT_MAX;
        return true;
      default:
        return false;
    }
  }

  void BackUp(int count) override { backup_amount_ = count; }

  bool Skip(int count) override {
    ABSL_LOG(FATAL) << "Not implemented.";
    return false;
  }
  int64_t ByteCount() const override {
    ABSL_LOG(FATAL) << "Not implemented.";
    return 0;
  }

  int backup_amount_;

 private:
  // On 32-bit platforms, only allocate 1024 bytes because we would exhaust our
  // address space otherwise. This will result in UB when
  // CodedInputStream::Refresh computes an out-of-bounds pointer by adding
  // INT_MAX (from case 1 above) but this is largely only a theoretical issue.
  std::unique_ptr<char[], void (*)(void*)> buffer_{
      static_cast<char*>(calloc(sizeof(void*) > 4 ? INT_MAX : 1024, 1)), free};
  int64_t buffer_count_;
};

TEST_F(CodedStreamTest, InputOver2G) {
  // CodedInputStream should gracefully handle input over 2G and call
  // input.BackUp() with the correct number of bytes on destruction.
  ReallyBigInputStream input;

  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
    log.StartCapturingLogs();
    CodedInputStream coded_input(&input);
    std::string str;
    EXPECT_TRUE(coded_input.ReadString(&str, 512));
    EXPECT_TRUE(coded_input.ReadString(&str, 1024));
  }

  EXPECT_EQ(INT_MAX - 512, input.backup_amount_);
}

INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, ResetCords, testing::ValuesIn(kResetCords),
    [](const testing::TestParamInfo<ResetCords::ParamType>& param_info) {
      return absl::StrCat("ResetCords_", param_info.param ? "true" : "false");
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, BlockSizesWithResetCords,
    testing::Combine(testing::ValuesIn(kBlockSizes),
                     testing::ValuesIn(kResetCords)),
    [](const testing::TestParamInfo<BlockSizesWithResetCords::ParamType>&
           param_info) {
      return absl::StrCat("BlockSize_", std::get<0>(param_info.param),
                          "_ResetCords_",
                          std::get<1>(param_info.param) ? "true" : "false");
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, VarintErrorCasesWithSizes,
    testing::Combine(testing::ValuesIn(kVarintErrorCases),
                     testing::ValuesIn(kBlockSizes)),
    [](const testing::TestParamInfo<VarintErrorCasesWithSizes::ParamType>&
           param_info) {
      return absl::StrCat("VarintErrorCase_",
                          std::get<0>(param_info.param).name, "_BlockSize_",
                          std::get<1>(param_info.param));
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, VarintSizeCases, testing::ValuesIn(kVarintSizeCases),
    [](const testing::TestParamInfo<VarintSizeCases::ParamType>& param_info) {
      return absl::StrCat("VarintSizeCase_Value_", param_info.param.value);
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, VarintCases, testing::ValuesIn(kVarintCases),
    [](const testing::TestParamInfo<VarintCases::ParamType>& param_info) {
      return absl::StrCat("VarintCase_Value_", param_info.param.value);
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, VarintCasesWithSizes,
    testing::Combine(testing::ValuesIn(kVarintCases),
                     testing::ValuesIn(kBlockSizes)),
    [](const testing::TestParamInfo<VarintCasesWithSizes::ParamType>&
           param_info) {
      return absl::StrCat("VarintCase_Value_",
                          std::get<0>(param_info.param).value, "_BlockSize_",
                          std::get<1>(param_info.param));
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, BlockSizes, testing::ValuesIn(kBlockSizes),
    [](const testing::TestParamInfo<BlockSizes::ParamType>& param_info) {
      return absl::StrCat("BlockSize_", param_info.param);
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, SignedVarintCasesWithSizes,
    testing::Combine(testing::ValuesIn(kSignExtendedVarintCases),
                     testing::ValuesIn(kBlockSizes)),
    [](const testing::TestParamInfo<SignedVarintCasesWithSizes::ParamType>&
           param_info) {
      return absl::StrCat("SignedVarintCase_Value_",
                          std::get<0>(param_info.param) < 0 ? "_Negative_" : "",
                          std::abs(std::get<0>(param_info.param)),
                          "_BlockSize_", std::get<1>(param_info.param));
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, Fixed16Cases, testing::ValuesIn(kFixed16Cases),
    [](const testing::TestParamInfo<Fixed16Cases::ParamType>& param_info) {
      return absl::StrCat("Fixed16Case_Value_", param_info.param.value);
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, Fixed32Cases, testing::ValuesIn(kFixed32Cases),
    [](const testing::TestParamInfo<Fixed32Cases::ParamType>& param_info) {
      return absl::StrCat("Fixed32Case_Value_", param_info.param.value);
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, Fixed64Cases, testing::ValuesIn(kFixed64Cases),
    [](const testing::TestParamInfo<Fixed64Cases::ParamType>& param_info) {
      return absl::StrCat("Fixed64Case_Value_", param_info.param.value);
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, Fixed16CasesWithSizes,
    testing::Combine(testing::ValuesIn(kFixed16Cases),
                     testing::ValuesIn(kBlockSizes)),
    [](const testing::TestParamInfo<Fixed16CasesWithSizes::ParamType>&
           param_info) {
      return absl::StrCat("Fixed16Case_Value_",
                          std::get<0>(param_info.param).value, "_BlockSize_",
                          std::get<1>(param_info.param));
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, Fixed32CasesWithSizes,
    testing::Combine(testing::ValuesIn(kFixed32Cases),
                     testing::ValuesIn(kBlockSizes)),
    [](const testing::TestParamInfo<Fixed32CasesWithSizes::ParamType>&
           param_info) {
      return absl::StrCat("Fixed32Case_Value_",
                          std::get<0>(param_info.param).value, "_BlockSize_",
                          std::get<1>(param_info.param));
    });
INSTANTIATE_TEST_SUITE_P(
    CodedStreamUnitTest, Fixed64CasesWithSizes,
    testing::Combine(testing::ValuesIn(kFixed64Cases),
                     testing::ValuesIn(kBlockSizes)),
    [](const testing::TestParamInfo<Fixed64CasesWithSizes::ParamType>&
           param_info) {
      return absl::StrCat("Fixed64Case_Value_",
                          std::get<0>(param_info.param).value, "_BlockSize_",
                          std::get<1>(param_info.param));
    });

}  // namespace
}  // namespace io
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
