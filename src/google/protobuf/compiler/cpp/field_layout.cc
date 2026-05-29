#include "google/protobuf/compiler/cpp/field_layout.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/compiler/split_map.h"
#include "absl/algorithm/container.h"
#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/types/optional.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/message_layout_helper.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/cpp/padding_optimizer.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/has_bits.h"


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

using ::google::protobuf::internal::kNoHasbit;

std::unique_ptr<MessageLayoutHelper> CreateMessageLayoutHelper(
    const google::protobuf::Descriptor* descriptor, const Options& options) {
  return std::make_unique<PaddingOptimizer>(descriptor);
}

}  // namespace

FieldLayout FieldLayout::BuildOptimizedLayout(
    const google::protobuf::Descriptor* absl_nonnull descriptor, const Options& options,
    int& num_weak_fields) {
  std::vector<const google::protobuf::FieldDescriptor*> optimized_order;
  std::vector<int> has_bit_indices;
  int max_has_bit_index = 0;

  SplitMap split_map;

  // Compute optimized field order to be used for layout and initialization
  // purposes.
  num_weak_fields =
      CollectFieldsExcludingWeakAndOneof(descriptor, options, optimized_order);
  const size_t initial_size = optimized_order.size();

  auto message_layout_helper = CreateMessageLayoutHelper(descriptor, options);
  optimized_order = message_layout_helper->OptimizeLayout(optimized_order,
                                                          options, split_map);
  ABSL_CHECK_EQ(initial_size, optimized_order.size());

  // This message has hasbits iff one or more fields need one.
  for (const FieldDescriptor* field : optimized_order) {
    if (HasHasbit(field, options)) {
      if (has_bit_indices.empty()) {
        has_bit_indices.resize(static_cast<size_t>(descriptor->field_count()),
                               kNoHasbit);
      }
      has_bit_indices[static_cast<size_t>(field->index())] =
          max_has_bit_index++;
    }
  }

  return FieldLayout(std::move(has_bit_indices), max_has_bit_index,
                     std::move(optimized_order), std::move(split_map));
}

FieldLayout FieldLayout::BuildForTesting(
    std::vector<const FieldDescriptor*> fields,
    std::vector<int> has_bit_indices) {
  const auto max_it = absl::c_max_element(has_bit_indices);
  int max_has_bit_index =
      max_it != has_bit_indices.end() ? std::max(*max_it, 0) : 0;
  return FieldLayout(std::move(has_bit_indices), max_has_bit_index,
                     std::move(fields), /*split_map=*/{});
}

bool FieldLayout::HasHasbits() const { return !has_bit_indices_.empty(); }

int FieldLayout::HasBitsSize() const { return (max_has_bit_index_ + 31) / 32; }

absl::optional<int> FieldLayout::GetHasBitIndex(
    const FieldDescriptor* field) const {
  size_t index = static_cast<size_t>(field->index());
  if (has_bit_indices_.empty() || has_bit_indices_[index] == kNoHasbit) {
    return absl::nullopt;
  }
  return has_bit_indices_[index];
}

absl::optional<int> FieldLayout::GetHasByteIndex(
    const FieldDescriptor* field) const {
  absl::optional<int> hasbit = GetHasBitIndex(field);
  if (!hasbit.has_value()) {
    return absl::nullopt;
  }
  return hasbit.value() / 8;
}

absl::optional<int> FieldLayout::GetHasWordIndex(
    const FieldDescriptor* field) const {
  auto has_bit_index = GetHasBitIndex(field);
  if (!has_bit_index.has_value()) {
    return absl::nullopt;
  }
  return has_bit_index.value() / 32;
}

void FieldLayout::PrintHasBitIndicesForSchema(io::Printer* p,
                                              size_t& entries) const {
  if (has_bit_indices_.empty()) {
    return;
  }
  entries += has_bit_indices_.size();
  for (size_t i = 0; i < has_bit_indices_.size(); ++i) {
    const std::string index = has_bit_indices_[i] != kNoHasbit
                                  ? absl::StrCat(has_bit_indices_[i])
                                  : "~0u";
    p->Emit({{"index", index}}, "$index$,\n");
  }
}

bool FieldLayout::IsSplit(const FieldDescriptor* field) const {
  return split_map_.IsSplit(field);
}

FieldLayout::FieldLayout(std::vector<int> has_bit_indices,
                         int max_has_bit_index,
                         std::vector<const FieldDescriptor*> fields,
                         SplitMap&& split_map)
    : has_bit_indices_(std::move(has_bit_indices)),
      max_has_bit_index_(max_has_bit_index),
      fields_(std::move(fields)),
      split_map_(std::move(split_map)) {
  // Verify that all split fields are placed at the end in the optimized order.
  ABSL_CHECK(absl::c_is_partitioned(
      fields_,
      [this](const FieldDescriptor* field) { return !IsSplit(field); }));
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
