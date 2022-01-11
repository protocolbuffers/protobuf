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

#include <google/protobuf/generated_message_tctable_impl.h>
#include <google/protobuf/wire_format_lite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {

namespace {

using ::testing::Eq;
using ::testing::Not;

MATCHER_P2(IsEntryForFieldNum, table, field_num,
           StrCat(negation ? "isn't " : "",
                        "the field entry for field number ", field_num)) {
  if (arg == nullptr) {
    *result_listener << "which is nullptr";
    return false;
  }
  // Use the entry's index to compare field numbers.
  size_t index = static_cast<const TcParseTableBase::FieldEntry*>(arg) -
                 &table->field_entries[0];
  uint32_t actual_field_num = table->field_numbers[index];
  if (actual_field_num != field_num) {
    *result_listener << "which is the entry for " << actual_field_num;
    return false;
  }
  return true;
}

TEST(IsEntryForFieldNumTest, Matcher) {
  TcParseTable<0, 3> table;
  table.field_numbers = {1, 2, 3};

  EXPECT_THAT(&table.field_entries[0], IsEntryForFieldNum(&table, 1));
  EXPECT_THAT(&table.field_entries[2], IsEntryForFieldNum(&table, 3));
  EXPECT_THAT(&table.field_entries[1], Not(IsEntryForFieldNum(&table, 3)));

  EXPECT_THAT(nullptr, Not(IsEntryForFieldNum(&table, 1)));
}

}  // namespace

class FindFieldEntryTest : public ::testing::Test {
 protected:
  // Calls the private `FindFieldEntry` function.
  template <size_t kNumFast, size_t kNumEntries>
  const TcParseTableBase::FieldEntry* FindFieldEntry(
      const TcParseTable<kNumFast, kNumEntries>& table, uint32_t tag) {
    return TcParser::FindFieldEntry(&table.header, tag);
  }

  // Returns the number of fields scanned during a small scan.
  static constexpr int small_scan_size() { return TcParser::kMtSmallScanSize; }
};

TEST_F(FindFieldEntryTest, SequentialFieldRange) {
  // Look up fields that are within the range of `num_sequential_fields`.
  TcParseTable<0, 5> table = {
      // header:
      {
          0, 0, 0, 0,  // has_bits_offset, extensions
          111,         // max_field_number
          0, 4,        // fast_idx_mask, num_sequential_fields
          2, 5,        // sequential_fields_start, num_field_entries
      },
      {},  // fast_entries
      // field_numbers:
      {{2, 3, 4, 5, 111}},
  };

  for (int i : table.field_numbers) {
    EXPECT_THAT(FindFieldEntry(table, i), IsEntryForFieldNum(&table, i));
  }
  for (int i : {0, 1, 6, 7, 110, 112, 500000000}) {
    GOOGLE_LOG(WARNING) << "Field " << i;
    EXPECT_THAT(FindFieldEntry(table, i), Eq(nullptr));
  }
}

TEST_F(FindFieldEntryTest, SmallScanRange) {
  // Look up fields past `num_sequential_fields`, but before binary search.
  ASSERT_THAT(small_scan_size(), Eq(4)) << "test needs to be updated";

  TcParseTable<0, 6> table = {
      // header:
      {
          0, 0, 0, 0,  // has_bits_offset, extensions
          111,         // max_field_number
          0, 1,        // fast_idx_mask, num_sequential_fields
          1, 6,        // sequential_fields_start, num_field_entries
      },
      {},  // fast_entries
      // field_numbers:
      {{// Sequential entries:
        1,
        // Small scan range:
        3, 4, 5, 7,
        // Binary search range:
        111}},
  };

  for (int i : table.field_numbers) {
    EXPECT_THAT(FindFieldEntry(table, i), IsEntryForFieldNum(&table, i));
  }
  for (int i : {0, 2, 6, 8, 9, 110, 112, 500000000}) {
    EXPECT_THAT(FindFieldEntry(table, i), Eq(nullptr));
  }
}

TEST_F(FindFieldEntryTest, BinarySearchRange) {
  // Fields after the sequential and small-scan ranges are looked up using
  // binary search.
  ASSERT_THAT(small_scan_size(), Eq(4)) << "test needs to be updated";

  TcParseTable<0, 10> table = {
      // header:
      {
          0, 0, 0, 0,  // has_bits_offset, extensions
          70,          // max_field_number
          0, 1,        // fast_idx_mask, num_sequential_fields
          1, 10,       // sequential_fields_start, num_field_entries
      },
      {},  // fast_entries
      // field_numbers:
      {{// Sequential entries:
        1,
        // Small scan range:
        3, 4, 5, 6,
        // Binary search range:
        8, 9, 11, 12, 70}},
  };
  for (int i : table.field_numbers) {
    EXPECT_THAT(FindFieldEntry(table, i), IsEntryForFieldNum(&table, i));
  }
  for (int i : {0, 2, 7, 10, 13, 69, 71, 112, 500000000}) {
    EXPECT_THAT(FindFieldEntry(table, i), Eq(nullptr));
  }
}

TEST_F(FindFieldEntryTest, OutOfRange) {
  // Look up tags that are larger than the maximum in the message.
  TcParseTable<0, 3> table = {
      // header:
      {
          0, 0, 0, 0,  // has_bits_offset, extensions
          3,           // max_field_number
          0, 3,        // fast_idx_mask, num_sequential_fields
          1, 3,        // sequential_fields_start, num_field_entries
      },
      {},  // fast_entries
      // field_numbers:
      {{1, 2, 3}},
  };

  for (int i : table.field_numbers) {
    EXPECT_THAT(FindFieldEntry(table, i), IsEntryForFieldNum(&table, i));
  }
  for (int i : {0, 4, 112, 500000000}) {
    EXPECT_THAT(FindFieldEntry(table, i), Eq(nullptr));
  }
}

TEST_F(FindFieldEntryTest, EmptyMessage) {
  // Ensure that tables with no fields are handled correctly.
  TcParseTable<0, 0> table = {{0}};

  for (int i : {0, 4, 112, 500000000}) {
    EXPECT_THAT(FindFieldEntry(table, i), Eq(nullptr));
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
