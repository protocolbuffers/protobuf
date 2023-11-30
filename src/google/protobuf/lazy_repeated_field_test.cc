#include "google/protobuf/lazy_repeated_field.h"

#include <time.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/casts.h"
#include "net/proto2/arena/arena_safe_unique_ptr.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/cord.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/internal_visibility_for_testing.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"
#include "thread/executor.h"
#include "thread/thread.h"
#include "thread/threadpool.h"

// Must be included last.
// clang-format off
#include "net/proto2/public/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {

template <typename Type>
const Type& GetRawForTest(const Message& message,
                          const FieldDescriptor* field) {
  return message.GetReflection()->GetRaw<Type>(message, field);
}

namespace internal {

namespace {
using ::google::protobuf::ArenaSafeUniquePtr;
using ::google::protobuf::MakeArenaSafeUnique;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::EqualsProto;
using ::testing::proto::WhenDeserialized;
using unittest::TestAllTypes;

// Note this is very similar to the LazyRepeatedPtrField state but the test
// ensures correctness for a new 'FRESH' state. In practice FRESH and CLEAR are
// identitcal.
enum LazyState {
  FRESH,                  // Default c'tor, empty unparsed, uninitialized.
  CLEARED,                // Cleared w/o a previous message.
  CLEARED_EXPOSED,        // Cleared w a previous message.
  UNINITIALIZED,          // !unparsed_.empty(), message == nullptr.
  UNINITIALIZED_EXPOSED,  // !unparsed_.empty(), message != nullptr but was
                          // cleared.
  INITIALIZED,            // Message was parsed from unparsed_ (unparsed_ still
                          // contains the serialized data).
  DIRTY,                  // Message was parsed and then mutated
                          // (unparsed_ is cleared for now).

  // Keep as last.
  PARSING_ERROR
};
LazyState ToPublic(LazyState in) {
  return (in == FRESH || in == CLEARED_EXPOSED) ? CLEARED
         : in == UNINITIALIZED_EXPOSED          ? UNINITIALIZED
                                                : in;
}
absl::string_view ToString(LazyState state) {
  static auto* state_name = new absl::flat_hash_map<LazyState, std::string>({
      {FRESH, "FRESH"},
      {CLEARED, "CLEARED"},
      {CLEARED_EXPOSED, "CLEARED_EXPOSED"},
      {UNINITIALIZED, "UNINITIALIZED"},
      {UNINITIALIZED_EXPOSED, "UNINITIALIZED_EXPOSED"},
      {INITIALIZED, "INITIALIZED"},
      {DIRTY, "DIRTY"},
      {PARSING_ERROR, "PARSING_ERROR"},
  });
  return (*state_name)[state];
}

void SetAllFields(RepeatedPtrField<TestAllTypes>* message) {
  for (int i = 0; i < 4; i++) {
    TestUtil::SetAllFields(message->Add());
  }
}

enum class AllocType {
  kArena,
  kHeap,
};

}  // namespace

class LazyRepeatedPtrFieldTest
    : public testing::TestWithParam<std::tuple<LazyState, AllocType>> {
 protected:
  LazyRepeatedPtrFieldTest()
      : arena_(IsOnArena() ? new Arena() : nullptr),
        lazy_field_(CreateLazyField(arena_.get())),
        prototype_(&unittest::TestAllTypes::default_instance()) {}

  ArenaSafeUniquePtr<LazyRepeatedPtrField> CreateLazyField(Arena* arena) {
    return TestUtil::CreateCustomDtorArenaPtr<LazyRepeatedPtrField>(arena);
  }

  bool IsOnArena() const {
    return std::get<1>(GetParam()) == AllocType::kArena;
  }

  // Return the state configured by the test.
  LazyState GetConfiguredState() const { return std::get<0>(GetParam()); }

  LazyState GetConfiguredPublicState() const {
    return ToPublic(GetConfiguredState());
  }

  const RepeatedPtrField<TestAllTypes>& Get() const {
    return lazy_field_->Get<TestAllTypes>(prototype_, arena_.get());
  }

  RepeatedPtrField<TestAllTypes>* Mutable() {
    return lazy_field_->Mutable<TestAllTypes>(prototype_, arena_.get());
  }

  // Return the state associated with LazyRepeatedPtrField.
  static LazyState GetActualPublicState(const LazyRepeatedPtrField& field) {
    switch (field.GetLogicalState()) {
      case LazyRepeatedPtrField::LogicalState::kClear:
      case LazyRepeatedPtrField::LogicalState::kClearExposed:
        return CLEARED;
      case LazyRepeatedPtrField::LogicalState::kParseRequired:
        return UNINITIALIZED;
      case LazyRepeatedPtrField::LogicalState::kDirty:
        EXPECT_TRUE(field.IsAllocated());
        if (field.HasParsingError()) {
          return PARSING_ERROR;
        }
        return DIRTY;
      case LazyRepeatedPtrField::LogicalState::kNoParseRequired:
        return INITIALIZED;
    }
  }

  static RepeatedPtrField<unittest::TestAllTypes>* InitInState(
      LazyState state, void (*init_func)(RepeatedPtrField<TestAllTypes>* value),
      LazyRepeatedPtrField* field, Arena* arena) {
    unittest::TestEagerMessage source_container;
    RepeatedPtrField<unittest::TestAllTypes>& source =
        *source_container.mutable_repeated_sub_message();
    if (init_func) init_func(&source);
    auto create_message = [&]() {
      return Arena::CreateMessage<RepeatedPtrField<unittest::TestAllTypes>>(
          arena);
    };
    absl::Cord unparsed;
    source_container.SerializeToCord(&unparsed);
    RepeatedPtrField<unittest::TestAllTypes>* object = nullptr;
    switch (state) {
      case FRESH:
        field->OverwriteForTest<unittest::TestAllTypes>(
            LazyRepeatedPtrField::RawState::kCleared, absl::Cord(), nullptr, 0,
            arena);
        break;
      case INITIALIZED:
        object = create_message();
        object->CopyFrom(source);
        field->OverwriteForTest(LazyRepeatedPtrField::RawState::kIsParsed,
                                unparsed, object, source.size(), arena);
        break;
      case CLEARED:
        field->OverwriteForTest<unittest::TestAllTypes>(
            LazyRepeatedPtrField::RawState::kNeedsParse, unparsed, nullptr,
            source.size(), arena);
        field->Clear();
        break;
      case CLEARED_EXPOSED:
        object = create_message();
        object->CopyFrom(source);
        field->OverwriteForTest(LazyRepeatedPtrField::RawState::kIsParsed,
                                unparsed, object, source.size(), arena);
        field->Clear();
        break;
      case UNINITIALIZED_EXPOSED:
        object = create_message();
        ABSL_FALLTHROUGH_INTENDED;
      case UNINITIALIZED:
        field->OverwriteForTest<unittest::TestAllTypes>(
            LazyRepeatedPtrField::RawState::kNeedsParse, unparsed, object,
            source.size(), arena);
        break;
      case DIRTY:
        object = create_message();
        object->CopyFrom(source);
        field->OverwriteForTest(LazyRepeatedPtrField::RawState::kIsParsed,
                                absl::Cord(), object, source.size(), arena);
        break;
      case PARSING_ERROR:
        object = create_message();
        object->CopyFrom(source);
        field->OverwriteForTest(LazyRepeatedPtrField::RawState::kParseError,
                                unparsed, object, source.size(), arena);
        break;
    }

    // Fresh isn't in public API translate to cleared.
    EXPECT_EQ(ToPublic(state), GetActualPublicState(*field));
    return object;
  }

  void SetUp() override {
    object_ = InitInState(GetConfiguredState(), &SetAllFields,
                          lazy_field_.get(), arena_.get());
  }

  void CheckContent(const RepeatedPtrField<TestAllTypes>& value) {
    switch (GetConfiguredState()) {
      case FRESH:
      case CLEARED:
      case CLEARED_EXPOSED:
        EXPECT_EQ(value.size(), 0);
        break;
      case PARSING_ERROR: {
        TestAllTypes expected_element;
        TestUtil::SetAllFields(&expected_element);
        absl::Cord expected;
        expected_element.SerializeToCord(&expected);

        for (int i = 0; i < value.size(); i++) {
          absl::Cord actual;
          value.at(i).SerializeToCord(&actual);
          EXPECT_EQ(expected, actual);
        }
      } break;
      default:
        EXPECT_EQ(value.size(), 4);
        for (int i = 0; i < value.size(); i++) {
          TestUtil::ExpectAllFieldsSet(value.at(i));
        }
        break;
    }
  }

  bool IsDirty() const {
    return lazy_field_->GetLogicalState() ==
           LazyRepeatedPtrField::LogicalState::kDirty;
  }

  const absl::Cord& unparsed() const { return lazy_field_->unparsed_; }

  void FillLazyRepeatedPtrField(LazyRepeatedPtrField* field) {
    protobuf_unittest::TestAllTypes::NestedMessage message;
    message.set_bb(42);
    absl::Cord serialized;
    message.SerializeToCord(&serialized);
    field->MergeFromCord(prototype_, serialized, /*size=*/1, arena_.get());
  }

  std::unique_ptr<Arena> arena_;
  ArenaSafeUniquePtr<LazyRepeatedPtrField> lazy_field_;
  RepeatedPtrField<unittest::TestAllTypes>* object_;
  const unittest::TestAllTypes* prototype_;
};

INSTANTIATE_TEST_SUITE_P(
    LazyRepeatedPtrFieldTestInstance, LazyRepeatedPtrFieldTest,
    testing::Combine(testing::Values(FRESH, CLEARED, CLEARED_EXPOSED,
                                     UNINITIALIZED, UNINITIALIZED_EXPOSED,
                                     INITIALIZED, DIRTY, PARSING_ERROR),
                     testing::Values(AllocType::kArena, AllocType::kHeap)),
    [](const testing::TestParamInfo<LazyRepeatedPtrFieldTest::ParamType>&
           info) {
      std::string name = absl::StrCat(
          ToString(std::get<0>(info.param)), "_",
          std::get<1>(info.param) == AllocType::kArena ? "Arena" : "Heap");
      return name;
    });

TEST_P(LazyRepeatedPtrFieldTest, Get) {
  const auto& value = Get();
  CheckContent(value);
  LazyState expected = (GetConfiguredPublicState() == UNINITIALIZED)
                           ? INITIALIZED
                           : GetConfiguredPublicState();
  EXPECT_EQ(expected, GetActualPublicState(*lazy_field_));
}

TEST_P(LazyRepeatedPtrFieldTest, Mutable) {
  switch (GetConfiguredState()) {
    case FRESH:
    case CLEARED_EXPOSED:
    case CLEARED: {
      SetAllFields(Mutable());
      EXPECT_EQ(DIRTY, GetActualPublicState(*lazy_field_));
      ASSERT_EQ(lazy_field_->size(), 4);
      for (int i = 0; i < 4; i++) {
        TestUtil::ExpectAllFieldsSet(Get().at(i));
      }
      break;
    }
    // Here pointer stability can be checked.
    // add some new repeated-fields and expect the old fields + new on
    // the old pointer (object_).
    case DIRTY:
    case INITIALIZED:
    case PARSING_ERROR: {
      TestUtil::ModifyRepeatedFields(Mutable()->Mutable(0));
      EXPECT_EQ(DIRTY, GetActualPublicState(*lazy_field_));
      TestUtil::ExpectRepeatedFieldsModified(object_->at(0));
      break;
    }
    // This case is similar to above, use repeated fields to check old state
    // is preserved, but no pointer stability guarantee (e.g. don't use
    // object_).
    case UNINITIALIZED:
    case UNINITIALIZED_EXPOSED: {
      TestUtil::ModifyRepeatedFields(Mutable()->Mutable(0));
      EXPECT_EQ(DIRTY, GetActualPublicState(*lazy_field_));
      TestUtil::ExpectRepeatedFieldsModified(Get().at(0));
      break;
    }
  }
}

TEST_P(LazyRepeatedPtrFieldTest, Clear) {
  lazy_field_->Clear();
  EXPECT_EQ(CLEARED, GetActualPublicState(*lazy_field_));
  EXPECT_EQ(Get().size(), 0);
}

TEST_P(LazyRepeatedPtrFieldTest, Swap) {
  auto other = CreateLazyField(arena_.get());
  EXPECT_EQ(CLEARED, GetActualPublicState(*other));
  LazyRepeatedPtrField::Swap(lazy_field_.get(), arena_.get(), other.get(),
                             arena_.get());
  EXPECT_EQ(GetConfiguredPublicState(), GetActualPublicState(*other));
  if (GetConfiguredState() == FRESH) {
    EXPECT_EQ(0, other->Get<TestAllTypes>(prototype_, arena_.get()).size());
  } else {
#ifndef PROTOBUF_FORCE_COPY_IN_SWAP
    switch (GetConfiguredState()) {
        // Pointer stability of object is kept.
      case DIRTY:
      case INITIALIZED:
      case PARSING_ERROR:
        EXPECT_EQ(object_, &other->Get<TestAllTypes>(prototype_, arena_.get()));
        break;

        // No pointer stability guaranteed when Get triggers new allocation.
      case CLEARED:
      case CLEARED_EXPOSED:
      case FRESH:
      case UNINITIALIZED:
      case UNINITIALIZED_EXPOSED:
        break;
    }
#endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
  }
  CheckContent(other->Get<TestAllTypes>(prototype_, arena_.get()));

  EXPECT_EQ(CLEARED, GetActualPublicState(*lazy_field_));
  EXPECT_EQ(0, Get().size());
}

TEST_P(LazyRepeatedPtrFieldTest, SwapDifferentArenas) {
  Arena arena2;
  auto other = CreateLazyField(&arena2);
  EXPECT_EQ(CLEARED, GetActualPublicState(*other));

  LazyRepeatedPtrField::Swap(lazy_field_.get(), arena_.get(), other.get(),
                             &arena2);
  EXPECT_EQ(GetConfiguredPublicState(), GetActualPublicState(*other));
  CheckContent(other->Get<TestAllTypes>(prototype_, &arena2));

  EXPECT_EQ(CLEARED, GetActualPublicState(*lazy_field_));
  EXPECT_EQ(0, Get().size());
}

// -------------------------------------------------------------------------- //

class LazyRepeatedInMessageTest : public testing::TestWithParam<AllocType> {
 protected:
  LazyRepeatedInMessageTest()
      : arena_(IsOnArena() ? new Arena() : nullptr),
        msg_(MakeArenaSafeUnique<unittest::TestLazyMessage>(arena_.get())) {}

  bool IsOnArena() const { return GetParam() == AllocType::kArena; }

  const LazyRepeatedPtrField& GetField() {
    return GetRawForTest<LazyRepeatedPtrField>(
        *msg_, msg_->GetDescriptor()->FindFieldByName("repeated_sub_message"));
  }

  std::unique_ptr<Arena> arena_;
  ArenaSafeUniquePtr<unittest::TestLazyMessage> msg_;
};

INSTANTIATE_TEST_SUITE_P(LazyRepeatedInMessageTestInstance,
                         LazyRepeatedInMessageTest,
                         testing::Values(AllocType::kArena, AllocType::kHeap));

TEST_P(LazyRepeatedInMessageTest, Construct) {
  EXPECT_EQ(GetField().GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kClear);
}

TEST_P(LazyRepeatedInMessageTest, GetEmpty) {
  const auto& value = msg_->repeated_sub_message();
  EXPECT_TRUE(value.empty());
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kClearExposed);
}

TEST_P(LazyRepeatedInMessageTest, MutableEmpty) {
  auto& value = *msg_->mutable_repeated_sub_message();
  EXPECT_TRUE(value.empty());
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kDirty);
}

TEST_P(LazyRepeatedInMessageTest, Mutable) {
  auto& value = *msg_->mutable_repeated_sub_message();
  auto* element = value.Add();
  TestUtil::SetAllFields(element);
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kDirty);
  EXPECT_EQ(msg_->repeated_sub_message_size(), 1);
  EXPECT_EQ(field.size(), 1);
  TestUtil::ExpectAllFieldsSet(*element);
}

TEST_P(LazyRepeatedInMessageTest, InternalSerialize) {
  unittest::TestEagerMessage truth;
  TestUtil::SetAllFields(truth.add_repeated_sub_message());
  TestUtil::SetAllFields(msg_->add_repeated_sub_message());
  EXPECT_EQ(msg_->SerializeAsString(), truth.SerializeAsString());
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kDirty);
}

TEST_P(LazyRepeatedInMessageTest, InternalParse) {
  unittest::TestEagerMessage truth;
  truth.add_repeated_sub_message()->set_optional_uint32(100);
  truth.add_repeated_sub_message()->set_optional_uint32(100);
  truth.add_repeated_sub_message()->add_repeated_uint32(100);
  truth.add_repeated_sub_message()->set_oneof_uint32(100);
  const std::string str = truth.SerializeAsString();
  unittest::TestEagerMessage truth_dst;
  ASSERT_TRUE(msg_->MergeFromString(str));
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kParseRequired);
  EXPECT_EQ(msg_->repeated_sub_message_size(), 4);
  EXPECT_EQ(field.size(), 4);
  // Calling size() doesn't trigger parsing.
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kParseRequired);
  EXPECT_EQ(msg_->repeated_sub_message(3).oneof_uint32(), 100);
  EXPECT_EQ(msg_->repeated_sub_message(2).repeated_uint32(0), 100);
  EXPECT_EQ(msg_->repeated_sub_message(1).optional_uint32(), 100);
  EXPECT_EQ(msg_->repeated_sub_message(0).optional_uint32(), 100);
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kNoParseRequired);
}

TEST_P(LazyRepeatedInMessageTest, InternalParseThenMutable) {
  unittest::TestEagerMessage truth;
  TestUtil::SetAllFields(truth.add_repeated_sub_message());
  const std::string str = truth.SerializeAsString();
  ASSERT_TRUE(msg_->MergeFromString(str));
  ASSERT_EQ(msg_->repeated_sub_message_size(), 1);
  TestUtil::ExpectAllFieldsSet(msg_->repeated_sub_message(0));
  TestUtil::ModifyRepeatedFields(msg_->mutable_repeated_sub_message(0));
  TestUtil::ExpectRepeatedFieldsModified(msg_->repeated_sub_message(0));
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kDirty);
}

TEST_P(LazyRepeatedInMessageTest, InternalParseThenAdd) {
  unittest::TestEagerMessage truth;
  TestUtil::SetOptionalFields(truth.add_repeated_sub_message());
  TestUtil::SetOneofFields(truth.add_repeated_sub_message());
  const std::string str = truth.SerializeAsString();
  ASSERT_TRUE(msg_->MergeFromString(str));
  ASSERT_EQ(msg_->repeated_sub_message_size(), 2);
  TestUtil::SetAllFields(msg_->add_repeated_sub_message());
  ASSERT_EQ(msg_->repeated_sub_message_size(), 3);
  TestUtil::ExpectAllFieldsSet(msg_->repeated_sub_message(2));
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kDirty);
}

TEST_P(LazyRepeatedInMessageTest, InternalParseThenSerialize) {
  unittest::TestEagerMessage truth;
  TestUtil::SetAllFields(truth.add_repeated_sub_message());
  TestUtil::SetOptionalFields(truth.add_repeated_sub_message());
  const std::string str = truth.SerializeAsString();
  ASSERT_TRUE(msg_->MergeFromString(str));
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kParseRequired);
  const std::string actual_str = msg_->SerializeAsString();
  EXPECT_EQ(actual_str, str);
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kParseRequired);
}

TEST_P(LazyRepeatedInMessageTest, InternalParseThenGetAndSerialize) {
  unittest::TestEagerMessage truth;
  TestUtil::SetAllFields(truth.add_repeated_sub_message());
  const std::string str = truth.SerializeAsString();
  ASSERT_TRUE(msg_->MergeFromString(str));
  TestUtil::ExpectAllFieldsSet(msg_->repeated_sub_message(0));
  const auto& field = GetField();
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kNoParseRequired);
  const std::string actual_str = msg_->SerializeAsString();
  EXPECT_EQ(actual_str, str);
  EXPECT_EQ(field.GetLogicalState(),
            LazyRepeatedPtrField::LogicalState::kNoParseRequired);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "net/proto2/public/port_undef.inc"
