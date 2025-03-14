// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/micro_string.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/base/config.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/port.h"


namespace google {
namespace protobuf {
namespace internal {
namespace {
constexpr size_t kMicroRepSize = sizeof(uint8_t) * 2;
constexpr size_t kLargeRepSize = sizeof(char*) + 2 * sizeof(uint32_t);

enum PreviousState { kInline, kMicroRep, kOwned, kUnowned, kString, kAlias };

static constexpr auto kUnownedPayload =
    MicroString::MakeUnownedPayload("0123456789");

class MicroStringPrevTest
    : public testing::TestWithParam<std::tuple<bool, PreviousState>> {
 protected:
  MicroStringPrevTest() : str_(MakeFromState(prev_state(), arena())) {}

  ~MicroStringPrevTest() override {
    if (!has_arena()) {
      str_.Destroy();
    }
  }

  static MicroString MakeFromState(PreviousState state, Arena* arena) {
    MicroString str;
    switch (state) {
      case kInline: {
        std::string input(MicroString::kInlineCapacity, 'x');
        str.Set(input, arena);
        ABSL_CHECK_EQ(str.Get(), input);
        break;
      }
      case kMicroRep:
        str.Set("Very long string", arena);
        ABSL_CHECK_EQ(str.Get(), "Very long string");
        break;
      case kOwned: {
        std::string very_very_long(MicroString::kMaxMicroRepCapacity + 1, 'x');
        str.Set(very_very_long, arena);
        ABSL_CHECK_EQ(str.Get(), very_very_long);
        break;
      }
      case kUnowned:
        str.SetUnowned(kUnownedPayload, arena);
        ABSL_CHECK_EQ(str.Get(), "0123456789");
        break;

      case kString: {
        static constexpr absl::string_view value =
            "This is a very long string too, which "
            "won't use std::string's inline rep.";
        str.SetString(std::string(value), arena);
        ABSL_CHECK_EQ(str.Get(), value);
        break;
      }
      case kAlias:
        str.SetAlias("Another long string, but aliased", arena);
        ABSL_CHECK_EQ(str.Get(), "Another long string, but aliased");
        break;
    }
    return str;
  }

  PreviousState prev_state() { return std::get<PreviousState>(GetParam()); }

  static auto IsStringRep(const MicroString& ms) {
    struct Robber : MicroString {
      using MicroString::is_large_rep;
      using MicroString::kString;
      using MicroString::large_rep_kind;
    };
    return (ms.*&Robber::is_large_rep)() &&
           (ms.*&Robber::large_rep_kind)() == Robber::kString;
  }

  void ExpectMemoryUsed(size_t prev_arena_used, bool allocated_on_arena,
                        size_t expected_string_used,
                        MicroString* str = nullptr) {
    if (str == nullptr) str = &str_;

    if (has_arena()) {
      const size_t expected_arena_increment =
          allocated_on_arena ? ArenaAlignDefault::Ceil(expected_string_used)
                             : 0;
      EXPECT_EQ(expected_arena_increment, arena_space_used() - prev_arena_used);
    }

    const size_t actual = str->SpaceUsedExcludingSelfLong();
    if (has_arena() && !IsStringRep(*str)) {
      EXPECT_EQ(actual, ArenaAlignDefault::Ceil(expected_string_used));
    } else {
      // When on heap we don't know how much we round up during allocation.
      // The actual must be at least what we expect.
      EXPECT_GE(actual, expected_string_used);
      // But it can be larger and we don't know how much. Round up a bit.
      EXPECT_LE(actual, expected_string_used + 16);
    }
  }

  bool has_arena() { return std::get<0>(GetParam()); }
  size_t arena_space_used() { return has_arena() ? arena()->SpaceUsed() : 0; }
  Arena* arena() { return has_arena() ? &arena_ : nullptr; }

  Arena arena_;
  MicroString str_;
};

struct Printer {
  static constexpr absl::string_view kNames[] = {"Inline",  "Micro",  "Owned",
                                                 "Unowned", "String", "Alias"};
  static_assert(kNames[kInline] == "Inline");
  static_assert(kNames[kMicroRep] == "Micro");
  static_assert(kNames[kOwned] == "Owned");
  static_assert(kNames[kUnowned] == "Unowned");
  static_assert(kNames[kString] == "String");
  static_assert(kNames[kAlias] == "Alias");
  template <typename T>
  std::string operator()(const T& in) const {
    return absl::StrCat(std::get<0>(in.param) ? "Arena" : "NoArena", "_",
                        kNames[std::get<1>(in.param)]);
  }
};

INSTANTIATE_TEST_SUITE_P(MicroStringTransitionTest, MicroStringPrevTest,
                         testing::Combine(testing::Bool(),
                                          testing::Values(kInline, kMicroRep,
                                                          kOwned, kUnowned,
                                                          kString, kAlias)),
                         Printer{});

TEST(MicroStringTest, InlineIsEnabledWhenExpected) {
#if defined(ABSL_IS_LITTLE_ENDIAN)
  constexpr bool kExpectInline = sizeof(uintptr_t) >= 8;
#else
  constexpr bool kExpectInline = false;
#endif
  if (kExpectInline) {
    EXPECT_TRUE(MicroString::kHasInlineRep);
    EXPECT_EQ(MicroString::kInlineCapacity, sizeof(MicroString) - 1);
  } else {
    EXPECT_FALSE(MicroString::kHasInlineRep);
    EXPECT_EQ(MicroString::kInlineCapacity, 0);
  }
}

TEST(MicroStringTest, DefaultIsEmpty) {
  MicroString str;
  EXPECT_EQ(str.Get(), "");
}

TEST(MicroStringTest, HasConstexprDefaultConstructor) {
  constexpr MicroString str;
  EXPECT_EQ(str.Get(), "");
}

TEST(MicroStringTest, ConstexprUnownedGlobal) {
  static constexpr auto payload = MicroString::MakeUnownedPayload("0123456789");
  static constexpr MicroString global_instance(payload);

  EXPECT_EQ("0123456789", global_instance.Get());
  EXPECT_EQ(static_cast<const void*>(payload.payload.payload),
            static_cast<const void*>(global_instance.Get().data()));
}

template <typename T>
void TestInline() {
  Arena arena;
  for (Arena* a : {static_cast<Arena*>(nullptr), &arena}) {
    for (size_t size = 0; size <= T::kInlineCapacity; ++size) {
      const absl::string_view input("ABCDEFGHIJKLMNOPQRSTUVWXYZ", size);
      T str;
      size_t used = arena.SpaceUsed();
      str.Set(input, a);
      EXPECT_EQ(str.Get(), input);
      EXPECT_EQ(used, arena.SpaceUsed());
      EXPECT_EQ(0, str.SpaceUsedExcludingSelfLong());

      // We explicitly don't call Destroy() here. If we allocated heap by
      // mistake it will be detected as a memory leak.
    }
  }
}

TEST(MicroStringTest, SetInlineFromClear) {
  if (!MicroString::kHasInlineRep) {
    GTEST_SKIP() << "Inline is not active";
  }

  TestInline<MicroString>();
  TestInline<MicroStringExtra<8>>();
  TestInline<MicroStringExtra<16>>();
}

template <int N>
void TestExtraCapacity(int expected_sizeof) {
  EXPECT_EQ(sizeof(MicroStringExtra<N>), expected_sizeof);
  EXPECT_EQ(MicroStringExtra<N>::kInlineCapacity, expected_sizeof - 1);
}

TEST(MicroStringTest, ExtraRequestedInlineSpace) {
  if (!MicroString::kHasInlineRep) {
    GTEST_SKIP() << "Inline is not active";
  }
  TestExtraCapacity<0>(8);
  TestExtraCapacity<1>(8);
  TestExtraCapacity<7>(8);
  TestExtraCapacity<8>(16);
  TestExtraCapacity<15>(16);
  TestExtraCapacity<16>(24);
  TestExtraCapacity<23>(24);
}

TEST(MicroStringTest, CapacityIsRoundedUp) {
  Arena arena;
  MicroString str;

  str.Set("0123456789", &arena);
  const size_t used = arena.SpaceUsed();
  EXPECT_EQ(str.Capacity(), 16 - kMicroRepSize);
  str.Set("01234567890123", &arena);
  EXPECT_EQ(used, arena.SpaceUsed());
}

TEST_P(MicroStringPrevTest, SetInline) {
  if (!MicroString::kHasInlineRep) {
    GTEST_SKIP() << "Inline is not active";
  }

  const absl::string_view input("ABCD");
  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.Set(input, arena());
  EXPECT_EQ(str_.Get(), input);
  // We never use more space than before, regardless of the previous state of
  // the class.
  EXPECT_EQ(used, arena_space_used());
  EXPECT_GE(self_used, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SetMicro) {
  for (size_t size : {str_.kInlineCapacity + 1, size_t{30}}) {
    const absl::string_view input("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", size);
    const size_t used = arena_space_used();
    const size_t self_used = str_.SpaceUsedExcludingSelfLong();
    const bool will_reuse = str_.Capacity() >= input.size();
    str_.Set(input, arena());
    EXPECT_EQ(str_.Get(), input);

    if (will_reuse) {
      // No change
      ExpectMemoryUsed(used, false, self_used);
    } else {
      ExpectMemoryUsed(used, true, kMicroRepSize + size);
    }
  }
}

TEST_P(MicroStringPrevTest, SetOwned) {
  for (size_t size : {str_.kMaxMicroRepCapacity + 1, size_t{300}}) {
    const std::string input(size, 'x');
    const size_t used = arena_space_used();
    const size_t self_used = str_.SpaceUsedExcludingSelfLong();
    const bool will_reuse = str_.Capacity() >= input.size();
    str_.Set(input, arena());
    EXPECT_EQ(str_.Get(), input);

    if (will_reuse) {
      ExpectMemoryUsed(used, false, self_used);
    } else {
      ExpectMemoryUsed(used, true, kLargeRepSize + size);
    }
  }
}

TEST_P(MicroStringPrevTest, SetAliasSmall) {
  if (!MicroString::kHasInlineRep) {
    GTEST_SKIP() << "Inline is not active";
  }
  const absl::string_view input("ABC");

  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.SetAlias(input, arena());
  auto out = str_.Get();
  if (prev_state() == kAlias) {
    // If we had an alias, we reuse the LargeRep to point to the new alias
    // regardless of size.
    EXPECT_EQ(out.data(), input.data());
  } else {
    // The data will be copied instead, because it is too small to alias.
    EXPECT_NE(out.data(), input.data());
  }
  EXPECT_EQ(out, input);

  // We should not need to allocate memory here in any case.
  ExpectMemoryUsed(used, false, self_used);
}

TEST_P(MicroStringPrevTest, SetAliasLarge) {
  const absl::string_view input("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

  const size_t used = arena_space_used();
  str_.SetAlias(input, arena());
  auto out = str_.Get();
  // Don't use op==, we want to check it points to the exact same buffer.
  EXPECT_EQ(out.data(), input.data());
  EXPECT_EQ(out.size(), input.size());

  ExpectMemoryUsed(used, prev_state() != kAlias, kLargeRepSize);
}

TEST_P(MicroStringPrevTest, SetUnowned) {
  static constexpr auto kUnownedPayload =
      MicroString::MakeUnownedPayload("This one is unowned.");

  const size_t used = arena_space_used();
  str_.SetUnowned(kUnownedPayload, arena());
  auto out = str_.Get();
  // Don't use op==, we want to check it points to the exact same buffer.
  EXPECT_EQ(out.data(), kUnownedPayload.payload.payload);
  EXPECT_EQ(out.size(), kUnownedPayload.payload.size);

  // Never uses more memory.
  ExpectMemoryUsed(used, false, 0);
}

TEST_P(MicroStringPrevTest, SetStringSmall) {
  if (!MicroString::kHasInlineRep) {
    GTEST_SKIP() << "Inline is not active";
  }

  std::string input(1, 'a');
  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  const bool will_reuse = str_.Capacity() >= input.size();
  str_.SetString(std::move(input), arena());
  // NOLINTNEXTLINE: We really didn't move from the input, so this is fine.
  EXPECT_EQ(str_.Get(), input);

  // Never uses more space.
  ExpectMemoryUsed(used, false, will_reuse ? self_used : 0);
}

TEST_P(MicroStringPrevTest, SetStringMedium) {
  std::string input(16, 'a');
  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.SetString(std::move(input), arena());
  // NOLINTNEXTLINE: We really didn't move from the input, so this is fine.
  EXPECT_EQ(str_.Get(), input);

  const bool will_reuse = !(prev_state() == kInline || prev_state() == kAlias ||
                            prev_state() == kUnowned);

  if (will_reuse) {
    ExpectMemoryUsed(used, false, self_used);
  } else {
    ExpectMemoryUsed(used, true, kMicroRepSize + input.size());
  }
}

TEST_P(MicroStringPrevTest, SetStringLarge) {
  std::string input(128, 'a');
  std::string copy = input;
  const char* copy_data = copy.data();
  const size_t used = arena_space_used();
  str_.SetString(std::move(copy), arena());
  EXPECT_EQ(str_.Get(), input);

  // Verify that the string was moved.
  EXPECT_EQ(copy_data, str_.Get().data());

  const size_t expected_use =
      prev_state() != kString ? kLargeRepSize + sizeof(std::string) : 0;

  // Never uses more space.
  if (has_arena()) {
    EXPECT_EQ(used + expected_use, arena_space_used());
  }
  EXPECT_EQ(kLargeRepSize + sizeof(std::string) + input.capacity() + /* \0 */ 1,
            str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, CopyConstruct) {
  const size_t used = arena_space_used();
  MicroString copy(arena(), str_);
  EXPECT_EQ(str_.Get(), copy.Get());

  size_t expected_use;
  switch (prev_state()) {
    case kUnowned:
    case kInline:
      // These won't use any memory.
      expected_use = 0;
      break;
    case kMicroRep:
    case kString:
    case kAlias:
      // These all copy as a normal setter
      expected_use = kMicroRepSize + str_.Get().size();
      break;
    case kOwned:
      expected_use = kLargeRepSize + str_.Get().size();
      break;
  }

  ExpectMemoryUsed(used, true, expected_use, &copy);

  if (!has_arena()) copy.Destroy();
}

TEST(MicroStringTest, UnownedIsPropagated) {
  MicroString src(kUnownedPayload);

  ASSERT_EQ(src.Get().data(), kUnownedPayload.payload.payload);

  {
    MicroString str(nullptr, src);
    EXPECT_EQ(str.Get().data(), src.Get().data());
    EXPECT_EQ(0, str.SpaceUsedExcludingSelfLong());
  }
  {
    MicroString str;
    EXPECT_NE(str.Get().data(), src.Get().data());
    str.SetUnowned(kUnownedPayload, nullptr);
    ASSERT_EQ(str.Get().data(), src.Get().data());
    EXPECT_EQ(0, str.SpaceUsedExcludingSelfLong());
  }
}

TEST_P(MicroStringPrevTest, AssignmentViaSetInline) {
  MicroString source = MakeFromState(kInline, arena());
  size_t used = arena_space_used();
  str_.Set(source, arena());
  EXPECT_EQ(str_.Get(), source.Get());
  // No new memory should be used.
  EXPECT_EQ(used, arena_space_used());
}

TEST_P(MicroStringPrevTest, AssignmentViaSetMicroRep) {
  MicroString source = MakeFromState(kMicroRep, arena());

  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  const bool will_reuse = str_.Capacity() >= source.Get().size();
  str_.Set(source, arena());
  EXPECT_EQ(str_.Get(), source.Get());
  if (will_reuse) {
    ExpectMemoryUsed(used, false, self_used);
  } else {
    ExpectMemoryUsed(used, true, kMicroRepSize + str_.Get().size());
  }

  if (!has_arena()) source.Destroy();
}

TEST_P(MicroStringPrevTest, AssignmentViaSetOwned) {
  MicroString source = MakeFromState(kOwned, arena());

  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  const bool will_reuse = str_.Capacity() >= source.Get().size();
  str_.Set(source, arena());
  EXPECT_EQ(str_.Get(), source.Get());
  if (will_reuse) {
    ExpectMemoryUsed(used, false, self_used);
  } else {
    ExpectMemoryUsed(used, true, kLargeRepSize + str_.Get().size());
  }

  if (!has_arena()) source.Destroy();
}

TEST_P(MicroStringPrevTest, AssignmentViaSetUnowned) {
  MicroString source = MakeFromState(kUnowned, arena());
  const size_t used = arena_space_used();
  str_.Set(source, arena());
  EXPECT_EQ(str_.Get(), source.Get());
  // No new memory should be used when setting an unowned value.
  EXPECT_EQ(used, arena_space_used());
  EXPECT_EQ(0, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, AssignmentViaSetString) {
  MicroString source = MakeFromState(kString, arena());

  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  const bool will_reuse = str_.Capacity() >= source.Get().size();
  str_.Set(source, arena());
  EXPECT_EQ(str_.Get(), source.Get());
  if (will_reuse) {
    ExpectMemoryUsed(used, false, self_used);
  } else {
    ExpectMemoryUsed(used, true, kMicroRepSize + str_.Get().size());
  }

  if (!has_arena()) source.Destroy();
}

TEST_P(MicroStringPrevTest, AssignmentViaSetAlias) {
  MicroString source = MakeFromState(kAlias, arena());

  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  const bool will_reuse = str_.Capacity() >= source.Get().size();
  str_.Set(source, arena());
  EXPECT_EQ(str_.Get(), source.Get());
  if (will_reuse) {
    ExpectMemoryUsed(used, false, self_used);
  } else {
    ExpectMemoryUsed(used, true, kMicroRepSize + str_.Get().size());
  }

  if (!has_arena()) source.Destroy();
}

TEST(MicroStringExtraTest, SettersWithinInline) {
  if (!MicroString::kHasInlineRep ||
      MicroStringExtra<8>::kInlineCapacity != 15) {
    GTEST_SKIP() << "Inline is not active";
  }

  Arena arena;
  size_t used = arena.SpaceUsed();
  size_t expected_use = ArenaAlignDefault::Ceil(kMicroRepSize + 16);
  MicroStringExtra<8> str15;
  // Setting 15 chars should work fine.
  str15.Set("123456789012345", &arena);
  EXPECT_EQ("123456789012345", str15.Get());
  EXPECT_EQ(used, arena.SpaceUsed());
  EXPECT_EQ(0, str15.SpaceUsedExcludingSelfLong());
  // But 16 should go in the heap.
  str15.Set("1234567890123456", &arena);
  EXPECT_EQ("1234567890123456", str15.Get());
  EXPECT_EQ(used + expected_use, arena.SpaceUsed());
  EXPECT_EQ(expected_use, str15.SpaceUsedExcludingSelfLong());

  used = arena.SpaceUsed();
  expected_use = ArenaAlignDefault::Ceil(kMicroRepSize + 24);
  // Same but a larger buffer.
  MicroStringExtra<16> str23;
  // Setting 15 chars should work fine.
  str23.Set("12345678901234567890123", &arena);
  EXPECT_EQ("12345678901234567890123", str23.Get());
  EXPECT_EQ(used, arena.SpaceUsed());
  EXPECT_EQ(0, str23.SpaceUsedExcludingSelfLong());
  // But 24 should go in the heap.
  str23.Set("123456789012345678901234", &arena);
  EXPECT_EQ("123456789012345678901234", str23.Get());
  EXPECT_EQ(used + expected_use, arena.SpaceUsed());
  EXPECT_EQ(expected_use, str23.SpaceUsedExcludingSelfLong());
}

TEST(MicroStringExtraTest, CopyConstructWithinInline) {
  if (!MicroString::kHasInlineRep) {
    GTEST_SKIP() << "Inline is not active";
  }

  Arena arena;
  const size_t used = arena.SpaceUsed();
  MicroStringExtra<16> inline_str;
  constexpr absl::string_view kStr10 = "1234567890";
  ABSL_CHECK_GT(kStr10.size(), MicroString::kInlineCapacity);
  ABSL_CHECK_LE(kStr10.size(), MicroStringExtra<16>::kInlineCapacity);
  inline_str.Set(kStr10, &arena);
  EXPECT_EQ(used, arena.SpaceUsed());
  MicroStringExtra<16> copy(nullptr, inline_str);
  EXPECT_EQ(kStr10, copy.Get());
  // Should not have used any extra memory.
  EXPECT_EQ(used, arena.SpaceUsed());
  EXPECT_EQ(0, copy.SpaceUsedExcludingSelfLong());
}

TEST(MicroStringExtraTest, SetStringUsesInlineSpace) {
  if (!MicroString::kHasInlineRep) {
    GTEST_SKIP() << "Inline is not active";
  }

  Arena arena;

  MicroStringExtra<40> str;
  const size_t used = arena.SpaceUsed();
  str.SetString(std::string(40, 'x'), &arena);
  // we can fit the chars in the inline space, so copy it.
  EXPECT_EQ(used, arena.SpaceUsed());

  std::string large(100, 'x');
  const size_t used_in_string = StringSpaceUsedExcludingSelfLong(large);
  str.SetString(std::move(large), &arena);
  // This one is too big, so we move the whole std::string.
  EXPECT_EQ(kLargeRepSize + sizeof(std::string), arena.SpaceUsed() - used);
  EXPECT_EQ(kLargeRepSize + sizeof(std::string) + used_in_string,
            str.SpaceUsedExcludingSelfLong());
}

size_t SpaceUsedExcludingSelfLong(const ArenaStringPtr& str) {
  return str.IsDefault() ? 0
                         : sizeof(std::string) +
                               StringSpaceUsedExcludingSelfLong(str.Get());
}

TEST(MicroStringTest, MemoryUsageComparison) {
  Arena arena;
  MicroString micro_str;
  ArenaStringPtr arena_str;
  arena_str.InitDefault();

  const std::string input(200, 'x');

  size_t size_min = 0;
  int64_t micro_str_used = 0;
  int64_t arena_str_used = 0;

  const auto print_range = [&](size_t size_max) {
    int64_t diff = micro_str_used - arena_str_used;
    absl::PrintF(
        "[%3d, %3d] MicroString-ArenaStringPtr=%3d (%s) MicroUsed=%3d "
        "ArenaStringPtrUsed=%3d\n",
        size_min, size_max, diff,
        diff == 0  ? "same "
        : diff < 0 ? "saves"
                   : "regrs",
        micro_str_used, arena_str_used);
  };
  for (size_t i = 1; i < input.size(); ++i) {
    absl::string_view this_input(input.data(), i);
    micro_str.Set(this_input, &arena);
    arena_str.Set(this_input, &arena);

    int64_t this_micro_str_used = micro_str.SpaceUsedExcludingSelfLong();
    int64_t this_arena_str_used = SpaceUsedExcludingSelfLong(arena_str);
    // We expect to always use the same or less memory.
    EXPECT_LE(this_micro_str_used, this_arena_str_used);

    int64_t diff = micro_str_used - arena_str_used;
    int64_t this_diff = this_micro_str_used - this_arena_str_used;

    if (this_diff != diff) {
      print_range(i - 1);
      size_min = i;
      micro_str_used = this_micro_str_used;
      arena_str_used = this_arena_str_used;
    }
  }
  print_range(input.size());
}


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

absl::string_view CodegenMicroStringGet(google::protobuf::internal::MicroString& str) {
  return str.Get();
}
absl::string_view CodegenArenaStringPtrGet(
    google::protobuf::internal::ArenaStringPtr& str) {
  return str.Get();
}
void CodegenMicroStringSet(google::protobuf::internal::MicroString& str,
                           absl::string_view input) {
  str.Set(input, nullptr);
}
void CodegenArenaStringPtrSet(google::protobuf::internal::ArenaStringPtr& str,
                              absl::string_view input) {
  str.Set(input, nullptr);
}
void CodegenMicroStringSetConstant(google::protobuf::internal::MicroString& str) {
  str.Set("value", nullptr);
}
void CodegenMicroStringExtraSetConstant(
    google::protobuf::internal::MicroStringExtra<8>& str) {
  str.Set("larger_value", nullptr);
}
void CodegenMicroStringInitOther(google::protobuf::internal::MicroString& str,
                                 const google::protobuf::internal::MicroString& other) {
  ::new (&str) google::protobuf::internal::MicroString(nullptr, other);
}
void CodegenMicroStringExtraInitOther(
    google::protobuf::internal::MicroStringExtra<8>& str,
    const google::protobuf::internal::MicroStringExtra<8>& other) {
  ::new (&str) google::protobuf::internal::MicroStringExtra<8>(nullptr, other);
}
void CodegenMicroStringSetOther(google::protobuf::internal::MicroString& str,
                                const google::protobuf::internal::MicroString& other) {
  str.Set(other, nullptr);
}
void CodegenMicroStringExtraSetOther(
    google::protobuf::internal::MicroStringExtra<8>& str,
    const google::protobuf::internal::MicroStringExtra<8>& other) {
  str.Set(other, nullptr);
}

int odr [[maybe_unused]] =
    (google::protobuf::internal::StrongPointer(&CodegenMicroStringGet),
     google::protobuf::internal::StrongPointer(&CodegenArenaStringPtrGet),
     google::protobuf::internal::StrongPointer(&CodegenMicroStringSetConstant),
     google::protobuf::internal::StrongPointer(&CodegenMicroStringExtraSetConstant),
     google::protobuf::internal::StrongPointer(&CodegenMicroStringSetOther),
     google::protobuf::internal::StrongPointer(&CodegenMicroStringExtraSetOther),
     google::protobuf::internal::StrongPointer(&CodegenMicroStringInitOther),
     google::protobuf::internal::StrongPointer(&CodegenMicroStringExtraInitOther), 0);
