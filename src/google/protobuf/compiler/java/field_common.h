#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_FIELD_COMMON_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_FIELD_COMMON_H__

#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// Field information used in FieldGenerators.
struct FieldGeneratorInfo {
  std::string name;
  std::string capitalized_name;
  std::string disambiguated_reason;
};

// Oneof information used in OneofFieldGenerators.
struct OneofGeneratorInfo {
  std::string name;
  std::string capitalized_name;
};

// Set some common variables used in variable FieldGenerators.
void SetCommonFieldVariables(
    const FieldDescriptor* descriptor, const FieldGeneratorInfo* info,
    absl::flat_hash_map<absl::string_view, std::string>* variables);

// Set some common oneof variables used in OneofFieldGenerators.
void SetCommonOneofVariables(
    const FieldDescriptor* descriptor, const OneofGeneratorInfo* info,
    absl::flat_hash_map<absl::string_view, std::string>* variables);

// Print useful comments before a field's accessors.
void PrintExtraFieldInfo(
    const absl::flat_hash_map<absl::string_view, std::string>& variables,
    io::Printer* printer);

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_FIELD_COMMON_H__
