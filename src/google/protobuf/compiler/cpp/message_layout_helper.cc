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
      field_partitions[FieldHotness(field, options, scc_analyzer)].push_back(
          field);
    }
  }

  return field_partitions;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
