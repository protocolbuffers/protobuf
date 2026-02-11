#include "google/protobuf/field_with_arena.h"

#include <string>
#include <utility>

#include <gtest/gtest.h>
#include "google/protobuf/arena.h"
#include "google/protobuf/internal_metadata_locator.h"

namespace google {
namespace protobuf {
namespace internal {

struct TestType {
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;

  int value;
  InternalMetadataResolver resolver;

  explicit TestType(int value) : value(value) {}
  TestType(InternalMetadataOffset offset, int value)
      : value(value), resolver(offset) {}

  Arena* GetArena() const { return ResolveArena<&TestType::resolver>(this); }
};

// Specialize `FieldArenaRep` so `Arena::Create` will construct a
// `FieldWithArena<TestType>` instead of a `TestType` when allocating `TestType`
// on an arena.
template <>
struct FieldArenaRep<TestType> {
  using Type = FieldWithArena<TestType>;

  static inline TestType* Get(Type* arena_rep) { return &arena_rep->field(); }
};

struct TestTypeNotDestructorSkippable {
  using InternalArenaConstructable_ = void;

  std::string value;
  InternalMetadataResolver resolver;

  explicit TestTypeNotDestructorSkippable(std::string value)
      : value(std::move(value)) {}
  TestTypeNotDestructorSkippable(InternalMetadataOffset offset,
                                 std::string value)
      : value(std::move(value)), resolver(offset) {}

  Arena* GetArena() const {
    return ResolveArena<&TestTypeNotDestructorSkippable::resolver>(this);
  }
};

// Specialize `TestTypeNotDestructorSkippable` so `Arena::Create` will construct
// a `FieldWithArena<TestTypeNotDestructorSkippable>` when allocating on an
// arena.
template <>
struct FieldArenaRep<TestTypeNotDestructorSkippable> {
  using Type = FieldWithArena<TestTypeNotDestructorSkippable>;

  static inline TestTypeNotDestructorSkippable* Get(Type* arena_rep) {
    return &arena_rep->field();
  }
};

namespace {

TEST(FieldWithArenaTest, NoArena) {
  auto* field = Arena::Create<TestType>(/*arena=*/nullptr, 10);

  EXPECT_EQ(field->value, 10);
  EXPECT_EQ(field->GetArena(), nullptr);

  delete field;
}

TEST(FieldWithArenaTest, WithArena) {
  google::protobuf::Arena arena;
  auto* field = Arena::Create<TestType>(&arena, 10);

  EXPECT_EQ(field->value, 10);
  EXPECT_EQ(field->GetArena(), &arena);

  // `field` should have been allocated as a `FieldWithArena<TestType>`.
  // Protobuf internal code should not rely on this fact outside of this test,
  // and we should never need to cast up to the containing type. We only do this
  // here to verify that `Arena::Create` behaved as expected.
  auto& field_with_arena = *reinterpret_cast<FieldWithArena<TestType>*>(field);
  EXPECT_EQ(field_with_arena.GetArena(), &arena);
}

TEST(FieldWithArenaTest, NoArenaWithDestructor) {
  auto* field = Arena::Create<TestTypeNotDestructorSkippable>(
      /*arena=*/nullptr, "Long string to force heap allocation");

  EXPECT_EQ(field->value, "Long string to force heap allocation");
  EXPECT_EQ(field->GetArena(), nullptr);

  delete field;
}

TEST(FieldWithArenaTest, WithArenaWithDestructor) {
  google::protobuf::Arena arena;
  auto* field = Arena::Create<TestTypeNotDestructorSkippable>(
      &arena, "Long string to force heap allocation");

  EXPECT_EQ(field->value, "Long string to force heap allocation");
  EXPECT_EQ(field->GetArena(), &arena);

  // `field` should have been allocated as a
  // `FieldWithArena<TestTypeNotDestructorSkippable>`. Protobuf internal code
  // should not rely on this fact outside of this test, and we should never need
  // to cast up to the containing type. We only do this here to verify that
  // `Arena::Create` behaved as expected.
  auto& field_with_arena =
      *reinterpret_cast<FieldWithArena<TestTypeNotDestructorSkippable>*>(field);
  EXPECT_EQ(field_with_arena.GetArena(), &arena);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
