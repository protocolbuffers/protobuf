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
  TestType(InternalMetadataOffset offset) : value(), resolver(offset) {}
  TestType(InternalMetadataOffset offset, int value)
      : value(value), resolver(offset) {}

  Arena* GetArena() const { return ResolveArena<&TestType::resolver>(this); }
};

struct TestTypeNotDestructorSkippable {
  using InternalArenaConstructable_ = void;

  std::string value;
  InternalMetadataResolver resolver;

  explicit TestTypeNotDestructorSkippable(std::string value)
      : value(std::move(value)) {}
  TestTypeNotDestructorSkippable(InternalMetadataOffset offset)
      : value(), resolver(offset) {}
  TestTypeNotDestructorSkippable(InternalMetadataOffset offset,
                                 std::string value)
      : value(std::move(value)), resolver(offset) {}

  Arena* GetArena() const {
    return ResolveArena<&TestTypeNotDestructorSkippable::resolver>(this);
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
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
