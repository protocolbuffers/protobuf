// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/micro_string.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
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

struct MicroStringTestPeer {
  static bool IsStringRep(const MicroString& str) {
    return str.is_large_rep() && str.large_rep_kind() == str.kString;
  }
};

namespace {
constexpr size_t kMicroRepSize = sizeof(uint8_t) * 2;
constexpr size_t kLargeRepSize = sizeof(char*) + 2 * sizeof(uint32_t);

enum PreviousState { kInline, kMicroRep, kOwned, kUnowned, kString, kAlias };

static constexpr auto kUnownedPayload =
    MicroString::MakeUnownedPayload("0123456789");

static constexpr auto kInlineInput =
    absl::string_view("0123456789").substr(0, MicroString::kInlineCapacity);

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
        str.Set(std::string(value), arena);
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
    if (has_arena() && !MicroStringTestPeer::IsStringRep(*str)) {
      EXPECT_EQ(actual, ArenaAlignDefault::Ceil(expected_string_used));
    } else {
      // When on heap we don't know how much we round up during allocation.
      // The actual must be at least what we expect.
      EXPECT_GE(actual, expected_string_used);
      // But it can be larger and we don't know how much. Round up a bit.
      EXPECT_LE(actual, 1.1 * expected_string_used + 32);
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

TEST(MicroStringTest, CheckExpectedInlineBufferSize) {
  EXPECT_EQ(MicroString::kInlineCapacity, sizeof(MicroString) - 1);
}

TEST(MicroStringTest, DefaultIsEmpty) {
  MicroString str;
  EXPECT_EQ(str.Get(), "");
}

TEST(MicroStringTest, ArenaConstructor) {
  MicroString str(static_cast<Arena*>(nullptr));
  EXPECT_EQ(str.Get(), "");

  Arena arena;
  MicroString str2(&arena);
  EXPECT_EQ(str2.Get(), "");
}

TEST(MicroStringTest, InitDefault) {
  alignas(MicroString) char buffer[sizeof(MicroString)];
  // Scribble the memory.
  memset(buffer, 0xCD, sizeof(buffer));
  MicroString* str = reinterpret_cast<MicroString*>(buffer);
  str->InitDefault();
  EXPECT_EQ(str->Get(), "");
  str->Set("Foo", nullptr);
  EXPECT_EQ(str->Get(), "Foo");
  str->Destroy();
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
  TestInline<MicroString>();
  TestInline<MicroStringExtra<8>>();
  TestInline<MicroStringExtra<16>>();
}

template <typename T>
std::string MakeControl(const T& t) {
  return std::string(t);
}
template <typename T>
std::string MakeControl(const std::reference_wrapper<T>& t) {
  return std::string(t.get());
}

template <typename S, typename T>
void SupportsExpectedInputType(T&& t) {
  const std::string control = MakeControl(t);

  Arena arena;
  S str;
  str.Set(std::forward<T>(t), &arena);
  EXPECT_EQ(str.Get(), control);
}

template <typename S>
void SupportsExpectedInputTypes() {
  std::string str = "Foo";
  absl::string_view view = "Foo";

  SupportsExpectedInputType<S>(view);
  // char array
  SupportsExpectedInputType<S>("Foo");
  // char pointer
  SupportsExpectedInputType<S>(static_cast<const char*>("Foo"));
  // string&&
  SupportsExpectedInputType<S>(std::string("Foo"));
  // string&
  SupportsExpectedInputType<S>(str);
  // const string&
  SupportsExpectedInputType<S>(std::as_const(str));
  // reference_wrappers
  SupportsExpectedInputType<S>(std::cref(view));
  SupportsExpectedInputType<S>(std::cref("Foo"));
  SupportsExpectedInputType<S>(std::cref(str));
}

TEST(MicroStringTest, SupportsExpectedInputTypes) {
  SupportsExpectedInputTypes<MicroString>();
  SupportsExpectedInputTypes<MicroStringExtra<15>>();
}

TEST(MicroStringTest, CapacityIsRoundedUpOnArena) {
  Arena arena;
  MicroString str;

  str.Set("0123456789", &arena);
  size_t used = arena.SpaceUsed();
  EXPECT_EQ(str.Capacity(), 16 - kMicroRepSize);
  str.Set("01234567890123", &arena);
  EXPECT_EQ(used, arena.SpaceUsed());

  std::string long_input(1001, 'x');
  str.Set(long_input, &arena);
  used = arena.SpaceUsed();
  const size_t expected_capacity = 1008 - (kLargeRepSize % 8);
  EXPECT_EQ(str.Capacity(), expected_capacity);
  long_input = std::string(expected_capacity, 'x');
  str.Set(long_input, &arena);
  EXPECT_EQ(used, arena.SpaceUsed());
}

TEST(MicroStringTest, CapacityIsRoundedUpOnHeap) {
  MicroString str;

  // We don't know the exact buffer size the allocator will give us so try a few
  // and verify loosely.
  const std::string very_long(1000, 'x');

  // For MicroRep
  for (size_t i = 10; i < 20; ++i) {
    str.Set(absl::string_view(very_long).substr(0, i), nullptr);
    EXPECT_GE(str.Capacity(), i);
    EXPECT_EQ((str.Capacity() + kMicroRepSize) % sizeof(void*), 0);
  }

  // For OwnedRep
  for (size_t i = 300; i < 340; ++i) {
    str.Set(absl::string_view(very_long).substr(0, i), nullptr);
    EXPECT_GE(str.Capacity(), i);
    EXPECT_EQ((str.Capacity() + kLargeRepSize) % sizeof(void*), 0);
  }

  str.Destroy();
}

TEST(MicroStringTest, CapacityRoundingUpStaysWithinBoundsForMicroRep) {
  const auto get_capacity_for_size = [&](size_t size) {
    MicroString str;
    std::string input = std::string(size, 'x');
    str.Set(input, nullptr);
    EXPECT_EQ(str.Get(), input);
    size_t cap = str.Capacity();
    str.Destroy();
    return cap;
  };

  EXPECT_EQ(get_capacity_for_size(200), 208 - kMicroRepSize);

  // These are in the boundary
  EXPECT_EQ(get_capacity_for_size(253), 256 - kMicroRepSize);
  // This is the maximum capacity for MicroRep
  EXPECT_EQ(get_capacity_for_size(254), 256 - kMicroRepSize);

  // This one jumps to LargeRep
  EXPECT_GE(get_capacity_for_size(255), 256);
}

TEST(MicroStringTest, PoisonsTheUnusedCapacity) {
  if (!internal::HasMemoryPoisoning()) {
    GTEST_SKIP() << "Memory poisoning is not enabled.";
  }

  MicroString str;

  std::string buf(500, 'x');

  const auto check = [&](size_t size) {
    SCOPED_TRACE(size);
    if (size != 0) {
      EXPECT_FALSE(internal::IsMemoryPoisoned(str.Get().data() + size - 1));
    }
    EXPECT_TRUE(internal::IsMemoryPoisoned(str.Get().data() + size));
  };
  const auto set = [&](size_t size) {
    str.Set(absl::string_view(buf).substr(0, size), nullptr);
    check(size);
  };

  set(10);
  // grow a bit on the existing buffer
  set(11);
  // shrink a bit
  set(5);
  // clear
  str.Clear();
  check(0);
  // and grow again
  set(6);

  // Now grow to large rep
  set(301);
  // and grow more
  set(302);
  // and shrink
  set(250);
  // clear
  str.Clear();
  check(0);
  // and grow again
  set(275);

  str.Destroy();
}

TEST_P(MicroStringPrevTest, SetNullView) {
  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.Set(absl::string_view(), arena());
  EXPECT_EQ(str_.Get(), "");
  EXPECT_EQ(used, arena_space_used());
  EXPECT_GE(self_used, str_.SpaceUsedExcludingSelfLong());

  // Again but with a non-constant size to avoid the CONSTANT_P path.
  size_t zero = time(nullptr) == 0;
  str_.Set(absl::string_view(nullptr, zero), arena());
  EXPECT_EQ(str_.Get(), "");
  EXPECT_EQ(used, arena_space_used());
  EXPECT_GE(self_used, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, Clear) {
  const std::string control(str_.Get());

  const size_t used = arena_space_used();

  str_.Clear();
  EXPECT_EQ(str_.Get(), "");

  EXPECT_EQ(used, arena_space_used());

  str_.Set(control, arena());
  EXPECT_EQ(str_.Get(), control);

  // Resetting to the original string should not use more memory.
  // Except for the aliasing kinds.
  if (prev_state() != kUnowned && prev_state() != kAlias) {
    EXPECT_EQ(used, arena_space_used());
  }
}

TEST(MicroStringTest, ClearOnAliasReusesSpace) {
  Arena arena;
  MicroString str;
  str.SetAlias("Some arbitrary string to alias here.", &arena);
  const size_t available_space = kLargeRepSize - kMicroRepSize;
  const size_t used = arena.SpaceUsed();
  str.Clear();
  EXPECT_EQ(str.Get(), "");
  EXPECT_EQ(kLargeRepSize, str.SpaceUsedExcludingSelfLong());

  std::string input(available_space, 'a');
  // No new space.
  str.Set(input, &arena);
  EXPECT_EQ(used, arena.SpaceUsed());
  EXPECT_EQ(kLargeRepSize, str.SpaceUsedExcludingSelfLong());

  // Now we have to realloc
  str.Set(absl::StrCat(input, "A"), &arena);
  EXPECT_LT(used, arena.SpaceUsed());
  EXPECT_LT(kLargeRepSize, str.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SetInline) {
  const absl::string_view input = kInlineInput;
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
  const absl::string_view input = kInlineInput;

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

  // In 32-bit mode, we will use memory that is not rounded to the arena
  // alignment because sizeof(LargeRep)==12. Avoid using `ExpectMemoryUsed`
  // because it expects it.
  EXPECT_EQ(0, arena_space_used() - used);
  EXPECT_EQ(self_used, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SetAliasLarge) {
  const absl::string_view input("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

  const size_t used = arena_space_used();
  str_.SetAlias(input, arena());
  auto out = str_.Get();
  // Don't use op==, we want to check it points to the exact same buffer.
  EXPECT_EQ(out.data(), input.data());
  EXPECT_EQ(out.size(), input.size());

  // In 32-bit mode, we will use memory that is not rounded to the arena
  // alignment because sizeof(LargeRep)==12. Avoid using `ExpectMemoryUsed`
  // because it expects it.
  EXPECT_EQ(prev_state() == kAlias || !has_arena()
                ? 0
                : ArenaAlignDefault::Ceil(kLargeRepSize),
            arena_space_used() - used);
  EXPECT_EQ(kLargeRepSize, str_.SpaceUsedExcludingSelfLong());
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
  std::string input(1, 'a');
  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  const bool will_reuse = str_.Capacity() >= input.size();
  str_.Set(std::move(input), arena());
  // NOLINTNEXTLINE: We really didn't move from the input, so this is fine.
  EXPECT_EQ(str_.Get(), input);

  // Never uses more space.
  ExpectMemoryUsed(used, false, will_reuse ? self_used : 0);
}

TEST_P(MicroStringPrevTest, SetStringMedium) {
  std::string input(16, 'a');
  const size_t used = arena_space_used();
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.Set(std::move(input), arena());
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
  str_.Set(std::move(copy), arena());
  EXPECT_EQ(str_.Get(), input);

  // Verify that the string was moved.
  EXPECT_EQ(copy_data, str_.Get().data());

  // In 32-bit mode, we will use memory that is not rounded to the arena
  // alignment because sizeof(StringRep) might not be a multiple of 8. Avoid
  // using `ExpectMemoryUsed` because it expects it.
  if (prev_state() != kString && has_arena()) {
    EXPECT_EQ(ArenaAlignDefault::Ceil(kLargeRepSize + sizeof(std::string)),
              arena_space_used() - used);
  }
  EXPECT_EQ(kLargeRepSize + sizeof(std::string) + input.capacity() + /* \0 */ 1,
            str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SelfSetView) {
  const std::string control(str_.Get());

  const size_t used = arena_space_used();
  const bool will_reuse = str_.Capacity() != 0;
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();

  str_.Set(str_.Get(), arena());
  EXPECT_EQ(str_.Get(), control);

  if (will_reuse) {
    ExpectMemoryUsed(used, false, self_used);
  }
}

TEST_P(MicroStringPrevTest, SelfSetSubstrView) {
  const std::string control(str_.Get());
  if (control.empty()) {
    GTEST_SKIP() << "Can't substr an empty input.";
  }

  const size_t used = arena_space_used();
  const bool will_reuse = str_.Capacity() != 0;
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();

  str_.Set(str_.Get().substr(1), arena());
  EXPECT_EQ(str_.Get(), absl::string_view(control).substr(1));

  if (will_reuse) {
    ExpectMemoryUsed(used, false, self_used);
  }
}

TEST_P(MicroStringPrevTest, SelfSetSubstrViewConstantSize) {
  const std::string control(str_.Get());
  if (control.size() < 3) {
    GTEST_SKIP() << "Can't substr an empty input.";
  }

  const size_t used = arena_space_used();
  const bool will_reuse = str_.Capacity() != 0;
  const size_t self_used = str_.SpaceUsedExcludingSelfLong();

  // Here we test the fastpath in SetMaybeConstant.
  // The input is an aliasing substr that overlaps with the destination, but
  // with constant size to trigger the fastpath.
  str_.Set(str_.Get().substr(1, 2), arena());
  EXPECT_EQ(str_.Get(), absl::string_view(control).substr(1, 2));

  if (will_reuse) {
    ExpectMemoryUsed(used, false, self_used);
  }
}

TEST_P(MicroStringPrevTest, InternalSwap) {
  MicroString other = MakeFromState(kOwned, arena());

  const std::string control_lhs(str_.Get());
  const std::string control_rhs(other.Get());

  str_.InternalSwap(&other);
  EXPECT_EQ(str_.Get(), control_rhs);
  EXPECT_EQ(other.Get(), control_lhs);

  if (!has_arena()) other.Destroy();
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

constexpr absl::string_view kPi =
    "3."
    "141592653589793238462643383279502884197169399375105820974944592307816406"
    "286208998628034825342117067982148086513282306647093844609550582231725359"
    "408128481117450284102701938521105559644622948954930381964428810975665933"
    "446128475648233786783165271201909145648566923460348610454326648213393607"
    "260249141273724587006606315588174881520920962829254091715364367892590360"
    "011330530548820466521384146951941511609433057270365759591953092186117381"
    "932611793105118548074462379962749567351885752724891227938183011949129833"
    "673362440656643086021394946395224737190702179860943702770539217176293176"
    "752384674818467669405132000568127145263560827785771342757789609173637178"
    "721468440901224953430146549585371050792279689258923542019956112129021960"
    "864034418159813629774771309960518707211349999998372978049951059731732816"
    "096318595024459455346908302642522308253344685035261931188171010003137838"
    "752886587533208381420617177669147303598253490428755468731159562863882353"
    "787593751957781857780532171226806613001927876611195909216420198";

void SetInChunksTest(size_t size) {
  MicroString str;

  const auto pi = kPi.substr(0, size);
  const size_t chunk_size = std::max(size / 10, size_t{1});
  str.SetInChunks(size, nullptr, [&](auto append) {
    for (auto input = pi; !input.empty();) {
      const auto chunk = input.substr(0, chunk_size);
      input.remove_prefix(chunk.size());
      append(chunk);
    }
  });
  EXPECT_EQ(str.Get(), pi);

  str.Destroy();
}

TEST(MicroStringTest, SetInChunksInline) { SetInChunksTest(5); }
TEST(MicroStringTest, SetInChunksMicro) { SetInChunksTest(50); }
TEST(MicroStringTest, SetInChunksOwned) { SetInChunksTest(500); }

TEST_P(MicroStringPrevTest, SetInChunksWithExistingState) {
  str_.SetInChunks(5, arena(), [](auto append) {
    append("C");
    append("H");
    append("U");
    append("N");
    append("K");
  });
  EXPECT_EQ(str_.Get(), "CHUNK");
}

TEST_P(MicroStringPrevTest, SetInChunksWithExistingStateAfterClear) {
  str_.Clear();
  str_.SetInChunks(3, arena(), [](auto append) { append("BAR"); });
  EXPECT_EQ(str_.Get(), "BAR");
}

TEST_P(MicroStringPrevTest, SetInChunksKeepsSizeValidEvenIfWeDontWriteAll) {
  // Here we say 5 bytes, but only append 4.
  // The final size should still be 4.
  str_.SetInChunks(5, arena(), [](auto append) {
    append("C");
    append("H");
    append("N");
    append("K");
  });
  EXPECT_EQ(str_.Get(), "CHNK");
}

TEST(MicroStringTest, SetInChunksWontPreallocateForVeryLargeFakeSize) {
  MicroString str;
  str.SetInChunks(1'000'000'000, nullptr, [](auto append) {
    append("first");
    append(" and ");
    append("third");
  });
  EXPECT_EQ(str.Get(), "first and third");
  EXPECT_LT(str.Capacity(), 1000);
  EXPECT_LT(str.SpaceUsedExcludingSelfLong(), 1000);
  str.Destroy();
}

TEST(MicroStringTest, SetInChunksAllowsVeryLargeValues) {
  if (sizeof(void*) < 8) {
    GTEST_SKIP() << "Might not be possible to allocate that much memory on "
                    "this platform.";
  }

  std::string total(1'000'000'000, 0);
  // Fill with some "random" data.
  unsigned char x = 17;
  for (char& c : total) {
    c = x;
    x = x * 19 + 7;
  }

  MicroString str;
  str.SetInChunks(total.size(), nullptr, [&](auto append) {
    constexpr size_t kChunks = 1000;
    const size_t kChunkSize = total.size() / kChunks;
    for (size_t i = 0; i < kChunks; ++i) {
      append(absl::string_view(total).substr(kChunkSize * i, kChunkSize));
    }
  });
  EXPECT_EQ(str.Get(), total);
  str.Destroy();
}

TEST(MicroStringTest, DefaultValueInstances) {
  static constexpr absl::string_view kInput =
      "This is the input. It is long enough to not fit in inline space.";
  MicroString str = MicroString::MakeDefaultValuePrototype(kInput);
  EXPECT_EQ(str.Get(), kInput);
  // We actually point to the input string data.
  EXPECT_EQ(static_cast<const void*>(str.Get().data()),
            static_cast<const void*>(kInput.data()));
  EXPECT_EQ(0, str.Capacity());
  EXPECT_EQ(0, str.SpaceUsedExcludingSelfLong());

  MicroString copy(nullptr, str);
  EXPECT_EQ(copy.Get(), kInput);
  // The copy is still pointing to the unowned buffer.
  EXPECT_EQ(static_cast<const void*>(str.Get().data()),
            static_cast<const void*>(copy.Get().data()));
  EXPECT_EQ(0, copy.Capacity());
  EXPECT_EQ(0, copy.SpaceUsedExcludingSelfLong());

  copy.Set("something else", nullptr);
  EXPECT_EQ(copy.Get(), "something else");
  EXPECT_NE(static_cast<const void*>(str.Get().data()),
            static_cast<const void*>(copy.Get().data()));
  EXPECT_NE(0, copy.Capacity());
  EXPECT_NE(0, copy.SpaceUsedExcludingSelfLong());

  // Reset to default.
  copy.ClearToDefault(str, nullptr);
  EXPECT_EQ(copy.Get(), kInput);
  EXPECT_EQ(static_cast<const void*>(str.Get().data()),
            static_cast<const void*>(copy.Get().data()));
  EXPECT_EQ(0, copy.Capacity());
  EXPECT_EQ(0, copy.SpaceUsedExcludingSelfLong());

  str.DestroyDefaultValuePrototype();
}

class MicroStringTestDefaultValueCopy : public testing::Test {
 protected:
  static constexpr absl::string_view kInput = "This is the input.";
  static constexpr absl::string_view kInput2 =
      "Like kInput, but larger so that kInput can fit on it.";

  MicroStringTestDefaultValueCopy()
      : str_(MicroString::MakeDefaultValuePrototype(kInput)) {
    ABSL_CHECK_EQ(str_.Get(), kInput);
  }

  ~MicroStringTestDefaultValueCopy() override {
    str_.DestroyDefaultValuePrototype();
  }

  MicroString str_;
};

TEST_F(MicroStringTestDefaultValueCopy, ClearingReusesIfArena) {
  Arena arena;
  MicroString copy_arena(&arena, str_);
  copy_arena.Set(kInput2, &arena);
  ASSERT_EQ(copy_arena.Get(), kInput2);
  const void* head = copy_arena.Get().data();
  const size_t used = copy_arena.SpaceUsedExcludingSelfLong();
  EXPECT_NE(0, used);

  // Reset to default. We reuse the arena memory to avoid leaking it.
  copy_arena.ClearToDefault(str_, &arena);
  EXPECT_EQ(copy_arena.Get(), kInput);
  EXPECT_EQ(static_cast<const void*>(copy_arena.Get().data()), head);
  EXPECT_EQ(used, copy_arena.SpaceUsedExcludingSelfLong());
}

TEST_F(MicroStringTestDefaultValueCopy, ClearingFreesIfHeap) {
  MicroString copy_heap(nullptr, str_);
  copy_heap.Set(kInput2, nullptr);
  ASSERT_EQ(copy_heap.Get(), kInput2);
  EXPECT_NE(0, copy_heap.SpaceUsedExcludingSelfLong());

  // Reset to default. We are freeing the memory.
  copy_heap.ClearToDefault(str_, nullptr);
  EXPECT_EQ(copy_heap.Get(), kInput);
  EXPECT_EQ(0, copy_heap.SpaceUsedExcludingSelfLong());
}

class MicroStringExtraTest : public testing::Test {
 protected:
  void SetUp() override {
    if (!MicroString::kAllowExtraCapacity) {
      GTEST_SKIP() << "Extra capacity is not allowed.";
    }
  }
};

template <int N>
void TestExtraCapacity(int expected_sizeof) {
  EXPECT_EQ(sizeof(MicroStringExtra<N>), expected_sizeof);
  EXPECT_EQ(MicroStringExtra<N>::kInlineCapacity, expected_sizeof - 1);
}

TEST_F(MicroStringExtraTest, ExtraRequestedInlineSpace) {
  // We write in terms of steps to support 64 and 32 bits.
  static constexpr size_t kStep = alignof(MicroString);
  TestExtraCapacity<0 * kStep + 0>(1 * kStep);
  TestExtraCapacity<0 * kStep + 1>(1 * kStep);
  TestExtraCapacity<1 * kStep - 1>(1 * kStep);
  TestExtraCapacity<1 * kStep + 0>(2 * kStep);
  TestExtraCapacity<2 * kStep - 1>(2 * kStep);
  TestExtraCapacity<2 * kStep + 0>(3 * kStep);
  TestExtraCapacity<3 * kStep - 1>(3 * kStep);
  TestExtraCapacity<3 * kStep + 0>(4 * kStep);
}

TEST_F(MicroStringExtraTest, SettersWithinInline) {
  Arena arena;
  size_t used = arena.SpaceUsed();
  size_t expected_use = ArenaAlignDefault::Ceil(kMicroRepSize + 16);
  MicroStringExtra<15> str15;
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
  MicroStringExtra<23> str23;
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

TEST_F(MicroStringExtraTest, CopyConstructWithinInline) {
  Arena arena;
  const size_t used = arena.SpaceUsed();
  MicroStringExtra<16> inline_str;
  constexpr absl::string_view kStr10 = "1234567890";
  ASSERT_GT(kStr10.size(), MicroString::kInlineCapacity);
  ASSERT_LE(kStr10.size(), MicroStringExtra<16>::kInlineCapacity);
  inline_str.Set(kStr10, &arena);
  EXPECT_EQ(used, arena.SpaceUsed());
  MicroStringExtra<16> copy(nullptr, inline_str);
  EXPECT_EQ(kStr10, copy.Get());
  // Should not have used any extra memory.
  EXPECT_EQ(used, arena.SpaceUsed());
  EXPECT_EQ(0, copy.SpaceUsedExcludingSelfLong());
}

TEST_F(MicroStringExtraTest, SetStringUsesInlineSpace) {
  Arena arena;

  MicroStringExtra<40> str;
  const size_t used = arena.SpaceUsed();
  str.Set(std::string(40, 'x'), &arena);
  // we can fit the chars in the inline space, so copy it.
  EXPECT_EQ(used, arena.SpaceUsed());

  std::string large(100, 'x');
  const size_t used_in_string = StringSpaceUsedExcludingSelfLong(large);
  str.Set(std::move(large), &arena);
  // This one is too big, so we move the whole std::string.
  EXPECT_EQ(ArenaAlignDefault::Ceil(kLargeRepSize + sizeof(std::string)),
            arena.SpaceUsed() - used);
  EXPECT_EQ(kLargeRepSize + sizeof(std::string) + used_in_string,
            str.SpaceUsedExcludingSelfLong());
}

TEST_F(MicroStringExtraTest, InternalSwap) {
  constexpr absl::string_view lhs_value =
      "Very long string that is not SSO and unlikely to use the same capacity "
      "as the other value.";
  constexpr absl::string_view rhs_value = "123456789012345";

  MicroStringExtra<15> lhs, rhs;
  lhs.Set(lhs_value, nullptr);
  rhs.Set(rhs_value, nullptr);

  const size_t used_lhs = lhs.SpaceUsedExcludingSelfLong();
  const size_t used_rhs = rhs.SpaceUsedExcludingSelfLong();

  // Verify setup.
  ASSERT_EQ(lhs.Get(), lhs_value);
  ASSERT_EQ(rhs.Get(), rhs_value);

  lhs.InternalSwap(&rhs);

  EXPECT_EQ(lhs.Get(), rhs_value);
  EXPECT_EQ(rhs.Get(), lhs_value);
  EXPECT_EQ(used_rhs, lhs.SpaceUsedExcludingSelfLong());
  EXPECT_EQ(used_lhs, rhs.SpaceUsedExcludingSelfLong());

  lhs.Destroy();
  rhs.Destroy();
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
    if (sizeof(void*) >= 8) {
      EXPECT_LE(this_micro_str_used, this_arena_str_used);
    } else {
      // Except that in 32-bit platforms we have heap alignment to 4 bytes, but
      // arena alignment is always 8. Take that fact into account by rounding up
      // the ArenaStringPtr use.
      EXPECT_LE(this_micro_str_used,
                ArenaAlignDefault::Ceil(this_arena_str_used));
    }

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
