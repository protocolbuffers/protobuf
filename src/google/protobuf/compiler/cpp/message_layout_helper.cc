#include "google/protobuf/compiler/cpp/message_layout_helper.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

bool EndsWithMsgPtr(const std::vector<const FieldDescriptor*>& fields,
                    const Options& options, MessageSCCAnalyzer* scc_analyzer) {
  const auto* last_field = fields.back();
  return last_field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
         !IsLazy(last_field, options, scc_analyzer) &&
         !last_field->is_repeated();
}

}  // namespace

size_t FieldGroup::EstimateMemorySize() const {
  size_t size = 0;
  for (const auto* field : fields_) {
    size += static_cast<size_t>(EstimateAlignmentSize(field));
  }
  return size;
}

void FieldGroup::Append(const FieldGroup& other) {
  UpdatePreferredLocationAndInsertOtherFields(other);
}

bool FieldGroup::operator<(const FieldGroup& other) const {
  return preferred_location_ < other.preferred_location_;
}

bool FieldGroup::UpdatePreferredLocationAndInsertOtherFields(
    const FieldGroup& other) {
  if (other.fields_.empty()) {
    return false;
  }

  // Preferred location is the average among all the fields, so we weight by
  // the number of fields on each FieldGroup object.
  preferred_location_ = (preferred_location_ * fields_.size() +
                         (other.preferred_location_ * other.fields_.size())) /
                        (fields_.size() + other.fields_.size());

  fields_.insert(fields_.end(), other.fields_.begin(), other.fields_.end());

  return true;
}

MessageLayoutHelper::FieldVector MessageLayoutHelper::DoOptimizeLayout(
    const FieldVector& fields, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) const {
  auto field_alignment_groups =
      BuildFieldAlignmentGroups(fields, options, scc_analyzer);
  auto field_groups =
      MergeFieldAlignmentGroups(std::move(field_alignment_groups));
  auto ordered_fields =
      BuildFieldDescriptorOrder(std::move(field_groups), options, scc_analyzer);
  return ordered_fields;
}

MessageLayoutHelper::FieldFamily MessageLayoutHelper::GetFieldFamily(
    const FieldDescriptor* field, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  if (field->is_repeated()) {
    return ShouldSplit(field, options) ? OTHER : REPEATED;
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
    return STRING;
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    return MESSAGE;
  } else if (CanInitializeByZeroing(field, options, scc_analyzer)) {
    return ZERO_INITIALIZABLE;
  } else {
    return OTHER;
  }
}

MessageLayoutHelper::FieldHotness MessageLayoutHelper::GetFieldHotnessCategory(
    const FieldDescriptor* field, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) const {
  if (ShouldSplit(field, options)) {
    return kSplit;
  } else if (field->is_repeated()) {
    return kRepeated;
  } else {
    return GetFieldHotness(field, options, scc_analyzer);
  }
}

MessageLayoutHelper::FieldAlignmentGroups
MessageLayoutHelper::BuildFieldAlignmentGroups(
    const FieldVector& fields, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) const {
  FieldAlignmentGroups field_alignment_groups;

  for (const auto* field : fields) {
    FieldFamily f = GetFieldFamily(field, options, scc_analyzer);

    FieldHotness hotness =
        GetFieldHotnessCategory(field, options, scc_analyzer);

    FieldGroup fg = SingleFieldGroup(field);
    switch (EstimateAlignmentSize(field)) {
      case 1:
        field_alignment_groups.aligned_to_1[f][hotness].push_back(fg);
        break;
      case 4:
        field_alignment_groups.aligned_to_4[f][hotness].push_back(fg);
        break;
      case 8:
        field_alignment_groups.aligned_to_8[f][hotness].push_back(fg);
        break;
      default:
        ABSL_LOG(FATAL) << "Unknown alignment size "
                        << EstimateAlignmentSize(field) << "for field "
                        << field->full_name() << ".";
    }
  }

  return field_alignment_groups;
}

MessageLayoutHelper::FieldPartitionArray
MessageLayoutHelper::MergeFieldAlignmentGroups(
    FieldAlignmentGroups&& field_alignment_groups) {
  // For each family, group fields to optimize locality and padding.
  for (size_t f = 0; f < kMaxFamily; ++f) {
    auto& aligned_to_1 = field_alignment_groups.aligned_to_1[f];
    auto& aligned_to_4 = field_alignment_groups.aligned_to_4[f];
    auto& aligned_to_8 = field_alignment_groups.aligned_to_8[f];

    // Group single-byte fields into groups of 4 bytes and combine them with the
    // existing 4-byte groups.
    auto aligned_1_to_4 = ConsolidateAlignedFieldGroups(
        aligned_to_1, /*alignment=*/1, /*target_alignment=*/4);
    for (size_t h = 0; h < kMaxHotness; ++h) {
      aligned_to_4[h].insert(aligned_to_4[h].end(), aligned_1_to_4[h].begin(),
                             aligned_1_to_4[h].end());
    }

    // Group 4-byte fields into groups of 8 bytes and combine them with the
    // existing 8-byte groups.
    auto aligned_4_to_8 = ConsolidateAlignedFieldGroups(
        aligned_to_4, /*alignment=*/4, /*target_alignment=*/8);
    for (size_t h = 0; h < kMaxHotness; ++h) {
      aligned_to_8[h].insert(aligned_to_8[h].end(), aligned_4_to_8[h].begin(),
                             aligned_4_to_8[h].end());
    }
  }

  return field_alignment_groups.aligned_to_8;
}

MessageLayoutHelper::FieldVector MessageLayoutHelper::BuildFieldDescriptorOrder(
    FieldPartitionArray&& field_groups, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) const {
  enum { kZeroLast = 0, kZeroFirst = 1, kRecipeMax };
  constexpr FieldFamily profiled_orders[kRecipeMax][kMaxFamily] = {
      {REPEATED, STRING, OTHER, MESSAGE, ZERO_INITIALIZABLE},
      {ZERO_INITIALIZABLE, MESSAGE, OTHER, STRING, REPEATED},
  };
  constexpr FieldFamily default_orders[kMaxFamily] = {
      REPEATED, STRING, MESSAGE, ZERO_INITIALIZABLE, OTHER};

  FieldVector fields;

  const bool has_profile = HasProfiledData();
  int recipe = kZeroLast;
  bool incomplete_block_at_end = false;
  for (size_t h = 0; h < kMaxHotness; ++h) {
    for (size_t family_order_idx = 0; family_order_idx < kMaxFamily;
         ++family_order_idx) {
      const size_t f = has_profile ? profiled_orders[recipe][family_order_idx]
                                   : default_orders[family_order_idx];
      auto& partition = field_groups[f][h];

      // If there is an incomplete 4-byte block, it should be placed at the
      // beginning or end.
      auto it = absl::c_find_if(partition, [](const FieldGroup& fg) {
        return fg.EstimateMemorySize() <= 4;
      });
      if (it != partition.end()) {
        // There should be at most one incomplete 4-byte block, and it will
        // always be the last element.
        ABSL_CHECK(it + 1 == partition.end());

        // The goal is to minimize padding, and only ZERO_INITIALIZABLE and
        // OTHER families can have primitive fields (alignment < 8). We lay
        // out ZERO_INITIALIZABLE then OTHER, so only hoist the incomplete
        // 4-byte block to the beginning if it's in the OTHER family,
        // otherwise place it at the end.
        if (incomplete_block_at_end) {
          // Move incomplete 4-byte block to the beginning.  This is done to
          // pair with the (possible) leftover blocks from the
          // ZERO_INITIALIZABLE family.
          it->SetPreferredLocation(-1);
          incomplete_block_at_end = false;
        } else {
          // Move incomplete 4-byte block to the end.
          it->SetPreferredLocation(double{FieldDescriptor::kMaxNumber});
          incomplete_block_at_end = true;
        }
      } else if (!partition.empty()) {
        incomplete_block_at_end = false;
      }

      std::stable_sort(partition.begin(), partition.end());
      for (const auto& aligned_fields : partition) {
        fields.insert(fields.end(), aligned_fields.fields().begin(),
                      aligned_fields.fields().end());
      }
    }

    if (!fields.empty() &&
        (EndsWithMsgPtr(fields, options, scc_analyzer) ||
         CanInitializeByZeroing(fields.back(), options, scc_analyzer))) {
      recipe = kZeroFirst;
    } else {
      recipe = kZeroLast;
    }
  }

  return fields;
}

std::array<std::vector<FieldGroup>, MessageLayoutHelper::kMaxHotness>
MessageLayoutHelper::ConsolidateAlignedFieldGroups(
    std::array<std::vector<FieldGroup>, kMaxHotness>& field_groups,
    size_t alignment, size_t target_alignment) {
  ABSL_CHECK_GT(target_alignment, alignment);
  ABSL_CHECK_EQ(target_alignment % alignment, size_t{0});

  const size_t size_inflation = target_alignment / alignment;
  std::array<std::vector<FieldGroup>, kMaxHotness> partitions_aligned_to_target;

  for (size_t h = 0; h < kMaxHotness; ++h) {
    auto& partition = field_groups[h];
    auto& target_partition = partitions_aligned_to_target[h];
    target_partition.reserve((field_groups.size() + size_inflation - 1) /
                             size_inflation);

    // Using stable_sort ensures that the output is consistent across runs.
    std::stable_sort(partition.begin(), partition.end());

    // Group fields into groups of `size_inflation` fields, which will be
    // aligned to `target_alignment`.
    for (size_t i = 0; i < partition.size(); i += size_inflation) {
      FieldGroup field_group;
      for (size_t j = i; j < partition.size() && j < i + size_inflation; ++j) {
        field_group.Append(partition[j]);
      }
      target_partition.push_back(field_group);
    }
  }

  return partitions_aligned_to_target;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
