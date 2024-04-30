#include "google/protobuf/compiler/java/field_common.h"

#include <string>

#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

void SetCommonFieldVariables(
    const FieldDescriptor* descriptor, const FieldGeneratorInfo* info,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  (*variables)["field_name"] = descriptor->name();
  (*variables)["name"] = info->name;
  (*variables)["classname"] = descriptor->containing_type()->name();
  (*variables)["capitalized_name"] = info->capitalized_name;
  (*variables)["disambiguated_reason"] = info->disambiguated_reason;
  (*variables)["constant_name"] = FieldConstantName(descriptor);
  (*variables)["number"] = absl::StrCat(descriptor->number());
  (*variables)["kt_dsl_builder"] = "_builder";
  // These variables are placeholders to pick out the beginning and ends of
  // identifiers for annotations (when doing so with existing variables would
  // be ambiguous or impossible). They should never be set to anything but the
  // empty string.
  (*variables)["{"] = "";
  (*variables)["}"] = "";
  (*variables)["kt_name"] = IsForbiddenKotlin(info->name)
                                ? absl::StrCat(info->name, "_")
                                : info->name;
  (*variables)["kt_capitalized_name"] =
      IsForbiddenKotlin(info->name) ? absl::StrCat(info->capitalized_name, "_")
                                    : info->capitalized_name;
  if (!descriptor->is_repeated()) {
    (*variables)["annotation_field_type"] =
        std::string(FieldTypeName(descriptor->type()));
  } else if (GetJavaType(descriptor) == JAVATYPE_MESSAGE &&
             IsMapEntry(descriptor->message_type())) {
    (*variables)["annotation_field_type"] =
        absl::StrCat(FieldTypeName(descriptor->type()), "MAP");
  } else {
    (*variables)["annotation_field_type"] =
        absl::StrCat(FieldTypeName(descriptor->type()), "_LIST");
    if (descriptor->is_packed()) {
      variables->insert(
          {"annotation_field_type",
           absl::StrCat(FieldTypeName(descriptor->type()), "_LIST_PACKED")});
    }
  }
}

void SetCommonOneofVariables(
    const FieldDescriptor* descriptor, const OneofGeneratorInfo* info,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  (*variables)["oneof_name"] = info->name;
  (*variables)["oneof_capitalized_name"] = info->capitalized_name;
  (*variables)["oneof_index"] =
      absl::StrCat(descriptor->containing_oneof()->index());
  (*variables)["oneof_stored_type"] = GetOneofStoredType(descriptor);
  (*variables)["set_oneof_case_message"] =
      absl::StrCat(info->name, "Case_ = ", descriptor->number());
  (*variables)["clear_oneof_case_message"] =
      absl::StrCat(info->name, "Case_ = 0");
  (*variables)["has_oneof_case_message"] =
      absl::StrCat(info->name, "Case_ == ", descriptor->number());
}

void PrintExtraFieldInfo(
    const absl::flat_hash_map<absl::string_view, std::string>& variables,
    io::Printer* printer) {
  auto it = variables.find("disambiguated_reason");
  if (it != variables.end() && !it->second.empty()) {
    printer->Print(
        variables,
        "// An alternative name is used for field \"$field_name$\" because:\n"
        "//     $disambiguated_reason$\n");
  }
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
