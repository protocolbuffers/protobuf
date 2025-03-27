#include "google/protobuf/compiler/cpp/message_layout_helper.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

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

FieldPartitionArray MessageLayoutHelper::PartitionFields(
    const std::vector<const FieldDescriptor*>& fields, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) const {
  FieldPartitionArray field_partitions;

  for (const auto* field : fields) {
    if (ShouldSplit(field, options)) {
      field_partitions[kSplit].push_back(field);
    } else if (field->is_repeated()) {
      field_partitions[kRepeated].push_back(field);
    } else {
      field_partitions[GetFieldHotness(field, options, scc_analyzer)].push_back(
          field);
    }
  }

  return field_partitions;
}

// Field layout policy slightly varies according to the type of proto fields
// but all fields are ordered in descending hotness.
//
// (1) REPEATED
// RepeatedFields and RepeatedPtrFields have their own constructors
// and therefore compiler inserts code that initializes those fields
// at three different constructors (default, copy, arena).
//
// As these fields can't be mixed with fields that are initialized in
// SharedCtor, repeated fields are laid out before other types of fields.
//
// (2) MESSAGE and ZERO_INITIALIZABLE
// Primitive fields, messages, etc. can be zero'ed at construction.
// These fields are currently laid out adjacently and bulk-initialized
// (memset zero'ed) in SharedCtor. Note that messages can't be cleared by memset
// unlike primitive fields. So, two types are laid out separately.
//
// (3) OTHER (STRING, LAZY_MESSAGE, etc.)
// Primitive fields with non-zero default values, lazy message, etc. are not
// initialized in bulk (memset) and can be mixed to minimize
// compartmentalization.
//
void MessageLayoutHelper::OptimizeLayoutByFamily(
    std::vector<const FieldDescriptor*>& fields, const Options& options,
    absl::Span<const FieldFamily> families, MessageSCCAnalyzer* scc_analyzer) {
  // First divide fields into those that align to 1 byte, 4 bytes or 8 bytes.
  std::vector<FieldGroup> aligned_to_1[kMaxFamily];
  std::vector<FieldGroup> aligned_to_4[kMaxFamily];
  std::vector<FieldGroup> aligned_to_8[kMaxFamily];
  for (const auto* field : fields) {
    FieldFamily f = OTHER;
    if (field->is_repeated()) {
      f = ShouldSplit(field, options) ? OTHER : REPEATED;
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      f = STRING;
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      f = MESSAGE;
    } else if (CanInitializeByZeroing(field, options, scc_analyzer)) {
      f = ZERO_INITIALIZABLE;
    }

    // Use all types of access information to order within "fields".
    FieldGroup fg = SingleFieldGroup(field);
    switch (EstimateAlignmentSize(field)) {
      case 1:
        aligned_to_1[f].push_back(fg);
        break;
      case 4:
        aligned_to_4[f].push_back(fg);
        break;
      case 8:
        aligned_to_8[f].push_back(fg);
        break;
      default:
        ABSL_LOG(FATAL) << "Unknown alignment size "
                        << EstimateAlignmentSize(field) << "for a field "
                        << field->full_name() << ".";
    }
  }

  // For each family, group fields to optimize locality and padding.
  for (size_t f = 0; f < kMaxFamily; ++f) {
    std::stable_sort(aligned_to_1[f].begin(), aligned_to_1[f].end());

    // Now group fields aligned to 1 byte into sets of 4, and treat those like a
    // single field aligned to 4 bytes.
    for (size_t i = 0; i < aligned_to_1[f].size(); i += 4) {
      FieldGroup field_group;
      for (size_t j = i; j < aligned_to_1[f].size() && j < i + 4; ++j) {
        field_group.Append(aligned_to_1[f][j]);
      }
      aligned_to_4[f].push_back(field_group);
    }
    // Using stable_sort ensures that the output is consistent across runs.
    std::stable_sort(aligned_to_4[f].begin(), aligned_to_4[f].end());

    // Now group fields aligned to 4 bytes (or the 4-field groups created above)
    // into pairs, and treat those like a single field aligned to 8 bytes.
    for (size_t i = 0; i < aligned_to_4[f].size(); i += 2) {
      FieldGroup field_group;
      for (size_t j = i; j < aligned_to_4[f].size() && j < i + 2; ++j) {
        field_group.Append(aligned_to_4[f][j]);
      }

      // `i` == `aligned_to_4[f].size() - 1` only if there are an odd number of
      // 4-byte field groups. In this case, the last 4-byte field group is
      // incomplete, and should be placed at the beginning or end.
      if (i == aligned_to_4[f].size() - 1) {
        // The goal is to minimize padding, and only ZERO_INITIALIZABLE and
        // OTHER families can have primitive fields (alignment < 8). We lay out
        // ZERO_INITIALIZABLE then OTHER, so only hoist the incomplete 4-byte
        // block to the beginning if it's in the OTHER family, otherwise place
        // it at the end.
        if (f == OTHER) {
          // Move incomplete 4-byte block to the beginning.  This is done to
          // pair with the (possible) leftover blocks from the
          // ZERO_INITIALIZABLE family.
          field_group.SetPreferredLocation(-1);
        } else {
          // Move incomplete 4-byte block to the end.
          field_group.SetPreferredLocation(double{FieldDescriptor::kMaxNumber});
        }
      }

      aligned_to_8[f].push_back(field_group);
    }

    std::stable_sort(aligned_to_8[f].begin(), aligned_to_8[f].end());
  }

  // Now pull out all the FieldDescriptors in order.
  fields.clear();
  for (auto f : families) {
    for (const auto& aligned_fields : aligned_to_8[f]) {
      fields.insert(fields.end(), aligned_fields.fields().begin(),
                    aligned_fields.fields().end());
    }
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
