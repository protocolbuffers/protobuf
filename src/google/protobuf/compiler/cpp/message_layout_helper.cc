#include "google/protobuf/compiler/cpp/message_layout_helper.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/cpp/parse_function_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_tctable_gen.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

bool EndsWithMsgPtr(const std::vector<const FieldDescriptor*>& fields,
                    const Options& options, MessageSCCAnalyzer* scc_analyzer) {
  return fields.back()->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
         !IsLazy(fields.back(), options, scc_analyzer) &&
         !fields.back()->is_repeated();
}

auto FindIncompleteBlock(std::vector<FieldGroup>& aligned_to_8) {
  return absl::c_find_if(aligned_to_8, [](const FieldGroup& fg) {
    return fg.EstimateMemorySize() <= 4;
  });
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

void MessageLayoutHelper::DoOptimizeLayout(
    std::vector<const FieldDescriptor*>& fields, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) const {
  FieldPartitionArray aligned_to_1;
  FieldPartitionArray aligned_to_4;
  FieldPartitionArray aligned_to_8;

  const auto ordered_fields = GetOrderedFields(descriptor_);
  auto field_options = ParseFunctionGenerator::BuildFieldOptions(
      descriptor_, ordered_fields, options, scc_analyzer, {}, {});
  auto table_info = ParseFunctionGenerator::BuildTcTableInfoFromDescriptor(
      descriptor_, options, field_options);
  const auto& fast_path_fields = table_info.fast_path_fields;

  for (const auto* field : fields) {
    FieldFamily f;
    if (field->is_repeated()) {
      f = ShouldSplit(field, options) ? OTHER : REPEATED;
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      f = STRING;
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      f = MESSAGE;
    } else if (CanInitializeByZeroing(field, options, scc_analyzer)) {
      f = ZERO_INITIALIZABLE;
    } else {
      f = OTHER;
    }

    FieldHotness hotness;
    if (ShouldSplit(field, options)) {
      hotness = kSplit;
    } else if (field->is_repeated()) {
      hotness = kRepeated;
    } else {
      hotness = GetFieldHotness(field, options, scc_analyzer);

      const uint32_t tag = GetRecodedTagForFastParsing(field);
      const uint32_t fast_idx = internal::TcParseTableBase::TagToIdx(
          tag, static_cast<uint32_t>(fast_path_fields.size()));
      if (hotness != kCold && fast_path_fields[fast_idx].AsField() != nullptr &&
          fast_path_fields[fast_idx].AsField()->field == field) {
        hotness = kFastParse;
      }
    }

    FieldGroup fg = SingleFieldGroup(field);
    switch (EstimateAlignmentSize(field)) {
      case 1:
        aligned_to_1[f][hotness].push_back(fg);
        break;
      case 4:
        aligned_to_4[f][hotness].push_back(fg);
        break;
      case 8:
        aligned_to_8[f][hotness].push_back(fg);
        break;
      default:
        ABSL_LOG(FATAL) << "Unknown alignment size "
                        << EstimateAlignmentSize(field) << "for field "
                        << field->full_name() << ".";
    }
  }

  // For each family, group fields to optimize locality and padding.
  for (size_t f = 0; f < kMaxFamily; ++f) {
    auto& family_aligned_to_1 = aligned_to_1[f];
    auto& family_aligned_to_4 = aligned_to_4[f];
    auto& family_aligned_to_8 = aligned_to_8[f];

    // Group single-byte fields into groups of 4 bytes and combine them with the
    // existing 4-byte groups.
    auto aligned_to_4 = ConsolidateAlignedFieldGroups(
        family_aligned_to_1, /*alignment=*/1, /*target_alignment=*/4);
    for (size_t h = 0; h < kMaxHotness; ++h) {
      family_aligned_to_4[h].insert(family_aligned_to_4[h].end(),
                                    aligned_to_4[h].begin(),
                                    aligned_to_4[h].end());
    }

    // Group 4-byte fields into groups of 8 bytes and combine them with the
    // existing 8-byte groups.
    auto aligned_to_8 = ConsolidateAlignedFieldGroups(
        family_aligned_to_4, /*alignment=*/4, /*target_alignment=*/8);
    for (size_t h = 0; h < kMaxHotness; ++h) {
      family_aligned_to_8[h].insert(family_aligned_to_8[h].end(),
                                    aligned_to_8[h].begin(),
                                    aligned_to_8[h].end());
    }
  }

  MaybeMergeHotIntoFast(aligned_to_8);

  // Now pull out all the FieldDescriptors in order.

  enum { kZeroLast = 0, kZeroFirst = 1, kRecipeMax };
  constexpr FieldFamily profiled_orders[kRecipeMax][kMaxFamily] = {
      {REPEATED, STRING, OTHER, MESSAGE, ZERO_INITIALIZABLE},
      {ZERO_INITIALIZABLE, MESSAGE, OTHER, STRING, REPEATED},
  };
  constexpr FieldFamily default_orders[kMaxFamily] = {
      REPEATED, STRING, MESSAGE, ZERO_INITIALIZABLE, OTHER};

  const bool has_profile = HasProfiledData();
  fields.clear();
  int recipe = kZeroLast;
  bool incomplete_block_at_end = false;
  for (size_t h = 0; h < kMaxHotness; ++h) {
    for (auto f : has_profile ? profiled_orders[recipe] : default_orders) {
      auto& partition = aligned_to_8[f][h];

      // If there is an incomplete 4-byte block, it should be placed at the
      // beginning or end.
      auto it = FindIncompleteBlock(partition);
      if (it != partition.end()) {
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

      if (!fields.empty() &&
          (EndsWithMsgPtr(fields, options, scc_analyzer) ||
           CanInitializeByZeroing(fields.back(), options, scc_analyzer))) {
        recipe = kZeroFirst;
      } else {
        recipe = kZeroLast;
      }
    }
  }
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

void MessageLayoutHelper::MaybeMergeHotIntoFast(
    FieldPartitionArray& field_groups) {
  size_t num_fast_fields = 0;
  for (size_t f = 0; f < kMaxFamily; ++f) {
    for (const auto& group : field_groups[f][kFastParse]) {
      num_fast_fields += group.num_fields();
    }
  }

  size_t num_fields = 0;
  for (size_t f = 0; f < kMaxFamily; ++f) {
    for (const auto& group : field_groups[f][kHot]) {
      num_fields += group.num_fields();
    }
  }

  if (num_fast_fields + num_fields <=
      internal::TailCallTableInfo::kMaxFastFieldHasbitIndex + 1) {
    for (size_t f = 0; f < kMaxFamily; ++f) {
      if (field_groups[f][kHot].empty()) {
        continue;
      }

      // Sort fast fields by size in descending order.
      std::stable_sort(field_groups[f][kFastParse].begin(),
                       field_groups[f][kFastParse].end(),
                       [](const FieldGroup& fg1, const FieldGroup& fg2) {
                         return fg1.EstimateMemorySize() >
                                fg2.EstimateMemorySize();
                       });
      // Sort hot fields by size in ascending order.
      std::stable_sort(
          field_groups[f][kHot].begin(), field_groups[f][kHot].end(),
          [](const FieldGroup& fg1, const FieldGroup& fg2) {
            return fg1.EstimateMemorySize() < fg2.EstimateMemorySize();
          });

      for (auto fast_it = field_groups[f][kFastParse].begin(),
                hot_it = field_groups[f][kHot].begin();
           fast_it != field_groups[f][kFastParse].end() &&
           hot_it != field_groups[f][kHot].end();
           ++fast_it) {
        while (hot_it != field_groups[f][kHot].end() &&
               fast_it->EstimateMemorySize() + hot_it->EstimateMemorySize() >
                   8) {
          ++hot_it;
        }

        if (hot_it != field_groups[f][kHot].end()) {
          fast_it->Append(*hot_it);
          hot_it = field_groups[f][kHot].erase(hot_it);
        }
      }

      field_groups[f][kFastParse].insert(field_groups[f][kFastParse].end(),
                                         field_groups[f][kHot].begin(),
                                         field_groups[f][kHot].end());
      field_groups[f][kHot].clear();
    }
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
