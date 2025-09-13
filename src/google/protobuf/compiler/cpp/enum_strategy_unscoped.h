#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_STRATEGY_UNSCOPED_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_STRATEGY_UNSCOPED_H__

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/enum_strategy.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google::protobuf::compiler::cpp {

class UnscopedEnumStrategy : public EnumStrategy {
 public:
  static constexpr absl::string_view kEnumKeywords = "enum";

  void GenerateEnumDefinitionBlock(
      io::Printer* p, const EnumStrategyContext& ctx) const override;

  void GenerateSymbolImports(  //
      io::Printer* p, const EnumStrategyContext& ctx) const override;
};

}  // namespace protobuf
}  // namespace google::compiler::cpp

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_STRATEGY_UNSCOPED_H__
