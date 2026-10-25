// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/message_layout_helper.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/types/optional.h"
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
                    const Options& options) {
  auto* last_field = fields.back();
  return last_field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
         !IsLazy(last_field, options) && !last_field->is_repeated();
}

size_t AlignUp(size_t offset, size_t alignment) {
  return (offset + alignment - 1) & ~(alignment - 1);
}

}  // namespace

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
  const size_t other_align =
      static_cast<size_t>(EstimateAlignmentSize(other.fields_.front()));
  estimated_memory_size_ =
      static_cast<uint32_t>(AlignUp(estimated_memory_size_, other_align) +
                            other.estimated_memory_size_);

  fields_.insert(fields_.end(), other.fields_.begin(), other.fields_.end());

  return true;
}

MessageLayoutHelper::FieldVector MessageLayoutHelper::DoOptimizeLayout(
    const FieldVector& fields, const Options& options) const {
  // When a message has has-bits, Impl_ emits HasBits<words> (4 * words bytes)
  // followed by CachedSize (4 bytes) ahead of field members, starting fields at
  // offset 4 (mod 8) whenever words is positive and even.
  const size_t has_bit_words = (static_cast<size_t>(absl::c_count_if(
                                    fields,
                                    [&options](const FieldDescriptor* field) {
                                      return HasHasbit(field, options);
                                    })) +
                                31) /
                               32;
  const size_t initial_offset_mod_8 =
      (has_bit_words > 0 && has_bit_words % 2 == 0) ? 4 : 0;

  auto field_alignment_groups = BuildFieldAlignmentGroups(fields, options);
  auto field_groups =
      MergeFieldAlignmentGroups(std::move(field_alignment_groups));
  auto ordered_fields = BuildFieldDescriptorOrder(
      std::move(field_groups), initial_offset_mod_8, options);
  return ordered_fields;
}

// This function determines the order of the field hotness groups in the
// message. The higher the number, the closer to the top of the message.
constexpr size_t MessageLayoutHelper::FieldHotnessIndex(FieldHotness hotness) {
  return static_cast<size_t>(hotness);
}

MessageLayoutHelper::FieldFamily MessageLayoutHelper::GetFieldFamily(
    const FieldDescriptor* field, const Options& options) {
  if (field->is_repeated()) {
    return ShouldSplit(field, options) ? OTHER : REPEATED;
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
    return STRING;
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    return MESSAGE;
  } else if (CanInitializeByZeroing(field, options)) {
    return ZERO_INITIALIZABLE;
  } else {
    return OTHER;
  }
}

std::vector<internal::TailCallTableInfo::FastFieldInfo>
MessageLayoutHelper::BuildFastParseTable(const Options& options) const {
  FieldVector ordered_fields;
  for (const auto* field : GetOrderedFields(descriptor_)) {
    if (IsLayoutOptimized(field, options)) {
      ordered_fields.push_back(field);
    }
  }
  auto field_options = ParseFunctionGenerator::BuildFieldOptions(
      descriptor_, ordered_fields,
      /*get_has_bit_index=*/[](const auto*) { return absl::nullopt; }, options);
  auto table_info = ParseFunctionGenerator::BuildTcTableInfoFromDescriptor(
      descriptor_, options, field_options);
  return table_info.fast_path_fields;
}

bool MessageLayoutHelper::IsFastPathField(
    const FieldDescriptor* field,
    const std::vector<internal::TailCallTableInfo::FastFieldInfo>&
        fast_path_fields) {
  if (!internal::IsFieldTypeEligibleForFastParsing(field)) {
    return false;
  }

  const uint32_t coded_tag = internal::GetRecodedTagForFastParsing(field);
  const uint32_t fast_idx = internal::TcParseTableBase::TagToIdx(
      coded_tag, static_cast<uint32_t>(fast_path_fields.size()));
  const auto* fast_field = fast_path_fields[fast_idx].AsField();
  return fast_field != nullptr && fast_field->field == field;
}

bool MessageLayoutHelper::ShouldPromoteToFastParse(
    const FieldDescriptor* field, FieldHotness hotness,
    const std::vector<internal::TailCallTableInfo::FastFieldInfo>&
        fast_path_fields) {
  // Only promote warm and hot fields to fast-parse.
  if (hotness < FieldHotness::kWarm) return false;
  return IsFastPathField(field, fast_path_fields);
}

MessageLayoutHelper::FieldAlignmentGroups
MessageLayoutHelper::BuildFieldAlignmentGroups(const FieldVector& fields,
                                               const Options& options) const {
  FieldAlignmentGroups field_alignment_groups;
  const auto fast_path_fields = BuildFastParseTable(options);

  for (const auto* field : fields) {
    FieldFamily f = GetFieldFamily(field, options);

    FieldHotness hotness;
    if (ShouldSplit(field, options)) {
      hotness = FieldHotness::kSplit;
    } else if (field->is_repeated()) {
      hotness = FieldHotness::kRepeated;
    } else {
      hotness = GetFieldHotness(field, options);
    }

    if (ShouldPromoteToFastParse(field, hotness, fast_path_fields)) {
      hotness = FieldHotness::kFastParse;
    }

    FieldGroup fg = SingleFieldGroup(field);
    switch (EstimateAlignmentSize(field)) {
      case 1:
        field_alignment_groups.aligned_to_1[f][FieldHotnessIndex(hotness)]
            .push_back(fg);
        break;
      case 4:
        field_alignment_groups.aligned_to_4[f][FieldHotnessIndex(hotness)]
            .push_back(fg);
        break;
      case 8:
        field_alignment_groups.aligned_to_8[f][FieldHotnessIndex(hotness)]
            .push_back(fg);
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

    // Group single-byte fields into 4-byte chunks. Hold back any trailing
    // 1..3-byte group until 4-byte fields have paired into 8-byte words, so a
    // partial bool group cannot sort between 4-byte fields and leave one
    // unpaired.
    auto aligned_1_to_4 = ConsolidateAlignedFieldGroups(
        aligned_to_1, /*alignment=*/1, /*target_alignment=*/4);
    std::array<FieldGroup, kMaxHotness> sub_4_groups;
    for (size_t h = 0; h < kMaxHotness; ++h) {
      if (!aligned_1_to_4[h].empty() &&
          aligned_1_to_4[h].back().estimated_memory_size() < 4) {
        sub_4_groups[h] = std::move(aligned_1_to_4[h].back());
        aligned_1_to_4[h].pop_back();
      }
      aligned_to_4[h].insert(aligned_to_4[h].end(), aligned_1_to_4[h].begin(),
                             aligned_1_to_4[h].end());
    }

    // Group 4-byte fields into 8-byte words, keeping any leftover 1..3-byte
    // group separate so BuildFieldDescriptorOrder can pack it independently
    // across partition boundaries.
    auto aligned_4_to_8 = ConsolidateAlignedFieldGroups(
        aligned_to_4, /*alignment=*/4, /*target_alignment=*/8);
    for (size_t h = 0; h < kMaxHotness; ++h) {
      if (sub_4_groups[h].num_fields() > 0) {
        aligned_4_to_8[h].push_back(std::move(sub_4_groups[h]));
      }
      aligned_to_8[h].insert(aligned_to_8[h].end(), aligned_4_to_8[h].begin(),
                             aligned_4_to_8[h].end());
    }
  }

  MaybeMergeHotIntoFast(field_alignment_groups.aligned_to_8);

  return field_alignment_groups.aligned_to_8;
}

MessageLayoutHelper::FieldVector MessageLayoutHelper::BuildFieldDescriptorOrder(
    FieldPartitionArray&& field_groups, size_t initial_offset_mod_8,
    const Options& options) const {
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
  size_t offset_mod_8 = initial_offset_mod_8;
  // Iterate from hottest to coldest partition, so the hottest fields are placed
  // at the top of the message.
  for (size_t h = kMaxHotness - 1; h < kMaxHotness; --h) {
    if (h == FieldHotnessIndex(FieldHotness::kSplit)) {
      // Split fields live in a separate struct.
      offset_mod_8 = 0;
    }
    for (size_t family_order_idx = 0; family_order_idx < kMaxFamily;
         ++family_order_idx) {
      const size_t f = has_profile ? profiled_orders[recipe][family_order_idx]
                                   : default_orders[family_order_idx];
      auto& partition = field_groups[f][h];

      auto can_hoist_to_front = [&](const FieldGroup& fg) {
        (void)fg;
        return true;
      };

      // Place sub-8-byte groups at the partition boundary rather than ahead of
      // equal-access 8-byte fields. If the previous partition left trailing
      // bytes in an 8-byte word, hoist top-access groups that fit into the
      // open word (filling any sub-4-byte gap first); otherwise push sub-8-byte
      // groups to the end of their access tier, keeping 4-byte-aligned groups
      // ahead of 1-byte tails.
      size_t sim_off = offset_mod_8;
      double hoist_location = -10.0;
      if (0 < sim_off && sim_off < 4) {
        for (FieldGroup& fg : partition) {
          if (fg.estimated_memory_size() < 4 && can_hoist_to_front(fg) &&
              EstimateAlignmentSize(fg.fields().front()) == 1 &&
              sim_off + fg.estimated_memory_size() <= 4) {
            fg.SetPreferredLocation(hoist_location);
            hoist_location += 1.0;
            sim_off = (sim_off + fg.estimated_memory_size()) % 8;
            break;
          }
        }
      }
      for (FieldGroup& fg : partition) {
        if (fg.estimated_memory_size() >= 8 || fg.preferred_location() < 0) {
          continue;
        }
        const size_t first_align =
            static_cast<size_t>(EstimateAlignmentSize(fg.fields().front()));
        if (sim_off > 0 && can_hoist_to_front(fg) &&
            AlignUp(sim_off, first_align) + fg.estimated_memory_size() <= 8) {
          fg.SetPreferredLocation(hoist_location);
          hoist_location += 1.0;
          sim_off =
              (AlignUp(sim_off, first_align) + fg.estimated_memory_size()) % 8;
        } else {
          const int tail_rank = fg.estimated_memory_size() % 4 == 0 ? 1
                                : first_align == 4                  ? 2
                                                                    : 3;
          fg.SetPreferredLocation(
              double{FieldDescriptor::kMaxNumber} * tail_rank +
              std::min(fg.preferred_location(),
                       double{FieldDescriptor::kMaxNumber} - 1.0));
        }
      }

      std::stable_sort(partition.begin(), partition.end());
      for (const auto& aligned_fields : partition) {
        for (const auto* field : aligned_fields.fields()) {
          const size_t align =
              static_cast<size_t>(EstimateAlignmentSize(field));
          offset_mod_8 = (AlignUp(offset_mod_8, align) + align) % 8;
        }
        fields.insert(fields.end(), aligned_fields.fields().begin(),
                      aligned_fields.fields().end());
      }
    }

    if (!fields.empty() && (EndsWithMsgPtr(fields, options) ||
                            CanInitializeByZeroing(fields.back(), options))) {
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

void MessageLayoutHelper::FillPaddingFromPartition(
    std::vector<FieldGroup>& dst_partition,
    std::vector<FieldGroup>& src_partition, size_t alignment) {
  // We want to combine as many field groups as possible into dst_partition to
  // minimize padding. To do this, we sort the dst_partition by size in
  // ascending order, and src_partition by size in descending order. Then, we
  // iterate through the dst_partition and combine each group with the
  // largest group from src_partition that fits. By sorting src_partition in
  // descending order, when searching for a group from src_partition that fits
  // the next largest group in dst_partition, we can resume iteration from where
  // previously left off.

  // Sort dst_partition by size in ascending order.
  std::stable_sort(dst_partition.begin(), dst_partition.end(),
                   [](const FieldGroup& fg1, const FieldGroup& fg2) {
                     return fg1.estimated_memory_size() <
                            fg2.estimated_memory_size();
                   });
  // Sort src_partition by size in descending order.
  std::stable_sort(src_partition.begin(), src_partition.end(),
                   [](const FieldGroup& fg1, const FieldGroup& fg2) {
                     return fg1.estimated_memory_size() >
                            fg2.estimated_memory_size();
                   });

  // Iterate through dst_partition and combine each group with the largest group
  // from src_partition that fits.
  for (auto dst_it = dst_partition.begin(), src_it = src_partition.begin();
       dst_it != dst_partition.end() && src_it != src_partition.end();
       ++dst_it) {
    while (src_it != src_partition.end() &&
           AlignUp(dst_it->estimated_memory_size(),
                   static_cast<size_t>(
                       EstimateAlignmentSize(src_it->fields().front()))) +
                   src_it->estimated_memory_size() >
               alignment) {
      ++src_it;
    }

    if (src_it != src_partition.end()) {
      dst_it->Append(*src_it);
      src_it = src_partition.erase(src_it);
    }
  }

  for (const auto& field_group : dst_partition) {
    ABSL_CHECK(field_group.estimated_memory_size() <= alignment)
        << "Field group of size " << field_group.estimated_memory_size()
        << " should have been merged into a group at most " << alignment
        << " bytes.";
  }
}

void MessageLayoutHelper::MaybeMergeHotIntoFast(
    FieldPartitionArray& field_groups) {
  constexpr size_t kFastParseIdx = FieldHotnessIndex(FieldHotness::kFastParse);
  constexpr size_t kHotIdx = FieldHotnessIndex(FieldHotness::kHot);

  size_t num_fast_fields = 0;
  for (size_t f = 0; f < kMaxFamily; ++f) {
    for (const auto& group : field_groups[f][kFastParseIdx]) {
      num_fast_fields += group.num_fields();
    }
  }

  size_t num_hot_fields = 0;
  for (size_t f = 0; f < kMaxFamily; ++f) {
    for (const auto& group : field_groups[f][kHotIdx]) {
      num_hot_fields += group.num_fields();
    }
  }

  if (num_fast_fields + num_hot_fields <=
      internal::TailCallTableInfo::kMaxFastFieldHasbitIndex + 1) {
    for (size_t f = 0; f < kMaxFamily; ++f) {
      if (field_groups[f][kHotIdx].empty()) {
        continue;
      }

      FillPaddingFromPartition(field_groups[f][kFastParseIdx],
                               field_groups[f][kHotIdx], /*alignment=*/8);

      // Append all remaining hot fields to the end of the fast parse group.
      field_groups[f][kFastParseIdx].insert(
          field_groups[f][kFastParseIdx].end(),
          field_groups[f][kHotIdx].begin(), field_groups[f][kHotIdx].end());
      field_groups[f][kHotIdx].clear();
    }
  }
}

bool operator<(MessageLayoutHelper::FieldHotness h1,
               MessageLayoutHelper::FieldHotness h2) {
  return MessageLayoutHelper::FieldHotnessIndex(h1) <
         MessageLayoutHelper::FieldHotnessIndex(h2);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
