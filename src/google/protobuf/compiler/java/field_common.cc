#include "google/protobuf/compiler/java/field_common.h"

#include <cstddef>
#include <string>

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/names.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

std::string GetKotlinPropertyName(std::string capitalized_name);

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
  auto kt_property_name = GetKotlinPropertyName(info->capitalized_name);
  (*variables)["kt_property_name"] = kt_property_name;
  (*variables)["kt_safe_name"] = IsForbiddenKotlin(kt_property_name)
                                     ? absl::StrCat("`", kt_property_name, "`")
                                     : kt_property_name;
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

// Locale-independent ASCII upper and lower case munging.
static bool IsUpper(char c) {
  return static_cast<unsigned int>(c - 'A') <= 'Z' - 'A';
}

static char ToLower(char c) { return IsUpper(c) ? c - 'A' + 'a' : c; }

// Returns the name by which the generated Java getters and setters should be
// referenced from Kotlin as properties. In the simplest case, the original name
// is something like `foo_bar`, which gets translated into `getFooBar()` etc,
// and that in turn can be referenced from Kotlin as `fooBar`.
//
// The algorithm for translating proto names into Java getters and setters is
// straightforward. The first letter of each underscore-separated word gets
// uppercased and the underscores are deleted. There are no other changes, so in
// particular if the proto name has a string of capitals then those remain
// as-is.
//
// The algorithm that the Kotlin compiler uses to derive the property name is
// slightly more complicated. If the first character after `get` (etc) is a
// capital and the second isn't, then the property name is just that string with
// its first letter lowercased. So `getFoo` becomes `foo` and `getX` becomes
// `x`. But if there is more than one capital, then all but the last get
// lowercased. So `getHTMLPage` becomes `htmlPage`. If there are only capitals
// then they all get lowercased, so `getID` becomes `id`.
// TODO: move this to a Kotlin-specific location
std::string GetKotlinPropertyName(std::string capitalized_name) {
  // Find the first non-capital. If it is the second character, then we just
  // need to lowercase the first one. Otherwise we need to lowercase everything
  // up to but not including the last capital, except that if everything is
  // capitals then everything must be lowercased.
  std::string kt_property_name = capitalized_name;
  size_t first_non_capital;
  for (first_non_capital = 0; first_non_capital < capitalized_name.length() &&
                              IsUpper(capitalized_name[first_non_capital]);
       first_non_capital++) {
  }
  size_t stop = first_non_capital;
  if (stop > 1 && stop < capitalized_name.length()) stop--;
  for (size_t i = 0; i < stop; i++) {
    kt_property_name[i] = ToLower(kt_property_name[i]);
  }
  return kt_property_name;
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
