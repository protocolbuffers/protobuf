#include "google/protobuf/compiler/cpp/enum_strategy_scoped.h"

#include "google/protobuf/compiler/cpp/enum_strategy.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google::protobuf::compiler::cpp {

using Sub = ::google::protobuf::io::Printer::Sub;

void ScopedEnumStrategy::GenerateEnumDefinitionBlock(
    io::Printer* p, const EnumStrategyContext& ctx) const {
  auto v1 = p->WithVars(ctx.enum_vars);

  p->Emit(
      {
          {"values",
           [&] {
             for (int i = 0; i < ctx.enum_->value_count(); ++i) {
               const auto* value = ctx.enum_->value(i);
               p->Emit(
                   {
                       Sub("Msg_Enum_VALUE", EnumValueName(value))
                           .AnnotatedAs(value),
                       {"kNumber", Int32ToString(value->number())},
                       {"DEPRECATED",
                        value->options().deprecated() ? "[[deprecated]]" : ""},
                   },
                   R"cc(
                     $Msg_Enum_VALUE$$ DEPRECATED$ = $kNumber$,
                   )cc");
             }
           }},
          // Only emit annotations for the $Msg_Enum$ used in the `enum`
          // definition.
          Sub("Msg_Enum_annotated", p->LookupVar("Msg_Enum"))
              .AnnotatedAs(ctx.enum_),
      },
      R"cc(
        enum class $Msg_Enum_annotated$ : int {
          $values$,
        };
      )cc");
}

void ScopedEnumStrategy::GenerateSymbolImports(
    io::Printer* p, const EnumStrategyContext& ctx) const {
  if (!ctx.is_nested) {
    return;
  }

  auto v = p->WithVars(ctx.enum_vars);
  p->Emit({Sub("Enum_", p->LookupVar("Enum_")).AnnotatedAs(ctx.enum_)}, R"cc(
    using $Enum_$ = $Msg_Enum$;
  )cc");
}

}  // namespace protobuf
}  // namespace google::compiler::cpp
