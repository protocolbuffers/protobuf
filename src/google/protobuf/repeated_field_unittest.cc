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
// TODO:  Improve this unittest to bring it up to the standards of
//   other proto2 unittests.

#include "google/protobuf/repeated_field.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <list>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/config.h"
#include "absl/numeric/bits.h"
#include "absl/strings/cord.h"
#include "absl/types/span.h"
#include "google/protobuf/arena_test_util.h"
#include "google/protobuf/internal_visibility_for_testing.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
// TODO: Remove.
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/unittest.pb.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace {

using ::proto2_unittest::TestAllTypes;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::Le;

TEST(RepeatedFieldIterator, Traits) {
  using It = RepeatedField<absl::Cord>::iterator;
  EXPECT_TRUE((std::is_same<It::value_type, absl::Cord>::value));
  EXPECT_TRUE((std::is_same<It::reference, absl::Cord&>::value));
  EXPECT_TRUE((std::is_same<It::pointer, absl::Cord*>::value));
  EXPECT_TRUE((std::is_same<It::difference_type, std::ptrdiff_t>::value));
  EXPECT_TRUE((std::is_same<It::iterator_category,
                            std::random_access_iterator_tag>::value));
#if __cplusplus >= 202002L
  EXPECT_TRUE((
      std::is_same<It::iterator_concept, std::contiguous_iterator_tag>::value));
#else
  EXPECT_TRUE((std::is_same<It::iterator_concept,
                            std::random_access_iterator_tag>::value));
#endif
}

TEST(ConstRepeatedFieldIterator, Traits) {
  using It = RepeatedField<absl::Cord>::const_iterator;
  EXPECT_TRUE((std::is_same<It::value_type, absl::Cord>::value));
  EXPECT_TRUE((std::is_same<It::reference, const absl::Cord&>::value));
  EXPECT_TRUE((std::is_same<It::pointer, const absl::Cord*>::value));
  EXPECT_TRUE((std::is_same<It::difference_type, std::ptrdiff_t>::value));
  EXPECT_TRUE((std::is_same<It::iterator_category,
                            std::random_access_iterator_tag>::value));
#if __cplusplus >= 202002L
  EXPECT_TRUE((
      std::is_same<It::iterator_concept, std::contiguous_iterator_tag>::value));
#else
  EXPECT_TRUE((std::is_same<It::iterator_concept,
                            std::random_access_iterator_tag>::value));
#endif
}

TEST(RepeatedField, ConstInit) {
  PROTOBUF_CONSTINIT static RepeatedField<int> field{};  // NOLINT
  EXPECT_TRUE(field.empty());
}

// Test operations on a small RepeatedField.
TEST(RepeatedField, Small) {
  RepeatedField<int> field;

  EXPECT_TRUE(field.empty());
  EXPECT_EQ(field.size(), 0);

  field.Add(5);

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 1);
  EXPECT_EQ(field.Get(0), 5);
  EXPECT_EQ(field.at(0), 5);

  field.Add(42);

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), 5);
  EXPECT_EQ(field.at(0), 5);
  EXPECT_EQ(field.Get(1), 42);
  EXPECT_EQ(field.at(1), 42);

  field.Set(1, 23);

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), 5);
  EXPECT_EQ(field.at(0), 5);
  EXPECT_EQ(field.Get(1), 23);
  EXPECT_EQ(field.at(1), 23);

  field.at(1) = 25;

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), 5);
  EXPECT_EQ(field.at(0), 5);
  EXPECT_EQ(field.Get(1), 25);
  EXPECT_EQ(field.at(1), 25);

  field.RemoveLast();

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 1);
  EXPECT_EQ(field.Get(0), 5);
  EXPECT_EQ(field.at(0), 5);

  field.Clear();

  EXPECT_TRUE(field.empty());
  EXPECT_EQ(field.size(), 0);
  if (sizeof(void*) == 8) {
    // Usage should be 0 because this should fit in SOO space.
    EXPECT_EQ(field.SpaceUsedExcludingSelf(), 0);
  }
}


// Test operations on a RepeatedField which is large enough to allocate a
// separate array.
TEST(RepeatedField, Large) {
  RepeatedField<int> field;

  for (int i = 0; i < 16; i++) {
    field.Add(i * i);
  }

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 16);

  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field.Get(i), i * i);
  }

  int expected_usage = 16 * sizeof(int);
  EXPECT_GE(field.SpaceUsedExcludingSelf(), expected_usage);
}

template <typename Rep>
void CheckAllocationSizes() {
  using T = typename Rep::value_type;
  // Use a large initial block to make the checks below easier to predict.
  std::string buf(1 << 20, 0);

  Arena arena(&buf[0], buf.size());
  auto* rep = Arena::Create<Rep>(&arena);
  size_t prev = arena.SpaceUsed();

  for (int i = 0; i < 100; ++i) {
    rep->Add(T{});
    if (sizeof(void*) == 8) {
      size_t new_used = arena.SpaceUsed();
      size_t last_alloc = new_used - prev;
      prev = new_used;

      // When we actually allocated something, check the size.
      if (last_alloc != 0) {
        // Must be `>= 16`, as expected by the Arena.
        ASSERT_GE(last_alloc, 16);
        // Must be of a power of two.
        size_t log2 = absl::bit_width(last_alloc) - 1;
        ASSERT_EQ((1 << log2), last_alloc);
      }

      // The byte size must be a multiple of 8 when not SOO.
      const int capacity_bytes = rep->Capacity() * sizeof(T);
      if (capacity_bytes > internal::kSooCapacityBytes) {
        ASSERT_EQ(capacity_bytes % 8, 0);
      }
    }
  }
}

TEST(RepeatedField, ArenaAllocationSizesMatchExpectedValues) {
  // RepeatedField guarantees that in 64-bit mode we never allocate anything
  // smaller than 16 bytes from an arena.
  // This is important to avoid a branch in the reallocation path.
  // This is also important because allocating anything less would be wasting
  // memory.
  // If the allocation size is wrong, ReturnArrayMemory will ABSL_DCHECK.
  EXPECT_NO_FATAL_FAILURE(CheckAllocationSizes<RepeatedField<bool>>());
  EXPECT_NO_FATAL_FAILURE(CheckAllocationSizes<RepeatedField<uint32_t>>());
  EXPECT_NO_FATAL_FAILURE(CheckAllocationSizes<RepeatedField<uint64_t>>());
  EXPECT_NO_FATAL_FAILURE(CheckAllocationSizes<RepeatedField<absl::Cord>>());
}

TEST(RepeatedField, NaturalGrowthOnArenasReuseBlocks) {
  Arena arena;
  std::vector<RepeatedField<int>*> values;

  static constexpr int kNumFields = 100;
  static constexpr int kNumElems = 1000;
  for (int i = 0; i < kNumFields; ++i) {
    values.push_back(Arena::Create<RepeatedField<int>>(&arena));
    auto& field = *values.back();
    for (int j = 0; j < kNumElems; ++j) {
      field.Add(j);
    }
  }

  size_t expected = values.size() * values[0]->Capacity() * sizeof(int);
  // Use a 2% slack for other overhead. If we were not reusing the blocks, the
  // actual value would be ~2x the expected.
  EXPECT_THAT(arena.SpaceUsed(), AllOf(Ge(expected), Le(1.02 * expected)));
}

// Test swapping between various types of RepeatedFields.
TEST(RepeatedField, SwapSmallSmall) {
  RepeatedField<int> field1;
  RepeatedField<int> field2;

  field1.Add(5);
  field1.Add(42);

  EXPECT_FALSE(field1.empty());
  EXPECT_EQ(field1.size(), 2);
  EXPECT_EQ(field1.Get(0), 5);
  EXPECT_EQ(field1.Get(1), 42);

  EXPECT_TRUE(field2.empty());
  EXPECT_EQ(field2.size(), 0);

  field1.Swap(&field2);

  EXPECT_TRUE(field1.empty());
  EXPECT_EQ(field1.size(), 0);

  EXPECT_FALSE(field2.empty());
  EXPECT_EQ(field2.size(), 2);
  EXPECT_EQ(field2.Get(0), 5);
  EXPECT_EQ(field2.Get(1), 42);
}

TEST(RepeatedField, SwapLargeSmall) {
  RepeatedField<int> field1;
  RepeatedField<int> field2;

  for (int i = 0; i < 16; i++) {
    field1.Add(i * i);
  }
  field2.Add(5);
  field2.Add(42);
  field1.Swap(&field2);

  EXPECT_EQ(field1.size(), 2);
  EXPECT_EQ(field1.Get(0), 5);
  EXPECT_EQ(field1.Get(1), 42);
  EXPECT_EQ(field2.size(), 16);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field2.Get(i), i * i);
  }
}

TEST(RepeatedField, SwapLargeLarge) {
  RepeatedField<int> field1;
  RepeatedField<int> field2;

  field1.Add(5);
  field1.Add(42);
  for (int i = 0; i < 16; i++) {
    field1.Add(i);
    field2.Add(i * i);
  }
  field2.Swap(&field1);

  EXPECT_EQ(field1.size(), 16);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field1.Get(i), i * i);
  }
  EXPECT_EQ(field2.size(), 18);
  EXPECT_EQ(field2.Get(0), 5);
  EXPECT_EQ(field2.Get(1), 42);
  for (int i = 2; i < 18; i++) {
    EXPECT_EQ(field2.Get(i), i - 2);
  }
}

template <int kSize>
void TestMemswap() {
  SCOPED_TRACE(kSize);

  const auto a_char = [](int i) -> char { return (i % ('z' - 'a')) + 'a'; };
  const auto b_char = [](int i) -> char { return (i % ('Z' - 'A')) + 'A'; };
  std::string a, b;
  for (int i = 0; i < kSize; ++i) {
    a += a_char(i);
    b += b_char(i);
  }
  // We will not swap these.
  a += '+';
  b += '-';

  std::string expected_a = b, expected_b = a;
  expected_a.back() = '+';
  expected_b.back() = '-';

  internal::memswap<kSize>(&a[0], &b[0]);

  // ODR use the functions in a way that forces the linker to keep them. That
  // way we can see their generated code.
  volatile auto odr_use_for_asm_dump = &internal::memswap<kSize>;
  (void)odr_use_for_asm_dump;

  EXPECT_EQ(expected_a, a);
  EXPECT_EQ(expected_b, b);
}

TEST(Memswap, VerifyWithSmallAndLargeSizes) {
  // Arbitrary sizes
  TestMemswap<0>();
  TestMemswap<1>();
  TestMemswap<10>();
  TestMemswap<100>();
  TestMemswap<1000>();
  TestMemswap<10000>();
  TestMemswap<100000>();
  TestMemswap<1000000>();

  // Pointer aligned sizes
  TestMemswap<sizeof(void*) * 1>();
  TestMemswap<sizeof(void*) * 7>();
  TestMemswap<sizeof(void*) * 17>();
  TestMemswap<sizeof(void*) * 27>();

  // Test also just the block size and no leftover.
  TestMemswap<64 * 1>();
  TestMemswap<64 * 2>();
  TestMemswap<64 * 3>();
  TestMemswap<64 * 4>();
}

// Determines how much space was reserved by the given field by adding elements
// to it until it re-allocates its space.
static int ReservedSpace(RepeatedField<int>* field) {
  const int* ptr = field->data();
  do {
    field->Add(0);
  } while (field->data() == ptr);

  return field->size() - 1;
}

TEST(RepeatedField, ReserveMoreThanDouble) {
  // Reserve more than double the previous space in the field and expect the
  // field to reserve exactly the amount specified.
  RepeatedField<int> field;
  field.Reserve(20);

  EXPECT_LE(20, ReservedSpace(&field));
}

TEST(RepeatedField, ReserveLessThanDouble) {
  // Reserve less than double the previous space in the field and expect the
  // field to grow by double instead.
  RepeatedField<int> field;
  field.Reserve(20);
  int capacity = field.Capacity();
  field.Reserve(capacity * 1.5);

  EXPECT_LE(2 * capacity, ReservedSpace(&field));
}

TEST(RepeatedField, ReserveLessThanExisting) {
  // Reserve less than the previous space in the field and expect the
  // field to not re-allocate at all.
  RepeatedField<int> field;
  field.Reserve(20);
  const int* previous_ptr = field.data();
  field.Reserve(10);

  EXPECT_EQ(previous_ptr, field.data());
  EXPECT_LE(20, ReservedSpace(&field));
}

TEST(RepeatedField, Resize) {
  RepeatedField<int> field;
  field.Resize(2, 1);
  EXPECT_EQ(2, field.size());
  field.Resize(5, 2);
  EXPECT_EQ(5, field.size());
  field.Resize(4, 3);
  ASSERT_EQ(4, field.size());
  EXPECT_EQ(1, field.Get(0));
  EXPECT_EQ(1, field.Get(1));
  EXPECT_EQ(2, field.Get(2));
  EXPECT_EQ(2, field.Get(3));
  field.Resize(0, 4);
  EXPECT_TRUE(field.empty());
}

TEST(RepeatedField, ReserveLowerClamp) {
  int clamped_value = internal::CalculateReserveSize<bool, sizeof(void*)>(0, 1);
  EXPECT_GE(clamped_value, sizeof(void*) / sizeof(bool));
  EXPECT_EQ((internal::RepeatedFieldLowerClampLimit<bool, sizeof(void*)>()),
            clamped_value);
  // EXPECT_EQ(clamped_value, (internal::CalculateReserveSize<bool,
  // sizeof(void*)>( clamped_value, 2)));

  clamped_value = internal::CalculateReserveSize<int, sizeof(void*)>(0, 1);
  EXPECT_GE(clamped_value, sizeof(void*) / sizeof(int));
  EXPECT_EQ((internal::RepeatedFieldLowerClampLimit<int, sizeof(void*)>()),
            clamped_value);
  // EXPECT_EQ(clamped_value, (internal::CalculateReserveSize<int,
  // sizeof(void*)>( clamped_value, 2)));
}

TEST(RepeatedField, ReserveGrowth) {
  // Make sure the field capacity doubles in size on repeated reservation.
  for (int size = internal::RepeatedFieldLowerClampLimit<int, sizeof(void*)>(),
           i = 0;
       i < 4; ++i) {
    int next =
        sizeof(Arena*) >= sizeof(int)
            ?
            // for small enough elements, we double number of total bytes
            ((2 * (size * sizeof(int) + sizeof(Arena*))) - sizeof(Arena*)) /
                sizeof(int)
            :
            // we just double the number of elements if too large size.
            size * 2;
    EXPECT_EQ(next, (internal::CalculateReserveSize<int, sizeof(void*)>(
                        size, size + 1)));
    size = next;
  }
}

TEST(RepeatedField, ReserveLarge) {
  const int old_size = 10;
  // This is a size we won't get by doubling:
  const int new_size = old_size * 3 + 1;

  // Reserving more than 2x current capacity should grow directly to that size.
  EXPECT_EQ(new_size, (internal::CalculateReserveSize<int, sizeof(void*)>(
                          old_size, new_size)));
}

TEST(RepeatedField, ReserveHuge) {
  if (internal::HasAnySanitizer()) {
    GTEST_SKIP() << "Disabled because sanitizer is active";
  }
  // Largest value that does not clamp to the large limit:
  constexpr int non_clamping_limit =
      (std::numeric_limits<int>::max() - sizeof(Arena*)) / 2;
  ASSERT_LT(2 * non_clamping_limit, std::numeric_limits<int>::max());
  EXPECT_LT((internal::CalculateReserveSize<int, sizeof(void*)>(
                non_clamping_limit, non_clamping_limit + 1)),
            std::numeric_limits<int>::max());

  // Smallest size that *will* clamp to the upper limit:
  constexpr int min_clamping_size = std::numeric_limits<int>::max() / 2 + 1;
  EXPECT_EQ((internal::CalculateReserveSize<int, sizeof(void*)>(
                min_clamping_size, min_clamping_size + 1)),
            std::numeric_limits<int>::max());

#ifdef PROTOBUF_TEST_ALLOW_LARGE_ALLOC
  // The rest of this test may allocate several GB of memory, so it is only
  // built if explicitly requested.
  RepeatedField<int> huge_field;

  // Reserve a size for huge_field that will clamp.
  huge_field.Reserve(min_clamping_size);
  EXPECT_GE(huge_field.Capacity(), min_clamping_size);
  ASSERT_LT(huge_field.Capacity(), std::numeric_limits<int>::max() - 1);

  // The array containing all the fields is, in theory, up to MAXINT-1 in size.
  // However, some compilers can't handle a struct whose size is larger
  // than 2GB, and the protocol buffer format doesn't handle more than 2GB of
  // data at once, either.  So we limit it, but the code below accesses beyond
  // that limit.

  // Allocation may return more memory than we requested. However, the updated
  // size must still be clamped to a valid range.
  huge_field.Reserve(huge_field.Capacity() + 1);
  EXPECT_EQ(huge_field.Capacity(), std::numeric_limits<int>::max());
#endif  // PROTOBUF_TEST_ALLOW_LARGE_ALLOC
}

TEST(RepeatedField, MergeFrom) {
  RepeatedField<int> source, destination;
  source.Add(4);
  source.Add(5);
  destination.Add(1);
  destination.Add(2);
  destination.Add(3);

  destination.MergeFrom(source);

  ASSERT_EQ(5, destination.size());
  EXPECT_EQ(1, destination.Get(0));
  EXPECT_EQ(2, destination.Get(1));
  EXPECT_EQ(3, destination.Get(2));
  EXPECT_EQ(4, destination.Get(3));
  EXPECT_EQ(5, destination.Get(4));
}


TEST(RepeatedField, CopyFrom) {
  RepeatedField<int> source, destination;
  source.Add(4);
  source.Add(5);
  destination.Add(1);
  destination.Add(2);
  destination.Add(3);

  destination.CopyFrom(source);

  ASSERT_EQ(2, destination.size());
  EXPECT_EQ(4, destination.Get(0));
  EXPECT_EQ(5, destination.Get(1));
}

TEST(RepeatedField, CopyFromSelf) {
  RepeatedField<int> me;
  me.Add(3);
  me.CopyFrom(me);
  ASSERT_EQ(1, me.size());
  EXPECT_EQ(3, me.Get(0));
}

TEST(RepeatedField, Erase) {
  RepeatedField<int> me;
  RepeatedField<int>::iterator it = me.erase(me.begin(), me.end());
  EXPECT_TRUE(me.begin() == it);
  EXPECT_EQ(0, me.size());

  me.Add(1);
  me.Add(2);
  me.Add(3);
  it = me.erase(me.begin(), me.end());
  EXPECT_TRUE(me.begin() == it);
  EXPECT_EQ(0, me.size());

  me.Add(4);
  me.Add(5);
  me.Add(6);
  it = me.erase(me.begin() + 2, me.end());
  EXPECT_TRUE(me.begin() + 2 == it);
  EXPECT_EQ(2, me.size());
  EXPECT_EQ(4, me.Get(0));
  EXPECT_EQ(5, me.Get(1));

  me.Add(6);
  me.Add(7);
  me.Add(8);
  it = me.erase(me.begin() + 1, me.begin() + 3);
  EXPECT_TRUE(me.begin() + 1 == it);
  EXPECT_EQ(3, me.size());
  EXPECT_EQ(4, me.Get(0));
  EXPECT_EQ(7, me.Get(1));
  EXPECT_EQ(8, me.Get(2));
}

// Add contents of empty container to an empty field.
TEST(RepeatedField, AddRange1) {
  RepeatedField<int> me;
  std::vector<int> values;

  me.Add(values.begin(), values.end());
  ASSERT_EQ(me.size(), 0);
}

// Add contents of container with one thing to an empty field.
TEST(RepeatedField, AddRange2) {
  RepeatedField<int> me;
  std::vector<int> values;
  values.push_back(-1);

  me.Add(values.begin(), values.end());
  ASSERT_EQ(me.size(), 1);
  ASSERT_EQ(me.Get(0), values[0]);
}

// Add contents of container with more than one thing to an empty field.
TEST(RepeatedField, AddRange3) {
  RepeatedField<int> me;
  std::vector<int> values;
  values.push_back(0);
  values.push_back(1);

  me.Add(values.begin(), values.end());
  ASSERT_EQ(me.size(), 2);
  ASSERT_EQ(me.Get(0), values[0]);
  ASSERT_EQ(me.Get(1), values[1]);
}

// Add contents of container with more than one thing to a non-empty field.
TEST(RepeatedField, AddRange4) {
  RepeatedField<int> me;
  me.Add(0);
  me.Add(1);

  std::vector<int> values;
  values.push_back(2);
  values.push_back(3);

  me.Add(values.begin(), values.end());
  ASSERT_EQ(me.size(), 4);
  ASSERT_EQ(me.Get(0), 0);
  ASSERT_EQ(me.Get(1), 1);
  ASSERT_EQ(me.Get(2), values[0]);
  ASSERT_EQ(me.Get(3), values[1]);
}

// Add contents of a stringstream in order to test code paths where there is
// an input iterator.
TEST(RepeatedField, AddRange5) {
  RepeatedField<int> me;
  me.Add(0);

  std::stringstream ss;
  ss << 1 << ' ' << 2;

  me.Add(std::istream_iterator<int>(ss), std::istream_iterator<int>());
  ASSERT_EQ(me.size(), 3);
  ASSERT_EQ(me.Get(0), 0);
  ASSERT_EQ(me.Get(1), 1);
  ASSERT_EQ(me.Get(2), 2);
}

// Add contents of container with a quirky iterator like std::vector<bool>
TEST(RepeatedField, AddRange6) {
  RepeatedField<bool> me;
  me.Add(true);
  me.Add(false);

  std::vector<bool> values;
  values.push_back(true);
  values.push_back(true);
  values.push_back(false);

  me.Add(values.begin(), values.end());
  ASSERT_EQ(me.size(), 5);
  ASSERT_EQ(me.Get(0), true);
  ASSERT_EQ(me.Get(1), false);
  ASSERT_EQ(me.Get(2), true);
  ASSERT_EQ(me.Get(3), true);
  ASSERT_EQ(me.Get(4), false);
}

// Add contents of absl::Span which evaluates to const T on access.
TEST(RepeatedField, AddRange7) {
  int ints[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  absl::Span<const int> span(ints);
  auto p = span.begin();
  static_assert(std::is_convertible<decltype(p), const int*>::value, "");
  RepeatedField<int> me;
  me.Add(span.begin(), span.end());

  ASSERT_EQ(me.size(), 10);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(me.Get(i), i);
  }
}

TEST(RepeatedField, AddAndAssignRanges) {
  RepeatedField<int> field;

  int vals[] = {2, 27, 2875, 609250};
  field.Assign(std::begin(vals), std::end(vals));

  ASSERT_EQ(field.size(), 4);
  EXPECT_EQ(field.Get(0), 2);
  EXPECT_EQ(field.Get(1), 27);
  EXPECT_EQ(field.Get(2), 2875);
  EXPECT_EQ(field.Get(3), 609250);

  field.Add(std::begin(vals), std::end(vals));
  ASSERT_EQ(field.size(), 8);
  EXPECT_EQ(field.Get(0), 2);
  EXPECT_EQ(field.Get(1), 27);
  EXPECT_EQ(field.Get(2), 2875);
  EXPECT_EQ(field.Get(3), 609250);
  EXPECT_EQ(field.Get(4), 2);
  EXPECT_EQ(field.Get(5), 27);
  EXPECT_EQ(field.Get(6), 2875);
  EXPECT_EQ(field.Get(7), 609250);
}

TEST(RepeatedField, CopyConstructIntegers) {
  auto token = internal::InternalVisibilityForTesting{};
  using RepeatedType = RepeatedField<int>;
  RepeatedType original;
  original.Add(1);
  original.Add(2);

  RepeatedType fields1(original);
  ASSERT_EQ(2, fields1.size());
  EXPECT_EQ(1, fields1.Get(0));
  EXPECT_EQ(2, fields1.Get(1));

  RepeatedType fields2(token, nullptr, original);
  ASSERT_EQ(2, fields2.size());
  EXPECT_EQ(1, fields2.Get(0));
  EXPECT_EQ(2, fields2.Get(1));
}

TEST(RepeatedField, CopyConstructCords) {
  auto token = internal::InternalVisibilityForTesting{};
  using RepeatedType = RepeatedField<absl::Cord>;
  RepeatedType original;
  original.Add(absl::Cord("hello"));
  original.Add(absl::Cord("world and text to avoid SSO"));

  RepeatedType fields1(original);
  ASSERT_EQ(2, fields1.size());
  EXPECT_EQ("hello", fields1.Get(0));
  EXPECT_EQ("world and text to avoid SSO", fields1.Get(1));

  RepeatedType fields2(token, nullptr, original);
  ASSERT_EQ(2, fields1.size());
  EXPECT_EQ("hello", fields1.Get(0));
  EXPECT_EQ("world and text to avoid SSO", fields2.Get(1));
}

TEST(RepeatedField, CopyConstructIntegersWithArena) {
  auto token = internal::InternalVisibilityForTesting{};
  using RepeatedType = RepeatedField<int>;
  RepeatedType original;
  original.Add(1);
  original.Add(2);

  Arena arena;
  alignas(RepeatedType) char mem[sizeof(RepeatedType)];
  RepeatedType& fields1 = *new (mem) RepeatedType(token, &arena, original);
  ASSERT_EQ(2, fields1.size());
  EXPECT_EQ(1, fields1.Get(0));
  EXPECT_EQ(2, fields1.Get(1));
}

TEST(RepeatedField, CopyConstructCordsWithArena) {
  auto token = internal::InternalVisibilityForTesting{};
  using RepeatedType = RepeatedField<absl::Cord>;
  RepeatedType original;
  original.Add(absl::Cord("hello"));
  original.Add(absl::Cord("world and text to avoid SSO"));

  Arena arena;
  alignas(RepeatedType) char mem[sizeof(RepeatedType)];
  RepeatedType& fields1 = *new (mem) RepeatedType(token, &arena, original);
  ASSERT_EQ(2, fields1.size());
  EXPECT_EQ("hello", fields1.Get(0));
  EXPECT_EQ("world and text to avoid SSO", fields1.Get(1));

  // Contract requires dtor to be invoked for absl::Cord
  fields1.~RepeatedType();
}

TEST(RepeatedField, IteratorConstruct) {
  std::vector<int> values;
  RepeatedField<int> empty(values.begin(), values.end());
  ASSERT_EQ(values.size(), empty.size());

  values.push_back(1);
  values.push_back(2);

  RepeatedField<int> field(values.begin(), values.end());
  ASSERT_EQ(values.size(), field.size());
  EXPECT_EQ(values[0], field.Get(0));
  EXPECT_EQ(values[1], field.Get(1));

  RepeatedField<int> other(field.begin(), field.end());
  ASSERT_EQ(values.size(), other.size());
  EXPECT_EQ(values[0], other.Get(0));
  EXPECT_EQ(values[1], other.Get(1));
}

TEST(RepeatedField, CopyAssign) {
  RepeatedField<int> source, destination;
  source.Add(4);
  source.Add(5);
  destination.Add(1);
  destination.Add(2);
  destination.Add(3);

  destination = source;

  ASSERT_EQ(2, destination.size());
  EXPECT_EQ(4, destination.Get(0));
  EXPECT_EQ(5, destination.Get(1));
}

TEST(RepeatedField, SelfAssign) {
  // Verify that assignment to self does not destroy data.
  RepeatedField<int> source, *p;
  p = &source;
  source.Add(7);
  source.Add(8);

  *p = source;

  ASSERT_EQ(2, source.size());
  EXPECT_EQ(7, source.Get(0));
  EXPECT_EQ(8, source.Get(1));
}

TEST(RepeatedField, MoveConstruct) {
  {
    RepeatedField<int> source;
    source.Add(1);
    source.Add(2);
    RepeatedField<int> destination = std::move(source);
    EXPECT_THAT(destination, ElementsAre(1, 2));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_TRUE(source.empty());
  }
  {
    Arena arena;
    RepeatedField<int>* source = Arena::Create<RepeatedField<int>>(&arena);
    source->Add(1);
    source->Add(2);
    RepeatedField<int> destination = std::move(*source);
    EXPECT_EQ(nullptr, destination.GetArena());
    EXPECT_THAT(destination, ElementsAre(1, 2));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_THAT(*source, ElementsAre(1, 2));
  }
}

TEST(RepeatedField, MoveAssign) {
  {
    RepeatedField<int> source;
    source.Add(1);
    source.Add(2);
    RepeatedField<int> destination;
    destination.Add(3);
    destination = std::move(source);
    EXPECT_THAT(destination, ElementsAre(1, 2));
    EXPECT_THAT(source, ElementsAre(3));
  }
  {
    Arena arena;
    RepeatedField<int>* source = Arena::Create<RepeatedField<int>>(&arena);
    source->Add(1);
    source->Add(2);
    RepeatedField<int>* destination = Arena::Create<RepeatedField<int>>(&arena);
    destination->Add(3);
    *destination = std::move(*source);
    EXPECT_THAT(*destination, ElementsAre(1, 2));
    EXPECT_THAT(*source, ElementsAre(3));
  }
  {
    Arena source_arena;
    RepeatedField<int>* source =
        Arena::Create<RepeatedField<int>>(&source_arena);
    source->Add(1);
    source->Add(2);
    Arena destination_arena;
    RepeatedField<int>* destination =
        Arena::Create<RepeatedField<int>>(&destination_arena);
    destination->Add(3);
    *destination = std::move(*source);
    EXPECT_THAT(*destination, ElementsAre(1, 2));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_THAT(*source, ElementsAre(1, 2));
  }
  {
    Arena arena;
    RepeatedField<int>* source = Arena::Create<RepeatedField<int>>(&arena);
    source->Add(1);
    source->Add(2);
    RepeatedField<int> destination;
    destination.Add(3);
    destination = std::move(*source);
    EXPECT_THAT(destination, ElementsAre(1, 2));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_THAT(*source, ElementsAre(1, 2));
  }
  {
    RepeatedField<int> source;
    source.Add(1);
    source.Add(2);
    Arena arena;
    RepeatedField<int>* destination = Arena::Create<RepeatedField<int>>(&arena);
    destination->Add(3);
    *destination = std::move(source);
    EXPECT_THAT(*destination, ElementsAre(1, 2));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_THAT(source, ElementsAre(1, 2));
  }
  {
    RepeatedField<int> field;
    // An alias to defeat -Wself-move.
    RepeatedField<int>& alias = field;
    field.Add(1);
    field.Add(2);
    field = std::move(alias);
    EXPECT_THAT(field, ElementsAre(1, 2));
  }
  {
    Arena arena;
    RepeatedField<int>* field = Arena::Create<RepeatedField<int>>(&arena);
    field->Add(1);
    field->Add(2);
    *field = std::move(*field);
    EXPECT_THAT(*field, ElementsAre(1, 2));
  }
}

TEST(RepeatedField, MutableDataIsMutable) {
  RepeatedField<int> field;
  field.Add(1);
  EXPECT_EQ(1, field.Get(0));
  // The fact that this line compiles would be enough, but we'll check the
  // value anyway.
  *field.mutable_data() = 2;
  EXPECT_EQ(2, field.Get(0));
}

TEST(RepeatedField, SubscriptOperators) {
  RepeatedField<int> field;
  field.Add(1);
  EXPECT_EQ(1, field.Get(0));
  EXPECT_EQ(1, field[0]);
  EXPECT_EQ(field.Mutable(0), &field[0]);
  const RepeatedField<int>& const_field = field;
  EXPECT_EQ(field.data(), &const_field[0]);
}

TEST(RepeatedField, Truncate) {
  RepeatedField<int> field;

  field.Add(12);
  field.Add(34);
  field.Add(56);
  field.Add(78);
  EXPECT_EQ(4, field.size());

  field.Truncate(3);
  EXPECT_EQ(3, field.size());

  field.Add(90);
  EXPECT_EQ(4, field.size());
  EXPECT_EQ(90, field.Get(3));

  // Truncations that don't change the size are allowed, but growing is not
  // allowed.
  field.Truncate(field.size());
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(field.Truncate(field.size() + 1), "new_size");
#endif
}

TEST(RepeatedCordField, AddRemoveLast) {
  RepeatedField<absl::Cord> field;
  field.Add(absl::Cord("foo"));
  field.RemoveLast();
}

TEST(RepeatedCordField, AddClear) {
  RepeatedField<absl::Cord> field;
  field.Add(absl::Cord("foo"));
  field.Clear();
}

TEST(RepeatedCordField, Resize) {
  RepeatedField<absl::Cord> field;
  field.Resize(10, absl::Cord("foo"));
}

TEST(RepeatedField, Cords) {
  RepeatedField<absl::Cord> field;

  field.Add(absl::Cord("foo"));
  field.Add(absl::Cord("bar"));
  field.Add(absl::Cord("baz"));
  field.Add(absl::Cord("moo"));
  field.Add(absl::Cord("corge"));

  EXPECT_EQ("foo", std::string(field.Get(0)));
  EXPECT_EQ("corge", std::string(field.Get(4)));

  // Test swap.  Note:  One of the swapped objects is using internal storage,
  //   the other is not.
  RepeatedField<absl::Cord> field2;
  field2.Add(absl::Cord("grault"));
  field.Swap(&field2);
  EXPECT_EQ(1, field.size());
  EXPECT_EQ("grault", std::string(field.Get(0)));
  EXPECT_EQ(5, field2.size());
  EXPECT_EQ("foo", std::string(field2.Get(0)));
  EXPECT_EQ("corge", std::string(field2.Get(4)));

  // Test SwapElements().
  field2.SwapElements(1, 3);
  EXPECT_EQ("moo", std::string(field2.Get(1)));
  EXPECT_EQ("bar", std::string(field2.Get(3)));

  // Make sure cords are cleared correctly.
  field2.RemoveLast();
  EXPECT_TRUE(field2.Add()->empty());
  field2.Clear();
  EXPECT_TRUE(field2.Add()->empty());
}

TEST(RepeatedField, TruncateCords) {
  RepeatedField<absl::Cord> field;

  field.Add(absl::Cord("foo"));
  field.Add(absl::Cord("bar"));
  field.Add(absl::Cord("baz"));
  field.Add(absl::Cord("moo"));
  EXPECT_EQ(4, field.size());

  field.Truncate(3);
  EXPECT_EQ(3, field.size());

  field.Add(absl::Cord("corge"));
  EXPECT_EQ(4, field.size());
  EXPECT_EQ("corge", std::string(field.Get(3)));

  // Truncating to the current size should be fine (no-op), but truncating
  // to a larger size should crash.
  field.Truncate(field.size());
#if defined(GTEST_HAS_DEATH_TEST) && !defined(NDEBUG)
  EXPECT_DEATH(field.Truncate(field.size() + 1), "new_size");
#endif
}

TEST(RepeatedField, ResizeCords) {
  RepeatedField<absl::Cord> field;
  field.Resize(2, absl::Cord("foo"));
  EXPECT_EQ(2, field.size());
  field.Resize(5, absl::Cord("bar"));
  EXPECT_EQ(5, field.size());
  field.Resize(4, absl::Cord("baz"));
  ASSERT_EQ(4, field.size());
  EXPECT_EQ("foo", std::string(field.Get(0)));
  EXPECT_EQ("foo", std::string(field.Get(1)));
  EXPECT_EQ("bar", std::string(field.Get(2)));
  EXPECT_EQ("bar", std::string(field.Get(3)));
  field.Resize(0, absl::Cord("moo"));
  EXPECT_TRUE(field.empty());
}

TEST(RepeatedField, ExtractSubrange) {
  // Exhaustively test every subrange in arrays of all sizes from 0 through 9.
  for (int sz = 0; sz < 10; ++sz) {
    for (int num = 0; num <= sz; ++num) {
      for (int start = 0; start < sz - num; ++start) {
        // Create RepeatedField with sz elements having values 0 through sz-1.
        RepeatedField<int32_t> field;
        for (int i = 0; i < sz; ++i) field.Add(i);
        EXPECT_EQ(field.size(), sz);

        // Create a catcher array and call ExtractSubrange.
        int32_t catcher[10];
        for (int i = 0; i < 10; ++i) catcher[i] = -1;
        field.ExtractSubrange(start, num, catcher);

        // Does the resulting array have the right size?
        EXPECT_EQ(field.size(), sz - num);

        // Were the removed elements extracted into the catcher array?
        for (int i = 0; i < num; ++i) EXPECT_EQ(catcher[i], start + i);
        EXPECT_EQ(catcher[num], -1);

        // Does the resulting array contain the right values?
        for (int i = 0; i < start; ++i) EXPECT_EQ(field.Get(i), i);
        for (int i = start; i < field.size(); ++i)
          EXPECT_EQ(field.Get(i), i + num);
      }
    }
  }
}

TEST(RepeatedField, TestSAddFromSelf) {
  RepeatedField<int> field;
  field.Add(0);
  for (int i = 0; i < 1000; i++) {
    field.Add(field[0]);
  }
}

// We have, or at least had bad callers that never triggered our DCHECKS
// Here we check we DO fail on bad Truncate calls under debug, and do nothing
// under opt compiles.
TEST(RepeatedField, HardenAgainstBadTruncate) {
  RepeatedField<int> field;
  for (int size = 0; size < 10; ++size) {
    field.Truncate(size);
#if GTEST_HAS_DEATH_TEST
    EXPECT_DEBUG_DEATH(field.Truncate(size + 1), "new_size <= old_size");
    EXPECT_DEBUG_DEATH(field.Truncate(size + 2), "new_size <= old_size");
#elif defined(NDEBUG)
    field.Truncate(size + 1);
    field.Truncate(size + 1);
#endif
    EXPECT_EQ(field.size(), size);
    field.Add(1);
  }
}

#if defined(GTEST_HAS_DEATH_TEST) && (defined(ABSL_HAVE_ADDRESS_SANITIZER) || \
                                      defined(ABSL_HAVE_MEMORY_SANITIZER))

// This function verifies that the code dies under ASAN or MSAN trying to both
// read and write the reserved element directly beyond the last element.
void VerifyDeathOnWriteAndReadAccessBeyondEnd(RepeatedField<int64_t>& field) {
  auto* end = field.Mutable(field.size() - 1) + 1;
#if defined(ABSL_HAVE_ADDRESS_SANITIZER)
  EXPECT_DEATH(*end = 1, "container-overflow");
  EXPECT_DEATH(EXPECT_NE(*end, 1), "container-overflow");
#elif defined(ABSL_HAVE_MEMORY_SANITIZER)
  EXPECT_DEATH(EXPECT_NE(*end, 1), "use-of-uninitialized-value");
#endif

  // Confirm we died a death of *SAN
  EXPECT_EQ(field.AddAlreadyReserved(), end);
  *end = 1;
  EXPECT_EQ(*end, 1);
}

TEST(RepeatedField, PoisonsMemoryOnAdd) {
  RepeatedField<int64_t> field;
  do {
    field.Add(0);
  } while (field.size() == field.Capacity());
  VerifyDeathOnWriteAndReadAccessBeyondEnd(field);
}

TEST(RepeatedField, PoisonsMemoryOnAddAlreadyReserved) {
  RepeatedField<int64_t> field;
  field.Reserve(2);
  field.AddAlreadyReserved();
  VerifyDeathOnWriteAndReadAccessBeyondEnd(field);
}

TEST(RepeatedField, PoisonsMemoryOnAddNAlreadyReserved) {
  RepeatedField<int64_t> field;
  field.Reserve(10);
  field.AddNAlreadyReserved(8);
  VerifyDeathOnWriteAndReadAccessBeyondEnd(field);
}

TEST(RepeatedField, PoisonsMemoryOnResize) {
  RepeatedField<int64_t> field;
  field.Add(0);
  do {
    field.Resize(field.size() + 1, 1);
  } while (field.size() == field.Capacity());
  VerifyDeathOnWriteAndReadAccessBeyondEnd(field);

  // Shrink size
  field.Resize(field.size() - 1, 1);
  VerifyDeathOnWriteAndReadAccessBeyondEnd(field);
}

TEST(RepeatedField, PoisonsMemoryOnTruncate) {
  RepeatedField<int64_t> field;
  field.Add(0);
  field.Add(1);
  field.Truncate(1);
  VerifyDeathOnWriteAndReadAccessBeyondEnd(field);
}

TEST(RepeatedField, PoisonsMemoryOnReserve) {
  RepeatedField<int64_t> field;
  field.Add(1);
  field.Reserve(field.Capacity() + 1);
  VerifyDeathOnWriteAndReadAccessBeyondEnd(field);
}

TEST(RepeatedField, PoisonsMemoryOnAssign) {
  RepeatedField<int64_t> src;
  RepeatedField<int64_t> field;
  src.Add(1);
  src.Add(2);
  field.Reserve(3);
  field = src;
  VerifyDeathOnWriteAndReadAccessBeyondEnd(field);
}

#endif

TEST(RepeatedField, Cleanups) {
  Arena arena;
  auto growth = internal::CleanupGrowth(
      arena, [&] { Arena::Create<RepeatedField<int>>(&arena); });
  EXPECT_THAT(growth.cleanups, testing::IsEmpty());

  void* ptr;
  growth = internal::CleanupGrowth(
      arena, [&] { ptr = Arena::Create<RepeatedField<absl::Cord>>(&arena); });
  EXPECT_THAT(growth.cleanups, testing::UnorderedElementsAre(ptr));
}

TEST(RepeatedField, InitialSooCapacity) {
  if (sizeof(void*) == 8) {
    EXPECT_EQ(RepeatedField<bool>().Capacity(), 3);
    EXPECT_EQ(RepeatedField<int32_t>().Capacity(), 2);
    EXPECT_EQ(RepeatedField<int64_t>().Capacity(), 1);
    EXPECT_EQ(RepeatedField<absl::Cord>().Capacity(), 0);
  } else {
    EXPECT_EQ(RepeatedField<bool>().Capacity(), 0);
    EXPECT_EQ(RepeatedField<int32_t>().Capacity(), 0);
    EXPECT_EQ(RepeatedField<int64_t>().Capacity(), 0);
    EXPECT_EQ(RepeatedField<absl::Cord>().Capacity(), 0);
  }
}

// ===================================================================

// Iterator tests stolen from net/proto/proto-array_unittest.

class RepeatedFieldIteratorTest : public testing::Test {
 protected:
  void SetUp() override {
    for (int i = 0; i < 3; ++i) {
      proto_array_.Add(i);
    }
  }

  RepeatedField<int> proto_array_;
};

TEST_F(RepeatedFieldIteratorTest, Convertible) {
  RepeatedField<int>::iterator iter = proto_array_.begin();
  RepeatedField<int>::const_iterator c_iter = iter;
  RepeatedField<int>::value_type value = *c_iter;
  EXPECT_EQ(0, value);
}

TEST_F(RepeatedFieldIteratorTest, MutableIteration) {
  RepeatedField<int>::iterator iter = proto_array_.begin();
  EXPECT_EQ(0, *iter);
  ++iter;
  EXPECT_EQ(1, *iter++);
  EXPECT_EQ(2, *iter);
  ++iter;
  EXPECT_TRUE(proto_array_.end() == iter);

  EXPECT_EQ(2, *(proto_array_.end() - 1));
}

TEST_F(RepeatedFieldIteratorTest, ConstIteration) {
  const RepeatedField<int>& const_proto_array = proto_array_;
  RepeatedField<int>::const_iterator iter = const_proto_array.begin();
  EXPECT_EQ(0, *iter);
  ++iter;
  EXPECT_EQ(1, *iter++);
  EXPECT_EQ(2, *iter);
  ++iter;
  EXPECT_TRUE(const_proto_array.end() == iter);
  EXPECT_EQ(2, *(const_proto_array.end() - 1));
}

TEST_F(RepeatedFieldIteratorTest, Mutation) {
  RepeatedField<int>::iterator iter = proto_array_.begin();
  *iter = 7;
  EXPECT_EQ(7, proto_array_.Get(0));
}

// -----------------------------------------------------------------------------
// Unit-tests for the insert iterators
// `google::protobuf::RepeatedFieldBackInserter`,
// `google::protobuf::AllocatedRepeatedFieldBackInserter`
// Ported from util/gtl/proto-array-iterators_unittest.

class RepeatedFieldInsertionIteratorsTest : public testing::Test {
 protected:
  std::list<double> halves;
  std::list<int> fibonacci;
  TestAllTypes protobuffer;

  void SetUp() override {
    fibonacci.push_back(1);
    fibonacci.push_back(1);
    fibonacci.push_back(2);
    fibonacci.push_back(3);
    fibonacci.push_back(5);
    fibonacci.push_back(8);
    std::copy(fibonacci.begin(), fibonacci.end(),
              RepeatedFieldBackInserter(protobuffer.mutable_repeated_int32()));

    halves.push_back(1.0);
    halves.push_back(0.5);
    halves.push_back(0.25);
    halves.push_back(0.125);
    halves.push_back(0.0625);
    std::copy(halves.begin(), halves.end(),
              RepeatedFieldBackInserter(protobuffer.mutable_repeated_double()));
  }
};

TEST_F(RepeatedFieldInsertionIteratorsTest, Fibonacci) {
  EXPECT_TRUE(std::equal(fibonacci.begin(), fibonacci.end(),
                         protobuffer.repeated_int32().begin()));
  EXPECT_TRUE(std::equal(protobuffer.repeated_int32().begin(),
                         protobuffer.repeated_int32().end(),
                         fibonacci.begin()));
}

TEST_F(RepeatedFieldInsertionIteratorsTest, Halves) {
  EXPECT_TRUE(std::equal(halves.begin(), halves.end(),
                         protobuffer.repeated_double().begin()));
  EXPECT_TRUE(std::equal(protobuffer.repeated_double().begin(),
                         protobuffer.repeated_double().end(), halves.begin()));
}

TEST(RepeatedField, CheckedGetOrAbortTest) {
  RepeatedField<int> field;

  // Empty container tests.
  EXPECT_DEATH(CheckedGetOrAbort(field, -1), "index: -1, size: 0");
  EXPECT_DEATH(CheckedGetOrAbort(field, field.size()), "index: 0, size: 0");

  // Non-empty container tests
  field.Add(5);
  field.Add(4);
  EXPECT_DEATH(CheckedGetOrAbort(field, 2), "index: 2, size: 2");
  EXPECT_DEATH(CheckedGetOrAbort(field, -1), "index: -1, size: 2");
}

TEST(RepeatedField, CheckedMutableOrAbortTest) {
  RepeatedField<int> field;

  // Empty container tests.
  EXPECT_DEATH(CheckedMutableOrAbort(&field, -1), "index: -1, size: 0");
  EXPECT_DEATH(CheckedMutableOrAbort(&field, field.size()),
               "index: 0, size: 0");

  // Non-empty container tests
  field.Add(5);
  field.Add(4);
  EXPECT_DEATH(CheckedMutableOrAbort(&field, 2), "index: 2, size: 2");
  EXPECT_DEATH(CheckedMutableOrAbort(&field, -1), "index: -1, size: 2");
}

}  // namespace

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
