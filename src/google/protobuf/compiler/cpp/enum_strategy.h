#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_STRATEGY_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_STRATEGY_H__

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google::protobuf::compiler::cpp {

struct ValueLimits {
  const EnumValueDescriptor* min;
  const EnumValueDescriptor* max;

  inline static ValueLimits FromEnum(const EnumDescriptor* descriptor) {
    const EnumValueDescriptor* min_desc = descriptor->value(0);
    const EnumValueDescriptor* max_desc = descriptor->value(0);

    for (int i = 1; i < descriptor->value_count(); ++i) {
      if (descriptor->value(i)->number() < min_desc->number()) {
        min_desc = descriptor->value(i);
      }
      if (descriptor->value(i)->number() > max_desc->number()) {
        max_desc = descriptor->value(i);
      }
    }

    return ValueLimits{min_desc, max_desc};
  }
};

inline absl::flat_hash_map<absl::string_view, std::string> EnumVars(
    const EnumDescriptor* enum_,     //
    const Options& options,          //
    const EnumValueDescriptor* min,  //
    const EnumValueDescriptor* max) {
  auto classname = ClassName(enum_, false);
  return {
      {"Enum", std::string(enum_->name())},
      {"Enum_", ResolveKnownNameCollisions(enum_->name(),
                                           enum_->containing_type() != nullptr
                                               ? NameContext::kMessage
                                               : NameContext::kFile,
                                           NameKind::kType)},
      {"Msg_Enum", classname},
      {"::Msg_Enum", QualifiedClassName(enum_, options)},
      {"Msg_Enum_",
       enum_->containing_type() == nullptr ? "" : absl::StrCat(classname, "_")},
      {"kMin", absl::StrCat(min->number())},
      {"kMax", absl::StrCat(max->number())},
      {"return_type", CppGenerator::GetResolvedSourceFeatures(*enum_)
                              .GetExtension(::pb::cpp)
                              .enum_name_uses_string_view()
                          ? "::absl::string_view"
                          : "const ::std::string&"},
  };
}

struct EnumStrategyContext {
  const EnumDescriptor* enum_;  // NOLINT
  const Options& options;
  const ValueLimits& limits;
  const absl::flat_hash_map<absl::string_view, std::string>& enum_vars;
  bool generate_array_size;
  bool should_cache;
  bool has_reflection;
  bool is_nested;
};

class EnumStrategy {
 public:
  virtual ~EnumStrategy() = default;

  virtual void GenerateEnumDefinitionBlock(
      io::Printer* p, const EnumStrategyContext& context) const = 0;

  virtual void GenerateSymbolImports(
      io::Printer* p, const EnumStrategyContext& context) const = 0;

  inline static bool EnumIsUnscoped(const EnumDescriptor* enum_) {
    return CppGenerator::GetResolvedSourceFeatures(*enum_)
        .GetExtension(::pb::cpp)
        .legacy_unscoped_enum();
  }
};

}  // namespace protobuf
}  // namespace google::compiler::cpp

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_STRATEGY_H__
