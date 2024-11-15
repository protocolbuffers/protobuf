// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arena.h"

#include <time.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <new>  // IWYU pragma: keep for operator new
#include <string>
#include <thread>  // NOLINT
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/synchronization/barrier.h"
#include "absl/utility/utility.h"
#include "google/protobuf/arena_cleanup.h"
#include "google/protobuf/arena_test_util.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_arena.pb.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format_lite.h"

#include "absl/synchronization/mutex.h"

// Must be included last
#include "google/protobuf/port_def.inc"

using proto2_arena_unittest::ArenaMessage;
using protobuf_unittest::NestedTestAllTypes;
using protobuf_unittest::TestAllExtensions;
using protobuf_unittest::TestAllTypes;
using protobuf_unittest::TestEmptyMessage;
using protobuf_unittest::TestOneof2;
using protobuf_unittest::TestRepeatedString;
using ::testing::ElementsAreArray;

namespace google {
namespace protobuf {

class Notifier {
 public:
  Notifier() : count_(0) {}
  void Notify() { count_++; }
  int GetCount() { return count_; }

 private:
  int count_;
};

class SimpleDataType {
 public:
  SimpleDataType() : notifier_(nullptr) {}
  void SetNotifier(Notifier* notifier) { notifier_ = notifier; }
  virtual ~SimpleDataType() {
    if (notifier_ != nullptr) {
      notifier_->Notify();
    }
  };

 private:
  Notifier* notifier_;
};

// A simple class that does not allow copying and so cannot be used as a
// parameter type without "const &".
class PleaseDontCopyMe {
 public:
  explicit PleaseDontCopyMe(int value) : value_(value) {}
  PleaseDontCopyMe(const PleaseDontCopyMe&) = delete;
  PleaseDontCopyMe& operator=(const PleaseDontCopyMe&) = delete;

  int value() const { return value_; }

 private:
  int value_;
};

// A class that takes four different types as constructor arguments.
class MustBeConstructedWithOneThroughFour {
 public:
  MustBeConstructedWithOneThroughFour(int one, const char* two,
                                      const std::string& three,
                                      const PleaseDontCopyMe* four)
      : one_(one), two_(two), three_(three), four_(four) {}
  MustBeConstructedWithOneThroughFour(
      const MustBeConstructedWithOneThroughFour&) = delete;
  MustBeConstructedWithOneThroughFour& operator=(
      const MustBeConstructedWithOneThroughFour&) = delete;

  int one_;
  const char* const two_;
  std::string three_;
  const PleaseDontCopyMe* four_;
};

// A class that takes eight different types as constructor arguments.
class MustBeConstructedWithOneThroughEight {
 public:
  MustBeConstructedWithOneThroughEight(int one, const char* two,
                                       const std::string& three,
                                       const PleaseDontCopyMe* four, int five,
                                       const char* six,
                                       const std::string& seven,
                                       const std::string& eight)
      : one_(one),
        two_(two),
        three_(three),
        four_(four),
        five_(five),
        six_(six),
        seven_(seven),
        eight_(eight) {}
  MustBeConstructedWithOneThroughEight(
      const MustBeConstructedWithOneThroughEight&) = delete;
  MustBeConstructedWithOneThroughEight& operator=(
      const MustBeConstructedWithOneThroughEight&) = delete;

  int one_;
  const char* const two_;
  std::string three_;
  const PleaseDontCopyMe* four_;
  int five_;
  const char* const six_;
  std::string seven_;
  std::string eight_;
};

TEST(ArenaTest, ArenaConstructable) {
  EXPECT_TRUE(Arena::is_arena_constructable<TestAllTypes>::type::value);
  EXPECT_TRUE(Arena::is_arena_constructable<const TestAllTypes>::type::value);
  EXPECT_FALSE(Arena::is_arena_constructable<Arena>::type::value);
}

TEST(ArenaTest, DestructorSkippable) {
  EXPECT_TRUE(Arena::is_destructor_skippable<TestAllTypes>::type::value);
  EXPECT_TRUE(Arena::is_destructor_skippable<const TestAllTypes>::type::value);
  EXPECT_FALSE(Arena::is_destructor_skippable<Arena>::type::value);
}

template <int>
struct EmptyBase {};
struct ArenaCtorBase {
  using InternalArenaConstructable_ = void;
};
struct ArenaDtorBase {
  using DestructorSkippable_ = void;
};

template <bool arena_ctor, bool arena_dtor>
void TestCtorAndDtorTraits(std::vector<absl::string_view> def,
                           std::vector<absl::string_view> copy,
                           std::vector<absl::string_view> with_int) {
  static auto& actions = *new std::vector<absl::string_view>;
  struct TraitsProber
      : std::conditional_t<arena_ctor, ArenaCtorBase, EmptyBase<0>>,
        std::conditional_t<arena_dtor, ArenaDtorBase, EmptyBase<1>>,
        Message {
    TraitsProber() : Message(nullptr, nullptr) { actions.push_back("()"); }
    TraitsProber(const TraitsProber&) : Message(nullptr, nullptr) {
      actions.push_back("(const T&)");
    }
    explicit TraitsProber(int) : Message(nullptr, nullptr) {
      actions.push_back("(int)");
    }
    explicit TraitsProber(Arena* arena) : Message(nullptr, nullptr) {
      actions.push_back("(Arena)");
    }
    TraitsProber(Arena* arena, const TraitsProber&)
        : Message(nullptr, nullptr) {
      actions.push_back("(Arena, const T&)");
    }
    TraitsProber(Arena* arena, int) : Message(nullptr, nullptr) {
      actions.push_back("(Arena, int)");
    }
    ~TraitsProber() { actions.push_back("~()"); }

    TraitsProber* New(Arena*) const {
      ABSL_LOG(FATAL);
      return nullptr;
    }
    const internal::ClassData* GetClassData() const PROTOBUF_FINAL {
      ABSL_LOG(FATAL);
      return nullptr;
    }
  };

  static_assert(
      !arena_ctor || Arena::is_arena_constructable<TraitsProber>::value, "");
  static_assert(
      !arena_dtor || Arena::is_destructor_skippable<TraitsProber>::value, "");

  {
    actions.clear();
    Arena arena;
    Arena::Create<TraitsProber>(&arena);
  }
  EXPECT_THAT(actions, ElementsAreArray(def));

  const TraitsProber p;
  {
    actions.clear();
    Arena arena;
    Arena::Create<TraitsProber>(&arena, p);
  }
  EXPECT_THAT(actions, ElementsAreArray(copy));

  {
    actions.clear();
    Arena arena;
    Arena::Create<TraitsProber>(&arena, 17);
  }
  EXPECT_THAT(actions, ElementsAreArray(with_int));
}

TEST(ArenaTest, AllConstructibleAndDestructibleCombinationsWorkCorrectly) {
  TestCtorAndDtorTraits<false, false>({"()", "~()"}, {"(const T&)", "~()"},
                                      {"(int)", "~()"});
  // If the object is not arena constructible, then the destructor is always
  // called even if marked as skippable.
  TestCtorAndDtorTraits<false, true>({"()", "~()"}, {"(const T&)", "~()"},
                                     {"(int)", "~()"});

  // Some types are arena constructible but we can't skip the destructor. Those
  // are constructed with an arena but still destroyed.
  TestCtorAndDtorTraits<true, false>({"(Arena)", "~()"},
                                     {"(Arena, const T&)", "~()"},
                                     {"(Arena, int)", "~()"});
  TestCtorAndDtorTraits<true, true>({"(Arena)"}, {"(Arena, const T&)"},
                                    {"(Arena, int)"});
}

TEST(ArenaTest, BasicCreate) {
  Arena arena;
  EXPECT_TRUE(Arena::Create<int32_t>(&arena) != nullptr);
  EXPECT_TRUE(Arena::Create<int64_t>(&arena) != nullptr);
  EXPECT_TRUE(Arena::Create<float>(&arena) != nullptr);
  EXPECT_TRUE(Arena::Create<double>(&arena) != nullptr);
  EXPECT_TRUE(Arena::Create<std::string>(&arena) != nullptr);
  arena.Own(new int32_t);
  arena.Own(new int64_t);
  arena.Own(new float);
  arena.Own(new double);
  arena.Own(new std::string);
  arena.Own<int>(nullptr);
  Notifier notifier;
  SimpleDataType* data = Arena::Create<SimpleDataType>(&arena);
  data->SetNotifier(&notifier);
  data = new SimpleDataType;
  data->SetNotifier(&notifier);
  arena.Own(data);
  arena.Reset();
  EXPECT_EQ(2, notifier.GetCount());
}

TEST(ArenaTest, CreateAndConstCopy) {
  Arena arena;
  const std::string s("foo");
  const std::string* s_copy = Arena::Create<std::string>(&arena, s);
  EXPECT_TRUE(s_copy != nullptr);
  EXPECT_EQ("foo", s);
  EXPECT_EQ("foo", *s_copy);
}

TEST(ArenaTest, CreateAndNonConstCopy) {
  Arena arena;
  std::string s("foo");
  const std::string* s_copy = Arena::Create<std::string>(&arena, s);
  EXPECT_TRUE(s_copy != nullptr);
  EXPECT_EQ("foo", s);
  EXPECT_EQ("foo", *s_copy);
}

TEST(ArenaTest, CreateAndMove) {
  Arena arena;
  std::string s("foo");
  const std::string* s_move = Arena::Create<std::string>(&arena, std::move(s));
  EXPECT_TRUE(s_move != nullptr);
  EXPECT_TRUE(s.empty());  // NOLINT
  EXPECT_EQ("foo", *s_move);
}

TEST(ArenaTest, CreateWithFourConstructorArguments) {
  Arena arena;
  const std::string three("3");
  const PleaseDontCopyMe four(4);
  const MustBeConstructedWithOneThroughFour* new_object =
      Arena::Create<MustBeConstructedWithOneThroughFour>(&arena, 1, "2", three,
                                                         &four);
  EXPECT_TRUE(new_object != nullptr);
  ASSERT_EQ(1, new_object->one_);
  ASSERT_STREQ("2", new_object->two_);
  ASSERT_EQ("3", new_object->three_);
  ASSERT_EQ(4, new_object->four_->value());
}

TEST(ArenaTest, CreateWithEightConstructorArguments) {
  Arena arena;
  const std::string three("3");
  const PleaseDontCopyMe four(4);
  const std::string seven("7");
  const std::string eight("8");
  const MustBeConstructedWithOneThroughEight* new_object =
      Arena::Create<MustBeConstructedWithOneThroughEight>(
          &arena, 1, "2", three, &four, 5, "6", seven, eight);
  EXPECT_TRUE(new_object != nullptr);
  ASSERT_EQ(1, new_object->one_);
  ASSERT_STREQ("2", new_object->two_);
  ASSERT_EQ("3", new_object->three_);
  ASSERT_EQ(4, new_object->four_->value());
  ASSERT_EQ(5, new_object->five_);
  ASSERT_STREQ("6", new_object->six_);
  ASSERT_EQ("7", new_object->seven_);
  ASSERT_EQ("8", new_object->eight_);
}

class PleaseMoveMe {
 public:
  explicit PleaseMoveMe(const std::string& value) : value_(value) {}
  PleaseMoveMe(PleaseMoveMe&&) = default;
  PleaseMoveMe(const PleaseMoveMe&) = delete;

  const std::string& value() const { return value_; }

 private:
  std::string value_;
};

TEST(ArenaTest, CreateWithMoveArguments) {
  Arena arena;
  PleaseMoveMe one("1");
  const PleaseMoveMe* new_object =
      Arena::Create<PleaseMoveMe>(&arena, std::move(one));
  EXPECT_TRUE(new_object);
  ASSERT_EQ("1", new_object->value());
}

TEST(ArenaTest, InitialBlockTooSmall) {
  // Construct a small blocks of memory to be used by the arena allocator; then,
  // allocate an object which will not fit in the initial block.
  for (uint32_t size = 0; size <= internal::SerialArena::kBlockHeaderSize + 32;
       size++) {
    std::vector<char> arena_block(size);
    ArenaOptions options;
    options.initial_block = arena_block.data();
    options.initial_block_size = arena_block.size();

    // Try sometimes with non-default block sizes so that we exercise paths
    // with and without ArenaImpl::Options.
    if ((size % 2) != 0) {
      options.start_block_size += 8;
    }

    Arena arena(options);

    char* p = Arena::CreateArray<char>(&arena, 96);
    uintptr_t allocation = reinterpret_cast<uintptr_t>(p);

    // Ensure that the arena allocator did not return memory pointing into the
    // initial block of memory.
    uintptr_t arena_start = reinterpret_cast<uintptr_t>(arena_block.data());
    uintptr_t arena_end = arena_start + arena_block.size();
    EXPECT_FALSE(allocation >= arena_start && allocation < arena_end);

    // Write to the memory we allocated; this should (but is not guaranteed to)
    // trigger a check for heap corruption if the object was allocated from the
    // initially-provided block.
    memset(p, '\0', 96);
  }
}

TEST(ArenaTest, CreateDestroy) {
  TestAllTypes original;
  TestUtil::SetAllFields(&original);

  // Test memory leak.
  Arena arena;
  TestAllTypes* heap_message = Arena::Create<TestAllTypes>(nullptr);
  TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);

  *heap_message = original;
  *arena_message = original;

  Arena::Destroy(heap_message);
  Arena::Destroy(arena_message);

  // The arena message should still exist.
  EXPECT_EQ(strlen(original.optional_string().c_str()),
            strlen(arena_message->optional_string().c_str()));
}

TEST(ArenaTest, MoveCtorOnArena) {
  Arena arena;

  ASSERT_EQ(arena.SpaceUsed(), 0);

  auto* original = Arena::Create<NestedTestAllTypes>(&arena);
  TestUtil::SetAllFields(original->mutable_payload());
  TestUtil::ExpectAllFieldsSet(original->payload());

  auto usage_original = arena.SpaceUsed();
  auto* moved = Arena::Create<NestedTestAllTypes>(&arena, std::move(*original));
  auto usage_by_move = arena.SpaceUsed() - usage_original;

  TestUtil::ExpectAllFieldsSet(moved->payload());

  // The only extra allocation with moves is sizeof(NestedTestAllTypes).
  EXPECT_EQ(usage_by_move, sizeof(NestedTestAllTypes));
  EXPECT_LT(usage_by_move + sizeof(TestAllTypes), usage_original);

  // Status after move is unspecified and must not be assumed. It's merely
  // checking current implementation specifics for protobuf internal.
  TestUtil::ExpectClear(original->payload());
}

TEST(ArenaTest, RepeatedFieldMoveCtorOnArena) {
  Arena arena;

  auto* original = Arena::Create<RepeatedField<int32_t>>(&arena);
  original->Add(1);
  original->Add(2);
  ASSERT_EQ(original->size(), 2);
  ASSERT_EQ(original->Get(0), 1);
  ASSERT_EQ(original->Get(1), 2);

  auto* moved =
      Arena::Create<RepeatedField<int32_t>>(&arena, std::move(*original));

  EXPECT_EQ(moved->size(), 2);
  EXPECT_EQ(moved->Get(0), 1);
  EXPECT_EQ(moved->Get(1), 2);

  // Status after move is unspecified and must not be assumed. It's merely
  // checking current implementation specifics for protobuf internal.
  EXPECT_EQ(original->size(), 0);
}

TEST(ArenaTest, RepeatedPtrFieldMoveCtorOnArena) {
  Arena arena;

  ASSERT_EQ(arena.SpaceUsed(), 0);

  auto* original = Arena::Create<RepeatedPtrField<TestAllTypes>>(&arena);
  auto* msg = original->Add();
  TestUtil::SetAllFields(msg);
  TestUtil::ExpectAllFieldsSet(*msg);

  auto usage_original = arena.SpaceUsed();
  auto* moved = Arena::Create<RepeatedPtrField<TestAllTypes>>(
      &arena, std::move(*original));
  auto usage_by_move = arena.SpaceUsed() - usage_original;

  EXPECT_EQ(moved->size(), 1);
  TestUtil::ExpectAllFieldsSet(moved->Get(0));

  // The only extra allocation with moves is sizeof(RepeatedPtrField).
  EXPECT_EQ(usage_by_move, sizeof(internal::RepeatedPtrFieldBase));
  EXPECT_LT(usage_by_move + sizeof(TestAllTypes), usage_original);

  // Status after move is unspecified and must not be assumed. It's merely
  // checking current implementation specifics for protobuf internal.
  EXPECT_EQ(original->size(), 0);
}

struct OnlyArenaConstructible {
  using InternalArenaConstructable_ = void;
  explicit OnlyArenaConstructible(Arena* arena) {}
};

TEST(ArenaTest, ArenaOnlyTypesCanBeConstructed) {
  Arena arena;
  Arena::Create<OnlyArenaConstructible>(&arena);
}

TEST(ArenaTest, GetConstructTypeWorks) {
  using T = TestAllTypes;
  using Peer = internal::ArenaTestPeer;
  using CT = typename Peer::ConstructType;
  EXPECT_EQ(CT::kDefault, (Peer::GetConstructType<T>()));
  EXPECT_EQ(CT::kCopy, (Peer::GetConstructType<T, const T&>()));
  EXPECT_EQ(CT::kCopy, (Peer::GetConstructType<T, T&>()));
  EXPECT_EQ(CT::kCopy, (Peer::GetConstructType<T, const T&&>()));
  EXPECT_EQ(CT::kMove, (Peer::GetConstructType<T, T&&>()));
  EXPECT_EQ(CT::kUnknown, (Peer::GetConstructType<T, double&>()));
  EXPECT_EQ(CT::kUnknown, (Peer::GetConstructType<T, T&, T&>()));

  // For non-protos, it's always unknown
  EXPECT_EQ(CT::kUnknown, (Peer::GetConstructType<int, const int&>()));
}

#ifdef __cpp_if_constexpr
class DispatcherTestProto : public Message {
 public:
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  // For the test below to construct.
  explicit DispatcherTestProto(absl::in_place_t) : Message(nullptr, nullptr) {}
  explicit DispatcherTestProto(Arena*) : Message(nullptr, nullptr) {
    ABSL_LOG(FATAL);
  }
  DispatcherTestProto(Arena*, const DispatcherTestProto&)
      : Message(nullptr, nullptr) {
    ABSL_LOG(FATAL);
  }
  const internal::ClassData* GetClassData() const PROTOBUF_FINAL {
    ABSL_LOG(FATAL);
  }
};
// We use a specialization to inject behavior for the test.
// This test is very intrusive and will have to be fixed if we change the
// implementation of CreateMessage.
absl::string_view hook_called;
template <>
void* Arena::DefaultConstruct<DispatcherTestProto>(Arena*) {
  hook_called = "default";
  return nullptr;
}
template <>
void* Arena::CopyConstruct<DispatcherTestProto>(Arena*, const void*) {
  hook_called = "copy";
  return nullptr;
}
template <>
DispatcherTestProto* Arena::CreateArenaCompatible<DispatcherTestProto, int>(
    Arena*, int&&) {
  hook_called = "fallback";
  return nullptr;
}

TEST(ArenaTest, CreateArenaConstructable) {
  TestAllTypes original;
  TestUtil::SetAllFields(&original);

  Arena arena;
  auto copied = Arena::Create<TestAllTypes>(&arena, original);

  TestUtil::ExpectAllFieldsSet(*copied);
  EXPECT_EQ(copied->GetArena(), &arena);
  EXPECT_EQ(copied->optional_nested_message().GetArena(), &arena);
}

TEST(ArenaTest, CreateRepeatedPtrField) {
  Arena arena;
  auto repeated = Arena::Create<RepeatedPtrField<TestAllTypes>>(&arena);
  TestUtil::SetAllFields(repeated->Add());

  TestUtil::ExpectAllFieldsSet(repeated->Get(0));
  EXPECT_EQ(repeated->GetArena(), &arena);
  EXPECT_EQ(repeated->Get(0).GetArena(), &arena);
  EXPECT_EQ(repeated->Get(0).optional_nested_message().GetArena(), &arena);
}

TEST(ArenaTest, CreateMessageDispatchesToSpecialFunctions) {
  hook_called = "";
  Arena::Create<DispatcherTestProto>(nullptr);
  EXPECT_EQ(hook_called, "default");

  DispatcherTestProto ref(absl::in_place);
  const DispatcherTestProto& cref = ref;

  hook_called = "";
  Arena::Create<DispatcherTestProto>(nullptr);
  EXPECT_EQ(hook_called, "default");

  hook_called = "";
  Arena::Create<DispatcherTestProto>(nullptr, ref);
  EXPECT_EQ(hook_called, "copy");

  hook_called = "";
  Arena::Create<DispatcherTestProto>(nullptr, cref);
  EXPECT_EQ(hook_called, "copy");

  hook_called = "";
  Arena::Create<DispatcherTestProto>(nullptr, 1);
  EXPECT_EQ(hook_called, "fallback");
}
#endif  // __cpp_if_constexpr

TEST(ArenaTest, Parsing) {
  TestAllTypes original;
  TestUtil::SetAllFields(&original);

  // Test memory leak.
  Arena arena;
  TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
  arena_message->ParseFromString(original.SerializeAsString());
  TestUtil::ExpectAllFieldsSet(*arena_message);

  // Test that string fields have nul terminator bytes (earlier bug).
  EXPECT_EQ(strlen(original.optional_string().c_str()),
            strlen(arena_message->optional_string().c_str()));
}

TEST(ArenaTest, UnknownFields) {
  TestAllTypes original;
  TestUtil::SetAllFields(&original);

  // Test basic parsing into (populating) and reading out of unknown fields on
  // an arena.
  Arena arena;
  TestEmptyMessage* arena_message = Arena::Create<TestEmptyMessage>(&arena);
  arena_message->ParseFromString(original.SerializeAsString());

  TestAllTypes copied;
  copied.ParseFromString(arena_message->SerializeAsString());
  TestUtil::ExpectAllFieldsSet(copied);

  // Exercise UFS manual manipulation (setters).
  arena_message = Arena::Create<TestEmptyMessage>(&arena);
  arena_message->mutable_unknown_fields()->AddVarint(
      TestAllTypes::kOptionalInt32FieldNumber, 42);
  copied.Clear();
  copied.ParseFromString(arena_message->SerializeAsString());
  EXPECT_TRUE(copied.has_optional_int32());
  EXPECT_EQ(42, copied.optional_int32());

  // Exercise UFS swap path.
  TestEmptyMessage* arena_message_2 = Arena::Create<TestEmptyMessage>(&arena);
  arena_message_2->Swap(arena_message);
  copied.Clear();
  copied.ParseFromString(arena_message_2->SerializeAsString());
  EXPECT_TRUE(copied.has_optional_int32());
  EXPECT_EQ(42, copied.optional_int32());

  // Test field manipulation.
  TestEmptyMessage* arena_message_3 = Arena::Create<TestEmptyMessage>(&arena);
  arena_message_3->mutable_unknown_fields()->AddVarint(1000, 42);
  arena_message_3->mutable_unknown_fields()->AddFixed32(1001, 42);
  arena_message_3->mutable_unknown_fields()->AddFixed64(1002, 42);
  arena_message_3->mutable_unknown_fields()->AddLengthDelimited(1003, "");
  arena_message_3->mutable_unknown_fields()->DeleteSubrange(0, 2);
  arena_message_3->mutable_unknown_fields()->DeleteByNumber(1002);
  arena_message_3->mutable_unknown_fields()->DeleteByNumber(1003);
  EXPECT_TRUE(arena_message_3->unknown_fields().empty());
}

TEST(ArenaTest, Swap) {
  Arena arena1;
  Arena arena2;
  TestAllTypes* arena1_message;
  TestAllTypes* arena2_message;

  // Case 1: Swap(), no UFS on either message, both messages on different
  // arenas. Arena pointers should remain the same after swap.
  arena1_message = Arena::Create<TestAllTypes>(&arena1);
  arena2_message = Arena::Create<TestAllTypes>(&arena2);
  arena1_message->Swap(arena2_message);
  EXPECT_EQ(&arena1, arena1_message->GetArena());
  EXPECT_EQ(&arena2, arena2_message->GetArena());

  // Case 2: Swap(), UFS on one message, both messages on different arenas.
  arena1_message = Arena::Create<TestAllTypes>(&arena1);
  arena2_message = Arena::Create<TestAllTypes>(&arena2);
  arena1_message->mutable_unknown_fields()->AddVarint(1, 42);
  arena1_message->Swap(arena2_message);
  EXPECT_EQ(&arena1, arena1_message->GetArena());
  EXPECT_EQ(&arena2, arena2_message->GetArena());
  EXPECT_EQ(0, arena1_message->unknown_fields().field_count());
  EXPECT_EQ(1, arena2_message->unknown_fields().field_count());
  EXPECT_EQ(42, arena2_message->unknown_fields().field(0).varint());

  // Case 3: Swap(), UFS on both messages, both messages on different arenas.
  arena1_message = Arena::Create<TestAllTypes>(&arena1);
  arena2_message = Arena::Create<TestAllTypes>(&arena2);
  arena1_message->mutable_unknown_fields()->AddVarint(1, 42);
  arena2_message->mutable_unknown_fields()->AddVarint(2, 84);
  arena1_message->Swap(arena2_message);
  EXPECT_EQ(&arena1, arena1_message->GetArena());
  EXPECT_EQ(&arena2, arena2_message->GetArena());
  EXPECT_EQ(1, arena1_message->unknown_fields().field_count());
  EXPECT_EQ(1, arena2_message->unknown_fields().field_count());
  EXPECT_EQ(84, arena1_message->unknown_fields().field(0).varint());
  EXPECT_EQ(42, arena2_message->unknown_fields().field(0).varint());
}

TEST(ArenaTest, ReflectionSwapFields) {
  Arena arena1;
  Arena arena2;
  TestAllTypes* arena1_message;
  TestAllTypes* arena2_message;

  // Case 1: messages on different arenas, only one message is set.
  arena1_message = Arena::Create<TestAllTypes>(&arena1);
  arena2_message = Arena::Create<TestAllTypes>(&arena2);
  TestUtil::SetAllFields(arena1_message);
  const Reflection* reflection = arena1_message->GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(*arena1_message, &fields);
  reflection->SwapFields(arena1_message, arena2_message, fields);
  EXPECT_EQ(&arena1, arena1_message->GetArena());
  EXPECT_EQ(&arena2, arena2_message->GetArena());
  std::string output;
  arena1_message->SerializeToString(&output);
  EXPECT_EQ(0, output.size());
  TestUtil::ExpectAllFieldsSet(*arena2_message);
  reflection->SwapFields(arena1_message, arena2_message, fields);
  arena2_message->SerializeToString(&output);
  EXPECT_EQ(0, output.size());
  TestUtil::ExpectAllFieldsSet(*arena1_message);

  // Case 2: messages on different arenas, both messages are set.
  arena1_message = Arena::Create<TestAllTypes>(&arena1);
  arena2_message = Arena::Create<TestAllTypes>(&arena2);
  TestUtil::SetAllFields(arena1_message);
  TestUtil::SetAllFields(arena2_message);
  reflection->SwapFields(arena1_message, arena2_message, fields);
  EXPECT_EQ(&arena1, arena1_message->GetArena());
  EXPECT_EQ(&arena2, arena2_message->GetArena());
  TestUtil::ExpectAllFieldsSet(*arena1_message);
  TestUtil::ExpectAllFieldsSet(*arena2_message);

  // Case 3: messages on different arenas with different lifetimes.
  arena1_message = Arena::Create<TestAllTypes>(&arena1);
  {
    Arena arena3;
    TestAllTypes* arena3_message = Arena::Create<TestAllTypes>(&arena3);
    TestUtil::SetAllFields(arena3_message);
    reflection->SwapFields(arena1_message, arena3_message, fields);
  }
  TestUtil::ExpectAllFieldsSet(*arena1_message);

  // Case 4: one message on arena, the other on heap.
  arena1_message = Arena::Create<TestAllTypes>(&arena1);
  TestAllTypes message;
  TestUtil::SetAllFields(arena1_message);
  reflection->SwapFields(arena1_message, &message, fields);
  EXPECT_EQ(&arena1, arena1_message->GetArena());
  EXPECT_EQ(nullptr, message.GetArena());
  arena1_message->SerializeToString(&output);
  EXPECT_EQ(0, output.size());
  TestUtil::ExpectAllFieldsSet(message);
}

TEST(ArenaTest, SetAllocatedMessage) {
  Arena arena;
  TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
  TestAllTypes::NestedMessage* nested = new TestAllTypes::NestedMessage;
  nested->set_bb(118);
  arena_message->set_allocated_optional_nested_message(nested);
  EXPECT_EQ(118, arena_message->optional_nested_message().bb());
}

TEST(ArenaTest, ReleaseMessage) {
  Arena arena;
  TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
  arena_message->mutable_optional_nested_message()->set_bb(118);
  std::unique_ptr<TestAllTypes::NestedMessage> nested(
      arena_message->release_optional_nested_message());
  EXPECT_EQ(118, nested->bb());

  TestAllTypes::NestedMessage* released_null =
      arena_message->release_optional_nested_message();
  EXPECT_EQ(nullptr, released_null);
}

TEST(ArenaTest, SetAllocatedString) {
  Arena arena;
  TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
  std::string* allocated_str = new std::string("hello");
  arena_message->set_allocated_optional_string(allocated_str);
  EXPECT_EQ("hello", arena_message->optional_string());
}

TEST(ArenaTest, ReleaseString) {
  Arena arena;
  TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
  arena_message->set_optional_string("hello");
  std::unique_ptr<std::string> released_str(
      arena_message->release_optional_string());
  EXPECT_EQ("hello", *released_str);

  // Test default value.
}


TEST(ArenaTest, SwapBetweenArenasWithAllFieldsSet) {
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  {
    Arena arena2;
    TestAllTypes* arena2_message = Arena::Create<TestAllTypes>(&arena2);
    TestUtil::SetAllFields(arena2_message);
    arena2_message->Swap(arena1_message);
    std::string output;
    arena2_message->SerializeToString(&output);
    EXPECT_EQ(0, output.size());
  }
  TestUtil::ExpectAllFieldsSet(*arena1_message);
}

TEST(ArenaTest, SwapBetweenArenaAndNonArenaWithAllFieldsSet) {
  TestAllTypes non_arena_message;
  TestUtil::SetAllFields(&non_arena_message);
  {
    Arena arena2;
    TestAllTypes* arena2_message = Arena::Create<TestAllTypes>(&arena2);
    TestUtil::SetAllFields(arena2_message);
    arena2_message->Swap(&non_arena_message);
    TestUtil::ExpectAllFieldsSet(*arena2_message);
    TestUtil::ExpectAllFieldsSet(non_arena_message);
  }
}

TEST(ArenaTest, UnsafeArenaSwap) {
  Arena shared_arena;
  TestAllTypes* message1 = Arena::Create<TestAllTypes>(&shared_arena);
  TestAllTypes* message2 = Arena::Create<TestAllTypes>(&shared_arena);
  TestUtil::SetAllFields(message1);
  message1->UnsafeArenaSwap(message2);
  TestUtil::ExpectAllFieldsSet(*message2);
}

TEST(ArenaTest, SwapBetweenArenasUsingReflection) {
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  {
    Arena arena2;
    TestAllTypes* arena2_message = Arena::Create<TestAllTypes>(&arena2);
    TestUtil::SetAllFields(arena2_message);
    const Reflection* r = arena2_message->GetReflection();
    r->Swap(arena1_message, arena2_message);
    std::string output;
    arena2_message->SerializeToString(&output);
    EXPECT_EQ(0, output.size());
  }
  TestUtil::ExpectAllFieldsSet(*arena1_message);
}

TEST(ArenaTest, SwapBetweenArenaAndNonArenaUsingReflection) {
  TestAllTypes non_arena_message;
  TestUtil::SetAllFields(&non_arena_message);
  {
    Arena arena2;
    TestAllTypes* arena2_message = Arena::Create<TestAllTypes>(&arena2);
    TestUtil::SetAllFields(arena2_message);
    const Reflection* r = arena2_message->GetReflection();
    r->Swap(&non_arena_message, arena2_message);
    TestUtil::ExpectAllFieldsSet(*arena2_message);
    TestUtil::ExpectAllFieldsSet(non_arena_message);
  }
}

TEST(ArenaTest, ReleaseFromArenaMessageMakesCopy) {
  TestAllTypes::NestedMessage* nested_msg = nullptr;
  std::string* nested_string = nullptr;
  {
    Arena arena;
    TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
    arena_message->mutable_optional_nested_message()->set_bb(42);
    *arena_message->mutable_optional_string() = "Hello";
    nested_msg = arena_message->release_optional_nested_message();
    nested_string = arena_message->release_optional_string();
  }
  EXPECT_EQ(42, nested_msg->bb());
  EXPECT_EQ("Hello", *nested_string);
  delete nested_msg;
  delete nested_string;
}

#if PROTOBUF_RTTI
TEST(ArenaTest, ReleaseFromArenaMessageUsingReflectionMakesCopy) {
  TestAllTypes::NestedMessage* nested_msg = nullptr;
  // Note: no string: reflection API only supports releasing submessages.
  {
    Arena arena;
    TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
    arena_message->mutable_optional_nested_message()->set_bb(42);
    const Reflection* r = arena_message->GetReflection();
    const FieldDescriptor* f = arena_message->GetDescriptor()->FindFieldByName(
        "optional_nested_message");
    nested_msg = DownCastMessage<TestAllTypes::NestedMessage>(
        r->ReleaseMessage(arena_message, f));
  }
  EXPECT_EQ(42, nested_msg->bb());
  delete nested_msg;
}
#endif  // PROTOBUF_RTTI

TEST(ArenaTest, SetAllocatedAcrossArenas) {
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  TestAllTypes::NestedMessage* heap_submessage =
      new TestAllTypes::NestedMessage();
  heap_submessage->set_bb(42);
  arena1_message->set_allocated_optional_nested_message(heap_submessage);
  // Should keep same object and add to arena's Own()-list.
  EXPECT_EQ(heap_submessage, arena1_message->mutable_optional_nested_message());
  {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
#if GTEST_HAS_DEATH_TEST
    EXPECT_DEBUG_DEATH(arena1_message->set_allocated_optional_nested_message(
                           arena2_submessage),
                       "submessage_arena");
#endif
    EXPECT_NE(arena2_submessage,
              arena1_message->mutable_optional_nested_message());
  }

  TestAllTypes::NestedMessage* arena1_submessage =
      Arena::Create<TestAllTypes::NestedMessage>(&arena1);
  arena1_submessage->set_bb(42);
  TestAllTypes* heap_message = new TestAllTypes;
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(
      heap_message->set_allocated_optional_nested_message(arena1_submessage),
      "submessage_arena");
#endif
  EXPECT_NE(arena1_submessage, heap_message->mutable_optional_nested_message());
  delete heap_message;
}

TEST(ArenaTest, UnsafeArenaSetAllocatedAcrossArenas) {
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
    arena1_message->unsafe_arena_set_allocated_optional_nested_message(
        arena2_submessage);
    EXPECT_EQ(arena2_submessage,
              arena1_message->mutable_optional_nested_message());
    EXPECT_EQ(arena2_submessage,
              arena1_message->unsafe_arena_release_optional_nested_message());
  }

  TestAllTypes::NestedMessage* arena1_submessage =
      Arena::Create<TestAllTypes::NestedMessage>(&arena1);
  arena1_submessage->set_bb(42);
  TestAllTypes* heap_message = new TestAllTypes;
  heap_message->unsafe_arena_set_allocated_optional_nested_message(
      arena1_submessage);
  EXPECT_EQ(arena1_submessage, heap_message->mutable_optional_nested_message());
  EXPECT_EQ(arena1_submessage,
            heap_message->unsafe_arena_release_optional_nested_message());
  delete heap_message;
}

TEST(ArenaTest, SetAllocatedAcrossArenasWithReflection) {
  // Same as above, with reflection.
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  const Reflection* r = arena1_message->GetReflection();
  const Descriptor* d = arena1_message->GetDescriptor();
  const FieldDescriptor* msg_field =
      d->FindFieldByName("optional_nested_message");
  TestAllTypes::NestedMessage* heap_submessage =
      new TestAllTypes::NestedMessage();
  heap_submessage->set_bb(42);
  r->SetAllocatedMessage(arena1_message, heap_submessage, msg_field);
  // Should keep same object and add to arena's Own()-list.
  EXPECT_EQ(heap_submessage, arena1_message->mutable_optional_nested_message());
  {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
#if GTEST_HAS_DEATH_TEST
    EXPECT_DEBUG_DEATH(
        r->SetAllocatedMessage(arena1_message, arena2_submessage, msg_field),
        "GetArena");
#endif
    EXPECT_NE(arena2_submessage,
              arena1_message->mutable_optional_nested_message());
  }

  TestAllTypes::NestedMessage* arena1_submessage =
      Arena::Create<TestAllTypes::NestedMessage>(&arena1);
  arena1_submessage->set_bb(42);
  TestAllTypes* heap_message = new TestAllTypes;
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(
      r->SetAllocatedMessage(heap_message, arena1_submessage, msg_field),
      "GetArena");
#endif
  EXPECT_NE(arena1_submessage, heap_message->mutable_optional_nested_message());
  delete heap_message;
}

TEST(ArenaTest, UnsafeArenaSetAllocatedAcrossArenasWithReflection) {
  // Same as above, with reflection.
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  const Reflection* r = arena1_message->GetReflection();
  const Descriptor* d = arena1_message->GetDescriptor();
  const FieldDescriptor* msg_field =
      d->FindFieldByName("optional_nested_message");
  {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
    r->UnsafeArenaSetAllocatedMessage(arena1_message, arena2_submessage,
                                      msg_field);
    EXPECT_EQ(arena2_submessage,
              arena1_message->mutable_optional_nested_message());
    EXPECT_EQ(arena2_submessage,
              arena1_message->unsafe_arena_release_optional_nested_message());
  }

  TestAllTypes::NestedMessage* arena1_submessage =
      Arena::Create<TestAllTypes::NestedMessage>(&arena1);
  arena1_submessage->set_bb(42);
  TestAllTypes* heap_message = new TestAllTypes;
  r->UnsafeArenaSetAllocatedMessage(heap_message, arena1_submessage, msg_field);
  EXPECT_EQ(arena1_submessage, heap_message->mutable_optional_nested_message());
  EXPECT_EQ(arena1_submessage,
            heap_message->unsafe_arena_release_optional_nested_message());
  delete heap_message;
}

TEST(ArenaTest, AddAllocatedWithReflection) {
  Arena arena1;
  ArenaMessage* arena1_message = Arena::Create<ArenaMessage>(&arena1);
  const Reflection* r = arena1_message->GetReflection();
  const Descriptor* d = arena1_message->GetDescriptor();
  // Message with cc_enable_arenas = true;
  const FieldDescriptor* fd = d->FindFieldByName("repeated_nested_message");
  r->AddMessage(arena1_message, fd);
  r->AddMessage(arena1_message, fd);
  r->AddMessage(arena1_message, fd);
  EXPECT_EQ(3, r->FieldSize(*arena1_message, fd));
}

TEST(ArenaTest, RepeatedPtrFieldAddClearedTest) {
  {
    RepeatedPtrField<TestAllTypes> repeated_field;
    EXPECT_TRUE(repeated_field.empty());
    EXPECT_EQ(0, repeated_field.size());
    // Ownership is passed to repeated_field.
    TestAllTypes* cleared = new TestAllTypes();
    repeated_field.AddAllocated(cleared);
    EXPECT_FALSE(repeated_field.empty());
    EXPECT_EQ(1, repeated_field.size());
  }
}

TEST(ArenaTest, AddAllocatedToRepeatedField) {
  // Heap->arena case.
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  for (int i = 0; i < 10; i++) {
    TestAllTypes::NestedMessage* heap_submessage =
        new TestAllTypes::NestedMessage();
    heap_submessage->set_bb(42);
    arena1_message->mutable_repeated_nested_message()->AddAllocated(
        heap_submessage);
    // Should not copy object -- will use arena_->Own().
    EXPECT_EQ(heap_submessage, &arena1_message->repeated_nested_message(i));
    EXPECT_EQ(42, arena1_message->repeated_nested_message(i).bb());
  }

  // Arena1->Arena2 case.
  arena1_message->Clear();
  for (int i = 0; i < 10; i++) {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
    arena1_message->mutable_repeated_nested_message()->AddAllocated(
        arena2_submessage);
    ASSERT_THAT(arena1_message->repeated_nested_message(), testing::SizeIs(1));
    EXPECT_EQ(
        arena1_message->mutable_repeated_nested_message()->at(0).GetArena(),
        &arena1);
    arena1_message->clear_repeated_nested_message();
  }

  // Arena->heap case.
  TestAllTypes* heap_message = new TestAllTypes;
  for (int i = 0; i < 10; i++) {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
    heap_message->mutable_repeated_nested_message()->AddAllocated(
        arena2_submessage);
    ASSERT_THAT(heap_message->repeated_nested_message(), testing::SizeIs(1));
    EXPECT_EQ(heap_message->mutable_repeated_nested_message()->at(0).GetArena(),
              nullptr);
    heap_message->clear_repeated_nested_message();
  }
  delete heap_message;

  // Heap->arena case for strings (which are not arena-allocated).
  arena1_message->Clear();
  for (int i = 0; i < 10; i++) {
    std::string* s = new std::string("Test");
    arena1_message->mutable_repeated_string()->AddAllocated(s);
    // Should not copy.
    EXPECT_EQ(s, &arena1_message->repeated_string(i));
    EXPECT_EQ("Test", arena1_message->repeated_string(i));
  }
}

TEST(ArenaTest, UnsafeArenaAddAllocatedToRepeatedField) {
  // Heap->arena case.
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  {
    auto* heap_submessage = new TestAllTypes::NestedMessage;
    arena1_message->mutable_repeated_nested_message()->UnsafeArenaAddAllocated(
        heap_submessage);
    // Should not copy object.
    EXPECT_EQ(heap_submessage, &arena1_message->repeated_nested_message(0));
    EXPECT_EQ(heap_submessage, arena1_message->mutable_repeated_nested_message()
                                   ->UnsafeArenaReleaseLast());
    delete heap_submessage;
  }

  // Arena1->Arena2 case.
  arena1_message->Clear();
  {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
    arena1_message->mutable_repeated_nested_message()->UnsafeArenaAddAllocated(
        arena2_submessage);
    // Should own object.
    EXPECT_EQ(arena2_submessage, &arena1_message->repeated_nested_message(0));
    EXPECT_EQ(arena2_submessage,
              arena1_message->mutable_repeated_nested_message()
                  ->UnsafeArenaReleaseLast());
  }

  // Arena->heap case.
  TestAllTypes* heap_message = new TestAllTypes;
  {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
    heap_message->mutable_repeated_nested_message()->UnsafeArenaAddAllocated(
        arena2_submessage);
    // Should own object.
    EXPECT_EQ(arena2_submessage, &heap_message->repeated_nested_message(0));
    EXPECT_EQ(arena2_submessage, heap_message->mutable_repeated_nested_message()
                                     ->UnsafeArenaReleaseLast());
  }
  delete heap_message;

  // Heap->arena case for strings (which are not arena-allocated).
  arena1_message->Clear();
  {
    std::string* s = new std::string("Test");
    arena1_message->mutable_repeated_string()->UnsafeArenaAddAllocated(s);
    // Should not copy.
    EXPECT_EQ(s, &arena1_message->repeated_string(0));
    EXPECT_EQ("Test", arena1_message->repeated_string(0));
    delete arena1_message->mutable_repeated_string()->UnsafeArenaReleaseLast();
  }
}

TEST(ArenaTest, AddAllocatedToRepeatedFieldViaReflection) {
  // Heap->arena case.
  Arena arena1;
  TestAllTypes* arena1_message = Arena::Create<TestAllTypes>(&arena1);
  const Reflection* r = arena1_message->GetReflection();
  const Descriptor* d = arena1_message->GetDescriptor();
  const FieldDescriptor* fd = d->FindFieldByName("repeated_nested_message");
  for (int i = 0; i < 10; i++) {
    TestAllTypes::NestedMessage* heap_submessage =
        new TestAllTypes::NestedMessage;
    heap_submessage->set_bb(42);
    r->AddAllocatedMessage(arena1_message, fd, heap_submessage);
    // Should not copy object -- will use arena_->Own().
    EXPECT_EQ(heap_submessage, &arena1_message->repeated_nested_message(i));
    EXPECT_EQ(42, arena1_message->repeated_nested_message(i).bb());
  }

  // Arena1->Arena2 case.
  arena1_message->Clear();
  for (int i = 0; i < 10; i++) {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
    r->AddAllocatedMessage(arena1_message, fd, arena2_submessage);
    ASSERT_THAT(arena1_message->repeated_nested_message(), testing::SizeIs(1));
    EXPECT_EQ(
        arena1_message->mutable_repeated_nested_message()->at(0).GetArena(),
        &arena1);
    arena1_message->clear_repeated_nested_message();
  }

  // Arena->heap case.
  TestAllTypes* heap_message = new TestAllTypes;
  for (int i = 0; i < 10; i++) {
    Arena arena2;
    TestAllTypes::NestedMessage* arena2_submessage =
        Arena::Create<TestAllTypes::NestedMessage>(&arena2);
    arena2_submessage->set_bb(42);
    r->AddAllocatedMessage(heap_message, fd, arena2_submessage);
    ASSERT_THAT(heap_message->repeated_nested_message(), testing::SizeIs(1));
    EXPECT_EQ(heap_message->mutable_repeated_nested_message()->at(0).GetArena(),
              nullptr);
    heap_message->clear_repeated_nested_message();
  }
  delete heap_message;
}

TEST(ArenaTest, ReleaseLastRepeatedField) {
  // Release from arena-allocated repeated field and ensure that returned object
  // is heap-allocated.
  Arena arena;
  TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
  for (int i = 0; i < 10; i++) {
    TestAllTypes::NestedMessage* nested =
        Arena::Create<TestAllTypes::NestedMessage>(&arena);
    nested->set_bb(42);
    arena_message->mutable_repeated_nested_message()->AddAllocated(nested);
  }

  for (int i = 0; i < 10; i++) {
    const TestAllTypes::NestedMessage* orig_submessage =
        &arena_message->repeated_nested_message(10 - 1 - i);  // last element
    TestAllTypes::NestedMessage* released =
        arena_message->mutable_repeated_nested_message()->ReleaseLast();
    EXPECT_NE(released, orig_submessage);
    EXPECT_EQ(42, released->bb());
    delete released;
  }

  // Test UnsafeArenaReleaseLast().
  for (int i = 0; i < 10; i++) {
    TestAllTypes::NestedMessage* nested =
        Arena::Create<TestAllTypes::NestedMessage>(&arena);
    nested->set_bb(42);
    arena_message->mutable_repeated_nested_message()->AddAllocated(nested);
  }

  for (int i = 0; i < 10; i++) {
    const TestAllTypes::NestedMessage* orig_submessage =
        &arena_message->repeated_nested_message(10 - 1 - i);  // last element
    TestAllTypes::NestedMessage* released =
        arena_message->mutable_repeated_nested_message()
            ->UnsafeArenaReleaseLast();
    EXPECT_EQ(released, orig_submessage);
    EXPECT_EQ(42, released->bb());
    // no delete -- |released| is on the arena.
  }

  // Test string case as well. ReleaseLast() in this case must copy the
  // string, even though it was originally heap-allocated and its pointer
  // was simply appended to the repeated field's internal vector, because the
  // string was placed on the arena's destructor list and cannot be removed
  // from that list (so the arena permanently owns the original instance).
  arena_message->Clear();
  for (int i = 0; i < 10; i++) {
    std::string* s = new std::string("Test");
    arena_message->mutable_repeated_string()->AddAllocated(s);
  }
  for (int i = 0; i < 10; i++) {
    const std::string* orig_element =
        &arena_message->repeated_string(10 - 1 - i);
    std::string* released =
        arena_message->mutable_repeated_string()->ReleaseLast();
    EXPECT_NE(released, orig_element);
    EXPECT_EQ("Test", *released);
    delete released;
  }
}

TEST(ArenaTest, UnsafeArenaAddAllocated) {
  Arena arena;
  TestAllTypes* message = Arena::Create<TestAllTypes>(&arena);
  for (int i = 0; i < 10; i++) {
    std::string* arena_string = Arena::Create<std::string>(&arena);
    message->mutable_repeated_string()->UnsafeArenaAddAllocated(arena_string);
    EXPECT_EQ(arena_string, message->mutable_repeated_string(i));
  }
}

TEST(ArenaTest, OneofMerge) {
  Arena arena;
  TestAllTypes* message0 = Arena::Create<TestAllTypes>(&arena);
  TestAllTypes* message1 = Arena::Create<TestAllTypes>(&arena);

  message0->set_oneof_string("x");
  ASSERT_TRUE(message0->has_oneof_string());
  message1->set_oneof_string("y");
  ASSERT_TRUE(message1->has_oneof_string());
  EXPECT_EQ("x", message0->oneof_string());
  EXPECT_EQ("y", message1->oneof_string());
  message0->MergeFrom(*message1);
  EXPECT_EQ("y", message0->oneof_string());
  EXPECT_EQ("y", message1->oneof_string());
}

TEST(ArenaTest, ArenaOneofReflection) {
  Arena arena;
  TestAllTypes* message = Arena::Create<TestAllTypes>(&arena);
  const Descriptor* desc = message->GetDescriptor();
  const Reflection* refl = message->GetReflection();

  const FieldDescriptor* string_field = desc->FindFieldByName("oneof_string");
  const FieldDescriptor* msg_field =
      desc->FindFieldByName("oneof_nested_message");
  const OneofDescriptor* oneof = desc->FindOneofByName("oneof_field");

  refl->SetString(message, string_field, "Test value");
  EXPECT_TRUE(refl->HasOneof(*message, oneof));
  refl->ClearOneof(message, oneof);
  EXPECT_FALSE(refl->HasOneof(*message, oneof));

  Message* submsg = refl->MutableMessage(message, msg_field);
  EXPECT_TRUE(refl->HasOneof(*message, oneof));
  refl->ClearOneof(message, oneof);
  EXPECT_FALSE(refl->HasOneof(*message, oneof));
  refl->MutableMessage(message, msg_field);
  EXPECT_TRUE(refl->HasOneof(*message, oneof));
  submsg = refl->ReleaseMessage(message, msg_field);
  EXPECT_FALSE(refl->HasOneof(*message, oneof));
  EXPECT_TRUE(submsg->GetArena() == nullptr);
  delete submsg;
}

void TestSwapRepeatedField(Arena* arena1, Arena* arena2) {
  // Test "safe" (copying) semantics for direct Swap() on RepeatedPtrField
  // between arenas.
  RepeatedPtrField<TestAllTypes> field1(arena1);
  RepeatedPtrField<TestAllTypes> field2(arena2);
  for (int i = 0; i < 10; i++) {
    TestAllTypes* t = Arena::Create<TestAllTypes>(arena1);
    t->set_optional_string("field1");
    t->set_optional_int32(i);
    if (arena1 != nullptr) {
      field1.UnsafeArenaAddAllocated(t);
    } else {
      field1.AddAllocated(t);
    }
  }
  for (int i = 0; i < 5; i++) {
    TestAllTypes* t = Arena::Create<TestAllTypes>(arena2);
    t->set_optional_string("field2");
    t->set_optional_int32(i);
    if (arena2 != nullptr) {
      field2.UnsafeArenaAddAllocated(t);
    } else {
      field2.AddAllocated(t);
    }
  }
  field1.Swap(&field2);
  EXPECT_EQ(5, field1.size());
  EXPECT_EQ(10, field2.size());
  EXPECT_TRUE(std::string("field1") == field2.Get(0).optional_string());
  EXPECT_TRUE(std::string("field2") == field1.Get(0).optional_string());
  // Ensure that fields retained their original order:
  for (int i = 0; i < field1.size(); i++) {
    EXPECT_EQ(i, field1.Get(i).optional_int32());
  }
  for (int i = 0; i < field2.size(); i++) {
    EXPECT_EQ(i, field2.Get(i).optional_int32());
  }
}

TEST(ArenaTest, SwapRepeatedField) {
  Arena arena;
  TestSwapRepeatedField(&arena, &arena);
}

TEST(ArenaTest, SwapRepeatedFieldWithDifferentArenas) {
  Arena arena1;
  Arena arena2;
  TestSwapRepeatedField(&arena1, &arena2);
}

TEST(ArenaTest, SwapRepeatedFieldWithNoArenaOnRightHandSide) {
  Arena arena;
  TestSwapRepeatedField(&arena, nullptr);
}

TEST(ArenaTest, SwapRepeatedFieldWithNoArenaOnLeftHandSide) {
  Arena arena;
  TestSwapRepeatedField(nullptr, &arena);
}

TEST(ArenaTest, ExtensionsOnArena) {
  Arena arena;
  // Ensure no leaks.
  TestAllExtensions* message_ext = Arena::Create<TestAllExtensions>(&arena);
  message_ext->SetExtension(protobuf_unittest::optional_int32_extension, 42);
  message_ext->SetExtension(protobuf_unittest::optional_string_extension,
                            std::string("test"));
  message_ext
      ->MutableExtension(protobuf_unittest::optional_nested_message_extension)
      ->set_bb(42);
}

TEST(ArenaTest, RepeatedFieldOnArena) {
  // Preallocate an initial arena block to avoid mallocs during hooked region.
  std::vector<char> arena_block(1024 * 1024);
  Arena arena(arena_block.data(), arena_block.size());
  const size_t initial_allocated_size = arena.SpaceAllocated();

  {
    // Fill some repeated fields on the arena to test for leaks. Also that the
    // newly allocated memory is approximately the size of the cleanups for the
    // repeated messages.
    RepeatedField<int32_t> repeated_int32(&arena);
    RepeatedPtrField<TestAllTypes> repeated_message(&arena);
    for (int i = 0; i < 100; i++) {
      repeated_int32.Add(42);
      repeated_message.Add()->set_optional_int32(42);
      EXPECT_EQ(&arena, repeated_message.Get(0).GetArena());
      const TestAllTypes* msg_in_repeated_field = &repeated_message.Get(0);
      TestAllTypes* msg = repeated_message.UnsafeArenaReleaseLast();
      EXPECT_EQ(msg_in_repeated_field, msg);
    }

    // UnsafeArenaExtractSubrange (i) should not leak and (ii) should return
    // on-arena pointers.
    for (int i = 0; i < 10; i++) {
      repeated_message.Add()->set_optional_int32(42);
    }
    TestAllTypes* extracted_messages[5];
    repeated_message.UnsafeArenaExtractSubrange(0, 5, extracted_messages);
    EXPECT_EQ(&arena, repeated_message.Get(0).GetArena());
    EXPECT_EQ(5, repeated_message.size());
    // Upper bound of the size of the cleanups of new repeated messages.
    const size_t upperbound_cleanup_size =
        2 * 110 * sizeof(internal::cleanup::CleanupNode);
    EXPECT_GT(initial_allocated_size + upperbound_cleanup_size,
              arena.SpaceAllocated());
  }

  // Now test ExtractSubrange's copying semantics.
  {
    RepeatedPtrField<TestAllTypes> repeated_message(&arena);
    for (int i = 0; i < 100; i++) {
      repeated_message.Add()->set_optional_int32(42);
    }

    TestAllTypes* extracted_messages[5];
    // ExtractSubrange should copy to the heap.
    repeated_message.ExtractSubrange(0, 5, extracted_messages);
    EXPECT_EQ(nullptr, extracted_messages[0]->GetArena());
    // We need to free the heap-allocated messages to prevent a leak.
    for (int i = 0; i < 5; i++) {
      delete extracted_messages[i];
      extracted_messages[i] = nullptr;
    }
  }

  // Now check that we can create RepeatedFields/RepeatedPtrFields themselves on
  // the arena. They have the necessary type traits so that they can behave like
  // messages in this way. This is useful for higher-level generic templated
  // code that may allocate messages or repeated fields of messages on an arena.
  {
    RepeatedPtrField<TestAllTypes>* repeated_ptr_on_arena =
        Arena::Create<RepeatedPtrField<TestAllTypes>>(&arena);
    for (int i = 0; i < 10; i++) {
      // Add some elements and let the leak-checker ensure that everything is
      // freed.
      repeated_ptr_on_arena->Add();
    }

    RepeatedField<int>* repeated_int_on_arena =
        Arena::Create<RepeatedField<int>>(&arena);
    for (int i = 0; i < 100; i++) {
      repeated_int_on_arena->Add(i);
    }

  }

  arena.Reset();
}


#if PROTOBUF_RTTI
TEST(ArenaTest, MutableMessageReflection) {
  Arena arena;
  TestAllTypes* message = Arena::Create<TestAllTypes>(&arena);
  const Reflection* r = message->GetReflection();
  const Descriptor* d = message->GetDescriptor();
  const FieldDescriptor* field = d->FindFieldByName("optional_nested_message");
  TestAllTypes::NestedMessage* submessage =
      DownCastMessage<TestAllTypes::NestedMessage>(
          r->MutableMessage(message, field));
  TestAllTypes::NestedMessage* submessage_expected =
      message->mutable_optional_nested_message();

  EXPECT_EQ(submessage_expected, submessage);
  EXPECT_EQ(&arena, submessage->GetArena());

  const FieldDescriptor* oneof_field =
      d->FindFieldByName("oneof_nested_message");
  submessage = DownCastMessage<TestAllTypes::NestedMessage>(
      r->MutableMessage(message, oneof_field));
  submessage_expected = message->mutable_oneof_nested_message();

  EXPECT_EQ(submessage_expected, submessage);
  EXPECT_EQ(&arena, submessage->GetArena());
}
#endif  // PROTOBUF_RTTI


TEST(ArenaTest, ClearOneofMessageOnArena) {
  if (!internal::DebugHardenClearOneofMessageOnArena()) {
    GTEST_SKIP() << "arena allocated oneof message fields are not hardened.";
  }

  Arena arena;
  auto* message = Arena::Create<unittest::TestOneof2>(&arena);
  // Intentionally nested to force poisoning recursively to catch the access.
  auto* child =
      message->mutable_foo_message()->mutable_child()->mutable_child();
  child->set_moo_int(100);
  message->clear_foo_message();

  if (internal::HasMemoryPoisoning()) {
#if GTEST_HAS_DEATH_TEST
    EXPECT_DEATH(EXPECT_EQ(child->moo_int(), 0), "use-after-poison");
#endif  // !GTEST_HAS_DEATH_TEST
  } else {
    EXPECT_NE(child->moo_int(), 100);
  }
}

TEST(ArenaTest, CopyValuesWithinOneof) {
  if (!internal::DebugHardenClearOneofMessageOnArena()) {
    GTEST_SKIP() << "arena allocated oneof message fields are not hardened.";
  }

  Arena arena;
  auto* message = Arena::Create<unittest::TestOneof>(&arena);
  auto* foo = message->mutable_foogroup();
  foo->set_a(100);
  foo->set_b("hello world");
  message->set_foo_string(message->foogroup().b());

  // As a debug hardening measure, `set_foo_string` would clear `foo` in
  // (!NDEBUG && !ASAN) and the copy wouldn't work.
  EXPECT_TRUE(message->foo_string().empty()) << message->foo_string();
}

void FillArenaAwareFields(TestAllTypes* message) {
  std::string test_string = "hello world";
  message->set_optional_int32(42);
  message->set_optional_string(test_string);
  message->set_optional_bytes(test_string);
  message->mutable_optional_nested_message()->set_bb(42);

  message->set_oneof_uint32(42);
  message->mutable_oneof_nested_message()->set_bb(42);
  message->set_oneof_string(test_string);
  message->set_oneof_bytes(test_string);

  message->add_repeated_int32(42);
  // No repeated string: not yet arena-aware.
  message->add_repeated_nested_message()->set_bb(42);
  message->mutable_optional_lazy_message()->set_bb(42);
}

// Test: no allocations occur on heap while touching all supported field types.
TEST(ArenaTest, NoHeapAllocationsTest) {
  if (internal::DebugHardenClearOneofMessageOnArena()) {
    GTEST_SKIP() << "debug hardening may cause heap allocation.";
  }

  // Allocate a large initial block to avoid mallocs during hooked test.
  std::vector<char> arena_block(128 * 1024);
  ArenaOptions options;
  options.initial_block = &arena_block[0];
  options.initial_block_size = arena_block.size();
  Arena arena(options);

  {
    // We need to call Arena::Create before NoHeapChecker because the ArenaDtor
    // allocates a new cleanup chunk.
    TestAllTypes* message = Arena::Create<TestAllTypes>(&arena);


    FillArenaAwareFields(message);
  }

  arena.Reset();
}

#if PROTOBUF_RTTI
// Test construction on an arena via generic MessageLite interface. We should be
// able to successfully deserialize on the arena without incurring heap
// allocations, i.e., everything should still be arena-allocation-aware.
TEST(ArenaTest, MessageLiteOnArena) {
  std::vector<char> arena_block(128 * 1024);
  ArenaOptions options;
  options.initial_block = &arena_block[0];
  options.initial_block_size = arena_block.size();
  Arena arena(options);
  const MessageLite* prototype = &TestAllTypes::default_instance();

  TestAllTypes initial_message;
  FillArenaAwareFields(&initial_message);
  std::string serialized;
  initial_message.SerializeToString(&serialized);

  {
    MessageLite* generic_message = prototype->New(&arena);


    EXPECT_TRUE(generic_message != nullptr);
    EXPECT_EQ(&arena, generic_message->GetArena());
    EXPECT_TRUE(generic_message->ParseFromString(serialized));
    TestAllTypes* deserialized = static_cast<TestAllTypes*>(generic_message);
    EXPECT_EQ(42, deserialized->optional_int32());
  }

  arena.Reset();
}
#endif  // PROTOBUF_RTTI

// Align n to next multiple of 8
uint64_t Align8(uint64_t n) { return (n + 7) & -8; }

TEST(ArenaTest, SpaceAllocated_and_Used) {
  Arena arena_1;
  EXPECT_EQ(0, arena_1.SpaceAllocated());
  EXPECT_EQ(0, arena_1.SpaceUsed());
  EXPECT_EQ(0, arena_1.Reset());
  Arena::CreateArray<char>(&arena_1, 320);
  // Arena will allocate slightly more than 320 for the block headers.
  EXPECT_LE(320, arena_1.SpaceAllocated());
  EXPECT_EQ(Align8(320), arena_1.SpaceUsed());
  EXPECT_LE(320, arena_1.Reset());

  // Test with initial block.
  std::vector<char> arena_block(1024);
  ArenaOptions options;
  options.start_block_size = 256;
  options.max_block_size = 8192;
  options.initial_block = &arena_block[0];
  options.initial_block_size = arena_block.size();
  Arena arena_2(options);
  EXPECT_EQ(1024, arena_2.SpaceAllocated());
  EXPECT_EQ(0, arena_2.SpaceUsed());
  EXPECT_EQ(1024, arena_2.Reset());
  Arena::CreateArray<char>(&arena_2, 55);
  EXPECT_EQ(1024, arena_2.SpaceAllocated());
  EXPECT_EQ(Align8(55), arena_2.SpaceUsed());
  EXPECT_EQ(1024, arena_2.Reset());
}

namespace {

void VerifyArenaOverhead(Arena& arena, size_t overhead) {
  EXPECT_EQ(0, arena.SpaceAllocated());

  // Allocate a tiny block and record the allocation size.
  constexpr size_t kTinySize = 8;
  Arena::CreateArray<char>(&arena, kTinySize);
  uint64_t space_allocated = arena.SpaceAllocated();

  // Next allocation expects to fill up the block but no new block.
  uint64_t next_size = space_allocated - overhead - kTinySize;
  Arena::CreateArray<char>(&arena, next_size);

  EXPECT_EQ(space_allocated, arena.SpaceAllocated());
}

}  // namespace

TEST(ArenaTest, FirstArenaOverhead) {
  Arena arena;
  VerifyArenaOverhead(arena, internal::SerialArena::kBlockHeaderSize);
}


TEST(ArenaTest, StartingBlockSize) {
  Arena default_arena;
  EXPECT_EQ(0, default_arena.SpaceAllocated());

  // Allocate something to get starting block size.
  Arena::CreateArray<char>(&default_arena, 1);
  ArenaOptions options;
  // First block size should be the default starting block size.
  EXPECT_EQ(default_arena.SpaceAllocated(), options.start_block_size);

  // Use a custom starting block size.
  options.start_block_size *= 2;
  Arena custom_arena(options);
  Arena::CreateArray<char>(&custom_arena, 1);
  EXPECT_EQ(custom_arena.SpaceAllocated(), options.start_block_size);
}

TEST(ArenaTest, BlockSizeDoubling) {
  Arena arena;
  EXPECT_EQ(0, arena.SpaceUsed());
  EXPECT_EQ(0, arena.SpaceAllocated());

  // Allocate something to get initial block size.
  Arena::CreateArray<char>(&arena, 1);
  auto first_block_size = arena.SpaceAllocated();

  // Keep allocating until space used increases.
  while (arena.SpaceAllocated() == first_block_size) {
    Arena::CreateArray<char>(&arena, 1);
  }
  ASSERT_GT(arena.SpaceAllocated(), first_block_size);
  auto second_block_size = (arena.SpaceAllocated() - first_block_size);

  EXPECT_GE(second_block_size, 2*first_block_size);
}

TEST(ArenaTest, Alignment) {
  Arena arena;
  for (int i = 0; i < 200; i++) {
    void* p = Arena::CreateArray<char>(&arena, i);
    ABSL_CHECK_EQ(reinterpret_cast<uintptr_t>(p) % 8, 0u) << i << ": " << p;
  }
}

TEST(ArenaTest, BlockSizeSmallerThanAllocation) {
  for (size_t i = 0; i <= 8; ++i) {
    ArenaOptions opt;
    opt.start_block_size = opt.max_block_size = i;
    Arena arena(opt);

    *Arena::Create<int64_t>(&arena) = 42;
    EXPECT_GE(arena.SpaceAllocated(), 8);
    EXPECT_EQ(8, arena.SpaceUsed());

    *Arena::Create<int64_t>(&arena) = 42;
    EXPECT_GE(arena.SpaceAllocated(), 16);
    EXPECT_EQ(16, arena.SpaceUsed());
  }
}

TEST(ArenaTest, GetArenaShouldReturnTheArenaForArenaAllocatedMessages) {
  Arena arena;
  ArenaMessage* message = Arena::Create<ArenaMessage>(&arena);
  const ArenaMessage* const_pointer_to_message = message;
  EXPECT_EQ(&arena, message->GetArena());
  EXPECT_EQ(&arena, const_pointer_to_message->GetArena());

  // Test that the Message* / MessageLite* specialization SFINAE works.
  const Message* const_pointer_to_message_type = message;
  EXPECT_EQ(&arena, const_pointer_to_message_type->GetArena());
  const MessageLite* const_pointer_to_message_lite_type = message;
  EXPECT_EQ(&arena, const_pointer_to_message_lite_type->GetArena());
}

TEST(ArenaTest, GetArenaShouldReturnNullForNonArenaAllocatedMessages) {
  ArenaMessage message;
  const ArenaMessage* const_pointer_to_message = &message;
  EXPECT_EQ(nullptr, message.GetArena());
  EXPECT_EQ(nullptr, const_pointer_to_message->GetArena());
}

TEST(ArenaTest, AddCleanup) {
  Arena arena;
  for (int i = 0; i < 100; i++) {
    arena.Own(new int);
  }
}

struct DestroyOrderRecorder {
  std::vector<int>* destroy_order;
  int i;

  DestroyOrderRecorder(std::vector<int>* destroy_order, int i)
      : destroy_order(destroy_order), i(i) {}
  ~DestroyOrderRecorder() { destroy_order->push_back(i); }
};

// TODO: we do not guarantee this behavior, but some users rely on
// it. We need to decide whether we want to guarantee this. In the meantime,
// user code should avoid adding new dependencies on this.
// Tests that when using an Arena from a single thread, objects are destroyed in
// reverse order from construction.
TEST(ArenaTest, CleanupDestructionOrder) {
  std::vector<int> destroy_order;
  {
    Arena arena;
    for (int i = 0; i < 3; i++) {
      Arena::Create<DestroyOrderRecorder>(&arena, &destroy_order, i);
    }
  }
  EXPECT_THAT(destroy_order, testing::ElementsAre(2, 1, 0));
}

TEST(ArenaTest, SpaceReuseForArraysSizeChecks) {
  // Limit to 1<<20 to avoid using too much memory on the test.
  for (int i = 0; i < 20; ++i) {
    SCOPED_TRACE(i);
    Arena arena;
    std::vector<void*> pointers;

    const size_t size = 16 << i;

    for (int j = 0; j < 10; ++j) {
      pointers.push_back(Arena::CreateArray<char>(&arena, size));
    }

    for (void* p : pointers) {
      internal::ArenaTestPeer::ReturnArrayMemory(&arena, p, size);
    }

    std::vector<void*> second_pointers;
    for (int j = 9; j != 0; --j) {
      second_pointers.push_back(Arena::CreateArray<char>(&arena, size));
    }

    // The arena will give us back the pointers we returned, except the first
    // one. That one becomes part of the freelist data structure.
    ASSERT_THAT(second_pointers,
                testing::UnorderedElementsAreArray(
                    std::vector<void*>(pointers.begin() + 1, pointers.end())));
  }
}

TEST(ArenaTest, SpaceReusePoisonsAndUnpoisonsMemory) {
  if constexpr (!internal::HasMemoryPoisoning()) {
    GTEST_SKIP() << "Memory poisoning not enabled.";
  }

  char buf[1024]{};
  constexpr int kSize = 32;
  {
    Arena arena(buf, sizeof(buf));
    std::vector<void*> pointers;
    for (int i = 0; i < 100; ++i) {
      void* p = Arena::CreateArray<char>(&arena, kSize);
      // Simulate other ASan client managing shadow memory.
      internal::PoisonMemoryRegion(p, kSize);
      internal::UnpoisonMemoryRegion(p, kSize - 4);
      pointers.push_back(p);
    }
    for (void* p : pointers) {
      internal::ArenaTestPeer::ReturnArrayMemory(&arena, p, kSize);
      // The first one is not poisoned because it becomes the freelist.
      if (p != pointers[0]) {
        EXPECT_TRUE(internal::IsMemoryPoisoned(p));
      }
    }

    bool found_poison = false;
    for (char& c : buf) {
      if (internal::IsMemoryPoisoned(&c)) {
        found_poison = true;
        break;
      }
    }
    EXPECT_TRUE(found_poison);
  }

  // Should not be poisoned after destruction.
  for (char& c : buf) {
    ASSERT_FALSE(internal::IsMemoryPoisoned(&c));
  }
}

  });
  Arena arena_obj;
  // Delay deallocations until after the loop to prevent the allocator from
  // reusing the just-freed memory on the next iteration.
  std::vector<char*> recycle_bin;
  static_assert(kMinAlign <= kMaxAlign, "Real code always complies");
  for (Arena* arena : {static_cast<Arena*>(nullptr), &arena_obj}) {
    for (size_t num_elements = kMinElements; num_elements <= kMaxElements;
         (num_elements < 10) ? (num_elements += 1) : (num_elements += 10)) {
      const size_t array_bytes = num_elements * kSize;
      for (size_t align = kMinAlign; align <= kMaxAlign; align *= 2) {
        SCOPED_TRACE(absl::Substitute("arena=$0 num_elements=$1 align=$2",
                                      arena, num_elements, align));
        char* const arr = Arena::CreateArray<char>(arena, array_bytes);
        EXPECT_EQ(arr != nullptr, num_elements != 0);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(arr) % align, 0)
            << "arr=" << (void*)arr;
#if defined(PROTOBUF_ASAN) || defined(PROTOBUF_MSAN)
        // Touch the expected span of addresses to have the sanitizers verify
        // that the returned memory is not poisoned.
        T* const arr_t = reinterpret_cast<T* const>(arr);
        for (size_t i = 0; i < num_elements; ++i) {
          arr_t[i] = 0xAB;
        }
#endif
        if (arena == nullptr) {
          recycle_bin.push_back(arr);
        }
      }
    }
  }

  for (auto* ptr : recycle_bin) {
    delete[] ptr;
  }
}

struct InitializationVerifier {
  static constexpr int kDefaultScalar = 0x11;
  static constexpr int kDefaultArray = 0x22;

  static void* operator new(size_t, void* ptr) { return ptr; }

  static void* operator new(size_t n) {
    void* ret = ::operator new(n);
    memset(ret, kDefaultScalar, n);
    return ret;
  }

  // Ignore the possible alignment param. Need this just so
  // `Arena::CreateArray()` compiles.
  static void* operator new[](size_t n, ...) {
    void* ret = ::operator new[](n);
    memset(ret, kDefaultArray, n);
    return ret;
  }

  int a;
};

TEST(ArenaTest, ArenaValueInitialization) {
  std::unique_ptr<InitializationVerifier> scalar(
      Arena::Create<InitializationVerifier>(nullptr));
  // Create value initializes.
  EXPECT_EQ(scalar->a, 0);

  int pattern;
  memset(&pattern, InitializationVerifier::kDefaultArray, sizeof(pattern));

  std::unique_ptr<InitializationVerifier[]> array(
      Arena::CreateArray<InitializationVerifier>(nullptr, 2));
  // CreateArray default initializes.
  EXPECT_EQ(array[0].a, pattern);
  EXPECT_EQ(array[1].a, pattern);
}

TEST(ArenaTest, b237489184) {
  google::protobuf::ArenaOptions arena_options;
  arena_options.start_block_size = 1;
  arena_options.max_block_size = 10;
  google::protobuf::Arena arena{arena_options};
  // Fill our first block
  arena.OwnCustomDestructor(
      &arena, +[](void*) {});
  // then put a custom destructor on our second block
  arena.OwnCustomDestructor(
      &arena, +[](void*) {});
  // asan errors on `~Arena`
}

// Helper for BM_MultiThreadedAlloc.  Synchronously creates a new
// arena after every K iterations to prevent unbounded memory growth.
class ArenaManager {
 public:
  ArenaManager() = default;

  // Called by benchmark setup code to initialize number of threads.
  void Init(const benchmark::State& state) {
    absl::MutexLock l(&mu_);
    num_threads_ = state.threads();
    num_waiting_ = 0;
    phase_ = 0;
    delete arena_;
    arena_ = nullptr;
  }

  // Wait until all benchmark threads have made this call and then
  // returns a new arena to all of them.
  Arena* GetNextArena(bool initial_block) {
    mu_.Lock();
    num_waiting_++;
    if (num_waiting_ == num_threads_) {
      // Last thread in switches to a new arena.
      delete arena_;
      if (initial_block) {
        arena_ = new Arena(initial_block_, kDefaultStartBlockSize);
      } else {
        arena_ = new Arena;
      }
      phase_++;
      num_waiting_ = 0;
    } else {
      // Wait until last thread switches to a new arena.
      int64_t current = phase_;
      auto switched = [this, current] { return phase_ != current; };
      mu_.Await(absl::Condition(&switched));
    }
    Arena* result = arena_;
    mu_.Unlock();
    return result;
  }

 private:
  absl::Mutex mu_;
  int num_threads_ = 0;
  int num_waiting_ = 0;
  int64_t phase_ = 0;
  static constexpr size_t kDefaultStartBlockSize = 256;
  char initial_block_[kDefaultStartBlockSize];
  Arena* arena_ = nullptr;
};

ArenaManager* ArenaManagerSingleton() {
  static auto* arena_manager = new ArenaManager;
  return arena_manager;
}

// Synchronously creates a new list of arenas after every K iterations to
// prevent unbounded memory growth.  On average, each thread is assigned one
// arena.
class MultiArenaManager {
 public:
  // Re-initializes the arena manager to expect |num_threads_| threads.
  // Called by benchmark setup code.
  void Init(const benchmark::State& state) ABSL_NO_THREAD_SAFETY_ANALYSIS {
    num_threads_ = state.threads();
    num_waiting_ = 0;
    phase_ = 0;
    // Has to be before resetting the initial block in case arenas reference it.
    arena_list_.clear();
    initial_block_ =
        std::make_unique<char[]>(kDefaultStartBlockSize * num_threads_);
  }

  // Waits until all benchmark threads have made this call and then
  // refreshes the arena list.
  void UpdateArenaList(bool initial_block) ABSL_LOCKS_EXCLUDED(mu_) {
    absl::MutexLock l(&mu_);
    num_waiting_++;
    if (num_waiting_ == num_threads_) {
      // Last thread in switches to a new arena list.
      arena_list_.clear();
      if (initial_block) {
        for (int i = 0; i < num_threads_; i++) {
          char* block = &initial_block_[kDefaultStartBlockSize * i];
          arena_list_.push_back(
              std::make_unique<Arena>(block, kDefaultStartBlockSize));
        }
      } else {
        for (int i = 0; i < num_threads_; i++) {
          arena_list_.push_back(std::make_unique<Arena>());
        }
      }
      phase_++;
      num_waiting_ = 0;
    } else {
      // Waits until last thread switches to a new arena.
      int64_t current = phase_;
      auto switched = [this, current]() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        return phase_ != current;
      };
      mu_.Await(absl::Condition(&switched));
    }
  }

  // Gets a random arena from the list.
  // When calling this thread, UpdateArenaList guarantees no change to
  // arena_list_.
  Arena* GetRandomArena() {
    return arena_list_.at(rand() % num_threads_).get();
  }

  int GetThreadNum() const { return num_threads_; }

 private:
  absl::Mutex mu_;
  int num_threads_ = 0;
  int num_waiting_ ABSL_GUARDED_BY(mu_) = 0;
  int64_t phase_ ABSL_GUARDED_BY(mu_) = 0;
  static constexpr size_t kDefaultStartBlockSize = 256;
  std::unique_ptr<char[]> initial_block_;
  std::vector<std::unique_ptr<Arena>> arena_list_;
};

MultiArenaManager* MultiArenaManagerSingleton() {
  static auto* arena_manager = new MultiArenaManager;
  return arena_manager;
}

static void RunAlloc(benchmark::State& state, bool initial_block,
                     int minimum_number_of_arenas) {
  // We use an initial block if (initial_block != 0).
  int i = 0;
  for (auto s : state) {
    auto arena = ArenaManagerSingleton()->GetNextArena(initial_block);
    const int left =
        std::min<int>(state.max_iterations - i, minimum_number_of_arenas);
    i += left;
    for (int j = 0; j < left; j++) {
      benchmark::DoNotOptimize(arena);
      Arena::CreateArray<char>(arena, 64);
    }
  }
}

static void BM_Alloc(benchmark::State& state) {
  const int minimum_number_of_arenas = state.range(0);

  RunAlloc(state,
           /*initial_block=*/false, minimum_number_of_arenas);
}
BENCHMARK(BM_Alloc)->ThreadRange(1, 32)->Arg(1000)->Arg(10000)->Setup(
    [](const benchmark::State& state) {
      ArenaManagerSingleton()->Init(state);
    });

static void BM_Alloc_InitialBlock(benchmark::State& state) {
  const int minimum_number_of_arenas = state.range(0);

  RunAlloc(state,
           /*initial_block=*/true, minimum_number_of_arenas);
}
BENCHMARK(BM_Alloc_InitialBlock)
    ->ThreadRange(1, 32)
    ->Arg(1000)
    ->Arg(10000)
    ->Setup([](const benchmark::State& state) {
      ArenaManagerSingleton()->Init(state);
    });

static void RunAllocMultiArena(benchmark::State& state, bool initial_block,
                               int minimum_number_of_arenas) {
  // We use an initial block if (initial_block != 0).
  int i = 0;
  for (auto s : state) {
    state.PauseTiming();
    MultiArenaManagerSingleton()->UpdateArenaList(initial_block);
    std::vector<Arena*> arena_list;
    // MultiArenaManager has num_threads arenas. Fetch that many,
    // in random order. This is approximately a random permutation
    // of the available arenas, but not quite since it allows dups.
    int thread_num = MultiArenaManagerSingleton()->GetThreadNum();
    for (int j = 0; j < thread_num; j++) {
      auto* arena = MultiArenaManagerSingleton()->GetRandomArena();
      arena_list.push_back(arena);
    }
    state.ResumeTiming();

    const int left =
        std::min<int>(state.max_iterations - i, minimum_number_of_arenas);
    i += left;
    for (int j = 0; j < left; j++) {
      auto* arena = arena_list.at(j % thread_num);
      auto* array = Arena::CreateArray<char>(arena, 64);
      benchmark::DoNotOptimize(array);
    }
  }
}

static void BM_AllocMultiArena(benchmark::State& state) {
  const int minimum_number_of_arenas = state.range(0);

  RunAllocMultiArena(state,
                     /*initial_block=*/false, minimum_number_of_arenas);
}
BENCHMARK(BM_AllocMultiArena)
    ->ThreadRange(1, 32)
    ->Arg(1000)
    ->Arg(10000)
    ->Setup([](const benchmark::State& state) {
      MultiArenaManagerSingleton()->Init(state);
    });

static void BM_AllocMultiArena_InitialBlock(benchmark::State& state) {
  const int minimum_number_of_arenas = state.range(0);

  RunAllocMultiArena(state,
                     /*initial_block=*/true, minimum_number_of_arenas);
}
BENCHMARK(BM_AllocMultiArena_InitialBlock)
    ->ThreadRange(1, 32)
    ->Arg(1000)
    ->Arg(10000)
    ->Setup([](const benchmark::State& state) {
      MultiArenaManagerSingleton()->Init(state);
    });

static void BM_MemoryUsage(benchmark::State& state) {
  BenchmarkMemoryUsage(testing::kAllThreadUsage);
  for (auto s : state) {
    auto arena = std::make_unique<Arena>();
    auto* array = Arena::CreateArray<char>(arena.get(), 1);
    benchmark::DoNotOptimize(array);
  }
}
BENCHMARK(BM_MemoryUsage);

static void BM_CreateDestroy(benchmark::State& state) {
  for (auto s : state) {
    Arena arena;
    benchmark::DoNotOptimize(arena);
  }
}
BENCHMARK(BM_CreateDestroy);

static void BM_CreateDestroy_InitialBlock(benchmark::State& state) {
  const int kBufSize = Arena::kBlockOverhead + 64;
  char buf[kBufSize];
  for (auto s : state) {
    Arena arena(buf, kBufSize);
    benchmark::DoNotOptimize(arena);
  }
}
BENCHMARK(BM_CreateDestroy_InitialBlock);

static void BM_CreateAllocDestroy(benchmark::State& state) {
  for (auto s : state) {
    Arena arena;
    Arena::CreateArray<char>(&arena, 7);
  }
}
BENCHMARK(BM_CreateAllocDestroy);

static void BM_CreateAllocDestroy_InitialBlock(benchmark::State& state) {
  const int kBufSize = Arena::kBlockOverhead + 64;
  char buf[kBufSize];

  for (auto s : state) {
    Arena arena(buf, kBufSize);
    Arena::CreateArray<char>(&arena, 7);
  }
}
BENCHMARK(BM_CreateAllocDestroy_InitialBlock);

// We should make sure we do something with the returned memory. Otherwise
// the compiler will perform an optimization thats unrealistic to happen
// in normal use of arena allocation.
inline void Use(void* ptr) { *static_cast<uint64_t*>(ptr) = 0; }

PROTOBUF_NOINLINE
static void Impl_ArenaAlloc(google::protobuf::Arena* arena, int n_allocs_div_10) {
  for (int i = 0; i < n_allocs_div_10; i++) {
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
    Use(google::protobuf::Arena::CreateArray<char>(arena, sizeof(uint64_t)));
  }
}

static void BM_ArenaAlloc(benchmark::State& state) {
  const int n_allocs = state.range(0);

  ABSL_CHECK_EQ((n_allocs % 10), 0);
  int n_allocs_div_10 = n_allocs / 10;
  char buf[1024];
  for (auto s : state) {
    google::protobuf::Arena arena(buf, sizeof(buf));
    Impl_ArenaAlloc(&arena, n_allocs_div_10);
  }
}
BENCHMARK(BM_ArenaAlloc)->Arg(0)->Arg(10)->Arg(100);

PROTOBUF_NOINLINE
static void Impl_ArenaAllocWithCleanup(google::protobuf::Arena* arena,
                                       int n_allocs_div_10) {
  struct X {
    uint64_t data;
    ~X() { ABSL_BLOCK_TAIL_CALL_OPTIMIZATION(); }
  };
  for (int i = 0; i < n_allocs_div_10; i++) {
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
    Use(google::protobuf::Arena::Create<X>(arena));
  }
}

static void BM_ArenaAllocWithCleanup(benchmark::State& state) {
  const int n_allocs = state.range(0);

  ABSL_CHECK_EQ((n_allocs % 10), 0);
  int n_allocs_div_10 = n_allocs / 10;
  char buf[1024];
  for (auto s : state) {
    google::protobuf::Arena arena(buf, sizeof(buf));
    Impl_ArenaAllocWithCleanup(&arena, n_allocs_div_10);
  }
}
BENCHMARK(BM_ArenaAllocWithCleanup)->Arg(0)->Arg(10)->Arg(100);

void BM_ParseRepeatedStringFieldArena(benchmark::State& state) {
  TestAllTypes original;
  for (int i = 0; i < state.range(0); i++) {
    original.add_repeated_string("abc");
  }
  const std::string data = original.SerializeAsString();
  Arena arena;
  constexpr int kResetLimit = 10;
  for (auto s : state) {
    TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
    arena_message->ParseFromString(data);
    // Avoids memory in arena from ever increasing.
    if (state.iterations() % kResetLimit == 0) {
      arena.Reset();
    }
  }
}
BENCHMARK(BM_ParseRepeatedStringFieldArena)->Arg(10)->Arg(100)->Arg(1000);

void BM_ArenaParseRepeatedPtrField(benchmark::State& state) {
  TestAllTypes original;
  for (int i = 0; i < state.range(0); i++) {
    auto* nested = original.add_repeated_foreign_message();
    nested->set_c(1);
  }
  const std::string data = original.SerializeAsString();
  Arena arena;
  constexpr int kResetLimit = 10;
  for (auto s : state) {
    TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
    arena_message->ParseFromString(data);
    // Avoids memory in arena from ever increasing.
    if (state.iterations() % kResetLimit == 0) {
      arena.Reset();
    }
  }
}
BENCHMARK(BM_ArenaParseRepeatedPtrField)->Arg(10)->Arg(100)->Arg(1000);

void BM_ArenaMergeFromRepeatedPtrField(benchmark::State& state) {
  Arena arena;
  constexpr int kResetLimit = 10;
  auto* original = Arena::Create<TestAllTypes>(&arena);
  for (int i = 0; i < state.range(0); i++) {
    auto* nested = original->add_repeated_foreign_message();
    nested->set_c(1);
  }
  // Used to restore original after resetting arena.
  const std::string data = original->SerializeAsString();
  for (auto s : state) {
    TestAllTypes* arena_message = Arena::Create<TestAllTypes>(&arena);
    arena_message->MergeFrom(*original);
    // Avoids memory in arena from ever increasing.
    if (state.iterations() % kResetLimit == 0) {
      arena.Reset();
      original = Arena::Create<TestAllTypes>(&arena);
      original->ParseFromString(data);
    }
  }
}
BENCHMARK(BM_ArenaMergeFromRepeatedPtrField)->Arg(10)->Arg(100)->Arg(1000);

static void BM_CopyConstructorRepeatedPtrField(benchmark::State& state) {
  const int n_elems = state.range(0);

  TestAllTypes original;
  for (int i = 0; i < n_elems; i++) {
    auto* nested = original.add_repeated_foreign_message();
    nested->set_c(1);
  }
  for (auto s : state) {
    TestAllTypes message(original);
    benchmark::DoNotOptimize(message);
  }
}
BENCHMARK(BM_CopyConstructorRepeatedPtrField)->Arg(10)->Arg(100)->Arg(1000);

namespace internal {
class ArenaBenchmark {
 public:
  static auto GetSerialArena(ThreadSafeArena& arena) {
    return arena.GetSerialArenaFallback(0);
  }
  static uint64_t ExpectLifecycleIdIncrementLocally(uint64_t prev) {
    uint64_t curr = ThreadSafeArena::thread_cache().next_lifecycle_id;
    if (prev == 0) return curr;
    EXPECT_LT(curr - prev, ThreadSafeArena::ThreadCache::kPerThreadIds);
    return curr;
  }
};
}  // namespace internal

TEST(ArenaTest, IncrementLifecycleIdLocally) {
  Arena arena;
  int num_reset = 1000;
  uint64_t prev = 0;
  while (--num_reset >= 0) {
    arena.Reset();
    prev = internal::ArenaBenchmark::ExpectLifecycleIdIncrementLocally(prev);

    auto* msg = Arena::Create<TestAllTypes>(&arena);
    TestUtil::SetAllFields(msg);
    TestUtil::ExpectAllFieldsSet(*msg);
  }
}

// Helper class that places (scopes) an `AllocateAtLeast` hook for the current
// thread, multiplies the requested size by a provided `multiplier`, and counts
// the number of `AllocAtLeast` invocations for the scoped execution.
class ScopedAllocAtLeastHook {
 public:
  // Sets `internal:allocate_at_least_hook` to this instance.
  explicit ScopedAllocAtLeastHook(size_t multiplier) : multiplier_(multiplier) {
    internal::SetAllocateAtLeastHook(&ScopedAllocAtLeastHook::Hook, this);
  }

  // Sets `internal:allocate_at_least_hook` to null
  ~ScopedAllocAtLeastHook() { internal::SetAllocateAtLeastHook(nullptr); }

  // Returns the number of allocations executed inside this scope.
  size_t allocation_count() const { return alloc_count_; }

 private:
  static internal::SizedPtr Hook(size_t size, void* context) {
    auto self = static_cast<ScopedAllocAtLeastHook*>(context);
    ++self->alloc_count_;
    size *= self->multiplier_;
    return {::operator new(size), size};
  }

  int alloc_count_ = 0;
  const size_t multiplier_;
};

TEST(ArenaTest, UsesExtraSizeReturnedByAllocateAtLeastForNewBlocks) {
  if (!internal::HaveAllocateAtLeastHook()) {
    GTEST_SKIP() << "AllocateAtLeastHook() not available";
  }

  // We test that AllocateAtLeast is used properly by counting the total
  // count of AllocateAtLeast() calls for a fixed number of times, and compare
  // those counts with and without returning quadruple the size requested
  std::array<int, 2> new_block_count{0};
  for (int run = 0; run < 2; ++run) {
    ScopedAllocAtLeastHook hook(run == 0 ? 1 : 4);

    // Hit 'NewBlock' logic in arena allocate
    Arena arena;
    for (int i = 0; i < 100; ++i) {
      arena.AllocateAligned(128);
    }
    new_block_count[run] = hook.allocation_count();
  }
  EXPECT_LT(new_block_count[1], new_block_count[0]);
}

TEST(ArenaTest, UsesExtraSizeReturnedByAllocateAtLeastForThreadArenaArray) {
  if (!internal::HaveAllocateAtLeastHook()) {
    GTEST_SKIP() << "AllocateAtLeastHook() not available";
  }

  // We test that AllocateAtLeast is used properly by counting the total
  // count of AllocateAtLeast() calls for a fixed number of times, and compare
  // those counts with and without returning quadruple the size requested
  // Hit 'NewSerialArena` logic.
  std::array<std::atomic<int>, 2> new_thread_count{0};
  for (int run = 0; run < 2; ++run) {
    // The counts for single AllocateAligned are identical, but the array
    // allocation for SerialArena pointers should be less allocs.
    Arena arena;
    std::vector<std::thread> threads;
    for (int i = 0; i < 256; ++i) {
      threads.emplace_back([&] {
        ScopedAllocAtLeastHook hook(run == 0 ? 1 : 4);
        arena.AllocateAligned(8);
        new_thread_count[run] += hook.allocation_count();
      });
    }
    for (auto& thread : threads) {
      thread.join();
    }
  }
  EXPECT_LT(new_thread_count[1], new_thread_count[0]);
}

TEST(ArenaTest, AllStringBlockSizesEmplaceCorrectly) {
  // Allocate buffer large enough to fit exponentially growing string
  // blocks up to 64K, which should cover any reasonable max size.
  std::vector<char> arena_block(64 << 10);
  ArenaOptions options;
  options.initial_block = arena_block.data();
  options.initial_block_size = arena_block.size();
  Arena arena(options);
  int runs = (64 << 10) / sizeof(std::string);
  for (int i = 0; i < runs; ++i) {
    Arena::Create<std::string>(&arena);
  }
}

PROTOBUF_NOINLINE
static void Impl_SerialArenaAlloc(internal::SerialArena* serial,
                                  int n_allocs_div_10) {
  for (int i = 0; i < n_allocs_div_10; i++) {
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
    Use(serial->AllocateAligned(sizeof(uint64_t)));
  }
}

static void BM_SerialArenaAlloc(benchmark::State& state) {
  const int n_allocs = state.range(0);

  ABSL_CHECK_EQ((n_allocs % 10), 0);
  int n_allocs_div_10 = n_allocs / 10;

  char mem[1024];
  for (auto s : state) {
    internal::ThreadSafeArena arena(mem, sizeof(mem));
    Impl_SerialArenaAlloc(internal::ArenaBenchmark::GetSerialArena(arena),
                          n_allocs_div_10);
  }
}
BENCHMARK(BM_SerialArenaAlloc)->Arg(0)->Arg(10)->Arg(100);

template <bool use_arena>
static void BM_RepeatedStringAdd(benchmark::State& state) {
  const auto count = static_cast<size_t>(state.range(0));
  const std::string long_string(sizeof(std::string), 'a');
  for (auto s : state) {
    Arena arena;
    auto res =
        MakeArenaSafeUnique<TestRepeatedString>(use_arena ? &arena : nullptr);
    for (uint32_t i = 0; i < count; ++i) {
      *res->add_repeated_string1() = "aaa";
      *res->add_repeated_string2() = long_string;

      *res->add_repeated_bytes11() = "aaa";
      *res->add_repeated_bytes12() = long_string;
    }
  }
}

static void BM_RepeatedStringAddWithArena(benchmark::State& state) {
  BM_RepeatedStringAdd<true>(state);
}

static void BM_RepeatedStringAddWithoutArena(benchmark::State& state) {
  BM_RepeatedStringAdd<false>(state);
}

BENCHMARK(BM_RepeatedStringAddWithArena)->Arg(0)->Arg(10)->Arg(100);
BENCHMARK(BM_RepeatedStringAddWithoutArena)->Arg(0)->Arg(10)->Arg(100);



}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
