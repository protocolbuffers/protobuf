#include "google/protobuf/compiler/cpp/field_chunk.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

using proto2_unittest::TestAllTypes;
using testing::IsEmpty;

template <typename ProtoT>
std::vector<const google::protobuf::FieldDescriptor*> CreateFieldArray(
    absl::Span<const char*> field_names) {
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  fields.reserve(field_names.size());
  const Descriptor* descriptor = ProtoT::GetDescriptor();
  for (auto name : field_names) {
    const auto* f = descriptor->FindFieldByName(name);
    ABSL_CHECK_NE(f, nullptr);
    fields.push_back(f);
  }
  return fields;
}

template <typename ProtoT, typename PredT>
std::vector<FieldChunk> CreateAndCollectFields(
    absl::Span<const char*> field_names, PredT&& predicate) {
  return CollectFields(CreateFieldArray<ProtoT>(field_names), {}, predicate);
}

TEST(CollectFieldsTest, SingleChunk) {
  const char* field_names[] = {"repeated_int32", "optional_int32",
                               "optional_int64"};

  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* lhs, const FieldDescriptor* rhs) {
        return true;
      });

  EXPECT_EQ(chunks.size(), 1);
}

TEST(CollectFieldsTest, AllDifferentChunks) {
  const char* field_names[] = {"repeated_int32", "optional_int32",
                               "optional_int64"};

  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* lhs, const FieldDescriptor* rhs) {
        return false;
      });

  EXPECT_EQ(chunks.size(), 3);
}

TEST(CollectFieldsTest, RepeatedAndSingular) {
  const char* field_names[] = {"repeated_int32", "optional_int32",
                               "optional_int64"};

  auto fields = CreateFieldArray<TestAllTypes>(field_names);
  auto chunks = CollectFields(
      fields, {}, [](const FieldDescriptor* lhs, const FieldDescriptor* rhs) {
        return lhs->is_repeated() == rhs->is_repeated();
      });

  ASSERT_EQ(chunks.size(), 2);

  EXPECT_EQ(chunks[0].fields.size(), 1);
  EXPECT_TRUE(chunks[0].fields.front()->is_repeated());

  EXPECT_EQ(chunks[1].fields.size(), 2);
  EXPECT_FALSE(chunks[1].fields.front()->is_repeated());
}

TEST(FindNextTest, AllEqual) {
  const char* field_names[] = {"optional_int32", "optional_int64"};
  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* lhs, const FieldDescriptor* rhs) {
        return false;
      });
  auto it = FindNextUnequalChunk(
      chunks.begin(), chunks.end(),
      [](const FieldChunk&, const FieldChunk&) { return true; });

  EXPECT_EQ(it, chunks.end());
}

TEST(FindNextTest, AllUnequal) {
  const char* field_names[] = {"optional_int32", "optional_int64"};
  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* lhs, const FieldDescriptor* rhs) {
        return false;
      });
  auto it = FindNextUnequalChunk(
      chunks.begin(), chunks.end(),
      [](const FieldChunk&, const FieldChunk&) { return false; });

  ASSERT_NE(it, chunks.end());
  EXPECT_EQ(it->fields, chunks[1].fields);
}

TEST(ExtractTest, ExtractNone) {
  const char* field_names[] = {"optional_int32", "optional_int64"};
  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* lhs, const FieldDescriptor* rhs) {
        return false;
      });

  ASSERT_EQ(chunks.size(), 2);
  auto extracted =
      ExtractFields(chunks.begin(), chunks.end(),
                    [](const FieldDescriptor* f) { return false; });

  EXPECT_THAT(extracted, IsEmpty());
}

TEST(ExtractTest, ExtractAll) {
  const char* field_names[] = {"optional_int32", "optional_int64"};
  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* lhs, const FieldDescriptor* rhs) {
        return false;
      });

  ASSERT_EQ(chunks.size(), 2);

  auto extracted = ExtractFields(chunks.begin(), chunks.end(),
                                 [](const FieldDescriptor* f) { return true; });

  ASSERT_EQ(chunks.size(), 2);
  ASSERT_THAT(chunks[0].fields, IsEmpty());
  ASSERT_THAT(chunks[1].fields, IsEmpty());

  auto fields = CreateFieldArray<TestAllTypes>(field_names);
  EXPECT_THAT(extracted, testing::ElementsAreArray(fields));
}

TEST(GenChunkMaskTest, ValidMaskFromFields) {
  const char* field_names[] = {"optional_int32"};

  auto fields = CreateFieldArray<TestAllTypes>(field_names);
  ASSERT_EQ(fields.size(), 1);
  ASSERT_EQ(fields[0]->index(), 0);

  const uint32_t kHasbitIdx = 3;
  uint32_t mask = GenChunkMask(fields, {kHasbitIdx});
  EXPECT_EQ(mask, 1 << kHasbitIdx);
}

TEST(GenChunkMaskTest, ValidMaskFromChunks) {
  const char* field_names[] = {"optional_int32", "optional_int64"};
  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* lhs, const FieldDescriptor* rhs) {
        return false;
      });

  ASSERT_EQ(chunks.size(), 2);
  ASSERT_EQ(chunks[0].fields.size(), 1);
  ASSERT_EQ(chunks[0].fields[0]->index(), 0);
  ASSERT_EQ(chunks[1].fields.size(), 1);
  ASSERT_EQ(chunks[1].fields[0]->index(), 1);

  const uint32_t kHasbitIdxAt0 = 3;
  const uint32_t kHasbitIdxAt1 = 5;
  std::vector<int> has_bit_indices = {kHasbitIdxAt0, kHasbitIdxAt1};
  uint32_t mask = GenChunkMask(chunks.begin(), chunks.end(), has_bit_indices);
  EXPECT_EQ(mask, (1 << kHasbitIdxAt0) | (1 << kHasbitIdxAt1));
}


}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
