#include "google/protobuf/lazy_repeated_field.h"

#include <time.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/casts.h"
#include "net/proto2/arena/arena_safe_unique_ptr.h"
#include "google/protobuf/message.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/cord.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor_lite.h"
#include "google/protobuf/internal_visibility_for_testing.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"
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
using ::google::protobuf::internal::ParseContext;
using ::google::protobuf::internal::UnalignedLoad;
using ::google::protobuf::internal::WireFormat;
using ::google::protobuf::internal::WireFormatLite;
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

void SetMergeTargetFields(RepeatedPtrField<TestAllTypes>* rpt, size_t size) {
  for (size_t i = 0; i < size; i++) {
    auto* message = rpt->Add();
    TestUtil::SetOptionalFields(message);
    TestUtil::AddRepeatedFields1(message);
  }
}

void SetMergeSourceFields(RepeatedPtrField<TestAllTypes>* rpt, size_t size) {
  for (size_t i = 0; i < size; i++) {
    auto* message = rpt->Add();
    TestUtil::AddRepeatedFields2(message);
    TestUtil::SetDefaultFields(message);
    TestUtil::SetOneofFields(message);
  }
}

void SetAllFields(RepeatedPtrField<TestAllTypes>* rpt, size_t size) {
  for (size_t i = 0; i < size; i++) {
    TestUtil::SetAllFields(rpt->Add());
  }
}

void ExpectAllFieldsSet(const RepeatedPtrField<TestAllTypes>& message,
                        size_t size) {
  for (size_t i = 0; i < size; i++) {
    TestUtil::ExpectAllFieldsSet(message.Get(i));
  }
}

enum class AllocType {
  kArena,
  kHeap,
};

ArenaSafeUniquePtr<LazyRepeatedPtrField> CreateLazyField(Arena* arena) {
  return google::protobuf::MakeArenaSafeUnique<LazyRepeatedPtrField>(arena);
}

}  // namespace

class LazyRepeatedPtrFieldTestBase : public testing::Test {
 public:
  explicit LazyRepeatedPtrFieldTestBase(bool is_on_arena)
      : arena_(is_on_arena ? new Arena() : nullptr),
        lazy_field_(CreateLazyField(arena_.get())),
        prototype_(&unittest::TestAllTypes::default_instance()) {}

  const RepeatedPtrField<TestAllTypes>& Get() const {
    return lazy_field_->Get(prototype_, arena_.get());
  }

  RepeatedPtrField<TestAllTypes>* Mutable() {
    return lazy_field_->Mutable(prototype_, arena_.get());
  }

  std::unique_ptr<Arena> arena_;
  ArenaSafeUniquePtr<LazyRepeatedPtrField> lazy_field_;
  const unittest::TestAllTypes* prototype_;
};

class LazyRepeatedPtrFieldTest : public testing::WithParamInterface<
                                     std::tuple<LazyState, AllocType, size_t>>,
                                 public LazyRepeatedPtrFieldTestBase {
 public:
  LazyRepeatedPtrFieldTest() : LazyRepeatedPtrFieldTestBase(IsOnArena()) {}

  bool IsOnArena() const {
    return std::get<1>(GetParam()) == AllocType::kArena;
  }

  size_t GetConfiguredSize() const { return std::get<2>(GetParam()); }

  // Return the state configured by the test.
  LazyState GetConfiguredState() const { return std::get<0>(GetParam()); }

  LazyState GetConfiguredPublicState() const {
    return ToPublic(GetConfiguredState());
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

  static std::pair<RepeatedPtrField<unittest::TestAllTypes>*, size_t>
  InitInState(LazyState state,
              void (*init_func)(RepeatedPtrField<TestAllTypes>* value,
                                size_t size),
              LazyRepeatedPtrField* field, size_t size, Arena* arena) {
    unittest::TestEagerMessage source_container;
    RepeatedPtrField<unittest::TestAllTypes>& source =
        *source_container.mutable_repeated_sub_message();
    if (init_func) init_func(&source, size);
    auto create_message = [&]() {
      return Arena::CreateMessage<RepeatedPtrField<unittest::TestAllTypes>>(
          arena);
    };
    absl::Cord unparsed;
    source_container.SerializeToCord(&unparsed);
    LOG(ERROR) << unparsed.size();
    RepeatedPtrField<unittest::TestAllTypes>* object = nullptr;
    size_t actual_size = size;
    switch (state) {
      case FRESH:
        field->OverwriteForTest<unittest::TestAllTypes>(
            LazyRepeatedPtrField::RawState::kCleared, absl::Cord(), nullptr,
            arena);
        actual_size = 0;
        break;
      case INITIALIZED:
        object = create_message();
        object->CopyFrom(source);
        field->OverwriteForTest(LazyRepeatedPtrField::RawState::kIsParsed,
                                unparsed, object, arena);
        break;
      case CLEARED:
        field->OverwriteForTest<unittest::TestAllTypes>(
            LazyRepeatedPtrField::RawState::kNeedsParse, unparsed, nullptr,
            arena);
        field->Clear();
        actual_size = 0;
        break;
      case CLEARED_EXPOSED:
        object = create_message();
        object->CopyFrom(source);
        field->OverwriteForTest(LazyRepeatedPtrField::RawState::kIsParsed,
                                unparsed, object, arena);
        field->Clear();
        actual_size = 0;
        break;
      case UNINITIALIZED_EXPOSED:
        object = create_message();
        ABSL_FALLTHROUGH_INTENDED;
      case UNINITIALIZED:
        field->OverwriteForTest<unittest::TestAllTypes>(
            LazyRepeatedPtrField::RawState::kNeedsParse, unparsed, object,
            arena);
        break;
      case DIRTY:
        object = create_message();
        object->CopyFrom(source);
        field->OverwriteForTest(LazyRepeatedPtrField::RawState::kIsParsed,
                                absl::Cord(), object, arena);
        break;
      case PARSING_ERROR:
        object = create_message();
        object->CopyFrom(source);
        field->OverwriteForTest(LazyRepeatedPtrField::RawState::kParseError,
                                unparsed, object, arena);
        break;
    }

    // Fresh isn't in public API translate to cleared.
    EXPECT_EQ(ToPublic(state), GetActualPublicState(*field));
    return {object, actual_size};
  }

  void SetUp() override {
    object_ = InitInState(GetConfiguredState(), &SetAllFields,
                          lazy_field_.get(), GetConfiguredSize(), arena_.get())
                  .first;
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
        EXPECT_EQ(value.size(), GetConfiguredSize());
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

  void FillLazyRepeatedPtrField(LazyRepeatedPtrField* field, Arena* arena) {
    protobuf_unittest::TestLazyMessage message;
    message.add_repeated_sub_message()->set_optional_int32(42);
    absl::Cord serialized;
    message.SerializeToCord(&serialized);
    field->MergeFrom(prototype_, serialized, arena);
  }

  RepeatedPtrField<unittest::TestAllTypes>* object_;
};

INSTANTIATE_TEST_SUITE_P(
    LazyRepeatedPtrFieldTestInstance, LazyRepeatedPtrFieldTest,
    testing::Combine(testing::Values(FRESH, CLEARED, CLEARED_EXPOSED,
                                     UNINITIALIZED, UNINITIALIZED_EXPOSED,
                                     INITIALIZED, DIRTY, PARSING_ERROR),
                     testing::Values(AllocType::kArena, AllocType::kHeap),
                     testing::Values(1, 4)),
    [](const testing::TestParamInfo<LazyRepeatedPtrFieldTest::ParamType>&
           info) {
      std::string name = absl::StrCat(
          ToString(std::get<0>(info.param)), "_",
          std::get<1>(info.param) == AllocType::kArena ? "Arena" : "Heap", "_",
          std::get<2>(info.param));
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
      SetAllFields(Mutable(), GetConfiguredSize());
      EXPECT_EQ(DIRTY, GetActualPublicState(*lazy_field_));
      ASSERT_EQ(Get().size(), GetConfiguredSize());
      for (size_t i = 0; i < GetConfiguredSize(); i++) {
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

TEST_P(LazyRepeatedPtrFieldTest, GetDynamic) {
  CheckContent(*reinterpret_cast<const RepeatedPtrField<TestAllTypes>*>(
      lazy_field_->GetDynamic(unittest::TestAllTypes::descriptor(),
                              MessageFactory::generated_factory(),
                              arena_.get())));
  LazyState expected = (GetConfiguredPublicState() == UNINITIALIZED)
                           ? INITIALIZED
                           : GetConfiguredPublicState();
  EXPECT_EQ(expected, GetActualPublicState(*lazy_field_));
}

TEST_P(LazyRepeatedPtrFieldTest, MutableDynamic) {
  auto mutable_dynamic = [this]() {
    return reinterpret_cast<RepeatedPtrField<TestAllTypes>*>(
        lazy_field_->MutableDynamic(unittest::TestAllTypes::descriptor(),
                                    MessageFactory::generated_factory(),
                                    arena_.get()));
  };
  switch (GetConfiguredState()) {
    // Clear and fresh should have a brand new object; can use TestAllFields.
    case FRESH:
    case CLEARED_EXPOSED:
    case CLEARED: {
      SetAllFields(mutable_dynamic(), GetConfiguredSize());
      EXPECT_EQ(DIRTY, GetActualPublicState(*lazy_field_));
      ExpectAllFieldsSet(Get(), GetConfiguredSize());
      break;
    }

    // Here pointer stability can be checked.
    // add some new repeated-fields and expect the old fields + new on
    // the old pointer (object_).
    case DIRTY:
    case INITIALIZED:
    case PARSING_ERROR: {
      TestUtil::ModifyRepeatedFields(mutable_dynamic()->Mutable(0));
      EXPECT_EQ(DIRTY, GetActualPublicState(*lazy_field_));
      TestUtil::ExpectRepeatedFieldsModified(object_->at(0));
      break;
    }

    // This case is similar to above, use repeated fields to check old state
    // is preserved, but no pointer stability guarantee (e.g. don't use
    // object_).
    case UNINITIALIZED:
    case UNINITIALIZED_EXPOSED: {
      TestUtil::ModifyRepeatedFields(mutable_dynamic()->Mutable(0));
      EXPECT_EQ(DIRTY, GetActualPublicState(*lazy_field_));
      TestUtil::ExpectRepeatedFieldsModified(Get().at(0));
      break;
    }
  }
}

TEST_P(LazyRepeatedPtrFieldTest, Clear) {
  lazy_field_->Clear();
  EXPECT_EQ(GetActualPublicState(*lazy_field_), CLEARED);
  EXPECT_EQ(Get().size(), 0);
}

TEST_P(LazyRepeatedPtrFieldTest, Size) {
  int expected;
  switch (GetConfiguredState()) {
    case FRESH:
    case CLEARED:
    case CLEARED_EXPOSED:
      expected = 0;
      break;
    case INITIALIZED:
    case UNINITIALIZED_EXPOSED:
    case UNINITIALIZED:
    case PARSING_ERROR:
    case DIRTY:
      expected = GetConfiguredSize();
      break;
  }
  EXPECT_EQ(expected, Get().size());
}

TEST_P(LazyRepeatedPtrFieldTest, MergeFromStateExpectations) {
  if (std::get<0>(GetParam()) != LazyState::FRESH) {
    // This test is param agnostic this is just to have it run exactly once.
    return;
  }
  struct Answer {
    LazyState src;
    LazyState dest;
    LazyState expected;
  };
  using LS = LazyState;
  Answer answers[] = {
      // Note that 'FRESH" is actually cleared internally with a null object.
      {LS::FRESH, LS::FRESH, LS::FRESH},
      {LS::FRESH, LS::CLEARED, LS::CLEARED},
      {LS::FRESH, LS::CLEARED_EXPOSED, LS::CLEARED},
      {LS::FRESH, LS::UNINITIALIZED, LS::UNINITIALIZED},
      {LS::FRESH, LS::UNINITIALIZED_EXPOSED, LS::UNINITIALIZED},
      {LS::FRESH, LS::INITIALIZED, LS::INITIALIZED},
      {LS::FRESH, LS::DIRTY, LS::DIRTY},
      {LS::FRESH, LS::PARSING_ERROR, LS::PARSING_ERROR},

      {LS::CLEARED, LS::FRESH, LS::FRESH},
      {LS::CLEARED, LS::CLEARED, LS::CLEARED},
      {LS::CLEARED, LS::CLEARED_EXPOSED, LS::CLEARED},
      {LS::CLEARED, LS::UNINITIALIZED, LS::UNINITIALIZED},
      {LS::CLEARED, LS::UNINITIALIZED_EXPOSED, LS::UNINITIALIZED},
      {LS::CLEARED, LS::INITIALIZED, LS::INITIALIZED},
      {LS::CLEARED, LS::DIRTY, LS::DIRTY},
      {LS::CLEARED, LS::PARSING_ERROR, LS::PARSING_ERROR},

      {LS::CLEARED_EXPOSED, LS::FRESH, LS::FRESH},
      {LS::CLEARED_EXPOSED, LS::CLEARED, LS::CLEARED},
      {LS::CLEARED_EXPOSED, LS::CLEARED_EXPOSED, LS::CLEARED},
      {LS::CLEARED_EXPOSED, LS::UNINITIALIZED, LS::UNINITIALIZED},
      {LS::CLEARED_EXPOSED, LS::UNINITIALIZED_EXPOSED,
       LS::UNINITIALIZED_EXPOSED},
      {LS::CLEARED_EXPOSED, LS::INITIALIZED, LS::INITIALIZED},
      {LS::CLEARED_EXPOSED, LS::DIRTY, LS::DIRTY},
      {LS::CLEARED_EXPOSED, LS::PARSING_ERROR, LS::PARSING_ERROR},

      {LS::UNINITIALIZED, LS::FRESH, LS::UNINITIALIZED},
      {LS::UNINITIALIZED, LS::CLEARED, LS::UNINITIALIZED},
      {LS::UNINITIALIZED, LS::CLEARED_EXPOSED, LS::DIRTY},
      {LS::UNINITIALIZED, LS::UNINITIALIZED, LS::UNINITIALIZED},
      {LS::UNINITIALIZED, LS::UNINITIALIZED_EXPOSED, LS::UNINITIALIZED},
      {LS::UNINITIALIZED, LS::INITIALIZED, LS::DIRTY},
      {LS::UNINITIALIZED, LS::DIRTY, LS::DIRTY},
      {LS::UNINITIALIZED, LS::PARSING_ERROR, LS::DIRTY},

      {LS::UNINITIALIZED_EXPOSED, LS::FRESH, LS::UNINITIALIZED_EXPOSED},
      {LS::UNINITIALIZED_EXPOSED, LS::CLEARED, LS::UNINITIALIZED_EXPOSED},
      {LS::UNINITIALIZED_EXPOSED, LS::CLEARED_EXPOSED, LS::DIRTY},
      {LS::UNINITIALIZED_EXPOSED, LS::UNINITIALIZED, LS::UNINITIALIZED_EXPOSED},
      {LS::UNINITIALIZED_EXPOSED, LS::UNINITIALIZED_EXPOSED,
       LS::UNINITIALIZED_EXPOSED},
      {LS::UNINITIALIZED_EXPOSED, LS::INITIALIZED, LS::DIRTY},
      {LS::UNINITIALIZED_EXPOSED, LS::DIRTY, LS::DIRTY},
      {LS::UNINITIALIZED_EXPOSED, LS::PARSING_ERROR, LS::DIRTY},

      {LS::INITIALIZED, LS::FRESH, LS::UNINITIALIZED},
      {LS::INITIALIZED, LS::CLEARED, LS::UNINITIALIZED},
      {LS::INITIALIZED, LS::CLEARED_EXPOSED, LS::DIRTY},
      {LS::INITIALIZED, LS::UNINITIALIZED, LS::UNINITIALIZED},
      {LS::INITIALIZED, LS::UNINITIALIZED_EXPOSED, LS::UNINITIALIZED},
      {LS::INITIALIZED, LS::INITIALIZED, LS::DIRTY},
      {LS::INITIALIZED, LS::DIRTY, LS::DIRTY},
      {LS::INITIALIZED, LS::PARSING_ERROR, LS::DIRTY},

      {LS::DIRTY, LS::FRESH, LS::DIRTY},
      {LS::DIRTY, LS::CLEARED, LS::DIRTY},
      {LS::DIRTY, LS::CLEARED_EXPOSED, LS::DIRTY},
      {LS::DIRTY, LS::UNINITIALIZED, LS::DIRTY},
      {LS::DIRTY, LS::UNINITIALIZED_EXPOSED, LS::DIRTY},
      {LS::DIRTY, LS::INITIALIZED, LS::DIRTY},
      {LS::DIRTY, LS::DIRTY, LS::DIRTY},
      {LS::DIRTY, LS::PARSING_ERROR, LS::DIRTY},

      {LS::PARSING_ERROR, LS::FRESH, LS::DIRTY},
      {LS::PARSING_ERROR, LS::CLEARED, LS::DIRTY},
      {LS::PARSING_ERROR, LS::CLEARED_EXPOSED, LS::DIRTY},
      {LS::PARSING_ERROR, LS::UNINITIALIZED, LS::DIRTY},
      {LS::PARSING_ERROR, LS::UNINITIALIZED_EXPOSED, LS::DIRTY},
      {LS::PARSING_ERROR, LS::INITIALIZED, LS::DIRTY},
      {LS::PARSING_ERROR, LS::DIRTY, LS::DIRTY},
      {LS::PARSING_ERROR, LS::PARSING_ERROR, LS::DIRTY},
  };

  static_assert(7 == PARSING_ERROR,
                "A new value was added to LazyState update this.");
  EXPECT_EQ(8 * 8, std::size(answers));

  auto* arena = arena_.get();
  for (const auto& answer : answers) {
    auto source = CreateLazyField(arena);
    auto dest = CreateLazyField(arena);
    InitInState(answer.src, &SetMergeSourceFields, source.get(),
                GetConfiguredSize(), arena);
    InitInState(answer.dest, &SetMergeTargetFields, dest.get(),
                GetConfiguredSize(), arena);

    dest->MergeFrom(prototype_, *source, arena, arena);
    EXPECT_EQ(ToPublic(answer.expected), GetActualPublicState(*dest))
        << "Expression dest.MergeFrom(source) != expected with\n"
        << "\tsource: " << answer.src << "\n"
        << "\tdest: " << answer.dest << "\n"
        << "\texpected: " << answer.expected;
  }
}

TEST_P(LazyRepeatedPtrFieldTest, CopyConstruct) {
  LazyRepeatedPtrField empty;
  LazyState empty_state = GetActualPublicState(empty);

  auto visibility = internal::InternalVisibilityForTesting{};
  LazyState state = std::get<0>(GetParam());
  bool use_arena = std::get<1>(GetParam()) == AllocType::kArena;

  // Create source without using an arena: the copy constructor is agnostic
  // to the source being arena allocated or not: it simply copies its values.
  auto source = CreateLazyField(nullptr);
  InitInState(state, &SetMergeSourceFields, source.get(), GetConfiguredSize(),
              nullptr);

  Arena* arena = use_arena ? arena_.get() : nullptr;
  const auto& prototype = unittest::TestAllTypes::default_instance();

  LazyRepeatedPtrField* dest;
  if (arena != nullptr) {
    auto* mem = arena->AllocateAligned(sizeof(LazyRepeatedPtrField));
    dest = new (mem) LazyRepeatedPtrField(visibility, arena, *source, arena);
  } else {
    dest = new LazyRepeatedPtrField(*source);
  }

  // If the source is either empty or cleared, the copy is in state empty.
  if (ToPublic(state) == LazyState::CLEARED) {
    EXPECT_EQ(GetActualPublicState(*dest), empty_state);
  } else if (state == LazyState::DIRTY) {
    EXPECT_EQ(GetActualPublicState(*dest), LazyState::DIRTY);
  } else if (state == LazyState::PARSING_ERROR) {
    EXPECT_EQ(GetActualPublicState(*dest), LazyState::PARSING_ERROR);
  } else {
    EXPECT_EQ(GetActualPublicState(*dest), LazyState::UNINITIALIZED);
  }
  const auto& dest_value = dest->Get(&prototype, arena);
  const auto& source_value = source->Get(&prototype, nullptr);
  EXPECT_EQ(dest_value.size(), source_value.size());
  for (int i = 0; i < dest_value.size(); i++) {
    EXPECT_THAT(dest_value.Get(i), EqualsProto(source_value.Get(i)));
  }

  if (arena == nullptr) {
    delete dest;
  }
}

TEST_P(LazyRepeatedPtrFieldTest, MergeFromClean) {
  for (LazyState src_state :
       {LazyState::FRESH, LazyState::CLEARED, LazyState::CLEARED_EXPOSED}) {
    auto src = CreateLazyField(arena_.get());
    InitInState(src_state, &SetAllFields, src.get(), 0, arena_.get());
    auto dest = CreateLazyField(arena_.get());
    InitInState(GetConfiguredState(), &SetAllFields, dest.get(),
                GetConfiguredSize(), arena_.get());
    dest->MergeFrom(prototype_, *src, arena_.get(), arena_.get());
    EXPECT_EQ(GetConfiguredPublicState(), GetActualPublicState(*dest));
    CheckContent(dest->Get(prototype_, arena_.get()));
  }
}

TEST_P(LazyRepeatedPtrFieldTest, MergeFromUnparsed) {
  for (LazyState src_state :
       {LazyState::UNINITIALIZED, LazyState::INITIALIZED}) {
    auto src = CreateLazyField(arena_.get());
    const size_t src_size = 2;
    InitInState(src_state, &SetAllFields, src.get(), src_size, arena_.get());
    auto dest = CreateLazyField(arena_.get());
    const auto dest_size =
        InitInState(GetConfiguredState(), &SetAllFields, dest.get(),
                    GetConfiguredSize(), arena_.get())
            .second;
    dest->MergeFrom(prototype_, *src, arena_.get(), arena_.get());

    LazyState expected;
    switch (GetConfiguredState()) {
      // If pointers were exposed msg stays dirty.
      case INITIALIZED:
      case DIRTY:
      case CLEARED_EXPOSED:
      case PARSING_ERROR:
        expected = DIRTY;
        break;

      // Otherwise msg is uninitialized.
      case UNINITIALIZED:
      case UNINITIALIZED_EXPOSED:
      case CLEARED:
      case FRESH:
        expected = UNINITIALIZED;
        break;
    }

    EXPECT_EQ(expected, GetActualPublicState(*dest));
    const auto& merged = dest->Get(prototype_, arena_.get());
    EXPECT_EQ(merged.size(), dest_size + src_size);
    for (int i = 0; i < merged.size(); i++) {
      TestUtil::ExpectAllFieldsSet(merged.Get(i));
    }
  }
}

TEST_P(LazyRepeatedPtrFieldTest, MergeFromDirty) {
  auto src = CreateLazyField(arena_.get());
  const size_t src_size = 3;
  InitInState(DIRTY, &SetAllFields, src.get(), src_size, arena_.get());
  auto dest = CreateLazyField(arena_.get());
  const auto dest_size =
      InitInState(GetConfiguredState(), &SetAllFields, dest.get(),
                  GetConfiguredSize(), arena_.get())
          .second;
  dest->MergeFrom(prototype_, *src, arena_.get(), arena_.get());
  EXPECT_EQ(DIRTY, GetActualPublicState(*dest));
  const auto& merged = dest->Get(prototype_, arena_.get());
  EXPECT_EQ(merged.size(), dest_size + src_size);
  for (int i = 0; i < merged.size(); i++) {
    TestUtil::ExpectAllFieldsSet(merged.Get(i));
  }
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

TEST_P(LazyRepeatedPtrFieldTest, ArenaSwap) {
  using LRF = LazyRepeatedPtrField;
  // Swap when both LazyFields are on the same arena.
  {
    Arena arena1;
    Arena arena2;
    LRF* arena1_allocated1 = CreateLazyField(&arena1).get();
    LRF* arena1_allocated2 = CreateLazyField(&arena1).get();
    LRF* arena2_allocated1 = CreateLazyField(&arena2).get();
    std::unique_ptr<LRF> heap_allocated(new LRF(nullptr));
    // All four combinations that we need to test.
    LRF* fields[4][2] = {{arena1_allocated1, arena1_allocated2},
                         {arena1_allocated1, arena2_allocated1},
                         {arena1_allocated1, heap_allocated.get()},
                         {heap_allocated.get(), arena1_allocated1}};
    // Arenas used for the combinations above.
    Arena* field_arenas[4][2] = {{&arena1, &arena1},
                                 {&arena1, &arena2},
                                 {&arena1, nullptr},
                                 {nullptr, &arena1}};

    for (int i = 2; i < 3; ++i) {
      LRF* field1 = fields[i][0];
      LRF* field2 = fields[i][1];
      Arena* field1_arena = field_arenas[i][0];
      Arena* field2_arena = field_arenas[i][1];
      // Clear the fields1;
      InitInState(FRESH, nullptr, field1, GetConfiguredSize(), field1_arena);
      InitInState(FRESH, nullptr, field2, GetConfiguredSize(), field2_arena);
      FillLazyRepeatedPtrField(field1, field1_arena);
      EXPECT_TRUE(nullptr == field1->TryGetMessage());
      EXPECT_TRUE(field1->HasUnparsed());
      // Parse field into message.
      field1->Mutable(prototype_, field1_arena);
      // Swap with field2;
      LRF::Swap(field1, field1_arena, field2, field2_arena);
      EXPECT_TRUE(nullptr == field1->TryGetMessage());
      EXPECT_FALSE(field1->HasUnparsed());
      EXPECT_TRUE(nullptr != field2->TryGetMessage());
      EXPECT_FALSE(field2->HasUnparsed());
      EXPECT_EQ(42,
                field2->Get(prototype_, field2_arena).Get(0).optional_int32());
    }
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "net/proto2/public/port_undef.inc"
