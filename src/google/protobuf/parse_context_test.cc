// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/parse_context.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/test_zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/wire_format_lite.h"
#include "utf8_validity.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

constexpr int kTooLargeSize =
    static_cast<int>(std::numeric_limits<uint32_t>::max());
constexpr int kMaxLimit = std::numeric_limits<int>::max();

TEST(ParseContextTest, NoOverflowMaxDepth) {
  constexpr char kPayload[] =
      "\x81\x81\x81\x81\x08\x01\x00";  // Malformed varint size.

  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, kPayload);
  proto2_unittest::TestAllTypes proto;
  const char* result = ctx.ParseMessage(&proto, kPayload);

  ASSERT_EQ(result, nullptr);
}

TEST(ParseContextTest, VerifyUTF8) {
  constexpr absl::string_view kPayload = "hello world, a valid UTF8 payload";

  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, kPayload);

  ptr = ctx.VerifyUTF8(ptr, static_cast<int>(kPayload.size()));

  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(ctx.DataAvailable(ptr), 0);
}

TEST(ParseContextTest, VerifyFragmentedUTF8) {
  constexpr absl::string_view kPayload =
      "hello world, a valid UTF8 payload #1"
      "hello world, a valid UTF8 payload #2"
      "hello world, a valid UTF8 payload #3"
      "hello world, a valid UTF8 payload #4"
      "hello world, a valid UTF8 payload #5";

  const char* ptr;
  io::ArrayInputStream ais(kPayload.data(), kPayload.size(), /*block_size*/ 13);
  ParseContext ctx(kMaxLimit, false, &ptr, &ais);

  ptr = ctx.VerifyUTF8(ptr, static_cast<int>(kPayload.size()));

  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(ctx.DataAvailable(ptr), 0);
}

TEST(ParseContextTest, FailsOnInvalidUTF8) {
  constexpr absl::string_view kInvalidUtf8 = "\xe2\x82\x28";

  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, kInvalidUtf8);

  ptr = ctx.VerifyUTF8(ptr, static_cast<int>(kInvalidUtf8.size()));

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, FailsOnFragmentedInvalidUTF8) {
  constexpr absl::string_view kInvalidUtf8 =
      "hello world, a valid UTF8 payload #1"
      "hello world, a valid UTF8 payload #2"
      "hello world, a valid UTF8 payload #3"
      "hello world, a valid UTF8 payload #4"
      "hello world, a valid UTF8 payload #5\xe2\x82\x28";

  const char* ptr;
  io::ArrayInputStream ais(kInvalidUtf8.data(), kInvalidUtf8.size(),
                           /*block_size*/ 13);
  ParseContext ctx(kMaxLimit, false, &ptr, &ais);

  ptr = ctx.VerifyUTF8(ptr, static_cast<int>(kInvalidUtf8.size()));

  EXPECT_EQ(ptr, nullptr);
  EXPECT_GT(ctx.DataAvailable(ptr), 0);
}

TEST(ParseContextTest, SkipTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");

  ptr = ctx.Skip(ptr, kTooLargeSize);

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, ReadStringTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");
  std::string out;

  ptr = ctx.ReadString(ptr, kTooLargeSize, &out);

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, AppendStringTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");
  std::string out;

  ptr = ctx.AppendString(ptr, kTooLargeSize, &out);

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, ReadCordTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");
  absl::Cord out;

  ptr = ctx.ReadCord(ptr, kTooLargeSize, &out);

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, VerifyUTF8TooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");

  ptr = ctx.VerifyUTF8(ptr, static_cast<uint32_t>(kTooLargeSize));

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, ReadChunkAndCallbackTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");

  ptr = ctx.ReadChunkAndCallback(ptr, kTooLargeSize,
                                 [&](const char* ptr, int s) {});

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, ReadCordMustWorkAcrossFragments) {
  const char* ptr;
  io::internal::TestZeroCopyInputStream stream({"hello ", "world"});
  ParseContext ctx(kMaxLimit, false, &ptr, &stream);
  absl::Cord out;
  ptr = ctx.ReadCord(ptr, 7, &out);
  EXPECT_EQ(out, "hello w");
}

TEST(ParseContextTest, ReadCordFromMultipleFragments) {
  const char* ptr;
  io::internal::TestZeroCopyInputStream stream({"a", "bc", "def"});
  ParseContext ctx(kMaxLimit, false, &ptr, &stream);
  absl::Cord out;
  ptr = ctx.ReadCord(ptr, 6, &out);
  EXPECT_EQ(out, "abcdef");
}

TEST(ParseContextTest, ReadCordFromMultipleFragmentsWithEmptyFragment) {
  const char* ptr;
  io::internal::TestZeroCopyInputStream stream({"a", "bc", "", "", "def"});
  ParseContext ctx(kMaxLimit, false, &ptr, &stream);
  absl::Cord out;
  ptr = ctx.ReadCord(ptr, 6, &out);
  EXPECT_EQ(out, "abcdef");
}


template <typename DataT, typename PeekFunc>
void TestAdvancePtrSinkingAndPeeking(
    const std::vector<std::string>& input, std::vector<int> advance_by,
    const PeekFunc& peek_func = [](absl::string_view) { return true; }) {
  std::string flattened = absl::StrJoin(input, "");
  // To fully traverse the input.
  int available = static_cast<int>(flattened.size());
  // To ensure that we can cover the entire data.
  advance_by.push_back(available);

  io::internal::TestZeroCopyInputStream stream(input);
  const char* ptr;
  ParseContext ctx(io::CodedInputStream::GetDefaultRecursionLimit(), false,
                   &ptr, &stream);
  if (ctx.Done(&ptr)) return;

  WireFormatStringSink sink(ptr);
  sink.data.reserve(flattened.size());
  for (auto advance : advance_by) {
    int adv = std::min(available, advance);
    available -= adv;
    ABSL_CHECK_GE(adv, 0);
    ptr = ctx.AdvancePtrMaybeFlush<DataT>(ptr, adv, sink, peek_func);
    if (!ptr) break;

    if (available == 0) {
      sink.Flush(ptr);
      break;
    }
  }

  // Output must match the flattened input.
  EXPECT_EQ(sink.data, flattened);
}

void TestAdvancePtrPeeking(const std::vector<std::string>& input,
                           std::vector<int> advance_by) {
  // Peeked data must match the flattened input.
  std::string peeked;

  TestAdvancePtrSinkingAndPeeking<char>(input, advance_by,
                                        [&peeked](absl::string_view view) {
                                          absl::StrAppend(&peeked, view);
                                          return true;
                                        });

  EXPECT_EQ(peeked, absl::StrJoin(input, ""));
}

TEST(ParseAndFlushTest, SmallViewInput) {
  TestAdvancePtrPeeking({"hello world"}, {});
}

TEST(ParseAndFlushTest, SmallViewInputIncremental) {
  TestAdvancePtrPeeking({"hello world"}, {1, 2, 3, 4, 5});
}

TEST(ParseAndFlushTest, LargeViewInput) {
  constexpr absl::string_view kPayload =
      "hello world, a valid UTF8 payload #1"
      "hello world, a valid UTF8 payload #2"
      "hello world, a valid UTF8 payload #3"
      "hello world, a valid UTF8 payload #4"
      "hello world, a valid UTF8 payload #5";

  TestAdvancePtrPeeking({std::string{kPayload}}, {});
}

TEST(ParseAndFlushTest, LargeViewInputIncremental) {
  constexpr absl::string_view kPayload =
      "hello world, a valid UTF8 payload #1"
      "hello world, a valid UTF8 payload #2"
      "hello world, a valid UTF8 payload #3"
      "hello world, a valid UTF8 payload #4"
      "hello world, a valid UTF8 payload #5";

  TestAdvancePtrPeeking({std::string{kPayload}}, {1, 3, 5, 7, 9, 11});
}


TEST(ParseAndFlush, TestAdvancePtrPeekingRegression1) {
  // "ptr" from ParseContext may not be flush-ready if the fragmented input has
  // empty fragments. This is not plausible in practice.
  TestAdvancePtrPeeking({"", "", "", "", "0", "pppppp", ""}, {50});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression2) {
  // From fragmented input, advancing ptr by greater than kSlopBytes is not
  // safe.
  TestAdvancePtrPeeking(
      {"", "\364\364", "###", "\022", "\223\036", "\366", "\366", "\366",
       "\366", "\366", "\366", "\366", "\366", "\366", "\024", "\223"},
      {1, 50});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression3) {
  TestAdvancePtrPeeking({"\315\315", ""}, {3});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression4) {
  TestAdvancePtrPeeking({"\323", "\323", "", "\033", "", ""}, {});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression5) {
  TestAdvancePtrPeeking(
      {"9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9",
       "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9"},
      {2});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression6) {
  TestAdvancePtrPeeking(
      {"\314", "\314", "\314", "\314", "\314", "\314", "\314", "\327", "\327",
       "\327", "\327", "\327", "\327", "\314", "\314", "\314", "\271", "\017"},
      {16, 16, 16, 6, 16, 12, 16, 16, 16, 16, 16,
       16, 16, 16, 2, 6,  16, 16, 16, 16, 16, 16});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression7) {
  TestAdvancePtrPeeking(
      {"", "\244\030",
       "\361\361\361\361\361\361\361\361\361\361\361\361\361\361\361\361\361"
       "\361\361\361\361\361\361\361",
       "\301", "9\300", "\332\332\332\332\332\330\330\300", "\250", "\235", "",
       "\026"},
      {13, 1, 1, 9,  1,  12, 16, 14, 8, 7,  6, 8, 1,  14, 16, 1, 16, 13, 16,
       1,  1, 1, 14, 16, 4,  1,  1,  9, 12, 1, 1, 16, 1,  1,  1, 16, 7});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression8) {
  TestAdvancePtrPeeking(
      {"", "\343\343\343\343\343\343\343",
       "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||", "&",
       "\360", "\207\207\207\207\207\207\207\207\207\207", "\352", "\340", ",",
       "((((((", "\275", ""},
      {1,  1,  8,  4,  1,  1,  4,  4,  4,  4,  4,  10, 10, 14,
       13, 7,  3,  16, 11, 12, 16, 15, 16, 10, 15, 15, 14, 14,
       11, 14, 10, 2,  16, 14, 14, 14, 8,  14, 14, 14});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression9) {
  TestAdvancePtrPeeking(
      {"D", "", "NNN", "J", "\270", "\200\200", "\021\021", "\320\320\320",
       "\227",
       "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
       "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
       "ff",
       "\331", "&", "L", "", "\237", "", "\311", "\353"},
      {96, 96, 96, 96, 96, 96, 96, 96, 96, 96,  96, 96, 96, 96, 96, 96,
       96, 30, 30, 30, 30, 30, 30, 30, 28, 30,  30, 30, 30, 30, 30, 1,
       26, 9,  31, 57, 38, 34, 97, 96, 96, 100, 96, 96, 96, 96, 96, 100,
       96, 96, 7,  99, 96, 96, 96, 96, 96, 96,  96, 96, 1,  96, 96, 96,
       96, 96, 96, 96, 96, 96, 96, 44, 96, 61,  66, 14});
}

void VerifyUTF8AndCopy(const std::vector<std::string>& input) {
  std::string flattened = absl::StrJoin(input, "");
  // To fully traverse the input.
  io::internal::TestZeroCopyInputStream stream(input);
  const char* ptr;
  ParseContext ctx(io::CodedInputStream::GetDefaultRecursionLimit(), false,
                   &ptr, &stream);

  WireFormatStringSink sink(ptr);
  sink.data.reserve(flattened.size());
  ptr = ctx.VerifyUTF8MaybeFlush(ptr, static_cast<int64_t>(flattened.size()),
                                 sink);
  if (ptr == nullptr) {
    EXPECT_FALSE(utf8_range::IsStructurallyValid(flattened));
    return;
  }

  sink.Flush(ptr);

  // UTF8 validation must match the flattened input.
  EXPECT_TRUE(utf8_range::IsStructurallyValid(flattened));
  // If successful, the output must match the flattened input.
  EXPECT_EQ(sink.data, flattened);
}

TEST(VerifyUTF8AndFlush, ValidInput) { VerifyUTF8AndCopy({"hello world!"}); }

TEST(VerifyUTF8AndFlush, InvalidInput) {
  constexpr absl::string_view kInvalidUtf8 = "\xe2\x82\x28";
  VerifyUTF8AndCopy({std::string(kInvalidUtf8)});
}

TEST(VerifyUTF8AndFlush, InvalidSplit) {
  VerifyUTF8AndCopy({std::string("\xe2\x82"), std::string("\x28")});
}

TEST(VerifyUTF8AndFlush, IncompleteUtf8) {
  ASSERT_FALSE(utf8_range::IsStructurallyValid("\xf0"));
  VerifyUTF8AndCopy({std::string("\xf0")});
}

TEST(VerifyUTF8AndFlush, InvalidSeparatelyButValidTogether) {
  ASSERT_TRUE(utf8_range::IsStructurallyValid("\xf0\x9f\x98\x80"));
  VerifyUTF8AndCopy({std::string("\xf0"), std::string("\x9f\x98\x80")});
}


template <typename T>
RepeatedField<T>* MutablePackedFixedField(
    proto2_unittest::TestPackedTypes* msg) {
  if constexpr (std::is_same_v<T, uint32_t>) {
    return msg->mutable_packed_fixed32();
  } else {
    static_assert(std::is_same_v<T, uint64_t>);
    return msg->mutable_packed_fixed64();
  }
}

template <typename T>
std::vector<T> MakeTestValues(size_t count, uint64_t seed = 0) {
  std::vector<T> values(count);
  for (size_t i = 0; i < count; ++i) {
    values[i] = static_cast<T>((i + seed) * 2654435761ULL + 17ULL);
  }
  return values;
}

template <typename T>
std::string MakePackedFixedPayload(const std::vector<T>& values) {
  std::string payload(values.size() * sizeof(T), '\0');
  uint8_t* dst = reinterpret_cast<uint8_t*>(payload.data());
  for (T v : values) {
    if constexpr (sizeof(T) == 4) {
      dst = io::CodedOutputStream::WriteLittleEndian32ToArray(
          static_cast<uint32_t>(v), dst);
    } else {
      static_assert(sizeof(T) == 8);
      dst = io::CodedOutputStream::WriteLittleEndian64ToArray(
          static_cast<uint64_t>(v), dst);
    }
  }
  return payload;
}

template <typename T>
class ReadPackedFixedTest : public ::testing::Test {};

using PackedFixedTypes = ::testing::Types<uint32_t, uint64_t>;
TYPED_TEST_SUITE(ReadPackedFixedTest, PackedFixedTypes);

TYPED_TEST(ReadPackedFixedTest,
           MultiChunkMatchesSingleChunkAndSinglePassArenaAllocation) {
  constexpr size_t kNumElements = 1024;
  const std::vector<TypeParam> expected =
      MakeTestValues<TypeParam>(kNumElements);
  const std::string payload = MakePackedFixedPayload(expected);
  const int payload_bytes = static_cast<int>(payload.size());

  // Single-chunk baseline on Heap.
  proto2_unittest::TestPackedTypes single_heap_msg;
  RepeatedField<TypeParam>* single_heap_out =
      MutablePackedFixedField<TypeParam>(&single_heap_msg);
  {
    io::ArrayInputStream ais(payload.data(), payload_bytes);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    ptr = ctx.ReadPackedFixed(single_heap_msg, ptr, payload_bytes,
                              single_heap_out);
    ASSERT_NE(ptr, nullptr);
  }
  ASSERT_EQ(
      std::vector<TypeParam>(single_heap_out->begin(), single_heap_out->end()),
      expected);

  // Single-chunk baseline on Arena.
  Arena single_arena;
  auto* single_arena_msg =
      Arena::Create<proto2_unittest::TestPackedTypes>(&single_arena);
  RepeatedField<TypeParam>* single_arena_out =
      MutablePackedFixedField<TypeParam>(single_arena_msg);
  {
    io::ArrayInputStream ais(payload.data(), payload_bytes);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    ptr = ctx.ReadPackedFixed(*single_arena_msg, ptr, payload_bytes,
                              single_arena_out);
    ASSERT_NE(ptr, nullptr);
  }
  ASSERT_EQ(std::vector<TypeParam>(single_arena_out->begin(),
                                   single_arena_out->end()),
            expected);

  // Multi-chunk ArrayInputStream across various block sizes (including
  // unaligned block_size = 7).
  for (int block_size : {7, 16, 64, 256}) {
    proto2_unittest::TestPackedTypes multi_heap_msg;
    RepeatedField<TypeParam>* multi_heap_out =
        MutablePackedFixedField<TypeParam>(&multi_heap_msg);
    {
      io::ArrayInputStream ais(payload.data(), payload_bytes, block_size);
      const char* ptr = nullptr;
      ParseContext ctx(kMaxLimit, false, &ptr, &ais);
      ptr = ctx.ReadPackedFixed(multi_heap_msg, ptr, payload_bytes,
                                multi_heap_out);
      ASSERT_NE(ptr, nullptr) << "block_size=" << block_size;
    }
    EXPECT_EQ(
        std::vector<TypeParam>(multi_heap_out->begin(), multi_heap_out->end()),
        expected)
        << "block_size=" << block_size;
    EXPECT_EQ(multi_heap_out->Capacity(), single_heap_out->Capacity())
        << "block_size=" << block_size;

    Arena multi_arena;
    auto* multi_arena_msg =
        Arena::Create<proto2_unittest::TestPackedTypes>(&multi_arena);
    RepeatedField<TypeParam>* multi_arena_out =
        MutablePackedFixedField<TypeParam>(multi_arena_msg);
    {
      io::ArrayInputStream ais(payload.data(), payload_bytes, block_size);
      const char* ptr = nullptr;
      ParseContext ctx(kMaxLimit, false, &ptr, &ais);
      ptr = ctx.ReadPackedFixed(*multi_arena_msg, ptr, payload_bytes,
                                multi_arena_out);
      ASSERT_NE(ptr, nullptr) << "block_size=" << block_size;
    }
    EXPECT_EQ(std::vector<TypeParam>(multi_arena_out->begin(),
                                     multi_arena_out->end()),
              expected)
        << "block_size=" << block_size;
    EXPECT_EQ(multi_arena_out->Capacity(), single_arena_out->Capacity())
        << "block_size=" << block_size;
    EXPECT_EQ(multi_arena.SpaceUsed(), single_arena.SpaceUsed())
        << "block_size=" << block_size;
  }

  // Multi-chunk TestZeroCopyInputStream with empty fragments interleaved.
  std::vector<std::string> fragments = {""};
  for (size_t offset = 0; offset < payload.size();) {
    size_t step = std::min<size_t>(23, payload.size() - offset);
    fragments.emplace_back(payload.substr(offset, step));
    fragments.emplace_back("");
    offset += step;
  }
  Arena frag_arena;
  auto* frag_msg = Arena::Create<proto2_unittest::TestPackedTypes>(&frag_arena);
  RepeatedField<TypeParam>* frag_out =
      MutablePackedFixedField<TypeParam>(frag_msg);
  {
    io::internal::TestZeroCopyInputStream stream(fragments);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &stream);
    ptr = ctx.ReadPackedFixed(*frag_msg, ptr, payload_bytes, frag_out);
    ASSERT_NE(ptr, nullptr);
  }
  EXPECT_EQ(std::vector<TypeParam>(frag_out->begin(), frag_out->end()),
            expected);
  EXPECT_EQ(frag_out->Capacity(), single_arena_out->Capacity());
  EXPECT_EQ(frag_arena.SpaceUsed(), single_arena.SpaceUsed());

  // End-to-end ParseFromZeroCopyStream on TestPackedTypes.
  const std::string wire = single_heap_msg.SerializeAsString();
  Arena single_e2e_arena;
  auto* single_e2e_msg =
      Arena::Create<proto2_unittest::TestPackedTypes>(&single_e2e_arena);
  io::ArrayInputStream single_e2e_ais(wire.data(),
                                      static_cast<int>(wire.size()));
  ASSERT_TRUE(single_e2e_msg->ParseFromZeroCopyStream(&single_e2e_ais));

  Arena multi_e2e_arena;
  auto* multi_e2e_msg =
      Arena::Create<proto2_unittest::TestPackedTypes>(&multi_e2e_arena);
  io::ArrayInputStream multi_e2e_ais(wire.data(), static_cast<int>(wire.size()),
                                     64);
  ASSERT_TRUE(multi_e2e_msg->ParseFromZeroCopyStream(&multi_e2e_ais));
  RepeatedField<TypeParam>* multi_e2e_out =
      MutablePackedFixedField<TypeParam>(multi_e2e_msg);
  EXPECT_EQ(
      std::vector<TypeParam>(multi_e2e_out->begin(), multi_e2e_out->end()),
      expected);
  EXPECT_EQ(multi_e2e_arena.SpaceUsed(), single_e2e_arena.SpaceUsed());
}

TYPED_TEST(ReadPackedFixedTest, AppendToNonEmptyFieldAcrossChunks) {
  constexpr size_t kInitialElements = 37;
  constexpr size_t kAppendedElements = 500;
  constexpr size_t kTotalElements = kInitialElements + kAppendedElements;
  const std::vector<TypeParam> initial_values =
      MakeTestValues<TypeParam>(kInitialElements, /*seed=*/100);
  const std::vector<TypeParam> appended_values =
      MakeTestValues<TypeParam>(kAppendedElements, /*seed=*/1000);
  std::vector<TypeParam> expected_all = initial_values;
  expected_all.insert(expected_all.end(), appended_values.begin(),
                      appended_values.end());

  const std::string payload = MakePackedFixedPayload(appended_values);
  const int payload_bytes = static_cast<int>(payload.size());

  auto populate_initial = [&](RepeatedField<TypeParam>* field) {
    for (TypeParam v : initial_values) {
      field->Add(v);
    }
  };

  // Single-chunk reference on Arena and Heap.
  Arena single_arena;
  auto* single_arena_msg =
      Arena::Create<proto2_unittest::TestPackedTypes>(&single_arena);
  RepeatedField<TypeParam>* single_arena_out =
      MutablePackedFixedField<TypeParam>(single_arena_msg);
  populate_initial(single_arena_out);
  {
    io::ArrayInputStream ais(payload.data(), payload_bytes);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    ptr = ctx.ReadPackedFixed(*single_arena_msg, ptr, payload_bytes,
                              single_arena_out);
    ASSERT_NE(ptr, nullptr);
  }

  proto2_unittest::TestPackedTypes single_heap_msg;
  RepeatedField<TypeParam>* single_heap_out =
      MutablePackedFixedField<TypeParam>(&single_heap_msg);
  populate_initial(single_heap_out);
  {
    io::ArrayInputStream ais(payload.data(), payload_bytes);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    ptr = ctx.ReadPackedFixed(single_heap_msg, ptr, payload_bytes,
                              single_heap_out);
    ASSERT_NE(ptr, nullptr);
  }

  // Multi-chunk append on Arena.
  Arena multi_arena;
  auto* multi_arena_msg =
      Arena::Create<proto2_unittest::TestPackedTypes>(&multi_arena);
  RepeatedField<TypeParam>* multi_arena_out =
      MutablePackedFixedField<TypeParam>(multi_arena_msg);
  populate_initial(multi_arena_out);
  {
    io::ArrayInputStream ais(payload.data(), payload_bytes, /*block_size=*/64);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    ptr = ctx.ReadPackedFixed(*multi_arena_msg, ptr, payload_bytes,
                              multi_arena_out);
    ASSERT_NE(ptr, nullptr);
  }
  EXPECT_EQ(
      std::vector<TypeParam>(multi_arena_out->begin(), multi_arena_out->end()),
      expected_all);
  EXPECT_EQ(multi_arena_out->Capacity(), single_arena_out->Capacity());
  EXPECT_EQ(multi_arena.SpaceUsed(), single_arena.SpaceUsed());
  // On Arena, capacity is exact kTotalElements (or kTotalElements + 1 when
  // 4-byte elements are rounded up to 8-byte alignment).
  constexpr int kExpectedArenaCapacity =
      static_cast<int>((internal::ArenaAlignDefault::Ceil(
                            8 + kTotalElements * sizeof(TypeParam)) -
                        8) /
                       sizeof(TypeParam));
  EXPECT_EQ(multi_arena_out->Capacity(), kExpectedArenaCapacity);

  // Multi-chunk append on Heap.
  proto2_unittest::TestPackedTypes multi_heap_msg;
  RepeatedField<TypeParam>* multi_heap_out =
      MutablePackedFixedField<TypeParam>(&multi_heap_msg);
  populate_initial(multi_heap_out);
  {
    io::ArrayInputStream ais(payload.data(), payload_bytes, /*block_size=*/64);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    ptr =
        ctx.ReadPackedFixed(multi_heap_msg, ptr, payload_bytes, multi_heap_out);
    ASSERT_NE(ptr, nullptr);
  }
  EXPECT_EQ(
      std::vector<TypeParam>(multi_heap_out->begin(), multi_heap_out->end()),
      expected_all);
  EXPECT_EQ(multi_heap_out->Capacity(), single_heap_out->Capacity());
}

TYPED_TEST(ReadPackedFixedTest, MalformedOrTruncatedSizeGuardsUpfrontReserve) {
  const std::string small_payload(128, '\x01');

  // 1a. Unaligned size across multiple chunks on a short stream must not
  // pre-reserve the forged size before failing on stream EOF.
  {
    Arena arena;
    auto* msg = Arena::Create<proto2_unittest::TestPackedTypes>(&arena);
    RepeatedField<TypeParam>* out = MutablePackedFixedField<TypeParam>(msg);
    io::ArrayInputStream ais(small_payload.data(), 64, /*block_size=*/32);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    const int unaligned_huge_size =
        100000 * static_cast<int>(sizeof(TypeParam)) + 1;
    ptr = ctx.ReadPackedFixed(*msg, ptr, unaligned_huge_size, out);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_LT(out->Capacity(), 100);
  }

  // 1b. Unaligned size across multiple chunks where stream has enough bytes
  // still fails at the tail check without pre-reserving upfront.
  {
    Arena arena;
    auto* msg = Arena::Create<proto2_unittest::TestPackedTypes>(&arena);
    RepeatedField<TypeParam>* out = MutablePackedFixedField<TypeParam>(msg);
    io::ArrayInputStream ais(small_payload.data(),
                             static_cast<int>(small_payload.size()),
                             /*block_size=*/32);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    const int unaligned_size = 12 * static_cast<int>(sizeof(TypeParam)) + 1;
    ptr = ctx.ReadPackedFixed(*msg, ptr, unaligned_size, out);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_LT(out->Capacity(), 100);
  }

  // 1c. Unaligned size within a single chunk returns nullptr.
  for (int unaligned_size : {1, 5 * static_cast<int>(sizeof(TypeParam)) + 1}) {
    proto2_unittest::TestPackedTypes msg;
    RepeatedField<TypeParam>* out = MutablePackedFixedField<TypeParam>(&msg);
    io::ArrayInputStream ais(small_payload.data(),
                             static_cast<int>(small_payload.size()));
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    ptr = ctx.ReadPackedFixed(msg, ptr, unaligned_size, out);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_LT(out->Capacity(), 100);
  }

  // 2a. Limit-exceeding size via PushLimit(ptr, 64) fails HasEnoughTillLimit
  // and avoids upfront reservation.
  {
    Arena arena;
    auto* msg = Arena::Create<proto2_unittest::TestPackedTypes>(&arena);
    RepeatedField<TypeParam>* out = MutablePackedFixedField<TypeParam>(msg);
    io::ArrayInputStream ais(small_payload.data(),
                             static_cast<int>(small_payload.size()),
                             /*block_size=*/32);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    auto limit_token = ctx.PushLimit(ptr, 64);
    ptr = ctx.ReadPackedFixed(*msg, ptr, 10'000'000, out);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_LT(arena.SpaceUsed(), 1024);
    (void)ctx.PopLimit(std::move(limit_token));
  }

  // 2b. Limit-exceeding size via BoundedZCIS{&stream, 64} fails
  // HasEnoughTillLimit and avoids upfront reservation.
  {
    Arena arena;
    auto* msg = Arena::Create<proto2_unittest::TestPackedTypes>(&arena);
    RepeatedField<TypeParam>* out = MutablePackedFixedField<TypeParam>(msg);
    io::ArrayInputStream ais(small_payload.data(),
                             static_cast<int>(small_payload.size()),
                             /*block_size=*/32);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, BoundedZCIS{&ais, 64});
    ptr = ctx.ReadPackedFixed(*msg, ptr, 10'000'000, out);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_LT(arena.SpaceUsed(), 1024);
  }

  // 3. Truncated unbounded stream with forged huge size (> kSafeStringSize)
  // caps upfront reservation at kSafeStringSize and fails cleanly on EOF.
  for (int forged_size : {std::numeric_limits<int>::max() - 32,
                          std::numeric_limits<int>::max() - 31}) {
    Arena arena;
    auto* msg = Arena::Create<proto2_unittest::TestPackedTypes>(&arena);
    RepeatedField<TypeParam>* out = MutablePackedFixedField<TypeParam>(msg);
    io::ArrayInputStream ais(small_payload.data(), 64, /*block_size=*/32);
    const char* ptr = nullptr;
    ParseContext ctx(kMaxLimit, false, &ptr, &ais);
    ptr = ctx.ReadPackedFixed(*msg, ptr, forged_size, out);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_LE(static_cast<size_t>(out->Capacity()) * sizeof(TypeParam),
              kSafeStringSize + 1024);
    EXPECT_LE(arena.SpaceUsed(), kSafeStringSize + 4096);
  }

  // End-to-end ParseFromZeroCopyStream with forged huge length prefix on a
  // truncated multi-chunk stream returns false without OOM.
  {
    constexpr int kFieldNumber =
        std::is_same_v<TypeParam, uint32_t>
            ? proto2_unittest::TestPackedTypes::kPackedFixed32FieldNumber
            : proto2_unittest::TestPackedTypes::kPackedFixed64FieldNumber;
    uint8_t header_buf[20];
    uint8_t* p = io::CodedOutputStream::WriteVarint32ToArray(
        WireFormatLite::MakeTag(kFieldNumber,
                                WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        header_buf);
    p = io::CodedOutputStream::WriteVarint32ToArray(
        static_cast<uint32_t>(std::numeric_limits<int>::max() - 31), p);
    std::string malformed_wire(reinterpret_cast<const char*>(header_buf),
                               static_cast<size_t>(p - header_buf));
    malformed_wire.append(64, '\x01');

    Arena arena;
    auto* msg = Arena::Create<proto2_unittest::TestPackedTypes>(&arena);
    io::ArrayInputStream ais(malformed_wire.data(),
                             static_cast<int>(malformed_wire.size()),
                             /*block_size=*/16);
    EXPECT_FALSE(msg->ParseFromZeroCopyStream(&ais));
    EXPECT_LE(arena.SpaceUsed(), kSafeStringSize + 4096);
  }
}


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
