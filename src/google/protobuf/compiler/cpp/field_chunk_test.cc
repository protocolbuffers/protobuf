#include "google/protobuf/compiler/cpp/field_chunk.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/field_layout.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/has_bits.h"
#include "google/protobuf/unittest.pb.h"


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

using ::proto2_unittest::TestAllTypes;

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

template <typename ProtoT, typename ProjT>
std::vector<FieldChunk> CreateAndCollectFields(
    absl::Span<const char*> field_names, const ProjT proj) {
  return CollectFields(CreateFieldArray<ProtoT>(field_names), {}, proj);
}

TEST(CollectFieldsTest, SingleChunk) {
  const char* field_names[] = {"repeated_int32", "optional_int32",
                               "optional_int64"};

  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* field) { return true; });

  EXPECT_EQ(chunks.size(), 1);
}

TEST(CollectFieldsTest, AllDifferentChunks) {
  const char* field_names[] = {"repeated_int32", "optional_int32",
                               "optional_int64"};

  auto chunks = CreateAndCollectFields<TestAllTypes>(
      field_names, [](const FieldDescriptor* field) { return field; });

  EXPECT_EQ(chunks.size(), 3);
}

TEST(CollectFieldsTest, RepeatedAndSingular) {
  const char* field_names[] = {"repeated_int32", "optional_int32",
                               "optional_int64"};

  auto fields = CreateFieldArray<TestAllTypes>(field_names);
  auto chunks = CollectFields(fields, {}, [](const FieldDescriptor* field) {
    return field->is_repeated();
  });

  ASSERT_EQ(chunks.size(), 2);

  EXPECT_EQ(chunks[0].fields.size(), 1);
  EXPECT_TRUE(chunks[0].fields.front()->is_repeated());

  EXPECT_EQ(chunks[1].fields.size(), 2);
  EXPECT_FALSE(chunks[1].fields.front()->is_repeated());
}

TEST(GenChunkMaskTest, ValidMaskFromFields) {
  const char* field_names[] = {"optional_int32", "optional_int64"};

  auto fields = CreateFieldArray<TestAllTypes>(field_names);
  ASSERT_EQ(fields.size(), 2);
  ASSERT_EQ(fields[0]->index(), 0);
  ASSERT_EQ(fields[1]->index(), 1);

  const uint32_t kHasbitIdxAt0 = 3;
  const uint32_t kHasbitIdxAt1 = 5;
  auto field_layout =
      FieldLayout::BuildForTesting(fields, {kHasbitIdxAt0, kHasbitIdxAt1});
  uint32_t mask = GenChunkMask(fields, field_layout);
  EXPECT_EQ(mask, (1 << kHasbitIdxAt0) | (1 << kHasbitIdxAt1));
}

TEST(GenChunkMaskTest, FullWordMaskFromFields) {
  const Descriptor* descriptor = TestAllTypes::GetDescriptor();
  std::vector<const FieldDescriptor*> fields;
  for (int i = 0; i < 37; ++i) {
    fields.push_back(descriptor->field(i));
  }

  // First two fields have no hasbit (kNoHasbit), the rest have the bits 0..34.
  std::vector<int> has_bit_indices(37, internal::kNoHasbit);
  std::iota(has_bit_indices.begin() + 2, has_bit_indices.end(), 0);

  auto field_layout = FieldLayout::BuildForTesting(fields, has_bit_indices);
  ASSERT_EQ(field_layout.CountFieldsSharingHasWord(0), 32);
  ASSERT_EQ(field_layout.CountFieldsSharingHasWord(1), 3);

  auto hasbit_fields = absl::MakeSpan(fields).subspan(2);

  EXPECT_EQ(GenChunkMask(hasbit_fields.subspan(0, 32), field_layout),
            ~uint32_t{0});
  EXPECT_EQ(GenChunkMask(hasbit_fields.subspan(0, 31), field_layout),
            (uint32_t{1} << 31) - 1);

  EXPECT_EQ(GenChunkMask(hasbit_fields.subspan(32, 3), field_layout),
            ~uint32_t{0});
  EXPECT_EQ(GenChunkMask(hasbit_fields.subspan(32, 2), field_layout),
            uint32_t{0b11});

  auto exact_word_layout = FieldLayout::BuildForTesting(
      std::vector<const FieldDescriptor*>(fields.begin(), fields.begin() + 34),
      std::vector<int>(has_bit_indices.begin(), has_bit_indices.begin() + 34));
  ASSERT_EQ(exact_word_layout.CountFieldsSharingHasWord(0), 32);
  EXPECT_EQ(GenChunkMask(hasbit_fields.subspan(0, 32), exact_word_layout),
            ~uint32_t{0});
}

FieldChunk CreateChunkWithFirstNFields(const Descriptor* descriptor, int n,
                                       bool has_hasbit = true) {
  ABSL_CHECK_LE(n, descriptor->field_count());
  FieldChunk chunk(has_hasbit);
  for (int i = 0; i < n; ++i) {
    chunk.fields.push_back(descriptor->field(i));
  }
  return chunk;
}

std::vector<std::vector<int>> ExtractBatchFieldIndices(
    absl::Span<const FieldChunk> chunks) {
  std::vector<std::vector<int>> batches;

  for (const FieldChunk& chunk : chunks) {
    std::vector<int>& batch = batches.emplace_back();
    absl::c_transform(chunk.fields, std::back_inserter(batch),
                      [](const FieldDescriptor* f) { return f->index(); });
  }

  return batches;
}

TEST(PartitionToMinimizeExpectedBranchesTest,
     DefaultOptionsBatchesAllFieldsTogether) {
  FieldChunk chunk =
      CreateChunkWithFirstNFields(TestAllTypes::GetDescriptor(), 4);

  // Without a profile, default presence probability is 0.0 (absence 1.0), so
  // batching all fields together costs 1.0 expected branch check.
  std::vector<FieldChunk> partitioned =
      PartitionToMinimizeExpectedBranches(chunk, Options{});

  std::vector<std::vector<int>> expected = {{0, 1, 2, 3}};
  EXPECT_EQ(ExtractBatchFieldIndices(partitioned), expected);
}


}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
