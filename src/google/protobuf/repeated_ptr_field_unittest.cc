// Protocol Buffers - Google's data interchange format
// Copyright 2008, 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/repeated_ptr_field.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/numeric/bits.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/arena_test_util.h"
#include "google/protobuf/internal_visibility_for_testing.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace {

using ::proto2_unittest::TestAllTypes;
using ::proto2_unittest::TestMessageWithManyRepeatedPtrFields;
using ::testing::A;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::Le;

TEST(RepeatedPtrOverPtrsIteratorTest, Traits) {
  using It = RepeatedPtrField<std::string>::pointer_iterator;
  static_assert(std::is_same<It::value_type, std::string*>::value, "");
  static_assert(std::is_same<It::reference, std::string*&>::value, "");
  static_assert(std::is_same<It::pointer, std::string**>::value, "");
  static_assert(std::is_same<It::difference_type, std::ptrdiff_t>::value, "");
  static_assert(std::is_same<It::iterator_category,
                             std::random_access_iterator_tag>::value,
                "");
#if __cplusplus >= 202002L
  static_assert(
      std::is_same<It::iterator_concept, std::contiguous_iterator_tag>::value,
      "");
#else
  static_assert(std::is_same<It::iterator_concept,
                             std::random_access_iterator_tag>::value,
                "");
#endif
}

#ifdef __cpp_lib_to_address
TEST(RepeatedPtrOverPtrsIteratorTest, ToAddress) {
  // empty container
  RepeatedPtrField<std::string> field;
  EXPECT_THAT(std::to_address(field.pointer_begin()), A<std::string**>());
  EXPECT_EQ(std::to_address(field.pointer_begin()),
            std::to_address(field.pointer_end()));

  // "null" iterator
  using It = RepeatedPtrField<std::string>::pointer_iterator;
  EXPECT_THAT(std::to_address(It()), A<std::string**>());
}
#endif

TEST(ConstRepeatedPtrOverPtrsIterator, Traits) {
  using It = RepeatedPtrField<std::string>::const_pointer_iterator;
  static_assert(std::is_same<It::value_type, const std::string*>::value, "");
  static_assert(std::is_same<It::reference, const std::string* const&>::value,
                "");
  static_assert(std::is_same<It::pointer, const std::string* const*>::value,
                "");
  static_assert(std::is_same<It::difference_type, std::ptrdiff_t>::value, "");
  static_assert(std::is_same<It::iterator_category,
                             std::random_access_iterator_tag>::value,
                "");
#if __cplusplus >= 202002L
  static_assert(
      std::is_same<It::iterator_concept, std::contiguous_iterator_tag>::value,
      "");
#else
  static_assert(std::is_same<It::iterator_concept,
                             std::random_access_iterator_tag>::value,
                "");
#endif
}

TEST(RepeatedPtrFieldTest, SimpleAddWithStrings) {
  RepeatedPtrField<std::string> field;
  ASSERT_EQ(field.size(), 0);
  field.Add("foo");
  ASSERT_EQ(field.size(), 1);
  field.Add("bar");
  ASSERT_EQ(field.size(), 2);
  field.Add("buz");
  ASSERT_EQ(field.size(), 3);
  field.Add("qux");
  ASSERT_EQ(field.size(), 4);
  EXPECT_EQ(field.Get(0), "foo");
  EXPECT_EQ(field.Get(1), "bar");
  EXPECT_EQ(field.Get(2), "buz");
  EXPECT_EQ(field.Get(3), "qux");
}

TEST(RepeatedPtrFieldTest, SimpleAddWithMessages) {
  using TestType = TestAllTypes::NestedMessage;
  RepeatedPtrField<TestType> field;
  const auto make_val = [](int i) {
    TestType x;
    x.set_bb(i);
    return x;
  };
  ASSERT_EQ(field.size(), 0);
  field.Add(make_val(1));
  ASSERT_EQ(field.size(), 1);
  field.Add(make_val(2));
  ASSERT_EQ(field.size(), 2);
  field.Add(make_val(3));
  ASSERT_EQ(field.size(), 3);
  EXPECT_EQ(field.Get(0).bb(), 1);
  EXPECT_EQ(field.Get(1).bb(), 2);
  EXPECT_EQ(field.Get(2).bb(), 3);
}

TEST(RepeatedPtrFieldTest, MoveAdd) {
  RepeatedPtrField<TestAllTypes> field;
  TestAllTypes test_all_types;
  auto* optional_nested_message =
      test_all_types.mutable_optional_nested_message();
  optional_nested_message->set_bb(42);
  field.Add(std::move(test_all_types));

  EXPECT_EQ(optional_nested_message,
            field.Mutable(0)->mutable_optional_nested_message());
}

TEST(RepeatedPtrFieldTest, ConstInit) {
  PROTOBUF_CONSTINIT static RepeatedPtrField<std::string> field{};  // NOLINT
  EXPECT_TRUE(field.empty());
}

TEST(RepeatedPtrFieldTest, ClearThenReserveMore) {
  // Test that Reserve properly destroys the old internal array when it's forced
  // to allocate a new one, even when cleared-but-not-deleted objects are
  // present. Use a 'string' and > 16 bytes length so that the elements are
  // non-POD and allocate -- the leak checker will catch any skipped destructor
  // calls here.
  RepeatedPtrField<std::string> field;
  for (int i = 0; i < 32; i++) {
    *field.Add() = std::string("abcdefghijklmnopqrstuvwxyz0123456789");
  }
  EXPECT_EQ(32, field.size());
  field.Clear();
  EXPECT_EQ(0, field.size());
  EXPECT_LE(32, field.Capacity());

  field.Reserve(1024);
  EXPECT_EQ(0, field.size());
  EXPECT_LE(1024, field.Capacity());
  // Finish test -- |field| should destroy the cleared-but-not-yet-destroyed
  // strings.
}

// This helper overload set tests whether X::f can be called with a braced pair,
// X::f({a, b}) of std::string iterators (specifically, pointers: That call is
// ambiguous if and only if the call to ValidResolutionPointerRange is not.
template <typename X>
auto ValidResolutionPointerRange(const std::string* p)
    -> decltype(X::f({p, p + 2}), std::true_type{});
template <typename X>
std::false_type ValidResolutionPointerRange(void*);

TEST(RepeatedPtrFieldTest, UnambiguousConstructor) {
  struct X {
    static bool f(std::vector<std::string>) { return false; }
    static bool f(google::protobuf::RepeatedPtrField<std::string>) { return true; }

    static bool g(std::vector<int>) { return false; }
    static bool g(google::protobuf::RepeatedPtrField<std::string>) { return true; }
  };

  // RepeatedPtrField has no initializer-list constructor, and a constructor
  // from to const char* values is excluded by its constraints.
  EXPECT_FALSE(X::f({"abc", "xyz"}));

  // Construction from a pair of int* is also not ambiguous.
  int a[5] = {};
  EXPECT_FALSE(X::g({a, a + 5}));

  // Construction from string iterators for the unique string overload "g"
  // works.
  // Disabling this for now, this is actually ambiguous with libstdc++.
  // std::string b[2] = {"abc", "xyz"};
  // EXPECT_TRUE(X::g({b, b + 2}));

  // Construction from string iterators for "f" is ambiguous, since both
  // containers are equally good.
  //
  // X::f({b, b + 2});  // error => ValidResolutionPointerRange is unambiguous.
  EXPECT_FALSE(decltype(ValidResolutionPointerRange<X>(nullptr))::value);
}

TEST(RepeatedPtrFieldTest, Small) {
  RepeatedPtrField<std::string> field;

  EXPECT_TRUE(field.empty());
  EXPECT_EQ(field.size(), 0);

  field.Add()->assign("foo");

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 1);
  EXPECT_EQ(field.Get(0), "foo");
  EXPECT_EQ(field.at(0), "foo");

  field.Add()->assign("bar");

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), "foo");
  EXPECT_EQ(field.at(0), "foo");
  EXPECT_EQ(field.Get(1), "bar");
  EXPECT_EQ(field.at(1), "bar");

  field.Mutable(1)->assign("baz");

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), "foo");
  EXPECT_EQ(field.at(0), "foo");
  EXPECT_EQ(field.Get(1), "baz");
  EXPECT_EQ(field.at(1), "baz");

  field.RemoveLast();

  EXPECT_FALSE(field.empty());
  EXPECT_EQ(field.size(), 1);
  EXPECT_EQ(field.Get(0), "foo");
  EXPECT_EQ(field.at(0), "foo");

  field.Clear();

  EXPECT_TRUE(field.empty());
  EXPECT_EQ(field.size(), 0);
}

TEST(RepeatedPtrFieldTest, Large) {
  RepeatedPtrField<std::string> field;

  for (int i = 0; i < 16; i++) {
    *field.Add() += 'a' + i;
  }

  EXPECT_EQ(field.size(), 16);

  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field.Get(i).size(), 1);
    EXPECT_EQ(field.Get(i)[0], 'a' + i);
  }

  int min_expected_usage = 16 * sizeof(std::string);
  EXPECT_GE(field.SpaceUsedExcludingSelf(), min_expected_usage);
}

namespace {

template <typename Elem>
void CheckAllocationSizes() {
  using Field = RepeatedPtrField<Elem>;
  // Use a large initial block to make the checks below easier to predict.
  std::string buf(1 << 20, 0);
  Arena arena(&buf[0], buf.size());
  auto* field = Arena::Create<Field>(&arena);
  size_t prev_used = arena.SpaceUsed();

  for (int i = 0; i < 100; ++i) {
    field->Add(Elem{});
    if (sizeof(void*) == 8) {
      // For `RepeatedPtrField`, we also allocate the T on the arena.
      // Subtract those from the count.
      size_t new_used = arena.SpaceUsed() - (sizeof(Elem) * (i + 1));
      size_t last_alloc = new_used - prev_used;
      prev_used = new_used;

      // When we actually allocated something, check the size.
      if (last_alloc != 0) {
        // Must be `>= 16`, as expected by the Arena.
        ASSERT_GE(last_alloc, 16);
        // Must be of a power of two.
        size_t log2 = absl::bit_width(last_alloc) - 1;
        ASSERT_EQ((1 << log2), last_alloc);
      }

      // The byte size must be a multiple of 8 when not SOO.
      const int capacity_bytes = field->Capacity() * sizeof(Elem);
      if (capacity_bytes > internal::kSooCapacityBytes) {
        ASSERT_EQ(capacity_bytes % 8, 0);
      }
    }
  }
}

}  // namespace

TEST(RepeatedPtrFieldTest, ArenaAllocationSizesMatchExpectedValues) {
  EXPECT_NO_FATAL_FAILURE(CheckAllocationSizes<std::string>());
  EXPECT_NO_FATAL_FAILURE(CheckAllocationSizes<TestAllTypes::NestedMessage>());
}

TEST(RepeatedPtrFieldTest, NaturalGrowthOnArenasReuseBlocks) {
  using Elem = std::string;
  using Field = RepeatedPtrField<Elem>;

  Arena arena;
  std::vector<Field*> fields;
  static constexpr int kNumFields = 100;
  static constexpr int kNumElems = 1000;
  std::optional<int> common_capacity;
  for (int i = 0; i < kNumFields; ++i) {
    fields.push_back(Arena::Create<Field>(&arena));
    auto& field = *fields.back();
    for (int j = 0; j < kNumElems; ++j) {
      field.Add("");
    }
    if (!common_capacity.has_value()) {
      common_capacity = field.Capacity();
    } else {
      ASSERT_EQ(field.Capacity(), *common_capacity);
    }
  }

  const size_t expected = kNumFields * (*common_capacity) * sizeof(Elem*) +
                          kNumFields * kNumElems * sizeof(Elem);
  // Use a 2% slack for other overhead.
  // If we were not reusing the blocks, the actual value would be ~2x the
  // expected.
  EXPECT_THAT(arena.SpaceUsed(), AllOf(Ge(expected), Le(1.02 * expected)));
}

TEST(RepeatedPtrFieldTest, AddAndAssignRanges) {
  RepeatedPtrField<std::string> field;

  const char* vals[] = {"abc", "x", "yz", "xyzzy"};
  field.Assign(std::begin(vals), std::end(vals));

  ASSERT_EQ(field.size(), 4);
  EXPECT_EQ(field.Get(0), "abc");
  EXPECT_EQ(field.Get(1), "x");
  EXPECT_EQ(field.Get(2), "yz");
  EXPECT_EQ(field.Get(3), "xyzzy");

  field.Add(std::begin(vals), std::end(vals));
  ASSERT_EQ(field.size(), 8);
  EXPECT_EQ(field.Get(0), "abc");
  EXPECT_EQ(field.Get(1), "x");
  EXPECT_EQ(field.Get(2), "yz");
  EXPECT_EQ(field.Get(3), "xyzzy");
  EXPECT_EQ(field.Get(4), "abc");
  EXPECT_EQ(field.Get(5), "x");
  EXPECT_EQ(field.Get(6), "yz");
  EXPECT_EQ(field.Get(7), "xyzzy");
}

TEST(RepeatedPtrFieldTest, SwapSmallSmall) {
  RepeatedPtrField<std::string> field1;
  RepeatedPtrField<std::string> field2;

  EXPECT_TRUE(field1.empty());
  EXPECT_EQ(field1.size(), 0);
  EXPECT_TRUE(field2.empty());
  EXPECT_EQ(field2.size(), 0);

  field1.Add()->assign("foo");
  field1.Add()->assign("bar");

  EXPECT_FALSE(field1.empty());
  EXPECT_EQ(field1.size(), 2);
  EXPECT_EQ(field1.Get(0), "foo");
  EXPECT_EQ(field1.Get(1), "bar");

  EXPECT_TRUE(field2.empty());
  EXPECT_EQ(field2.size(), 0);

  field1.Swap(&field2);

  EXPECT_TRUE(field1.empty());
  EXPECT_EQ(field1.size(), 0);

  EXPECT_EQ(field2.size(), 2);
  EXPECT_EQ(field2.Get(0), "foo");
  EXPECT_EQ(field2.Get(1), "bar");
}

TEST(RepeatedPtrFieldTest, SwapLargeSmall) {
  RepeatedPtrField<std::string> field1;
  RepeatedPtrField<std::string> field2;

  field2.Add()->assign("foo");
  field2.Add()->assign("bar");
  for (int i = 0; i < 16; i++) {
    *field1.Add() += 'a' + i;
  }
  field1.Swap(&field2);

  EXPECT_EQ(field1.size(), 2);
  EXPECT_EQ(field1.Get(0), "foo");
  EXPECT_EQ(field1.Get(1), "bar");
  EXPECT_EQ(field2.size(), 16);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field2.Get(i).size(), 1);
    EXPECT_EQ(field2.Get(i)[0], 'a' + i);
  }
}

TEST(RepeatedPtrFieldTest, SwapLargeLarge) {
  RepeatedPtrField<std::string> field1;
  RepeatedPtrField<std::string> field2;

  field1.Add()->assign("foo");
  field1.Add()->assign("bar");
  for (int i = 0; i < 16; i++) {
    *field1.Add() += 'A' + i;
    *field2.Add() += 'a' + i;
  }
  field2.Swap(&field1);

  EXPECT_EQ(field1.size(), 16);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field1.Get(i).size(), 1);
    EXPECT_EQ(field1.Get(i)[0], 'a' + i);
  }
  EXPECT_EQ(field2.size(), 18);
  EXPECT_EQ(field2.Get(0), "foo");
  EXPECT_EQ(field2.Get(1), "bar");
  for (int i = 2; i < 18; i++) {
    EXPECT_EQ(field2.Get(i).size(), 1);
    EXPECT_EQ(field2.Get(i)[0], 'A' + i - 2);
  }
}

static int ReservedSpace(RepeatedPtrField<std::string>* field) {
  const std::string* const* ptr = field->data();
  do {
    field->Add();
  } while (field->data() == ptr);

  return field->size() - 1;
}

TEST(RepeatedPtrFieldTest, ReserveMoreThanDouble) {
  RepeatedPtrField<std::string> field;
  field.Reserve(20);

  EXPECT_LE(20, ReservedSpace(&field));
}

TEST(RepeatedPtrFieldTest, ReserveLessThanDouble) {
  RepeatedPtrField<std::string> field;
  field.Reserve(20);

  int capacity = field.Capacity();
  // Grow by 1.5x
  field.Reserve(capacity + (capacity >> 2));

  EXPECT_LE(2 * capacity, ReservedSpace(&field));
}

TEST(RepeatedPtrFieldTest, ReserveLessThanExisting) {
  RepeatedPtrField<std::string> field;
  field.Reserve(20);
  const std::string* const* previous_ptr = field.data();
  field.Reserve(10);

  EXPECT_EQ(previous_ptr, field.data());
  EXPECT_LE(20, ReservedSpace(&field));
}

TEST(RepeatedPtrFieldTest, ReserveDoesntLoseAllocated) {
  // Check that a bug is fixed:  An earlier implementation of Reserve()
  // failed to copy pointers to allocated-but-cleared objects, possibly
  // leading to segfaults.
  RepeatedPtrField<std::string> field;
  std::string* first = field.Add();
  field.RemoveLast();

  field.Reserve(20);
  EXPECT_EQ(first, field.Add());
}


// TODO: Re-evaluate if this is still needed once the bug is fixed.
TEST(RepeatedPtrFieldTest, AddRvalueToCleared) {
  // Check that an added rvalue correctly overwrites a cleared SOO element.
  {
    RepeatedPtrField<std::string> field;
    field.Add()->assign("foo");
    ASSERT_THAT(field, ElementsAre("foo"));
    field.RemoveLast();
    ASSERT_EQ(field.size(), 0);
    field.Add(std::string{"bar"});
    EXPECT_THAT(field, ElementsAre("bar"));
  }
  // Check that an added rvalue correctly overwrites a cleared non-SOO element
  // in the Rep.
  {
    RepeatedPtrField<std::string> field;
    field.Add()->assign("foo");
    field.Add()->assign("bar");
    field.Add()->assign("baz");
    EXPECT_THAT(field, ElementsAre("foo", "bar", "baz"));
    field.RemoveLast();
    EXPECT_THAT(field, ElementsAre("foo", "bar"));
    field.Add(std::string{"qux"});
    EXPECT_THAT(field, ElementsAre("foo", "bar", "qux"));
  }
}

// Test all code paths in AddAllocated().
TEST(RepeatedPtrFieldTest, AddAllocated) {
  RepeatedPtrField<std::string> field;
  while (field.size() < field.Capacity()) {
    field.Add()->assign("filler");
  }

  const auto ensure_at_capacity = [&] {
    while (field.size() < field.Capacity()) {
      field.Add()->assign("filler");
    }
  };
  const auto ensure_not_at_capacity = [&] { field.Reserve(field.size() + 1); };

  ensure_at_capacity();
  int index = field.size();

  // First branch:  Field is at capacity with no cleared objects.
  ASSERT_EQ(field.size(), field.Capacity());
  std::string* foo = new std::string("foo");
  field.AddAllocated(foo);
  EXPECT_EQ(index + 1, field.size());
  EXPECT_EQ(foo, &field.Get(index));

  // Last branch:  Field is not at capacity and there are no cleared objects.
  ensure_not_at_capacity();
  std::string* bar = new std::string("bar");
  field.AddAllocated(bar);
  ++index;
  EXPECT_EQ(index + 1, field.size());
  EXPECT_EQ(bar, &field.Get(index));

  // Third branch:  Field is not at capacity and there are no cleared objects.
  ensure_not_at_capacity();
  field.RemoveLast();
  std::string* baz = new std::string("baz");
  field.AddAllocated(baz);
  EXPECT_EQ(index + 1, field.size());
  EXPECT_EQ(baz, &field.Get(index));

  // Second branch:  Field is at capacity but has some cleared objects.
  ensure_at_capacity();
  field.RemoveLast();
  index = field.size();
  std::string* moo = new std::string("moo");
  field.AddAllocated(moo);
  EXPECT_EQ(index + 1, field.size());
  // We should have discarded the cleared object.
  EXPECT_EQ(moo, &field.Get(index));
}

TEST(RepeatedPtrFieldDeathTest, AddMethodsDontAcceptNull) {
#if !defined(NDEBUG)
  RepeatedPtrField<std::string> field;
  EXPECT_DEATH(field.AddAllocated(nullptr), "nullptr");
  EXPECT_DEATH(field.UnsafeArenaAddAllocated(nullptr), "nullptr");
#endif
}

TEST(RepeatedPtrFieldTest, AddAllocatedDifferentArena) {
  RepeatedPtrField<TestAllTypes> field;
  Arena arena;
  auto* msg = Arena::Create<TestAllTypes>(&arena);
  field.AddAllocated(msg);
}

TEST(RepeatedPtrFieldTest, MergeFromString) {
  RepeatedPtrField<std::string> source, destination;
  source.Add()->assign("4");
  source.Add()->assign("5");
  destination.Add()->assign("1");
  destination.Add()->assign("2");
  destination.Add()->assign("3");

  destination.MergeFrom(source);

  ASSERT_EQ(5, destination.size());
  EXPECT_EQ("1", destination.Get(0));
  EXPECT_EQ("2", destination.Get(1));
  EXPECT_EQ("3", destination.Get(2));
  EXPECT_EQ("4", destination.Get(3));
  EXPECT_EQ("5", destination.Get(4));

  destination.Clear();

  ASSERT_EQ(0, destination.size());

  destination.MergeFrom(source);

  ASSERT_EQ(2, destination.size());
  EXPECT_EQ("4", destination.Get(0));
  EXPECT_EQ("5", destination.Get(1));

  delete destination.ReleaseLast();

  ASSERT_EQ(1, destination.size());
  EXPECT_EQ("4", destination.Get(0));

  destination.MergeFrom(source);

  ASSERT_EQ(3, destination.size());
  EXPECT_EQ("4", destination.Get(0));
  EXPECT_EQ("4", destination.Get(1));
  EXPECT_EQ("5", destination.Get(2));
}

TEST(RepeatedPtrFieldTest, MergeFromMessage) {
  RepeatedPtrField<TestAllTypes::NestedMessage> source, destination;
  source.Add()->set_bb(4);
  source.Add()->set_bb(5);
  destination.Add()->set_bb(1);
  destination.Add()->set_bb(2);
  destination.Add()->set_bb(3);

  destination.MergeFrom(source);

  ASSERT_EQ(5, destination.size());
  EXPECT_EQ(1, destination.Get(0).bb());
  EXPECT_EQ(2, destination.Get(1).bb());
  EXPECT_EQ(3, destination.Get(2).bb());
  EXPECT_EQ(4, destination.Get(3).bb());
  EXPECT_EQ(5, destination.Get(4).bb());

  destination.Clear();

  ASSERT_EQ(0, destination.size());

  destination.MergeFrom(source);

  ASSERT_EQ(2, destination.size());
  EXPECT_EQ(4, destination.Get(0).bb());
  EXPECT_EQ(5, destination.Get(1).bb());

  delete destination.ReleaseLast();

  ASSERT_EQ(1, destination.size());
  EXPECT_EQ(4, destination.Get(0).bb());

  destination.MergeFrom(source);

  ASSERT_EQ(3, destination.size());
  EXPECT_EQ(4, destination.Get(0).bb());
  EXPECT_EQ(4, destination.Get(1).bb());
  EXPECT_EQ(5, destination.Get(2).bb());
}

TEST(RepeatedPtrFieldTest, MergeFromStringWithArena) {
  using Field = RepeatedPtrField<std::string>;
  Arena arena;
  Field* source = Arena::Create<Field>(&arena);
  Field* destination = Arena::Create<Field>(&arena);
  source->Add()->assign("4");
  source->Add()->assign("5");
  destination->Add()->assign("1");
  destination->Add()->assign("2");
  destination->Add()->assign("3");

  destination->MergeFrom(*source);

  ASSERT_EQ(5, destination->size());
  EXPECT_EQ("1", destination->Get(0));
  EXPECT_EQ("2", destination->Get(1));
  EXPECT_EQ("3", destination->Get(2));
  EXPECT_EQ("4", destination->Get(3));
  EXPECT_EQ("5", destination->Get(4));
}

TEST(RepeatedPtrFieldTest, MergeFromMessageWithArena) {
  using Field = RepeatedPtrField<TestAllTypes::NestedMessage>;
  Arena arena;
  Field* source = Arena::Create<Field>(&arena);
  Field* destination = Arena::Create<Field>(&arena);
  source->Add()->set_bb(4);
  source->Add()->set_bb(5);
  destination->Add()->set_bb(1);
  destination->Add()->set_bb(2);
  destination->Add()->set_bb(3);

  destination->MergeFrom(*source);

  ASSERT_EQ(5, destination->size());
  EXPECT_EQ(1, destination->Get(0).bb());
  EXPECT_EQ(2, destination->Get(1).bb());
  EXPECT_EQ(3, destination->Get(2).bb());
  EXPECT_EQ(4, destination->Get(3).bb());
  EXPECT_EQ(5, destination->Get(4).bb());
}


TEST(RepeatedPtrFieldTest, CopyFrom) {
  RepeatedPtrField<std::string> source, destination;
  source.Add()->assign("4");
  source.Add()->assign("5");
  destination.Add()->assign("1");
  destination.Add()->assign("2");
  destination.Add()->assign("3");

  destination.CopyFrom(source);

  ASSERT_EQ(2, destination.size());
  EXPECT_EQ("4", destination.Get(0));
  EXPECT_EQ("5", destination.Get(1));
}

TEST(RepeatedPtrFieldTest, CopyFromSelf) {
  RepeatedPtrField<std::string> me;
  me.Add()->assign("1");
  me.CopyFrom(me);
  ASSERT_EQ(1, me.size());
  EXPECT_EQ("1", me.Get(0));
}

TEST(RepeatedPtrFieldTest, Erase) {
  RepeatedPtrField<std::string> me;
  RepeatedPtrField<std::string>::iterator it = me.erase(me.begin(), me.end());
  EXPECT_TRUE(me.begin() == it);
  EXPECT_EQ(0, me.size());

  *me.Add() = "1";
  *me.Add() = "2";
  *me.Add() = "3";
  it = me.erase(me.begin(), me.end());
  EXPECT_TRUE(me.begin() == it);
  EXPECT_EQ(0, me.size());

  *me.Add() = "4";
  *me.Add() = "5";
  *me.Add() = "6";
  it = me.erase(me.begin() + 2, me.end());
  EXPECT_TRUE(me.begin() + 2 == it);
  EXPECT_EQ(2, me.size());
  EXPECT_EQ("4", me.Get(0));
  EXPECT_EQ("5", me.Get(1));

  *me.Add() = "6";
  *me.Add() = "7";
  *me.Add() = "8";
  it = me.erase(me.begin() + 1, me.begin() + 3);
  EXPECT_TRUE(me.begin() + 1 == it);
  EXPECT_EQ(3, me.size());
  EXPECT_EQ("4", me.Get(0));
  EXPECT_EQ("7", me.Get(1));
  EXPECT_EQ("8", me.Get(2));
}

TEST(RepeatedPtrFieldTest, CopyConstruct) {
  auto token = internal::InternalVisibilityForTesting{};
  RepeatedPtrField<std::string> source;
  source.Add()->assign("1");
  source.Add()->assign("2");

  RepeatedPtrField<std::string> destination1(source);
  ASSERT_EQ(2, destination1.size());
  EXPECT_EQ("1", destination1.Get(0));
  EXPECT_EQ("2", destination1.Get(1));

  RepeatedPtrField<std::string> destination2(token, nullptr, source);
  ASSERT_EQ(2, destination2.size());
  EXPECT_EQ("1", destination2.Get(0));
  EXPECT_EQ("2", destination2.Get(1));
}

TEST(RepeatedPtrFieldTest, CopyConstructWithArena) {
  auto token = internal::InternalVisibilityForTesting{};
  RepeatedPtrField<std::string> source;
  source.Add()->assign("1");
  source.Add()->assign("2");

  Arena arena;
  RepeatedPtrField<std::string> destination(token, &arena, source);
  ASSERT_EQ(2, destination.size());
  EXPECT_EQ("1", destination.Get(0));
  EXPECT_EQ("2", destination.Get(1));
}

TEST(RepeatedPtrFieldTest, IteratorConstruct_String) {
  std::vector<std::string> values;
  values.push_back("1");
  values.push_back("2");

  RepeatedPtrField<std::string> field(values.begin(), values.end());
  ASSERT_EQ(values.size(), field.size());
  EXPECT_EQ(values[0], field.Get(0));
  EXPECT_EQ(values[1], field.Get(1));

  RepeatedPtrField<std::string> other(field.begin(), field.end());
  ASSERT_EQ(values.size(), other.size());
  EXPECT_EQ(values[0], other.Get(0));
  EXPECT_EQ(values[1], other.Get(1));
}

TEST(RepeatedPtrFieldTest, IteratorConstruct_Proto) {
  typedef TestAllTypes::NestedMessage Nested;
  std::vector<Nested> values;
  values.push_back(Nested());
  values.back().set_bb(1);
  values.push_back(Nested());
  values.back().set_bb(2);

  RepeatedPtrField<Nested> field(values.begin(), values.end());
  ASSERT_EQ(values.size(), field.size());
  EXPECT_EQ(values[0].bb(), field.Get(0).bb());
  EXPECT_EQ(values[1].bb(), field.Get(1).bb());

  RepeatedPtrField<Nested> other(field.begin(), field.end());
  ASSERT_EQ(values.size(), other.size());
  EXPECT_EQ(values[0].bb(), other.Get(0).bb());
  EXPECT_EQ(values[1].bb(), other.Get(1).bb());
}

TEST(RepeatedPtrFieldTest, SmallOptimization) {
  // Properties checked here are not part of the contract of RepeatedPtrField,
  // but we test them to verify that SSO is working as expected by the
  // implementation.

  // We use an arena to easily measure memory usage, but not needed.
  Arena arena;
  auto* array = Arena::Create<RepeatedPtrField<std::string>>(&arena);
  EXPECT_EQ(array->Capacity(), 1);
  EXPECT_EQ(array->SpaceUsedExcludingSelf(), 0);
  std::string str;
  auto usage_before = arena.SpaceUsed();
  // We use UnsafeArenaAddAllocated just to grow the array without creating
  // objects or causing extra cleanup costs in the arena to make the
  // measurements simpler.
  array->UnsafeArenaAddAllocated(&str);
  // No backing array, just the string.
  EXPECT_EQ(array->SpaceUsedExcludingSelf(), sizeof(str));
  // We have not used any arena space.
  EXPECT_EQ(usage_before, arena.SpaceUsed());
  // Verify the string is where we think it is.
  EXPECT_EQ(&*array->begin(), &str);
  EXPECT_EQ(array->pointer_begin()[0], &str);
  auto is_inlined = [array]() {
    return std::less_equal<void*>{}(array, &*array->pointer_begin()) &&
           std::less<void*>{}(&*array->pointer_begin(), array + 1);
  };
  // The T** in pointer_begin points into the sso in the object.
  EXPECT_TRUE(is_inlined());

  // Adding a second object stops sso.
  std::string str2;
  array->UnsafeArenaAddAllocated(&str2);
  EXPECT_EQ(array->Capacity(), 3);
  // Backing array and the strings.
  EXPECT_EQ(array->SpaceUsedExcludingSelf(),
            (1 + array->Capacity()) * sizeof(void*) + 2 * sizeof(str));
  // We used some arena space now.
  EXPECT_LT(usage_before, arena.SpaceUsed());
  // And the pointer_begin is not in the sso anymore.
  EXPECT_FALSE(is_inlined());
}

TEST(RepeatedPtrFieldTest, CopyAssign) {
  RepeatedPtrField<std::string> source, destination;
  source.Add()->assign("4");
  source.Add()->assign("5");
  destination.Add()->assign("1");
  destination.Add()->assign("2");
  destination.Add()->assign("3");

  destination = source;

  ASSERT_EQ(2, destination.size());
  EXPECT_EQ("4", destination.Get(0));
  EXPECT_EQ("5", destination.Get(1));
}

TEST(RepeatedPtrFieldTest, SelfAssign) {
  // Verify that assignment to self does not destroy data.
  RepeatedPtrField<std::string> source, *p;
  p = &source;
  source.Add()->assign("7");
  source.Add()->assign("8");

  *p = source;

  ASSERT_EQ(2, source.size());
  EXPECT_EQ("7", source.Get(0));
  EXPECT_EQ("8", source.Get(1));
}

TEST(RepeatedPtrFieldTest, MoveConstruct) {
  {
    RepeatedPtrField<std::string> source;
    *source.Add() = "1";
    *source.Add() = "2";
    const std::string* const* data = source.data();
    RepeatedPtrField<std::string> destination = std::move(source);
    EXPECT_EQ(data, destination.data());
    EXPECT_THAT(destination, ElementsAre("1", "2"));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_TRUE(source.empty());
  }
  {
    Arena arena;
    RepeatedPtrField<std::string>* source =
        Arena::Create<RepeatedPtrField<std::string>>(&arena);
    *source->Add() = "1";
    *source->Add() = "2";
    RepeatedPtrField<std::string> destination = std::move(*source);
    EXPECT_EQ(nullptr, destination.GetArena());
    EXPECT_THAT(destination, ElementsAre("1", "2"));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_THAT(*source, ElementsAre("1", "2"));
  }
}

TEST(RepeatedPtrFieldTest, MoveAssign) {
  {
    RepeatedPtrField<std::string> source;
    *source.Add() = "1";
    *source.Add() = "2";
    RepeatedPtrField<std::string> destination;
    *destination.Add() = "3";
    const std::string* const* source_data = source.data();
    destination = std::move(source);
    EXPECT_EQ(source_data, destination.data());
    EXPECT_THAT(destination, ElementsAre("1", "2"));
    EXPECT_THAT(source, ElementsAre("3"));
  }
  {
    Arena arena;
    RepeatedPtrField<std::string>* source =
        Arena::Create<RepeatedPtrField<std::string>>(&arena);
    *source->Add() = "1";
    *source->Add() = "2";
    RepeatedPtrField<std::string>* destination =
        Arena::Create<RepeatedPtrField<std::string>>(&arena);
    *destination->Add() = "3";
    const std::string* const* source_data = source->data();
    *destination = std::move(*source);
    EXPECT_EQ(source_data, destination->data());
    EXPECT_THAT(*destination, ElementsAre("1", "2"));
    EXPECT_THAT(*source, ElementsAre("3"));
  }
  {
    Arena source_arena;
    RepeatedPtrField<std::string>* source =
        Arena::Create<RepeatedPtrField<std::string>>(&source_arena);
    *source->Add() = "1";
    *source->Add() = "2";
    Arena destination_arena;
    RepeatedPtrField<std::string>* destination =
        Arena::Create<RepeatedPtrField<std::string>>(&destination_arena);
    *destination->Add() = "3";
    *destination = std::move(*source);
    EXPECT_THAT(*destination, ElementsAre("1", "2"));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_THAT(*source, ElementsAre("1", "2"));
  }
  {
    Arena arena;
    RepeatedPtrField<std::string>* source =
        Arena::Create<RepeatedPtrField<std::string>>(&arena);
    *source->Add() = "1";
    *source->Add() = "2";
    RepeatedPtrField<std::string> destination;
    *destination.Add() = "3";
    destination = std::move(*source);
    EXPECT_THAT(destination, ElementsAre("1", "2"));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_THAT(*source, ElementsAre("1", "2"));
  }
  {
    RepeatedPtrField<std::string> source;
    *source.Add() = "1";
    *source.Add() = "2";
    Arena arena;
    RepeatedPtrField<std::string>* destination =
        Arena::Create<RepeatedPtrField<std::string>>(&arena);
    *destination->Add() = "3";
    *destination = std::move(source);
    EXPECT_THAT(*destination, ElementsAre("1", "2"));
    // This property isn't guaranteed but it's useful to have a test that would
    // catch changes in this area.
    EXPECT_THAT(source, ElementsAre("1", "2"));
  }
  {
    RepeatedPtrField<std::string> field;
    // An alias to defeat -Wself-move.
    RepeatedPtrField<std::string>& alias = field;
    *field.Add() = "1";
    *field.Add() = "2";
    const std::string* const* data = field.data();
    field = std::move(alias);
    EXPECT_EQ(data, field.data());
    EXPECT_THAT(field, ElementsAre("1", "2"));
  }
  {
    Arena arena;
    RepeatedPtrField<std::string>* field =
        Arena::Create<RepeatedPtrField<std::string>>(&arena);
    *field->Add() = "1";
    *field->Add() = "2";
    const std::string* const* data = field->data();
    *field = std::move(*field);
    EXPECT_EQ(data, field->data());
    EXPECT_THAT(*field, ElementsAre("1", "2"));
  }
}

TEST(RepeatedPtrFieldTest, MutableDataIsMutable) {
  RepeatedPtrField<std::string> field;
  *field.Add() = "1";
  EXPECT_EQ("1", field.Get(0));
  // The fact that this line compiles would be enough, but we'll check the
  // value anyway.
  std::string** data = field.mutable_data();
  **data = "2";
  EXPECT_EQ("2", field.Get(0));
}

TEST(RepeatedPtrFieldTest, SubscriptOperators) {
  RepeatedPtrField<std::string> field;
  *field.Add() = "1";
  EXPECT_EQ("1", field.Get(0));
  EXPECT_EQ("1", field[0]);
  EXPECT_EQ(field.Mutable(0), &field[0]);
  const RepeatedPtrField<std::string>& const_field = field;
  EXPECT_EQ(*field.data(), &const_field[0]);
}

TEST(RepeatedPtrFieldTest, ExtractSubrange) {
  // Exhaustively test every subrange in arrays of all sizes from 0 through 9
  // with 0 through 3 cleared elements at the end.
  for (int sz = 0; sz < 10; ++sz) {
    for (int num = 0; num <= sz; ++num) {
      for (int start = 0; start < sz - num; ++start) {
        for (int extra = 0; extra < 4; ++extra) {
          std::vector<std::string*> subject;

          // Create an array with "sz" elements and "extra" cleared elements.
          // Use an arena to avoid copies from debug-build stability checks.
          Arena arena;
          auto& field = *Arena::Create<RepeatedPtrField<std::string>>(&arena);
          for (int i = 0; i < sz + extra; ++i) {
            subject.push_back(new std::string());
            field.AddAllocated(subject[i]);
          }
          EXPECT_EQ(field.size(), sz + extra);
          for (int i = 0; i < extra; ++i) field.RemoveLast();
          EXPECT_EQ(field.size(), sz);

          // Create a catcher array and call ExtractSubrange.
          std::string* catcher[10];
          for (int i = 0; i < 10; ++i) catcher[i] = nullptr;
          field.ExtractSubrange(start, num, catcher);

          // Does the resulting array have the right size?
          EXPECT_EQ(field.size(), sz - num);

          // Were the removed elements extracted into the catcher array?
          for (int i = 0; i < num; ++i)
            EXPECT_EQ(*catcher[i], *subject[start + i]);
          EXPECT_EQ(nullptr, catcher[num]);

          // Does the resulting array contain the right values?
          for (int i = 0; i < start; ++i)
            EXPECT_EQ(field.Mutable(i), subject[i]);
          for (int i = start; i < field.size(); ++i)
            EXPECT_EQ(field.Mutable(i), subject[i + num]);

          // Reinstate the cleared elements.
          for (int i = 0; i < extra; ++i) field.Add();
          EXPECT_EQ(field.size(), sz - num + extra);

          // Make sure the extra elements are all there (in some order).
          for (int i = sz; i < sz + extra; ++i) {
            int count = 0;
            for (int j = sz; j < sz + extra; ++j) {
              if (field.Mutable(j - num) == subject[i]) count += 1;
            }
            EXPECT_EQ(count, 1);
          }

          // Release the caught elements.
          for (int i = 0; i < num; ++i) delete catcher[i];
        }
      }
    }
  }
}

TEST(RepeatedPtrFieldTest, DeleteSubrange) {
  // DeleteSubrange is a trivial extension of ExtendSubrange.
}

TEST(RepeatedPtrFieldTest, Cleanups) {
  Arena arena;
  auto growth = internal::CleanupGrowth(
      arena, [&] { Arena::Create<RepeatedPtrField<std::string>>(&arena); });
  EXPECT_THAT(growth.cleanups, testing::IsEmpty());

  growth = internal::CleanupGrowth(
      arena, [&] { Arena::Create<RepeatedPtrField<TestAllTypes>>(&arena); });
  EXPECT_THAT(growth.cleanups, testing::IsEmpty());
}


TEST(RepeatedPtrFieldDeathTest, CheckedGetOrAbortTest) {
  RepeatedPtrField<std::string> field;

  // Empty container tests.
  EXPECT_DEATH(internal::CheckedGetOrAbort(field, -1), "index: -1, size: 0");
  EXPECT_DEATH(internal::CheckedGetOrAbort(field, field.size()),
               "index: 0, size: 0");

  // Non-empty container tests
  field.Add()->assign("foo");
  field.Add()->assign("bar");
  EXPECT_DEATH(internal::CheckedGetOrAbort(field, 2), "index: 2, size: 2");
  EXPECT_DEATH(internal::CheckedGetOrAbort(field, -1), "index: -1, size: 2");
}

TEST(RepeatedPtrFieldDeathTest, CheckedMutableOrAbortTest) {
  RepeatedPtrField<std::string> field;

  // Empty container tests.
  EXPECT_DEATH(internal::CheckedMutableOrAbort(&field, -1),
               "index: -1, size: 0");
  EXPECT_DEATH(internal::CheckedMutableOrAbort(&field, field.size()),
               "index: 0, size: 0");

  // Non-empty container tests
  field.Add()->assign("foo");
  field.Add()->assign("bar");
  EXPECT_DEATH(internal::CheckedMutableOrAbort(&field, 2), "index: 2, size: 2");
  EXPECT_DEATH(internal::CheckedMutableOrAbort(&field, -1),
               "index: -1, size: 2");
}
// ===================================================================

class RepeatedPtrFieldIteratorTest : public testing::Test {
 protected:
  void SetUp() override {
    proto_array_.Add()->assign("foo");
    proto_array_.Add()->assign("bar");
    proto_array_.Add()->assign("baz");
  }

  RepeatedPtrField<std::string> proto_array_;
};

TEST_F(RepeatedPtrFieldIteratorTest, Convertible) {
  RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
  RepeatedPtrField<std::string>::const_iterator c_iter = iter;
  RepeatedPtrField<std::string>::value_type value = *c_iter;
  EXPECT_EQ("foo", value);
}

TEST_F(RepeatedPtrFieldIteratorTest, MutableIteration) {
  RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
  EXPECT_EQ("foo", *iter);
  ++iter;
  EXPECT_EQ("bar", *(iter++));
  EXPECT_EQ("baz", *iter);
  ++iter;
  EXPECT_TRUE(proto_array_.end() == iter);
  EXPECT_EQ("baz", *(--proto_array_.end()));
}

TEST_F(RepeatedPtrFieldIteratorTest, ConstIteration) {
  const RepeatedPtrField<std::string>& const_proto_array = proto_array_;
  RepeatedPtrField<std::string>::const_iterator iter =
      const_proto_array.begin();
  iter - const_proto_array.cbegin();
  EXPECT_EQ("foo", *iter);
  ++iter;
  EXPECT_EQ("bar", *(iter++));
  EXPECT_EQ("baz", *iter);
  ++iter;
  EXPECT_TRUE(const_proto_array.end() == iter);
  EXPECT_EQ("baz", *(--const_proto_array.end()));
}

TEST_F(RepeatedPtrFieldIteratorTest, MutableReverseIteration) {
  RepeatedPtrField<std::string>::reverse_iterator iter = proto_array_.rbegin();
  EXPECT_EQ("baz", *iter);
  ++iter;
  EXPECT_EQ("bar", *(iter++));
  EXPECT_EQ("foo", *iter);
  ++iter;
  EXPECT_TRUE(proto_array_.rend() == iter);
  EXPECT_EQ("foo", *(--proto_array_.rend()));
}

TEST_F(RepeatedPtrFieldIteratorTest, ConstReverseIteration) {
  const RepeatedPtrField<std::string>& const_proto_array = proto_array_;
  RepeatedPtrField<std::string>::const_reverse_iterator iter =
      const_proto_array.rbegin();
  EXPECT_EQ("baz", *iter);
  ++iter;
  EXPECT_EQ("bar", *(iter++));
  EXPECT_EQ("foo", *iter);
  ++iter;
  EXPECT_TRUE(const_proto_array.rend() == iter);
  EXPECT_EQ("foo", *(--const_proto_array.rend()));
}

TEST_F(RepeatedPtrFieldIteratorTest, RandomAccess) {
  RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
  RepeatedPtrField<std::string>::iterator iter2 = iter;
  ++iter2;
  ++iter2;
  EXPECT_TRUE(iter + 2 == iter2);
  EXPECT_TRUE(iter == iter2 - 2);
  EXPECT_EQ("baz", iter[2]);
  EXPECT_EQ("baz", *(iter + 2));
  EXPECT_EQ(3, proto_array_.end() - proto_array_.begin());
}

TEST_F(RepeatedPtrFieldIteratorTest, RandomAccessConst) {
  RepeatedPtrField<std::string>::const_iterator iter = proto_array_.cbegin();
  RepeatedPtrField<std::string>::const_iterator iter2 = iter;
  ++iter2;
  ++iter2;
  EXPECT_TRUE(iter + 2 == iter2);
  EXPECT_TRUE(iter == iter2 - 2);
  EXPECT_EQ("baz", iter[2]);
  EXPECT_EQ("baz", *(iter + 2));
  EXPECT_EQ(3, proto_array_.cend() - proto_array_.cbegin());
}

TEST_F(RepeatedPtrFieldIteratorTest, DifferenceConstConversion) {
  EXPECT_EQ(3, proto_array_.end() - proto_array_.cbegin());
  EXPECT_EQ(3, proto_array_.cend() - proto_array_.begin());
}

TEST_F(RepeatedPtrFieldIteratorTest, Comparable) {
  RepeatedPtrField<std::string>::const_iterator iter = proto_array_.begin();
  RepeatedPtrField<std::string>::const_iterator iter2 = iter + 1;
  EXPECT_TRUE(iter == iter);
  EXPECT_TRUE(iter != iter2);
  EXPECT_TRUE(iter < iter2);
  EXPECT_TRUE(iter <= iter2);
  EXPECT_TRUE(iter <= iter);
  EXPECT_TRUE(iter2 > iter);
  EXPECT_TRUE(iter2 >= iter);
  EXPECT_TRUE(iter >= iter);
}

TEST_F(RepeatedPtrFieldIteratorTest, ComparableConstConversion) {
  RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
  RepeatedPtrField<std::string>::const_iterator iter2 = iter + 1;
  EXPECT_TRUE(iter == iter);
  EXPECT_TRUE(iter == proto_array_.cbegin());
  EXPECT_TRUE(proto_array_.cbegin() == iter);
  EXPECT_TRUE(iter != iter2);
  EXPECT_TRUE(iter2 != iter);
  EXPECT_TRUE(iter < iter2);
  EXPECT_TRUE(iter <= iter2);
  EXPECT_TRUE(iter <= iter);
  EXPECT_TRUE(iter2 > iter);
  EXPECT_TRUE(iter2 >= iter);
  EXPECT_TRUE(iter >= iter);
}

// Uninitialized iterator does not point to any of the RepeatedPtrField.
TEST_F(RepeatedPtrFieldIteratorTest, UninitializedIterator) {
  RepeatedPtrField<std::string>::iterator iter;
  EXPECT_TRUE(iter != proto_array_.begin());
  EXPECT_TRUE(iter != proto_array_.begin() + 1);
  EXPECT_TRUE(iter != proto_array_.begin() + 2);
  EXPECT_TRUE(iter != proto_array_.begin() + 3);
  EXPECT_TRUE(iter != proto_array_.end());
}

TEST_F(RepeatedPtrFieldIteratorTest, STLAlgorithms_lower_bound) {
  proto_array_.Clear();
  proto_array_.Add()->assign("a");
  proto_array_.Add()->assign("c");
  proto_array_.Add()->assign("d");
  proto_array_.Add()->assign("n");
  proto_array_.Add()->assign("p");
  proto_array_.Add()->assign("x");
  proto_array_.Add()->assign("y");

  std::string v = "f";
  RepeatedPtrField<std::string>::const_iterator it =
      std::lower_bound(proto_array_.begin(), proto_array_.end(), v);

  EXPECT_EQ(*it, "n");
  EXPECT_TRUE(it == proto_array_.begin() + 3);
}

TEST_F(RepeatedPtrFieldIteratorTest, Mutation) {
  RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
  *iter = "moo";
  EXPECT_EQ("moo", proto_array_.Get(0));
}

// -------------------------------------------------------------------

class RepeatedPtrFieldPtrsIteratorTest : public testing::Test {
 protected:
  void SetUp() override {
    proto_array_.Add()->assign("foo");
    proto_array_.Add()->assign("bar");
    proto_array_.Add()->assign("baz");
    const_proto_array_ = &proto_array_;
  }

  RepeatedPtrField<std::string> proto_array_;
  const RepeatedPtrField<std::string>* const_proto_array_;
};

TEST_F(RepeatedPtrFieldPtrsIteratorTest, ConvertiblePtr) {
  RepeatedPtrField<std::string>::pointer_iterator iter =
      proto_array_.pointer_begin();
  static_cast<void>(iter);
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, ConvertibleConstPtr) {
  RepeatedPtrField<std::string>::const_pointer_iterator iter =
      const_proto_array_->pointer_begin();
  static_cast<void>(iter);
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, MutablePtrIteration) {
  RepeatedPtrField<std::string>::pointer_iterator iter =
      proto_array_.pointer_begin();
  EXPECT_EQ("foo", **iter);
  ++iter;
  EXPECT_EQ("bar", **(iter++));
  EXPECT_EQ("baz", **iter);
  ++iter;
  EXPECT_TRUE(proto_array_.pointer_end() == iter);
  EXPECT_EQ("baz", **(--proto_array_.pointer_end()));
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, MutableConstPtrIteration) {
  RepeatedPtrField<std::string>::const_pointer_iterator iter =
      const_proto_array_->pointer_begin();
  EXPECT_EQ("foo", **iter);
  ++iter;
  EXPECT_EQ("bar", **(iter++));
  EXPECT_EQ("baz", **iter);
  ++iter;
  EXPECT_TRUE(const_proto_array_->pointer_end() == iter);
  EXPECT_EQ("baz", **(--const_proto_array_->pointer_end()));
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, RandomPtrAccess) {
  RepeatedPtrField<std::string>::pointer_iterator iter =
      proto_array_.pointer_begin();
  RepeatedPtrField<std::string>::pointer_iterator iter2 = iter;
  ++iter2;
  ++iter2;
  EXPECT_TRUE(iter + 2 == iter2);
  EXPECT_TRUE(iter == iter2 - 2);
  EXPECT_EQ("baz", *iter[2]);
  EXPECT_EQ("baz", **(iter + 2));
  EXPECT_EQ(3, proto_array_.end() - proto_array_.begin());
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, RandomConstPtrAccess) {
  RepeatedPtrField<std::string>::const_pointer_iterator iter =
      const_proto_array_->pointer_begin();
  RepeatedPtrField<std::string>::const_pointer_iterator iter2 = iter;
  ++iter2;
  ++iter2;
  EXPECT_TRUE(iter + 2 == iter2);
  EXPECT_TRUE(iter == iter2 - 2);
  EXPECT_EQ("baz", *iter[2]);
  EXPECT_EQ("baz", **(iter + 2));
  EXPECT_EQ(3, const_proto_array_->end() - const_proto_array_->begin());
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, DifferenceConstConversion) {
  EXPECT_EQ(3,
            proto_array_.pointer_end() - const_proto_array_->pointer_begin());
  EXPECT_EQ(3,
            const_proto_array_->pointer_end() - proto_array_.pointer_begin());
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, ComparablePtr) {
  RepeatedPtrField<std::string>::pointer_iterator iter =
      proto_array_.pointer_begin();
  RepeatedPtrField<std::string>::pointer_iterator iter2 = iter + 1;
  EXPECT_TRUE(iter == iter);
  EXPECT_TRUE(iter != iter2);
  EXPECT_TRUE(iter < iter2);
  EXPECT_TRUE(iter <= iter2);
  EXPECT_TRUE(iter <= iter);
  EXPECT_TRUE(iter2 > iter);
  EXPECT_TRUE(iter2 >= iter);
  EXPECT_TRUE(iter >= iter);
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, ComparableConstPtr) {
  RepeatedPtrField<std::string>::const_pointer_iterator iter =
      const_proto_array_->pointer_begin();
  RepeatedPtrField<std::string>::const_pointer_iterator iter2 = iter + 1;
  EXPECT_TRUE(iter == iter);
  EXPECT_TRUE(iter != iter2);
  EXPECT_TRUE(iter < iter2);
  EXPECT_TRUE(iter <= iter2);
  EXPECT_TRUE(iter <= iter);
  EXPECT_TRUE(iter2 > iter);
  EXPECT_TRUE(iter2 >= iter);
  EXPECT_TRUE(iter >= iter);
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, ComparableConstConversion) {
  RepeatedPtrField<std::string>::pointer_iterator iter =
      proto_array_.pointer_begin();
  RepeatedPtrField<std::string>::const_pointer_iterator iter2 = iter + 1;
  EXPECT_TRUE(iter == iter);
  EXPECT_TRUE(iter == const_proto_array_->pointer_begin());
  EXPECT_TRUE(const_proto_array_->pointer_begin() == iter);
  EXPECT_TRUE(iter != iter2);
  EXPECT_TRUE(iter2 != iter);
  EXPECT_TRUE(iter < iter2);
  EXPECT_TRUE(iter <= iter2);
  EXPECT_TRUE(iter <= iter);
  EXPECT_TRUE(iter2 > iter);
  EXPECT_TRUE(iter2 >= iter);
  EXPECT_TRUE(iter >= iter);
}

// Uninitialized iterator does not point to any of the RepeatedPtrOverPtrs.
// Dereferencing an uninitialized iterator crashes the process.
TEST_F(RepeatedPtrFieldPtrsIteratorTest, UninitializedPtrIterator) {
  RepeatedPtrField<std::string>::pointer_iterator iter;
  EXPECT_TRUE(iter != proto_array_.pointer_begin());
  EXPECT_TRUE(iter != proto_array_.pointer_begin() + 1);
  EXPECT_TRUE(iter != proto_array_.pointer_begin() + 2);
  EXPECT_TRUE(iter != proto_array_.pointer_begin() + 3);
  EXPECT_TRUE(iter != proto_array_.pointer_end());
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, UninitializedConstPtrIterator) {
  RepeatedPtrField<std::string>::const_pointer_iterator iter;
  EXPECT_TRUE(iter != const_proto_array_->pointer_begin());
  EXPECT_TRUE(iter != const_proto_array_->pointer_begin() + 1);
  EXPECT_TRUE(iter != const_proto_array_->pointer_begin() + 2);
  EXPECT_TRUE(iter != const_proto_array_->pointer_begin() + 3);
  EXPECT_TRUE(iter != const_proto_array_->pointer_end());
}

// This comparison functor is required by the tests for RepeatedPtrOverPtrs.
// They operate on strings and need to compare strings as strings in
// any stl algorithm, even though the iterator returns a pointer to a
// string
// - i.e. *iter has type std::string*.
struct StringLessThan {
  bool operator()(const std::string* z, const std::string* y) const {
    return *z < *y;
  }
};

TEST_F(RepeatedPtrFieldPtrsIteratorTest, PtrSTLAlgorithms_lower_bound) {
  proto_array_.Clear();
  proto_array_.Add()->assign("a");
  proto_array_.Add()->assign("c");
  proto_array_.Add()->assign("d");
  proto_array_.Add()->assign("n");
  proto_array_.Add()->assign("p");
  proto_array_.Add()->assign("x");
  proto_array_.Add()->assign("y");

  {
    std::string v = "f";
    RepeatedPtrField<std::string>::pointer_iterator it =
        std::lower_bound(proto_array_.pointer_begin(),
                         proto_array_.pointer_end(), &v, StringLessThan());

    ABSL_CHECK(*it != nullptr);

    EXPECT_EQ(**it, "n");
    EXPECT_TRUE(it == proto_array_.pointer_begin() + 3);
  }
  {
    std::string v = "f";
    RepeatedPtrField<std::string>::const_pointer_iterator it = std::lower_bound(
        const_proto_array_->pointer_begin(), const_proto_array_->pointer_end(),
        &v, StringLessThan());

    ABSL_CHECK(*it != nullptr);

    EXPECT_EQ(**it, "n");
    EXPECT_EQ(it, const_proto_array_->pointer_begin() + 3);
  }
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, PtrMutation) {
  RepeatedPtrField<std::string>::pointer_iterator iter =
      proto_array_.pointer_begin();
  **iter = "moo";
  EXPECT_EQ("moo", proto_array_.Get(0));
  EXPECT_EQ("bar", proto_array_.Get(1));
  EXPECT_EQ("baz", proto_array_.Get(2));
  ++iter;
  delete *iter;
  *iter = new std::string("a");
  ++iter;
  delete *iter;
  *iter = new std::string("b");
  EXPECT_EQ("a", proto_array_.Get(1));
  EXPECT_EQ("b", proto_array_.Get(2));
}

TEST_F(RepeatedPtrFieldPtrsIteratorTest, Sort) {
  proto_array_.Add()->assign("c");
  proto_array_.Add()->assign("d");
  proto_array_.Add()->assign("n");
  proto_array_.Add()->assign("p");
  proto_array_.Add()->assign("a");
  proto_array_.Add()->assign("y");
  proto_array_.Add()->assign("x");
  EXPECT_EQ("foo", proto_array_.Get(0));
  EXPECT_EQ("n", proto_array_.Get(5));
  EXPECT_EQ("x", proto_array_.Get(9));
  std::sort(proto_array_.pointer_begin(), proto_array_.pointer_end(),
            StringLessThan());
  EXPECT_EQ("a", proto_array_.Get(0));
  EXPECT_EQ("baz", proto_array_.Get(2));
  EXPECT_EQ("y", proto_array_.Get(9));
}

// -----------------------------------------------------------------------------
// Unit-tests for the insert iterators
// `google::protobuf::RepeatedPtrFieldBackInserter`,
// `google::protobuf::AllocatedRepeatedPtrFieldBackInserter`
// Ported from util/gtl/proto-array-iterators_unittest.

class RepeatedPtrFieldInsertionIteratorsTest : public testing::Test {
 protected:
  using Nested = TestAllTypes::NestedMessage;

  std::vector<std::string> words_;
  Nested nesteds_[2];
  std::vector<Nested*> nested_ptrs_;
  TestAllTypes protobuffer_;

  void SetUp() override {
    words_.push_back("Able");
    words_.push_back("was");
    words_.push_back("I");
    words_.push_back("ere");
    words_.push_back("I");
    words_.push_back("saw");
    words_.push_back("Elba");
    std::copy(
        words_.begin(), words_.end(),
        RepeatedFieldBackInserter(protobuffer_.mutable_repeated_string()));

    nesteds_[0].set_bb(17);
    nesteds_[1].set_bb(4711);
    std::copy(&nesteds_[0], &nesteds_[2],
              RepeatedFieldBackInserter(
                  protobuffer_.mutable_repeated_nested_message()));

    nested_ptrs_.push_back(new Nested);
    nested_ptrs_.back()->set_bb(170);
    nested_ptrs_.push_back(new Nested);
    nested_ptrs_.back()->set_bb(47110);
    std::copy(nested_ptrs_.begin(), nested_ptrs_.end(),
              RepeatedFieldBackInserter(
                  protobuffer_.mutable_repeated_nested_message()));
  }

  void TearDown() override {
    for (auto ptr : nested_ptrs_) {
      delete ptr;
    }
  }
};

TEST_F(RepeatedPtrFieldInsertionIteratorsTest, Words1) {
  ASSERT_EQ(words_.size(), protobuffer_.repeated_string_size());
  for (size_t i = 0; i < words_.size(); ++i) {
    EXPECT_EQ(words_.at(i), protobuffer_.repeated_string(i));
  }
}

TEST_F(RepeatedPtrFieldInsertionIteratorsTest, Words2) {
  words_.clear();
  words_.push_back("sing");
  words_.push_back("a");
  words_.push_back("song");
  words_.push_back("of");
  words_.push_back("six");
  words_.push_back("pence");
  protobuffer_.mutable_repeated_string()->Clear();
  std::copy(
      words_.begin(), words_.end(),
      RepeatedPtrFieldBackInserter(protobuffer_.mutable_repeated_string()));
  ASSERT_EQ(words_.size(), protobuffer_.repeated_string_size());
  for (size_t i = 0; i < words_.size(); ++i) {
    EXPECT_EQ(words_.at(i), protobuffer_.repeated_string(i));
  }
}

TEST_F(RepeatedPtrFieldInsertionIteratorsTest, Nesteds) {
  ASSERT_EQ(protobuffer_.repeated_nested_message_size(), 4);
  EXPECT_EQ(protobuffer_.repeated_nested_message(0).bb(), 17);
  EXPECT_EQ(protobuffer_.repeated_nested_message(1).bb(), 4711);
  EXPECT_EQ(protobuffer_.repeated_nested_message(2).bb(), 170);
  EXPECT_EQ(protobuffer_.repeated_nested_message(3).bb(), 47110);
}

TEST_F(RepeatedPtrFieldInsertionIteratorsTest,
       AllocatedRepeatedPtrFieldWithStringIntData) {
  std::vector<Nested*> data;
  TestAllTypes goldenproto;
  for (int i = 0; i < 10; ++i) {
    Nested* new_data = new Nested;
    new_data->set_bb(i);
    data.push_back(new_data);

    new_data = goldenproto.add_repeated_nested_message();
    new_data->set_bb(i);
  }
  TestAllTypes testproto;
  std::copy(data.begin(), data.end(),
            AllocatedRepeatedPtrFieldBackInserter(
                testproto.mutable_repeated_nested_message()));
  EXPECT_EQ(testproto.DebugString(), goldenproto.DebugString());
}

TEST_F(RepeatedPtrFieldInsertionIteratorsTest,
       AllocatedRepeatedPtrFieldWithString) {
  std::vector<std::string*> data;
  TestAllTypes goldenproto;
  for (int i = 0; i < 10; ++i) {
    std::string* new_data = new std::string;
    *new_data = absl::StrCat("name-", i);
    data.push_back(new_data);

    new_data = goldenproto.add_repeated_string();
    *new_data = absl::StrCat("name-", i);
  }
  TestAllTypes testproto;
  std::copy(data.begin(), data.end(),
            AllocatedRepeatedPtrFieldBackInserter(
                testproto.mutable_repeated_string()));
  EXPECT_EQ(testproto.DebugString(), goldenproto.DebugString());
}

TEST_F(RepeatedPtrFieldInsertionIteratorsTest,
       UnsafeArenaAllocatedRepeatedPtrFieldWithStringIntData) {
  std::vector<Nested*> data;
  Arena arena;
  auto* goldenproto = Arena::Create<TestAllTypes>(&arena);
  for (int i = 0; i < 10; ++i) {
    auto* new_data = goldenproto->add_repeated_nested_message();
    new_data->set_bb(i);
    data.push_back(new_data);
  }
  auto* testproto = Arena::Create<TestAllTypes>(&arena);
  std::copy(data.begin(), data.end(),
            UnsafeArenaAllocatedRepeatedPtrFieldBackInserter(
                testproto->mutable_repeated_nested_message()));
  EXPECT_EQ(testproto->DebugString(), goldenproto->DebugString());
}

TEST_F(RepeatedPtrFieldInsertionIteratorsTest,
       UnsafeArenaAllocatedRepeatedPtrFieldWithString) {
  std::vector<std::string*> data;
  Arena arena;
  auto* goldenproto = Arena::Create<TestAllTypes>(&arena);
  for (int i = 0; i < 10; ++i) {
    auto* new_data = goldenproto->add_repeated_string();
    *new_data = absl::StrCat("name-", i);
    data.push_back(new_data);
  }
  auto* testproto = Arena::Create<TestAllTypes>(&arena);
  std::copy(data.begin(), data.end(),
            UnsafeArenaAllocatedRepeatedPtrFieldBackInserter(
                testproto->mutable_repeated_string()));
  EXPECT_EQ(testproto->DebugString(), goldenproto->DebugString());
}

TEST_F(RepeatedPtrFieldInsertionIteratorsTest, MoveStrings) {
  std::vector<std::string> src = {"a", "b", "c", "d"};
  std::vector<std::string> copy =
      src;  // copy since move leaves in undefined state
  TestAllTypes testproto;
  std::move(copy.begin(), copy.end(),
            RepeatedFieldBackInserter(testproto.mutable_repeated_string()));

  ASSERT_THAT(testproto.repeated_string(), testing::ElementsAreArray(src));
}

TEST_F(RepeatedPtrFieldInsertionIteratorsTest, MoveProtos) {
  auto make_nested = [](int32_t x) {
    Nested ret;
    ret.set_bb(x);
    return ret;
  };
  std::vector<Nested> src = {make_nested(3), make_nested(5), make_nested(7)};
  std::vector<Nested> copy = src;  // copy since move leaves in undefined state
  TestAllTypes testproto;
  std::move(
      copy.begin(), copy.end(),
      RepeatedFieldBackInserter(testproto.mutable_repeated_nested_message()));

  ASSERT_EQ(src.size(), testproto.repeated_nested_message_size());
  for (size_t i = 0; i < src.size(); ++i) {
    EXPECT_EQ(src[i].DebugString(),
              testproto.repeated_nested_message(i).DebugString());
  }
}


}  // namespace
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
