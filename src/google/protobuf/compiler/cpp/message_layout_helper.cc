#include "google/protobuf/compiler/cpp/message_layout_helper.h"

#include <array>
#include <vector>

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

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
